#include "stdafx.h"

/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved  */
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/TextLabel.h"
#include "v8datamodel/ImageLabel.h"
#include "V8DataModel/Frame.h"
#include "V8DataModel/DebrisService.h"
#include "V8DataModel/ScreenGui.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/GuiBase2d.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/ScrollingFrame.h"
#include "GfxBase/IAdornableCollector.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "Util/LuaWebService.h"
#include "Script/Script.h"
#include "Script/ScriptContext.h"

#define START_PROJECTION_DISTANCE 50000.0f

DYNAMIC_FASTFLAGVARIABLE(SetCoreDisableNotifications, false)
DYNAMIC_FASTFLAGVARIABLE(SetCoreSendNotifications, false)
DYNAMIC_FASTFLAGVARIABLE(SetCoreMoveChat, false)
DYNAMIC_FASTFLAGVARIABLE(SetCoreDisableChatBar, false)

namespace RBX {

const char* const sBasePlayerGui = "BasePlayerGui";

static const int MAX_ON_SCREEN_MESSAGES = 3;
static const TextService::FontSize ON_SCREEN_MESSAGE_FONT_SIZE = TextService::SIZE_12;
static const int ON_SCREEN_MESSAGE_DISTANCE_TO_RIGHT = 300;
static const int ON_SCREEN_MESSAGE_DISTANCE_TO_BOTTOM = 40;
static const int ON_SCREEN_MESSAGE_DISTANCE_BETWEEN_SLOTS = 20;

BasePlayerGui::BasePlayerGui()
	: Super()
	, adornableCollector(new IAdornableCollector())
	, mouseWasOverGui(false)
{
	defaultSelectionImage = RBX::Creatable<Instance>::create<ImageLabel>();
	defaultSelectionImage->setImage( TextureId("rbxasset://textures/ui/SelectionBox.png") );
	defaultSelectionImage->setBackgroundTransparency(1);
	defaultSelectionImage->setSliceCenter(Rect2D::xyxy(19,19,43,43));
	defaultSelectionImage->setImageScale(GuiObject::SCALE_SLICED);
	defaultSelectionImage->setSize(UDim2(1,14,1,14));
	defaultSelectionImage->setPosition(UDim2(0,-7,0,-7));

	FASTLOG1(FLog::GuiTargetLifetime, "BasePlayerGui created: %p", this);
}

BasePlayerGui::~BasePlayerGui()
{
	FASTLOG1(FLog::GuiTargetLifetime, "BasePlayerGui destroyed: %p", this);
}


bool BasePlayerGui::findModalGuiObject()
{
	boost::shared_ptr<const Instances> c(this->getChildren().read());
	if(c)
	{
		for (Instances::const_iterator iter = c->begin(); iter!=c->end(); ++iter)
		{
			if(ScreenGui* screenGui = Instance::fastDynamicCast<ScreenGui>(iter->get()))
			{
				if(screenGui->hasModalDialog())
					return true;
			}
		}
	}
	return false;
}

void BasePlayerGui::setSelectedObject(GuiObject* value)
{
	shared_ptr<GuiObject> currentSelectedObject = getSelectedObject();
	if (!currentSelectedObject || currentSelectedObject.get() != value)
	{
		if (shared_ptr<GuiObject> lastSelection = selectedGuiObject.lock())
		{
			lastSelection->selectionLostEvent();
		}

		selectedGuiObject = weak_from(value);

		if (shared_ptr<GuiObject> sharedSelection = selectedGuiObject.lock())
		{
			sharedSelection->selectionGainedEvent();
		}

		GuiService* guiService = ServiceProvider::find<GuiService>(this);

		if (!guiService)
		{
			return;
		}

		if (Instance::fastDynamicCast<PlayerGui>(this))
		{
            guiService->raisePropertyChanged(GuiService::prop_selectedGuiObject);
		}
		else if (Instance::fastDynamicCast<CoreGuiService>(this))
		{
            guiService->raisePropertyChanged(GuiService::prop_selectedCoreGuiObject);
		}
	}
}

float getAngleBetween(const Vector2& beginPoint, const Vector2& endPoint, const Vector2& inputDirection)
{
	Vector2 unitCenter = (endPoint - beginPoint);
	unitCenter.unitize();
	Vector2 inputUnitVector(inputDirection);
	inputUnitVector.unitize();
	float dotProduct = inputUnitVector.dot(unitCenter);
	return acosf(dotProduct);
}

void getClosestPoints(float& aPointCenter, float& bPointCenter, const float aSize, const float bSize)
{
	if ( (aPointCenter + aSize/2.0f) >= (bPointCenter - bSize/2.0f) )
	{
		aPointCenter = (aPointCenter + aSize/2.0f);
		bPointCenter = aPointCenter;
	}
	else
	{
		float midPoint = (bPointCenter + aPointCenter)/2.0f;

		if (midPoint <= (aPointCenter + aSize/2.0f))
		{
			aPointCenter = midPoint;
		}
		else
		{
			aPointCenter += aSize/2.0f;
		}

		if (midPoint >= (bPointCenter - bSize/2.0f))
		{
			bPointCenter = midPoint;
		}
		else
		{
			bPointCenter -= bSize/2.0f;
		}
	}
}

bool getClosestGuiPoints(const Rect2D& selectedRect, const Rect2D& guiObjectRect, Vector2& selectedPoint, Vector2& objectPoint)
{
	if (selectedRect.intersectsOrTouches(guiObjectRect))
	{
		return false;
	}

	if (selectedPoint.y < objectPoint.y)
	{
		getClosestPoints(selectedPoint.y, objectPoint.y, selectedRect.height(), guiObjectRect.height());
	}
	else if (selectedPoint.y > objectPoint.y)
	{
		getClosestPoints(objectPoint.y, selectedPoint.y, guiObjectRect.height(), selectedRect.height());
	}

	if (selectedPoint.x < objectPoint.x)
	{
		getClosestPoints(selectedPoint.x, objectPoint.x, selectedRect.width(), guiObjectRect.width());
	}
	else if (selectedPoint.x > objectPoint.x)
	{
		getClosestPoints(objectPoint.x, selectedPoint.x, guiObjectRect.width(), selectedRect.width());
	}

	return true;
}

bool BasePlayerGui::isCloserGuiObject(const Rect2D& currentRect, const Vector2& inputDirection, const GuiObject* guiObjectToTest, 
									  float& minIntersectDist, float& minProjection, const GuiObject* nextSelectionGuiObject)
{
	if (!guiObjectToTest)
	{
		return false;
	}

	const std::string name = guiObjectToTest->getName();
	const Rect2D guiObjectRect = guiObjectToTest->getRect2D();

	Vector2 beginTestPoint = currentRect.center();
	Vector2 endTestPoint = guiObjectRect.center();

	bool isIntersecting = !getClosestGuiPoints(currentRect, guiObjectRect, beginTestPoint, endTestPoint);
	if (!isIntersecting && minIntersectDist == START_PROJECTION_DISTANCE)
	{
		const float maxAngle =  RBX::Math::piHalff()/4.0f;
		float newAngle = getAngleBetween(beginTestPoint, endTestPoint, inputDirection);

		if (!boost::math::isfinite(newAngle) || newAngle > maxAngle)
		{
			return false;
		}

		const Vector2 guiVector = endTestPoint - beginTestPoint;
		const Vector2 largeInputDirection = inputDirection * 100000.0f; // make sure we get an accurate projection onto the input vector

		if (largeInputDirection.length() <= 0.0f)
		{
			return false;
		}

		float projection = (guiVector.dot(largeInputDirection)/largeInputDirection.length());
		if (projection < minProjection)
		{
			minProjection = projection;
			return true;
		}
		else if(projection == minProjection && nextSelectionGuiObject)
		{
			 // we have a tie, let the one with the closest center point win
			 const float testDistance = (currentRect.center() - guiObjectToTest->getRect2D().center()).length();
			 const float nextDistance = (currentRect.center() - nextSelectionGuiObject->getRect2D().center()).length();
			 if (testDistance < nextDistance)
			 {
				 return true;
			 }
		}
	}
	else if (isIntersecting) // rects are intersecting, just compare center points
	{
		const float maxAngle =  RBX::Math::piHalff()/4.0f;
		const Vector2 beginPoint = currentRect.center();
		const Vector2 endPoint = guiObjectRect.center();

		const float newAngle = getAngleBetween(beginPoint, endPoint, inputDirection);

		if (boost::math::isfinite(newAngle) && newAngle <= maxAngle)
		{
			const float intersectionDist = (beginPoint - endPoint).length();
			if (intersectionDist < minIntersectDist)
			{
				minIntersectDist = intersectionDist;
				return true;
			}
		}
	}

	return false;
}

void collectGuiObjects(std::vector<GuiObject*>& guiObjectsToTest, Instance* rootObject)
{
	if (!rootObject)
		return;

	boost::shared_ptr<const Instances> c(rootObject->getChildren().read());
	if(c)
	{
		for (Instances::const_iterator iter = c->begin(); iter!=c->end(); ++iter)
		{
			if(ScreenGui* screenGui = Instance::fastDynamicCast<ScreenGui>(iter->get()))
			{
				screenGui->getGuiObjectsForSelection(guiObjectsToTest);
			}
			collectGuiObjects(guiObjectsToTest, iter->get());
		}
	}
}

GuiObject* checkGuiObjectForNextDirection(const shared_ptr<GuiObject>& guiObject, const Vector2& direction)
{
	if (direction.y > 0.0f && guiObject->getNextSelectionDown() && guiObject->getNextSelectionDown()->isCurrentlyVisible())
	{
		return guiObject->getNextSelectionDown();
	}

	if (direction.y < 0.0f && guiObject->getNextSelectionUp() && guiObject->getNextSelectionUp()->isCurrentlyVisible())
	{
		return guiObject->getNextSelectionUp();
	}

	if (direction.x > 0.0f && guiObject->getNextSelectionRight() && guiObject->getNextSelectionRight()->isCurrentlyVisible())
	{
		return guiObject->getNextSelectionRight();
	}

	if (direction.x < 0.0f && guiObject->getNextSelectionLeft() && guiObject->getNextSelectionLeft()->isCurrentlyVisible())
	{
		return guiObject->getNextSelectionLeft();
	}

	return NULL;
}

GuiObject* BasePlayerGui::checkForTupleSelection(const Vector2& direction, const Rect2D& currentRect,
												 const shared_ptr<const Reflection::Tuple>& selectionTuple)
{
	float minProjection = START_PROJECTION_DISTANCE;
	float minIntersectionDist = minProjection;

	GuiObject* nextSelectedGuiObject = NULL;

	if (selectionTuple && selectionTuple->values.size() > 0)
	{
		Reflection::ValueArray guiObjectValues = selectionTuple->values;
		for (Reflection::ValueArray::iterator tupleIter = guiObjectValues.begin(); tupleIter != guiObjectValues.end(); ++tupleIter)
		{
			if (tupleIter->isType<shared_ptr<Instance> >())
			{
				shared_ptr<Instance> selectableObject = tupleIter->cast<shared_ptr<Instance> >();
				if (GuiObject* selectableGuiObject = Instance::fastDynamicCast<GuiObject>(selectableObject.get()))
				{
					ScrollingFrame* ancestorScrollingFrame = selectableGuiObject->findFirstAncestorOfType<ScrollingFrame>();

					if (selectableGuiObject->isCurrentlyVisible() || (ancestorScrollingFrame && ancestorScrollingFrame->isCurrentlyVisible()))
					{
						if (isCloserGuiObject(currentRect, direction, selectableGuiObject,
							minIntersectionDist, minProjection, nextSelectedGuiObject))
						{
							nextSelectedGuiObject = selectableGuiObject;
						}
					}
				}
			}
		}
	}

	return nextSelectedGuiObject;
}

GuiObject* BasePlayerGui::checkForDefaultGuiSelection(const Vector2& direction, const Rect2D& currentRect, 
													  const shared_ptr<Instance> selectionAncestor)
{
	float minProjection = START_PROJECTION_DISTANCE;
	float minIntersectionDist = minProjection;

	GuiObject* nextSelectedGuiObject = NULL;

	std::vector<GuiObject*> guiObjectsToTest;
	collectGuiObjects(guiObjectsToTest, this);

	for (std::vector<GuiObject*>::iterator iter = guiObjectsToTest.begin(); iter != guiObjectsToTest.end(); ++iter)
	{
		if (selectionAncestor)
		{
			if ( ((*iter) == selectionAncestor.get() || (*iter)->isDescendantOf2(selectionAncestor)) && 
				isCloserGuiObject(currentRect, direction, (*iter),
									minIntersectionDist, minProjection, nextSelectedGuiObject))
			{
				nextSelectedGuiObject = (*iter);
			}
		}
		else if (isCloserGuiObject(currentRect, direction, (*iter), 
									minIntersectionDist, minProjection, nextSelectedGuiObject))
		{
			nextSelectedGuiObject = (*iter);
		}
	}

	return nextSelectedGuiObject;
}

GuiObject* BasePlayerGui::selectNewGuiObject(const Vector2& direction)
{
	GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this);
	if (!guiService)
	{
		return NULL;
	}

