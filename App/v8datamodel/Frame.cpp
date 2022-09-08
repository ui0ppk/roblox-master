#include "stdafx.h"

#include "V8DataModel/Frame.h"
#include "Gui/Guidraw.h"


namespace RBX
{
	const char* const sFrame = "Frame";
	const Reflection::EnumPropDescriptor<Frame, Frame::Style> prop_style("Style", category_Data, &Frame::getStyle, &Frame::setStyle);

	namespace Reflection {
	template<>
	EnumDesc<Frame::Style>::EnumDesc()
		:EnumDescriptor("FrameStyle")
	{
		addPair(Frame::CUSTOM_STYLE, "Custom");
		addPair(Frame::BLUE_CHAT_STYLE, "ChatBlue");
		addPair(Frame::ROBLOX_SQUARE_STYLE, "RobloxSquare");
		addPair(Frame::ROBLOX_ROUND_STYLE, "RobloxRound");
		addPair(Frame::GREEN_CHAT_STYLE, "ChatGreen");
		addPair(Frame::RED_CHAT_STYLE, "ChatRed");
		addPair(Frame::ROBLOX_DROPSHADOW_STYLE, "DropShadow");
	}
	}//namespace Reflection


	Frame::Frame()
		: DescribedCreatable<Frame, GuiObject, sFrame>("Frame", false)
		, style(Frame::CUSTOM_STYLE)
	{}

	Rect2D Frame::getChildRect2D() const 
	{	
		switch(style)
		{
			case Frame::CUSTOM_STYLE:
				return getRect2D(); 
			case Frame::BLUE_CHAT_STYLE:
			case Frame::GREEN_CHAT_STYLE:
			case Frame::RED_CHAT_STYLE:
				return Scale9Rect2D(getRect2D(), 17, 60);
			case Frame::ROBLOX_SQUARE_STYLE:
			case Frame::ROBLOX_ROUND_STYLE:
			case Frame::ROBLOX_DROPSHADOW_STYLE:
				return Scale9Rect2D(getRect2D(), 8, 21);
			default:
				RBXASSERT(0);
				return getRect2D();
		}
	}

	void Frame::setStyle(Style value)
	{
		if(style != value)
		{
			style = value;
			raisePropertyChanged(prop_style);

			forceResize();
		}
	}

	
	void Frame::render2d(Adorn* adorn) 
	{
		static const TextureId sBlue =			TextureId("rbxasset://textures/ui/dialog_blue.png");
		static const TextureId sGreen =			TextureId("rbxasset://textures/ui/dialog_green.png");
		static const TextureId sRed =			TextureId("rbxasset://textures/ui/dialog_red.png");
		static const TextureId sBlackSquare =	TextureId("rbxasset://textures/blackBkg_square.png");
		static const TextureId sBlackRound =	TextureId("rbxasset://textures/blackBkg_round.png");
		static const TextureId sNewSquare =		TextureId("rbxasset://textures/ui/newBkg_square.png");

		GuiObject* clippingObject = firstAncestorClipping();

		switch(style)
		{
		case Frame::CUSTOM_STYLE:
			Super::render2d(adorn);
			break;
		case Frame::BLUE_CHAT_STYLE:
		{
            Rect2D rect = Rect2D::xyxy(7, 7, 33, 33);
            Color4 color = Color3::white();
            render2dScale9Impl2(adorn, sBlue, image, rect, clippingObject, color);
			break;
		}
		case Frame::GREEN_CHAT_STYLE:
		{
            Rect2D rect = Rect2D::xyxy(7, 7, 33, 33);
            Color4 color = Color3::white();
            render2dScale9Impl2(adorn, sGreen, image, rect, clippingObject, color);
			break;
		}
		case Frame::RED_CHAT_STYLE:
		{
            Rect2D rect = Rect2D::xyxy(7, 7, 33, 33);
            Color4 color = Color3::white();
			render2dScale9Impl2(adorn, sRed, image, rect, clippingObject, color);
			break;
		}
		case Frame::ROBLOX_SQUARE_STYLE:
		{
			Rect2D rect;
			setBackgroundTransparency(0.0f);
			render2dScale9Impl(adorn, sBlackSquare, Vector2int16(7,7), Vector2(14,14), image, rect, clippingObject);
			break;
		}
		case Frame::ROBLOX_ROUND_STYLE:
		{
			Rect2D rect;
			setBackgroundTransparency(0.0f);
			render2dScale9Impl(adorn, sBlackRound, Vector2int16(7,7), Vector2(14,14), image, rect, clippingObject);
			break;
		}
		case Frame::ROBLOX_DROPSHADOW_STYLE:
		{
			Rect2D rect;
			setBackgroundTransparency(0.0f);
			render2dScale9Impl(adorn, sNewSquare, Vector2int16(10,10), Vector2(40,40), image, rect, clippingObject);
			break;
		}
		}
		
		renderStudioSelectionBox(adorn);
	}
}
