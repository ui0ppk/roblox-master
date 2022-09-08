#include "stdafx.h"

#include "V8DataModel/TextLabel.h"
#include "Network/Players.h"

DYNAMIC_FASTFLAGVARIABLE(TextTransparencyRenderingFix, false)

namespace RBX {

const char* const sTextLabel = "TextLabel";

TextLabel::TextLabel()
	: DescribedCreatable<TextLabel,GuiLabel,sTextLabel>("TextLabel")
	, GuiTextMixin("Label",BrickColor::brickBlack().color3())
{
	setGuiQueue(GUIQUEUE_TEXT);
}

IMPLEMENT_GUI_TEXT_MIXIN(TextLabel);

void TextLabel::render2d(Adorn* adorn) 
{
	render2dContext(adorn, NULL);	
}

void TextLabel::render2dContext(Adorn* adorn, const Instance* context)
{
	if(filterState == ContentFilter::Waiting)
		if(ContentFilter* contentFilter = ServiceProvider::create<ContentFilter>(this))
			filterState = contentFilter->getStringState(text);

	if(filterState != ContentFilter::Succeeded)
		if(ContentFilter* contentFilter = ServiceProvider::create<ContentFilter>(context))
			filterState = contentFilter->getStringState(text);

	if(filterState == ContentFilter::Succeeded)
	{
        if (getRenderTextAlpha(getTextTransparency()) > 0.0f) {
		    render2dTextImpl(adorn, getRenderBackgroundColor4(), getText(), 
			    getFont(), getFontSize(), getRenderTextColor4(),getRenderTextStrokeColor4(), getTextWrap(), getTextScale(),
			    getXAlignment(), getYAlignment());
        } 
        else if (DFFlag::TextTransparencyRenderingFix)
        {
            render2dImpl(adorn, getRenderBackgroundColor4());
        }
	}

	renderStudioSelectionBox(adorn);
}

}  // namespace RBX
