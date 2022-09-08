#include "stdafx.h"

#include "V8DataModel/TextButton.h"
#include "Util/Rect.h"
#include "Network/Players.h"

namespace RBX {

const char* const sGuiTextButton = "TextButton";

GuiTextButton::GuiTextButton()
	: DescribedCreatable<GuiTextButton,GuiButton,sGuiTextButton>("TextButton")
	, GuiTextMixin("Button", BrickColor::brickBlack().color3())
{}

GuiTextButton::GuiTextButton(Verb* v)
	: DescribedCreatable<GuiTextButton,GuiButton,sGuiTextButton>("TextButton")
	, GuiTextMixin("Button", BrickColor::brickBlack().color3())
{
	verb = v;
}

IMPLEMENT_GUI_TEXT_MIXIN(GuiTextButton)

void GuiTextButton::render2d(Adorn* adorn)
{
	render2dContext(adorn, NULL);
}

void GuiTextButton::render2dContext(Adorn* adorn, const Instance* context)
{
	Rect2D rect;
	render2dButtonImpl(adorn, rect);

	if(filterState == ContentFilter::Waiting)
		if(ContentFilter* contentFilter = ServiceProvider::create<ContentFilter>(this))
			filterState = contentFilter->getStringState(text);

	if(filterState != ContentFilter::Succeeded)
		if(ContentFilter* contentFilter = ServiceProvider::create<ContentFilter>(context))
			filterState = contentFilter->getStringState(text);

	if(filterState == ContentFilter::Succeeded)
		render2dTextImpl(adorn, rect, getText(), getFont(), getFontSize(), getRenderTextColor4(), getRenderTextStrokeColor4(), getTextWrap(), getTextScale(), getXAlignment(), getYAlignment());

	renderStudioSelectionBox(adorn);
}


	

} // Namespace
