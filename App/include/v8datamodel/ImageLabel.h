#pragma once

#include "V8DataModel/GuiObject.h"
#include "V8DataModel/GuiMixin.h"

namespace RBX
{
	extern const char* const sImageLabel;

	class ImageLabel 
		: public DescribedCreatable<ImageLabel, GuiLabel, sImageLabel>
		, public GuiImageMixin
	{	
	public:
		ImageLabel();

		DECLARE_GUI_IMAGE_MIXIN(ImageLabel);

	private:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void renderBackground2d(Adorn* adorn);
	};
}
