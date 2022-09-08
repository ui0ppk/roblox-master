/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Gui/ChatWidget.h"
#include "Network/Players.h"
#include "Util/SoundService.h"

namespace RBX {

Gui::WidgetState UnifiedImageWidget::getWidgetState() const
{
	switch (getMenuState())
	{
	default:
	case NOTHING:	return Gui::NOTHING;

	case HOVER:		return Gui::HOVER;

	case SHOWN_APPEARING:
	case SHOWN:		return Gui::DOWN_OVER;
	}
}

void UnifiedImageWidget::render2dMe(Adorn* adorn)
{
	if (isVisible()) {
		if (guiImageDraw.setImageFromName(adorn, imageName, imageState)) {			
			guiImageDraw.render2d(adorn, true, getMyRect(adorn->getCanvas()), getWidgetState(), false);
		}
	}
}

//////////////////////////////////////////////////

bool ChatButton::isVisible() const
{
	RBX::Network::Players* players = ServiceProvider::find<RBX::Network::Players>(this);

	return (players && players->getLocalPlayer() && RBX::Network::Players::clientIsPresent(this));
}

//////////////////////////////////////////////////

ChatWidget::ChatWidget(const std::string& text, std::string code)
{
	setName(text);
	this->code = code;
}

std::string ChatWidget::findMenuString(GuiItem* item)
{
	if (Instance::fastDynamicCast<ChatWidget>(item))
	{
		RBXASSERT(item->getParent());

		int childIndex = item->getParent()->findChildIndex(item);

		return findMenuString(item->getGuiParent()) + "_" + StringConverter<int>::convertToString(childIndex);
	}
	else
	{
		return "";
	}
}


void ChatWidget::onMenuStateChanged()
{
	if (getMenuState() == HOVER)
	{
		setMenuState(SHOWN);
	}
}

GuiResponse ChatWidget::process(const shared_ptr<InputObject>& event)
{
	if (	isVisible() 
		&&	event->isMouseEvent()
		&&	(event->isLeftMouseUpEvent())
		&& 	getMyRect(Canvas(event->getWindowSize())).pointInRect(event->get2DPosition())) 
	{
		if (Network::Players* players = ServiceProvider::find<Network::Players>(this))
		{
			ServiceProvider::create<Soundscape::SoundService>(this)->playSound(SoundWorld::ClickSound());
			//std::string chatString = "Chat" + findMenuString(this);
			std::string chatString = "/sc " + this->code;
			players->chat(chatString);
			return GuiResponse::sunkAndFinished();
		}
	}
	return Super::process(event);
}



} // namespace