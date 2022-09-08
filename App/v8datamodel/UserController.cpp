/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/UserController.h"
#include "V8DataModel/PVInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/VehicleSeat.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/UserInputService.h"
#include "Network/Players.h"
#include "Humanoid/Humanoid.h"
#include "V8tree/Service.h"
#include "Util/UserInputBase.h"
#include "Util/Math.h"
#include "Util/NavKeys.h"
#include "Gui/Widget.h"
#include "AppDraw/DrawPrimitives.h"
#include "Gui/GuiDraw.h"
#include "G3D/g3dmath.h"

LOGGROUP(UserInputProfile)

#define TOUCH_LEFT_RIGHT_STEER_MIN 0.2

namespace RBX {

const char* const  sControllerService = "ControllerService";
const char* const  sController = "Controller";
const char* const  sHumanoidController = "HumanoidController";
const char* const  sVehicleController = "VehicleController";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<Controller, void(Controller::Button, std::string)> func_BindButton(&Controller::bindButton, "BindButton", "button", "caption", Security::None);
static Reflection::BoundFuncDesc<Controller, void(Controller::Button, std::string)> dep_bindButton(&Controller::bindButton, "bindButton", "button", "caption", Security::None, Reflection::Descriptor::Attributes::deprecated(func_BindButton));
static Reflection::BoundFuncDesc<Controller, void(Controller::Button)> controller_unbindButton(&Controller::unbindButton, "UnbindButton", "button", Security::None);
static Reflection::BoundFuncDesc<Controller, bool(Controller::Button)> controller_getButton(&Controller::getButton, "GetButton", "button", Security::None);
static Reflection::BoundFuncDesc<Controller, bool(Controller::Button)> dep_getButton(&Controller::getButton, "getButton", "button", Security::None, Reflection::Descriptor::Attributes::deprecated(controller_getButton));

static Reflection::EventDesc<Controller, void(Controller::Button)>	event_ButtonChanged(&Controller::buttonChangedSignal, "ButtonChanged", "button");
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<Controller::Button>::EnumDesc()
:EnumDescriptor("Button")
{
	addPair(Controller::JUMP, "Jump");
	addPair(Controller::DISMOUNT, "Dismount");
}

template<>
Controller::Button& Variant::convert<Controller::Button>(void)
{
	return genericConvert<Controller::Button>();
}
}//namespace Reflection

