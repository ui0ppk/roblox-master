#include <stdint.h>
#include <strings.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>

#include "RobloxInfo.h"
#include "FastLog.h"
#include "JNIMain.h"
#include "JNIUtil.h"
#include "RobloxInput.h"
#include "RobloxView.h"
#include "v8datamodel/UserInputService.h"

using namespace RBX;
using namespace RBX::JNI;

extern "C" {
JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassInput(
		JNIEnv* jenv, jclass obj, jint eventId, jint xPos, jint yPos,
		jint eventType, jint windowWidth, jint windowHeight)
{
	RobloxInput::getRobloxInput().processEvent(eventId, xPos, yPos, eventType,
			windowWidth, windowHeight);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativeGamepadConnectEvent(JNIEnv* jenv, jclass obj, jint deviceId)
{
    RobloxInput::getRobloxInput().handleGamepadConnect(deviceId);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativeGamepadDisconnectEvent(JNIEnv* jenv, jclass obj, jint deviceId)
{
    RobloxInput::getRobloxInput().handleGamepadDisconnect(deviceId);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativeSetGamepadSupportedKey(JNIEnv* jenv, jclass obj, jint deviceId, jint keyCode, jboolean supported)
{
	RobloxInput::getRobloxInput().handleGamepadKeyCodeSupportChanged(deviceId, keyCode, supported);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativeGamepadButtonEvent(JNIEnv* jenv, jclass obj, jint deviceId, jint keyCode, jint buttonState)
{
	RobloxInput::getRobloxInput().handleGamepadButtonInput(deviceId, keyCode, buttonState);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativeGamepadAxisEvent(JNIEnv* jenv, jclass obj, jint deviceId, jint actionType, jfloat newValueX, jfloat newValueY, jfloat newValueZ)
{
	RobloxInput::getRobloxInput().handleGamepadAxisInput(deviceId, actionType, newValueX, newValueY, newValueZ);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassPinchGesture(
		JNIEnv* jenv, jclass obj, jint state, jfloat pinchScale,
		jfloat velocity, jint position1X, jint position1Y, jint position2X, jint position2Y)
{
	shared_ptr<RBX::Reflection::ValueArray> touchLocations(rbx::make_shared<RBX::Reflection::ValueArray>());
	touchLocations->push_back(RBX::Vector2(position1X, position1Y));
	touchLocations->push_back(RBX::Vector2(position2X, position2Y));

	shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<
			RBX::Reflection::Tuple>(3);
	args->values[0] = pinchScale;
	args->values[1] = velocity;

	switch (state) {
	case 0:
		args->values[2] = RBX::InputObject::INPUT_STATE_BEGIN;
		break;
	case 1:
		args->values[2] = RBX::InputObject::INPUT_STATE_CHANGE;
		break;
	case 2:
		args->values[2] = RBX::InputObject::INPUT_STATE_END;
		break;
	default:
		args->values[2] = RBX::InputObject::INPUT_STATE_NONE;
		break;
	}

	RobloxInput::getRobloxInput().sendGestureEvent(
			RBX::UserInputService::GESTURE_PINCH, args, touchLocations);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassTapGesture(
		JNIEnv* jenv, jclass obj, jint positionX, jint positionY) {
	shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<
			RBX::Reflection::Tuple>(0);

	shared_ptr<RBX::Reflection::ValueArray> touchLocations(
			rbx::make_shared<RBX::Reflection::ValueArray>());
	touchLocations->push_back(RBX::Vector2(positionX, positionY));

	RobloxInput::getRobloxInput().sendGestureEvent(
			RBX::UserInputService::GESTURE_TAP, args, touchLocations);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassSwipeGesture(JNIEnv* jenv, jclass obj, jint swipeDirection, jint numOfTouches)
{
	shared_ptr<RBX::Reflection::ValueArray> touchLocations(rbx::make_shared<RBX::Reflection::ValueArray>());

	shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(2);
	switch (swipeDirection)
	{
	case 0:
		args->values[0] = RBX::UserInputService::DIRECTION_RIGHT;
		break;
	case 1:
		args->values[0] = RBX::UserInputService::DIRECTION_DOWN;
		break;
	case 2:
		args->values[0] = RBX::UserInputService::DIRECTION_LEFT;
		break;
	case 3:
		args->values[0] = RBX::UserInputService::DIRECTION_UP;
		break;
	default:
		args->values[0] = RBX::UserInputService::DIRECTION_NONE;
		break;
	}
	args->values[1] = numOfTouches;

	RobloxInput::getRobloxInput().sendGestureEvent(RBX::UserInputService::GESTURE_SWIPE, args, touchLocations);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassLongPressGesture(JNIEnv* jenv, jclass obj, jint state, jint positionX, jint positionY)
{
	shared_ptr<RBX::Reflection::ValueArray> touchLocations(rbx::make_shared<RBX::Reflection::ValueArray>());
	touchLocations->push_back(RBX::Vector2(positionX, positionY));

	shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(1);
	switch (state)
	{
	case 0:
		args->values[0] = RBX::InputObject::INPUT_STATE_BEGIN;
		break;
	case 1:
		args->values[0] = RBX::InputObject::INPUT_STATE_CHANGE;
		break;
	case 2:
		args->values[0] = RBX::InputObject::INPUT_STATE_END;
		break;
	default:
		args->values[0] = RBX::InputObject::INPUT_STATE_NONE;
		break;
	}

	RobloxInput::getRobloxInput().sendGestureEvent(RBX::UserInputService::GESTURE_LONGPRESS, args, touchLocations);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassPanGesture(JNIEnv* jenv, jclass obj, jint state, jint positionX, jint positionY,jfloat totalTranslationX, jfloat totalTranslationY, jfloat velocity)
{
	shared_ptr<RBX::Reflection::ValueArray> touchLocations(rbx::make_shared<RBX::Reflection::ValueArray>());
	touchLocations->push_back(RBX::Vector2(positionX, positionY));

	RBX::Vector2 rbxTotalTranslation(totalTranslationX, totalTranslationY);
	RBX::Vector2 rbxVelocity(velocity, velocity);

	shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(3);
	args->values[0] = rbxTotalTranslation;
	args->values[1] = rbxVelocity;
	switch (state)
	{
	case 0:
		args->values[2] = RBX::InputObject::INPUT_STATE_BEGIN;
		break;
	case 1:
		args->values[2] = RBX::InputObject::INPUT_STATE_CHANGE;
		break;
	case 2:
		args->values[2] = RBX::InputObject::INPUT_STATE_END;
		break;
	default:
		args->values[2] = RBX::InputObject::INPUT_STATE_NONE;
		break;
	}

	RobloxInput::getRobloxInput().sendGestureEvent(RBX::UserInputService::GESTURE_PAN, args, touchLocations);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassRotateGesture(JNIEnv* jenv, jclass obj, jint state, jfloat rotation, jfloat velocity, jint position1X, jint position1Y, jint position2X, jint position2Y)
{
	shared_ptr<RBX::Reflection::ValueArray> touchLocations(rbx::make_shared<RBX::Reflection::ValueArray>());
	touchLocations->push_back(RBX::Vector2(position1X, position1Y));
	touchLocations->push_back(RBX::Vector2(position2X, position2Y));

	shared_ptr<RBX::Reflection::Tuple> args = rbx::make_shared<RBX::Reflection::Tuple>(3);
	args->values[0] = rotation;
	args->values[1] = velocity;

	switch (state)
	{
	case 0:
		args->values[2] = RBX::InputObject::INPUT_STATE_BEGIN;
		break;
	case 1:
		args->values[2] = RBX::InputObject::INPUT_STATE_CHANGE;
		break;
	case 2:
		args->values[2] = RBX::InputObject::INPUT_STATE_END;
		break;
	default:
		args->values[2] = RBX::InputObject::INPUT_STATE_NONE;
		break;
	}

	RobloxInput::getRobloxInput().sendGestureEvent(RBX::UserInputService::GESTURE_ROTATE, args, touchLocations);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassGravityChange(JNIEnv* jenv, jclass obj, jfloat x, jfloat y, jfloat z)
{
	RobloxInput::getRobloxInput().sendGravityEvent(x,y,z);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassAccelerometerChange(JNIEnv* jenv, jclass obj, jfloat x, jfloat y, jfloat z)
{
	RobloxInput::getRobloxInput().sendAccelerometerEvent(x,y,z);
}
JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativePassGyroscopeChange(JNIEnv* jenv, jclass obj, jfloat eulerX, jfloat eulerY, jfloat eulerZ,
		jfloat quaternionX, jfloat quaternionY, jfloat quaternionZ, jfloat quaternionW)
{
	RobloxInput::getRobloxInput().sendGyroscopeEvent(eulerX,eulerY,eulerZ,
			quaternionX, quaternionY, quaternionZ, quaternionW);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativeSetAccelerometerEnabled(JNIEnv* jenv, jclass obj, jboolean enabled)
{
	RobloxInput::getRobloxInput().setAccelerometerEnabled(enabled);
}

JNIEXPORT void JNICALL Java_com_roblox_client_InputListener_nativeSetGyroscopeEnabled(JNIEnv* jenv, jclass obj, jboolean enabled)
{
	RobloxInput::getRobloxInput().setGyroscopeEnabled(enabled);
}

} //extern "C"