	Rect2D currentRect =  Rect2D::xywh(Vector2::zero(), Vector2::zero());
	GuiService::SelectionGroupPair selectionGroup;

	const shared_ptr<GuiObject> sharedSelectedGuiObject = selectedGuiObject.lock();

	if (sharedSelectedGuiObject)
	{
		currentRect = sharedSelectedGuiObject->getRect2D();
		selectionGroup = guiService->getSelectedObjectGroup(sharedSelectedGuiObject);
	}
	else
	{
		return NULL;
	}

	GuiObject* closestGuiObject = NULL;

	if (GuiObject* newSelectedObject = checkGuiObjectForNextDirection(sharedSelectedGuiObject, direction))
	{
		closestGuiObject = newSelectedObject;
	}
	
	if (!closestGuiObject)
	{
		shared_ptr<const Reflection::Tuple> selectionTuple = selectionGroup.second;

		if (selectionTuple)
		{
			if (selectionTuple->values.size() > 0)
			{
				closestGuiObject = checkForTupleSelection(direction, currentRect, selectionGroup.second);
			}
		}
		else
		{
			closestGuiObject = checkForDefaultGuiSelection(direction, currentRect, selectionGroup.first.lock());
		}
	}

	if (closestGuiObject)
	{
		setSelectedObject(closestGuiObject);
	}

