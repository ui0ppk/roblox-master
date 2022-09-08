#include "stdafx.h"

#include "V8DataModel/GuiObject.h"
#include "Network/Players.h"
#include "GfxBase/Adorn.h"
#include "Gui/Guidraw.h"
#include "V8DataModel/TweenService.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/ScreenGui.h"
#include "V8DataModel/BillboardGui.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/UserInputService.h"
#include "v8datamodel/ScrollingFrame.h"
#include "v8datamodel/GuiService.h"
#include "v8datamodel/Folder.h"
#include "v8datamodel/ImageLabel.h"
#include "v8datamodel/ImageButton.h"
#include "Script/ScriptContext.h"
#include "FastLog.h"

#include "VMProtect/VMProtectSDK.h"

#define MAX_BORDER_SIZE_PIXEL 100

DYNAMIC_FASTFLAGVARIABLE(ElasticEasingUseTwoPi, false)
DYNAMIC_FASTFLAGVARIABLE(TurnOffFakeEventsForInputEvents, false)
FASTFLAGVARIABLE(TweenCallbacksDuringRenderStep, false)

FASTFLAGVARIABLE(FixSlice9Scale, true)

namespace RBX {

const char* const  sGuiObject = "GuiObject";



REFLECTION_BEGIN();
//////////////////////////////////////////////////////////
static const Reflection::BoundFuncDesc<GuiObject, bool(UDim2, UDim2, GuiObject::TweenEasingDirection, GuiObject::TweenEasingStyle, float, bool, Lua::WeakFunctionRef)> func_tweenSizeAndPosition(&GuiObject::tweenSizeAndPosition, "TweenSizeAndPosition", "endSize", "endPosition", "easingDirection", GuiObject::EASING_DIRECTION_OUT, "easingStyle", GuiObject::EASING_STYLE_QUAD, "time", 1, "override", false, "callback", Lua::WeakFunctionRef(), Security::None);
static const Reflection::BoundFuncDesc<GuiObject, bool(UDim2, GuiObject::TweenEasingDirection, GuiObject::TweenEasingStyle, float, bool, Lua::WeakFunctionRef)> func_tweenPosition(&GuiObject::tweenPosition, "TweenPosition", "endPosition", "easingDirection", GuiObject::EASING_DIRECTION_OUT, "easingStyle", GuiObject::EASING_STYLE_QUAD, "time", 1, "override", false, "callback", Lua::WeakFunctionRef(), Security::None);
static const Reflection::BoundFuncDesc<GuiObject, bool(UDim2, GuiObject::TweenEasingDirection, GuiObject::TweenEasingStyle, float, bool, Lua::WeakFunctionRef)> func_tweenSize(&GuiObject::tweenSize, "TweenSize", "endSize", "easingDirection", GuiObject::EASING_DIRECTION_OUT, "easingStyle", GuiObject::EASING_STYLE_QUAD, "time", 1, "override", false, "callback", Lua::WeakFunctionRef(), Security::None);

static const Reflection::PropDescriptor<GuiObject, UDim2>	prop_Size("Size", category_Data, &GuiObject::getSize, &GuiObject::setSize);
static const Reflection::PropDescriptor<GuiObject, UDim2>	prop_Position("Position", category_Data, &GuiObject::getPosition, &GuiObject::setPosition);
static const Reflection::PropDescriptor<GuiObject, int>		prop_BorderSizePixel("BorderSizePixel", category_Data, &GuiObject::getBorderSizePixel, &GuiObject::setBorderSizePixel);
const Reflection::PropDescriptor<GuiObject, int>            GuiObject::prop_ZIndex("ZIndex", category_Data, &GuiObject::getZIndex, &GuiObject::setZIndex);

static const Reflection::PropDescriptor<GuiObject, float> prop_Rotation("Rotation", category_Data, &GuiObject::getRotation, &GuiObject::setRotation);

static const Reflection::EnumPropDescriptor<GuiObject, GuiObject::SizeConstraint>	prop_SizeConstraint("SizeConstraint", category_Data, &GuiObject::getSizeConstraint, &GuiObject::setSizeConstraint);

static const Reflection::PropDescriptor<GuiObject, BrickColor> prop_BorderColor("BorderColor", category_Data, &GuiObject::getBorderColor, &GuiObject::setBorderColor, Reflection::PropertyDescriptor::LEGACY_SCRIPTING);
static const Reflection::PropDescriptor<GuiObject, Color3>	   prop_BorderColor3("BorderColor3", category_Data, &GuiObject::getBorderColor3, &GuiObject::setBorderColor3);

static const Reflection::PropDescriptor<GuiObject, BrickColor> prop_BackgroundColor("BackgroundColor", category_Data, &GuiObject::getBackgroundColor, &GuiObject::setBackgroundColor, Reflection::PropertyDescriptor::LEGACY_SCRIPTING);
static const Reflection::PropDescriptor<GuiObject, Color3> prop_BackgroundColor3("BackgroundColor3", category_Data, &GuiObject::getBackgroundColor3, &GuiObject::setBackgroundColor3);

static const Reflection::PropDescriptor<GuiObject, float>      prop_BackgroundTransparency("BackgroundTransparency", category_Data, &GuiObject::getBackgroundTransparency, &GuiObject::setBackgroundTransparency);

static const Reflection::PropDescriptor<GuiObject, bool> prop_Draggable("Draggable", category_Behavior, &GuiObject::getDraggable, &GuiObject::setDraggable);
static const Reflection::PropDescriptor<GuiObject, bool> prop_Clipping("ClipsDescendants", category_Behavior, &GuiObject::getClipping, &GuiObject::setClipping);

const Reflection::PropDescriptor<GuiObject, bool> GuiObject::prop_Visible("Visible", category_Data, &GuiObject::getVisible, &GuiObject::setVisible);
static const Reflection::PropDescriptor<GuiObject, bool> prop_Active("Active", category_Data, &GuiObject::getActive, &GuiObject::setActive);
static const Reflection::PropDescriptor<GuiObject, bool> prop_Selectable("Selectable", category_Data, &GuiObject::getSelectable, &GuiObject::setSelectable, Reflection::PropertyDescriptor::STANDARD, Security::None);

static const Reflection::PropDescriptor<GuiObject, float> prop_Transparency("Transparency", category_Data, &GuiObject::getTransparencyLegacy, &GuiObject::setTransparencyLegacy, Reflection::PropertyDescriptor::LEGACY_SCRIPTING);

static Reflection::RemoteEventDesc<GuiObject, void(int, int)> event_MouseEnter(&GuiObject::mouseEnterSignal, "MouseEnter", "x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiObject, void(int, int)> event_MouseLeave(&GuiObject::mouseLeaveSignal, "MouseLeave", "x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiObject, void(int, int)> event_MouseMoved(&GuiObject::mouseMovedSignal, "MouseMoved", "x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiObject, void(int, int)> event_MouseWheelForward	(&GuiObject::mouseWheelForwardSignal,	"MouseWheelForward", "x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiObject, void(int, int)> event_MouseWheelBackward	(&GuiObject::mouseWheelBackwardSignal,	"MouseWheelBackward", "x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::RemoteEventDesc<GuiObject, void(int, int)> event_DragStopped(&GuiObject::dragStoppedSignal, "DragStopped" , "x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiObject, void(UDim2)> event_DragBegin(&GuiObject::dragBeginSignal, "DragBegin", "initialPosition", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);

// High Level touch events
static Reflection::EventDesc<GuiObject, void(shared_ptr<const RBX::Reflection::ValueArray>)> event_TapGesture(&GuiObject::tapGestureEvent, "TouchTap", "touchPositions", Security::None);
static Reflection::EventDesc<GuiObject, void(shared_ptr<const RBX::Reflection::ValueArray>,float,float,InputObject::UserInputState)> event_PinchGesture(&GuiObject::pinchGestureEvent, "TouchPinch", "touchPositions", "scale", "velocity", "state", Security::None);
static Reflection::EventDesc<GuiObject, void(UserInputService::SwipeDirection, int)> event_SwipeGesture(&GuiObject::swipeGestureEvent, "TouchSwipe", "swipeDirection", "numberOfTouches",Security::None);
static Reflection::EventDesc<GuiObject, void(shared_ptr<const RBX::Reflection::ValueArray>,InputObject::UserInputState)> event_LongPressGesture(&GuiObject::longPressGestureEvent, "TouchLongPress", "touchPositions", "state", Security::None);
static Reflection::EventDesc<GuiObject, void(shared_ptr<const RBX::Reflection::ValueArray>,float,float,InputObject::UserInputState)> event_RotateGesture(&GuiObject::rotateGestureEvent, "TouchRotate", "touchPositions","rotation", "velocity", "state", Security::None);
static Reflection::EventDesc<GuiObject, void(shared_ptr<const RBX::Reflection::ValueArray>,Vector2,Vector2,InputObject::UserInputState)> event_PanGesture(&GuiObject::panGestureEvent, "TouchPan", "touchPositions", "totalTranslation", "velocity", "state", Security::None);

// Low Level Generic input events
static Reflection::EventDesc<GuiObject, void(shared_ptr<Instance>)> event_InputBegin(&GuiObject::inputBeganEvent, "InputBegan", "input", Security::None);
static Reflection::EventDesc<GuiObject, void(shared_ptr<Instance>)> event_InputUpdate(&GuiObject::inputChangedEvent, "InputChanged", "input", Security::None);
static Reflection::EventDesc<GuiObject, void(shared_ptr<Instance>)> event_InputEnd(&GuiObject::inputEndedEvent, "InputEnded", "input", Security::None);

// Gamepad selection overrides
static Reflection::RefPropDescriptor<GuiObject, GuiObject> prop_nextSelectionUp("NextSelectionUp", category_Behavior, &GuiObject::getNextSelectionUp, &GuiObject::setNextSelectionUp, Reflection::PropertyDescriptor::STANDARD);
static Reflection::RefPropDescriptor<GuiObject, GuiObject> prop_nextSelectionDown("NextSelectionDown", category_Behavior, &GuiObject::getNextSelectionDown, &GuiObject::setNextSelectionDown, Reflection::PropertyDescriptor::STANDARD);
static Reflection::RefPropDescriptor<GuiObject, GuiObject> prop_nextSelectionLeft("NextSelectionLeft", category_Behavior, &GuiObject::getNextSelectionLeft, &GuiObject::setNextSelectionLeft, Reflection::PropertyDescriptor::STANDARD);
static Reflection::RefPropDescriptor<GuiObject, GuiObject> prop_nextSelectionRight("NextSelectionRight", category_Behavior, &GuiObject::getNextSelectionRight, &GuiObject::setNextSelectionRight, Reflection::PropertyDescriptor::STANDARD);

static Reflection::RefPropDescriptor<GuiObject, GuiObject> prop_SelectionImageObject("SelectionImageObject", category_Appearance, &GuiObject::getSelectionImageObject, &GuiObject::setSelectionImageObject, Reflection::PropertyDescriptor::STANDARD);

static Reflection::EventDesc<GuiObject, void()> event_SelectionGained(&GuiObject::selectionGainedEvent, "SelectionGained", Security::None);
static Reflection::EventDesc<GuiObject, void()> event_SelectionLost(&GuiObject::selectionLostEvent, "SelectionLost", Security::None);

IMPLEMENT_EVENT_REPLICATOR(GuiObject,event_MouseEnter,		"MouseEnter",		MouseEnter);
IMPLEMENT_EVENT_REPLICATOR(GuiObject,event_MouseLeave,		"MouseLeave",		MouseLeave);
IMPLEMENT_EVENT_REPLICATOR(GuiObject,event_MouseMoved,		"MouseMoved",		MouseMoved);
IMPLEMENT_EVENT_REPLICATOR(GuiObject,event_MouseWheelForward, "MouseWheelForward", MouseWheelForward);
IMPLEMENT_EVENT_REPLICATOR(GuiObject,event_MouseWheelBackward, "MouseWheelBackward",	 MouseWheelBackward);

IMPLEMENT_EVENT_REPLICATOR(GuiObject,event_DragStopped,		"DragStopped",		DragStopped);
IMPLEMENT_EVENT_REPLICATOR(GuiObject,event_DragBegin,		"DragBegin",		DragBegin);
REFLECTION_END();

namespace Reflection
{
template<>
EnumDesc<RBX::GuiObject::TweenEasingDirection>::EnumDesc()
:EnumDescriptor("EasingDirection")
{
	addPair(RBX::GuiObject::EASING_DIRECTION_IN, "In");
	addPair(RBX::GuiObject::EASING_DIRECTION_OUT, "Out");
	addPair(RBX::GuiObject::EASING_DIRECTION_IN_OUT, "InOut");
}
template<>
RBX::GuiObject::TweenEasingDirection& Variant::convert<RBX::GuiObject::TweenEasingDirection>(void)
{
	return genericConvert<RBX::GuiObject::TweenEasingDirection>();
}

template<>
EnumDesc<RBX::GuiObject::TweenEasingStyle>::EnumDesc()
:EnumDescriptor("EasingStyle")
{
	addPair(RBX::GuiObject::EASING_STYLE_LINEAR,	"Linear");
	addPair(RBX::GuiObject::EASING_STYLE_SINE,		"Sine");
	addPair(RBX::GuiObject::EASING_STYLE_BACK,		"Back");
	addPair(RBX::GuiObject::EASING_STYLE_QUAD,		"Quad");
	addPair(RBX::GuiObject::EASING_STYLE_QUART,	"Quart");
	addPair(RBX::GuiObject::EASING_STYLE_QUINT,	"Quint");
	addPair(RBX::GuiObject::EASING_STYLE_BOUNCE,	"Bounce");
	addPair(RBX::GuiObject::EASING_STYLE_ELASTIC,	"Elastic");
}
template<>
RBX::GuiObject::TweenEasingStyle& Variant::convert<RBX::GuiObject::TweenEasingStyle>(void)
{
	return genericConvert<RBX::GuiObject::TweenEasingStyle>();
}

template<>
EnumDesc<RBX::GuiObject::TweenStatus>::EnumDesc()
:EnumDescriptor("TweenStatus")
{
	addPair(RBX::GuiObject::TWEEN_CANCELED,		"Canceled");
	addPair(RBX::GuiObject::TWEEN_COMPLETED,	"Completed");
}
template<>
RBX::GuiObject::TweenStatus& Variant::convert<RBX::GuiObject::TweenStatus>(void)
{
	return genericConvert<RBX::GuiObject::TweenStatus>();
}
}
template<>
bool StringConverter<GuiObject::TweenEasingStyle>::convertToValue(const std::string& text, GuiObject::TweenEasingStyle& value)
{
	return Reflection::EnumDesc<GuiObject::TweenEasingStyle>::singleton().convertToValue(text.c_str(),value);
}

template<>
bool RBX::StringConverter<GuiObject::TweenEasingDirection>::convertToValue(const std::string& text, GuiObject::TweenEasingDirection& value)
{
	return Reflection::EnumDesc<GuiObject::TweenEasingDirection>::singleton().convertToValue(text.c_str(),value);
}


template<>
bool RBX::StringConverter<GuiObject::TweenStatus>::convertToValue(const std::string& text, GuiObject::TweenStatus& value)
{
	return Reflection::EnumDesc<GuiObject::TweenStatus>::singleton().convertToValue(text.c_str(),value);
}

GuiObject::GuiObject(const char* name, bool active) 
	: DescribedNonCreatable<GuiObject, GuiBase2d, sGuiObject>(name)
	, size()
	, sizeConstraint(RELATIVE_XY)  
	, position()
	, borderSizePixel(1)
	, borderColor(BrickColor::brickBlack().color3())
	, backgroundColor(BrickColor::brickGray().color3())
	, visible(true)
	, active(active)
	, backgroundTransparency(0.0f)
	, guiState(Gui::NOTHING)
	, lastMouseDownType(InputObject::TYPE_NONE)
	, serverGuiObject(false)
	, draggable(false)
	, dragging(false)
	, clipping(false)
	, selectionBox(false)
	, selectable(false)
	, draggingEndedConnection(NULL)
	, CONSTRUCT_EVENT_REPLICATOR(GuiObject,GuiObject::mouseEnterSignal,event_MouseEnter,MouseEnter)
	, CONSTRUCT_EVENT_REPLICATOR(GuiObject,GuiObject::mouseLeaveSignal,event_MouseLeave,MouseLeave)
	, CONSTRUCT_EVENT_REPLICATOR(GuiObject,GuiObject::mouseMovedSignal,event_MouseMoved,MouseMoved)
	, CONSTRUCT_EVENT_REPLICATOR(GuiObject,GuiObject::dragStoppedSignal,event_DragStopped,DragStopped)
	, CONSTRUCT_EVENT_REPLICATOR(GuiObject,GuiObject::dragBeginSignal,event_DragBegin,DragBegin)
	, CONSTRUCT_EVENT_REPLICATOR(GuiObject,GuiObject::mouseWheelForwardSignal, 	event_MouseWheelForward,	MouseWheelForward)
	, CONSTRUCT_EVENT_REPLICATOR(GuiObject,GuiObject::mouseWheelBackwardSignal, event_MouseWheelBackward,	MouseWheelBackward)
{
	CONNECT_EVENT_REPLICATOR(MouseEnter);
	CONNECT_EVENT_REPLICATOR(MouseLeave);
	CONNECT_EVENT_REPLICATOR(MouseMoved);
	CONNECT_EVENT_REPLICATOR(DragStopped);
	CONNECT_EVENT_REPLICATOR(DragBegin);
	CONNECT_EVENT_REPLICATOR(MouseWheelForward);
	CONNECT_EVENT_REPLICATOR(MouseWheelBackward);
}

static void InvokeCallback(boost::function<void(GuiObject::TweenStatus)> func, GuiObject::TweenStatus status)
{
	func(status);
}

void GuiObject::UpdateTween(Tween& tween, GuiObject* obj, boost::function<void(GuiObject*, UDim2)> updateFunc, float timeStep)
{
	if(tween.delayTime > 0)
	{
		tween.delayTime -= (float) timeStep;
		if(tween.delayTime <= 0)
		{
			updateFunc(obj, tween.start);
			tween.elapsedTime += std::abs<float>(tween.delayTime);
		}
		else
		{
			return;
		}
	}
	else
	{
		//Normal time update
		tween.elapsedTime += (float) timeStep;
	}
	if(tween.elapsedTime >= tween.totalTime)
	{
		updateFunc(obj, tween.end);

		if(tween.callback)
		{
			if (FFlag::TweenCallbacksDuringRenderStep)
			{
				if (TweenService* tweenService = RBX::ServiceProvider::create<TweenService>(obj))
				{
					tweenService->addTweenCallback(tween.callback, TWEEN_COMPLETED);
				}
			}
			else if(DataModel* dm = DataModel::get(obj))
			{
				dm->submitTask(boost::bind(&InvokeCallback, tween.callback, TWEEN_COMPLETED), DataModelJob::Write);
			}
			tween.callback.clear();
		}
	}
	else
	{
		updateFunc(obj, GuiObject::TweenInterpolate(tween.style, tween.variance,
			tween.elapsedTime, tween.totalTime, tween.start, tween.end)); 
	}
}

static void InvokeTweenStatusCallback(boost::weak_ptr<GuiObject> weakThis, Lua::WeakFunctionRef callback, GuiObject::TweenStatus tweenStatus)
{
    if (Lua::ThreadRef threadRef = callback.lock())
    {
        if (shared_ptr<GuiObject> strongThis = weakThis.lock())
        {
            if (ScriptContext* sc = ServiceProvider::create<ScriptContext>(strongThis.get()))
            {
                Reflection::Tuple args;
                args.values.push_back(tweenStatus);
                
                try
                {
                    sc->callInNewThread(callback, args);
                }
                catch (RBX::base_exception& e)
                {
                    StandardOut::singleton()->printf(MESSAGE_ERROR, "Unexpected error while invoking callback: %s", e.what());
                }
            }
        }
    }
}

static void InvokeRemoveOnTweenEnd(boost::weak_ptr<GuiObject> weakThis,  Lua::WeakFunctionRef callback)
{
	if(shared_ptr<GuiObject> strongThis = weakThis.lock())
		strongThis->setParent(NULL);
}

bool GuiObject::tweenPosition(UDim2 endPosition, TweenEasingDirection style, TweenEasingStyle variance,float time,  bool overwrite, Lua::WeakFunctionRef callback)
{
	return tweenPositionDelay(getPosition(), endPosition, time, style, variance, 0.0f, overwrite, boost::bind(&InvokeTweenStatusCallback, weak_from(this), callback, _1));
}

bool GuiObject::tweenPosition(UDim2 endPosition, TweenEasingDirection style, TweenEasingStyle variance,float time,  bool overwrite, bool removeOnCallback)
{
	TweenService* tweenService = ServiceProvider::create<TweenService>(this);
	if(removeOnCallback)
		return tweenPositionDelay(getPosition(), endPosition, time, style, variance, 0.0f, overwrite, boost::bind(&InvokeRemoveOnTweenEnd, weak_from(this), Lua::WeakFunctionRef()), tweenService);
	else
		return tweenPositionDelay(getPosition(), endPosition, time, style, variance, 0.0f, overwrite, boost::bind(&InvokeTweenStatusCallback, weak_from(this), Lua::WeakFunctionRef(), _1), tweenService);
}

bool GuiObject::tweenPosition(UDim2 endPosition, TweenEasingDirection style, TweenEasingStyle variance,float time,  bool overwrite, bool removeOnCallback, TweenService* tweenService)
{
	if(removeOnCallback)
		return tweenPositionDelay(getPosition(), endPosition, time, style, variance, 0.0f, overwrite, boost::bind(&InvokeRemoveOnTweenEnd, weak_from(this), Lua::WeakFunctionRef()), tweenService);
	else
		return tweenPositionDelay(getPosition(), endPosition, time, style, variance, 0.0f, overwrite, boost::bind(&InvokeTweenStatusCallback, weak_from(this), Lua::WeakFunctionRef(), _1), tweenService);
}

bool GuiObject::tweenSize(UDim2 endSize, TweenEasingDirection style, TweenEasingStyle variance, float time, bool overwrite, Lua::WeakFunctionRef callback)
{
	return tweenSizeDelay(getSize(), endSize, time, style, variance, 0.0f, overwrite, boost::bind(&InvokeTweenStatusCallback, weak_from(this), callback, _1));
}

bool GuiObject::tweenSizeAndPosition(UDim2 endSize, UDim2 endPosition, TweenEasingDirection style, TweenEasingStyle variance, float time, bool overwrite, Lua::WeakFunctionRef callback)
{
	if(!overwrite){
		if(tweens && (tweens->positionTween || tweens->sizeTween)){
			return false;
		}
	}

	bool result = tweenSize(endSize, style, variance, time, overwrite, callback);
	result = tweenPosition(endPosition, style, variance, time, overwrite, Lua::WeakFunctionRef()) && result;

	return result;
}
bool GuiObject::tweenPositionDelay(UDim2 startPosition, UDim2 endPosition, float time, TweenEasingDirection style, TweenEasingStyle variance, float delayTime, bool overwrite, boost::function<void(TweenStatus)> callback)
{
	TweenService* tweenService = ServiceProvider::create<TweenService>(this);
	return tweenPositionDelay(startPosition, endPosition, time, style, variance, delayTime, overwrite, callback, tweenService);
}

bool GuiObject::tweenPositionDelay(UDim2 startPosition, UDim2 endPosition, float time, TweenEasingDirection style, TweenEasingStyle variance, float delayTime, bool overwrite, boost::function<void(TweenStatus)> callback, TweenService* tweenService)
{
	if(!tweenService)
		throw std::runtime_error("Can only tween objects in the workspace");

	if(!tweens)
	{
		tweens.reset(new Tweens());
		tweenService->addTweeningObject(weak_from(this));
	}

	if(tweens->positionTween)
	{
		if(!overwrite)
			return false;

		if(tweens->positionTween->callback)
		{
			if (FFlag::TweenCallbacksDuringRenderStep)
			{
				tweenService->addTweenCallback(tweens->positionTween->callback, TWEEN_CANCELED);
			}
			else if(DataModel* dm = DataModel::get(this))
			{
				dm->submitTask(boost::bind(&InvokeCallback, tweens->positionTween->callback, TWEEN_CANCELED), DataModelJob::Write);
		}
		}
		tweens->positionTween.reset();
	}
	RBXASSERT(!tweens->positionTween);

	tweens->positionTween.reset(new Tween(startPosition, endPosition, time, style, variance, delayTime));
	tweens->positionTween->callback = callback;
	return true;
}
bool GuiObject::tweenSizeDelay(UDim2 startSize, UDim2 endSize, float time, TweenEasingDirection style, TweenEasingStyle variance, float delayTime, bool overwrite, boost::function<void(TweenStatus)> callback)
{
	TweenService* tweenService = ServiceProvider::create<TweenService>(this);
	if(!tweenService)
		throw std::runtime_error("Can only tween objects in the workspace");

	if(!tweens)
	{
		tweens.reset(new Tweens());
		tweenService->addTweeningObject(weak_from(this));
	}

	if(tweens->sizeTween)
	{
		if(!overwrite)
			return false;

		if(tweens->sizeTween->callback)
		{
			if (FFlag::TweenCallbacksDuringRenderStep)
			{
				tweenService->addTweenCallback(tweens->sizeTween->callback, TWEEN_CANCELED);
			}
			else if(DataModel* dm = DataModel::get(this))
			{
				dm->submitTask(boost::bind(&InvokeCallback, tweens->sizeTween->callback, TWEEN_CANCELED), DataModelJob::Write);
		}
		}
		tweens->sizeTween.reset();
	}
	RBXASSERT(!tweens->sizeTween);

	tweens->sizeTween.reset(new Tween(startSize, endSize, time, style, variance, delayTime));
	tweens->sizeTween->callback = callback;
	return true;

}

bool GuiObject::tweenStep(const double& timeStep)
{
	if(!tweens)
		return true;

	if(tweens->positionTween){
		UpdateTween(*(tweens->positionTween), this, &GuiObject::setPosition, (float)timeStep);
		if(tweens->positionTween->isDone())
			tweens->positionTween.reset();
	}
	if(tweens->sizeTween){
		UpdateTween(*(tweens->sizeTween), this, &GuiObject::setSize, (float)timeStep);
		if(tweens->sizeTween->isDone())
			tweens->sizeTween.reset();

	}

	if(tweens->empty())
		tweens.reset();

	return !tweens;
}

UDim2 GuiObject::TweenInterpolate(TweenEasingDirection style, TweenEasingStyle variance, 
								  float elapsedTime, float totalTime, const UDim2& startValue, const UDim2& endValue)
{
	switch(variance)
	{
	case EASING_STYLE_LINEAR:	
		return (endValue-startValue) * (elapsedTime/totalTime) + startValue;
	case EASING_STYLE_SINE:
	{
		float factor = 1;
		switch(style)
		{
		case EASING_DIRECTION_IN:
			factor = 1 - cos(elapsedTime/totalTime * Math::piHalf());
			break;
		case EASING_DIRECTION_OUT:
			factor = sin(elapsedTime/totalTime * Math::piHalf());
			break;
		case EASING_DIRECTION_IN_OUT:
			factor =  -0.5 * (cos(Math::pi()*elapsedTime/totalTime) - 1);
			break;
		}
		return (endValue-startValue) * factor + startValue;
	}
	case EASING_STYLE_BACK:
	{
		float factor = 1;
		const float s = 1.70158;
		switch(style)
		{
			case EASING_DIRECTION_IN:
			{
				const float t = elapsedTime/totalTime;
				factor = t*t*((s + 1)*t - s);
				break;
			}
			case EASING_DIRECTION_OUT:
			{
				const float t = elapsedTime/totalTime-1;
				factor = t*t*((s+1)*t+s) + 1;
				break;
			}
			case EASING_DIRECTION_IN_OUT:
			{
				float t = elapsedTime / (totalTime*0.5);
				if(t < 1)
					factor = 0.5*t*t*(((s*1.525) + 1)*t - (s*1.525));
				else{
					t -= 2;
					factor = 0.5*(t*t*(((s*1.525)+1)*t+(s*.1525)) + 2);
				}
				break;
			}
		}
		return ( endValue-startValue) * factor + startValue;
	}
	case EASING_STYLE_QUAD:
	{
		float factor = 1;
		switch(style)
		{
			case EASING_DIRECTION_IN:
			{
				float t = elapsedTime/totalTime;
				factor = t*t;
				break;
			}
			case EASING_DIRECTION_OUT:
			{
				float t = elapsedTime/totalTime;
				factor = -t*(t-2);
				break;
			}
			case EASING_DIRECTION_IN_OUT:
			{
				float t = elapsedTime/(totalTime * 0.5);

				if (t < 1) 
					factor = 0.5*t*t;
				else
					factor = -0.5 * ((t-1)*(t-3) - 1);
				break;
			}
		}
		return (endValue-startValue)*factor + startValue;
	}
	case EASING_STYLE_QUART:
	{
		float factor = 1;
		switch(style){
			case EASING_DIRECTION_IN:
			{
				const float t = elapsedTime/totalTime;
				factor = pow(t, 4);
				break;
			}
			case EASING_DIRECTION_OUT:
			{
				const float t = elapsedTime/totalTime - 1;
				factor = -(pow(t, 4)-1);
				break;
			}
			case EASING_DIRECTION_IN_OUT:
			{
				const float t = elapsedTime/(totalTime * 0.5);
				if (t < 1) 
					factor = 0.5 * pow(t,4);
				else
					factor = -0.5 * (pow(t-2, 4) -2);
				break;
			}
		}
		return (endValue-startValue)*factor + startValue;
	}
	case EASING_STYLE_QUINT:
	{
		float factor = 1;
		switch(style){
			case EASING_DIRECTION_IN:
			{
				const float t = elapsedTime/totalTime;
				factor = pow(t,5);
				break;
			}
			case EASING_DIRECTION_OUT:
			{
				const float t = elapsedTime/totalTime - 1;
				factor = pow(t,5) + 1;
				break;
			}
			case EASING_DIRECTION_IN_OUT:
			{
				const float t = elapsedTime/(totalTime * 0.5);

				if(t < 1)
					factor = 0.5*pow(t,5);
				else
					factor = 0.5*(pow(t-2, 5) + 2);
				break;
			}
		}
		return (endValue-startValue)*factor + startValue;
	}
	case EASING_STYLE_BOUNCE:
	{
		switch(style){
			case EASING_DIRECTION_IN:
				return (endValue-startValue) - TweenInterpolate(EASING_DIRECTION_OUT, EASING_STYLE_BOUNCE, totalTime-elapsedTime, totalTime, UDim2(), endValue-startValue) + startValue;
			case EASING_DIRECTION_OUT:
			{
				const float timePercent = elapsedTime/totalTime;
				float factor = 1;
				if(timePercent < 1/2.75)
					factor = 7.5625*pow(timePercent,2);
				else if(timePercent < 2/2.75)
					factor = 7.5625*pow(timePercent-1.5/2.75, 2) + .75;
				else if(timePercent < 2.5/2.75)
					factor = 7.5625*pow(timePercent-2.25/2.75, 2) + .9375;
				else
					factor = 7.5625*pow(timePercent-2.625/2.75, 2) + .984375;
				return (endValue-startValue)*factor + startValue;
			}
			case EASING_DIRECTION_IN_OUT:
				if(elapsedTime < totalTime*0.5)
					return TweenInterpolate(EASING_DIRECTION_IN, EASING_STYLE_BOUNCE, elapsedTime*2, totalTime, UDim2(), endValue-startValue)*0.5 + startValue;
				else
					return TweenInterpolate(EASING_DIRECTION_OUT, EASING_STYLE_BOUNCE, elapsedTime*2-totalTime, totalTime, UDim2(), endValue-startValue)*0.5 + (endValue-startValue)*.5 + startValue;
			default:
				RBXASSERT(0);
				return UDim2();
		}
	}
	case EASING_STYLE_ELASTIC:
	{
		if(elapsedTime == 0)
			return startValue;
		float t = elapsedTime/totalTime;
		if(t >= 1)
			return endValue;

		float factor = 1;
		switch(style) {
			case EASING_DIRECTION_IN:
			{
				float p = totalTime * .3;
				float s = p / 4;
				t -= 1;
				factor = -(pow(2,10*t) * sin((t*totalTime-s)*(DFFlag::ElasticEasingUseTwoPi ? Math::twoPi() : Math::piHalf())/p));
				break;
			}
			case EASING_DIRECTION_OUT:
			{
				float p=totalTime*.3;
				float s = p/4;
				factor = 1 + pow(2,-10*t) * sin( (t*totalTime-s)*(DFFlag::ElasticEasingUseTwoPi ? Math::twoPi() : Math::piHalf())/p );
				break;
			}
			case EASING_DIRECTION_IN_OUT:
			{
				t = elapsedTime / (totalTime * 0.5);
				float p=totalTime*(.3*1.5);
				float s = p/4;

				if (t < 1) {
					t -= 1;
					factor = -.5 * pow(2,10*t) * sin( (t*totalTime-s)*(DFFlag::ElasticEasingUseTwoPi ? Math::twoPi() : Math::piHalf())/p );
				}
				else {
					t -= 1;
					factor = 1 + 0.5 * pow(2,-10*t) * sin( (t*totalTime-s)*(DFFlag::ElasticEasingUseTwoPi ? Math::twoPi() : Math::piHalf())/p );
				}
				break;
			}
		}
		return (endValue-startValue)*factor + startValue;
	}
	default:
		RBXASSERT(0);
		return UDim2();
	}
}

void GuiObject::setServerGuiObject()
{
	serverGuiObject = true;
	//Since we are running the server, we need to be watching for local listeners
	eventReplicatorMouseEnter.setListenerMode(!mouseEnterSignal.empty());
	eventReplicatorMouseLeave.setListenerMode(!mouseLeaveSignal.empty());
	eventReplicatorMouseMoved.setListenerMode(!mouseMovedSignal.empty());
	eventReplicatorMouseWheelForward	.setListenerMode(!mouseWheelForwardSignal.empty());
	eventReplicatorMouseWheelBackward	.setListenerMode(!mouseWheelBackwardSignal.empty());

	eventReplicatorDragBegin.setListenerMode(!dragBeginSignal.empty());
	eventReplicatorDragStopped.setListenerMode(!dragStoppedSignal.empty());
}

static Vector2 getConstrainedSize(const Vector2& parentSize, GuiObject::SizeConstraint sizeConstraint)
{
	switch(sizeConstraint){
		case GuiObject::RELATIVE_XX: return Vector2(parentSize.x, parentSize.x); 
		case GuiObject::RELATIVE_YY: return Vector2(parentSize.y, parentSize.y);
		case GuiObject::RELATIVE_XY: 
		default:					 return parentSize;
	}
}

Vector2 GuiObject::getAbsolutePosition() const
{
    if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
    {
        Vector4 guiInset = guiService->getGlobalGuiInset();
        return Vector2(absolutePosition.x - guiInset.x, absolutePosition.y - guiInset.y);
    }
    return absolutePosition;
}

void GuiObject::handleDrag(RBX::Vector2 mousePosition)
{	
	if(mousePosition != lastMousePosition)
	{
		Vector2 mouseDelta = mousePosition - lastMousePosition;
		setPosition(RBX::UDim2(position.x.scale,position.x.offset + mouseDelta.x, position.y.scale,position.y.offset + mouseDelta.y));
		lastMousePosition = mousePosition;
	}
}

void GuiObject::handleDragging(const shared_ptr<InputObject>& event)
{
	if (mouseIsOver(event->get2DPosition()) && event->getUserInputState() == InputObject::INPUT_STATE_BEGIN && !dragging)
	{
		if (event->isLeftMouseDownEvent() || event->isTouchEvent())
		{
			dragging = true;
			handleDragBegin(event->get2DPosition());
			dragBeginSignal(getPosition());
			if (draggingBeganInputObject ==  shared_ptr<InputObject>())
			{
				if(UserInputService* inputService = RBX::ServiceProvider::find<UserInputService>(this))
				{
					draggingBeganInputObject = shared_from(event.get());
					draggingEndedConnection = inputService->coreInputEndedEvent.connect(boost::bind(&GuiObject::draggingEnded,this,_1));
				}
			}
		}
	} 
	else if (dragging)
	{
		handleDrag(event->get2DPosition());
	}
}

void GuiObject::handleDragBegin(Vector2 mousePosition)
{
	lastMousePosition = mousePosition;
}

bool GuiObject::descendantOfBillboardGui()
{
	Instance* parent = getParent();
	if(!parent)
		return false;
	else if( parent->fastDynamicCast<BillboardGui>() )
		return true;
	
	if(GuiObject* guiObject = parent->fastDynamicCast<GuiObject>())
		return guiObject->descendantOfBillboardGui();

	return false;
}

bool GuiObject::recalculateAbsolutePlacement(const Rect2D& parentViewport)
{
    GuiObject* guiParent = fastDynamicCast<GuiObject>(getParent());
	Rotation2D parentRotation = guiParent ? guiParent->getAbsoluteRotation() : Rotation2D();

	bool result = false;

    // Update size
	Vector2 constrainedParentSize = getConstrainedSize(parentViewport.wh(), sizeConstraint);

	result = setAbsoluteSize(size * constrainedParentSize) || result;
    
    // Update position; note that we rotate the center of the element
	Vector2 transformedPosition = parentRotation.rotate(parentViewport.x0y0() + (position*parentViewport.wh()) + absoluteSizeFloat * 0.5f) - absoluteSizeFloat * 0.5f;

	result = setAbsolutePosition(transformedPosition);

    // Update rotation
	Rotation2D transformedRotation = Rotation2D(parentRotation.getAngle().combine(rotation), absolutePosition + absoluteSizeFloat * 0.5f);

	result = setAbsoluteRotation(transformedRotation) || result;

    return result;
}

void GuiObject::forceResize()
{
	if(GuiBase2d* parent = Instance::fastDynamicCast<GuiBase2d>(getParent()))
	{
		handleResize(parent->getCanvasRect(), true);
	}
}
void GuiObject::checkForResize()
{
	Folder* folderParent = NULL;

	if(GuiBase2d* parent = Instance::fastDynamicCast<GuiBase2d>(getParent()))
	{
		handleResize(parent->getCanvasRect(), false);
	} 
	else if ((folderParent = Instance::fastDynamicCast<Folder>(getParent())))
	{
		GuiBase2d* guiParent = NULL;
		Instance *instParent = folderParent->getParent();
		while (instParent != NULL && guiParent == NULL && folderParent != NULL)
		{
			folderParent = Instance::fastDynamicCast<Folder>(instParent);
			guiParent = Instance::fastDynamicCast<GuiBase2d>(instParent);
			instParent = instParent->getParent();
		}
		if (guiParent)
		{
			handleResize(guiParent->getCanvasRect(), false);
		}
	}
}

void GuiObject::setSize(UDim2 value)
{
	if(size != value){
		size = value;
		raisePropertyChanged(prop_Size);
		
		checkForResize();
	}
}

void GuiObject::setSizeConstraint(SizeConstraint value)
{
	if(sizeConstraint != value){
		sizeConstraint = value;
		raisePropertyChanged(prop_SizeConstraint);
		
		checkForResize();
	}
}

void GuiObject::setPosition(UDim2 value)
{
    RBXASSERT(isFinite(value.x.scale) || isFinite(value.y.scale));
    
	if(position != value){
		position = value;
		raisePropertyChanged(prop_Position);
		
		checkForResize();
	}
}

void GuiObject::setRotation(float value)
{
	if(rotation.getValue() != value){
		rotation = RotationAngle(value);
        raisePropertyChanged(prop_Rotation);
		
		checkForResize();
	}
}

bool GuiObject::setAbsoluteRotation(const Rotation2D& value)
{
	if (absoluteRotation != value)
	{
        absoluteRotation = value;
        return true;
	}
    
    return false;
}

void GuiObject::setSelectionImageObject(GuiObject* value)
{
	if (value != selectionImageObject.get())
	{
		selectionImageObject = shared_from(value);
		raisePropertyChanged(prop_SelectionImageObject);
	}
}

void GuiObject::setClipping(bool value)
{
	if(clipping != value)
	{
		clipping = value;
		raisePropertyChanged(prop_Clipping);
	}
}

GuiObject* GuiObject::firstAncestorClipping()
{
	Instance* parent = getParent();
	while(parent != NULL)
	{
		if(GuiObject* guiParent = fastDynamicCast<GuiObject>(parent))
			if(guiParent->getClipping())
				return guiParent;
		parent = parent->getParent();
	}
	return NULL;
}

void GuiObject::setDraggable(bool value)
{
	if(draggable != value)
	{
		draggable = value;
		raisePropertyChanged(prop_Draggable);
	}
}

void GuiObject::setBorderSizePixel(int value)
{
	if(value < 0) value = 0;

	if (value > MAX_BORDER_SIZE_PIXEL ) value = MAX_BORDER_SIZE_PIXEL;

	if(borderSizePixel != value){
		borderSizePixel = value;
		raisePropertyChanged(prop_BorderSizePixel);
	}
}

void GuiObject::setZIndex(int value)
{
	if(RBX::Security::Context::current().hasPermission(RBX::Security::RobloxScript)){
		//We can access level 0 
		if(value < GuiBase::minZIndex()) 
			value = GuiBase::minZIndex();
	}
	else{
		if(value < GuiBase::minZIndex2d()) 
			value = GuiBase::minZIndex2d();
	}
	if(value > GuiBase::maxZIndex2d()) value = GuiBase::maxZIndex2d();

	if(zIndex != value){
		zIndex = value;
		raisePropertyChanged(prop_ZIndex);
	}
}

Rect2D GuiObject::getClippedRect()
{
	GuiObject* ancestorClipper = firstAncestorClipping();
	Rect2D clippedRect = getRect2D();
	while(ancestorClipper != NULL)
	{
		clippedRect = ancestorClipper->getRect2D().intersect(clippedRect);
		ancestorClipper = ancestorClipper->firstAncestorClipping();
	}

	return clippedRect;
}

void GuiObject::setBackgroundColor(BrickColor value)
{	
	setBackgroundColor3(value.color3());
}

void GuiObject::setBackgroundColor3(Color3 value)
{	
	if(backgroundColor != value){
		backgroundColor = value;
		raisePropertyChanged(prop_BackgroundColor);
		raisePropertyChanged(prop_BackgroundColor3);
	}
}

void GuiObject::setBorderColor(BrickColor value)
{	
	setBorderColor3(value.color3());
}

void GuiObject::setBorderColor3(Color3 value)
{	
	if(borderColor != value){
		borderColor = value;
		raisePropertyChanged(prop_BorderColor);
		raisePropertyChanged(prop_BorderColor3);
	}
}

void GuiObject::setVisible(bool value)
{
	if(visible != value){
		visible = value;
		raisePropertyChanged(prop_Visible);
		if(!visible){
			guiState = Gui::NOTHING;
		}
	}
}

void GuiObject::setActive(bool value)
{
	if(active != value)
	{
		active = value;
		raisePropertyChanged(prop_Active);
		if(!active){
			guiState = Gui::NOTHING;
		}
	}
}

void GuiObject::setSelectable(bool value)
{
	if(selectable != value)
	{
		selectable = value;
		raisePropertyChanged(prop_Selectable);
	}
}


GuiObject* getRawPointer(weak_ptr<GuiObject> weakObject)
{
	GuiObject* rawPointer = NULL;
	if (shared_ptr<GuiObject> sharedPointer = weakObject.lock())
	{
		rawPointer = sharedPointer.get();
	}
	return rawPointer;
}

GuiObject* GuiObject::getNextSelectionUp() const
{ 
	return getRawPointer(nextSelectionUp);
}
void GuiObject::setNextSelectionUp(GuiObject* value)
{
	if (value != getRawPointer(nextSelectionUp))
	{
		nextSelectionUp = weak_from(value);
		raisePropertyChanged(prop_nextSelectionUp);
	}
}

GuiObject* GuiObject::getNextSelectionDown() const
{
	return getRawPointer(nextSelectionDown);
}
void GuiObject::setNextSelectionDown(GuiObject* value)
{
	if (value != getRawPointer(nextSelectionDown))
	{
		nextSelectionDown = weak_from(value);
		raisePropertyChanged(prop_nextSelectionDown);
	}
}

GuiObject* GuiObject::getNextSelectionLeft() const
{
	return getRawPointer(nextSelectionLeft);
}
void GuiObject::setNextSelectionLeft(GuiObject* value)
{
	if (value != getRawPointer(nextSelectionLeft))
	{
		nextSelectionLeft = weak_from(value);
		raisePropertyChanged(prop_nextSelectionLeft);
	}
}

GuiObject* GuiObject::getNextSelectionRight() const
{
	return getRawPointer(nextSelectionRight);
}
void GuiObject::setNextSelectionRight(GuiObject* value)
{
	if (value != getRawPointer(nextSelectionRight))
	{
		nextSelectionRight = weak_from(value);
		raisePropertyChanged(prop_nextSelectionRight);
	}
}

void GuiObject::onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
{
	Super::onPropertyChanged(descriptor);

	eventReplicatorMouseEnter.onPropertyChanged(descriptor);
	eventReplicatorMouseLeave.onPropertyChanged(descriptor);
	eventReplicatorMouseMoved.onPropertyChanged(descriptor);
	eventReplicatorMouseWheelForward.onPropertyChanged(descriptor);
	eventReplicatorMouseWheelBackward.onPropertyChanged(descriptor);

	eventReplicatorDragStopped.onPropertyChanged(descriptor);
	eventReplicatorDragBegin.onPropertyChanged(descriptor);
}


void GuiObject::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);

	if(!serverGuiObject && Network::Players::serverIsPresent(this,false)){
		setServerGuiObject();
	}

	checkForResize();
}

bool GuiObject::askSetParent(const Instance* instance) const
{
	if(const GuiBase2d* guiBase = Instance::fastDynamicCast<GuiBase2d>(instance)){
		return !guiBase->isGuiLeaf();
	}
	return false;
}

void GuiObject::setBackgroundTransparency(float value)
{
	if (value != backgroundTransparency) {
		backgroundTransparency = value;
		raisePropertyChanged(prop_BackgroundTransparency);
	}
}

float GuiObject::getRenderBackgroundAlpha() const
{
	return G3D::clamp(1.0f - getBackgroundTransparency(), 0.0f, 1.0f);

}
Color4 GuiObject::getRenderBackgroundColor4() const
{
	return Color4(getBackgroundColor3(), getRenderBackgroundAlpha());
}


//Used to render these object in old pure-adorn system
void GuiObject::legacyRender2d(Adorn* adorn, const Rect2D& parentRectangle)
{
	//Only allow this when the object is not in the workspace
	if(getParent() == NULL){
		handleResize(parentRectangle, true);

		recursiveRender2d(adorn);
	}
}

void GuiObject::render2d(Adorn* adorn)
{
	render2dImpl(adorn, getRenderBackgroundColor4());

	renderStudioSelectionBox(adorn);
}

//Common base that saves the rect for later use
void GuiObject::render2dImpl(	Adorn* adorn, 
								const Color4& _backgroundColor,
								Rect2D& rect)
{
	rect = getRect2D();			// virtual call to GuiBase

	float alpha = _backgroundColor.a;

	if(alpha > 0)
	{	
		Color4 _borderColor = Color4(getBorderColor3(), alpha);
		int _borderSize = getBorderSizePixel();

		GuiObject* clippingObject = firstAncestorClipping();

		if (clippingObject == NULL || !absoluteRotation.empty())
		{
			adorn->rect2d(rect, _backgroundColor, absoluteRotation);
			if(_borderSize > 0) adorn->outlineRect2d(rect, (float)_borderSize, _borderColor, absoluteRotation);
		}
		else
		{
			adorn->rect2d(rect, _backgroundColor, clippingObject->getClippedRect());
			if(_borderSize > 0) adorn->outlineRect2d(rect, (float)_borderSize, _borderColor, clippingObject->getClippedRect());
		}
	}
}

void GuiObject::renderSelectionFrame(Adorn* adorn, GuiObject* baseSelectionObject)
{
	if (selectionImageObject && selectionImageObject.get())
	{
		baseSelectionObject = selectionImageObject.get();
	}

	if (baseSelectionObject && baseSelectionObject->getVisible())
	{
		// need to set selectedGuiObject renderer to match this object's size, pos and rotation temporarily
		const UDim2 selectedGuiObjPos = baseSelectionObject->getPosition();
		Vector2 absolutePosition = getAbsolutePosition() + (getAbsoluteSize() * Vector2(selectedGuiObjPos.x.scale, selectedGuiObjPos.y.scale)) + Vector2(selectedGuiObjPos.x.offset, selectedGuiObjPos.y.offset);
		if (GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(this))
		{
			absolutePosition += guiService->getGlobalGuiInset().xy();
		}

		const UDim2 selectedGuiObjSize = baseSelectionObject->getSize();
		Vector2 absoluteSize = (getAbsoluteSize() * Vector2(selectedGuiObjSize.x.scale, selectedGuiObjSize.y.scale)) + Vector2(selectedGuiObjSize.x.offset, selectedGuiObjSize.y.offset);

		Vector2 originalAbsPosition = baseSelectionObject->getAbsolutePosition();
		const Vector2 originalAbsSize = baseSelectionObject->getAbsoluteSize();
		const Rotation2D originalRotation = baseSelectionObject->getAbsoluteRotation();
		const RotationAngle selectionObjectRotation = RotationAngle(baseSelectionObject->getRotation());

		baseSelectionObject->setAbsolutePosition(absolutePosition, false);
		baseSelectionObject->setAbsoluteSize(absoluteSize, false);
		baseSelectionObject->setAbsoluteRotation( Rotation2D(getAbsoluteRotation().getAngle().combine(selectionObjectRotation), baseSelectionObject->getRect2D().center()) );


		// we are setup, now render the selection
		baseSelectionObject->renderBackground2d(adorn);
		baseSelectionObject->render2d(adorn);


		// we rendered, now set the properties back to normal
		if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
		{
			Vector4 guiInset = guiService->getGlobalGuiInset();
			originalAbsPosition += guiInset.xy();
		}

		baseSelectionObject->setAbsolutePosition(originalAbsPosition, false);
		baseSelectionObject->setAbsoluteSize(originalAbsSize, false);
		baseSelectionObject->setAbsoluteRotation(originalRotation);
	}
}

void GuiObject::renderStudioSelectionBox(Adorn* adorn)
{
	if(selectionBox)
	{
		Rect2D outlineRect = getRect2D();
		outlineRect = Rect2D::xyxy(outlineRect.x0() - 1.0f, outlineRect.y0() - 1.0f,outlineRect.x1() + 1.0f,outlineRect.y1() + 1.0f);
		adorn->outlineRect2d(outlineRect,3.0f,RBX::Color4(RBX::Color3(0.1f, 0.6f, 1.0f)), absoluteRotation);
	}
}
//Used by children without text
void GuiObject::render2dImpl(	Adorn* adorn, 
								const Color4& _backgroundColor)
{
	Rect2D rect;
	render2dImpl(adorn, _backgroundColor, rect);
}

Rect2D GuiObject::Scale9Rect2D(const Rect2D& rect, float border, float minSize)
{
	float scale = 1.0f;
	scale = std::min(scale, rect.width() / minSize);
	scale = std::min(scale, rect.height() / minSize);

	float xInset = border*scale;
	float yInset = border*scale;

	return Rect2D::xywh(rect.x0() + xInset, rect.y0() + yInset, rect.width() - xInset*2, rect.height() - yInset*2);
}

static void computeScale9Sizes(Rect output[9], const Rect2D& rect, const Vector2int16& scaleEdgeSize, const Vector2& minSize)
{
	float scale = 1.0f;
	scale = std::min(scale, (rect.width() / minSize.x));
	scale = std::min(scale, (rect.height() / minSize.y));

	float xSize = scaleEdgeSize.x*scale;
	float ySize = scaleEdgeSize.y*scale;

	//(x0, y0)-------(xLeft, y0)-------(xRight, y0)-------(x1,y0)
	//   |                |                 |                |
	//   |       1        |        2        |        3       |
	//   |                |                 |                |
	//(x0, yTop)-----(xLeft, yTop)-----(xRight, yTop)-----(x1,yTop)
	//   |                |                 |                |
	//   |       4        |        5        |        6       |
	//   |                |                 |                |
	//(x0, yBottom)--(xLeft, yBottom)--(xRight, yBottom)--(x1,yBottom)
	//   |                |                 |                |
	//   |       7        |        8        |        9       |
	//   |                |                 |                |
	//(x0, y1)-------(xLeft, y1)-------(xRight, y1)-------(x1,y1)

	float xLeft = rect.x0() + floor(xSize);
	float xRight = rect.x1() - floor(xSize);
	
	float yTop = rect.y0() + floor(ySize);
	float yBottom = rect.y1() - floor(ySize);

	output[0] = Rect(rect.x0(),	rect.y0(),	xLeft,		yTop);
	output[1] = Rect(xLeft,		rect.y0(),	xRight,		yTop);
	output[2] = Rect(xRight,	rect.y0(),	rect.x1(),	yTop);
	output[3] = Rect(rect.x0(),	yTop,		xLeft,		yBottom);
	output[4] = Rect(xLeft,		yTop,		xRight,		yBottom);
	output[5] = Rect(xRight,	yTop,		rect.x1(),	yBottom);
	output[6] = Rect(rect.x0(), yBottom,	xLeft,		rect.y1());
	output[7] = Rect(xLeft,		yBottom,	xRight,		rect.y1());
	output[8] = Rect(xRight,	yBottom,	rect.x1(),	rect.y1());
}

void GuiObject::render2dScale9Impl( Adorn* adorn,
				  				    const TextureId& texId,
								    const Vector2int16& scaleEdgeSize, 
									const Vector2& minSize,
									GuiDrawImage& guiImageDraw,
                                    Rect2D& rect,
									GuiObject* clippingObject)
{
	rect = getRect2D();					// virtual call to GuiBase2d

	Rect sizes[9];
	computeScale9Sizes(sizes, rect, scaleEdgeSize, minSize);

	if(guiImageDraw.setImage(adorn, texId, GuiDrawImage::NORMAL, NULL, this, ".Texture"))
	{
        Color4 color = Color4(Color3::white(), 1.0f - getBackgroundTransparency());

		for(int i = 0; i<9; ++i)
		{
			int rowNum = i / 3;
			int colNum = i % 3;

			if(sizes[i].size().x > 0 && sizes[i].size().y > 0)
			{
				Vector2 texul(colNum * 0.33333f, rowNum * 0.33333f);
				Vector2 texbr((colNum + 1) * 0.33333f, (rowNum + 1) * 0.33333f);
				if(clippingObject == NULL || !absoluteRotation.empty())
					guiImageDraw.render2d(adorn, true, sizes[i], texul, texbr, color, absoluteRotation, Gui::NOTHING, false);
				else
					guiImageDraw.render2d(adorn, true, sizes[i], texul, texbr, color, clippingObject->getClippedRect(), Gui::NOTHING, false);
			}	
		}
	}
}

static void computeScale9Sizes2(Rect output[9], Rect uvs[9], const Rect2D& renderRect, Rect2D& sliceRect, const Rect2D& textureBounds, const Vector2& textureSize)
{
    // Scale the corners down, only if there is not enough room for all the corners.
    float nonScalingX = sliceRect.x0() + (textureBounds.width() - sliceRect.x1());
    float nonScalingY = sliceRect.y0() + (textureBounds.height() - sliceRect.y1());

	float scale = 1.0f;
    scale = std::min(scale, (renderRect.width() / nonScalingX));
    scale = std::min(scale, (renderRect.height() / nonScalingY));
    
    //(x0, y0)-------(xLeft, y0)-------(xRight, y0)-------(x1,y0)
    //   |                |                 |                |
    //   |       0        |        1        |        2       |
    //   |                |                 |                |
    //(x0, yTop)-----(xLeft, yTop)-----(xRight, yTop)-----(x1,yTop)
    //   |                |                 |                |
    //   |       3        |        4        |        5       |
    //   |                |                 |                |
    //(x0, yBottom)--(xLeft, yBottom)--(xRight, yBottom)--(x1,yBottom)
    //   |                |                 |                |
    //   |       6        |        7        |        8       |
    //   |                |                 |                |
    //(x0, y1)-------(xLeft, y1)-------(xRight, y1)-------(x1,y1)
    
	float xLeft;
	float yTop;

	if (FFlag::FixSlice9Scale)
	{
		xLeft = std::max(renderRect.x0(), renderRect.x0() + (sliceRect.x0() * scale));
		yTop = std::max(renderRect.y0(), renderRect.y0() + (sliceRect.y0() * scale));
	}
	else
	{
		xLeft = std::max(renderRect.x0(), (renderRect.x0() + sliceRect.x0()) * scale);
		yTop = std::max(renderRect.y0(), (renderRect.y0() + sliceRect.y0()) * scale);
	}

    float xRight = std::min(renderRect.x1(), renderRect.x1() - (textureBounds.width() - sliceRect.x1()) * scale);
    float yBottom = std::min(renderRect.y1(), renderRect.y1() - (textureBounds.height() - sliceRect.y1()) * scale);

    output[0] = Rect(renderRect.x0(),renderRect.y0(),	xLeft,		    yTop);
    output[1] = Rect(xLeft,		    renderRect.y0(),	xRight,		    yTop);
    output[2] = Rect(xRight,	    renderRect.y0(),	renderRect.x1(),	yTop);
    output[3] = Rect(renderRect.x0(),yTop,		    xLeft,		    yBottom);
    output[4] = Rect(xLeft,		    yTop,		    xRight,		    yBottom);
    output[5] = Rect(xRight,	    yTop,		    renderRect.x1(),	yBottom);
    output[6] = Rect(renderRect.x0(),yBottom,	    xLeft,		    renderRect.y1());
    output[7] = Rect(xLeft,		    yBottom,	    xRight,		    renderRect.y1());
    output[8] = Rect(xRight,	    yBottom,	    renderRect.x1(),	renderRect.y1());
    
	// Set-up for for UV coords
	sliceRect = Rect2D::xyxy(sliceRect.x0y0() + textureBounds.x0y0(), sliceRect.x1y1() + textureBounds.x0y0());

    xLeft = sliceRect.x0();
    xRight = sliceRect.x1();
    
    yTop = sliceRect.y0();
    yBottom = sliceRect.y1();

	float innerXRight = xRight;
	float innerYBottom = yBottom;
	if (output[4].size().x < xRight - xLeft) 
		innerXRight = xLeft + output[4].size().x;
	if (output[4].size().y < yBottom - yTop) 
		innerYBottom = yTop + output[4].size().y;

    uvs[0] = Rect(Rect2D::xyxy(textureBounds.x0(),	textureBounds.y0(),	xLeft,				yTop)				/ textureSize);
	uvs[1] = Rect(Rect2D::xyxy(xLeft,				textureBounds.y0(),	innerXRight,		yTop)				/ textureSize);
    uvs[2] = Rect(Rect2D::xyxy(xRight,				textureBounds.y0(),	textureBounds.x1(),	yTop)				/ textureSize);
    uvs[3] = Rect(Rect2D::xyxy(textureBounds.x0(),	yTop,				xLeft,				innerYBottom)		/ textureSize);
    uvs[4] = Rect(Rect2D::xyxy(xLeft,				yTop,				innerXRight,		innerYBottom)		/ textureSize);
    uvs[5] = Rect(Rect2D::xyxy(xRight,				yTop,				textureBounds.x1(),	innerYBottom)		/ textureSize);
    uvs[6] = Rect(Rect2D::xyxy(textureBounds.x0(),	yBottom,			xLeft,				textureBounds.y1())	/ textureSize);
    uvs[7] = Rect(Rect2D::xyxy(xLeft,				yBottom,			innerXRight,		textureBounds.y1())	/ textureSize);
    uvs[8] = Rect(Rect2D::xyxy(xRight,				yBottom,			textureBounds.x1(),	textureBounds.y1())	/ textureSize);
}

void GuiObject::render2dScale9Impl2(Adorn* adorn,
                                    const TextureId& texId,
                                    GuiDrawImage& guiImageDraw,
                                    Rect2D& scaleRect,
                                    GuiObject* clippingObject,
                                    Color4& color,
									Rect2D* overrideRect,
									Rect2D* imageOffsetRect)
{
	Rect2D rect = getRect2D(); // virtual call to GuiBase2d
	if (overrideRect)
	{
		rect = *overrideRect;
	}

    Rect sizes[9];
    Rect uvs[9];
    
    Vector2 imageSize;
    if(guiImageDraw.setImage(adorn, texId, GuiDrawImage::NORMAL, &imageSize, this, ".Texture"))
    {
		Rect2D centerRect = scaleRect;
		Rect2D imageSizeRect = Rect2D::xywh(Vector2::zero(), imageSize);
		if (imageOffsetRect)
		{
			imageSizeRect = *imageOffsetRect;
		}

        computeScale9Sizes2(sizes, uvs, rect, centerRect, imageSizeRect, imageSize);
        float imageWidth = rect.width();
        float imageHeight = rect.height();
        for(int i = 0; i<9; ++i)
        {
            if(sizes[i].size().x > 0 && sizes[i].size().y > 0 && imageWidth > 0 && imageHeight > 0)
            {
                if(clippingObject == NULL || !absoluteRotation.empty())
                    guiImageDraw.render2d(adorn, true, sizes[i], uvs[i].low, uvs[i].high, color, absoluteRotation, Gui::NOTHING, false);
                else
                    guiImageDraw.render2d(adorn, true, sizes[i], uvs[i].low, uvs[i].high, color, clippingObject->getClippedRect(), Gui::NOTHING, false);
			}
        }
    }
}

Vector2 GuiObject::render2dTextImpl(	Adorn* adorn, 
									const Color4& _backgroundColor,
									const std::string& _textName,
									TextService::Font		_font, 
									TextService::FontSize _fontSizeEnum,
									const Color4& _textColor,
									const Color4& _textStrokeColor,
									bool _textWrap,
									bool _textScale,
									TextService::XAlignment _xalignment,
									TextService::YAlignment _yalignment)
{
	//Background color
	Rect2D rect;
	render2dImpl(adorn, _backgroundColor, rect);

	return render2dTextImpl(adorn, rect, _textName, _font, _fontSizeEnum, _textColor, _textStrokeColor, _textWrap, _textScale, _xalignment, _yalignment);
}
float GuiObject::getFontSizeScale(bool _textScale, bool _textWrap, TextService::FontSize _fontSizeEnum, const Rect2D& rect)
{
	float normalFontSize = convertFontSize(_fontSizeEnum);
	return normalFontSize;
}

float GuiObject::getScaledFontSize( const Rect2D&		rect,
									const std::string&	_textName,
									TextService::Font	_font,
									bool				_textWrap,
									float				_fontSize)
{
	if(TextService* textService = ServiceProvider::create<TextService>(this))																							
	{
		if(Typesetter* typesetter = textService->getTypesetter(_font))
		{
			float biggestLegal = convertFontSize(TextService::SIZE_SMALLEST);
			float min = biggestLegal;
			float max = convertFontSize(TextService::SIZE_LARGEST);

			bool textFits = false;

			while(biggestLegal <= max)
			{
				float mid = min + floor((max - min) / 2.0f);
				typesetter->measure(_textName,
									mid,
									_textWrap ? rect.wh() : Vector2::zero(),
									&textFits);

				if(textFits &&  biggestLegal < mid)
				{
					biggestLegal = mid;
					min = mid + 1.0f;
				}
				else
					max = mid - 1.0f;
			}
			return biggestLegal;
		}
	}

	return _fontSize;
}

Vector2 GuiObject::render2dTextImpl(	Adorn* adorn, 
                                    const Rect2D& rectIn,
									const std::string& _textName,
									TextService::Font		_font, 
									TextService::FontSize _fontSizeEnum,
									const Color4& _textColor,
									const Color4& _textStrokeColor,
									bool _textWrap,
									bool _textScale,
									TextService::XAlignment _xalignment,
									TextService::YAlignment _yalignment)
{
    Rect2D rect;
    float _fontSize = 0; 
    bool autoScale = _textScale && adorn->useFontSmoothScalling();

    rect = autoScale ? getRect2DFloat() : rectIn;
    if (adorn->useFontSmoothScalling())
        _fontSize = getFontSizeScale(_textScale, _textWrap, _fontSizeEnum, rect);
    else
        _fontSize = _textScale ? getScaledFontSize(rect,_textName,_font,_textWrap,_fontSize) : getFontSizeScale(_textScale, _textWrap, _fontSizeEnum, rect);

	Vector2 pos;
	switch(_xalignment){
		case TextService::XALIGNMENT_LEFT:
			pos.x = rect.x0();
			break;
		case TextService::XALIGNMENT_RIGHT:
			pos.x = rect.x1();			
			break;
		case TextService::XALIGNMENT_CENTER:
			pos.x = rect.center().x;
			break;
	}
	switch(_yalignment){
		case TextService::YALIGNMENT_TOP:
			pos.y = rect.y0();			
			break;
		case TextService::YALIGNMENT_CENTER:
			pos.y = rect.center().y;	
			break;
		case TextService::YALIGNMENT_BOTTOM:
			pos.y = rect.y1();			
			break;
	}

	GuiObject* clippingObject = firstAncestorClipping();
	Vector2 wh = _textWrap ? rect.wh() : Vector2::zero();

	if (clippingObject == NULL || !absoluteRotation.empty())
	{
		return adorn->drawFont2D(	_textName,
									pos,
									_fontSize,
                                    autoScale,
									_textColor,
									_textStrokeColor,
									TextService::ToTextFont(_font),
									TextService::ToTextXAlign(_xalignment),
									TextService::ToTextYAlign(_yalignment),
									wh,
									getClipping() ? rect : RBX::Rect2D::xyxy(-1,-1,-1,-1),
									absoluteRotation);
	}
	else
	{
		RBX::Rect2D clippedRect = clippingObject->getClippedRect();
		if(getClipping())
			clippedRect = clippedRect.intersect(rect);

		return adorn->drawFont2D(	_textName,
									pos,
									_fontSize,
                                    autoScale,
									_textColor,
									_textStrokeColor,
									TextService::ToTextFont(_font),
									TextService::ToTextXAlign(_xalignment),
									TextService::ToTextYAlign(_yalignment),
									wh,
									clippedRect);
	}
}

void GuiObject::fireGenericInputEvent(const shared_ptr<InputObject>& event)
{
	// state and type should always be set appropriately
	RBXASSERT(event->getUserInputState() != InputObject::INPUT_STATE_NONE && event->getUserInputType() != InputObject::TYPE_NONE);

	if( event->isKeyEvent() ) //todo: make these work? not sure if key events should register or not
		return;
	
	if (!event->isPublicEvent())
	{
		return;
	}
    
    // if we send a "fake" mouse event on a touch device, don't register as generic event
    // fake mouse events are needed from touch for backwards compatibility
    if(RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(this))
        if(event->isMouseEvent() && !inputService->getMouseEnabled() && inputService->getTouchEnabled())
            return;

	bool isOver = mouseIsOver(event->get2DPosition());

	// we have seen this object before, but it is no longer over, call end
	if (interactedInputObjects.find(event) != interactedInputObjects.end() && !isOver)
	{
		inputEndedEvent(event);
        interactedInputObjects.erase(event);
		return;
	}

	if (!isOver)
		return;

	if (!DFFlag::TurnOffFakeEventsForInputEvents || event->isPublicEvent())
	{
	switch (event->getUserInputState())
	{
	case InputObject::INPUT_STATE_BEGIN:
		inputBeganEvent(event);
		interactedInputObjects[event] = true;
		break;
	case InputObject::INPUT_STATE_CHANGE:
        // haven't seen this object before, store then call began
        if (interactedInputObjects.find(event) == interactedInputObjects.end())
        {
            interactedInputObjects[event] = true;
            inputBeganEvent(event);
        }
        else
            inputChangedEvent(event);
		break;
	case InputObject::INPUT_STATE_END:
		inputEndedEvent(event);
		interactedInputObjects.erase(event);
		break;
	case InputObject::INPUT_STATE_NONE:
	default: // intentional fall thru (none and default do not fire anything)
		break;
	}
}
}


GuiResponse GuiObject::process(const shared_ptr<InputObject>& event)
{
	fireGenericInputEvent(event);

	//Only process mouse events for "Active" objects
	if (event->isMouseEvent())
	{
		return processMouseEvent(event);
	}
	else if (event->isKeyEvent()) 
	{
		return processKeyEvent(event);
	}
	else if (event->isTouchEvent())
	{
		return processTouchEvent(event);
	}
	else if (event->isGamepadEvent())
	{
		return processGamepadEvent(event);
	}
    else
    {
        return GuiResponse::notSunk();
    }
}
    
void GuiObject::fireGestureEvent(const UserInputService::Gesture& gesture, const shared_ptr<const RBX::Reflection::ValueArray>& touchPositions, const shared_ptr<const Reflection::Tuple>& args)
{
    switch (gesture)
    {
        case UserInputService::GESTURE_TAP:
            tapGestureEvent(touchPositions);
            break;
        case UserInputService::GESTURE_SWIPE:
            RBXASSERT(args->values.size() == 2);
            swipeGestureEvent(args->values[0].cast<UserInputService::SwipeDirection>(),args->values[1].cast<int>());
            break;
        case UserInputService::GESTURE_ROTATE:
            RBXASSERT(args->values.size() == 3);
            rotateGestureEvent(touchPositions,args->values[0].cast<float>(),args->values[1].cast<float>(), args->values[2].cast<InputObject::UserInputState>());
            break;
        case UserInputService::GESTURE_PINCH:
            RBXASSERT(args->values.size() == 3);
            pinchGestureEvent(touchPositions,args->values[0].cast<float>(),args->values[1].cast<float>(), args->values[2].cast<InputObject::UserInputState>());
            break;
        case UserInputService::GESTURE_LONGPRESS:
            RBXASSERT(args->values.size() == 1);
            longPressGestureEvent(touchPositions,args->values[0].cast<InputObject::UserInputState>());
            break;
        case UserInputService::GESTURE_PAN:
            RBXASSERT(args->values.size() == 3);
            panGestureEvent(touchPositions,args->values[0].cast<RBX::Vector2>(),args->values[1].cast<RBX::Vector2>(),args->values[2].cast<InputObject::UserInputState>());
            break;
        case UserInputService::GESTURE_NONE:
        default: // intentional fall thru
            break;
    }
}
    
GuiResponse GuiObject::processGesture(const UserInputService::Gesture& gesture, const shared_ptr<const RBX::Reflection::ValueArray>& touchPositions, const shared_ptr<const Reflection::Tuple>& args)
{
    if (!visible)
        return GuiResponse::notSunk();
    
    bool gestureIsOver = true;
    // make sure all touches were in view
    for(Reflection::ValueArray::const_iterator iter = touchPositions->begin(); iter != touchPositions->end(); ++iter)
    {
        if( !mouseIsOver(iter->get<RBX::Vector2>()) )
        {
            gestureIsOver = false;
            break;
        }
    }
    
    if ( gestureIsOver )
    {
        if (!active)
        {
            GuiResponse notSunkResponseOver = GuiResponse::notSunkMouseWasOver();
            notSunkResponseOver.setTarget(this);
            
            return notSunkResponseOver;
        }
        
        fireGestureEvent(gesture,touchPositions,args);
        
        GuiResponse sunkResponse = GuiResponse::sunk();
        sunkResponse.setTarget(this);
        
        return sunkResponse;
    }
    
    return GuiResponse::notSunk();
}

bool GuiObject::mouseIsOver(const Vector2& mousePosition)
{
	Rect rect(getRect2D());

	if (!absoluteRotation.empty())
	{
		Vector2 mousePositionLocal = absoluteRotation.inverse().rotate(mousePosition);

        return rect.pointInRect(mousePositionLocal);
	}

    if (GuiObject* clippingObject = firstAncestorClipping())
    {
        RBX::Rect2D clippedRect = getRect2D().intersect(clippingObject->getClippedRect());
        rect = Rect(clippedRect);
    }

	return rect.pointInRect(mousePosition);
}

GuiResponse GuiObject::processKeyEvent(const shared_ptr<InputObject>& event)
{
	return GuiResponse::notSunk();
}

GuiResponse GuiObject::processTouchEvent(const shared_ptr<InputObject>& event)
{
	if (getActive() && getDraggable())
	{
		handleDragging(event);
	}
	return (mouseIsOver(event->get2DPosition()) && getActive()) ? GuiResponse::sunk() : GuiResponse::notSunk();
}

GuiResponse GuiObject::processGamepadEvent(const shared_ptr<InputObject>& event)
{
	return GuiResponse::notSunk();
}

GuiResponse GuiObject::processMouseEventInternal(const shared_ptr<InputObject>& event, bool fireEvents)
{
	bool mouseOver = mouseIsOver(event->get2DPosition());
    bool eventMoved = (event->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT);
    bool eventLeftButtonDown = event->isLeftMouseDownEvent();
    bool eventRightButtonDown = event->isRightMouseDownEvent();
    bool eventButtonDown = (eventLeftButtonDown || eventRightButtonDown);
    bool eventMouseWheelForward = event->isMouseWheelForward();
    bool eventMouseWheelBackward = event->isMouseWheelBackward();
    bool wasNothing = (guiState == Gui::NOTHING);
    bool wasHover = (guiState == Gui::HOVER);
    bool wasDownOver = (guiState == Gui::DOWN_OVER);
    bool wasDownAway = (guiState == Gui::DOWN_AWAY);
    
    Vector2 mousePosition = event->get2DPosition();
    
    // 1. Figure out events
	if (fireEvents)
	{
		if (mouseOver && (wasNothing || wasDownAway))		{mouseEnterSignal(mousePosition.x, mousePosition.y);}
		if (mouseOver && eventMoved)						{mouseMovedSignal(mousePosition.x, mousePosition.y);}
		if (mouseOver && eventMouseWheelForward)			{mouseWheelForwardSignal(mousePosition.x, mousePosition.y);}
		if (mouseOver && eventMouseWheelBackward)			{mouseWheelBackwardSignal(mousePosition.x, mousePosition.y);}
		if (!mouseOver && (wasHover || wasDownOver))		{mouseLeaveSignal(mousePosition.x, mousePosition.y);}
	}

	if(UserInputService* inputService = RBX::ServiceProvider::find<UserInputService>(this))
	{
		if (eventLeftButtonDown && mouseOver)
		{
			inputService->setLastDownGuiObject(weak_from(this), InputObject::TYPE_MOUSEBUTTON1);
		}
		else if (eventRightButtonDown && mouseOver)
		{
			inputService->setLastDownGuiObject(weak_from(this), InputObject::TYPE_MOUSEBUTTON2);
		}
	}

	if(UserInputService* inputService = RBX::ServiceProvider::find<UserInputService>(this))
	{
		if (getActive() && getDraggable() && inputService->getMouseEnabled() && !inputService->getTouchEnabled())
		{
			handleDragging(event);
		}
	}
    
    // 2. Do State Table
	Gui::WidgetState oldState = guiState;
	guiState = Gui::NOTHING;
	switch (oldState)
	{
		case Gui::NOTHING:
		case Gui::HOVER:
		{
			if (mouseOver && eventButtonDown) {
				guiState = Gui::DOWN_OVER;
			}
			else if (mouseOver) {
				guiState = Gui::HOVER;
			}
			break;
		}
		case Gui::DOWN_OVER:
		case Gui::DOWN_AWAY:
		{
			if (mouseOver && (eventButtonDown || eventMoved)) {
				guiState = Gui::DOWN_OVER;
			}
			else if (!mouseOver && (eventButtonDown || eventMoved))	{
				guiState = Gui::DOWN_AWAY;
			}
			else if (event->getUserInputType() != lastMouseDownType)
			{
				guiState = Gui::DOWN_AWAY;
			}
			break;
		}
	}

	if (eventButtonDown && mouseOver)
	{
		lastMouseDownType = event->getUserInputType();
	}
    
    return (mouseOver && getActive()) ? GuiResponse::notSunkMouseWasOver() : GuiResponse::notSunk();
}

GuiResponse GuiObject::processMouseEvent(const shared_ptr<InputObject>& event)
{
	return processMouseEventInternal(event, true);
}

void GuiObject::draggingEnded(const shared_ptr<Instance>& event)
{
	if (InputObject* inputObject = Instance::fastDynamicCast<InputObject>(event.get()))
	{
		if (draggingBeganInputObject.get() == inputObject)
		{
			draggingEndedConnection.disconnect();
			Vector2 mousePosition = inputObject->get2DPosition();
			Vector2 dragPosition = Vector2(mousePosition.x, mousePosition.y);
			if (GuiService* guiService = RBX::ServiceProvider::find<RBX::GuiService>(this))
			{
				dragPosition -= guiService->getGlobalGuiInset().xy();
			}
			dragStoppedSignal(dragPosition.x, dragPosition.y);
			draggingBeganInputObject.reset();
			dragging = false;
		}
	}
}

bool GuiObject::isCurrentlyVisible()
{
	// Huh? This line of code doesn't make sense to me:  -Erik
	// this code is used to see if this object is being rendered, so
	// if the object itself, or any of its parents are not not set to render
	// we should return false
	if(getVisible() == false)
		return false;

	Rect2D clippedRect = getRect2D();

	if (clippedRect.wh().length() > 0.0f)
	{
		if (GuiObject* clippingObject = firstAncestorClipping())
		{
			clippedRect = clippedRect.intersect(clippingObject->getClippedRect());
			if (clippedRect.wh() == Vector2::zero())
			{
				return false;
			}
		}

		if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
		{
			Vector2 screenRes = guiService->getScreenResolution();
			if (!clippedRect.intersects(Rect2D::xywh(Vector2::zero() + guiService->getGlobalGuiInset().xy(),screenRes)))
				return false;
		}
	}

	if (GuiObject* parentGuiObject = Instance::fastDynamicCast<GuiObject>(getParent()))
		return parentGuiObject->isCurrentlyVisible();
	
	if (Instance::fastDynamicCast<RBX::ScreenGui>(getParent()))
		return true;
	
	return false;
}


bool GuiObject::isSelectedObject()
{
	if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
	{
		if (GuiObject* selectedGuiObject = guiService->getSelectedGuiObject())
		{
			return selectedGuiObject == this;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


const char* const  sGuiButton = "GuiButton";

REFLECTION_BEGIN();
static const Reflection::BoundFuncDesc<GuiButton, void(std::string)> func_setVerb(&GuiButton::setVerb,"SetVerb", "verb", Security::RobloxScript);

static Reflection::RemoteEventDesc<GuiButton, void()>		   event_MouseButton1Click(&GuiButton::mouseButton1ClickSignal, "MouseButton1Click", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiButton, void()>		   event_MouseButton2Click(&GuiButton::mouseButton2ClickSignal, "MouseButton2Click", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiButton, void(int, int)> event_MouseButton1Down (&GuiButton::mouseButton1DownSignal,  "MouseButton1Down", "x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiButton, void(int, int)> event_MouseButton1Up	  (&GuiButton::mouseButton1UpSignal,	"MouseButton1Up",	"x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiButton, void(int, int)> event_MouseButton2Down (&GuiButton::mouseButton2DownSignal,	"MouseButton2Down", "x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<GuiButton, void(int, int)> event_MouseButton2Up	  (&GuiButton::mouseButton2UpSignal,	"MouseButton2Up",	"x", "y", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
IMPLEMENT_EVENT_REPLICATOR(GuiButton,event_MouseButton1Click,"MouseButton1Click",MouseButton1Click);
IMPLEMENT_EVENT_REPLICATOR(GuiButton,event_MouseButton2Click,"MouseButton2Click",MouseButton2Click);
IMPLEMENT_EVENT_REPLICATOR(GuiButton,event_MouseButton1Down, "MouseButton1Down", MouseButton1Down);
IMPLEMENT_EVENT_REPLICATOR(GuiButton,event_MouseButton1Up,   "MouseButton1Up",	 MouseButton1Up);
IMPLEMENT_EVENT_REPLICATOR(GuiButton,event_MouseButton2Down, "MouseButton2Down", MouseButton2Down);
IMPLEMENT_EVENT_REPLICATOR(GuiButton,event_MouseButton2Up,	 "MouseButton2Up",	 MouseButton2Up);

static const Reflection::PropDescriptor<GuiButton, bool> prop_AutoButtonColor("AutoButtonColor", category_Data, &GuiButton::getAutoButtonColor, &GuiButton::setAutoButtonColor);
static const Reflection::PropDescriptor<GuiButton, bool> prop_Selected("Selected", category_Data, &GuiButton::getSelected, &GuiButton::setSelected);
const Reflection::PropDescriptor<GuiButton, bool> GuiButton::prop_Modal("Modal", category_Data, &GuiButton::getModal, &GuiButton::setModal);
static const Reflection::EnumPropDescriptor<GuiButton, GuiButton::Style> prop_Style("Style", category_Data, &GuiButton::getStyle, &GuiButton::setStyle);
REFLECTION_END();


namespace Reflection {
template<>
EnumDesc<GuiButton::Style>::EnumDesc()
	:EnumDescriptor("ButtonStyle")
{
	addPair(GuiButton::CUSTOM_STYLE, "Custom");
	addPair(GuiButton::ROBLOX_RED_STYLE, "RobloxButtonDefault");
	addPair(GuiButton::ROBLOX_GREY_STYLE, "RobloxButton");
	addPair(GuiButton::ROBLOX_BUTTON_ROUND_STYLE, "RobloxRoundButton");
	addPair(GuiButton::ROBLOX_BUTTON_ROUND_DEFAULT_STYLE, "RobloxRoundDefaultButton");
	addPair(GuiButton::ROBLOX_BUTTON_ROUND_DROPDOWN_STYLE, "RobloxRoundDropdownButton");
}
}//namespace Reflection

GuiButton::GuiButton(const char* name)
	: DescribedNonCreatable<GuiButton,GuiObject,sGuiButton>(name, true)
	, autoButtonColor (true)
	, selected(false)
	, clicked(false)
	, modal(false)
	, style(CUSTOM_STYLE)
	, verb(NULL)
	, verbToSet("")
    , shouldFireClickedEvent(true)
	, CONSTRUCT_EVENT_REPLICATOR(GuiButton,GuiButton::mouseButton1ClickSignal,	event_MouseButton1Click,MouseButton1Click)
	, CONSTRUCT_EVENT_REPLICATOR(GuiButton,GuiButton::mouseButton2ClickSignal, 	event_MouseButton2Click,MouseButton2Click)
	, CONSTRUCT_EVENT_REPLICATOR(GuiButton,GuiButton::mouseButton1DownSignal,	event_MouseButton1Down, MouseButton1Down)
	, CONSTRUCT_EVENT_REPLICATOR(GuiButton,GuiButton::mouseButton1UpSignal, 	event_MouseButton1Up,   MouseButton1Up)
	, CONSTRUCT_EVENT_REPLICATOR(GuiButton,GuiButton::mouseButton2DownSignal,	event_MouseButton2Down, MouseButton2Down)
	, CONSTRUCT_EVENT_REPLICATOR(GuiButton,GuiButton::mouseButton2UpSignal, 	event_MouseButton2Up,	MouseButton2Up)
{
	selectable = true;
	CONNECT_EVENT_REPLICATOR(MouseButton1Click);
	CONNECT_EVENT_REPLICATOR(MouseButton2Click);
	CONNECT_EVENT_REPLICATOR(MouseButton1Down);
	CONNECT_EVENT_REPLICATOR(MouseButton1Up);
	CONNECT_EVENT_REPLICATOR(MouseButton2Down);
	CONNECT_EVENT_REPLICATOR(MouseButton2Up);
}

void GuiButton::setServerGuiObject()
{
	Super::setServerGuiObject();

	eventReplicatorMouseButton1Click	.setListenerMode(!mouseButton1ClickSignal.empty());
	eventReplicatorMouseButton2Click	.setListenerMode(!mouseButton2ClickSignal.empty());
	eventReplicatorMouseButton1Down		.setListenerMode(!mouseButton1DownSignal.empty());
	eventReplicatorMouseButton1Up		.setListenerMode(!mouseButton1UpSignal.empty());
	eventReplicatorMouseButton2Down		.setListenerMode(!mouseButton2DownSignal.empty());
	eventReplicatorMouseButton2Up		.setListenerMode(!mouseButton2UpSignal.empty());
}
void GuiButton::onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
{
	Super::onPropertyChanged(descriptor);

	eventReplicatorMouseButton1Click.onPropertyChanged(descriptor);
	eventReplicatorMouseButton2Click.onPropertyChanged(descriptor);
	eventReplicatorMouseButton1Down.onPropertyChanged(descriptor);
	eventReplicatorMouseButton1Up.onPropertyChanged(descriptor);
	eventReplicatorMouseButton2Down.onPropertyChanged(descriptor);
	eventReplicatorMouseButton2Up.onPropertyChanged(descriptor);
}

Rect2D GuiButton::getChildRect2D() const 
{	
	switch(style)
	{
		case GuiButton::CUSTOM_STYLE:
			return getRect2D();
		case GuiButton::ROBLOX_RED_STYLE:
		case GuiButton::ROBLOX_GREY_STYLE:
			return Scale9Rect2D(getRect2D(), 12, 36);
		case GuiButton::ROBLOX_BUTTON_ROUND_STYLE:
		case GuiButton::ROBLOX_BUTTON_ROUND_DEFAULT_STYLE:
		case GuiButton::ROBLOX_BUTTON_ROUND_DROPDOWN_STYLE:
			return Scale9Rect2D(getRect2D(), 12, 26);
		default:
			RBXASSERT(0);
			return getRect2D();
	}
}

void GuiButton::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider, newProvider);

	if(newProvider && verbToSet != "") 
		setVerb(verbToSet);
}

void GuiButton::setVerb(std::string verbString)
{
    // People were modifying verbString when it was loaded into a register by using CE.
    VMProtectBeginMutation(NULL);
	if(Workspace* workspace = ServiceProvider::find<Workspace>(this))
	{
#ifdef RBX_STUDIO_BUILD
        verb = workspace->getVerb(verbString);
#else
        verb = workspace->getWhitelistVerb(verbString);
        if (verb && verb->getVerbSecurity())
        {
            RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag10, HATE_VERB_SNATCH);
            verb = NULL;
        }
#endif
		verbToSet = "";
	}
	else
		verbToSet = verbString; // our object isn't in the workspace yet, save string for later use (when we reset our parent)
    VMProtectEnd();
}

void GuiButton::setModal(bool value)
{
	if(modal != value)
	{
		modal = value;
		raisePropertyChanged(prop_Modal);
	}
}

void GuiButton::setStyle(Style value)
{
	if(style != value){
		style = value;
		raisePropertyChanged(prop_Style);

		switch(style)
		{
			case GuiButton::CUSTOM_STYLE:
				images.reset();
				forceResize();
				break;
			case GuiButton::ROBLOX_RED_STYLE:
			case GuiButton::ROBLOX_GREY_STYLE:
			case GuiButton::ROBLOX_BUTTON_ROUND_STYLE:
			case GuiButton::ROBLOX_BUTTON_ROUND_DEFAULT_STYLE:
			case GuiButton::ROBLOX_BUTTON_ROUND_DROPDOWN_STYLE:
				if(!images.get())
				{
					images.reset(new GuiDrawImage[4]);
				}
				forceResize();
				break;
		}
	}
}

void GuiButton::setAutoButtonColor(bool value)
{
	if(autoButtonColor != value){
		autoButtonColor = value;
		raisePropertyChanged(prop_AutoButtonColor);
	}
}

void GuiButton::setSelected(bool value)
{
	if(selected != value)
	{
		selected = value;
		raisePropertyChanged(prop_Selected);
	}
}

void GuiButton::render2dButtonImpl(Adorn* adorn, Rect2D& rect)
{
	static const TextureId sGreyTransp =	TextureId("rbxasset://textures/ui/btn_greyTransp.png");
	static const TextureId sGrey =			TextureId("rbxasset://textures/ui/btn_grey.png");
	static const TextureId sWhite =			TextureId("rbxasset://textures/ui/btn_white.png");
	static const TextureId sRed	=			TextureId("rbxasset://textures/ui/btn_red.png");
	static const TextureId sRedGlow =		TextureId("rbxasset://textures/ui/btn_redGlow.png");
	static const TextureId sNewGrey	=		TextureId("rbxasset://textures/ui/btn_newGrey.png");
	static const TextureId sNewGreyGlow =	TextureId("rbxasset://textures/ui/btn_newGreyGlow.png");
	static const TextureId sNewBlue	=		TextureId("rbxasset://textures/ui/btn_newBlue.png");
	static const TextureId sNewBlueGlow =	TextureId("rbxasset://textures/ui/btn_newBlueGlow.png");
	static const TextureId sNewWhite	=	TextureId("rbxasset://textures/ui/btn_newWhite.png");
	static const TextureId sNewWhiteGlow =	TextureId("rbxasset://textures/ui/btn_newWhiteGlow.png");

	GuiObject* clippingObject = firstAncestorClipping();

	switch(style)
	{
		case CUSTOM_STYLE:
		{
			Color4 renderColor = getRenderBackgroundColor4();

			if (getAutoButtonColor()) {
				Color3 myBackColor = getBackgroundColor3();

				switch (guiState)
				{
					case Gui::HOVER:		renderColor = Color4(myBackColor.lerp(Color3::black(), 0.3f), renderColor.a);		break;
					case Gui::DOWN_OVER:	renderColor = Color4(myBackColor.lerp(Color3::white(), 0.3f), renderColor.a);		break;
					case Gui::DOWN_AWAY:	renderColor = Color4(myBackColor.lerp(Color3::black(), 0.3f), renderColor.a);		break;
					case Gui::NOTHING:	break;
				}
			}

			Super::render2dImpl(adorn, renderColor, rect);
			return;
		}
		case ROBLOX_GREY_STYLE:
		{
			if(getAutoButtonColor())
			{
				if (!getActive()) {
					setBackgroundTransparency(0.0f);
					render2dScale9Impl(adorn, sGreyTransp, Vector2int16(8,8), Vector2int16(36,36), images[0], rect, clippingObject);
				}
				else if (!getSelected()) {
					switch(guiState)
					{
					case Gui::NOTHING:
					case Gui::DOWN_AWAY:
						setBackgroundTransparency(0.0f);
						render2dScale9Impl(adorn, sGrey, Vector2int16(8,8), Vector2int16(36,36), images[1], rect, clippingObject);
						break;
					case Gui::HOVER:
					case Gui::DOWN_OVER:					
						setBackgroundTransparency(0.0f);
						render2dScale9Impl(adorn, sWhite, Vector2int16(8,8), Vector2int16(36,36), images[2], rect, clippingObject);
						break;
					}
				}
				else {
					switch(guiState)
					{
					case Gui::NOTHING:
					case Gui::HOVER:
					case Gui::DOWN_AWAY:
					case Gui::DOWN_OVER:
						setBackgroundTransparency(0.0f);
						render2dScale9Impl(adorn, sRed, Vector2int16(8,8), Vector2int16(36,36), images[3], rect, clippingObject);
						break;
					}
				}
			}
			else
			{
				RBXASSERT(images.get());
				setBackgroundTransparency(0.0f);
				render2dScale9Impl(adorn, sGrey, Vector2int16(8,8), Vector2int16(36,36), images[1], rect, clippingObject);
			}
			rect = getChildRect2D();
			break;
		}
		case ROBLOX_RED_STYLE:
		{
			if(getAutoButtonColor())
			{
				if (!getActive()) {
					setBackgroundTransparency(0.0f);
					render2dScale9Impl(adorn, sGreyTransp, Vector2int16(8,8), Vector2int16(36,36), images[0], rect, clippingObject);
				}
				else if (!getSelected()) {
					switch(guiState)
					{
					case Gui::NOTHING:
					case Gui::DOWN_AWAY:
						setBackgroundTransparency(0.0f);
						render2dScale9Impl(adorn, sRed, Vector2int16(8,8), Vector2int16(36,36), images[1], rect, clippingObject);
						break;
					case Gui::HOVER:
					case Gui::DOWN_OVER:					
						setBackgroundTransparency(0.0f);
						render2dScale9Impl(adorn, sRedGlow, Vector2int16(8,8), Vector2int16(36,36), images[2], rect, clippingObject);
						break;
					}
				}
				else {
					switch(guiState)
					{
					case Gui::NOTHING:
					case Gui::HOVER:
					case Gui::DOWN_AWAY:
					case Gui::DOWN_OVER:
						setBackgroundTransparency(0.0f);
						render2dScale9Impl(adorn, sRedGlow, Vector2int16(8,8), Vector2int16(36,36), images[3], rect, clippingObject);
						break;
					}
				}
			}
			else
			{
				RBXASSERT(images.get());
				setBackgroundTransparency(0.0f);
				render2dScale9Impl(adorn, sRed, Vector2int16(8,8), Vector2int16(36,36), images[1], rect, clippingObject);
			}
			rect = getChildRect2D();
			break;
		}
		case ROBLOX_BUTTON_ROUND_STYLE:
		case ROBLOX_BUTTON_ROUND_DEFAULT_STYLE:
		case ROBLOX_BUTTON_ROUND_DROPDOWN_STYLE:
		{
			const TextureId *pNormal = NULL;
			const TextureId *pButtonPressed = NULL;
			switch (style) {
				case ROBLOX_BUTTON_ROUND_DEFAULT_STYLE:
					pNormal = &sNewBlue;
					pButtonPressed = &sNewBlueGlow;			
					break;
				default:
				case ROBLOX_BUTTON_ROUND_STYLE:
					pNormal = &sNewGrey;
					pButtonPressed = &sNewGreyGlow;			
					break;
				case ROBLOX_BUTTON_ROUND_DROPDOWN_STYLE:
					pNormal = &sNewWhite;
					pButtonPressed = &sNewWhiteGlow;			
					break;
			}
			Vector2int16 cornerSize = Vector2int16(6,6);
			Vector2int16 minSize = Vector2int16(20,20);

			if(getAutoButtonColor())
			{
				if (!getActive()) {
					setBackgroundTransparency(0.0f);
					render2dScale9Impl(adorn, sNewGrey, cornerSize, minSize, images[0], rect, clippingObject);
				}
				else if (!getSelected()) {
					switch(guiState)
					{
					case Gui::NOTHING:
					case Gui::DOWN_AWAY:
						setBackgroundTransparency(0.0f);
						render2dScale9Impl(adorn, *pNormal, cornerSize, minSize, images[1], rect, clippingObject);
						break;
					case Gui::HOVER:
					case Gui::DOWN_OVER:					
						setBackgroundTransparency(0.0f);
						render2dScale9Impl(adorn, *pButtonPressed, cornerSize, minSize, images[2], rect, clippingObject);
						break;
					}
				}
				else {
					switch(guiState)
					{
					case Gui::NOTHING:
					case Gui::HOVER:
					case Gui::DOWN_AWAY:
					case Gui::DOWN_OVER:
						setBackgroundTransparency(0.0f);
						render2dScale9Impl(adorn, *pButtonPressed, cornerSize, minSize, images[3], rect, clippingObject);
						break;
					}
				}
			}
			else
			{
				RBXASSERT(images.get());
				setBackgroundTransparency(0.0f);
				render2dScale9Impl(adorn, *pNormal, cornerSize, minSize, images[1], rect, clippingObject);
			}
			rect = getChildRect2D();
			break;
		}
	}

	renderStudioSelectionBox(adorn);
}

GuiResponse GuiButton::processTouchEvent(const shared_ptr<InputObject>& event)
{
	shouldFireClickedEvent = true;
        
	if ( event->isTouchEvent() && mouseIsOver(event->get2DPosition()) && getActive() && !getDraggable())
	{
		if( RBX::ScrollingFrame* scrollFrame = findFirstAncestorOfType<RBX::ScrollingFrame>())
		{
			bool processedInput = scrollFrame->processInputFromDescendant(event);
			shouldFireClickedEvent = !scrollFrame->isTouchScrolling();

			if (processedInput)
			{
				return GuiResponse::sunk();
			}
		}
	}

	return Super::processTouchEvent(event);
}

GuiResponse GuiButton::checkForSelectedObjectClick(const shared_ptr<InputObject>& event)
{
	if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
	{
		if (GuiObject* selectedGuiObject = guiService->getSelectedGuiObject())
		{
			if (selectedGuiObject == this)
			{
				Rect2D rect = getRect2D();
				Vector2 pos = Vector2(rect.x0() + rect.width()/2.0f, rect.y0() + rect.height()/2.0f);

                if (event->getUserInputState() == InputObject::INPUT_STATE_BEGIN)
                {
					lastSelectedObjectEvent = event;
                    mouseButton1DownSignal(pos.x, pos.y);
                }
                if (event->getUserInputState() == InputObject::INPUT_STATE_END)
                {
                    mouseButton1UpSignal(pos.x, pos.y);
					shared_ptr<InputObject> lastSharedObj = lastSelectedObjectEvent.lock();
                    if (event == lastSharedObj)
                    {
                        mouseButton1ClickSignal();

						VMProtectBeginMutation(NULL);
						if (verb != NULL)
						{
							clicked = false;
							if (verb->isEnabled())
							{
								Verb::doItWithChecks(verb, RBX::DataModel::get(this));
							}
						}
						VMProtectEnd();
						lastSelectedObjectEvent = weak_ptr<InputObject>();
                    }
				}
				return GuiResponse::sunk();
			}
		}
	}

	return GuiResponse::notSunk();
}

GuiResponse GuiButton::processKeyEvent(const shared_ptr<InputObject>& event)
{
	// todo: don't hard code selection key? maybe this is ok
	if (event->getKeyCode() == SDLK_RETURN)
	{
		GuiResponse response = checkForSelectedObjectClick(event);
		if (response.wasSunk())
		{
			return response;
		}
	}

	return Super::processKeyEvent(event);
}

GuiResponse GuiButton::processGamepadEvent(const shared_ptr<InputObject>& event)
{
	// todo: don't hard code selection button? maybe this is ok
	if (event->getKeyCode() == SDLK_GAMEPAD_BUTTONA)
	{
		GuiResponse response = checkForSelectedObjectClick(event);
		if (response.wasSunk())
		{
			return response;
		}
	}

	return Super::processGamepadEvent(event);
}

GuiResponse GuiButton::processMouseEvent(const shared_ptr<InputObject>& event)
{
    Vector2 mousePosition = event->get2DPosition();
    
    bool mouseOver = mouseIsOver(mousePosition);
    bool eventLeftButtonDown = event->isLeftMouseDownEvent();
    bool eventLeftButtonUp = event->isLeftMouseUpEvent();
    bool eventRightButtonDown = event->isRightMouseDownEvent();
    bool eventRightButtonUp = event->isRightMouseUpEvent();
    bool wasDownOver = (guiState == Gui::DOWN_OVER);
    bool sinkEvent = false;

	if (eventLeftButtonUp && wasDownOver && shouldFireClickedEvent)		{sinkEvent = true;	clicked = true; mouseButton1ClickSignal();}
	if (eventRightButtonUp && wasDownOver && shouldFireClickedEvent)	{sinkEvent = true;	mouseButton2ClickSignal();}
	if (mouseOver && eventLeftButtonDown)								{sinkEvent = true;	mouseButton1DownSignal(mousePosition.x, mousePosition.y);}
	if (mouseOver && eventRightButtonDown)								{sinkEvent = true;	mouseButton2DownSignal(mousePosition.x, mousePosition.y);}
	if (mouseOver && eventLeftButtonUp)									{sinkEvent = true;	mouseButton1UpSignal(mousePosition.x, mousePosition.y);}
	if (mouseOver && eventRightButtonUp)								{sinkEvent = true;	mouseButton2UpSignal(mousePosition.x, mousePosition.y);}
    
    GuiResponse answer = Super::processMouseEvent(event);			//We need to call our classParent to deal with the state table
    
    VMProtectBeginMutation(NULL);
    if (verb != NULL && clicked)
    {
        clicked = false;
        if (verb->isEnabled())
        {
            Verb::doItWithChecks(verb, RBX::DataModel::get(this));
        }
    }
    VMProtectEnd();

    if (sinkEvent) 
	{
        return GuiResponse::sunk();
    }

	return answer;	// used(this) will set a target and sink this event at the PlayerGui processing level - no other GuiObjects will see it.
}



////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


const char* const  sGuiLabel = "GuiLabel";

GuiLabel::GuiLabel(const char* name)
	: DescribedNonCreatable<GuiLabel,GuiObject,sGuiLabel>(name, false)
{}




}
