/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Mouse.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/MouseCommand.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/GuiService.h"
#include "v8datamodel/UserInputService.h"

namespace RBX {


const char *const sMouse = "Mouse";

REFLECTION_BEGIN();
static Reflection::EventDesc<Mouse, void() > desc_Mouse_Move(&Mouse::moveSignal, "Move");
static Reflection::EventDesc<Mouse, void() > desc_Mouse_Idle(&Mouse::idleSignal, "Idle");
static Reflection::EventDesc<Mouse, void() > desc_Mouse_Button1Down(&Mouse::button1DownSignal, "Button1Down");
static Reflection::EventDesc<Mouse, void() > desc_Mouse_Button2Down(&Mouse::button2DownSignal, "Button2Down");
static Reflection::EventDesc<Mouse, void() > desc_Mouse_Button1Up(&Mouse::button1UpSignal, "Button1Up");
static Reflection::EventDesc<Mouse, void() > desc_Mouse_Button2Up(&Mouse::button2UpSignal, "Button2Up");
static Reflection::EventDesc<Mouse, void() > desc_Mouse_WheelForward(&Mouse::wheelForwardSignal, "WheelForward");
static Reflection::EventDesc<Mouse, void() > desc_Mouse_WheelBackward(&Mouse::wheelBackwardSignal, "WheelBackward");
static Reflection::EventDesc<Mouse, void(std::string) > desc_Mouse_KeyDown(&Mouse::keyDownSignal, "KeyDown", "key", Reflection::Descriptor::Attributes::deprecated(UserInputService::event_InputBegin));
static Reflection::EventDesc<Mouse, void(std::string) > dep_Mouse_KeyDown(&Mouse::keyDownSignal, "keyDown", "key", Reflection::Descriptor::Attributes::deprecated(UserInputService::event_InputBegin));
static Reflection::EventDesc<Mouse, void(std::string) > desc_Mouse_KeyUp(&Mouse::keyUpSignal, "KeyUp", "key", Reflection::Descriptor::Attributes::deprecated(UserInputService::event_InputEnd));

static Reflection::PropDescriptor<Mouse, G3D::CoordinateFrame> desc_Hit("Hit", category_Data, &Mouse::getHit, NULL);
static Reflection::PropDescriptor<Mouse, G3D::CoordinateFrame> dep_Hit("hit", category_Data, &Mouse::getHit, NULL, Reflection::PropertyDescriptor::Attributes::deprecated(desc_Hit, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
static Reflection::PropDescriptor<Mouse, G3D::CoordinateFrame> desc_Origin("Origin", category_Data, &Mouse::getOrigin, NULL);
static Reflection::PropDescriptor<Mouse, RBX::RbxRay> desc_UnitRay("UnitRay", category_Data, &Mouse::getUnitRay, NULL);
static Reflection::RefPropDescriptor<Mouse, Instance> desc_TargetFilter("TargetFilter", category_Data, &Mouse::getTargetFilterUnsafe, &Mouse::setTargetFilter);
static Reflection::RefPropDescriptor<Mouse, PartInstance> desc_Target("Target", category_Data, &Mouse::getTarget, NULL);
static Reflection::RefPropDescriptor<Mouse, PartInstance> dep_Target("target", category_Data, &Mouse::getTarget, NULL, Reflection::PropertyDescriptor::Attributes::deprecated(desc_Target));
static Reflection::EnumPropDescriptor<Mouse, NormalId> desc_TargetSurface("TargetSurface", category_Data, &Mouse::getTargetSurface, NULL);
static Reflection::PropDescriptor<Mouse, int> desc_Mouse_X("X", category_Data, &Mouse::getX, NULL);
static Reflection::PropDescriptor<Mouse, int> desc_Mouse_Y("Y", category_Data, &Mouse::getY, NULL);
static Reflection::PropDescriptor<Mouse, int> desc_Mouse_ViewSizeX("ViewSizeX", category_Data, &Mouse::getViewSizeX, NULL);
static Reflection::PropDescriptor<Mouse, int> desc_Mouse_ViewSizeY("ViewSizeY", category_Data, &Mouse::getViewSizeY, NULL);
static Reflection::PropDescriptor<Mouse, TextureId> desc_Mouse_Icon("Icon", category_Data, &Mouse::getIcon, &Mouse::setIcon);
REFLECTION_END();

Mouse::Mouse()
	: workspace(NULL)
	, command(NULL)
{
}

Mouse::~Mouse()
{
}

CoordinateFrame Mouse::getHit() const
{
	checkActive();
	CoordinateFrame cf(getOrigin());
	if (lastEvent)
	{		
		FilterDescendents filter(getTargetFilter());
		MouseCommand::getPartByLocalCharacter(getWorkspace(), lastEvent, &filter, cf.translation);
	}
	return cf;
}
RBX::RbxRay Mouse::getUnitRay() const
{
	checkActive();

	RBX::RbxRay ray;
	if (lastEvent && lastEvent->getUserInputState() != InputObject::INPUT_STATE_NONE)
	{
		ray = MouseCommand::getUnitMouseRay(lastEvent, getWorkspace());
	}

	return ray;
}
CoordinateFrame Mouse::getOrigin() const
{
	checkActive();
	RBX::RbxRay ray;
	if (lastEvent && lastEvent->getUserInputState() != InputObject::INPUT_STATE_NONE)
	{
		ray = MouseCommand::getUnitMouseRay(lastEvent, getWorkspace());
	}
	CoordinateFrame cf(ray.origin());
	cf.lookAt(ray.origin()+ray.direction());	// TODO: I wish that Matrix3 had a "lookAt" function
	return cf;
}

shared_ptr<Instance> Mouse::getTargetFilter() const
{
	checkActive();

	return targetFilter.lock();
}
void Mouse::setTargetFilter(Instance* value)
{
	setTargetFilterUnsafe(value);
}

void Mouse::setTargetFilterUnsafe(Instance* value)
{
	if (targetFilter.lock() != shared_from(value))
	{
		targetFilter = shared_from(value);
		raisePropertyChanged(desc_TargetFilter);
	}
}

PartInstance* Mouse::getTarget() const
{
	checkActive();

	FilterDescendents filter(getTargetFilter());
	return (lastEvent && lastEvent->getUserInputState() != InputObject::INPUT_STATE_NONE) ?
			MouseCommand::getPart(getWorkspace(), lastEvent, &filter) : NULL;
}

NormalId Mouse::getTargetSurface() const
{
	checkActive();

	FilterDescendents filter(getTargetFilter());
	Surface surface = (lastEvent && lastEvent->getUserInputState() != InputObject::INPUT_STATE_NONE) ?
			MouseCommand::getSurface(getWorkspace(), lastEvent, &filter) : Surface();
	return surface.getNormalId();
}

void Mouse::cacheInputObject(const shared_ptr<InputObject>& inputObject)
{
	lastEvent = Creatable<Instance>::create<InputObject>(*inputObject);
}

void Mouse::update(const shared_ptr<InputObject>& inputObject)
{
	switch (inputObject->getUserInputType())
	{
	case InputObject::TYPE_MOUSEMOVEMENT:
		cacheInputObject(inputObject);
		// TODO: rate control this event???
		moveSignal();
		break;
	case InputObject::TYPE_MOUSEIDLE:
		cacheInputObject(inputObject);
		idleSignal();
		break;
	case InputObject::TYPE_MOUSEBUTTON1:
		cacheInputObject(inputObject);
		if(inputObject->isLeftMouseDownEvent())
			button1DownSignal();
		else if(inputObject->isLeftMouseUpEvent())
			button1UpSignal();
		break;
	case InputObject::TYPE_MOUSEBUTTON2:
		cacheInputObject(inputObject);
		if(inputObject->isRightMouseDownEvent())
			button2DownSignal();
		else if(inputObject->isRightMouseUpEvent())
			button2UpSignal();
		break;
	case InputObject::TYPE_MOUSEWHEEL:
		if (inputObject->isMouseWheelForward())
			wheelForwardSignal();
		else if (inputObject->isMouseWheelBackward())
			wheelBackwardSignal();
		break;
	case InputObject::TYPE_KEYBOARD:
		// HACK ALERT! - Converting KeyCodes to ASCII this way only works for ASCII mapped keys!
		// See SDL_keysym.h

		if (inputObject->isKeyDownEvent()) // key down
			keyDownSignal(std::string(1, inputObject->getKeyCode()));
		else if (inputObject->isKeyUpEvent()) // key up
			keyUpSignal(std::string(1, inputObject->getKeyCode()));
		break;
    default:
        break;
	}
}

void Mouse::setWorkspace(Workspace* workspace)
{
	if (workspace != this->workspace)
	{
		if (UserInputService* userInputService = ServiceProvider::find<UserInputService>(this->workspace))
		{
			userInputService->popMouseIcon(icon);
		}
	}

	this->workspace = workspace;
}

TextureId Mouse::getIcon() const
{
	checkActive();

	return icon;
}
void Mouse::setIcon(const TextureId& value)
{
	if (icon != value)
	{
		if (UserInputService* userInputService = ServiceProvider::find<UserInputService>(workspace))
		{
			userInputService->popMouseIcon(icon);
			if (!value.toString().empty())
			{
				userInputService->pushMouseIcon(value);
			}
		}

		icon = value;
		raisePropertyChanged(desc_Mouse_Icon);
	}
}

int Mouse::getX() const
{
	checkActive();

    if (lastEvent)
    {
        return lastEvent->getPosition().x;
    }
    
    return -1;
}
int Mouse::getY() const
{
	checkActive();
    
    if (lastEvent)
    {
        return lastEvent->getPosition().y;
    }
    
    return -1;
}

int Mouse::getViewSizeX() const
{
	checkActive();

	if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(workspace))
	{
		return guiService->getScreenResolution().x - guiService->getGlobalGuiInset().x;
	}
    
    return 0;
}

int Mouse::getViewSizeY() const
{
	checkActive();

	if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(workspace))
	{
		return guiService->getScreenResolution().y - guiService->getGlobalGuiInset().y;
	}

    return 0;
}

void Mouse::checkActive() const {
	if (!workspace) 
	{
		RBXASSERT(0);
		throw std::runtime_error("This Mouse is no longer active");
	}
}

Workspace* Mouse::getWorkspace() const
{
	return workspace;
}


} // namespace
