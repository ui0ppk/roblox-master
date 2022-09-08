#pragma once

#include "V8DataModel/InputObject.h"
#include "G3D/Vector2.h"

#define DIRECTINPUT_VERSION 0x0800

#include <dinput.h>

class UserInputUtil 
{
private:
	static void wrapMouseBorder(const G3D::Vector2& delta,
						G3D::Vector2& wrapMouseDelta, 
						G3D::Vector2& wrapMousePosition,
						const G3D::Vector2& windowSize,
						const int borderWidth,
						const float creepFactor);

public:
	typedef BYTE DiKeys[256];

	static const float HybridSensitivity;
	static const float MouseTug;

	static RBX::ModCode				createModCode(const DiKeys& diKeys); 
	static RBX::InputObject::UserInputState msgToEventState(UINT uMsg);
	static RBX::InputObject::UserInputType	msgToEventType(UINT uMsg);
	static DWORD					keyCodeToDirectInput(RBX::KeyCode keyCode);
	static RBX::KeyCode				directInputToKeyCode(DWORD diKey);
	static DWORD					keyCodeToVK(RBX::KeyCode diKey);
	static G3D::Vector2				didodToVector2(const DIDEVICEOBJECTDATA& didod);
	static bool						isCtrlDown(RBX::ModCode modCode);

	static void						wrapMouseNone(const G3D::Vector2& delta, 
										G3D::Vector2& wrapMouseDelta, 
										G3D::Vector2& wrapMousePosition);

	static void						wrapFullScreen(const G3D::Vector2& delta,
										G3D::Vector2& wrapMouseDelta, 
										G3D::Vector2& wrapMousePosition,
										const G3D::Vector2& windowSize);

	static void						wrapMouseHorizontalTransition(const G3D::Vector2& delta,
										G3D::Vector2& wrapMouseDelta, 
										G3D::Vector2& wrapMousePosition,
										const G3D::Vector2& windowSize);

	static void						wrapMouseBorderLock(const G3D::Vector2& delta,
										G3D::Vector2& wrapMouseDelta, 
										G3D::Vector2& wrapMousePosition,
										const G3D::Vector2& windowSize);

	static void						wrapMouseBorderTransition(const G3D::Vector2& delta,
										G3D::Vector2& wrapMouseDelta, 
										G3D::Vector2& wrapMousePosition,
										const G3D::Vector2& windowSize);

	static void						wrapMouseCenter(const G3D::Vector2& delta, 
										G3D::Vector2& wrapMouseDelta, 
										G3D::Vector2& wrapMousePosition);

	static void						wrapMousePos(const G3D::Vector2& delta, 
										G3D::Vector2& wrapMouseDelta, 
										G3D::Vector2& wrapMousePosition,
										const G3D::Vector2& windowSize,
										G3D::Vector2& posToWrapTo,
										bool autoMoveMouse);

	static void						wrapMouseHorizontalCenter(const G3D::Vector2& delta, 
										G3D::Vector2& wrapMouseDelta, 
										G3D::Vector2& wrapMousePosition);
};