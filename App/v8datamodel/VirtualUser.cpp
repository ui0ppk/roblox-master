/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/VirtualUser.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/UserController.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/Camera.h"
#include "util/UserInputBase.h"
#include "G3D/Quat.h"

namespace RBX {

    REFLECTION_BEGIN();
	static Reflection::BoundFuncDesc<VirtualUser, void(RBX::Vector2, RBX::CoordinateFrame)> func_ClickButton1(&VirtualUser::clickButton1, "ClickButton1", "position", "camera", RBX::CoordinateFrame(), Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, void(RBX::Vector2, RBX::CoordinateFrame)> func_Button1Down(&VirtualUser::button1Down, "Button1Down", "position", "camera", RBX::CoordinateFrame(), Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, void(RBX::Vector2, RBX::CoordinateFrame)> func_Button1Up(&VirtualUser::button1Up, "Button1Up", "position", "camera", RBX::CoordinateFrame(), Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, void(RBX::Vector2, RBX::CoordinateFrame)> func_ClickButton2(&VirtualUser::clickButton2, "ClickButton2", "position", "camera", RBX::CoordinateFrame(), Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, void(RBX::Vector2, RBX::CoordinateFrame)> func_Button2Down(&VirtualUser::button2Down, "Button2Down", "position", "camera", RBX::CoordinateFrame(), Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, void(RBX::Vector2, RBX::CoordinateFrame)> func_Button2Up(&VirtualUser::button2Up, "Button2Up", "position", "camera", RBX::CoordinateFrame(), Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, void(RBX::Vector2, RBX::CoordinateFrame)> func_MoveMouse(&VirtualUser::moveMouse, "MoveMouse", "position", "camera", RBX::CoordinateFrame(), Security::TestLocalUser);

	static Reflection::BoundFuncDesc<VirtualUser, void()> func_StartRecording(&VirtualUser::startRecording, "StartRecording", Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, std::string()> func_StopRecording(&VirtualUser::stopRecording, "StopRecording", Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, void()> func_CaptureController(&VirtualUser::captureInputDevice, "CaptureController", Security::TestLocalUser);

	static Reflection::BoundFuncDesc<VirtualUser, void(std::string)> func_PressKey(&VirtualUser::pressKey, "TypeKey", "key", Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, void(std::string)> func_SetKeyDown(&VirtualUser::setKeyDown, "SetKeyDown", "key", Security::TestLocalUser);
	static Reflection::BoundFuncDesc<VirtualUser, void(std::string)> func_SetKeyUp(&VirtualUser::setKeyUp, "SetKeyUp", "key", Security::TestLocalUser);
    REFLECTION_END();

	const char* const sVirtualUser = "VirtualUser";

	DataModel* VirtualUser::getDataModel()
	{
		DataModel* dataModel = DataModel::get(this);
		if (!dataModel)
			throw std::runtime_error("The service is not installed");
		return dataModel;
	}

	class VirtualHardwareDevice : public UserInputBase
	{
		bool keyDownStates[512];	// TODO: Use boost bitfield?
	public:
		// For debugging convenience, we use an 800x600 virtual screen size
		static const int screenWidth = 800;
		static const int screenHeight = 600;

		Vector2 position;

		VirtualHardwareDevice()
		{
			memset(keyDownStates, 0, sizeof(keyDownStates));
		}
		virtual void centerCursor()
		{
			position = Vector2(screenWidth/2, screenHeight/2);
		}
		virtual bool keyDown(KeyCode code) const
		{
			return keyDownStates[code];
		}
        virtual void setKeyState(RBX::KeyCode code, RBX::ModCode modCode, char modifiedKey, bool isDown)
        {
			if (code<0)
				return;
            if ((size_t)code>=sizeof(keyDownStates))
				return;
			keyDownStates[code] = isDown;
		}
        virtual Vector2 getCursorPosition()
		{
			return position;
		}
		void renderGameCursor(Adorn* adorn)
		{
			// Can't reliably render the cursor
		}
	};

	VirtualUser::VirtualUser()
	{
	}

	void VirtualUser::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
	{
		if (virtualHardwareDevice)
		{
			ControllerService* service = ServiceProvider::find<ControllerService>(oldProvider);
			if (service && service->getHardwareDevice()==virtualHardwareDevice.get())
				service->setHardwareDevice(NULL);
			virtualHardwareDevice.reset();
		}

		Super::onServiceProvider(oldProvider, newProvider);
	}

	void VirtualUser::captureInputDevice()
	{
		if (virtualHardwareDevice)
			return;

		// Replace the hardware controller with ours
		ControllerService* service = ServiceProvider::create<ControllerService>(this);
		if (!service)
			throw std::runtime_error("Can't access controller");

		virtualHardwareDevice.reset(new VirtualHardwareDevice());
		service->setHardwareDevice(virtualHardwareDevice.get());
	}

	void VirtualUser::sendMouseEvent(InputObject::UserInputType eventType, InputObject::UserInputState eventState, RBX::Vector2 position, RBX::CoordinateFrame camera)
	{
		captureInputDevice();

		// Convert from normalized coordinates to virtual screen coordinates
		position = position.clamp(-1, 1);
		position.x = VirtualHardwareDevice::screenWidth/2 * (1 + position.x);
		position.y = VirtualHardwareDevice::screenHeight/2 * (1 - position.y);

		virtualHardwareDevice->position = position;

		Vector2int16 mousePosition((short)position.x, (short)position.y);

		if (RBX::DataModel* dataModel = RBX::DataModel::get(this))
		{
			dataModel->getWorkspace()->getCamera()->setViewport(Vector2int16(VirtualHardwareDevice::screenWidth, VirtualHardwareDevice::screenHeight));
		}
		const shared_ptr<InputObject>& downEvent = Creatable<Instance>::create<InputObject>(eventType, eventState, Vector3(mousePosition,0), Vector3::zero(), RBX::DataModel::get(this));

		Camera* c = getDataModel()->getWorkspace()->getCamera();
		if (c && camera != CoordinateFrame())
		{
			//RBX::CoordinateFrame oldCamera = c->getCameraCoordinateFrame();
			c->setCameraCoordinateFrame(camera);
			getDataModel()->processInputObject(downEvent);
			//c->setCameraCoordinateFrame(oldCamera);
		}
		else
			getDataModel()->processInputObject(downEvent);
	}

	void VirtualUser::moveMouse(RBX::Vector2 position, RBX::CoordinateFrame camera)
	{
		sendMouseEvent(InputObject::TYPE_MOUSEMOVEMENT, InputObject::INPUT_STATE_CHANGE, position, camera);
	}

	void VirtualUser::clickButton1(RBX::Vector2 position, RBX::CoordinateFrame camera)
	{
		button1Down(position, camera);
		button1Up(position, camera);
	}
	void VirtualUser::button1Down(RBX::Vector2 position, RBX::CoordinateFrame camera)
	{
		sendMouseEvent(InputObject::TYPE_MOUSEBUTTON1, InputObject::INPUT_STATE_BEGIN, position, camera);
	}
	void VirtualUser::button1Up(RBX::Vector2 position, RBX::CoordinateFrame camera)
	{
		sendMouseEvent(InputObject::TYPE_MOUSEBUTTON1, InputObject::INPUT_STATE_END, position, camera);
	}

	void VirtualUser::clickButton2(RBX::Vector2 position, RBX::CoordinateFrame camera)
	{
		button2Down(position, camera);
		button2Up(position, camera);
	}
	void VirtualUser::button2Down(RBX::Vector2 position, RBX::CoordinateFrame camera)
	{
		sendMouseEvent(InputObject::TYPE_MOUSEBUTTON2, InputObject::INPUT_STATE_BEGIN, position, camera);
	}
	void VirtualUser::button2Up(RBX::Vector2 position, RBX::CoordinateFrame camera)
	{
		sendMouseEvent(InputObject::TYPE_MOUSEBUTTON2, InputObject::INPUT_STATE_END, position, camera);
	}

	KeyCode VirtualUser::convert(const std::string& key)
	{
		// Hex numbers are converted:
		if (key.substr(0, 2)=="0x")
			return (KeyCode) strtol(key.c_str(), NULL, 0);

		// 3-digit strings are interpreted as decimal numbers
		if (key.length()==3)
			return (KeyCode) strtol(key.c_str(), NULL, 0);

		if (key.length()!=1)
			throw RBX::runtime_error("Unsupported key %s", key.c_str());

		KeyCode keyCode = (KeyCode) key[0];
		return keyCode;
	}

	void VirtualUser::setKeyDown(std::string key)
	{
		captureInputDevice();

		KeyCode keyCode = convert(key);

		if (virtualHardwareDevice->keyDown(keyCode))
			return;

        virtualHardwareDevice->setKeyState(keyCode, RBX::KMOD_NONE, 0, true);

		const shared_ptr<InputObject>& downEvent = Creatable<Instance>::create<InputObject>(InputObject::TYPE_KEYBOARD, InputObject::INPUT_STATE_BEGIN, keyCode, (ModCode)0, (char)0, RBX::DataModel::get(this));
		getDataModel()->processInputObject(downEvent);
	}

	void VirtualUser::setKeyUp(std::string key)
	{
		captureInputDevice();

		KeyCode keyCode = convert(key);

		if (!virtualHardwareDevice->keyDown(keyCode))
			return;

        virtualHardwareDevice->setKeyState(keyCode, RBX::KMOD_NONE, 0, false);

		const shared_ptr<InputObject>& upEvent = Creatable<Instance>::create<InputObject>(InputObject::TYPE_KEYBOARD, InputObject::INPUT_STATE_END, keyCode, (ModCode)0,(char)0, RBX::DataModel::get(this));
		getDataModel()->processInputObject(upEvent);
	}

	void VirtualUser::pressKey(std::string key)
	{
		setKeyDown(key);
		setKeyUp(key);
	}

	void VirtualUser::startRecording()
	{
		if (recordingConnection.connected())
			throw std::runtime_error("Already recording");

		recording.clear();
		recording << "-- Begin Recording\n";
		recording << "local virtualUser = game:GetService('VirtualUser')\n";
		recording << "virtualUser:CaptureController()\n";
		lastEventTime = Time::now<Time::Fast>();
		recordingConnection = getDataModel()->InputObjectProcessed.connect(boost::bind(&VirtualUser::onInputObject, this, _1));
	}


	static RBX::Vector2 toNormalized(Vector2int16 windowSize, const Vector2int16 mousePosition)
	{
		Vector2 position;

		position.x = (2 * (float)mousePosition.x/(float)windowSize.x - 1);
		position.y = - (2 * (float)mousePosition.y/(float)windowSize.y - 1);

		return position.clamp(-1, 1);
	}

	void VirtualUser::writeWait()
	{
		Time now = Time::now<Time::Fast>();
		recording << "wait(" << (now - lastEventTime).seconds() << ")\n";
		lastEventTime = now;
	}

	void VirtualUser::writeKey(const char* func, const shared_ptr<InputObject>& event)
	{
		writeWait();
		recording << "virtualUser:" << func << "('" << RBX::format("0x%02x", event->getKeyCode()) << "')";
		if (event->isTextCharacterKey())
			recording << " -- " << event->modifiedKey;
		recording << '\n';
	}

	void VirtualUser::writeMouse(const char* func, const shared_ptr<InputObject>& event)
	{
		writeWait();
		Vector2 m = toNormalized(event->getWindowSize(), event->get2DPosition());
		CoordinateFrame camera;
		if (Camera* c = getDataModel()->getWorkspace()->getCamera())
			camera = c->getCameraCoordinateFrame();\

		G3D::Quat q = camera.rotation;

		recording << "virtualUser:" << func << "(";
			recording << "Vector2.new(" << m.x << ", " << m.y << "), ";
			recording << "CFrame.new(" 
				<< camera.translation.x << ", " << camera.translation.y << ", " << camera.translation.z << ", " 
				<< q.x << ", " << q.y << ", " << q.z << ", " << q.w 
				<< ")";
		recording << ")\n";
	}

	void VirtualUser::onInputObject( const shared_ptr<InputObject>& event )
	{
		switch (event->getUserInputType())
		{
		case InputObject::TYPE_KEYBOARD:
			RBXASSERT(event->isKeyDownEvent() || event->isKeyUpEvent());

			if (event->isKeyDownEvent())
				writeKey("SetKeyDown", event);
			else if (event->isKeyUpEvent())
				writeKey("SetKeUp", event);
			break;
		case InputObject::TYPE_MOUSEBUTTON1:
			RBXASSERT(event->isLeftMouseDownEvent() || event->isLeftMouseUpEvent());

			if (event->isLeftMouseDownEvent())
				writeMouse("Button1Down", event);
			else if (event->isLeftMouseUpEvent())
				writeMouse("Button1Up", event);
			break;
        default:
            break;
		}
	}

	std::string VirtualUser::stopRecording()
	{
		if (!recordingConnection.connected())
			throw std::runtime_error("Not recording");

		recordingConnection.disconnect();
		recording << "-- End Recording\n";
		return recording.str();
	}


} // namespace RBX