	return closestGuiObject;
}

bool BasePlayerGui::scriptShouldRun(BaseScript* script)
{
	RBXASSERT(isAncestorOf(script));

	bool answer = false;

	if (script->fastDynamicCast<LocalScript>())
	{
		{
			//Either we're in the old mode (LocalScripts run on Client), or this script is elligible to be run ClientSide
			RBX::Network::Player* localPlayer = Network::Players::findLocalPlayer(this);
			answer = (localPlayer && (getParent() == localPlayer));
			if(answer){
				script->setLocalPlayer(shared_from(localPlayer));
			}
		}
	}
	else
	{
		answer = Network::Players::backendProcessing(this, false);		// modified by db 12/2/09 - this is called when BaseScript is not in the workspace at times
	}

	return answer;
}

bool BasePlayerGui::askAddChild(const Instance* instance) const
{
	return true;
}

void BasePlayerGui::onDescendantAdded(Instance* instance)
{
	Super::onDescendantAdded(instance);
	
	if (IAdornable* iR = dynamic_cast<IAdornable*>(instance)) {
		adornableCollector->onRenderableDescendantAdded(iR);
	}
}

void BasePlayerGui::onDescendantRemoving(const shared_ptr<Instance>& instance)
{
	if (IAdornable* iR = dynamic_cast<IAdornable*>(instance.get())) {
		adornableCollector->onRenderableDescendantRemoving(iR);
	}

	Super::onDescendantRemoving(instance);
}

