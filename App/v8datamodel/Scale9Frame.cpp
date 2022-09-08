#include "stdafx.h"

#include "V8DataModel/Scale9Frame.h"

namespace RBX
{

REFLECTION_BEGIN();
const char* const sScale9Frame= "Scale9Frame";
static Reflection::PropDescriptor<Scale9Frame, Vector2int16> prop_scaleEdgeSize("ScaleEdgeSize", category_Data, &Scale9Frame::getScaleEdgeSize, &Scale9Frame::setScaleEdgeSize);
static Reflection::PropDescriptor<Scale9Frame, std::string> prop_slicePrefix("SlicePrefix", category_Data, &Scale9Frame::getSlicePrefix, &Scale9Frame::setSlicePrefix);
REFLECTION_END();

Scale9Frame::Scale9Frame() 
	: DescribedNonCreatable<Scale9Frame, GuiObject, sScale9Frame>(sScale9Frame, true)
	, scaleEdgeSize(0,0)
{

}
void Scale9Frame::setSlicePrefix(std::string value)
{
	if(slicePrefix != value)
	{
		slicePrefix = value;
		raisePropertyChanged(prop_slicePrefix);
	}
}

void Scale9Frame::setScaleEdgeSize(Vector2int16 value)
{
	if(scaleEdgeSize != value)
	{
		scaleEdgeSize = value;
		raisePropertyChanged(prop_scaleEdgeSize);

		checkForResize();
	}
}

void Scale9Frame::render2d(Adorn* adorn) 
{
	Vector2 imageSize; 
	if(image.setImage(adorn, slicePrefix, GuiDrawImage::NORMAL, &imageSize, this, ".SlicePrefix"))
	{
		Rect2D rect = Rect2D::xyxy(scaleEdgeSize, imageSize - scaleEdgeSize );
		Color4 color(Color3::white(), 1.0f - getBackgroundTransparency());
		render2dScale9Impl2(adorn, slicePrefix, image, rect, NULL, color);
	}
}

}
