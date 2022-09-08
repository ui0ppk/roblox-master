/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Message.h"
#include "Util/G3DCore.h"
#include "Util/Rect.h"
#include "AppDraw/DrawPrimitives.h"
#include "GfxBase/Adorn.h"


#include "Network/Player.h"
#include "Gui/ProfanityFilter.h"



void (*gMessageSetTextCallback)(const RBX::Message*);
void (*gMessageGetTextCallback)(const std::string& text);
void (*gMessageDtorCallback)(const RBX::Message*);

namespace RBX {

const char* const sMessage	= "Message";
const char* const sHint		= "Hint";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<Message, std::string> desc_Text("Text", category_Appearance, &Message::getText, &Message::setText);
REFLECTION_END();

Message::Message()
	:DescribedCreatable<Message, Instance, sMessage>("Message")
{
}

Message::~Message()
{
	if (gMessageDtorCallback)
	    gMessageDtorCallback(this);
}

void Message::setText(const std::string& value)		{
	if (!ProfanityFilter::ContainsProfanity(value))
	{
		std::string newValue = value;
		if(newValue.size() > ContentFilter::MAX_CONTENT_FILTER_SIZE){
			newValue = newValue.substr(0, ContentFilter::MAX_CONTENT_FILTER_SIZE);
		}

		if(text != newValue){
			text = newValue;
			filterState = ContentFilter::Waiting;
			this->raiseChanged(desc_Text);

			if (gMessageSetTextCallback)
				gMessageSetTextCallback(this);
		}
	}
}

const std::string& Message::getText() const {
	if (gMessageGetTextCallback)
	    gMessageGetTextCallback(this->text);
    return text;
}

void Message::renderFullScreen(Adorn* adorn)
{
	RBXASSERT(filterState == ContentFilter::Succeeded);

    Rect2D rect = adorn->getUserGuiRect();

	Rect translucentArea = rect;

	translucentArea.inset(100);

	Vector2 textPos = translucentArea.center();
	
	adorn->rect2d(translucentArea.toRect2D(), Color4(0.5, 0.5, 0.5, 0.5));

	adorn->drawFont2D(		text,
							textPos,
							14,			// size
                            false,
							Color3::white(),
							Color3::black(),
							Text::FONT_LEGACY,
							Text::XALIGN_CENTER,
							Text::YALIGN_CENTER );
}

void Message::render2d(Adorn* adorn)
{
	if (text.size() > 0)
	{
		if (filterState == ContentFilter::Waiting)
        {
			if (ContentFilter* contentFilter = ServiceProvider::create<ContentFilter>(this))
            {
				filterState = contentFilter->getStringState(text);
			}
		}

		if (filterState == ContentFilter::Succeeded)
        {
            renderFullScreen(adorn);
		}
	}
}

void Hint::render2d(Adorn* adorn)
{
    Rect2D rect = adorn->getUserGuiRect();
    Rect blackArea = Rect(rect.x0(), rect.y0(), rect.x1(), rect.y0() + 20);
	Vector2 textPos = blackArea.center();
    
	adorn->rect2d(blackArea.toRect2D(), Color3::black());

	adorn->drawFont2D(		text,
							textPos,
							14,			// size
                            false,
							Color3::white(),
							Color4::clear(),
							Text::FONT_LEGACY,
							Text::XALIGN_CENTER,
							Text::YALIGN_CENTER );


}	

} // namespace
