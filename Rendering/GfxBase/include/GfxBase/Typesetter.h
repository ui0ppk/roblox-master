#pragma once

#include "Util/G3DCore.h"
#include "Util/Rotation2D.h"
#include "GfxBase/Type.h"

namespace  RBX {
	class Adorn;

	namespace  Graphics {
		class Texture;
        class TextureManager;
        class TextureAtlas;
	};

	//abstract base class
	class Typesetter {
	public:
		virtual ~Typesetter() {}

		virtual Vector2 draw(
			Adorn*				adorn,
			const std::string&  s,
			const Vector2&      position,
			float               size,
            bool                autoScale,
			const Color4&       color,
			const Color4&       outline,
			RBX::Text::XAlign   xalign  = RBX::Text::XALIGN_LEFT,
			RBX::Text::YAlign   yalign  = RBX::Text::YALIGN_TOP,
			const Vector2&		availableSpace = Vector2::zero(),
			const Rect2D&		clippingRect = Rect2D::xyxy(-1,-1,-1,-1),
			const Rotation2D&   rotation = Rotation2D()) const = 0;


		virtual int getCursorPositionInText(
			const std::string&      s,
			const RBX::Vector2&     pos2D,
			float                   size,
			RBX::Text::XAlign       xalign,
			RBX::Text::YAlign       yalign,
			const RBX::Vector2&		availableSpace,
			const Rotation2D&       rotation,
			RBX::Vector2			cursorPos) const = 0;


		/**
		 Useful for drawing centered text and boxes around text.
		 */
		virtual Vector2 measure(
			const std::string&  s,
			float size,
			const Vector2& availableSpace = Vector2::zero(),
			bool* textFits = NULL
			) const = 0;

        virtual void loadResources(RBX::Graphics::TextureManager* textureManager, RBX::Graphics::TextureAtlas* glyphAtlas) = 0;
		virtual void releaseResources() = 0;
		virtual const shared_ptr<Graphics::Texture>& getTexture() const = 0;

		static bool isCharNonWhitespace(char c)
		{
			return (c >= '!' && c <='~');
		}
		static bool isCharWhitespace(char c)
		{
			return (c == ' ' || c == '\t' || c == '\n');
		}
		static bool isCharSupported(char c)
		{
			return isCharNonWhitespace(c) || isCharWhitespace(c);
		}
		static bool isStringSupported(std::string& stringToCheck)
		{
			for (std::string::iterator iter = stringToCheck.begin(); iter != stringToCheck.end(); ++iter)
			{
				if (!isCharSupported(*iter))
				{
					return false;
				}
			}
			return true;
		}
	};
}
