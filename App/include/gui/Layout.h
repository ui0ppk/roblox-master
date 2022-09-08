/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/Rect.h"
#include "Util/G3DCore.h"

namespace RBX {

	class Layout 
	{
	public:
		enum Style	{HORIZONTAL = 0, VERTICAL = 1};

		Rect::Location		xLocation;
		Rect::Location		yLocation;
		Vector2int16		offset;
		Style				layoutStyle;
		Color4				backdropColor;

		Layout() 
			: xLocation(Rect::LEFT)
			, yLocation(Rect::TOP)
			, offset(Vector2int16(0,0))
			, layoutStyle(HORIZONTAL)
			, backdropColor(Color4::clear())
		{}
	};
}	// namespace
