#include "stdafx.h"

#include "V8DataModel/ImageButton.h"

namespace RBX {

const char* const sGuiImageButton = "ImageButton";


GuiImageButton::GuiImageButton()
	: DescribedCreatable<GuiImageButton,GuiButton,sGuiImageButton>("ImageButton")
	, GuiImageMixin()
	, imageState(GuiDrawImage::ALL)
{}

GuiImageButton::GuiImageButton(Verb* theVerb) : DescribedCreatable<GuiImageButton,GuiButton,sGuiImageButton>("ImageButton")
	, GuiImageMixin()
	, imageState(GuiDrawImage::ALL)
{
	verb = theVerb;
}

IMPLEMENT_GUI_IMAGE_MIXIN(GuiImageButton);

void GuiImageButton::render2d(Adorn* adorn) 
{
	//Render the background
	Rect2D rect;
	render2dButtonImpl(adorn, rect);

	Gui::WidgetState oldGuiState = guiState;
	if (verb && verb->isSelected()) 
	{
		guiState = Gui::DOWN_OVER;
		active = verb->isEnabled();
	}

	renderImage(adorn);

	guiState = oldGuiState;
}

}
