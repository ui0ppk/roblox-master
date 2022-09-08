#pragma once

#include "Util/KeyCode.h"
#include "V8DataModel/InputObject.h"
#include "rbx/debug.h"
#include "G3D/Vector2.h"

#define DIRECTINPUT_VERSION 0x0800

#define WM_KEYUP				0
#define WM_KEYDOWN				1
#define WM_SETFOCUS				10
#define WM_KILLFOCUS			11

#define GET_X_LPARAM(l)			((short)(l & 0x0000FFFF))
#define GET_Y_LPARAM(l)			((short)((l & 0xFFFF0000) >> 16))

#define MAKEXYLPARAM(x, y)		(((unsigned short) x) | ((unsigned short) y) << 16)


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
	typedef uint8_t DiKeys[256];
	
	static const float HybridSensitivity;
	static const float MouseTug;

	static RBX::ModCode				createModCode(const DiKeys& diKeys); 
	static RBX::InputObject::UserInputType	msgToEventType(unsigned int uMsg);
    static RBX::InputObject::UserInputState	msgToEventState(unsigned int uMsg);
	static DWORD					keyCodeToDirectInput(RBX::KeyCode keyCode);
	static RBX::KeyCode				directInputToKeyCode(DWORD diKey);
	static DWORD					keyCodeToVK(RBX::KeyCode diKey);
	
//	static G3D::Vector2				didodToVector2(const DIDEVICEOBJECTDATA& didod);
//
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

	static void						wrapMouseHorizontalCenter(const G3D::Vector2& delta, 
										G3D::Vector2& wrapMouseDelta, 
										G3D::Vector2& wrapMousePosition);
	
	static void						wrapMousePos(const G3D::Vector2& delta, 
												 G3D::Vector2& wrapMouseDelta, 
												 G3D::Vector2& wrapMousePosition,
												 const G3D::Vector2& windowSize,
												 G3D::Vector2& posToWrapTo,
												 bool autoMoveMouse);
};
