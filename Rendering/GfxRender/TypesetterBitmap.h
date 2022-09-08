#pragma once

#include "GfxBase/Typesetter.h"

#include <boost/scoped_array.hpp>

#include "TextureRef.h"

namespace RBX
{
namespace Graphics
{
    class TextureManager;

	class TypesetterBitmap: public Typesetter
	{
	public:
		TypesetterBitmap(TextureManager* textureManager, const std::string& fontPath, const std::string& texturePath, float legacyHeightScale, bool retina);
		
		virtual Vector2 draw(
			Adorn*				adorn,
			const std::string&  s,
			const Vector2&      position,
			float               size,
            bool                autoScale,
			const Color4&       color,
			const Color4&       outline,
			RBX::Text::XAlign   xalign,
			RBX::Text::YAlign   yalign,
			const Vector2&		availableSpace,
			const Rect2D&		clippingRect,
			const Rotation2D&   rotation) const;

		virtual int getCursorPositionInText(
			const std::string&      s,
			const RBX::Vector2&     pos2D,
			float                   size,
			RBX::Text::XAlign       xalign,
			RBX::Text::YAlign       yalign,
			const RBX::Vector2&		availableSpace,
			const Rotation2D&       rotation,
			RBX::Vector2			cursorPos) const;

		virtual Vector2 measure(
			const std::string&  s,
			float size,
			const Vector2& availableSpace,
			bool* textFits
			) const;
	
		const shared_ptr<Texture>& getTexture() const
		{
			return texture.getTexture();
		}

		void releaseResources()
		{
			texture = TextureRef();
		}

		void loadResources(RBX::Graphics::TextureManager* textureManager, RBX::Graphics::TextureAtlas* glyphAtlas);

		
	private:
		struct Info
		{
			int sizeCount;
			int glyphCount;
		};
		
		struct Glyph
		{
			unsigned short code;
			short xoffset;
			short yoffset;
			short xadvance;
			short width;
			short height;
			unsigned int kerningOffset;

			float u0, v0;
			float u1, v1;
		};
		
		struct GlyphLine
		{
			const char* data;
			size_t length;
			
			int yoffset;
			int width;
		};
		
		float legacyHeightScale;
        float retinaScale;
		
		Info info;
		
		const unsigned char* glyphRemap; // glyph code -> glyph index
		const float* sizeTable;
		const Glyph* glyphTable;
		const char* kerningTable;

		std::string fontTexturePath;

		boost::scoped_array<char> fontData;
		
		TextureRef texture;
		
        int getSizeIndex(float height) const;
		
        Vector2int16 layoutRatio(const std::string& s, std::vector<GlyphLine>* lines, int sizeIndex, const Vector2& availableSpace) const;
		Vector2int16 layout(const std::string& s, std::vector<GlyphLine>* lines, int sizeIndex, const Vector2int16& availableSpace, bool useAvailableSpace, bool onlyProperFit,  bool* textFits) const;
        
        Vector2 measureLining(std::vector<GlyphLine>& lines, int sizeIndex) const;

        Vector2 drawScaledImpl(
            Adorn*				adorn,
            const std::string&  s,
            const Vector2&      position,
            const Color4&       color,
            const Color4&       outline,
            RBX::Text::XAlign   xalign,
            RBX::Text::YAlign   yalign,
            const Vector2&		availableSpace,
            const Rect2D&		clippingRect,
            const Rotation2D&   rotation) const;

		Vector2 drawImpl(
			Adorn*				adorn,
			const std::string&  s,
			const Vector2&      position,
			float               size,
			const Color4&       color,
			const Color4&       outline,
			RBX::Text::XAlign   xalign,
			RBX::Text::YAlign   yalign,
			const Vector2&		availableSpace,
			const Rect2D&		clippingRect,
			const Rotation2D&   rotation) const;
	};
}
}
