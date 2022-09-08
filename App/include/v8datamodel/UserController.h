/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Humanoid/Humanoid.h"
#include "Util/SteppedInstance.h"
#include "Util/KeyCode.h"
#include "V8DataModel/Camera.h"
#include <boost/unordered_map.hpp>

namespace RBX {

	class UserInputBase;
	class VehicleSeat;
	class SkateboardPlatform;
	class ButtonBindingWidget;
	class DataModel;

	extern const char* const sControllerService;
	class ControllerService 
		: public DescribedNonCreatable<ControllerService, Instance, sControllerService, Reflection::ClassDescriptor::INTERNAL_LOCAL>
		, public Service
	{
		UserInputBase* hardwareDevice;

	public:
		ControllerService();

		UserInputBase* getHardwareDevice() {return hardwareDevice; }
		const UserInputBase* getHardwareDevice() const {return hardwareDevice; }

		void setHardwareDevice(UserInputBase* value) {hardwareDevice = value;}
	};


	extern const char* const sController;
	class Controller : public Reflection::Described<Controller, sController, Instance>
					 , public IStepped
	{
	private:
		typedef Reflection::Described<Controller, sController, Instance> Super;
	protected:
		const UserInputBase* getHardwareDevice() const;

		// Instance
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider) {
			Super::onServiceProvider(oldProvider, newProvider);
			onServiceProviderIStepped(oldProvider, newProvider);
		}

		//////////////////////////////////////////////////////////////////////////
		// Instance
		DataModel* dataModel;
		/*override*/ void onAncestorChanged(const AncestorChanged& event);

		friend class ButtonBindingWidget;
	public:
		Controller();
		virtual ~Controller();


		// mapped enum to SDLK enums. This is purely by convenience.
		// please don't serialize this enum without considering wether or not this is desirable.
		// Right now enum names are based on "intent", that is preferable to users directly referencing keys.
		// this would allow a user mapping layer to get in here later.
		// nothing prevents us from adding direct references to key names here as well. But if 
		// the actual values of these enums are exposed somehow (eg: through serialization),
		// then we lose the ability to decouple them.
		enum Button
		{
			JUMP = SDLK_SPACE,
			DISMOUNT = SDLK_BACKSPACE// SDLK_ESCAPE
		};

	protected:
		struct BoundButton
		{
			BoundButton(Button button, bool pressed, const std::string& caption)
				: button(button)
				, pressed(pressed)
				, caption(caption)
			{}
			Button button;
			bool pressed;
			std::string caption;
			shared_ptr<ButtonBindingWidget> guiWidget;
		};

		typedef boost::unordered_map<Button, BoundButton> BoundButtonSet;
		BoundButtonSet boundButtons;

		void setButton(Button button, bool value);
	public:

		bool isButtonBound(Button button) const;
		std::string getButtonCaption(Button button) const;

		void bindButton(Button button, std::string caption);
		void unbindButton(Button button);

		bool getButton(Button button) const;
		bool getButton(Button button);

		// shortcut for common actions.
		bool getJump() const { return getButton(JUMP); }
		bool getDismount() const { return getButton(DISMOUNT); } // do we need this on the controller? probably shoudn't be here.


		rbx::signal<void(Button)> buttonChangedSignal;
	};


	extern const char* const sHumanoidController;
	class HumanoidController : public DescribedCreatable<HumanoidController, Controller, sHumanoidController>
	{
	private:
		int stepsRotating;
		float panSensitivity;

		void rotateCam(int leftRight, Camera* camera, float gameStep);

		bool preventMovement(Humanoid*);
		void updateCamera(const Stepped&, const NavKeys&);

		// IStepped
		/*override*/ void onStepped(const Stepped& event) {}
	public:
		HumanoidController();

		float getPanSensitivity() const { return panSensitivity; }
		void setPanSensitivity(float newPan) { panSensitivity = newPan; }
	};


	extern const char* const sVehicleController;
	class VehicleController : public DescribedCreatable<VehicleController, Controller, sVehicleController>
	{
	private:
		// IStepped
		/*override*/ void onStepped(const Stepped& event);
        
        void onSteppedKeyboardInput(shared_ptr<VehicleSeat> seat);
        void onSteppedTouchInput(shared_ptr<VehicleSeat> seat);

		weak_ptr<VehicleSeat> vehicleSeat;

	public:
		VehicleController();

		void setVehicleSeat(VehicleSeat* value);
	};

} // namepsace