// all descendants
void BasePlayerGui::render3dAdorn(Adorn* adorn)
{
	adornableCollector->render3dAdornItems(adorn);
}


void BasePlayerGui::append3dSortedAdorn(std::vector<AdornableDepth>& sortedAdorn, const Camera* camera) const
{
	adornableCollector->append3dSortedAdornItems(sortedAdorn, camera);
}

void BasePlayerGui::tryRenderSelectionFrame(Adorn* adorn)
{
	if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
	{
		if (GuiObject* selectedGuiObject = guiService->getSelectedGuiObject())
		{
			if (selectedGuiObject->isDescendantOf(this) && selectedGuiObject->isCurrentlyVisible())
			{
				if (GuiObject* imageObject = selectionImageObject.get())
				{
					selectedGuiObject->renderSelectionFrame(adorn, imageObject);
				}
				else
				{
					selectedGuiObject->renderSelectionFrame(adorn, defaultSelectionImage.get());
				}
			}
		}
	}
}

void BasePlayerGui::render2d(Adorn* adorn)
{
	adornableCollector->render2dItems(adorn);
	tryRenderSelectionFrame(adorn);
}

GuiResponse BasePlayerGui::processChildren(boost::function<GuiResponse(GuiBase2d*)> func)
{
    if (this->getChildren())
    {
        boost::shared_ptr<const Instances> c(this->getChildren().read());
        Instances::const_iterator end = c->end();
        for (Instances::const_iterator iter = c->begin(); iter!=end; ++iter)
        {
            if (GuiBase2d* guiBase = fastDynamicCast<GuiBase2d>((*iter).get()))
            {
                GuiResponse answer = func(guiBase);
                if(answer.wasSunk())
                    return answer;
            }
        }
    }
    
    return GuiResponse::notSunk();
}
    
GuiResponse BasePlayerGui::processInputOnChild(GuiBase2d* guiBase, const shared_ptr<InputObject>& event)
{
    GuiResponse answer = guiBase->process(event);
    
    if (answer.wasSunk() || answer.getMouseWasOver())
    {
        mouseWasOverGui = true;
    }
    
    return answer;
}
    
GuiResponse BasePlayerGui::process(const shared_ptr<InputObject>& event)
{
	mouseWasOverGui = false;
	Instance* mouseOverGuiBase = NULL;

	boost::shared_ptr<const Instances> c(this->getChildren().read());
	if (c)
	{
		Instances::const_reverse_iterator rend = c->rend();

		for (Instances::const_reverse_iterator iter = c->rbegin(); iter!=rend; ++iter)
		{
			if (GuiBase* guiBase = fastDynamicCast<GuiBase>((*iter).get())) 
			{
				GuiResponse answer = guiBase->process(event);
				if (answer.wasSunk())
				{
					answer.setMouseWasOver();
					return answer;
				}
				if (answer.getMouseWasOver()) 
				{
					mouseOverGuiBase = answer.getTarget().get();
					mouseWasOverGui = true;
				}
			}
		}
	}

	GuiResponse answer = mouseWasOverGui ? GuiResponse::notSunkMouseWasOver() : GuiResponse::notSunk();
	answer.setTarget(mouseOverGuiBase);
	return answer;
}
    
GuiResponse BasePlayerGui::processGestureOnChild(GuiBase2d* guiBase, const UserInputService::Gesture gesture, shared_ptr<const RBX::Reflection::ValueArray> touchPositions, shared_ptr<const Reflection::Tuple> args)
{
    return guiBase->processGesture(gesture, touchPositions, args);
}
    