template<>
bool RBX::StringConverter<Controller::Button>::convertToValue(const std::string& text, Controller::Button& value)
{
	if(text.find("Jump")){
		value = Controller::JUMP;
		return true;
	}
	if(text.find("Dismount")){
		value = Controller::DISMOUNT;
		return true;
	}
	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const UserInputBase* Controller::getHardwareDevice() const
{
	const ControllerService* controllerService = Instance::fastDynamicCast<ControllerService>(getParent());
	RBXASSERT(controllerService == ServiceProvider::find<ControllerService>(this));

	return controllerService ? controllerService->getHardwareDevice() : NULL;
}

extern const char *const sButtonBindingWidget = "ButtonBindingWidget";
class ButtonBindingWidget					 				// common root of Tool, HopperBin - stuff that can go in the hopper and on the player
	: public DescribedNonCreatable<ButtonBindingWidget, Widget, sButtonBindingWidget>
{
private:
	typedef DescribedNonCreatable<ButtonBindingWidget, Widget, sButtonBindingWidget> Super;
	GuiDrawImage guiImageDraw;
	TextureId textureId;
	Controller::Button button;
	weak_ptr<Controller> controller;

	// Instance
	/*override*/ bool askSetParent(const Instance* instance) const;
	/*override*/ bool askAddChild(const Instance* instance) const;
protected:

	// GuiItem
	/*override*/ Vector2 getSize(Canvas canvas) const;
	/*override*/ void render2d(Adorn* adorn);
	/*override*/ bool isEnabled() {return true;}

	/*override*/ void onClick(const shared_ptr<InputObject>& event);
public:
	
	static std::string getKeyName(Controller::Button button);

	ButtonBindingWidget(Controller::Button button, Controller* controller);
	void setTextureId(const TextureId& value);
	const TextureId getTextureId() const;

	virtual bool drawEnabled() const {return true;}
	virtual bool drawSelected() const {return false;}

};

RBX_REGISTER_CLASS(ButtonBindingWidget);

/////////////////////////////////////////////////////////////////////////////////////////
ButtonBindingWidget::ButtonBindingWidget(Controller::Button button, Controller* controller)
: button(button)
{
	this->controller = shared_from(controller);
}
void ButtonBindingWidget::onClick(const shared_ptr<InputObject>& event)
{
	shared_ptr<Controller> sp = controller.lock();
	if(sp)
	{
		sp->setButton(button, true);
		//sp->setButton(button, false); // note: find way to toggle.
	}
}

void ButtonBindingWidget::setTextureId(const TextureId& value)
{
	if (value != textureId) {
		textureId = value;

		//raisePropertyChanged(desc_TextureId);
	}
}

const TextureId ButtonBindingWidget::getTextureId() const
{
	return textureId;
}

bool ButtonBindingWidget::askAddChild(const Instance* instance) const
{
	return true;	// Anything is OK as a child
}

bool ButtonBindingWidget::askSetParent(const Instance* instance) const
{
	return true;
}

Vector2 ButtonBindingWidget::getSize(Canvas canvas) const
{
	float width = canvas.toPixelSize(Vector2(8, 8)).x;
	return Vector2(width, width);
}

std::string ButtonBindingWidget::getKeyName(Controller::Button button)
{
	switch(button)
	{
	case Controller::DISMOUNT:
		return "Backspace";
	default: return "";
	}
}

void ButtonBindingWidget::render2d(Adorn* adorn)
{
    if (UserInputService* userInputService = ServiceProvider::find<UserInputService>(this))
        if(!userInputService->getKeyboardEnabled())
            return; // rendering is only for keyboard shortcuts
    
	bool viewHack = false;
	if (Workspace* workspace = ServiceProvider::find<Workspace>(this)) {
		if (workspace->imageServerViewHack) {
			viewHack = true;
		}
	}

	Rect draw = viewHack ? adorn->getCanvas().size : getMyRect(adorn->getCanvas());
	draw = Rect::fromLowSize(Vector2(0,0),Vector2(200,30));

	// compute font size
	int fontSize = adorn->getCanvas().normalizedFontSize(14);


	// Translucent background
	if (!viewHack)
	{
		adorn->rect2d(draw.toRect2D(), Color4(0.0f,0.0f,0.0f,0.5f));
	}

	std::string keyName = getKeyName(button);
    
    if (isEnabled() && keyName != "")
    {
        // Number
        std::string text = "Press " + keyName + " to " + getTitle();
        adorn->drawFont2D(
                    text,
                    draw.center(),
                    (float)fontSize,
                    false,
                    Color3::white(),
                    Color4::clear(),
                    Text::FONT_ARIALBOLD,
                    Text::XALIGN_CENTER,
                    Text::YALIGN_CENTER );
    }

	if (drawSelected()) 
	{
		adorn->outlineRect2d(
							draw.toRect2D(), 
							3,
							Color3::green());
	}

	// Draw image or command name
	/*if (guiImageDraw.setImage(adorn, textureId, GuiDrawImage::ALL)) 
	{
		guiImageDraw.render2d(adorn, (isEnabled() || viewHack), draw.inset(fontSize), widgetState, drawSelected());
	}
	else 
	{
		Super::render2d(adorn);		// widget rendering
	}*/
}


///////////////////////////////////////////////////////////////////////////////////////////////////
Controller::Controller()
: IStepped(StepType_HighPriority),
  dataModel(NULL)
{
	FASTLOG1(FLog::ISteppedLifetime, "Controller created - %p", this);
}

Controller::~Controller()
{
	FASTLOG1(FLog::ISteppedLifetime, "Controller destroyed - %p", this);
}

bool Controller::isButtonBound(Button button) const
{
	return boundButtons.find(button) != boundButtons.end();
}

std::string Controller::getButtonCaption(Button button) const
{
	BoundButtonSet::const_iterator it = boundButtons.find(button);
	if(it != boundButtons.end())
	{
		return it->second.caption;
	}
	else
	{
		throw std::runtime_error("Button has not been bound. Call BindButton first."); 
	}
}

bool Controller::getButton(Button button) const
{
	BoundButtonSet::const_iterator it = boundButtons.find(button);
	if(it != boundButtons.end())
	{
		return it->second.pressed;
	}
	else
	{
		throw std::runtime_error("Button has not been bound. Call BindButton first."); 
	}
}

bool Controller::getButton(Button button)
{
	return (const_cast<const Controller*>(this))->getButton(button);
}


void Controller::bindButton(Button button, std::string caption)
{
	boundButtons.insert(std::make_pair(button, BoundButton(button, false, caption)));
}

void Controller::unbindButton(Button button)
{
	BoundButtonSet::iterator it = boundButtons.find(button);
	if(it != boundButtons.end())
	{
		it->second.guiWidget->setParent(NULL);
		boundButtons.erase(it);
	}
}

void Controller::setButton(Button button, bool value)
{
	BoundButtonSet::iterator it = boundButtons.find(button);
	if(it != boundButtons.end())
	{
		if(value != it->second.pressed)
		{
			it->second.pressed = value;
			buttonChangedSignal(button);
		}
	} 
	// else ignore.
}

// Into / out of the datamodel
void Controller::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);		// Note - moved this to first

	DataModel* newDataModel = DataModel::get(this);

	if (newDataModel != dataModel) 
	{
		dataModel = newDataModel;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

VehicleController::VehicleController()
{}

void VehicleController::setVehicleSeat(VehicleSeat* value)
{
	RBXASSERT(vehicleSeat.expired());

	vehicleSeat = shared_from<VehicleSeat>(value);
}
    
void VehicleController::onSteppedKeyboardInput(shared_ptr<VehicleSeat> seat)
{
    const UserInputBase* hardwareDevice = getHardwareDevice();
    if (!hardwareDevice)
        return;
    
    NavKeys nav;
	if(DataModel* dm = DataModel::get(this))
		hardwareDevice->getNavKeys(nav,dm->getSharedSuppressNavKeys());
    
    int forwardBackward = nav.forwardBackwardArrow() + nav.forwardBackwardASDW();
    int leftRight = nav.leftRightArrow() + nav.leftRightASDW();
    seat->setThrottle(forwardBackward);
    seat->setSteer(-leftRight);			// vehicle - left == -1, right == 1
}
    
void VehicleController::onSteppedTouchInput(shared_ptr<VehicleSeat> seat)
{
    Humanoid* localHumanoid = seat->getLocalHumanoid();
    if(!localHumanoid)
        return;
    
    seat->setThrottle(0);
    seat->setSteer(0);
    
    UserInputService* userInputService = ServiceProvider::find<UserInputService>(this);
    
    
    // todo: remove getWalkDirection call when everything uses moveDirection
    Vector3 moveDirection = localHumanoid->getWalkDirection();
    Vector2 movementVector = Vector2::zero();
    
    if (moveDirection != Vector3::zero())
    {
        movementVector = userInputService->getInputWalkDirection();
        movementVector /= userInputService->getMaxInputWalkValue();
    }
    else
    {
        moveDirection = localHumanoid->getRawMovementVector();
        movementVector = Vector2(moveDirection.x,moveDirection.z);
    }
    
    if(moveDirection != Vector3::zero())
    {
        if( UserInputService* userInputService = ServiceProvider::find<UserInputService>(this) )
        {
            if( movementVector.y > 0)
                seat->setThrottle(-1);
            else if( movementVector.y < 0 )
                seat->setThrottle(1);
            
            float steerTolerance = TOUCH_LEFT_RIGHT_STEER_MIN;
            if(userInputService->getTouchEnabled())
                steerTolerance *= 4;
            
            if( movementVector.x < -steerTolerance)
                seat->setSteer(-1);
            else if( movementVector.x > steerTolerance)
                seat->setSteer(1);

        }
    }
}

void VehicleController::onStepped(const Stepped& event)
{
	shared_ptr<VehicleSeat> sharedSeat = vehicleSeat.lock();
	if (!sharedSeat)
    {
		RBXASSERT(0);
		return;
	}

    UserInputService* userInputService = ServiceProvider::find<UserInputService>(this);
    
    if(userInputService->getTouchEnabled())
        onSteppedTouchInput(sharedSeat);
    
    if(userInputService->getKeyboardEnabled())
        onSteppedKeyboardInput(sharedSeat);
}



///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////

HumanoidController::HumanoidController()
:	stepsRotating(0),
	panSensitivity(0.06f)
{}

void HumanoidController::rotateCam(int leftRight, Camera* camera, float gameStep)
{
	camera->panRadians(panSensitivity * leftRight * static_cast<float>(gameStep) * 30.0f);
	stepsRotating++;
}

bool HumanoidController::preventMovement( Humanoid* humanoid )
{
	// do not allow movement if player is
	//  - typing
	//  - sitting
	//  - on a platform
	return humanoid->getTyping() || 
		   humanoid->getSit() || 
		   humanoid->getPlatformStanding();
}

void HumanoidController::updateCamera( const Stepped& event, const NavKeys& nav )
{
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	Camera* camera = workspace->getCamera();
	
	// if we're in cam lock mode, rotate by ASDW
	//  otherwise, rotate by arrows
	int rotation = RBX::GameBasicSettings::singleton().camLockedInCamLockMode() ?
		rotation = nav.leftRightASDW() :
		rotation = nav.leftRightArrow();

	if( camera->getCameraType() != Camera::LOCKED_CAMERA && rotation != 0 )
	{
		// if we have a rotation, rotate by it
		rotateCam( rotation, camera, event.gameStep );
	}
	else
	{
		// if we didn't rotate, reset steps rotating
		stepsRotating = 0;
	}
}
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


ControllerService::ControllerService() 
: hardwareDevice(NULL) 
{
	shared_ptr<HumanoidController> humanoidController = Creatable<Instance>::create<HumanoidController>();
	humanoidController->setParent(this);
}




} // namespace
