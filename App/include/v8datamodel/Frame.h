#pragma once

#include "V8DataModel/GuiObject.h"

namespace RBX
{
	extern const char* const sFrame;

	class Frame 
		: public DescribedCreatable<Frame, GuiObject, sFrame>
	{
	private:
		typedef DescribedCreatable<Frame, GuiObject, sFrame> Super;
	public:
		enum Style
		{
			CUSTOM_STYLE			= 0,
			BLUE_CHAT_STYLE			= 1,
			ROBLOX_SQUARE_STYLE		= 2,
			ROBLOX_ROUND_STYLE		= 3,
			GREEN_CHAT_STYLE		= 4,
			RED_CHAT_STYLE			= 5,
			ROBLOX_DROPSHADOW_STYLE	= 6,
		};

		Frame();

		Style getStyle() const { return style; }
		void setStyle(Style style);

		
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ void render2d(Adorn* adorn);

		
		/////////////////////////////////////////////////////////////
		// GuiBase2d
		//
		/*override*/ Rect2D getChildRect2D() const;
	private:
		Style style;
		GuiDrawImage image;
	};
}