GuiResponse BasePlayerGui::processGesture(const UserInputService::Gesture gesture, shared_ptr<const RBX::Reflection::ValueArray> touchPositions, shared_ptr<const Reflection::Tuple> args)
{
    GuiResponse answer = GuiResponse::notSunk();
    
    if (this->getChildren())
    {
        boost::shared_ptr<const Instances> c(this->getChildren().read());
        Instances::const_iterator end = c->end();
        for (Instances::const_iterator iter = c->begin(); iter!=end; ++iter)
        {
            if (GuiBase2d* guiBase = fastDynamicCast<GuiBase2d>((*iter).get()))
            {
                answer = processGestureOnChild(guiBase, gesture, touchPositions, args);
                if (answer.wasSunk() || answer.getMouseWasOver())
                {
                    return answer;
                }
            }
        }
    }
    
    return answer;
}


REFLECTION_BEGIN();
static Reflection::RefPropDescriptor<PlayerGui, GuiObject> prop_PlayerGuiSelectionImageObject("SelectionImageObject", category_Appearance, &PlayerGui::getSelectionImageObject, &PlayerGui::setSelectionImageObject, Reflection::PropertyDescriptor::STANDARD);
static const Reflection::BoundFuncDesc<PlayerGui, void(float)> func_setTopbarTransparency(&PlayerGui::setTopbarTransparency, "SetTopbarTransparency", "transparency", Security::None);
static const Reflection::BoundFuncDesc<PlayerGui, float(void)> func_getTopbarTransparency(&PlayerGui::getTopbarTransparency, "GetTopbarTransparency", Security::None);
static Reflection::EventDesc<PlayerGui, void(float)> event_topbarTransparencyChangedSignal(&PlayerGui::topbarTransparencyChangedSignal, "TopbarTransparencyChangedSignal", "transparency", Security::None);
REFLECTION_END();

const char* const sPlayerGui = "PlayerGui";
PlayerGui::PlayerGui()
: Super(),
    topbarTransparency(0.5f)
{
	setName("PlayerGui");

}

void PlayerGui::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (TeleportService* ts = ServiceProvider::create<TeleportService>(newProvider))
	{
		if (ts->getTempCustomTeleportLoadingGui() && TeleportService::didTeleport())
		{
			if (DataModel::get(newProvider)->getCreatorID() == TeleportService::getPreviousCreatorId() && DataModel::get(newProvider)->getCreatorType() == TeleportService::getPreviousCreatorType())
			{
				if (RBX::ScreenGui* loadingGui = Instance::fastDynamicCast<RBX::ScreenGui>(TeleportService::getCustomTeleportLoadingGui().get()))
				{
					loadingGui->setParent(this);
				}
			}
			ts->getTempCustomTeleportLoadingGui()->destroy();
		}
	}
	Super::onServiceProvider(oldProvider, newProvider);
}

bool PlayerGui::askSetParent(const Instance* parent) const
{
	return Instance::fastDynamicCast<RBX::Network::Player>(parent)!=NULL;
}
bool PlayerGui::askForbidParent(const Instance* parent) const
{
	return !askSetParent(parent);
}

void PlayerGui::setTopbarTransparency(float transparency)
{
    if(RBX::Network::Players::frontendProcessing(this))
    {
        if (G3D::isFinite(transparency))
        {
			transparency = G3D::clamp(transparency, 0.0f, 1.0f);

            if (topbarTransparency != transparency)
            {
                topbarTransparency = transparency;
                topbarTransparencyChangedSignal(topbarTransparency);
            }
        }
    }
    else
    {
        throw std::runtime_error("PlayerGui:SetTopbarTransparency can only be set from a local script.");
    }
}

float PlayerGui::getTopbarTransparency(void)
{
    if(!RBX::Network::Players::frontendProcessing(this))
    {
        throw std::runtime_error("PlayerGui:GetTopbarTransparency should only be accessed from a local script.");
    }
    return topbarTransparency;
}

void PlayerGui::setSelectionImageObject(GuiObject* value)
{
	if (value != selectionImageObject.get())
	{
		selectionImageObject = shared_from(value);
		raisePropertyChanged(prop_PlayerGuiSelectionImageObject);
	}
}

const char *const sStarterGuiService = "StarterGui";

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<StarterGuiService, bool> prop_showGui("ShowDevelopmentGui", category_Data, &StarterGuiService::getShowGui, &StarterGuiService::setShowGui);
const Reflection::PropDescriptor<StarterGuiService, bool> StarterGuiService::prop_ResetPlayerGui("ResetPlayerGuiOnSpawn", category_Data, &StarterGuiService::getResetPlayerGui, &StarterGuiService::setResetPlayerGui, Reflection::PropertyDescriptor::STANDARD, Security::None);

