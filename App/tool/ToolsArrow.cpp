/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Tool/ToolsArrow.h"
#include "Tool/DragTool.h"
#include "Tool/AdvDragTool.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Camera.h"
#include "Tool/Dragger.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "AppDraw/Draw.h"
#include "GfxBase/Adorn.h"
#include "Util/Sound.h"
#include "Util/UserInputBase.h"
#include "Util/Selectable.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/ToolsSurface.h"
#include "v8datamodel/changehistory.h"
#include "v8datamodel/UserInputService.h"

FASTFLAGVARIABLE(StudioDE6194FixEnabled, false)

DYNAMIC_FASTFLAGVARIABLE(RestoreTransparencyOnToolChange, false)

namespace RBX {

const char* const sBoxSelectCommand	= "BoxSelect";
const char* const sAdvArrowTool  	= "AdvArrow";

bool ArrowToolBase::showDraggerGrid = false;

DRAG::DraggerGridMode AdvArrowToolBase::advGridMode = DRAG::ONE_STUD;
bool AdvArrowToolBase::advManualJointMode = true;
DRAG::ManualJointType AdvArrowToolBase::advManualJointType = DRAG::STRONG_MANUAL_JOINT;
bool AdvArrowToolBase::advManipInProgress = false;

bool AdvArrowToolBase::advCollisionCheckMode = true;

bool AdvArrowToolBase::advLocalTranslationMode = false;
bool AdvArrowToolBase::advLocalRotationMode = true;
    
bool AdvArrowToolBase::advCreateJointsMode = true;
AdvArrowToolBase::PartsTransparencyCollection AdvArrowToolBase::originalPartsTransparency;

void ArrowToolBase::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	onMouseIdle(inputObject);
}

void ArrowToolBase::onMouseIdle(const shared_ptr<InputObject>& inputObject)
{
	overInstance = getUnlockedPart(inputObject);

	// capture state of the alt key, we need it when rendering the adornments
	if(UserInputService* userInputService = ServiceProvider::find<UserInputService>(workspace))
		altKeyDown = userInputService->isAltDown();
}

const std::string ArrowToolBase::getCursorName() const
{
	return overInstance ? "DragCursor" : "ArrowCursor";
}

Decal* ArrowToolBase::findDecal(PartInstance* p, const shared_ptr<InputObject>& inputObject)
{
	// try to grab decal if there is one and this part is selected already
	int surfId;
	Surface s = getSurface(workspace, inputObject, NULL, p, surfId);
	if (s.getPartInstance())
	{
		if (p->getChildren())
		{
			Instances::const_iterator end = p->getChildren()->end();
			for (Instances::const_iterator iter = p->getChildren()->begin(); iter != end; ++iter)
			{
				if (Decal* d = Instance::fastDynamicCast<Decal>(iter->get()))
				{
					if (d->getFace() == s.getNormalId()) // the decal is on the selected face
					{
						return d;
					}
				}
			}
		}
	}
	return NULL;
}

shared_ptr<MouseCommand> ArrowToolBase::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	RBXASSERT(inputObject->isLeftMouseDownEvent());
	Vector3 hitWorld;

	PartInstance* frontPart = getUnlockedPart(inputObject, hitWorld);

	if (frontPart) 
	{
        Instance* selectable3d;

		UserInputService* userInputService = ServiceProvider::find<UserInputService>(workspace);
		bool isAltKeyDown = false;

		if(userInputService)
			isAltKeyDown = userInputService->isAltDown();

        if ( isAltKeyDown )
            selectable3d = frontPart;
        else
            selectable3d = getTopSelectable3d(frontPart);

		ServiceClient< Selection > selection(workspace);

		if ( userInputService->isShiftDown() || userInputService->isCtrlDown() )
		{
			if (selection->isSelected(selectable3d))
				selection->removeFromSelection(selectable3d);
			else									
				selection->addToSelection(selectable3d);
		}
		else
		{
			shared_ptr<Instance> selectIfNoDrag;

			if (selectable3d && selection->isSelected(selectable3d)) 
				selectIfNoDrag = shared_from<Instance>(findDecal(frontPart, inputObject));
			else if(selectable3d)
				selection->setSelection(selectable3d);

			ServiceClient< FilteredSelection<Instance> > instanceSelection(workspace);
			return DragTool::onMouseDown(	
                frontPart,
				hitWorld,
				instanceSelection->items(),
				inputObject,
				workspace,
				selectIfNoDrag );
		}
	}
	else
    {
		shared_ptr<MouseCommand> boxSelect = Creatable<MouseCommand>::create<BoxSelectCommand>(workspace);
		return boxSelect->onMouseDown(inputObject);
	}

	return shared_ptr<MouseCommand>();
}

