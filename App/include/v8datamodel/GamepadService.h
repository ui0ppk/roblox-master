//
//  GamepadService.h
//  App
//
//  Created by Ben Tkacheff on 1/21/15.
//
//

#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

#define RBX_MAX_GAMEPADS 8

namespace RBX
{

	class BasePlayerGui;

    extern const char* const sGamepadService;

	typedef boost::unordered_map<RBX::KeyCode, shared_ptr<RBX::InputObject> > Gamepad;
	typedef boost::unordered_map<int, Gamepad> Gamepads;

    class GamepadService
    : public DescribedNonCreatable<GamepadService, Instance, sGamepadService>
    , public Service
    {
    private:
        typedef DescribedNonCreatable<GamepadService, Instance, sGamepadService> Super;
        
		Gamepads gamepads;

		boost::unordered_map<InputObject::UserInputType, bool> gamepadNavigationEnabledMap;

		RBX::Timer<RBX::Time::Fast> repeatGuiSelectionTimer;
		RBX::Timer<RBX::Time::Fast> fastRepeatGuiSelectionTimer;

		bool autoGuiSelectionAllowed; // whether this game allows for automatic gui selection with gamepad keys

		Vector2 lastGuiSelectionDirection; // helper for determining how to use thumbstick for gui selection

		rbx::signals::scoped_connection updateInputConnection;
		rbx::signals::scoped_connection inputEndedConnection;
		rbx::signals::scoped_connection inputChangedConnection;
		rbx::signals::scoped_connection cameraCframeUpdateConnection;
        
        shared_ptr<RBX::InputObject> createInputObjectForGamepadKeyCode(RBX::KeyCode keyCode, RBX::InputObject::UserInputType gamepadType);
		void createControllerKeyMapForController(int controllerIndex);

		Vector2 getGuiSelectionDirection(const shared_ptr<InputObject>& event);

		bool isVectorInDeadzone(const Vector2& correctGuiDirection) const;

		GuiResponse autoSelectGui();
		GuiObject* getRandomShownGuiObject(Instance* object);

		void currentCameraChanged(shared_ptr<Camera> newCurrentCamera);
		void cameraCframeChanged(CoordinateFrame cframe);

		void updateOnInputStep();
		void onInputChanged(const shared_ptr<Instance>& event);
		void onInputEnded(const shared_ptr<Instance>& event);

		GuiResponse process(const shared_ptr<InputObject>& event, BasePlayerGui* guiToProcess);
        
        // Instance
        /*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

    public:
		GamepadService();

		static InputObject::UserInputType getGamepadEnumForInt(int controllerIndex);
		static int getGamepadIntForEnum(InputObject::UserInputType gamepadEnum);
      
        Gamepad getGamepadState(int controllerIndex);

		bool setAutoGuiSelectionAllowed(bool value);
		bool getAutoGuiSelectionAllowed() const { return autoGuiSelectionAllowed; }

		bool isNavigationGamepad(InputObject::UserInputType gamepadType);
		void setNavigationGamepad(InputObject::UserInputType gamepadType, bool enabled);

		boost::unordered_map<InputObject::UserInputType, bool> getNavigationGamepadMap() { return gamepadNavigationEnabledMap; }

		GuiResponse processDev(const shared_ptr<InputObject>& event);
		GuiResponse processCore(const shared_ptr<InputObject>& event);

		GuiResponse trySelectGuiObject(const Vector2& inputVector, const shared_ptr<InputObject>& event, BasePlayerGui* guiToProcess);
		GuiResponse trySelectGuiObject(const Vector2& inputVector);
    };
}