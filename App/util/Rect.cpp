/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/Rect.h"
#include "rbx/Debug.h"

namespace RBX {

const float Rect::BORDER_RATIO = 0.15f;			// 0.0 to 1.0;
const float Rect::BORDER_RATIO_DRAG = 0.08f;		// 0.0 to 1.0;
const float Rect::BORDER_RATIO_THIN = 0.02f;		// 0.0 to 1.0;

Rect::Location Rect::pointInBorder(const G3D::Vector2& point, 
									 float borderRatio)
{
	G3D::Vector2 delta = high * borderRatio;

	if (point.x < delta.x)				return LEFT;
	if (point.x > (high.x - delta.x))	return RIGHT;
	if (point.y < delta.y)				return TOP;
	if (point.y > (high.y - delta.y))	return BOTTOM;

	return NONE;
}

void Rect::unionWith(const Rect& other)
{
	low.x = G3D::min(low.x, other.low.x);
	low.y = G3D::min(low.y, other.low.y);
	high.x = G3D::max(high.x, other.high.x);
	high.y = G3D::max(high.y, other.high.y);
}


Vector2 Rect::positionPoint(Location xLocation, Location yLocation) const
{
	Vector2 answer = center();

	switch (xLocation)
	{
	case CENTER:	break;
	case LEFT:		answer.x = low.x; break;
	case RIGHT:		answer.x = high.x; break;
	default:		RBXASSERT(0);	break;
	}	

	switch (yLocation)
	{
	case CENTER:	break;
	case TOP:		answer.y = low.y; break;
	case BOTTOM:	answer.y = high.y; break;
	default:		RBXASSERT(0);	break;
	}	

	return answer;
}


Vector2 Rect::positionPoint(const Vector2& point, Location xLocation, Location yLocation) const
{
	Vector2 answer;

	switch (xLocation)
	{
	case LEFT:		answer.x = low.x + point.x; break;
	case RIGHT:		answer.x = high.x - point.x; break;
	default:		RBXASSERT(0);	break;
	}	

	switch (yLocation)
	{
	case TOP:		answer.y = low.y + point.y; break;
	case BOTTOM:	answer.y = high.y - point.y; break;
	default:		RBXASSERT(0);	break;
	}	

	return answer;
}


Rect Rect::positionChild(const Rect& child, Location xLocation, Location yLocation) const
{
	Vector2 answer;
	Vector2 childSize = child.size();

	switch (xLocation)
	{
		case LEFT:		answer.x = low.x; break;
		case CENTER:	answer.x = 0.5f * (high.x - childSize.x); break;
		case RIGHT:		answer.x = high.x - childSize.x; break;
		default:		RBXASSERT(0);	break;
	}	

	switch (yLocation)
	{
		case TOP:		answer.y = low.y; break;
		case CENTER:	answer.y = 0.5f * (high.y - childSize.y); break;
		case BOTTOM:	answer.y = high.y - childSize.y; break;
		default:		RBXASSERT(0);	break;
	}	
	return Rect(answer, answer + childSize);
}

} // namespace