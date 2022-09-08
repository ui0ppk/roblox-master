#pragma once

#include "V8DataModel/GuiObject.h"
#include "V8DataModel/GuiText.h"

namespace RBX
{
	extern const char* const sGuiTextButton;

	class GuiTextButton
		: public DescribedCreatable<GuiTextButton, GuiButton, sGuiTextButton>
		, public GuiTextMixin
	{
	private:
		typedef DescribedCreatable<GuiTextButton, GuiButton, sGuiTextButton> Super;
	public:
		GuiTextButton();
		GuiTextButton(Verb* v);

		DECLARE_GUI_TEXT_MIXIN();
		
	private:
		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void render2dContext(Adorn* adorn, const Instance* context);
	};
}