shared_ptr<MouseCommand> ArrowToolBase::onPeekKeyDown(const shared_ptr<InputObject>& inputObject)
{
#ifdef STUDIO_CAMERA_CONTROL_SHORTCUTS
	RBXASSERT(inputObject->isKeyDownEvent());
	switch(inputObject->getKeyCode())
	{
	case SDLK_f: // we want to zoom in on any selected object
		if(UserInputService* userInputService = ServiceProvider::find<UserInputService>(workspace))
			if( userInputService->isCtrlDown() && !workspace->getCamera()->isCharacterCamera() )
				if(Verb* zoomExtentsVerb = workspace->getWhitelistVerb("Camera", "Zoom", "Extents"))
					zoomExtentsVerb->doIt(&workspace->getDataState());
		break;
	case SDLK_LEFTBRACKET: // take us back one in our camera history
		if(Camera* camera = workspace->getCamera())
			camera->stepCameraHistoryBackward();
		break;
	case SDLK_RIGHTBRACKET: // take us forward one in our camera history
		if(Camera* camera = workspace->getCamera())
			camera->stepCameraHistoryForward();
		break;
    default:
        break;
	}
#endif

	return Super::onPeekKeyDown(inputObject);
}

void ArrowToolBase::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);
    renderHoverOver(adorn,false);
}

/**
 * Renders a hover over selection box when the mouse cursor is over an instance.
 *  When the drill down key is held, the part under the cursor is displayed rather than
 *  the top level instance.
 * 
 * @param   adorn           used to render the selection box
 * @param   drillDownOnly   will only display box if the drill down key is held down
 */
void ArrowToolBase::renderHoverOver(Adorn* adorn,bool drillDownOnly)
{
    if ( overInstance && (!drillDownOnly || (drillDownOnly && altKeyDown)) )
    {
        Instance* instance;
        
        if ( altKeyDown )
            instance = overInstance;
        else
            instance = getTopSelectable3d(overInstance);

        if ( instance )
        {
            IAdornable* adornable = dynamic_cast<IAdornable*>(instance);
            if ( adornable )
                adornable->render3dSelect(adorn,SELECT_HOVER);
        }
    }
}

AdvArrowToolBase::JointCreationMode AdvArrowToolBase::getJointCreationMode()
{
	if (advCreateJointsMode)
	{
		if (advManualJointMode)
			return AdvArrowToolBase::WELD_ALL;

		return AdvArrowToolBase::SURFACE_JOIN_ONLY;
	}
	return AdvArrowToolBase::NO_JOIN;
}

void AdvArrowToolBase::restoreSavedPartsTransparency()
{
	for (PartsTransparencyCollection::const_iterator it = originalPartsTransparency.begin(); it != originalPartsTransparency.end(); ++it)
	{
		if (shared_ptr<PartInstance> part = it->first.lock())
			part->setTransparency(it->second);
	}

	originalPartsTransparency.clear();
}

void AdvArrowToolBase::savePartTransparency(shared_ptr<PartInstance> part)
{
	originalPartsTransparency[part] = part->getTransparencyUi();
	part->setTransparency(0.5);
}


namespace Reflection
{
	template<> EnumDesc<AdvArrowToolBase::JointCreationMode>::EnumDesc()
		:EnumDescriptor("JointCreationMode")
	{
		addPair(AdvArrowToolBase::WELD_ALL, "All");
		addPair(AdvArrowToolBase::SURFACE_JOIN_ONLY, "Surface");
		addPair(AdvArrowToolBase::NO_JOIN, "None");
	}

	template<>
	AdvArrowToolBase::JointCreationMode& Variant::convert<AdvArrowToolBase::JointCreationMode>(void)
	{
		return genericConvert<AdvArrowToolBase::JointCreationMode>();
	}
}

template<>
bool StringConverter<AdvArrowToolBase::JointCreationMode>::convertToValue(const std::string& text, AdvArrowToolBase::JointCreationMode& value)
{
	return Reflection::EnumDesc<AdvArrowToolBase::JointCreationMode>::singleton().convertToValue(text.c_str(),value);
}

/*****************************************************************************/
// Advanced Arrow Tool
/*****************************************************************************/

const std::string AdvArrowToolBase::getCursorName() const
{
	return overInstance ? "advCursor-openedHand" : "advCursor-default";
}

