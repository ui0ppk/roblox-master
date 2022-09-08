 /* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Gui/GuiDraw.h"

#include "GfxBase/Adorn.h"

namespace RBX {

	class UnifiedImageWidget : public UnifiedWidget 
	{
	protected:
		GuiDrawImage guiImageDraw;
		std::string imageName;
		unsigned imageState;
	public:
		UnifiedImageWidget(const std::string& imageName, int imageState) 
			: imageName(imageName)
			, imageState(imageState)
		{
		}

		Gui::WidgetState getWidgetState() const;

		/*override*/ void render2dMe(Adorn* adorn);
		/*override*/ void setSize(const Vector2& _size)		{guiImageDraw.setImageSize(_size);}
		/*override*/ Vector2 getSize(Canvas canvas) const				{return guiImageDraw.getImageSize();}
	};

	class ChatButton : public UnifiedImageWidget
	{
	private:
		/*override*/ bool isVisible() const;
	public:
		ChatButton(const std::string& imageName, unsigned imageState) 
			: UnifiedImageWidget(imageName, imageState)
		{
		}
	};

	class ChatWidget :	public UnifiedWidget
	{
	private:
		typedef UnifiedWidget Super;
		std::string findMenuString(GuiItem* item);

		std::string code;

		// Unified Widget
		/*override*/ void onMenuStateChanged();

		/*override*/ GuiResponse process(const shared_ptr<InputObject>& event);

	public:
		ChatWidget(const std::string& text, std::string code);
	};

} // namespace
