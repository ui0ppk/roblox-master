//
//  UserInputService.cpp
//  App
//
//  Created by Ben Tkacheff on 8/28/12.
//
//
#include "stdafx.h"

#include "V8DataModel/UserInputService.h"
#include "V8DataModel/Bindable.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Filters.h"
#include "V8DataModel/ClickDetector.h"
#include "V8DataModel/MouseCommand.h"
#include "V8DataModel/TextBox.h"
#include "V8DataModel/PlayerGui.h"
#include "v8datamodel/UserController.h"
#include "v8datamodel/Frame.h"
#include "v8datamodel/GamepadService.h"
#include "v8datamodel/TouchInputService.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "v8datamodel/ScreenGui.h"
#include "reflection/object.h"
#include "Humanoid/Humanoid.h"
#include "Util/UserInputBase.h"
#include "Util/Quaternion.h"
#include "Util/NavKeys.h"

#include "rbx/Profiler.h"

#include <boost/algorithm/string.hpp>

#ifdef __APPLE__ // needed for pasting ability on OS X
#if !RBX_PLATFORM_IOS
#import <Cocoa/Cocoa.h>
#endif
#endif

LOGGROUP(UserInputProfile)

FASTFLAGVARIABLE(UseInGameTopBar, false)
FASTFLAGVARIABLE(MobileToggleChatVisibleIcon, false)
DYNAMIC_FASTINTVARIABLE(MoveInGameChatToTopPlaceId, 0)
FASTFLAGVARIABLE(LuaChatPhoneFontSize, false)

DYNAMIC_FASTINTVARIABLE(LuaChatFloodCheckMessages, 7)
DYNAMIC_FASTINTVARIABLE(LuaChatFloodCheckInterval, 15)
FASTFLAGVARIABLE(LuaChatFiltering, false)

DYNAMIC_FASTFLAGVARIABLE(GetFocusedTextBoxEnabled, false)

FASTFLAGVARIABLE(FlyCamOnRenderStep, false)

FASTFLAGVARIABLE(PlayerDropDownEnabled, false)

DYNAMIC_FASTFLAGVARIABLE(EnableShowStatsLua, false)

FASTFLAGVARIABLE(UserUseNewControlScript, false)

FASTFLAG(UserAllCamerasInLua)

namespace RBX {
    
    namespace Reflection {
        template<>
        EnumDesc<UserInputService::SwipeDirection>::EnumDesc()
        :EnumDescriptor("SwipeDirection")
        {
            addPair(UserInputService::DIRECTION_RIGHT, "Right");
            addPair(UserInputService::DIRECTION_LEFT, "Left");
            addPair(UserInputService::DIRECTION_UP, "Up");
            addPair(UserInputService::DIRECTION_DOWN, "Down");
            addPair(UserInputService::DIRECTION_NONE, "None");
        }
        
        template<>
        EnumDesc<UserInputService::Platform>::EnumDesc()
        :EnumDescriptor("Platform")
		{
            addPair(UserInputService::PLATFORM_WINDOWS, "Windows");
            addPair(UserInputService::PLATFORM_OSX, "OSX");
            addPair(UserInputService::PLATFORM_IOS, "IOS");
            addPair(UserInputService::PLATFORM_ANDROID, "Android");
			addPair(UserInputService::PLATFORM_XBOXONE, "XBoxOne");
			addPair(UserInputService::PLATFORM_PS4, "PS4");
			addPair(UserInputService::PLATFORM_PS3, "PS3");
			addPair(UserInputService::PLATFORM_XBOX360, "XBox360");
			addPair(UserInputService::PLATFORM_WIIU, "WiiU");
			addPair(UserInputService::PLATFORM_NX, "NX");
			addPair(UserInputService::PLATFORM_OUYA, "Ouya");
			addPair(UserInputService::PLATFORM_ANDROIDTV, "AndroidTV");
			addPair(UserInputService::PLATFORM_CHROMECAST, "Chromecast");
			addPair(UserInputService::PLATFORM_LINUX, "Linux");
			addPair(UserInputService::PLATFORM_STEAMOS, "SteamOS");
			addPair(UserInputService::PLATFORM_WEBOS, "WebOS");
			addPair(UserInputService::PLATFORM_DOS, "DOS");
			addPair(UserInputService::PLATFORM_BEOS, "BeOS");
			addPair(UserInputService::PLATFORM_UWP, "UWP");
            addPair(UserInputService::PLATFORM_NONE, "None");
        }

		template<>
		EnumDesc<UserInputService::MouseType>::EnumDesc()
			:EnumDescriptor("MouseBehavior")
		{
			addPair(UserInputService::MOUSETYPE_DEFAULT, "Default");
			addPair(UserInputService::MOUSETYPE_LOCKCENTER, "LockCenter");
			addPair(UserInputService::MOUSETYPE_LOCKCURRENT, "LockCurrentPosition");
		}

		template<>
		EnumDesc<UserInputService::OverrideMouseIconBehavior>::EnumDesc()
			:EnumDescriptor("OverrideMouseIconBehavior")
		{
			addPair(UserInputService::OVERRIDE_BEHAVIOR_NONE, "None");
			addPair(UserInputService::OVERRIDE_BEHAVIOR_FORCESHOW, "ForceShow");
			addPair(UserInputService::OVERRIDE_BEHAVIOR_FORCEHIDE, "ForceHide");
		}

        template<>
        EnumDesc<UserInputService::UserCFrame>::EnumDesc()
        :EnumDescriptor("UserCFrame")
        {
			addPair(UserInputService::USERCFRAME_HEAD, "Head");
            addPair(UserInputService::USERCFRAME_LEFTHAND, "LeftHand");
            addPair(UserInputService::USERCFRAME_RIGHTHAND, "RightHand");
        }
        
        template<>
		UserInputService::UserCFrame& Variant::convert<UserInputService::UserCFrame>(void)
        {
            return genericConvert<UserInputService::UserCFrame>();
        }
    } // namespace Reflection
    
    template<>
	bool StringConverter<UserInputService::UserCFrame>::convertToValue(const std::string& text, UserInputService::UserCFrame& value)
    {
		return Reflection::EnumDesc<UserInputService::UserCFrame>::singleton().convertToValue(text.c_str(), value);
    }

    const char* const sUserInputService = "UserInputService";
    
    boost::mutex UserInputService::InputEventsMutex;
    
	// turn this on to debug touch events
	// will draw a square on the screen for each touch event
    bool UserInputService::DrawTouchEvents = false;
    RBX::Color3 UserInputService::DrawTouchColor = RBX::Color3(1,0,0);
    RBX::Color3 UserInputService::DrawMoveColor = RBX::Color3(0,1,0);
    
    REFLECTION_BEGIN();
	// ROBLOX Only functions/events/props
	static Reflection::BoundFuncDesc<UserInputService, UserInputService::Platform(void)> func_getPlatform(&UserInputService::getPlatformLua, "GetPlatform", Security::RobloxScript);

	static Reflection::BoundFuncDesc<UserInputService, InputObject::UserInputType()> func_getLastInputType(&UserInputService::getLastInputType, "GetLastInputType", Security::None);
	static Reflection::EventDesc<UserInputService, void(InputObject::UserInputType)> event_lastInputTypeChanged(&UserInputService::lastInputTypeChangedSignal, "LastInputTypeChanged", "lastInputType", Security::None);

    static const Reflection::PropDescriptor<UserInputService, bool>  prop_TouchEnabled("TouchEnabled", category_Data, &UserInputService::getTouchEnabled, NULL);
    static const Reflection::PropDescriptor<UserInputService, bool>  prop_KeyboardEnabled("KeyboardEnabled", category_Data, &UserInputService::getKeyboardEnabled, NULL);
    static const Reflection::PropDescriptor<UserInputService, bool>  prop_MouseEnabled("MouseEnabled", category_Data, &UserInputService::getMouseEnabled, NULL);
    static const Reflection::PropDescriptor<UserInputService, bool>  prop_GamepadEnabled("GamepadEnabled", category_Data, &UserInputService::getGamepadEnabled, NULL);

	//todo: Remove these events/functions
	static Reflection::EventDesc<UserInputService, void()> event_JumpRequest(&UserInputService::jumpRequestEvent, "JumpRequest", Security::None);
	Reflection::PropDescriptor<UserInputService, bool>  UserInputService::prop_ModalEnabled("ModalEnabled", category_Behavior, &UserInputService::getModalEnabled, &UserInputService::setModalEnabled, Reflection::PropertyDescriptor::SCRIPTING, Security::None);
    
    // High Level touch events
	static Reflection::EventDesc<UserInputService, void(shared_ptr<const Reflection::ValueArray>, bool)> event_TapGesture(&UserInputService::tapGestureEvent, "TouchTap", "touchPositions", "gameProcessedEvent", Security::None);
    static Reflection::EventDesc<UserInputService, void(shared_ptr<const Reflection::ValueArray>,float,float,InputObject::UserInputState, bool)> event_PinchGesture(&UserInputService::pinchGestureEvent, "TouchPinch","touchPositions", "scale", "velocity", "state","gameProcessedEvent", Security::None);
    static Reflection::EventDesc<UserInputService, void(UserInputService::SwipeDirection, int, bool)> event_SwipeGesture(&UserInputService::swipeGestureEvent, "TouchSwipe", "swipeDirection", "numberOfTouches", "gameProcessedEvent", Security::None);
    static Reflection::EventDesc<UserInputService, void(shared_ptr<const Reflection::ValueArray>, InputObject::UserInputState, bool)> event_LongPressGesture(&UserInputService::longPressGestureEvent, "TouchLongPress", "touchPositions", "state", "gameProcessedEvent", Security::None);
    static Reflection::EventDesc<UserInputService, void(shared_ptr<const Reflection::ValueArray>, float,float,InputObject::UserInputState, bool)> event_RotateGesture(&UserInputService::rotateGestureEvent, "TouchRotate", "touchPositions", "rotation", "velocity", "state", "gameProcessedEvent", Security::None);
    static Reflection::EventDesc<UserInputService, void(shared_ptr<const Reflection::ValueArray>, RBX::Vector2,RBX::Vector2,InputObject::UserInputState, bool)> event_PanGesture(&UserInputService::panGestureEvent, "TouchPan", "touchPositions", "totalTranslation", "velocity", "state", "gameProcessedEvent", Security::None);
    
    // Low Level touch events
	static Reflection::EventDesc<UserInputService, 
		void(shared_ptr<Instance>, bool), 
		rbx::signal<void(shared_ptr<Instance>, bool)>,
		rbx::signal<void(shared_ptr<Instance>, bool)>* (UserInputService::*)(bool)> event_TouchStarted(&UserInputService::getTouchBeganEvent, "TouchStarted", "touch", "gameProcessedEvent", Security::None);
	static Reflection::EventDesc<UserInputService, 
		void(shared_ptr<Instance>, bool), 
		rbx::signal<void(shared_ptr<Instance>, bool)>,
		rbx::signal<void(shared_ptr<Instance>, bool)>* (UserInputService::*)(bool)>  event_TouchMoved(&UserInputService::getTouchChangedEvent, "TouchMoved", "touch", "gameProcessedEvent", Security::None);
	static Reflection::EventDesc<UserInputService, 
		void(shared_ptr<Instance>, bool), 
		rbx::signal<void(shared_ptr<Instance>, bool)>,
		rbx::signal<void(shared_ptr<Instance>, bool)>* (UserInputService::*)(bool)>  event_TouchEnded(&UserInputService::getTouchEndedEvent, "TouchEnded", "touch", "gameProcessedEvent", Security::None);

	// Low Level Generic input events
	Reflection::EventDesc<UserInputService, 
		void(shared_ptr<Instance>, bool), 
		rbx::signal<void(shared_ptr<Instance>, bool)>,
		rbx::signal<void(shared_ptr<Instance>, bool)>* (UserInputService::*)(bool)> UserInputService::event_InputBegin(&UserInputService::getInputBeganEvent, "InputBegan", "input", "gameProcessedEvent", Security::None);
	Reflection::EventDesc<UserInputService, 
		void(shared_ptr<Instance>, bool), 
		rbx::signal<void(shared_ptr<Instance>, bool)>,
		rbx::signal<void(shared_ptr<Instance>, bool)>* (UserInputService::*)(bool)> UserInputService::event_InputUpdate(&UserInputService::getInputChangedEvent, "InputChanged", "input", "gameProcessedEvent", Security::None);
	Reflection::EventDesc<UserInputService, 
		void(shared_ptr<Instance>, bool), 
		rbx::signal<void(shared_ptr<Instance>, bool)>,
		rbx::signal<void(shared_ptr<Instance>, bool)>* (UserInputService::*)(bool)> UserInputService::event_InputEnd(&UserInputService::getInputEndedEvent, "InputEnded", "input", "gameProcessedEvent", Security::None);
    