static const Reflection::BoundFuncDesc<StarterGuiService, void(StarterGuiService::CoreGuiType, bool)> func_setCoreGuiEnabled(&StarterGuiService::setCoreGuiEnabled,"SetCoreGuiEnabled", "coreGuiType", "enabled",  Security::None);
static const Reflection::BoundFuncDesc<StarterGuiService, bool(StarterGuiService::CoreGuiType)> func_getCoreGuiEnabled(&StarterGuiService::getCoreGuiEnabled,"GetCoreGuiEnabled", "coreGuiType", Security::None);
static Reflection::EventDesc<StarterGuiService, void(StarterGuiService::CoreGuiType,bool)> event_coreGuiChangedSignal(&StarterGuiService::coreGuiChangedSignal, "CoreGuiChangedSignal", "coreGuiType", "enabled", Security::RobloxScript);
static const Reflection::BoundFuncDesc<StarterGuiService, void(std::string, Lua::WeakFunctionRef)> func_registerSetCore(&StarterGuiService::registerSetCore,"RegisterSetCore", "parameterName", "setFunction", Security::RobloxScript);
static const Reflection::BoundFuncDesc<StarterGuiService, void(std::string, Lua::WeakFunctionRef)> func_registerGetCore(&StarterGuiService::registerGetCore,"RegisterGetCore", "parameterName", "getFunction", Security::RobloxScript);
static const Reflection::BoundFuncDesc<StarterGuiService, void(std::string, Reflection::Variant)> func_setCore(&StarterGuiService::setCore,"SetCore", "parameterName", "value", Security::None);
static const Reflection::BoundYieldFuncDesc<StarterGuiService, Reflection::Variant(std::string)> func_getCore(&StarterGuiService::getCore, "GetCore", "parameterName", Security::None);
REFLECTION_END();

namespace Reflection 
{
	template<>
	EnumDesc<StarterGuiService::CoreGuiType>::EnumDesc():EnumDescriptor("CoreGuiType")
	{
		addPair(StarterGuiService::COREGUI_PLAYERLISTGUI, "PlayerList");
		addPair(StarterGuiService::COREGUI_HEALTHGUI, "Health");
		addPair(StarterGuiService::COREGUI_BACKPACKGUI, "Backpack");
		addPair(StarterGuiService::COREGUI_CHATGUI, "Chat");
		addPair(StarterGuiService::COREGUI_ALL, "All");
	}

	template<>
	StarterGuiService::CoreGuiType& Variant::convert<StarterGuiService::CoreGuiType>(void)
	{
		return genericConvert<StarterGuiService::CoreGuiType>();
	}
} // namespace Reflection

template<>
bool StringConverter<StarterGuiService::CoreGuiType>::convertToValue(const std::string& text, StarterGuiService::CoreGuiType& value)
{
	return Reflection::EnumDesc<StarterGuiService::CoreGuiType>::singleton().convertToValue(text.c_str(),value);
}

StarterGuiService::StarterGuiService()
	:showGui(true),
    resetPlayerGui(true)
{
	Instance::setName(sStarterGuiService);

	for(int i = COREGUI_PLAYERLISTGUI; i <= COREGUI_ALL; ++i)
		coreGuiEnabledState[static_cast<StarterGuiService::CoreGuiType>(i)] = true;
}

void StarterGuiService::setCoreGuiEnabled(CoreGuiType type, bool enabled)
{
	if(!RBX::Network::Players::frontendProcessing(this))
		StandardOut::singleton()->print(MESSAGE_WARNING, "StarterGui:SetCoreGuiEnabled called from server script.  Did you mean to use a local script?");

	// setting the all state resets all keys to the all state
	if (type == COREGUI_ALL)
	{
		for(int i = COREGUI_PLAYERLISTGUI; i <= COREGUI_ALL; ++i)
			coreGuiEnabledState[static_cast<CoreGuiType>(i)] = enabled;
	}
	else
	{
		coreGuiEnabledState[type] = enabled;

		// check all other states to see if we have all same states
		// if we do, set COREGUI_ALL key to be true or false
		bool lastBool = enabled;
		for (int i = COREGUI_PLAYERLISTGUI; i < COREGUI_ALL; ++i)
		{
			CoreGuiType iteratedType = static_cast<CoreGuiType>(i);
			if (lastBool != coreGuiEnabledState[iteratedType])
			{
				coreGuiEnabledState[COREGUI_ALL] = false;
				lastBool = coreGuiEnabledState[iteratedType];
				break;
			}
		}

		if(lastBool == enabled)
			coreGuiEnabledState[COREGUI_ALL] = lastBool;
	}

	coreGuiChangedSignal(type,enabled);
}

bool StarterGuiService::getCoreGuiEnabled(CoreGuiType type)
{
	if(!RBX::Network::Players::frontendProcessing(this))
		StandardOut::singleton()->print(MESSAGE_WARNING, "StarterGui:GetCoreGuiEnabled called from server script.  Did you mean to use a local script?");

	return coreGuiEnabledState[type];
}

void StarterGuiService::registerSetCore(std::string parameterName, Lua::WeakFunctionRef setFunction)
{
	setCoreFunctions[parameterName] = setFunction;
}

void StarterGuiService::registerGetCore(std::string parameterName, Lua::WeakFunctionRef getFunction)
{
	getCoreFunctions[parameterName] = getFunction;
}

