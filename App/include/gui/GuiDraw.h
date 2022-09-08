/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/GuiCore.h"
#include "V8Xml/Reference.h" 
#include "Util/TextureId.h"
#include "GfxBase/TextureProxyBase.h"
#include "GfxBase/Adorn.h"
#include "rbx/signal.h"

namespace RBX {

	class Adorn;

	class GuiDrawImage
	{
	public:
		enum ImageState
		{
			NORMAL			= 0x1,
			HOVER			= 0x2,
			DOWN			= 0x4,
			DISABLE			= 0x8,
			SELECTED		= 0x10,
			SELECTED_HOVER	= 0x20,
			SELECTED_DOWN	= 0x40,
			ALL				= 0x7F
		};
	private:
		TextureId currentTexture;
		TextureId loadingTexture;
		RBX::TextureProxyBaseRef disable;
		RBX::TextureProxyBaseRef normal;
		RBX::TextureProxyBaseRef hover;
		RBX::TextureProxyBaseRef down;
		RBX::TextureProxyBaseRef selected;
		RBX::TextureProxyBaseRef selectedHover;
		RBX::TextureProxyBaseRef selectedDown;
		mutable Vector2 size;
		rbx::signals::scoped_connection unbindResourceSignalHint;

		void OnUnbindResourceSignalHint();

		void draw(Adorn* adorn, const RBX::TextureProxyBaseRef& texture, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color,
			const Rect& clipRect, const Color4& behind, const Color4& inFront);

		void draw(Adorn* adorn, const RBX::TextureProxyBaseRef& texture, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color,
            const Rotation2D& rotation, const Color4& behind, const Color4& inFront);

        template <typename Modifier>
		void render2dImpl(Adorn* adorn, bool enabled, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color,
			const Modifier& modifier, Gui::WidgetState state, bool isSelected);

		void tryCreateTextureProxy(Adorn *adorn, const std::string& contentString, const std::string& context, RBX::TextureProxyBaseRef& textureRef, bool& isWaiting);
	public:
		GuiDrawImage() : size(Vector2(0,0)) {}
		GuiDrawImage(Adorn *adorn, const std::string& textureName, unsigned imageState) {setImageFromName(adorn, textureName, imageState);}

		void render2d(Adorn* adorn, bool enabled, const Rect& rect,
			Gui::WidgetState state, bool isSelected);
		void render2d(Adorn* adorn, bool enabled, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Rotation2D& rotation,
			Gui::WidgetState state, bool isSelected);
		void render2d(Adorn* adorn, bool enabled, const Rect& rect, const Vector2& texul, const Vector2& texbr, const Color4& color, const Rect& clipRect,
			Gui::WidgetState state, bool isSelected);

		void setImageSize(const Vector2& _size);
		Vector2 getImageSize() const;

		bool setImage(Adorn* adorn, const TextureId& textureId, unsigned imageState, Vector2* outSize = NULL, Instance* contextInstance = NULL, const char* context = "");
		bool setImageFromName(Adorn* adorn, const std::string& textureName, unsigned imageState, Instance* contextInstance = NULL, const char* context = "");
		
		void computeUV(Vector2& uvtl, Vector2& uvbr, const Vector2& imageRectOffset, const Vector2& imageRectSize, const Vector2& imageSize);
	};

} // namespace