    // Textbox Stuff
    static Reflection::EventDesc<UserInputService, void(shared_ptr<Instance>)> event_TextboxFocus(&UserInputService::textBoxGainFocus, "TextBoxFocused", "textboxFocused", Security::None);
	static Reflection::EventDesc<UserInputService, void(shared_ptr<Instance>)> event_TextboxRelease(&UserInputService::textBoxReleaseFocus, "TextBoxFocusReleased", "textboxReleased", Security::None);
	static Reflection::BoundFuncDesc<UserInputService, shared_ptr<Instance>()> func_getFocusedTextBox(&UserInputService::getFocusedTextBox, "GetFocusedTextBox", Security::None);

	// Window Stuff
	static Reflection::EventDesc<UserInputService, void()> event_windowFocused(&UserInputService::windowFocused, "WindowFocused", Security::None);
	static Reflection::EventDesc<UserInputService, void()> event_windowFocusReleased(&UserInputService::windowFocusReleased, "WindowFocusReleased", Security::None);

	// Mouse Stuff
	static const Reflection::PropDescriptor<UserInputService, bool>  prop_mouseIconEnabled("MouseIconEnabled", category_Data, &UserInputService::getMouseIconEnabled, &UserInputService::setMouseIconEnabled);
	static const Reflection::EnumPropDescriptor<UserInputService, UserInputService::OverrideMouseIconBehavior>  prop_overrideMouseIconBehavior("OverrideMouseIconBehavior", category_Data, &UserInputService::getOverrideMouseIconBehavior, &UserInputService::setOverrideMouseIconBehavior, Reflection::PropertyDescriptor::SCRIPTING, Security::RobloxScript);
	static const Reflection::EnumPropDescriptor<UserInputService, UserInputService::MouseType>  prop_mouseType("MouseBehavior", category_Data, &UserInputService::getMouseType, &UserInputService::setMouseType);
    
	// Keyboard Stuff
	static Reflection::BoundFuncDesc<UserInputService, shared_ptr<const Reflection::ValueArray>()> func_getKeyboardState(&UserInputService::getKeyboardState, "GetKeysPressed", Security::None);
	static Reflection::BoundFuncDesc<UserInputService, bool(RBX::KeyCode)> func_isKeyCodeDown(&UserInputService::isKeyDown, "IsKeyDown", "keyCode", Security::None);

    // Motion Stuff
    static Reflection::EventDesc<UserInputService,
        void(shared_ptr<Instance>),
        rbx::signal<void(shared_ptr<Instance>)>,
        rbx::signal<void(shared_ptr<Instance>)>* (UserInputService::*)(bool)> event_AccelerometerChanged(&UserInputService::getOrCreateScriptAccelerometerEventSignal, "DeviceAccelerationChanged","acceleration", Security::None);
    static Reflection::EventDesc<UserInputService,
            void(shared_ptr<Instance>),
            rbx::signal<void(shared_ptr<Instance>)>,
            rbx::signal<void(shared_ptr<Instance>)>* (UserInputService::*)(bool)> event_gravityChanged(&UserInputService::getOrCreateScriptGravityEventSignal, "DeviceGravityChanged","gravity", Security::None);
    static Reflection::EventDesc<UserInputService,
        void(shared_ptr<Instance>, CoordinateFrame),
        rbx::signal<void(shared_ptr<Instance>, CoordinateFrame)>,
        rbx::signal<void(shared_ptr<Instance>, CoordinateFrame)>* (UserInputService::*)(bool)> event_GyroChanged(&UserInputService::getOrCreateScriptGyroEventSignal, "DeviceRotationChanged","rotation","cframe", Security::None);
    
    static Reflection::BoundFuncDesc<UserInputService, shared_ptr<Instance>(void)> func_getCurrentAcceleration(&UserInputService::getAcceleration, "GetDeviceAcceleration", Security::None);
    static Reflection::BoundFuncDesc<UserInputService, shared_ptr<Instance>(void)> func_getCurrentGravity(&UserInputService::getGravity, "GetDeviceGravity", Security::None);
    static Reflection::BoundFuncDesc<UserInputService, shared_ptr<const Reflection::Tuple>(void)> func_getCurrentRotation(&UserInputService::getRotation, "GetDeviceRotation", Security::None);
    
    static const Reflection::PropDescriptor<UserInputService, bool>  prop_accelerometerEnabled("AccelerometerEnabled", category_Data, &UserInputService::getAccelerometerEnabled, NULL);
    static const Reflection::PropDescriptor<UserInputService, bool>  prop_gyroscopeEnabled("GyroscopeEnabled", category_Data, &UserInputService::getGyroscopeEnabled, NULL);

	// Gamepad Stuff
	static Reflection::BoundFuncDesc<UserInputService, shared_ptr<const Reflection::ValueArray>()> func_getConnectedGamepads(&UserInputService::getConnectedGamepads, "GetConnectedGamepads", Security::None);
    static Reflection::BoundFuncDesc<UserInputService, bool(InputObject::UserInputType)> func_getGamepadConnected(&UserInputService::getGamepadConnected, "GetGamepadConnected", "gamepadNum", Security::None);
	static Reflection::EventDesc<UserInputService, void(InputObject::UserInputType)> event_gamepadConnected(&UserInputService::gamepadDisconnectedSignal, "GamepadDisconnected", "gamepadNum", Security::None);
	static Reflection::EventDesc<UserInputService, void(InputObject::UserInputType)> event_gamepadDisconnected(&UserInputService::gamepadConnectedSignal, "GamepadConnected", "gamepadNum", Security::None);

	static Reflection::BoundFuncDesc<UserInputService, shared_ptr<const Reflection::ValueArray>(InputObject::UserInputType)> func_getGamepadState(&UserInputService::getGamepadState, "GetGamepadState", "gamepadNum", Security::None);
	static Reflection::BoundFuncDesc<UserInputService, bool(InputObject::UserInputType, RBX::KeyCode)> func_getgamepadSupportsKeyCode(&UserInputService::gamepadSupports, "GamepadSupports", "gamepadNum", "gamepadKeyCode", Security::None);
	static Reflection::BoundFuncDesc<UserInputService, shared_ptr<const Reflection::ValueArray>(InputObject::UserInputType)> func_getSupportedGamepadKeyCodes(&UserInputService::getSupportedGamepadKeyCodes, "GetSupportedGamepadKeyCodes", "gamepadNum", Security::None);
	
	static Reflection::BoundFuncDesc<UserInputService, shared_ptr<const Reflection::ValueArray>()> func_getNavigationGamepads(&UserInputService::getNavigationGamepads, "GetNavigationGamepads", Security::None);
	static Reflection::BoundFuncDesc<UserInputService, bool(InputObject::UserInputType)> func_isNavigationGamepad(&UserInputService::isNavigationGamepad, "IsNavigationGamepad", "gamepadEnum", Security::None);
	static Reflection::BoundFuncDesc<UserInputService, void(InputObject::UserInputType, bool)> func_setNavigationGamepad(&UserInputService::setNavigationGamepad, "SetNavigationGamepad", "gamepadEnum", "enabled", Security::None);

	// VR Stuff
    static const Reflection::PropDescriptor<UserInputService, bool> prop_IsVREnabled("IsVREnabled", category_Data, &UserInputService::getVREnabled, NULL, Reflection::PropertyDescriptor::Attributes::deprecated());
	static const Reflection::PropDescriptor<UserInputService, CoordinateFrame> prop_UserHeadCFrame("UserHeadCFrame", category_Data, &UserInputService::getUserHeadCFrame, NULL, Reflection::PropertyDescriptor::Attributes::deprecated());

    static const Reflection::PropDescriptor<UserInputService, bool> prop_VREnabled("VREnabled", category_Data, &UserInputService::getVREnabled, NULL);
	static const Reflection::BoundFuncDesc<UserInputService, void()> func_RecenterUserHeadCFrame(&UserInputService::recenterUserHeadCFrame, "RecenterUserHeadCFrame", Security::None);
	static const Reflection::BoundFuncDesc<UserInputService, CoordinateFrame(UserInputService::UserCFrame)> func_GetUserCFrame(&UserInputService::getUserCFrameLua, "GetUserCFrame", "type", Security::None);
	static const Reflection::EventDesc<UserInputService, void(UserInputService::UserCFrame, CoordinateFrame value)> event_UserCFrameChanged(&UserInputService::userCFrameChanged, "UserCFrameChanged", "type", "value", Security::None);

    REFLECTION_END();

	bool UserInputService::isStudioEmulatingMobile = false;

    UserInputService::UserInputService() :
        isJump(false),
        walkDirection(Vector2::zero()),
        lastWalkDirection(Vector2::zero()),
        maxWalkDelta(0.0f),
        touchEnabled(false),
        keyboardEnabled(false),
        mouseEnabled(false),
        gamepadEnabled(false),
        accelerometerEnabled(false),
        gyroscopeEnabled(false),
        localCharacterJumpEnabled(true),
        wrapMode(WRAP_AUTO),
        modalEnabled(false),
        cameraZoom(0),
        cameraPanDelta(Vector2::zero()),
		cameraMouseTrack(Vector2::zero()),
		cameraMouseWrap(Vector2::zero()),
		mouseIconEnabled(true),
		mouseType(MOUSETYPE_DEFAULT),
        shouldFireAccelerationEvent(false),
        shouldFireGravityEvent(false),
        shouldFireRotationEvent(false),
		capsLocked(false),
		overrideMouseIconBehavior(OVERRIDE_BEHAVIOR_NONE),
		studioCamFlySteps(0),
		vrEnabled(false),
		recenterUserHeadCFrameRequested(false)
    {
		switch (getPlatform()) 
		{
            // desktop
			case UserInputService::PLATFORM_WINDOWS:
			case UserInputService::PLATFORM_OSX:
            {
                lastInputType = InputObject::TYPE_MOUSEMOVEMENT;
                setMouseEnabled(true);
                setKeyboardEnabled(true);
				break;
            }
            // mobile
			case UserInputService::PLATFORM_IOS:
			case UserInputService::PLATFORM_ANDROID:
            {
                lastInputType = InputObject::TYPE_TOUCH;
				setTouchEnabled(true);
                break;
            }
			// console
			case UserInputService::PLATFORM_XBOXONE:
			{
                lastInputType = InputObject::TYPE_GAMEPAD1;
				setGamepadEnabled(true);
				break;
			}
            case UserInputService::PLATFORM_NONE:
            default:
            {
                lastInputType = InputObject::TYPE_NONE;
				break;
            }
		}

		#ifdef _WIN32
        modPairs.push_back(std::make_pair(SDLK_LCTRL, KMOD_LCTRL));
		modPairs.push_back(std::make_pair(SDLK_RCTRL, KMOD_RCTRL));
		#elif __APPLE__
		modPairs.push_back(std::make_pair(SDLK_LMETA, KMOD_LMETA));
		modPairs.push_back(std::make_pair(SDLK_RMETA, KMOD_RMETA));
		#endif
		modPairs.push_back(std::make_pair(SDLK_LSHIFT, KMOD_LSHIFT));
		modPairs.push_back(std::make_pair(SDLK_RSHIFT, KMOD_RSHIFT));
		modPairs.push_back(std::make_pair(SDLK_CAPSLOCK, KMOD_CAPS));

#ifdef RBX_TEST_BUILD
		keyboardEnabled = true; // used for VirtualUser
#endif
    }

	bool UserInputService::IsUsingNewKeyboardEvents()
	{
#ifdef RBX_PLATFORM_UWP
		return true;
#endif
        return false;
	}
    
