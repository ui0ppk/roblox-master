#include <boost/unordered_map.hpp>
#include "V8DataModel/DataModel.h"
#include "V8DataModel/InputObject.h"
#include "V8DataModel/TextBox.h"

namespace RBX
{
    class GamepadService;
    class TouchInputService;
}

static boost::mutex ControllerBufferMutex;
static boost::mutex SupportedControllerKeyCodeMutex;

class RobloxInput
{

    typedef std::vector<G3D::Vector3> KeycodeInputs;
    typedef boost::unordered_map<RBX::KeyCode, KeycodeInputs> BufferedGamepadState;
    typedef boost::unordered_map<RBX::InputObject::UserInputType, BufferedGamepadState> BufferedGamepadStates;

public:
    RobloxInput();

    static RobloxInput& getRobloxInput()
    {
        static RobloxInput robloxInput;
        return robloxInput;
    }

    void processControllerBufferMap();

    void processEvent(int eventId, const int xPos, const int yPos, const int eventType, const float winSizeX, const float winSizeY);
    void sendGestureEvent(RBX::UserInputService::Gesture gestureToSend, shared_ptr<RBX::Reflection::Tuple> args, shared_ptr<RBX::Reflection::ValueArray> touchLocations);

    void passTextInput(RBX::TextBox* textBox, std::string newText, bool enterPressed, int cursorPosition);
    void externalReleaseFocus(RBX::TextBox* currentTextBox);

    void sendAccelerometerEvent(float x,float y, float z);
    void sendGyroscopeEvent(float eulerX,float eulerY,float eulerZ,
    		float quaternionX, float quaternionY, float quaternionZ, float quaternionW);
    void sendGravityEvent(float x,float y, float z);

    void setAccelerometerEnabled(bool enabled);
    void setGyroscopeEnabled(bool enabled);

    void handleGamepadConnect(int deviceId);
    void handleGamepadDisconnect(int deviceId);
    void handleGamepadButtonInput(int deviceId, int keyCode, int buttonState);
    void handleGamepadAxisInput(int deviceId, int actionType, float newValueX, float newValueY, float newValueZ);

    void getSupportedGamepadKeyCodes(RBX::InputObject::UserInputType gamepadEnum);
    void handleGamepadKeyCodeSupportChanged(int deviceId, int keyCode, bool supported);
private:
    shared_ptr<RBX::InputObject> tapInputObject;
    RBX::Vector3 tapLocation;
    int tapEventId;

    weak_ptr<RBX::TouchInputService> weakTouchInputService;
    weak_ptr<RBX::GamepadService> weakGamepadService;

    G3D::Vector2 tapTouchBeginPos;

    RBX::Timer<RBX::Time::Fast> tapTimer;

    typedef std::map<RBX::InputObject::UserInputType, bool> ConnectedControllerMap;

    ConnectedControllerMap connectedControllerMap;
    boost::unordered_map<int, RBX::InputObject::UserInputType> deviceIdToGamepadId;

    BufferedGamepadStates controllerBufferMap;

    boost::unordered_map<RBX::InputObject::UserInputType, shared_ptr<RBX::Reflection::ValueArray> > gamepadSupportedKeyCodes;

    RBX::DataModel* getDataModel();
    RBX::UserInputService* getInputService();
    RBX::TouchInputService* getTouchService();
    RBX::GamepadService* getGamepadService();

    RBX::InputObject::UserInputType mapDeviceIdToControllerEnum(int deviceId);

    void sendWorkspaceEvent(shared_ptr<RBX::InputObject> inputObject);
    void sendWorkspaceEvent(RBX::Vector3 touchPosition);

    void postEventProcessed(const shared_ptr<RBX::Instance>& event, bool processedEvent);
};