shared_ptr<MouseCommand> AdvArrowToolBase::onKeyDown(const shared_ptr<InputObject>& inputObject)
{
	switch (inputObject->getKeyCode())
	{
	case SDLK_g: // _G_rid mode cycle (1-stud -> 1/5-stud -> off -> 1-stud ->...)
		{
			switch(advGridMode)
			{
			case DRAG::ONE_STUD:
				{
					advGridMode = DRAG::QUARTER_STUD;
					break;
				}
			case DRAG::QUARTER_STUD:
				{
					advGridMode = DRAG::OFF;
					break;
				}
			case DRAG::OFF:
				{
					advGridMode = DRAG::ONE_STUD;
					break;
				}
			}
		}
		break;
	case SDLK_j:
		{
			if(AdvArrowToolBase::advManualJointMode)
				AdvArrowToolBase::advManualJointMode = false;
			else
				AdvArrowToolBase::advManualJointMode = true;
		}
		break;
    default:
        break;
	}
#ifdef STUDIO_CAMERA_CONTROL_SHORTCUTS
	return Super::onKeyDown(inputObject);
#else
	return shared_ptr<MouseCommand>();
#endif
}

shared_ptr<MouseCommand> AdvArrowToolBase::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
    RBXASSERT(inputObject->isLeftMouseDownEvent());
	Vector3 hitWorld;

	PartInstance* frontPart = getUnlockedPart(inputObject, hitWorld);

	if (frontPart) 
	{
        Instance* selectable3d;

		UserInputService* userInputService = ServiceProvider::find<UserInputService>(workspace);

        if ( userInputService->isAltDown() )
            selectable3d = frontPart;
        else
            selectable3d = getTopSelectable3d(frontPart);

		ServiceClient< Selection > selection(workspace);

		if ( userInputService->isCtrlDown() || userInputService->isShiftDown() )
		{
			if (selection->isSelected(selectable3d))
				selection->removeFromSelection(selectable3d);
			else									
				selection->addToSelection(selectable3d);
		}
		else
		{
			shared_ptr<Instance> selectIfNoDrag;

			if (selectable3d && selection->isSelected(selectable3d)) 
				selectIfNoDrag = shared_from<Instance>(findDecal(frontPart, inputObject));
			else if(selectable3d)
				selection->setSelection(selectable3d);

			ServiceClient< FilteredSelection<Instance> > instanceSelection(workspace);
			return AdvDragTool::onMouseDown(
                frontPart,
				hitWorld,
				instanceSelection->items(),
				inputObject,
				workspace,
				selectIfNoDrag );
		}
	}
	else
    {
		shared_ptr<MouseCommand> boxSelect = Creatable<MouseCommand>::create<BoxSelectCommand>(workspace);
		return boxSelect->onMouseDown(inputObject);
	}

	return shared_ptr<MouseCommand>();
}

void AdvArrowToolBase::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
	AdvArrowToolBase::advManipInProgress = true;
	determineManualJointConditions();
}

void AdvArrowToolBase::determineManualJointConditions(void)
{
	ServiceClient< FilteredSelection<Instance> > instanceSelection(workspace);
	manualJointHelper.setDisplayPotentialJoints(AdvArrowToolBase::advManualJointMode);
	manualJointHelper.setSelectedPrimitives(instanceSelection->items());
	if(AdvArrowToolBase::advManualJointMode || AdvArrowTool::advCreateJointsMode)
		manualJointHelper.findPermissibleJointSurfacePairs();

	if((manualJointHelper.getNumJointSurfacePairs() > 0 && AdvArrowTool::advManualJointMode) || manualJointHelper.autoJointsWereDetected())
		setCursor("advClosed-hand-weld");
	else if( instanceSelection->items().size() > 0 )
		setCursor("advClosed-hand-no-weld");
}

shared_ptr<MouseCommand> AdvArrowToolBase::onMouseUp(const shared_ptr<InputObject>& inputObject)
{
	if (DFFlag::RestoreTransparencyOnToolChange)
		restoreSavedPartsTransparency();

	if (AdvArrowToolBase::advManipInProgress)
	{
		AdvArrowToolBase::advManipInProgress = false;
		manualJointHelper.createJointsIfEnabledFromGui();
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
	}

	return shared_ptr<MouseCommand>();
}

///////////////////////////////////////////////////////////////////////////
//
// Box Select

BoxSelectCommand::BoxSelectCommand(Workspace* workspace)
	: Named<MouseCommand, sBoxSelectCommand>(workspace) 
	, selection(workspace)
{
	FASTLOG1(FLog::MouseCommandLifetime, "BoxSelectCommand created: %p", this);
}