void StarterGuiService::setCore(std::string parameterName, Reflection::Variant value)
{
	FunctionMap::iterator iter = setCoreFunctions.find(parameterName);
	if (iter != setCoreFunctions.end())
	{
		Lua::WeakFunctionRef setFunction = (*iter).second;
		if (Lua::ThreadRef threadRef = setFunction.lock())
		{
			if (ScriptContext* sc = ServiceProvider::create<ScriptContext>(this))
			{
				try
				{
					Reflection::Tuple args = Reflection::Tuple();
					args.values.push_back(value);
					sc->callInNewThread(setFunction, args);
				}
				catch (RBX::base_exception& e)
				{
					StandardOut::singleton()->printf(MESSAGE_ERROR, "SetCore: Unexpected error while invoking callback: %s", e.what());
				}
			}
		}
	}
	else
	{
		throw RBX::runtime_error("SetCore: %s has not been registered by the CoreScripts", parameterName.c_str());
	}
}

void StarterGuiService::getCore(std::string parameterName,  boost::function<void(Reflection::Variant)> resumeFunction, boost::function<void(std::string)> errorFunction)
{
	FunctionMap::iterator iter = getCoreFunctions.find(parameterName);
	if (iter != getCoreFunctions.end())
	{
		Lua::WeakFunctionRef getFunction = (*iter).second;
		if (Lua::ThreadRef threadRef = getFunction.lock())
		{
			if (ScriptContext* sc = ServiceProvider::create<ScriptContext>(this))
			{
				Reflection::Tuple result = Reflection::Tuple();
				Reflection::Tuple args = Reflection::Tuple();
				try
				{
					result = sc->callInNewThread(getFunction, args);
				}
				catch (RBX::base_exception& e)
				{
					errorFunction(RBX::format("GetCore: Unexpected error while invoking callback: %s", e.what()));
				}
				if (result.values.size() > 0)
				{
					resumeFunction(result.values[0].get<Reflection::Variant>());
				}
				else
				{
					errorFunction(RBX::format("GetCore: CoreScript function %s did not return a result", parameterName.c_str()));
				}
			}
		}
	}
	else
	{
		errorFunction(RBX::format("GetCore: %s has not been registered by the CoreScripts", parameterName.c_str()));
	}
}

void StarterGuiService::setShowGui(bool value)
{
	if(showGui != value){
		showGui = value;
		raisePropertyChanged(prop_showGui);
	}
}

void StarterGuiService::setResetPlayerGui(bool value)
{
	if(resetPlayerGui != value)
	{
		resetPlayerGui = value;
		raisePropertyChanged(prop_ResetPlayerGui);
	}
}

void StarterGuiService::render2d(Adorn* adorn)
{
	if(showGui){
		Super::render2d(adorn);
	}
}
void StarterGuiService::render3dAdorn(Adorn* adorn)
{
	if(showGui){
		Super::render3dAdorn(adorn);
	}
}
void StarterGuiService::append3dSortedAdorn(std::vector<AdornableDepth>& sortedAdorn, const Camera* camera) const
{
	if(showGui){
		Super::append3dSortedAdorn(sortedAdorn, camera);
	}
}
GuiResponse StarterGuiService::process(const shared_ptr<InputObject>& event)
{
	if(showGui){
		return Super::process(event);
	}
	return GuiResponse::notSunk();
}

const char *const sCoreGuiService = "CoreGui"; 

