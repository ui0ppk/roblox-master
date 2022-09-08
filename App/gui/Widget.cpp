/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Gui/Widget.h"
#include "AppDraw/Draw.h"
#include "AppDraw/DrawPrimitives.h"

namespace RBX {

/////////////////////////////////////////////////////////////////////////////////////
//
// Widget
//


Widget::Widget() : widgetState(Gui::NOTHING)
{
	setGuiSize(Vector2(100,24));
}


GuiResponse Widget::processMouse(const shared_ptr<InputObject>& event)
{
	bool mouseOver = getMyRect(Canvas(event->getWindowSize())).pointInRect(event->get2DPosition());

	switch (widgetState)
	{
	case Gui::NOTHING:
	case Gui::HOVER:
		if (mouseOver) 
		{
			if (!isEnabled()) 
			{
				widgetState = Gui::NOTHING;	
				return GuiResponse::sunk();
			}
			else 
			{
				switch (event->getUserInputType()) 
				{
				case InputObject::TYPE_MOUSEMOVEMENT:	
					{
						widgetState = Gui::HOVER;
						return GuiResponse::sunk();
					}
				case InputObject::TYPE_MOUSEBUTTON1:	
					{
						if(event->isLeftMouseDownEvent())
						{
							widgetState = Gui::DOWN_OVER;	
							return GuiResponse::sunk();
						}
					}
				default: 
					return GuiResponse::sunk();
				}
			}
		}
		else 
		{
			widgetState = Gui::NOTHING;	
			return GuiResponse::notSunk();
		}

	case Gui::DOWN_OVER:
		switch (event->getUserInputType())
		{
			case InputObject::TYPE_MOUSEBUTTON1:
			{
				if(event->isLeftMouseDownEvent())
					return GuiResponse::sunk();
				else if (event->isLeftMouseUpEvent())
				{
					widgetState = Gui::NOTHING;
					if (mouseOver) 
					{
						onClick(event);
						return GuiResponse::sunkAndFinished();
					}
					else
						return GuiResponse::notSunk();
				}
				break;
			}
			case InputObject::TYPE_MOUSEMOVEMENT:
			{
				if (!mouseOver)	// mouse was over, no longer over
					widgetState = Gui::DOWN_AWAY;
				return GuiResponse::sunk();
			}
            default:
                break;
		}
		break;

	case Gui::DOWN_AWAY:
		switch (event->getUserInputType())
		{
			case InputObject::TYPE_MOUSEMOVEMENT:
			{
				widgetState = mouseOver ? Gui::DOWN_OVER : Gui::DOWN_AWAY;
				return GuiResponse::sunk();
			}
			case InputObject::TYPE_MOUSEBUTTON1:
			{
				// left button down
				if(event->isLeftMouseDownEvent())
					return GuiResponse::sunk();
				else if(event->isLeftMouseUpEvent())
				{
					widgetState = Gui::NOTHING;
					if (mouseOver)
					{
						onClick(event);
						return GuiResponse::sunk();
					}
					else
						return GuiResponse::notSunk();
				}
				break;
			}
            default:
                break;
		}
		break;

	default:
		RBXASSERT(0);
		break;
	}
	return GuiResponse::notSunk();
}
	
GuiResponse Widget::processKey(const shared_ptr<InputObject>& event)
{
	return GuiResponse::notSunk();
}

GuiResponse Widget::process(const shared_ptr<InputObject>& event)
{
	if (!isEnabled()) {
		return GuiResponse::notSunk();
	}

	if (event->isMouseEvent()) {
		return processMouse(event);
	}
	else {
		return processKey(event);
	}
}

void Widget::render2d(Adorn* adorn)
{
	if (!isVisible()) {
		return;
	}

	if (widgetState == Gui::HOVER || widgetState == Gui::DOWN_AWAY) {
		adorn->rect2d(getMyRect2D(adorn->getCanvas()), Color3::gray());
	}
	else if (widgetState == Gui::DOWN_OVER) {
		adorn->rect2d(getMyRect2D(adorn->getCanvas()), Color3::yellow());
	}

	label2d(
		adorn,
		getTitle(), 
		isEnabled() ? getFontColor() : disabledFill(),
		G3D::Color4(.5,.5,.5,.25) );
}





} // namespace