    void UserInputService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
    {
		if (DFFlag::GetFocusedTextBoxEnabled)
		{
			if(oldProvider)
			{
				textboxFocusBeganConnection.disconnect();
				textboxFocusEndedConnection.disconnect();
			}
		}
        Super::onServiceProvider(oldProvider, newProvider);
        
        if (newProvider)
        {
            if(DrawTouchEvents)
            {
               drawTouchFrame = Creatable<Instance>::create<RBX::Frame>();
               drawTouchFrame->setBorderSizePixel(0);
               drawTouchFrame->setSize(RBX::UDim2(0,4,0,4));
            }

			if (DFFlag::GetFocusedTextBoxEnabled)
			{
				textboxFocusBeganConnection = textBoxGainFocus.connect(boost::bind(&UserInputService::textboxFocused, this, _1));
				textboxFocusEndedConnection = textBoxReleaseFocus.connect(boost::bind(&UserInputService::textboxFocusReleased, this, _1));
			}
            
            rotationInputObject = RBX::Creatable<Instance>::create<InputObject>(InputObject::TYPE_GYRO, InputObject::INPUT_STATE_CHANGE, Vector3(0,0,0), SDLK_UNKNOWN, RBX::DataModel::get(newProvider));
            accelerationInputObject = RBX::Creatable<Instance>::create<InputObject>(InputObject::TYPE_ACCELEROMETER, InputObject::INPUT_STATE_CHANGE, Vector3(0,0,0), SDLK_UNKNOWN, RBX::DataModel::get(newProvider));
            gravityInputObject = RBX::Creatable<Instance>::create<InputObject>(InputObject::TYPE_ACCELEROMETER, InputObject::INPUT_STATE_CHANGE, Vector3(0,0,0), SDLK_UNKNOWN, RBX::DataModel::get(newProvider));
            rotationCFrame = CoordinateFrame(Vector3(0,0,0));
            
			for (int i = InputObject::TYPE_GAMEPAD1; i <= InputObject::TYPE_GAMEPAD8; i++)
			{
				connectedGamepadsMap[(InputObject::UserInputType) i] = false;
			}

            fakeMouseEventsMap[InputObject::TYPE_MOUSEMOVEMENT] = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(RBX::InputObject::TYPE_MOUSEMOVEMENT,
                                                                                                            RBX::InputObject::INPUT_STATE_CHANGE,
                                                                                                            RBX::Vector3(-1,-1,0),
                                                                                                            RBX::Vector3(0,0,0),
																											RBX::DataModel::get(newProvider));
            fakeMouseEventsMap[InputObject::TYPE_MOUSEMOVEMENT]->setSourceUserInputType(RBX::InputObject::TYPE_TOUCH);

            fakeMouseEventsMap[InputObject::TYPE_MOUSEBUTTON1] = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(RBX::InputObject::TYPE_MOUSEBUTTON1,
                                                                                                            RBX::InputObject::INPUT_STATE_BEGIN,
                                                                                                            RBX::Vector3(-1,-1,0),
                                                                                                            RBX::Vector3(0,0,0),
																											RBX::DataModel::get(newProvider));
            fakeMouseEventsMap[InputObject::TYPE_MOUSEBUTTON1]->setSourceUserInputType(RBX::InputObject::TYPE_TOUCH);

			currentMousePosition = RBX::Creatable<Instance>::create<InputObject>(InputObject::TYPE_MOUSEMOVEMENT, InputObject::INPUT_STATE_CANCEL, Vector3(-10000,-10000,0), SDLK_UNKNOWN, RBX::DataModel::get(newProvider));
        }
    }
	bool UserInputService::isTenFootInterface()
	{
		UserInputService::Platform platform = getPlatform();

		return (platform == PLATFORM_XBOXONE) || (platform == PLATFORM_PS4) || (platform == PLATFORM_PS3) || (platform == PLATFORM_XBOX360)
			|| (platform == PLATFORM_WIIU) || (platform == PLATFORM_OUYA) || (platform == PLATFORM_ANDROIDTV) || (platform == PLATFORM_STEAMOS);
	}
	rbx::signal<void(shared_ptr<Instance>, bool)>* UserInputService::getInputBeganEvent(bool whatever)
	{
		return RBX::Security::Context::current().hasPermission(RBX::Security::RobloxScript) ? &coreInputBeganEvent : &inputBeganEvent;
	}
	rbx::signal<void(shared_ptr<Instance>, bool)>* UserInputService::getInputChangedEvent(bool whatever)
	{
		return RBX::Security::Context::current().hasPermission(RBX::Security::RobloxScript) ? &coreInputUpdatedEvent : &inputUpdatedEvent;
	}
	rbx::signal<void(shared_ptr<Instance>, bool)>* UserInputService::getInputEndedEvent(bool whatever)
	{
		return RBX::Security::Context::current().hasPermission(RBX::Security::RobloxScript) ? &coreInputEndedEvent : &inputEndedEvent;
	}

	rbx::signal<void(shared_ptr<Instance>, bool)>* UserInputService::getTouchBeganEvent(bool whatever)
	{
		return RBX::Security::Context::current().hasPermission(RBX::Security::RobloxScript) ? &coreTouchStartedEvent : &touchStartedEvent;
	}
	rbx::signal<void(shared_ptr<Instance>, bool)>* UserInputService::getTouchChangedEvent(bool whatever)
	{
		return RBX::Security::Context::current().hasPermission(RBX::Security::RobloxScript) ? &coreTouchMovedEvent : &touchMovedEvent;
	}
	rbx::signal<void(shared_ptr<Instance>, bool)>* UserInputService::getTouchEndedEvent(bool whatever)
	{
		return RBX::Security::Context::current().hasPermission(RBX::Security::RobloxScript) ? &coreTouchEndedEvent : &touchEndedEvent;
	}
    
    UserInputService::Platform UserInputService::getPlatform()
    {
#if defined(RBX_PLATFORM_IOS)
        return PLATFORM_IOS;
#elif defined(__APPLE__) && !defined(RBX_PLATFORM_IOS)
        return PLATFORM_OSX;
#elif defined(RBX_PLATFORM_DURANGO)
		return PLATFORM_XBOXONE;
#elif defined(RBX_PLATFORM_UWP)
		return PLATFORM_UWP;
#elif defined(RBX_PLATFORM_DURANGO)
		return PLATFORM_XBOXONE;
#elif defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO) && !defined(RBX_PLATFORM_UWP)
        return PLATFORM_WINDOWS;
#elif defined(__ANDROID__)
        return PLATFORM_ANDROID;
#else
        return PLATFORM_NONE;
#endif
        
    }
    
    void UserInputService::signalInputEventOnService(const shared_ptr<InputObject>& inputObject, const InputObject::UserInputState& inputState, bool processedEvent, bool menuIsOpen)
    {
        if (inputObject && inputObject.get())
        {
            if (!inputObject->isPublicEvent())
            {
                return;
            }
            
            boost::mutex::scoped_lock lock(InputEventsMutex);
            
            InputObject::UserInputState switchState = InputObject::INPUT_STATE_NONE;
           
			switchState = inputObject->getUserInputState();

            switch (switchState)
            {
                case InputObject::INPUT_STATE_BEGIN:
                {
					coreInputBeganEvent(inputObject, processedEvent);
					if (!menuIsOpen)
					{
						inputBeganEvent(inputObject, processedEvent);
					}

                    if (inputObject->isTouchEvent())
                    {
						coreTouchMovedEvent(inputObject, processedEvent);
						if (!menuIsOpen)
						{
							touchStartedEvent(inputObject, processedEvent);
						}
                    }
					else if(inputObject->getUserInputType() == InputObject::TYPE_FOCUS)
					{
						windowFocused();
					}
                    break;
                }
                case InputObject::INPUT_STATE_CHANGE:
                {
					coreInputUpdatedEvent(inputObject, processedEvent);
					if (!menuIsOpen)
					{
						inputUpdatedEvent(inputObject, processedEvent);
					}

                    if (inputObject->isTouchEvent())
					{
						coreTouchMovedEvent(inputObject, processedEvent);
						if (!menuIsOpen)
						{
							touchMovedEvent(inputObject, processedEvent);
						}
					}
                    
                    break;
                }
                case InputObject::INPUT_STATE_END:
                {
					coreInputEndedEvent(inputObject, processedEvent);
					if (!menuIsOpen)
					{
						inputEndedEvent(inputObject, processedEvent);
					}

                    if (inputObject->isTouchEvent())
                    {
						coreTouchEndedEvent(inputObject, processedEvent);
						if (!menuIsOpen)
						{
							touchEndedEvent(inputObject, processedEvent);
						}
                    }
					else if(inputObject->getUserInputType() == InputObject::TYPE_FOCUS)
					{
						windowFocusReleased();
					}
                    break;
                }
                case InputObject::INPUT_STATE_NONE:
                default: // intentional fall-thru
                    break;
            }
        }
    }
    
    void UserInputService::doDataModelProcessInput(DataModel* dataModel, const shared_ptr<InputObject>& inputObject, void* nativeInputObject, const InputObject::UserInputState& inputState)
    {
        bool processedEvent = dataModel->processInputObject(inputObject);
        if ( inputObject->isMouseEvent() )
        {
            processedMouseEvent(processedEvent, nativeInputObject, inputObject);
        }
        processedEventSignal(inputObject, processedEvent);

		bool menuIsOpen = false;
		if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(dataModel))
		{
			menuIsOpen = guiService->getMenuOpen();
		}

