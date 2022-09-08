#pragma once

#include "V8DataModel/GuiObject.h"
#include "V8DataModel/GuiMixin.h"

namespace RBX
{
	extern const char* const sGuiImageButton;

	class GuiImageButton
		: public DescribedCreatable<GuiImageButton, GuiButton, sGuiImageButton>
		, public GuiImageMixin
	{
	public:
		GuiImageButton();
		GuiImageButton(Verb* verb);

		DECLARE_GUI_IMAGE_MIXIN(GuiImageButton);		

		void setImageState(unsigned imageState)
		{
			this->imageState = imageState;
		}

	private:
		typedef DescribedCreatable<GuiImageButton, GuiButton, sGuiImageButton> Super;
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ void render2d(Adorn* adorn);
	protected:
		unsigned imageState;
	};
}