REFLECTION_BEGIN();
static Reflection::RefPropDescriptor<CoreGuiService, GuiObject> prop_CoreSelectionImageObject("SelectionImageObject", category_Appearance, &CoreGuiService::getSelectionImageObject, &CoreGuiService::setSelectionImageObject, Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
static const Reflection::PropDescriptor<CoreGuiService, int> prop_Version("Version", category_Data, &CoreGuiService::getGuiVersion, NULL);
REFLECTION_END();

CoreGuiService::CoreGuiService()
{
	setRobloxLocked(true);
	Instance::setName(sCoreGuiService);

	onScreenMessages.resize(MAX_ON_SCREEN_MESSAGES);
}

int CoreGuiService::getGuiVersion() const
{
	return 0;
}
void CoreGuiService::createRobloxScreenGui()
{
	screenGui = Creatable<RBX::Instance>::create<ScreenGui>();
	screenGui->setName("RobloxGui");
	screenGui->setRobloxLocked(true);
	screenGui->setParent(this);
}

shared_ptr<RBX::ScreenGui> CoreGuiService::getRobloxScreenGui()
{
    if (!screenGui)
    {
        createRobloxScreenGui();
    }
    
    if ( RBX::ScreenGui* theScreenGui = Instance::fastDynamicCast<RBX::ScreenGui>(screenGui.get()) )
    {
        return shared_from(theScreenGui);
    }
    
    return shared_ptr<RBX::ScreenGui>();
}

void CoreGuiService::onDescendantAdded(Instance* instance)
{
	if(instance)
    {
		instance->setRobloxLocked(true);
    }

	Super::onDescendantAdded(instance);
}

void CoreGuiService::addChild(RBX::Instance* instance)
{
	if(instance)
		instance->setParent(screenGui.get());
}

void CoreGuiService::removeChild(RBX::Instance* instance)
{
	if(screenGui->getChildren())
	{
		Instances::const_iterator end = screenGui->getChildren()->end();
		for (Instances::const_iterator iter = screenGui->getChildren()->begin(); iter != end; ++iter)
		{
			if(iter->get() == instance)
			{
				iter->get()->remove();
				return;
			}
		}
	}
}

void CoreGuiService::removeChild(const std::string &Name)
{
	if(screenGui->getChildren())
	{
		Instances::const_iterator end = screenGui->getChildren()->end();
		for (Instances::const_iterator iter = screenGui->getChildren()->begin(); iter != end; ++iter)
		{
			if(iter->get()->getName() == Name)
			{
				iter->get()->remove();
				return;
			}
		}
	}
}

Instance* CoreGuiService::findGuiChild(const std::string &Name)
{
	if(screenGui->getChildren())
	{
		Instances::const_iterator end = screenGui->getChildren()->end();
		for (Instances::const_iterator iter = screenGui->getChildren()->begin(); iter != end; ++iter)
		{
			if(iter->get()->getName() == Name)
				return iter->get();
		}
	}
	return NULL;
}

void CoreGuiService::setGuiVisibility(bool visible)
{
	if(screenGui->getChildren())
	{
		Instances::const_iterator end = screenGui->getChildren()->end();
		for (Instances::const_iterator iter = screenGui->getChildren()->begin(); iter != end; ++iter)
			if(RBX::GuiObject* obj = Instance::fastDynamicCast<GuiObject>(iter->get()))
				obj->setVisible(visible);
	}
}

void CoreGuiService::displayOnScreenMessage(int slot, const std::string &message, double duration)
{
	if (slot >= MAX_ON_SCREEN_MESSAGES)
		return;

	clearOnScreenMessage(slot);

	Rect2D rect = screenGui->fastDynamicCast<RBX::GuiBase2d>()->getRect2D();

	onScreenMessages[slot] = Creatable<RBX::Instance>::create<TextLabel>();

	RBX::Frame* bottomRightControl = Instance::fastDynamicCast<RBX::Frame>(screenGui->findFirstChildByName2("BottomRightControl",true).get());
	if(bottomRightControl)
		onScreenMessages[slot]->setParent(bottomRightControl);
	else
		onScreenMessages[slot]->setParent(screenGui.get());

	if (RBX::TextLabel *textLabel = Instance::fastDynamicCast<RBX::TextLabel>(onScreenMessages[slot].get()))
	{
		textLabel->setFontSize(ON_SCREEN_MESSAGE_FONT_SIZE);
		textLabel->setTextColor3(Color3::white());
		textLabel->setText(message);
		
		if(bottomRightControl)
		{
			onScreenMessages[slot]->setParent(bottomRightControl);
			textLabel->setSize(RBX::UDim2(0,textLabel->getTextBounds().x,1,0));
			textLabel->setPosition(RBX::UDim2(1, -textLabel->getTextBounds().x, -1.05 + (-slot - 0.5), 0));
		}
		else
		{
			onScreenMessages[slot]->setParent(screenGui.get());
			textLabel->setSize(RBX::UDim2(0,300,0,30));
			textLabel->setPosition(RBX::UDim2(1, (int)rect.x1() - ON_SCREEN_MESSAGE_DISTANCE_TO_RIGHT,
												0, (int)rect.y1() - ON_SCREEN_MESSAGE_DISTANCE_TO_BOTTOM - ON_SCREEN_MESSAGE_DISTANCE_BETWEEN_SLOTS * slot));
		}

		textLabel->setBackgroundColor3(Color3::black());
		textLabel->setBackgroundTransparency(0.5f);
		textLabel->setBorderSizePixel(0);
	}

	if (duration != 0)
	{
		RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);
		RBX::DebrisService *ds = RBX::ServiceProvider::find<RBX::DebrisService>(this);
		ds->addItem(onScreenMessages[slot], duration);
	}
}

void CoreGuiService::clearOnScreenMessage(int slot)
{
	if (slot >= MAX_ON_SCREEN_MESSAGES)
		return;

	if(shared_ptr<Instance> instance = onScreenMessages[slot]){
		RBX::Security::Impersonator impersonate(RBX::Security::LocalGUI_);
		RBX::DebrisService *ds = RBX::ServiceProvider::find<RBX::DebrisService>(this);
		ds->addItem(instance, 0);
	}
}

void CoreGuiService::setSelectionImageObject(GuiObject* value)
{
	if (value != selectionImageObject.get())
	{
		selectionImageObject = shared_from(value);
		raisePropertyChanged(prop_CoreSelectionImageObject);
	}
}
    
} // Namespace