		signalInputEventOnService(inputObject, inputState, processedEvent, menuIsOpen);
    }

	void UserInputService::dataModelProcessInput(const shared_ptr<InputObject>& inputObject, void* nativeInputObject, const InputObject::UserInputState& inputState)
	{
		if (inputObject)
		{
			if (inputObject->isPublicEvent())
			{
				updateLastInputType(inputObject);
			}
			updateCurrentMousePosition(inputObject);

			if (DataModel* dataModel = DataModel::get(this))
			{
                doDataModelProcessInput(dataModel, inputObject, nativeInputObject, inputState);
			}
		}
	}

	void UserInputService::updateLastInputType(const shared_ptr<InputObject>& inputObject)
	{
		if (lastInputType != inputObject->getUserInputType())
		{
			if ( (inputObject->getKeyCode() == SDLK_GAMEPAD_THUMBSTICK1 || inputObject->getKeyCode() == SDLK_GAMEPAD_THUMBSTICK2 ||
				inputObject->getKeyCode() == SDLK_GAMEPAD_BUTTONR2 || inputObject->getKeyCode() == SDLK_GAMEPAD_BUTTONL2) &&
				inputObject->getRawPosition().magnitude() < 0.2 )
			{
				return;
			}

			lastInputType = inputObject->getUserInputType();
			lastInputTypeChangedSignal(lastInputType);
		}
	}

	void UserInputService::updateCurrentMousePosition(const shared_ptr<InputObject>& inputObject)
	{
		if (lastInputType == InputObject::TYPE_MOUSEMOVEMENT && inputObject->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT)
		{
			currentMousePosition = inputObject;
		}
		else if (GamepadService::getGamepadIntForEnum(lastInputType) != -1 && GamepadService::getGamepadIntForEnum(inputObject->getUserInputType()) != -1)
		{
			if (Workspace* workspace = RBX::ServiceProvider::find<Workspace>(this))
			{
				Vector2 viewport = workspace->getCamera()->getViewport();
				Vector3 mousePos(viewport.x/2.0f,viewport.y/2.0f,0);

				if (RBX::Network::Players::findLocalPlayer(this))
				{
					DataModel* dataModel = DataModel::get(this);
					if (RBX::Workspace* workspace = dataModel->getWorkspace())
					{
						if (Camera* camera = workspace->getCamera())
						{
							float distance = (camera->getCameraFocus().translation - camera->getCameraCoordinateFrame().translation).length();
							if (distance > 0.55f)
							{
								mousePos = Vector3(viewport.x/2.0f,viewport.y/3.0f,0);
							}
						}
					}
				}

				currentMousePosition = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(InputObject::TYPE_MOUSEMOVEMENT, InputObject::INPUT_STATE_CHANGE,
					mousePos, Vector3::zero(), RBX::DataModel::get(this));
			}
		}
	}

    void UserInputService::processInputVector(EventsVector& eventVector, const InputObject::UserInputState& inputState)
    {
        EventsVector tmpEventsVector;
        {
            boost::mutex::scoped_lock lock(InputEventsMutex);
            if (eventVector == endEventsToProcess)
            {
                tempEndEvents = eventVector;
            }
            eventVector.swap(tmpEventsVector);
        }

        // process important events (i.e. key press, touch start/end, etc.)
        if (tmpEventsVector.size() > 0)
        {
            EventsVector::iterator end = tmpEventsVector.end();
            EventsVector::iterator begin = tmpEventsVector.begin();
            for (EventsVector::iterator iter = begin; iter != end; ++iter)
            {
                if(DrawTouchEvents)
                {
                    shared_ptr<Instance> newObj = drawTouchFrame->clone(RBX::ScriptingCreator);
                    Frame* newFrame = Instance::fastDynamicCast<Frame>(newObj.get());
                    Vector2 framePos = (*iter).first->get2DPosition();
                    newFrame->setPosition(UDim2(0,framePos.x,0,framePos.y));
                    newFrame->setBackgroundColor3(DrawTouchColor);
                    
                    shared_ptr<DataModel> dataModel = shared_from(DataModel::get(this));
                    if (CoreGuiService* cgs = dataModel->find<CoreGuiService>())
                    {
                        if (Instance* parent = cgs->findFirstChildByName("RobloxGui"))
                        {
                            newFrame->setParent(parent);
                        }
                    }
                }
                
                dataModelProcessInput((*iter).first, (*iter).second, inputState);
            }
        }

    }
    
    void UserInputService::processInputObjects()
    {
        DrawTouchColor = RBX::Color3(1,0,0);
        
        // process important start events (i.e. key down, touch start, etc.)
        processInputVector(beginEventsToProcess, InputObject::INPUT_STATE_BEGIN);
        
        if (DrawTouchEvents)
        {
            if (DrawMoveColor == Color3(0,1,0))
            {
                DrawMoveColor = Color3(0,1,1);
            }
            else if(DrawMoveColor == Color3(0,1,1))
            {
                DrawMoveColor = Color3(1,1,0);
            }
            else
            {
                DrawMoveColor = Color3(0,1,0);
            }
        }
        DrawTouchColor = DrawMoveColor;
        
        // process all events that only matter for a frame, usually changed events (mouse move, touch move, etc.)
        processInputVector(changeEventsToProcess, InputObject::INPUT_STATE_CHANGE);
        
        DrawTouchColor = RBX::Color3(0,0,1);
        
        // process important end events (i.e. key up, touch end, etc.)
        processInputVector(endEventsToProcess, InputObject::INPUT_STATE_END);
    }
    
    void UserInputService::processCameraInternal()
    {
        
        if (FFlag::UserAllCamerasInLua)
        {
            if (const DataModel* dataModel = DataModel::get(this))
            {
                if (const RBX::Workspace* workspace = dataModel->getWorkspace())
                {
                    if  (const Camera* camera = workspace->getConstCamera())
                    {
                        if (camera->hasClientPlayer())
                        {
                            return;
                        }
                    }
                }
            }
        }
		if (!FFlag::FlyCamOnRenderStep)
		{
			if (cameraZoom == 0 && 
				cameraPanDelta == Vector2::zero() &&
				cameraMouseWrap == Vector2::zero() &&
				cameraMouseTrack == Vector2::zero())
					return;
		}

        if (DataModel* dataModel = DataModel::get(this))
        {
			RBX::Workspace* workspace = dataModel->getWorkspace();
			if(!workspace)
				return;

			Camera* camera = workspace->getCamera();
			if(!camera)
				return;

			if (FFlag::FlyCamOnRenderStep)
			{
				if (!RBX::Network::Players::findLocalCharacter(this) || RBX::Network::Players::isCloudEdit(this))
				{
					if (ControllerService* service = ServiceProvider::find<ControllerService>(this)) 
					{
						if (const UserInputBase* hardwareDevice = service->getHardwareDevice()) 
						{
							NavKeys navKeys;
							if(DataModel* dm = DataModel::get(this))
								hardwareDevice->getNavKeys(navKeys,dm->getSharedSuppressNavKeys());

							if (navKeys.navKeyDown())
							{
								if (workspace->getCurrentMouseCommand())
									workspace->getCamera()->doFly(navKeys, studioCamFlySteps++);
							}
							else
							{
								if(studioCamFlySteps > 0)
									workspace->getCamera()->pushCameraHistoryStack();
								studioCamFlySteps = 0;
							}
						}
					}
				}
			}

			if( camera->getCameraType() == Camera::CUSTOM_CAMERA &&
				RBX::Network::Players::findLocalPlayer(dataModel) && !RBX::Network::Players::isCloudEdit(this))
			{
				cameraZoom = 0; 
				cameraPanDelta = Vector2::zero();
				cameraMouseWrap = Vector2::zero();
				cameraMouseTrack = Vector2::zero();
				return;
			}
            
            if (camera->getCameraType() != Camera::LOCKED_CAMERA)
            {
                if (camera->getCameraPanMode() == Camera::CAMERAPANMODE_CLASSIC)
                {
                    if (cameraPanDelta.x != 0)
                        camera->panRadians(cameraPanDelta.x);
                    if (cameraPanDelta.y != 0)
                        camera->tiltRadians(cameraPanDelta.y);
                    cameraPanDelta = Vector2::zero();
                }
                
                if (cameraZoom != 0)
                {
                    camera->zoom(cameraZoom);
                    cameraZoom = 0;
                }
            }

			if (cameraMouseTrack != Vector2::zero())
			{
				camera->onMouseTrack(cameraMouseTrack);
				cameraMouseTrack = Vector2::zero();
			}

			if (cameraMouseWrap != Vector2::zero())
			{
				camera->onMousePan(cameraMouseWrap);
				cameraMouseWrap = Vector2::zero();
			}
        }
    }
    
    
    void UserInputService::processToolEvents()
    {
        InputObjectVector toolEventsVectorTmp;
        {
            boost::mutex::scoped_lock lock(ToolEventsMutex);
            toolEventsVectorTmp.swap(toolEventsVector);
        }
        
        if (toolEventsVectorTmp.size() > 0)
        {
            if(RBX::DataModel* dataModel = RBX::DataModel::get(this))
            {
                for(InputObjectVector::iterator iter = toolEventsVectorTmp.begin(); iter != toolEventsVectorTmp.end(); ++iter)
                {
                    dataModel->processWorkspaceEvent(*iter);
                }
            }
        }

    }
    
    void UserInputService::processTextboxInternal()
    {
        TextBoxEventsVector tmpTextboxEventsVector;
        {
            boost::mutex::scoped_lock lock(TextboxEventsMutex);
            tmpTextboxEventsVector.swap(textboxFinishedVector);
        }
        
        if (tmpTextboxEventsVector.size() > 0)
        {
            for(TextBoxEventsVector::iterator iter = tmpTextboxEventsVector.begin(); iter != tmpTextboxEventsVector.end(); ++iter)
            {
				shared_ptr<InputObject> dummyReturnInput = RBX::Creatable<Instance>::create<InputObject>(InputObject::TYPE_KEYBOARD, InputObject::INPUT_STATE_BEGIN, Vector3::zero(), SDLK_RETURN, RBX::DataModel::get(this));
                textBoxFinishedEditing((*iter).first->c_str(), (*iter).second, (*iter).second ? dummyReturnInput : shared_ptr<InputObject>());
            }
        }
    }

	void UserInputService::textboxFocused(shared_ptr<Instance> textBox)
	{
		currentTextBox = textBox;
	}

	void UserInputService::textboxFocusReleased(shared_ptr<Instance> textBox)
	{
		currentTextBox = shared_ptr<Instance>();
	}

	shared_ptr<Instance> UserInputService::getFocusedTextBox()
	{
		if (!DFFlag::GetFocusedTextBoxEnabled)
		{
			RBX::StandardOut::singleton()->printf(MESSAGE_ERROR, "UserInputService:GetFocusedTextBox() is not yet enabled");
			return shared_ptr<Instance>();
		}
		return currentTextBox;
	}
    
    void UserInputService::cleanupCurrentTouches()
    {
        EventsVector::iterator end = tempEndEvents.end();
        EventsVector::iterator begin = tempEndEvents.begin();
        for (EventsVector::iterator iter = begin; iter != end; ++iter)
        {
            shared_ptr<InputObject> inputObject = (*iter).first;
            if (inputObject->isTouchEvent() && inputObject->getUserInputState() == InputObject::INPUT_STATE_END)
            {
                for(InputObjectVector::iterator iter = currentTouches.begin(); iter != currentTouches.end(); ++iter)
                {
                    if ((*iter) == inputObject)
                    {
                        currentTouches.erase(iter);
                        break;
                    }
                }
            }
        }
        
        tempEndEvents.clear();
    }
    
    void UserInputService::onRenderStep()
    {
        // allow any buffers to unload into InputObjects
        updateInputSignal();
        
        processInputObjects();
		processKeyboardEvents();
        processGestures();
        processMotionEvents();
        processToolEvents();
        processCameraInternal();
        processTextboxInternal();

        if(fireJumpRequestEvent)
        {
            jumpRequestEvent();
            fireJumpRequestEvent = false;
        }
        
        cleanupCurrentTouches();

		if (mouseType == MOUSETYPE_LOCKCENTER)
		{
			if (canUseMouseLockCenter())
			{
				if ( ControllerService* service = ServiceProvider::find<ControllerService>(this) )
				{
					if ( UserInputBase* userInput = service->getHardwareDevice() )
					{
						userInput->centerCursor();
					}
				}
			}
		}
    }
    
    void UserInputService::setModalEnabled(bool value)
    {
		if(modalEnabled != value)
		{
			modalEnabled = value;
			raisePropertyChanged(prop_ModalEnabled);
		}
    }
    bool UserInputService::getModalEnabled() const
    {
		return modalEnabled;
    }

    bool UserInputService::getAccelerometerEnabled() const
    {
        return accelerometerEnabled;
    }
    bool UserInputService::getGyroscopeEnabled() const
    {
        return gyroscopeEnabled;
    }
    bool UserInputService::getTouchEnabled() const
    {
		return touchEnabled || isStudioEmulatingMobile;
    }
    bool UserInputService::getKeyboardEnabled() const
    {
		return keyboardEnabled;
    }
    bool UserInputService::getMouseEnabled() const
	{
		return mouseEnabled;
    }
    bool UserInputService::getGamepadEnabled() const
    {
		return gamepadEnabled;
    }
    void UserInputService::setGyroscopeEnabled(bool value)
    {
        if (value != gyroscopeEnabled)
        {
            gyroscopeEnabled = value;
            raisePropertyChanged(prop_gyroscopeEnabled);
        }
    }
    void UserInputService::setAccelerometerEnabled(bool value)
    {
        if (value != accelerometerEnabled)
        {
            accelerometerEnabled = value;
            raisePropertyChanged(prop_accelerometerEnabled);
        }
    }
    void UserInputService::setTouchEnabled(bool value)
    {
        if (value != touchEnabled)
        {
            touchEnabled = value;
            raisePropertyChanged(prop_TouchEnabled);
        }
    }
    void UserInputService::setKeyboardEnabled(bool value)
    {
        if (value != keyboardEnabled)
        {
            keyboardEnabled = value;
            raisePropertyChanged(prop_KeyboardEnabled);
        }
    }
    void UserInputService::setMouseEnabled(bool value)
    {
        if (value != mouseEnabled)
        {
            mouseEnabled = value;
            raisePropertyChanged(prop_MouseEnabled);
        }
    }
    void UserInputService::setGamepadEnabled(bool value)
    {
        if (value != gamepadEnabled)
        {
            gamepadEnabled = value;
            raisePropertyChanged(prop_GamepadEnabled);
        }
    }

	bool UserInputService::getStudioEmulatingMobileEnabled()
	{
		return isStudioEmulatingMobile;
	}
	void UserInputService::setStudioEmulatingMobileEnabled(bool value)
	{
		isStudioEmulatingMobile = value;
	}
    
    bool UserInputService::getLocalCharacterJumpEnabled() const
    {
        return localCharacterJumpEnabled;
    }
    void UserInputService::setLocalCharacterJumpEnabled(bool value)
    {
        if(RBX::Network::Players::frontendProcessing(this))
        {
            if(value != localCharacterJumpEnabled)
            {
                localCharacterJumpEnabled = value;
                // raisePropertyChanged(prop_LocalCharacterJumpEnabled);
            }
        }
        else
            throw std::runtime_error("UserInputService.LocalCharacterJumpEnabled should only be accessed from a local script");
    }
    
    void UserInputService::setLocalHumanoidJump(weak_ptr<DataModel> weakDataModel, bool isJump)
    {
        if(shared_ptr<DataModel> dataModel = weakDataModel.lock())
        {
            RBX::Workspace* workspace = dataModel->getWorkspace();
            if(!workspace)
                return;
            
            RBX::ModelInstance* localCharacter = RBX::Network::Players::findLocalCharacter(workspace);
            if(RBX::Humanoid* localHumanoid = RBX::Humanoid::modelIsCharacter(localCharacter))
                localHumanoid->setJump(isJump);
        }
    }
    
    void UserInputService::jumpLocalCharacterLua()
    {
        setLocalHumanoidJump(shared_from(DataModel::get(this)), true);
    }
    
    void UserInputService::moveLocalCharacterInternal(weak_ptr<DataModel> weakDataModel, Vector2 walkDir, float walkDelta)
    {
        if(shared_ptr<DataModel> dataModel = weakDataModel.lock())
        {
            RBX::Workspace* workspace = dataModel->getWorkspace();
            if(!workspace)
                return;
            
            Camera* camera = workspace->getCamera();
            if(!camera)
                return;
            
            RBX::ModelInstance* localCharacter = RBX::Network::Players::findLocalCharacter(workspace);
            if(RBX::Humanoid* localHumanoid = RBX::Humanoid::modelIsCharacter(localCharacter))
            {
                if(walkDir == Vector2::zero())
                {
					if (isStudioEmulatingMobile)
						localHumanoid->setWalkingFromStudioTouchEmulation(false);
                    localHumanoid->setWalkDirection(Vector3::zero());
                    localHumanoid->setPercentWalkSpeed(1.0f);
                    return;
                }
				if (isStudioEmulatingMobile)
					localHumanoid->setWalkingFromStudioTouchEmulation(true);
                
                // set the percent walk speed for analog style controls (like a thumbstick)
                localHumanoid->setPercentWalkSpeed(walkDir.length()/walkDelta);
                Vector2 movementVector = walkDir.direction();
                
                // get the angle between the standard "forward" direction and our current desired direction
                RBX::Vector2 northDirection(0,-1);
                float crossMoveWithNorth = (movementVector.x * northDirection.y) - (movementVector.y * northDirection.x);
                float angle = -atan2(crossMoveWithNorth, movementVector.dot(northDirection));
                
                // get camera lookVector, projected into the xz plane
                RBX::Vector3 camLookVector = (camera->getCameraFocus().translation - camera->getCameraCoordinateFrame().translation).unit();
                RBX::Vector3 camLookProjected = G3D::Plane(Vector3(0,1,0),Vector3(0,0,0)).closestPoint(camLookVector);
                RBX::Vector2 camLook2d = RBX::Vector2(camLookProjected.x,camLookProjected.z);
                
                // rotate our desired move direction with the camera xz plane projection
                float cosAngle = cosf(angle);
                float sinAngle = sinf(angle);
                float xRotated = camLook2d.x * cosAngle - camLook2d.y * sinAngle;
                float zRotated = camLook2d.x * sinAngle + camLook2d.y * cosAngle;
                
                localHumanoid->setWalkDirection(RBX::Vector3(xRotated,0,zRotated));
            }
        }
    }
    
    
    void UserInputService::moveLocalCharacterLua(Vector2 walkDir, float maxWalkDelta)
    {
        walkDirection = walkDir;
        this->maxWalkDelta = fabs(maxWalkDelta);
        
        moveLocalCharacterInternal(shared_from(RBX::DataModel::get(this)), walkDir, maxWalkDelta);
    }
    
    void UserInputService::moveLocalCharacter(RBX::Vector2 movementVector, float maxMovementDelta)
    {
		walkDirection = movementVector;
		maxWalkDelta = fabs(maxMovementDelta);
    }
    
    void UserInputService::jumpLocalCharacter(bool jumpValue)
    {
        if(  getLocalCharacterJumpEnabled() )
            isJump = jumpValue;
		
		if (jumpValue)
			sendJumpRequestEvent();
    }

    void UserInputService::jumpOnceLocalCharacter(bool jumpValue)
    {
		if (jumpValue)
			sendJumpRequestEvent();
    }

    void UserInputService::processToolEvent(const shared_ptr<RBX::InputObject>& event)
    {
		boost::mutex::scoped_lock lock(ToolEventsMutex);
		toolEventsVector.push_back(event);
    }
    
    static void doCameraRotate(float panAmount, float tiltAmount, weak_ptr<DataModel> weakDataModel)
    {
        if(shared_ptr<DataModel> dataModel = weakDataModel.lock())
        {
            RBX::Workspace* workspace = dataModel->getWorkspace();
            if(!workspace)
                return;
            
            Camera* camera = workspace->getCamera();
            if(!camera)
                return;
            
			if (camera->getCameraType() != Camera::LOCKED_CAMERA && camera->getCameraPanMode() == Camera::CAMERAPANMODE_CLASSIC)
            {
                camera->panRadians(panAmount);
                camera->tiltRadians(tiltAmount);
            }
        }
    }

	void UserInputService::wrapCamera(RBX::Vector2 mouseWrap)
	{
		cameraMouseWrap += mouseWrap;
	}
    
    void UserInputService::rotateCamera(RBX::Vector2 mouseDelta)
    {
		cameraPanDelta += mouseDelta;
		cameraPanDelta = Vector2(G3D::clamp(cameraPanDelta.x,-100.0f,100.0f),cameraPanDelta.y);
    }
    
    void UserInputService::rotateCameraLua(RBX::Vector2 mouseDelta)
    {
        doCameraRotate( G3D::clamp(mouseDelta.x,-100.0f,100.0f), mouseDelta.y, shared_from(DataModel::get(this)));
    }
    
    static void doCameraZoom(float zoomAmount, weak_ptr<DataModel> weakDataModel)
    {
        if(shared_ptr<DataModel> dataModel = weakDataModel.lock())
        {
            RBX::Workspace* workspace = dataModel->getWorkspace();
            if(!workspace)
                return;
            
            Camera* camera = workspace->getCamera();
            if(!camera)
                return;
            
            if (camera->getCameraType() != Camera::LOCKED_CAMERA)
                camera->zoom(zoomAmount);
        }
    }
    
    void UserInputService::zoomCameraLua(float zoom)
    {
        doCameraZoom(zoom, shared_from(DataModel::get(this)));
    }
    
    void UserInputService::zoomCamera(float zoom)
    {
		cameraZoom += zoom;
    }

	void UserInputService::mouseTrackCamera(RBX::Vector2 mouseDelta)
	{
		cameraMouseTrack = mouseDelta;
	}
    
    static void doNothing(Reflection::Variant var) {}

    bool UserInputService::showStatsBasedOnInputString(const char* text)
    {
		if (DFFlag::EnableShowStatsLua)
			return false;

        bool retVal = false;
        shared_ptr<DataModel> dataModel = shared_from(DataModel::get(this));
        
        if (!dataModel)
            return retVal;
        
        
        if (strcmp(text, "Genstats") == 0)
        {
            dataModel->getGuiBuilder().toggleGeneralStats();
            retVal = true;
        }
        else if (strcmp(text, "Renstats") == 0)
        {
            dataModel->getGuiBuilder().toggleRenderStats();
            retVal = true;
        }
        else if (strcmp(text, "Netstats") == 0)
        {
            dataModel->getGuiBuilder().toggleNetworkStats();
            retVal = true;
        }
        else if (strcmp(text, "Phystats") == 0)
        {
            dataModel->getGuiBuilder().togglePhysicsStats();
            retVal = true;
        }
        else if (strcmp(text, "Sumstats") == 0)
        {
            dataModel->getGuiBuilder().toggleSummaryStats();
            retVal = true;
        }
        else if (strcmp(text, "Cusstats") == 0)
        {
            dataModel->getGuiBuilder().toggleCustomStats();
            retVal = true;
        }
        else if (boost::iequals(text, "console"))
        {
            if (CoreGuiService* cgs = dataModel->find<CoreGuiService>())
            {
                if (Instance* parent = cgs->findFirstChildByName("RobloxGui"))
                {
                    if (Instance* controlFrame = parent->findFirstChildByName("ControlFrame"))
                    {
                        parent = controlFrame;
                    }
                    if (BindableFunction* toggle = Instance::fastDynamicCast<BindableFunction>(parent->findFirstChildByName("ToggleDevConsole")))
                    {
                        toggle->invoke(rbx::make_shared<Reflection::Tuple>(), 
                            boost::bind(&doNothing, _1), boost::bind(&doNothing, _1));
                    }
                }
            }
            retVal = true;
        }
        
        return retVal;
    }
    
    void UserInputService::textboxDidFinishEditing(const char* text, bool shouldReturn)
    {
		shared_ptr<std::string> textString(new std::string(text));

		boost::mutex::scoped_lock lock(TextboxEventsMutex);
		textboxFinishedVector.push_back(std::pair<shared_ptr<std::string>, bool>(textString,shouldReturn));
    }
    

	//////////////////////////////////////////////////////////////////////////
	// Begin Gesture Input signals/functions
	//////////////////////////////////////////////////////////////////////////
    
    void UserInputService::addGestureEventToProcess(const UserInputService::Gesture gesture, shared_ptr<RBX::Reflection::ValueArray>& touchPositions, shared_ptr<Reflection::Tuple>& args)
    {
        if(touchPositions && args)
        {
            boost::mutex::scoped_lock lock(GestureEventsMutex);
            
            gestureEventsToProcess.push_back(std::pair<UserInputService::Gesture, std::pair<shared_ptr<RBX::Reflection::ValueArray>,shared_ptr<const Reflection::Tuple> > >(gesture, std::pair<shared_ptr<RBX::Reflection::ValueArray>, shared_ptr<const Reflection::Tuple> >(touchPositions, args)));
        }
    }
    
    void UserInputService::fireGestureEvent(const UserInputService::Gesture gesture, const shared_ptr<const RBX::Reflection::ValueArray>& touchPositions, const shared_ptr<const Reflection::Tuple>& args, const bool wasSunk)
    {
        switch (gesture)
        {
            case UserInputService::GESTURE_TAP:
                tapGestureEvent(touchPositions, wasSunk);
                break;
            case UserInputService::GESTURE_SWIPE:
                RBXASSERT(args->values.size() == 2);
                swipeGestureEvent(args->values[0].cast<UserInputService::SwipeDirection>(),args->values[1].cast<int>(), wasSunk);
                break;
            case UserInputService::GESTURE_ROTATE:
                RBXASSERT(args->values.size() == 3);
                rotateGestureEvent(touchPositions,args->values[0].cast<float>(),args->values[1].cast<float>(), args->values[2].cast<InputObject::UserInputState>(), wasSunk);
                break;
            case UserInputService::GESTURE_PINCH:
                RBXASSERT(args->values.size() == 3);
                pinchGestureEvent(touchPositions,args->values[0].cast<float>(),args->values[1].cast<float>(), args->values[2].cast<InputObject::UserInputState>(), wasSunk);
                break;
            case UserInputService::GESTURE_LONGPRESS:
                RBXASSERT(args->values.size() == 1);
                longPressGestureEvent(touchPositions,args->values[0].cast<InputObject::UserInputState>(), wasSunk);
                break;
            case UserInputService::GESTURE_PAN:
                RBXASSERT(args->values.size() == 3);
                panGestureEvent(touchPositions,args->values[0].cast<RBX::Vector2>(),args->values[1].cast<RBX::Vector2>(),args->values[2].cast<InputObject::UserInputState>(), wasSunk);
                break;
            case UserInputService::GESTURE_NONE:
            default: // intentional fall thru
                break;
        }

    }
    
    void UserInputService::processGestures()
    {
        GestureEventsVector tmpGestureEventsMap;
        {
            boost::mutex::scoped_lock lock(GestureEventsMutex);
            tmpGestureEventsMap.swap(gestureEventsToProcess);
        }
        
        if (tmpGestureEventsMap.size() > 0)
        {
			Vector2 guiOffset = Vector2::zero();
			if (GuiService* guiService = RBX::ServiceProvider::find<GuiService>(this))
			{
				guiOffset = guiService->getGlobalGuiInset().xy();
			}

            for (GestureEventsVector::iterator iter = tmpGestureEventsMap.begin(); iter != tmpGestureEventsMap.end(); ++iter)
            {
                GuiResponse response;

				shared_ptr<RBX::Reflection::ValueArray> touchPositions = (*iter).second.first;

				if (guiOffset != Vector2::zero())
				{
					for (RBX::Reflection::ValueArray::iterator valueIter = touchPositions->begin(); valueIter != touchPositions->end(); ++valueIter)
					{
						if ((*valueIter).isType<RBX::Vector2>())
						{
							RBX::Vector2 touchPosition = (*valueIter).cast<RBX::Vector2>();
							touchPosition -= guiOffset;
							*valueIter = touchPosition;
						}
					}
				}
                
                if(CoreGuiService* coreGui = ServiceProvider::find<CoreGuiService>(this))
                    response = coreGui->processGesture((*iter).first, touchPositions, (*iter).second.second);
            
                if (!response.wasSunk())
                {
                    if (RBX::Network::Players* players = ServiceProvider::create<RBX::Network::Players>(this))
                        if (RBX::Network::Player* player = players->getLocalPlayer())
                            if (PlayerGui* playerGui = player->findFirstChildOfType<PlayerGui>())
                                response = playerGui->processGesture((*iter).first, touchPositions, (*iter).second.second);
                }

                fireGestureEvent((*iter).first, touchPositions, (*iter).second.second, response.wasSunk());
            }
        }
    }
	//////////////////////////////////////////////////////////////////////////
	// Begin Generic Input signals/functions
	//////////////////////////////////////////////////////////////////////////
    
    bool UserInputService::inputVectorHasInputPair(const std::pair<shared_ptr<InputObject>, void*>& eventPair, const EventsVector& vectorToCheck)
    {
        if (vectorToCheck.size() > 0)
        {
            for (EventsVector::const_iterator iter = vectorToCheck.begin(); iter != vectorToCheck.end(); ++iter)
            {
                if (eventPair == (*iter))
                {
                    return true;
                }
            }
        }
        
        return false;
    }
    
    void UserInputService::eraseInputPairFromInputVector(const std::pair<shared_ptr<InputObject>, void*>& eventPair, EventsVector& vectorToCheck)
    {
        if (vectorToCheck.size() > 0)
        {
            for (EventsVector::iterator iter = vectorToCheck.begin(); iter != vectorToCheck.end(); ++iter)
            {
                if (eventPair == (*iter))
                {
                    vectorToCheck.erase(iter);
                    return;
                }
            }
        }
    }

    
    bool UserInputService::eraseInputTypeFromInputVector(const InputObject::UserInputType inputType, EventsVector& vectorToEraseFrom)
    {
        if(vectorToEraseFrom.size() > 0)
        {
            for (EventsVector::iterator iter = vectorToEraseFrom.begin(); iter != vectorToEraseFrom.end(); ++iter)
            {
                if (shared_ptr<InputObject> storedChangedInput = (*iter).first)
                {
                    if (storedChangedInput && storedChangedInput.get() && storedChangedInput->getUserInputType() == inputType)
                    {
                        vectorToEraseFrom.erase(iter);
                        return true;
                    }
                }
            }
        }
        
        return false;
    }
    
    // fires input events on datamodel w/o taking lock!!!! If you use this make sure you have lock!
    void UserInputService::dangerousFireInputEvent(const shared_ptr<InputObject>& inputObject, void* nativeInputObject)
    {
        dataModelProcessInput(inputObject, nativeInputObject, inputObject->getUserInputState());
        fireLegacyMouseEvent(inputObject, nativeInputObject, true);
    }
    
    void UserInputService::fireLegacyMouseEvent(const shared_ptr<InputObject>& inputObject, void* nativeInputObject, bool fireImmediately)
    {
        if (inputObject->isTouchEvent())
        {
            InputObject::UserInputType fakeInputType = InputObject::TYPE_NONE;
            switch (inputObject->getUserInputState())
            {
                case InputObject::INPUT_STATE_BEGIN:
                case InputObject::INPUT_STATE_END:
                    fakeInputType = InputObject::TYPE_MOUSEBUTTON1;
                    break;
                case InputObject::INPUT_STATE_CHANGE:
                    fakeInputType = InputObject::TYPE_MOUSEMOVEMENT;
                    break;
                default:
                    break;
            }
            
            if ((fakeInputType != InputObject::TYPE_NONE) && (fakeMouseEventsMap.find(fakeInputType) != fakeMouseEventsMap.end()))
            {
                fakeMouseEventsMap[fakeInputType]->setInputState(inputObject->getUserInputState());
                fakeMouseEventsMap[fakeInputType]->setPosition(inputObject->getRawPosition());
                
                if (fireImmediately)
                {
                    dangerousFireInputEvent(fakeMouseEventsMap[fakeInputType], nativeInputObject);
                }
                else
                {
                    fireInputEvent(fakeMouseEventsMap[fakeInputType], nativeInputObject);
                }
            }
        }
    }
    
    void UserInputService::fireInputEvent(const shared_ptr<InputObject>& inputObject, void* nativeInputObject, bool processed)
	{
        if(inputObject && inputObject.get())
        {
			if (!inputObject->isPublicEvent())
			{
				return;
			}
            
            {
                boost::mutex::scoped_lock lock(InputEventsMutex);
                
                std::pair<shared_ptr<InputObject>, void*> eventPair( inputObject, nativeInputObject);
                
                // always get start/end events
                if (inputObject->getUserInputState() == InputObject::INPUT_STATE_BEGIN)
                {
                    beginEventsToProcess.push_back(eventPair);
                    
                    if (inputObject->isTouchEvent())
                    {
                        currentTouches.push_back(inputObject);
                    }
                }
                else if (inputObject->getUserInputState() == InputObject::INPUT_STATE_END)
                {
                    endEventsToProcess.push_back(eventPair);
                }
                else // only process one 'change' event for each type, but make sure it isn't in a different queue!
                {
                    if (!inputVectorHasInputPair(eventPair, beginEventsToProcess) && !inputVectorHasInputPair(eventPair, endEventsToProcess))
                    {
                        if (inputObject->isTouchEvent()) // since more than one finger can be on the screen at once, only remove this particular pair
                        {
                            eraseInputPairFromInputVector(eventPair, changeEventsToProcess);
						}
						// erase the type, since all other input is singular (only can have multiple touches)
                        else
                        {
							if (inputObject->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT)
							{

								if(changeEventsToProcess.size() > 0)
								{
									for (EventsVector::iterator iter = changeEventsToProcess.begin(); iter != changeEventsToProcess.end() && !processed; ++iter)
									{
										if (shared_ptr<InputObject> storedChangedInput = (*iter).first)
										{
											if (storedChangedInput && storedChangedInput.get() && storedChangedInput->getUserInputType() == InputObject::TYPE_MOUSEMOVEMENT)
											{
												storedChangedInput->setDelta(storedChangedInput->getDelta() + inputObject->getDelta());
												storedChangedInput->setPosition(inputObject->getRawPosition());
												processed = true;
											}
										}
									}
								}

								if (!processed)									
								{

									if (mouseEventObject)
									{
											mouseEventObject->setInputType(inputObject->getUserInputType());
											mouseEventObject->setInputState(inputObject->getUserInputState());
											mouseEventObject->setDelta(inputObject->getDelta());
											mouseEventObject->setPosition(inputObject->getRawPosition());
									} else {
										mouseEventObject = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(inputObject->getUserInputType(), inputObject->getUserInputState(), 
											inputObject->getRawPosition(), inputObject->getDelta(), DataModel::get(this));
									}
									eventPair = std::pair<shared_ptr<InputObject>, void*>( mouseEventObject, nativeInputObject);
								}

							} 
							else 
							{
								eraseInputTypeFromInputVector(inputObject->getUserInputType(), changeEventsToProcess);
							}
                        }

						if (!processed)
							changeEventsToProcess.push_back(eventPair);
                    }
                }
            }
            
            fireLegacyMouseEvent(inputObject, nativeInputObject);
        }
	}
    
    //////////////////////////////////////////////////////////////////////////
	// Begin Motion Input signals/functions
	//////////////////////////////////////////////////////////////////////////
    shared_ptr<Instance> UserInputService::getAcceleration()
    {
        return accelerationInputObject;
    }
    shared_ptr<const Reflection::Tuple> UserInputService::getRotation()
    {
        shared_ptr<Reflection::Tuple> returnValues(new Reflection::Tuple(2));
        returnValues->values[0] = rotationInputObject;
		returnValues->values[1] = rotationCFrame;
        
        return returnValues;
    }
    shared_ptr<Instance> UserInputService::getGravity()
    {
        return gravityInputObject;
    }

    void UserInputService::fireAccelerationEvent(const RBX::Vector3& newAcceleration)
    {
        boost::mutex::scoped_lock lock(MotionEventsMutex);
        
        if (InputObject* accelInput = Instance::fastDynamicCast<InputObject>(accelerationInputObject.get()))
        {
            accelInput->setPosition(newAcceleration);
        }
        
        shouldFireAccelerationEvent = true;
    }
    
    void UserInputService::fireGravityEvent(const RBX::Vector3& newGravity)
    {
        boost::mutex::scoped_lock lock(MotionEventsMutex);
        
        if (InputObject* gravInput = Instance::fastDynamicCast<InputObject>(gravityInputObject.get()))
        {
            gravInput->setPosition(newGravity);
        }
        
        shouldFireGravityEvent = true;
    }
    
    void UserInputService::fireRotationEvent(const RBX::Vector3& newRotation, const RBX::Vector4& quaternion)
    {
        boost::mutex::scoped_lock lock(MotionEventsMutex);
        
        if (InputObject* gyroInput = Instance::fastDynamicCast<InputObject>(rotationInputObject.get()))
        {
        	gyroInput->setDelta(newRotation - gyroInput->getPosition());
            gyroInput->setPosition(newRotation);
            
            Quaternion quat(quaternion.w,quaternion.y,quaternion.z,quaternion.x);
            quat.normalize();
            
            quat.toRotationMatrix(rotationCFrame.rotation);
        }
        
        shouldFireRotationEvent = true;
    }
    
    void UserInputService::processMotionEvents()
    {
        boost::mutex::scoped_lock lock(MotionEventsMutex);
        
        if (shouldFireAccelerationEvent)
        {
            accelerometerChangedSignal(accelerationInputObject);
            shouldFireAccelerationEvent = false;
        }
        
        if (shouldFireGravityEvent)
        {
            gravityChangedSignal(gravityInputObject);
            shouldFireGravityEvent = false;
        }
        
        if (shouldFireRotationEvent)
        {
            gyroChangedSignal(rotationInputObject, rotationCFrame);
            shouldFireRotationEvent = false;
        }
    }
    
    rbx::signal<void(shared_ptr<Instance>, CoordinateFrame)>* UserInputService::getOrCreateScriptGyroEventSignal(bool create)
    {
        if (create)
        {
            if (!Network::Players::frontendProcessing(this))
            {
                throw std::runtime_error("DeviceRotationChanged event can only be used from local scripts");
            }
            else if (!gyroscopeEnabled)
            {
                RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "Trying to listen to rotation events on a device without a gyroscope.");
            }
            else
            {
                motionEventListeningStarted("gyro");
            }
        }
        
        return &gyroChangedSignal;
    }
    
    rbx::signal<void(shared_ptr<Instance>)>* UserInputService::getOrCreateScriptGravityEventSignal(bool create )
    {
        if (create)
        {
            if (!Network::Players::frontendProcessing(this))
            {
                throw std::runtime_error("DeviceGravityChanged event can only be used from local scripts");
            }
            else if (!accelerometerEnabled)
            {
                RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "Trying to listen to acceleration events on a device without a accelerometer.");
            }
            else
            {
                // uses accelerometer so make sure we are listening to that
                motionEventListeningStarted("accelerometer");
            }
        }
        
        return &gravityChangedSignal;
    }
    
    rbx::signal<void(shared_ptr<Instance>)>* UserInputService::getOrCreateScriptAccelerometerEventSignal(bool create )
    {
        if (create)
        {
            if (!Network::Players::frontendProcessing(this))
            {
                throw std::runtime_error("DeviceAccelerationChanged event can only be used from local scripts");
            }
            else
            {
                motionEventListeningStarted("accelerometer");
            }
        }
        
        return &accelerometerChangedSignal;
    }
    
	//////////////////////////////////////////////////////////////////////////
	// Begin Keyboard/Mouse Input signals/functions
	//////////////////////////////////////////////////////////////////////////
    void UserInputService::sendMouseEvent(const shared_ptr<RBX::InputObject>& event, void* nativeEventObject)
    {
		fireInputEvent(event, nativeEventObject);
	}
    
    std::string UserInputService::getPasteText()
    {
        std::string clipBoardText = "";
        
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO) && !defined(RBX_PLATFORM_UWP)
        if(::OpenClipboard(NULL))
		{
			HANDLE hData = ::GetClipboardData(CF_TEXT);
			if(hData)
			{
				clipBoardText = (char*) ::GlobalLock(hData);
				GlobalUnlock(hData);
			}
			::CloseClipboard();
		}
