/**
 * UserInputUtil.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "UserInputUtil.h"

// Standard C/C++ Headers
#include <cmath>

// Roblox Headers
#include "util/Rect.h"

using RBX::Vector2;

const float UserInputUtil::HybridSensitivity = 8.0f;
const float UserInputUtil::MouseTug = 15.0f;


// horizontal - will keep doing deltas
// vertial - will peg inset 2 pixels
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
	wrapMousePosition.y = std::max(-halfHeight, wrapMousePosition.y);

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
	RBX::Rect inner = RBX::Rect(-halfSize, halfSize).inset(borderWidth);	// in Wrap Coordinates
	Vector2 oldPosition = wrapMousePosition;
	inner.unionWith(oldPosition);					// now union of the border and old position - ratchet

	Vector2 newPositionUnclamped = oldPosition + delta;
	Vector2 newPositionClamped = inner.clamp(newPositionUnclamped);
	
	Vector2 positiveDistanceInBorder = newPositionUnclamped - newPositionClamped;

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


void UserInputUtil::wrapMouseHorizontalCenter(const Vector2& delta, 
											Vector2& wrapMouseDelta, 
											Vector2& wrapMousePosition)
{
	wrapMouseDelta.x += delta.x;
//	wrapMousePosition = G3D::Vector2::zero();
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
		
		if(autoMoveMouse && wrapMousePosition != Vector2::zero())
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
			autoMoveMouse = false;
		}
		
  }
	
	wrapMousePosition += delta;
	
}

// Maps DIK_* to RBX::SDLK_* 
RBX::KeyCode UserInputUtil::directInputToKeyCode(DWORD diKey)
{
	RBXASSERT(diKey<256);

	static RBX::KeyCode keymap[256];
	static bool initialized = false;
	if (!initialized)
	{
		for ( int i=0; i<256; ++i )
			keymap[i] = RBX::SDLK_UNKNOWN;
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
		initialized = true;
	}

	return keymap[keyCode];
}


RBX::ModCode UserInputUtil::createModCode(const DiKeys& diKeys)
{
	unsigned int modCode = 0;
	return (RBX::ModCode)modCode;
}


