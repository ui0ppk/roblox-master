#include "stdafx.h"

#include "V8DataModel/ImageLabel.h"


namespace RBX {

const char* const sImageLabel = "ImageLabel";


ImageLabel::ImageLabel()
	: DescribedCreatable<ImageLabel,GuiLabel,sImageLabel>("ImageLabel")
	, GuiImageMixin()
{}

IMPLEMENT_GUI_IMAGE_MIXIN(ImageLabel);

void ImageLabel::render2d(Adorn* adorn) 
{
	renderImage(adorn);
}

void ImageLabel::renderBackground2d(Adorn* adorn) 
{
	if(this->getBackgroundTransparency() < 1.0)
	{
		render2dImpl(adorn, getRenderBackgroundColor4());
	}
}

}