#endif
        
#if defined(__APPLE__)
#if !RBX_PLATFORM_IOS
        NSString* pasteString = [[NSPasteboard generalPasteboard] stringForType:NSPasteboardTypeString];
        if(pasteString)
            clipBoardText = [pasteString UTF8String];
#endif
#endif
        
        return clipBoardText;
    }
    
    std::vector<RBX::ModCode> UserInputService::getCommandModCodes()
    {
        std::vector<RBX::ModCode> modCodes;
#ifdef _WIN32
        modCodes.push_back(KMOD_LCTRL);
        modCodes.push_back(KMOD_RCTRL);
#elif __APPLE__
        modCodes.push_back(KMOD_LMETA);
        modCodes.push_back(KMOD_RMETA);
#endif
        return modCodes;
    }

	RBX::ModCode UserInputService::getCurrentModKey()
	{
		for(std::vector<std::pair<RBX::KeyCode, RBX::ModCode> >::iterator it = modPairs.begin(); it != modPairs.end(); ++it)
        {
            if (newKeyState.find(it->first) != newKeyState.end())
			{
				if (newKeyState[it->first]->getUserInputState() == InputObject::INPUT_STATE_BEGIN)
				{
					return it->second;
				}
			}
        }
		return KMOD_NONE;
	}

	char UserInputService::getModifiedKey(RBX::KeyCode key, RBX::ModCode mod)
	{
		// if our keycode is actually a modifier, don't return a char
		switch(key)
		{
		case RBX::SDLK_LSHIFT:
		case RBX::SDLK_RSHIFT:
		case RBX::SDLK_LCTRL:
		case RBX::SDLK_RCTRL:
		case RBX::SDLK_LMETA:
		case RBX::SDLK_RMETA:
		case RBX::SDLK_CAPSLOCK:
		case RBX::SDLK_LALT:
		case RBX::SDLK_RALT:
			return 0;
		default:
			break;
		}

		bool shift = ((mod & RBX::KMOD_RSHIFT) != 0) || ((mod & RBX::KMOD_LSHIFT) != 0);
		bool caps = ((mod & RBX::KMOD_CAPS) != 0);

		//XOR
		bool isUpper = (shift && !caps) || (!shift && caps);

		if ((key >= RBX::SDLK_a) && (key <= RBX::SDLK_z)) 
		{
			if (isUpper) 
			{
				return 'A' + (key - RBX::SDLK_a);
			}
			else
			{	
				return 'a' + (key - RBX::SDLK_a);
			}
		}

		// this is bad, doesn't work with non-standard keyboards
		if ((key >= RBX::SDLK_SPACE) && (key <= RBX::SDLK_AT)) {
			if(shift)
			{
				switch(key)
				{
				case RBX::SDLK_1:
					return RBX::SDLK_EXCLAIM;
					break;
				case RBX::SDLK_2:
					return RBX::SDLK_AT;
					break;
				case RBX::SDLK_3:
					return RBX::SDLK_HASH;
					break;
				case RBX::SDLK_4:
					return RBX::SDLK_DOLLAR;
					break;
				case RBX::SDLK_5:
					return RBX::SDLK_PERCENT;
					break;
				case RBX::SDLK_6:
					return RBX::SDLK_CARET;
					break;
				case RBX::SDLK_7:
					return RBX::SDLK_AMPERSAND;
					break;
				case RBX::SDLK_8:
					return RBX::SDLK_ASTERISK;
					break;
				case RBX::SDLK_9:
					return RBX::SDLK_LEFTPAREN;
					break;
				case RBX::SDLK_0:
					return RBX::SDLK_RIGHTPAREN;
					break;
                default:
                    break;
				}
			}
			else
			{
				return ' ' + (key - RBX::SDLK_SPACE);
			}
		}

		if(shift)
		{
			switch(key)
			{
			case RBX::SDLK_SEMICOLON:
				return RBX::SDLK_COLON;
				break;
			case RBX::SDLK_COMMA:
				return RBX::SDLK_LESS;
				break;
			case RBX::SDLK_PERIOD:
				return RBX::SDLK_GREATER;
				break;
			case RBX::SDLK_SLASH:
				return RBX::SDLK_QUESTION;
				break;
			case RBX::SDLK_MINUS:
				return RBX::SDLK_UNDERSCORE;
				break;
			case RBX::SDLK_BACKQUOTE:
				return RBX::SDLK_TILDE;
				break;
			case RBX::SDLK_LEFTBRACKET:
				return RBX::SDLK_LEFTCURLY;
				break;
			case RBX::SDLK_RIGHTBRACKET:
				return RBX::SDLK_RIGHTCURLY;
				break;
			case RBX::SDLK_BACKSLASH:
				return RBX::SDLK_PIPE;
				break;
			case RBX::SDLK_EQUALS:
				return RBX::SDLK_PLUS;
				break;
			case RBX::SDLK_QUOTE:
				return RBX::SDLK_QUOTEDBL;
				break;
            default:
                break;
			}
		}

		return key;
	}

	void UserInputService::processKeyboardEvents()
	{
		std::vector<KeyboardData> tmpKeyboardEventsVector;
		{
			boost::mutex::scoped_lock lock(KeyboardEventsMutex);
			keyboardEventsToProcess.swap(tmpKeyboardEventsVector);
		}

		for (std::vector<KeyboardData>::iterator iter = tmpKeyboardEventsVector.begin(); iter != tmpKeyboardEventsVector.end(); ++iter )
		{
			setKeyStateInternal(iter->keyCode, iter->modCode, iter->modifiedKey, iter->isDown);
		}
	}

	void UserInputService::setKeyStateInternal(RBX::KeyCode keyCode, ModCode modCode, char modifiedKey, bool isDown) 
	{
		if (keyCode == SDLK_CAPSLOCK && isDown)
		{
			capsLocked = !capsLocked;
		}

		shared_ptr<InputObject> inputObjectToFire = shared_ptr<InputObject>();

		if (newKeyState.find(keyCode) == newKeyState.end() )
		{
			newKeyState[keyCode] = RBX::Creatable<Instance>::create<InputObject>(InputObject::TYPE_KEYBOARD,
																					isDown ? InputObject::INPUT_STATE_BEGIN : InputObject::INPUT_STATE_END, 
																					keyCode, 
																					modCode, 
																					modifiedKey, 
																					DataModel::get(this));
			inputObjectToFire = newKeyState[keyCode];
		}
		else
		{
			shared_ptr<InputObject> inputObject = newKeyState[keyCode];
			inputObject->setInputState(isDown ? InputObject::INPUT_STATE_BEGIN : InputObject::INPUT_STATE_END);
			inputObject->mod = modCode;
			inputObject->modifiedKey = modifiedKey;
			inputObjectToFire = inputObject;
		}
	}

	void UserInputService::setKeyState(RBX::KeyCode keyCode, ModCode modCode, char modifiedKey, bool isDown) 
	{
        KeyboardData data;
        data.keyCode = keyCode;
        data.modCode = modCode;
        data.modifiedKey = modifiedKey;
        data.isDown = isDown;

        boost::mutex::scoped_lock lock(KeyboardEventsMutex);
        keyboardEventsToProcess.push_back(data);
	}

	bool UserInputService::isKeyDown(RBX::KeyCode keyCode) 
	{
        boost::unordered_map<RBX::KeyCode, shared_ptr<InputObject> >::iterator iter = newKeyState.find(keyCode);
		if (iter == newKeyState.end())
		{
			return false;
		}
		else
		{
            if (iter->second && iter->second.get())
            {
                return (iter->second->getUserInputState() == InputObject::INPUT_STATE_BEGIN);
            }
		}
        
        return false;
	}
	bool UserInputService::isCapsLocked()
	{
		return capsLocked;
	}

	void UserInputService::clearKeyStateInternal()
	{
		newKeyState.clear();
	}

	void UserInputService::resetKeyState() 
	{
		newKeyState.clear();
	}

	shared_ptr<const Reflection::ValueArray> UserInputService::getKeyboardState()
	{
		shared_ptr<Reflection::ValueArray> keyboardArray = rbx::make_shared<Reflection::ValueArray>();

		for (boost::unordered_map<RBX::KeyCode, shared_ptr<InputObject> >::iterator iter = newKeyState.begin(); iter != newKeyState.end(); ++iter)
		{
			if ((*iter).second->getUserInputState() == InputObject::INPUT_STATE_BEGIN)
			{
				shared_ptr<Instance> inputObject = (*iter).second;
				keyboardArray->push_back(inputObject);
			}
		}

		return keyboardArray;
	}

	Vector2 UserInputService::getKeyboardGuiSelectionDirection(const shared_ptr<InputObject>& inputObject)
	{
		// todo: allow these to be mapped?
		if (inputObject->isKeyDownEvent())
		{
			switch (inputObject->getKeyCode())
			{
			case SDLK_LEFT:
			case SDLK_a:
				return Vector2(-1,0);
			case SDLK_RIGHT:
			case SDLK_d:
				return Vector2(1,0);
			case SDLK_UP:
			case SDLK_w:
				return Vector2(0,-1);
			case SDLK_DOWN:
			case SDLK_s:
				return Vector2(0,1);

			default:
				return Vector2::zero();
				break;
			}
		}

		return Vector2::zero();
	}

	bool UserInputService::isMouseOverGui()
	{
		if (!currentMousePosition)
		{
			return false;
		}

		if (DataModel* dm = DataModel::get(this))
		{
			GuiResponse playerGuiResponse = dm->processPlayerGui(currentMousePosition);
			if (playerGuiResponse.wasSunk() || playerGuiResponse.getMouseWasOver())
			{
				return true;
			}

			if (Workspace* workspace = ServiceProvider::find<Workspace>(this))
			{
				GuiResponse surfaceGuiResponse = workspace->handleSurfaceGui(currentMousePosition);
				if (surfaceGuiResponse.wasSunk() || surfaceGuiResponse.getMouseWasOver())
				{
					return true;
				}
			}
		}

		return false;
	}


	//////////////////////////////////////////////////////////////////////////
	// Mouse Manipulation
   ///////////////////////////////////////////////////////////////////////////
	ContentId UserInputService::getDefaultMouseCursor(bool hoverOverActive, bool relativePathNoExtension)
	{
		std::string defaultKeyboardMouseCursorString;
		std::string defaultGamepadCursorString;

		if (hoverOverActive)
		{
			if (relativePathNoExtension)
			{
				defaultKeyboardMouseCursorString = "ArrowCursor";
				defaultGamepadCursorString = "Cursors/Gamepad/PointerOver";
			}
			else
			{
				defaultKeyboardMouseCursorString = "textures/ArrowCursor.png";
				defaultGamepadCursorString = "textures/Cursors/Gamepad/PointerOver.png";
			}
		}
		else
		{
			if (relativePathNoExtension)
			{
				defaultKeyboardMouseCursorString = "ArrowFarCursor";
				defaultGamepadCursorString = "Cursors/Gamepad/Pointer";
			}
			else
			{
				defaultKeyboardMouseCursorString = "textures/ArrowFarCursor.png";
				defaultGamepadCursorString = "textures/Cursors/Gamepad/Pointer.png";
			}
		}

		if (GamepadService::getGamepadIntForEnum(lastInputType) != -1)
		{
			if (relativePathNoExtension)
			{
				return ContentId(defaultGamepadCursorString);
			}
			return ContentId::fromAssets(defaultGamepadCursorString.c_str());
		}
		else if (lastInputType == InputObject::TYPE_MOUSEBUTTON1 || lastInputType == InputObject::TYPE_MOUSEBUTTON2 ||
			lastInputType == InputObject::TYPE_MOUSEDELTA || lastInputType == InputObject::TYPE_MOUSEMOVEMENT ||
			lastInputType == InputObject::TYPE_MOUSEWHEEL || lastInputType == InputObject::TYPE_KEYBOARD)
		{
			if (relativePathNoExtension)
			{
				return ContentId(defaultKeyboardMouseCursorString);
			}
			return ContentId::fromAssets(defaultKeyboardMouseCursorString.c_str());
		}

		if (keyboardEnabled)
		{
			if (relativePathNoExtension)
			{
				return ContentId(defaultKeyboardMouseCursorString);
			}
			return ContentId::fromAssets(defaultKeyboardMouseCursorString.c_str());
		}
		else if (gamepadEnabled)
		{
			if (relativePathNoExtension)
			{
				return ContentId(defaultGamepadCursorString);
			}
			return ContentId::fromAssets(defaultGamepadCursorString.c_str());
		}

		return ContentId();
	}
	TextureId UserInputService::getCurrentMouseIcon() 
	{ 
		return (( mouseIconStack.size() > 0) ? mouseIconStack.back() : TextureId());
	}
	void UserInputService::pushMouseIcon(TextureId newMouseIcon) 
	{ 
		mouseIconStack.push_back(newMouseIcon); 
	}
	void UserInputService::popMouseIcon()
	{ 
		if (mouseIconStack.size() > 0) 
			mouseIconStack.pop_back(); 
	}
	void UserInputService::popMouseIcon(TextureId mouseIconToRemove) 
	{ 
		mouseIconStack.erase(std::remove(mouseIconStack.begin(), mouseIconStack.end(), mouseIconToRemove), mouseIconStack.end());
	}

	void UserInputService::setMouseIconEnabled(bool enabled)
	{
		if (enabled != mouseIconEnabled)
		{
			mouseIconEnabled = enabled;
			mouseIconEnabledEvent(mouseIconEnabled);
			raisePropertyChanged(prop_MouseEnabled);
		}
	}
    
	void UserInputService::setMouseType(MouseType value)
	{
		if (value != mouseType)
		{
			mouseType = value;

			switch (mouseType)
			{
			case MOUSETYPE_DEFAULT:
				{
					setMouseWrapMode(UserInputService::WRAP_AUTO);
					break;
				}
			case MOUSETYPE_LOCKCENTER:
				{
					if ( ControllerService* service = ServiceProvider::find<ControllerService>(this) )
					{
						if ( UserInputBase* userInput = service->getHardwareDevice() )
						{
							userInput->centerCursor();
						}
					}
					break;
				}
			case MOUSETYPE_LOCKCURRENT:
			default: // intentional fall-thru
				break;
			}

			raisePropertyChanged(prop_mouseType);
		}
	}

	void UserInputService::setLastDownGuiObject(weak_ptr<GuiObject> object, InputObject::UserInputType type)
	{
		if (type == InputObject::TYPE_MOUSEBUTTON1)
		{
			lastDownGuiObjectLeft = object;
		}
		else if (type == InputObject::TYPE_MOUSEBUTTON2)
		{
			lastDownGuiObjectRight = object;
		}
	}

	weak_ptr<GuiObject> UserInputService::getLastDownGuiObject(InputObject::UserInputType type)
	{
		if (type == InputObject::TYPE_MOUSEBUTTON1)
		{
			return lastDownGuiObjectLeft;
		}
		else if (type == InputObject::TYPE_MOUSEBUTTON2)
		{
			return lastDownGuiObjectRight;
		}
		GuiObject* none = NULL;
		return weak_from(none);
	}

	UserInputService::WrapMode UserInputService::getMouseWrapMode() const
	{
		switch (mouseType)
		{
		case MOUSETYPE_LOCKCENTER:
		case MOUSETYPE_LOCKCURRENT: // intentional fall-thru
			{
				if (canUseMouseLockCenter())
				{
					return ((mouseType == MOUSETYPE_LOCKCENTER) ? UserInputService::WRAP_NONEANDCENTER : UserInputService::WRAP_NONE);
				}
				break;
			}
		default:
			break;
		}

		return wrapMode; 
	}



	//////////////////////////////////////////////////////////////////////////
	// Gamepad Functions
	//////////////////////////////////////////////////////////////////////////

	static void fireGamepadConnectionEvents(weak_ptr<DataModel> weakDM, bool isConnected, InputObject::UserInputType gamepadType)
	{
		if (shared_ptr<DataModel> sharedDM = weakDM.lock())
		{
			if (UserInputService* inputService = ServiceProvider::find<UserInputService>(sharedDM.get()))
			{
                inputService->setConnectedGamepad(gamepadType, isConnected);
                
				isConnected ? inputService->gamepadConnectedSignal(gamepadType) : 
								inputService->gamepadDisconnectedSignal(gamepadType);
			}
		}
	}

	shared_ptr<const Reflection::ValueArray> UserInputService::getConnectedGamepads()
	{
		shared_ptr<Reflection::ValueArray> gamepadConnectedArray = rbx::make_shared<Reflection::ValueArray>();
		for (boost::unordered_map<InputObject::UserInputType, bool >::iterator iter = connectedGamepadsMap.begin(); iter != connectedGamepadsMap.end(); ++iter)
		{
			if ((*iter).second)
			{
				gamepadConnectedArray->push_back((*iter).first);
			}
		}

		return gamepadConnectedArray;
	}
    
    bool UserInputService::getGamepadConnected(InputObject::UserInputType gamepadType)
    {
        if (connectedGamepadsMap.find(gamepadType) != connectedGamepadsMap.end())
        {
            return connectedGamepadsMap[gamepadType];
        }
        
        return false;
    }
    
    void UserInputService::setConnectedGamepad(InputObject::UserInputType gamepadType, bool isConnected)
    {
        connectedGamepadsMap[gamepadType] = isConnected;
        
        for (boost::unordered_map<InputObject::UserInputType, bool >::iterator iter = connectedGamepadsMap.begin(); iter != connectedGamepadsMap.end(); ++iter)
        {
            if ((*iter).second)
            {
                setGamepadEnabled(true);
                return;
            }
        }
        
        setGamepadEnabled(false);
    }

	void UserInputService::safeFireGamepadConnected(InputObject::UserInputType gamepadType)
	{
		if (DataModel* dm = DataModel::get(this))
		{
			dm->submitTask(boost::bind(&fireGamepadConnectionEvents, weak_from(dm), true, gamepadType), DataModelJob::Write);
		}
	}

	void UserInputService::safeFireGamepadDisconnected(InputObject::UserInputType gamepadType)
	{
		if (DataModel* dm = DataModel::get(this))
		{
			dm->submitTask(boost::bind(&fireGamepadConnectionEvents, weak_from(dm), false, gamepadType), DataModelJob::Write);
		}
	}

	void UserInputService::setSupportedGamepadKeyCodes(InputObject::UserInputType gamepadType, shared_ptr<const Reflection::ValueArray> newSupportedGamepadKeyCodes)
	{
		supportedGamepadKeyCodes[gamepadType] = newSupportedGamepadKeyCodes;
	}
	shared_ptr<const Reflection::ValueArray> UserInputService::getSupportedGamepadKeyCodes(InputObject::UserInputType gamepadType)
	{
		// this signal is listened to by SDLGameController, which calls setSupportedGamepadKeyCodes
		getSupportedGamepadKeyCodesSignal(gamepadType);

		// after signal this has been set
		return supportedGamepadKeyCodes[gamepadType];
	}
	bool UserInputService::gamepadSupports(InputObject::UserInputType gamepadType, RBX::KeyCode keyCode)
	{
		shared_ptr<const Reflection::ValueArray> gamepadKeyCodes = getSupportedGamepadKeyCodes(gamepadType);
		const Reflection::Variant keyCodeVariant(keyCode);

		if (gamepadKeyCodes)
		{
			for (Reflection::ValueArray::const_iterator iter = gamepadKeyCodes->begin(); iter != gamepadKeyCodes->end(); ++iter)
			{
				const Reflection::Variant iterVariant = (*iter);
				if (iterVariant.isType<RBX::KeyCode>())
				{
					RBX::KeyCode iterKeyCode = iterVariant.cast<RBX::KeyCode>();
					if (iterKeyCode == keyCode)
					{
						return true;
					}
				}
			}
		}

		return false;
	}

	shared_ptr<const Reflection::ValueArray> UserInputService::getGamepadState(InputObject::UserInputType gamepadType)
	{
		shared_ptr<Reflection::ValueArray> gamepadArray = rbx::make_shared<Reflection::ValueArray>();

		if (GamepadService* gamepadService = ServiceProvider::create<RBX::GamepadService>(this))
		{
			Gamepad gamepad = gamepadService->getGamepadState(GamepadService::getGamepadIntForEnum(gamepadType));

			for (Gamepad::iterator iter = gamepad.begin(); iter != gamepad.end(); ++iter)
			{
				shared_ptr<Instance> inputObject = (*iter).second;
				gamepadArray->push_back(inputObject);
			}
		}

		return gamepadArray;
	}

	shared_ptr<const Reflection::ValueArray> UserInputService::getNavigationGamepads()
	{
		shared_ptr<Reflection::ValueArray> gamepadNavigationArray = rbx::make_shared<Reflection::ValueArray>();
		if (GamepadService* gamepadService = ServiceProvider::create<RBX::GamepadService>(this))
		{
			boost::unordered_map<InputObject::UserInputType, bool> navigationGamepadMap = gamepadService->getNavigationGamepadMap();
			for (boost::unordered_map<InputObject::UserInputType, bool>::iterator iter = navigationGamepadMap.begin(); iter != navigationGamepadMap.end(); ++iter)
			{
				if ((*iter).second)
				{
					gamepadNavigationArray->push_back((*iter).first);
				}
			}
		}

		return gamepadNavigationArray;
	}

	bool UserInputService::isNavigationGamepad(InputObject::UserInputType gamepadType)
	{
		if (GamepadService* gamepadService = ServiceProvider::create<RBX::GamepadService>(this))
		{
			return gamepadService->isNavigationGamepad(gamepadType);
		}

		return false;
	}

	void UserInputService::setNavigationGamepad(InputObject::UserInputType gamepadType, bool enabled)
	{
		if (GamepadService* gamepadService = ServiceProvider::create<RBX::GamepadService>(this))
		{
			gamepadService->setNavigationGamepad(gamepadType, enabled);
		}
	}
    
    //////////////////////////////////////////////////////////////////////////
	// Misc Input signals/functions
	//////////////////////////////////////////////////////////////////////////
    
    void UserInputService::sendJumpRequestEvent()
    {
		fireJumpRequestEvent = true;
    }

	bool UserInputService::canUseMouseLockCenter() const
	{
		Workspace* workspace = ServiceProvider::find<Workspace>(this);

		return workspace && !workspace->hasModalGuiObjects() && !Profiler::isCapturingMouseInput();
	}

	void UserInputService::setVREnabled(bool value)
	{
		if (vrEnabled != value)
		{
			vrEnabled = value;
			raisePropertyChanged(prop_VREnabled);
			raisePropertyChanged(prop_IsVREnabled);
		}
	}

	void UserInputService::setUserHeadCFrame(const CoordinateFrame& value)
	{
		if (userHeadCFrame != value)
		{
			userHeadCFrame = value;
			raisePropertyChanged(prop_UserHeadCFrame);
		}
	}

	void UserInputService::setUserCFrame(UserCFrame type, const CoordinateFrame& value)
	{
		if (userCFrame[type] != value)
		{
			userCFrame[type] = value;
			userCFrameChanged(type, value);
		}
	}

	void UserInputService::recenterUserHeadCFrame()
	{
		recenterUserHeadCFrameRequested = true;
	}

	bool UserInputService::checkAndClearRecenterUserHeadCFrameRequest()
	{
		bool result = recenterUserHeadCFrameRequested;

		recenterUserHeadCFrameRequested = false;

		return result;
	}

} // Namespace RBX
