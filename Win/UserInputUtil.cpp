/**
 * UserInputUtil.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "UserInputUtil.h"
#include "Util/Rect.h"
#include "util/standardout.h"
#include "V8DataModel/GameBasicSettings.h"

#include <dinput.h>

using RBX::Vector2;
using RBX::Rect;


const float UserInputUtil::HybridSensitivity = 15.0f;
const float UserInputUtil::MouseTug = 20.0f;

bool UserInputUtil::isCtrlDown(RBX::ModCode modCode)
{
	return (modCode == RBX::KMOD_LCTRL || modCode == RBX::KMOD_RCTRL);
}


// horizontal - will keep doing deltas
// vertical - will peg inset 2 pixels
//
void UserInputUtil::wrapFullScreen(const Vector2& delta,
										Vector2& wrapMouseDelta, 
										Vector2& wrapMousePosition,
										const Vector2& windowSize)
{
	float wrapPositionY = wrapMousePosition.y + delta.y;
	wrapMouseBorderLock(delta, wrapMouseDelta, wrapMousePosition, windowSize);
	wrapMouseDelta.y = 0.0;
	wrapMousePosition.y = wrapPositionY;
	float halfHeight = (windowSize.y * 0.5);

	float wrapPositionX = wrapMousePosition.x;
	float halfWidth = (windowSize.x * 0.5);
	wrapMousePosition.x = G3D::clamp(wrapPositionX,-halfWidth,halfWidth);

	// This is changed so we can pop out into the chat area below
	wrapMousePosition.y = G3D::clamp(wrapMousePosition.y, -halfHeight, halfHeight);
	//wrapMousePosition.y = std::max(-halfHeight, wrapMousePosition.y);

	// Setting this to zero prevents the camera from panning automatically near the extents of the screen.
	wrapMouseDelta = Vector2::zero();
}

void UserInputUtil::wrapMouseHorizontalTransition(const Vector2& delta,
										Vector2& wrapMouseDelta, 
										Vector2& wrapMousePosition,
										const Vector2& windowSize)
{
	float wrapPositionY = wrapMousePosition.y + delta.y;
	wrapMouseBorderTransition(delta, wrapMouseDelta, wrapMousePosition, windowSize);
	wrapMouseDelta.y = 0.0;
	wrapMousePosition.y = wrapPositionY;
}


void UserInputUtil::wrapMouseBorder(const Vector2& delta,
									Vector2& wrapMouseDelta, 
									Vector2& wrapMousePosition,
									const Vector2& windowSize,
									const int borderWidth,
									const float creepFactor)

{
	Vector2 halfSize = windowSize * 0.5;
	Rect inner = Rect(-halfSize, halfSize).inset(borderWidth);	// in Wrap Coordinates
	Vector2 oldPosition = wrapMousePosition;
	inner.unionWith(oldPosition);					// now union of the border and old position - ratchet

	Vector2 newPositionUnclamped = oldPosition + delta;
	Vector2 newPositionClamped = inner.clamp(newPositionUnclamped);
	
	Vector2 positiveDistanceInBorder = newPositionUnclamped - newPositionClamped;


	if(!RBX::GameBasicSettings::singleton().inMousepanMode())
		wrapMousePosition = newPositionClamped + (positiveDistanceInBorder * creepFactor);
	wrapMouseDelta += positiveDistanceInBorder;
}

void UserInputUtil::wrapMouseBorderLock(const Vector2& delta,
									Vector2& wrapMouseDelta, 
									Vector2& wrapMousePosition,
									const Vector2& windowSize)
{
	wrapMouseBorder(
		delta,
		wrapMouseDelta,
		wrapMousePosition,
		windowSize, 
		6,
		0.0f);
}

void UserInputUtil::wrapMouseBorderTransition(const Vector2& delta,
									Vector2& wrapMouseDelta, 
									Vector2& wrapMousePosition,
									const Vector2& windowSize)
{
	wrapMouseBorder(
		delta,
		wrapMouseDelta,
		wrapMousePosition,
		windowSize, 
		20,
		0.05f);
}

void UserInputUtil::wrapMouseNone(const Vector2& delta, 
									Vector2& wrapMouseDelta, 
									Vector2& wrapMousePosition)
{
	wrapMouseDelta = Vector2::zero();
	wrapMousePosition += delta;
}

void UserInputUtil::wrapMouseCenter(const Vector2& delta, 
									Vector2& wrapMouseDelta, 
									Vector2& wrapMousePosition)
{
	wrapMouseDelta += delta;
	// don't move the cursor....
	// wrapMousePosition = G3D::Vector2::zero();
	
}

void UserInputUtil::wrapMousePos(const Vector2& delta, 
									Vector2& wrapMouseDelta, 
									Vector2& wrapMousePosition,
									const Vector2& windowSize,
									Vector2& posToWrapTo,
									bool autoMoveMouse)
{
	if(posToWrapTo.length() > 2)
	{
		Vector2 windowDelta = posToWrapTo/windowSize;
		wrapMouseDelta = windowDelta * HybridSensitivity;
		posToWrapTo -= (wrapMouseDelta * HybridSensitivity * 0.266f); // 0.266 is a tuning constant

		float xDiff = std::abs(wrapMousePosition.x/wrapMousePosition.length()) * 0.3f;
		float yDiff = std::abs(wrapMousePosition.y/wrapMousePosition.length()) * 0.4f;

		if(autoMoveMouse)
		{
			if(wrapMousePosition.x < 0)
			{
				wrapMousePosition.x += xDiff * wrapMouseDelta.length() * MouseTug;
				if(wrapMousePosition.x > 0)
					wrapMousePosition.x = 0;
			}
			else if (wrapMousePosition.x > 0)
			{
				wrapMousePosition.x -= xDiff * wrapMouseDelta.length()* MouseTug;
				if(wrapMousePosition.x < 0)
					wrapMousePosition.x = 0;
			}
			

			if(wrapMousePosition.y < 0)
			{
				wrapMousePosition.y += yDiff * wrapMouseDelta.length() * MouseTug;
				if(wrapMousePosition.y > 0)
					wrapMousePosition.y = 0;
			}
			else if (wrapMousePosition.y > 0)
			{
				wrapMousePosition.y -= yDiff * wrapMouseDelta.length() * MouseTug;
				if(wrapMousePosition.y < 0)
					wrapMousePosition.y = 0;
			}
		}

	}
	
	wrapMousePosition += delta;		
}


void UserInputUtil::wrapMouseHorizontalCenter(const Vector2& delta, 
											Vector2& wrapMouseDelta, 
											Vector2& wrapMousePosition)
{
	wrapMouseDelta.x += delta.x;
//	wrapMousePosition = G3D::Vector2::zero();
}



G3D::Vector2 UserInputUtil::didodToVector2(const DIDEVICEOBJECTDATA& didod)
{
	G3D::Vector2 answer(0,0);
	float data = (float) ((int)didod.dwData);

	if (didod.dwOfs==DIMOFS_X) {
		answer.x = data;
	}
	else {
		answer.y = data;
	}
	return answer;
}


// Maps DIK_* to RBX::SDLK_* 
RBX::KeyCode UserInputUtil::directInputToKeyCode(DWORD diKey)
{
	RBXASSERT(diKey>=0);
	RBXASSERT(diKey<256);

	static RBX::KeyCode keymap[256];
	static bool initialized = false;
	if (!initialized)
	{
		for ( int i=0; i<256; ++i )
			keymap[i] = RBX::SDLK_UNKNOWN;

		keymap[DIK_ESCAPE] = RBX::SDLK_ESCAPE;
		keymap[DIK_1] = RBX::SDLK_1;
		keymap[DIK_2] = RBX::SDLK_2;
		keymap[DIK_3] = RBX::SDLK_3;
		keymap[DIK_4] = RBX::SDLK_4;
		keymap[DIK_5] = RBX::SDLK_5;
		keymap[DIK_6] = RBX::SDLK_6;
		keymap[DIK_7] = RBX::SDLK_7;
		keymap[DIK_8] = RBX::SDLK_8;
		keymap[DIK_9] = RBX::SDLK_9;
		keymap[DIK_0] = RBX::SDLK_0;
		keymap[DIK_MINUS] = RBX::SDLK_MINUS;
		keymap[DIK_EQUALS] = RBX::SDLK_EQUALS;
		keymap[DIK_BACK] = RBX::SDLK_BACKSPACE;
		keymap[DIK_TAB] = RBX::SDLK_TAB;
		keymap[DIK_Q] = RBX::SDLK_q;
		keymap[DIK_W] = RBX::SDLK_w;
		keymap[DIK_E] = RBX::SDLK_e;
		keymap[DIK_R] = RBX::SDLK_r;
		keymap[DIK_T] = RBX::SDLK_t;
		keymap[DIK_Y] = RBX::SDLK_y;
		keymap[DIK_U] = RBX::SDLK_u;
		keymap[DIK_I] = RBX::SDLK_i;
		keymap[DIK_O] = RBX::SDLK_o;
		keymap[DIK_P] = RBX::SDLK_p;

		keymap[DIK_LBRACKET] = RBX::SDLK_LEFTBRACKET;
		keymap[DIK_AT] = RBX::SDLK_AT;
		keymap[DIK_RBRACKET] = RBX::SDLK_RIGHTBRACKET;
		keymap[DIK_PREVTRACK] = RBX::SDLK_EQUALS;
		keymap[DIK_COLON] = RBX::SDLK_COLON;
		keymap[DIK_KANJI] = RBX::SDLK_BACKQUOTE; // weird key mapping....

		keymap[DIK_RETURN] = RBX::SDLK_RETURN;
		keymap[DIK_LCONTROL] = RBX::SDLK_LCTRL;
		keymap[DIK_A] = RBX::SDLK_a;
		keymap[DIK_S] = RBX::SDLK_s;
		keymap[DIK_D] = RBX::SDLK_d;
		keymap[DIK_F] = RBX::SDLK_f;
		keymap[DIK_G] = RBX::SDLK_g;
		keymap[DIK_H] = RBX::SDLK_h;
		keymap[DIK_J] = RBX::SDLK_j;
		keymap[DIK_K] = RBX::SDLK_k;
		keymap[DIK_L] = RBX::SDLK_l;
		keymap[DIK_SEMICOLON] = RBX::SDLK_SEMICOLON;
		keymap[DIK_APOSTROPHE] = RBX::SDLK_QUOTE;
		keymap[DIK_GRAVE] = RBX::SDLK_BACKQUOTE;
		keymap[DIK_LSHIFT] = RBX::SDLK_LSHIFT;
		keymap[DIK_BACKSLASH] = RBX::SDLK_BACKSLASH;
		keymap[DIK_OEM_102] = RBX::SDLK_BACKSLASH;
		keymap[DIK_Z] = RBX::SDLK_z;
		keymap[DIK_X] = RBX::SDLK_x;
		keymap[DIK_C] = RBX::SDLK_c;
		keymap[DIK_V] = RBX::SDLK_v;
		keymap[DIK_B] = RBX::SDLK_b;
		keymap[DIK_N] = RBX::SDLK_n;
		keymap[DIK_M] = RBX::SDLK_m;
		keymap[DIK_COMMA] = RBX::SDLK_COMMA;
		keymap[DIK_PERIOD] = RBX::SDLK_PERIOD;
		keymap[DIK_SLASH] = RBX::SDLK_SLASH;
		keymap[DIK_RSHIFT] = RBX::SDLK_RSHIFT;
		keymap[DIK_MULTIPLY] = RBX::SDLK_KP_MULTIPLY;
		keymap[DIK_LMENU] = RBX::SDLK_LALT;
		keymap[DIK_SPACE] = RBX::SDLK_SPACE;
		keymap[DIK_CAPITAL] = RBX::SDLK_CAPSLOCK;
		keymap[DIK_F1] = RBX::SDLK_F1;
		keymap[DIK_F2] = RBX::SDLK_F2;
		keymap[DIK_F3] = RBX::SDLK_F3;
		keymap[DIK_F4] = RBX::SDLK_F4;
		keymap[DIK_F5] = RBX::SDLK_F5;
		keymap[DIK_F6] = RBX::SDLK_F6;
		keymap[DIK_F7] = RBX::SDLK_F7;
		keymap[DIK_F8] = RBX::SDLK_F8;
		keymap[DIK_F9] = RBX::SDLK_F9;
		keymap[DIK_F10] = RBX::SDLK_F10;
		keymap[DIK_NUMLOCK] = RBX::SDLK_NUMLOCK;
		keymap[DIK_SCROLL] = RBX::SDLK_SCROLLOCK;
		keymap[DIK_NUMPAD7] = RBX::SDLK_KP7;
		keymap[DIK_NUMPAD8] = RBX::SDLK_KP8;
		keymap[DIK_NUMPAD9] = RBX::SDLK_KP9;
		keymap[DIK_SUBTRACT] = RBX::SDLK_KP_MINUS;
		keymap[DIK_NUMPAD4] = RBX::SDLK_KP4;
		keymap[DIK_NUMPAD5] = RBX::SDLK_KP5;
		keymap[DIK_NUMPAD6] = RBX::SDLK_KP6;
		keymap[DIK_ADD] = RBX::SDLK_KP_PLUS;
		keymap[DIK_NUMPAD1] = RBX::SDLK_KP1;
		keymap[DIK_NUMPAD2] = RBX::SDLK_KP2;
		keymap[DIK_NUMPAD3] = RBX::SDLK_KP3;
		keymap[DIK_NUMPAD0] = RBX::SDLK_KP0;
		keymap[DIK_DECIMAL] = RBX::SDLK_KP_PERIOD;
		keymap[DIK_F11] = RBX::SDLK_F11;
		keymap[DIK_F12] = RBX::SDLK_F12;
		keymap[DIK_F13] = RBX::SDLK_F13;
		keymap[DIK_F14] = RBX::SDLK_F14;
		keymap[DIK_F15] = RBX::SDLK_F15;
		keymap[DIK_NUMPADEQUALS] = RBX::SDLK_KP_EQUALS;
		keymap[DIK_NUMPADENTER] = RBX::SDLK_KP_ENTER;
		keymap[DIK_RCONTROL] = RBX::SDLK_RCTRL;
		keymap[DIK_DIVIDE] = RBX::SDLK_KP_DIVIDE;
		keymap[DIK_SYSRQ] = RBX::SDLK_SYSREQ;
		keymap[DIK_RMENU] = RBX::SDLK_RALT;
		keymap[DIK_PAUSE] = RBX::SDLK_PAUSE;
		keymap[DIK_HOME] = RBX::SDLK_HOME;
		keymap[DIK_UP] = RBX::SDLK_UP;
		keymap[DIK_PRIOR] = RBX::SDLK_PAGEUP;
		keymap[DIK_LEFT] = RBX::SDLK_LEFT;
		keymap[DIK_RIGHT] = RBX::SDLK_RIGHT;
		keymap[DIK_END] = RBX::SDLK_END;
		keymap[DIK_DOWN] = RBX::SDLK_DOWN;
		keymap[DIK_NEXT] = RBX::SDLK_PAGEDOWN;
		keymap[DIK_INSERT] = RBX::SDLK_INSERT;
		keymap[DIK_DELETE] = RBX::SDLK_DELETE;
		keymap[DIK_LWIN] = RBX::SDLK_LMETA;
		keymap[DIK_RWIN] = RBX::SDLK_RMETA;
		keymap[DIK_APPS] = RBX::SDLK_MENU;
		initialized = true;
	}

	return keymap[diKey];
}

// Maps RBX::RBX::SDLK_* to DIK_* 
DWORD UserInputUtil::keyCodeToDirectInput(RBX::KeyCode keyCode)
{
	static DWORD keymap[RBX::SDLK_LAST];
	static bool initialized = false;
	if (!initialized)
	{
		for ( int i=0; i<RBX::SDLK_LAST; ++i )
			keymap[i] = 0;

		keymap[RBX::SDLK_ESCAPE] = DIK_ESCAPE;
		keymap[RBX::SDLK_1] = DIK_1;
		keymap[RBX::SDLK_2] = DIK_2;
		keymap[RBX::SDLK_3] = DIK_3;
		keymap[RBX::SDLK_4] = DIK_4;
		keymap[RBX::SDLK_5] = DIK_5;
		keymap[RBX::SDLK_6] = DIK_6;
		keymap[RBX::SDLK_7] = DIK_7;
		keymap[RBX::SDLK_8] = DIK_8;
		keymap[RBX::SDLK_9] = DIK_9;
		keymap[RBX::SDLK_0] = DIK_0;
		keymap[RBX::SDLK_MINUS] = DIK_MINUS;
		keymap[RBX::SDLK_EQUALS] = DIK_EQUALS;
		keymap[RBX::SDLK_BACKSPACE] = DIK_BACK;
		keymap[RBX::SDLK_TAB] = DIK_TAB;
		keymap[RBX::SDLK_q] = DIK_Q;
		keymap[RBX::SDLK_w] = DIK_W;
		keymap[RBX::SDLK_e] = DIK_E;
		keymap[RBX::SDLK_r] = DIK_R;
		keymap[RBX::SDLK_t] = DIK_T;
		keymap[RBX::SDLK_y] = DIK_Y;
		keymap[RBX::SDLK_u] = DIK_U;
		keymap[RBX::SDLK_i] = DIK_I;
		keymap[RBX::SDLK_o] = DIK_O;
		keymap[RBX::SDLK_p] = DIK_P;
		keymap[RBX::SDLK_LEFTBRACKET] = DIK_LBRACKET;
		keymap[RBX::SDLK_RIGHTBRACKET] = DIK_RBRACKET;
		keymap[RBX::SDLK_RETURN] = DIK_RETURN;
		keymap[RBX::SDLK_LCTRL] = DIK_LCONTROL;
		keymap[RBX::SDLK_a] = DIK_A;
		keymap[RBX::SDLK_s] = DIK_S;
		keymap[RBX::SDLK_d] = DIK_D;
		keymap[RBX::SDLK_f] = DIK_F;
		keymap[RBX::SDLK_g] = DIK_G;
		keymap[RBX::SDLK_h] = DIK_H;
		keymap[RBX::SDLK_j] = DIK_J;
		keymap[RBX::SDLK_k] = DIK_K;
		keymap[RBX::SDLK_l] = DIK_L;
		keymap[RBX::SDLK_SEMICOLON] = DIK_SEMICOLON;
		keymap[RBX::SDLK_QUOTE] = DIK_APOSTROPHE;
		keymap[RBX::SDLK_BACKQUOTE] = DIK_GRAVE;
		keymap[RBX::SDLK_LSHIFT] = DIK_LSHIFT;
		keymap[RBX::SDLK_BACKSLASH] = DIK_BACKSLASH;
		keymap[RBX::SDLK_BACKSLASH] = DIK_OEM_102;
		keymap[RBX::SDLK_z] = DIK_Z;
		keymap[RBX::SDLK_x] = DIK_X;
		keymap[RBX::SDLK_c] = DIK_C;
		keymap[RBX::SDLK_v] = DIK_V;
		keymap[RBX::SDLK_b] = DIK_B;
		keymap[RBX::SDLK_n] = DIK_N;
		keymap[RBX::SDLK_m] = DIK_M;
		keymap[RBX::SDLK_COMMA] = DIK_COMMA;
		keymap[RBX::SDLK_PERIOD] = DIK_PERIOD;
		keymap[RBX::SDLK_SLASH] = DIK_SLASH;
		keymap[RBX::SDLK_RSHIFT] = DIK_RSHIFT;
		keymap[RBX::SDLK_KP_MULTIPLY] = DIK_MULTIPLY;
		keymap[RBX::SDLK_LALT] = DIK_LMENU;
		keymap[RBX::SDLK_SPACE] = DIK_SPACE;
		keymap[RBX::SDLK_CAPSLOCK] = DIK_CAPITAL;
		keymap[RBX::SDLK_F1] = DIK_F1;
		keymap[RBX::SDLK_F2] = DIK_F2;
		keymap[RBX::SDLK_F3] = DIK_F3;
		keymap[RBX::SDLK_F4] = DIK_F4;
		keymap[RBX::SDLK_F5] = DIK_F5;
		keymap[RBX::SDLK_F6] = DIK_F6;
		keymap[RBX::SDLK_F7] = DIK_F7;
		keymap[RBX::SDLK_F8] = DIK_F8;
		keymap[RBX::SDLK_F9] = DIK_F9;
		keymap[RBX::SDLK_F10] = DIK_F10;
		keymap[RBX::SDLK_NUMLOCK] = DIK_NUMLOCK;
		keymap[RBX::SDLK_SCROLLOCK] = DIK_SCROLL;
		keymap[RBX::SDLK_KP7] = DIK_NUMPAD7;
		keymap[RBX::SDLK_KP8] = DIK_NUMPAD8;
		keymap[RBX::SDLK_KP9] = DIK_NUMPAD9;
		keymap[RBX::SDLK_KP_MINUS] = DIK_SUBTRACT;
		keymap[RBX::SDLK_KP4] = DIK_NUMPAD4;
		keymap[RBX::SDLK_KP5] = DIK_NUMPAD5;
		keymap[RBX::SDLK_KP6] = DIK_NUMPAD6;
		keymap[RBX::SDLK_KP_PLUS] = DIK_ADD;
		keymap[RBX::SDLK_KP1] = DIK_NUMPAD1;
		keymap[RBX::SDLK_KP2] = DIK_NUMPAD2;
		keymap[RBX::SDLK_KP3] = DIK_NUMPAD3;
		keymap[RBX::SDLK_KP0] = DIK_NUMPAD0;
		keymap[RBX::SDLK_KP_PERIOD] = DIK_DECIMAL;
		keymap[RBX::SDLK_F11] = DIK_F11;
		keymap[RBX::SDLK_F12] = DIK_F12;
		keymap[RBX::SDLK_F13] = DIK_F13;
		keymap[RBX::SDLK_F14] = DIK_F14;
		keymap[RBX::SDLK_F15] = DIK_F15;
		keymap[RBX::SDLK_KP_EQUALS] = DIK_NUMPADEQUALS;
		keymap[RBX::SDLK_KP_ENTER] = DIK_NUMPADENTER;
		keymap[RBX::SDLK_RCTRL] = DIK_RCONTROL;
		keymap[RBX::SDLK_KP_DIVIDE] = DIK_DIVIDE;
		keymap[RBX::SDLK_SYSREQ] = DIK_SYSRQ;
		keymap[RBX::SDLK_RALT] = DIK_RMENU;
		keymap[RBX::SDLK_PAUSE] = DIK_PAUSE;
		keymap[RBX::SDLK_HOME] = DIK_HOME;
		keymap[RBX::SDLK_UP] = DIK_UP;
		keymap[RBX::SDLK_PAGEUP] = DIK_PRIOR;
		keymap[RBX::SDLK_LEFT] = DIK_LEFT;
		keymap[RBX::SDLK_RIGHT] = DIK_RIGHT;
		keymap[RBX::SDLK_END] = DIK_END;
		keymap[RBX::SDLK_DOWN] = DIK_DOWN;
		keymap[RBX::SDLK_PAGEDOWN] = DIK_NEXT;
		keymap[RBX::SDLK_INSERT] = DIK_INSERT;
		keymap[RBX::SDLK_DELETE] = DIK_DELETE;
		keymap[RBX::SDLK_LMETA] = DIK_LWIN;
		keymap[RBX::SDLK_RMETA] = DIK_RWIN;
		keymap[RBX::SDLK_MENU] = DIK_APPS;
		initialized = true;
	}

	return keymap[keyCode];
}


// Maps RBX::RBX::SDLK_* to VK_* 
DWORD UserInputUtil::keyCodeToVK(RBX::KeyCode keyCode)
{
	static DWORD keymap[RBX::SDLK_LAST];
	static bool initialized = false;
	if (!initialized)
	{
		for ( int i=0; i<RBX::SDLK_LAST; ++i )
			keymap[i] = 0;

		keymap[RBX::SDLK_PRINT] = VK_PRINT;
		keymap[RBX::SDLK_SYSREQ] = VK_SNAPSHOT;
		keymap[RBX::SDLK_ESCAPE] = VK_ESCAPE;
		keymap[RBX::SDLK_BACKSPACE] = VK_BACK;
		keymap[RBX::SDLK_TAB] = VK_TAB;
		keymap[RBX::SDLK_RETURN] = VK_RETURN;
		keymap[RBX::SDLK_LCTRL] = VK_LCONTROL;
		keymap[RBX::SDLK_LSHIFT] = VK_LSHIFT;
		keymap[RBX::SDLK_BACKSLASH] = VK_OEM_102;
		keymap[RBX::SDLK_RSHIFT] = VK_RSHIFT;
		keymap[RBX::SDLK_KP_MULTIPLY] = VK_MULTIPLY;
		keymap[RBX::SDLK_LALT] = VK_LMENU;
		keymap[RBX::SDLK_SPACE] = VK_SPACE;
		keymap[RBX::SDLK_CAPSLOCK] = VK_CAPITAL;
		keymap[RBX::SDLK_F1] = VK_F1;
		keymap[RBX::SDLK_F2] = VK_F2;
		keymap[RBX::SDLK_F3] = VK_F3;
		keymap[RBX::SDLK_F4] = VK_F4;
		keymap[RBX::SDLK_F5] = VK_F5;
		keymap[RBX::SDLK_F6] = VK_F6;
		keymap[RBX::SDLK_F7] = VK_F7;
		keymap[RBX::SDLK_F8] = VK_F8;
		keymap[RBX::SDLK_F9] = VK_F9;
		keymap[RBX::SDLK_F10] = VK_F10;
		keymap[RBX::SDLK_NUMLOCK] = VK_NUMLOCK;
		keymap[RBX::SDLK_SCROLLOCK] = VK_SCROLL;
		keymap[RBX::SDLK_KP7] = VK_NUMPAD7;
		keymap[RBX::SDLK_KP8] = VK_NUMPAD8;
		keymap[RBX::SDLK_KP9] = VK_NUMPAD9;
		keymap[RBX::SDLK_KP_MINUS] = VK_SUBTRACT;
		keymap[RBX::SDLK_KP4] = VK_NUMPAD4;
		keymap[RBX::SDLK_KP5] = VK_NUMPAD5;
		keymap[RBX::SDLK_KP6] = VK_NUMPAD6;
		keymap[RBX::SDLK_KP_PLUS] = VK_ADD;
		keymap[RBX::SDLK_KP1] = VK_NUMPAD1;
		keymap[RBX::SDLK_KP2] = VK_NUMPAD2;
		keymap[RBX::SDLK_KP3] = VK_NUMPAD3;
		keymap[RBX::SDLK_KP0] = VK_NUMPAD0;
		keymap[RBX::SDLK_KP_PERIOD] = VK_DECIMAL;
		keymap[RBX::SDLK_F11] = VK_F11;
		keymap[RBX::SDLK_F12] = VK_F12;
		keymap[RBX::SDLK_F13] = VK_F13;
		keymap[RBX::SDLK_F14] = VK_F14;
		keymap[RBX::SDLK_F15] = VK_F15;
		keymap[RBX::SDLK_KP_ENTER] = VK_RETURN;
		keymap[RBX::SDLK_RCTRL] = VK_RCONTROL;
		keymap[RBX::SDLK_KP_DIVIDE] = VK_DIVIDE;
		keymap[RBX::SDLK_RALT] = VK_RMENU;
		keymap[RBX::SDLK_HOME] = VK_HOME;
		keymap[RBX::SDLK_UP] = VK_UP;
		keymap[RBX::SDLK_PAGEUP] = VK_PRIOR;
		keymap[RBX::SDLK_LEFT] = VK_LEFT;
		keymap[RBX::SDLK_RIGHT] = VK_RIGHT;
		keymap[RBX::SDLK_END] = VK_END;
		keymap[RBX::SDLK_DOWN] = VK_DOWN;
		keymap[RBX::SDLK_PAGEDOWN] = VK_NEXT;
		keymap[RBX::SDLK_INSERT] = VK_INSERT;
		keymap[RBX::SDLK_DELETE] = VK_DELETE;
		keymap[RBX::SDLK_LMETA] = VK_LWIN;
		keymap[RBX::SDLK_RMETA] = VK_RWIN;
		keymap[RBX::SDLK_MENU] = VK_APPS;
		initialized = true;
	}

	return keymap[keyCode];
}

RBX::InputObject::UserInputState UserInputUtil::msgToEventState(UINT uMsg)
{
	switch (uMsg)
	{
	case WM_MOUSEMOVE:
		return RBX::InputObject::INPUT_STATE_CHANGE;
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:  // intentional fall thru
		return RBX::InputObject::INPUT_STATE_BEGIN;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP: // intentional fall thru
		return RBX::InputObject::INPUT_STATE_END;
	default:
		return RBX::InputObject::INPUT_STATE_NONE;
	}
}
RBX::InputObject::UserInputType UserInputUtil::msgToEventType(UINT uMsg)
{
	switch (uMsg)
	{
		case WM_MOUSEMOVE:
			return RBX::InputObject::TYPE_MOUSEMOVEMENT;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP: // intentional fall thru
			return RBX::InputObject::TYPE_MOUSEBUTTON1;
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP: // intentional fall thru
			return RBX::InputObject::TYPE_MOUSEBUTTON2;
		default:
			return RBX::InputObject::TYPE_NONE;
	}
}


RBX::ModCode UserInputUtil::createModCode(const DiKeys& diKeys)
{
	unsigned int modCode = 0;

	if (diKeys[DIK_LSHIFT] & 0x80)
	{
		modCode = modCode | RBX::KMOD_LSHIFT;
	} 
	if (diKeys[DIK_RSHIFT] & 0x80)
	{
		modCode = modCode | RBX::KMOD_RSHIFT;
	} 
	if (diKeys[DIK_LCONTROL] & 0x80)
	{
		modCode = modCode | RBX::KMOD_LCTRL;
	} 
	if (diKeys[DIK_RCONTROL] & 0x80)
	{
		modCode = modCode | RBX::KMOD_RCTRL;
	} 
	if (diKeys[DIK_LMENU] & 0x80)
	{
		modCode = modCode | RBX::KMOD_LALT;
	} 
	if (diKeys[DIK_LMENU] & 0x80)
	{
		modCode = modCode | RBX::KMOD_RALT;
	}
	/*if (diKeys[DIK_CAPSLOCK] & 0x80)
	{
		modCode = modCode | RBX::KMOD_CAPS;
	}*/
	if(::GetKeyState(VK_CAPITAL))
	{
		modCode = modCode | RBX::KMOD_CAPS;
	}

	return (RBX::ModCode)modCode;
}