BoxSelectCommand::~BoxSelectCommand()
{
	FASTLOG1(FLog::MouseCommandLifetime, "BoxSelectCommand destriyed: %p", this);
}

void BoxSelectCommand::selectAnd(const std::set< shared_ptr<Instance> >& newItemsInBox)
{
	if (newItemsInBox == previousItemsInBox)
		return;

	Selection* selection = ServiceProvider::find<Selection>(DataModel::get(workspace));

	shared_ptr<Instances> instances(shared_ptr<Instances>(new Instances));
	for (std::set<shared_ptr<Instance> >::const_iterator iter = newItemsInBox.begin(); iter != newItemsInBox.end(); ++iter)
		instances->push_back(*iter);

	selection->setSelection(instances);

	previousItemsInBox = newItemsInBox;
}

void BoxSelectCommand::selectReverse(const std::set< shared_ptr<Instance> >& newItemsInBox)
{
	std::set<shared_ptr<Instance> > toggleItems;
	std::set_difference(
		newItemsInBox.begin(), newItemsInBox.end(), 
		previousItemsInBox.begin(), previousItemsInBox.end(), 
		std::inserter(toggleItems, toggleItems.begin())
		); 

	std::set_difference(
		previousItemsInBox.begin(), previousItemsInBox.end(), 
		newItemsInBox.begin(), newItemsInBox.end(), 
		std::inserter(toggleItems, toggleItems.begin())
		); 

	if (toggleItems.size() > 0)
	{
		shared_ptr<Instances> instances(shared_ptr<Instances>(new Instances));
		for (std::set<shared_ptr<Instance> >::const_iterator iter = toggleItems.begin(); iter != toggleItems.end(); ++iter)
			instances->push_back(*iter);

		selection->toggleSelection(instances);
	}
	
	previousItemsInBox = newItemsInBox;
}

shared_ptr<MouseCommand> BoxSelectCommand::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	previousItemsInBox.clear();

	capture();

	UserInputService* userInputService = ServiceProvider::find<UserInputService>(workspace);

	reverseSelecting = userInputService->isShiftDown();

	if (!reverseSelecting)
		selection->setSelection(shared_ptr<const Instances>());

	mouseDownView = inputObject->get2DPosition();
	mouseCurrentView = inputObject->get2DPosition();

	return Super::onMouseDown(inputObject);
}

void BoxSelectCommand::onMouseMove(const shared_ptr<InputObject>& inputObject)
{
	mouseCurrentView = inputObject->get2DPosition();

	std::set< shared_ptr<Instance> > instances;

	Rect2D r = Rect2D::xyxy(mouseDownView.x, mouseDownView.y, mouseCurrentView.x, mouseCurrentView.y);
	const Camera* camera = workspace->getConstCamera();
	getMouseInstances(instances, inputObject, r, camera, workspace);

	if (reverseSelecting)
		selectReverse(instances);
	else
		selectAnd(instances);			
}

void BoxSelectCommand::getMouseInstances(std::set< shared_ptr<Instance> >& instances, 
										  const shared_ptr<InputObject>& inputObject,
										  const Rect2D& selectBox, const Camera* camera,
										  Instance* currentInstance)
{
	for (size_t i = 0; i < currentInstance->numChildren(); i++) 
	{
		Instance* instance = currentInstance->getChild(i);
		IHasLocation* iLocation = currentInstance->queryTypedChild<IHasLocation>(i);
		if (!iLocation)
		{
			getMouseInstances(instances, inputObject, selectBox, camera, instance);
			continue;
		}

		Selectable* selectable = currentInstance->queryTypedChild<Selectable>(i);
		if (!selectable)
			continue;

        if (FFlag::StudioDE6194FixEnabled && !selectable->isSelectable3d())
            continue;

		if (!PartInstance::getLocked(instance))
		{
			// test intersection in Grid Units
			Vector3 gridPos = iLocation->getLocation().translation;
			Vector3 projection = camera->project(gridPos);
		
			if (selectBox.contains(projection.xy()))
				instances.insert(shared_from(instance));
		}
	}
}

void BoxSelectCommand::render2d(Adorn* adorn)
{
	G3D::Rect2D temp = G3D::Rect2D::xyxy(mouseDownView.x, mouseDownView.y,
										mouseCurrentView.x, mouseCurrentView.y);
	adorn->outlineRect2d(temp, 0.75f, Color3::gray());
}


} // namespace
