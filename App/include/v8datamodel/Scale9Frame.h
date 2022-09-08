/* Copyright 2003-2010 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/Frame.h"
#include "Util/TextureId.h"
#include "Gui/GuiDraw.h"

namespace RBX { 
	extern const char* const sScale9Frame;

	class Scale9Frame
		: public DescribedNonCreatable<Scale9Frame, GuiObject, sScale9Frame>
	{
	private:
		typedef DescribedNonCreatable<Scale9Frame, GuiObject, sScale9Frame> Super;
		
		Vector2int16 scaleEdgeSize;
		std::string slicePrefix;
		GuiDrawImage image;
	public:
		Scale9Frame();

		std::string getSlicePrefix() const { return slicePrefix; }
		void setSlicePrefix(std::string value);

		Vector2int16 getScaleEdgeSize() const { return scaleEdgeSize; }
		void setScaleEdgeSize(Vector2int16 value);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ void render2d(Adorn* adorn);
	};
}