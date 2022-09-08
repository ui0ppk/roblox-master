#include "stdafx.h"
#include "TypesetterDynamic.h"

#include "GfxBase/Adorn.h"
#include "StringConv.h"
#include "TextureManager.h"
#include "VisualEngine.h"
#include "TextureAtlas.h"

#include "GfxCore/Texture.h"
#include "GfxCore/Device.h"

#include "freetype/include/ft2build.h"
#include FT_FREETYPE_H
#include FT_MODULE_H
#include FT_TRUETYPE_DRIVER_H
#include FT_LCD_FILTER_H

LOGGROUP(Graphics)

FASTFLAG(FontSmoothScalling)
DYNAMIC_FASTFLAG(TextScaleDontWrapInWords)
FASTINTVARIABLE(FontSizePadding, 1)

#define FONT_PADDING 1.0f

	namespace RBX
{
	namespace Graphics
	{

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1
        static const uint8_t utf8d[] = {
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
            0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
            7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
            8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
            0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
            0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff
            0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
            1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
            1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
            1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
            1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
        };

        uint32_t inline utf8DecodeInternal(uint32_t* state, uint32_t* codep, uint32_t byte) {
            uint32_t type = utf8d[byte];

            *codep = (*state != UTF8_ACCEPT) ?
                (byte & 0x3fu) | (*codep << 6) :
            (0xff >> type) & (byte);

            *state = utf8d[256 + *state*16 + type];
            return *state;
        }

        static int round_int( double r ) 
        {
            return (r > 0.0) ? (r + 0.5) : (r - 0.5); 
        }

		class GlyphProvider
		{
		public:
            struct SizeSpecificData
            {
                unsigned ascender;
                unsigned descender;
                unsigned height;
            };

			GlyphProvider(const std::string& fontPath, TextureAtlas* textureAtlas, unsigned fontId)
				: textureAtlas(textureAtlas)
                , fontId(fontId)
                , library(NULL)
                , face(NULL)
			{
                memset(&errorGlyph, 0, sizeof(errorGlyph));
                memset(&errorSizeData, 0, sizeof(errorSizeData));

				FT_Error error = FT_Init_FreeType( &library );
				RBXASSERT(error == FT_Err_Ok);
                if (!library)
                {
                    FASTLOG(FLog::Graphics, "ERROR: Cannot init free type library");
                    return;
                }

                std::ifstream file(utf8_decode(fontPath).c_str(), std::ios::in | std::ios::binary);
                if (!file)
                {
                    FASTLOG(FLog::Graphics, "ERROR: Cannot load font file");
                    return;
                }

                std::ostringstream data;
                data << file.rdbuf();
                dataBuffer = data.str();

				error = FT_New_Memory_Face(library, (const unsigned char*)dataBuffer.c_str(), dataBuffer.size(), 0, &face);
                RBXASSERT(error == FT_Err_Ok);
                if (!face)
                {
                    FASTLOG(FLog::Graphics, "ERROR: Cannot create font face from memory");
                    return;
                }

                FT_UInt     interpreter_version = TT_INTERPRETER_VERSION_38;
                FT_Property_Set(library, "truetype", "interpreter-version", &interpreter_version );
            }

            const shared_ptr<Texture>& getTexture() const { return textureAtlas ? textureAtlas->getTexture() : errorTex; }

            unsigned roundToPixelsSigned(int size)
            {
                return (size > 0 ? (size + 32) : (size - 32)) / 64;
            }

            SizeSpecificData& getSizeData(unsigned size) 
            {
                if (!face || !library)
                    return errorSizeData;

                boost::unordered_map<unsigned, SizeSpecificData>::iterator it = sizeData.find(size);
                if (it != sizeData.end())
                {
                    return it->second;
                }
                else
                {
                    float sizeMult = (size + FInt::FontSizePadding) / (float)face->height;

                    // set size data
                    sizeData[size] = SizeSpecificData();
                    sizeData[size].ascender = round_int(face->ascender * sizeMult);
                    sizeData[size].descender = round_int(face->descender * sizeMult);
                    sizeData[size].height = round_int(face->height * sizeMult);

                    return sizeData[size];
                }
            }

            int getKerning(unsigned previous, unsigned current, unsigned size)
            {
                if (!face || !library)
                    return 0;

                GlyphKey key = GlyphKey(current, previous);
                float sizeMult = (size + FInt::FontSizePadding) / (float)face->height;

                KerningMap::iterator it = kerningMap.find(key);
                if (it != kerningMap.end())
                    return round_int(it->second * sizeMult);
                
                unsigned previousChId = FT_Get_Char_Index(face, previous);
                unsigned currentChId = FT_Get_Char_Index(face, current);

                FT_Vector delta;
                FT_Get_Kerning(face, previousChId, currentChId, FT_KERNING_UNSCALED, &delta);

                kerningMap[key] = delta.x;
                return round_int(delta.x * sizeMult);
            }

            boost::uint64_t getAtlasId(unsigned character, unsigned size)
            {
                boost::uint64_t atlasId = fontId * (boost::uint64_t)0xffffffff;
                atlasId += size * 0x10FFFF; // max UTF8 value
                atlasId += character;
                return atlasId;
            }

            void releaseAtlas()
            {
                textureAtlas = NULL;
            }

            void setAtlas(TextureAtlas* atlas)
            {
                textureAtlas = atlas;
            }

            bool setGlyphToSlot(unsigned character, unsigned size)
            {
                FT_Size_RequestRec_ request = {FT_SIZE_REQUEST_TYPE_REAL_DIM, 0,  (size + FInt::FontSizePadding) * 64, 0, 0};
                FT_Error error = FT_Request_Size(face, &request);
                RBXASSERT(error == FT_Err_Ok);
                if (error)
                    return false;        

                unsigned glyph_index = FT_Get_Char_Index(face, character);

                error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT);
                RBXASSERT(error == FT_Err_Ok);
                if (error)
                    return false;

                return true;
            }

            const TypesetterDynamic::Glyph& getGlyphMetrics(unsigned character, unsigned size) 
            {
                if (!face || !library)
                    return errorGlyph;

                GlyphKey key(character, size);
                GlyphMap::iterator it = glyphMap.find(key);
                if (it != glyphMap.end())
                    return it->second;

                if (!setGlyphToSlot(character, size))
                    return errorGlyph;

                const FT_GlyphSlot& slot = face->glyph;
                TypesetterDynamic::Glyph& glyph = glyphMap[key] = TypesetterDynamic::Glyph();

                glyph.height = roundToPixelsSigned(slot->metrics.height);
                glyph.width = roundToPixelsSigned(slot->metrics.width);
                glyph.xadvance = roundToPixelsSigned(slot->advance.x);
                glyph.xoffset = roundToPixelsSigned(slot->metrics.horiBearingX); 
                glyph.yoffset = roundToPixelsSigned(slot->metrics.horiBearingY);

                return glyph;
            }

            const TextureRegion& getGlyphUVs(unsigned character, unsigned size)
            {
                if (!face || !library || !textureAtlas)
                    return errorTexRegion;

                const TypesetterDynamic::Glyph& glyph = getGlyphMetrics(character, size);

                // try get UV from tex atlas
                boost::uint64_t atlasId = getAtlasId(character, size);
                const TextureRegion* texRegion;
                if (textureAtlas->getRegion(atlasId, texRegion))
                    return *texRegion;

                // early out heuristic
                Vector2int16 lastFailedAtlasSize = textureAtlas->getLastFailedSize();
                if (lastFailedAtlasSize.y <= (int)size)
                    return errorTexRegion;

                // rasterize, insert to atlas and profit
                if (lastFailedAtlasSize.x > glyph.width && lastFailedAtlasSize.y > glyph.height)
                {
                    if (!setGlyphToSlot(character, size))
                        return errorTexRegion;

                    if (face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
                    {
                        FT_Error error = FT_Render_Glyph( face->glyph, FT_RENDER_MODE_NORMAL );
                        RBXASSERT(error == FT_Err_Ok);
                        if (error)
                            return errorTexRegion;
                    }

                    FT_Bitmap& bitmap = face->glyph->bitmap;
                    if (textureAtlas->insert(atlasId, Vector2int16(bitmap.width, bitmap.rows), bitmap.pitch, bitmap.buffer, texRegion))
                        return *texRegion;
                }
                return errorTexRegion;
            }

		private:

			FT_Library library;
			FT_Face face;  

			typedef std::pair<unsigned, unsigned> GlyphKey;
			typedef boost::unordered_map<GlyphKey, TypesetterDynamic::Glyph> GlyphMap;
            typedef boost::unordered_map<GlyphKey, int> KerningMap;

            GlyphMap glyphMap;
            KerningMap kerningMap;

			boost::unordered_map<unsigned, SizeSpecificData> sizeData;

            TypesetterDynamic::Glyph errorGlyph;
            TextureRegion errorTexRegion;
            shared_ptr<Texture> errorTex;
			SizeSpecificData errorSizeData;

			unsigned fontId;
            std::string dataBuffer;

            TextureAtlas* textureAtlas;
		};

		TypesetterDynamic::TypesetterDynamic(TextureAtlas* textureAtlas, RBX::Graphics::TextureManager* textureManager, const std::string& fontPath, float legacyHeightScale, unsigned fontId, bool retina)
			: legacyHeightScale(legacyHeightScale)
			, retinaScale(retina ? 2 : 1)
        {
            glyphProvider = new GlyphProvider(fontPath, textureAtlas, fontId);
		}

        TypesetterDynamic::~TypesetterDynamic()
        {
            delete glyphProvider;
        }

		static void drawRect(Adorn* adorn, bool useClipping, const Rect2D& clippingRect, const Rotation2D& rotation, const Rect2D& glyphRect, const Vector2& uvtl, const Vector2& uvbr, const Color4& color)
		{
			if (!rotation.empty())
			{
				adorn->rect2d(glyphRect, uvtl, uvbr, color, rotation);
			}
			else if (useClipping)
			{
				Rect2D clippedRect = clippingRect.intersect(glyphRect);

				if (clippedRect == glyphRect)
				{
					adorn->rect2d(glyphRect, uvtl, uvbr, color, Rotation2D());
				}
				else if (clippedRect != RBX::Rect2D::xywh(0,0,0,0))
				{
					adorn->rect2d(glyphRect, uvtl, uvbr, color, clippingRect);
				}
			}
			else
			{
				adorn->rect2d(glyphRect, uvtl, uvbr, color, Rotation2D());
			}
		}

		static float fontAdjustX(float x, int width, float scale, Text::XAlign align)
		{
			switch (align)
			{
			case Text::XALIGN_CENTER:
				return x - (width / 2) * scale;
			case Text::XALIGN_RIGHT:
				return x - width * scale;
			default:
				return x;
			}
		}

		static float fontAdjustY(float y, int height, float scale, Text::YAlign align)
		{
			switch (align)
			{
			case Text::YALIGN_CENTER:
				return y - (height / 2) * scale;
			case Text::YALIGN_BOTTOM:
				return y - height * scale;
			default:
				return y;
			}
		}

		static inline bool isCharacterWhitespace(unsigned ch)
		{
			return ch == ' ' || ch == '\t';
		}

		static inline bool isCharacterZeroWidth(char ch)
		{
			return ch == '\1';
		}

		static const RBX::Vector2 kBorderOffsets[] =
		{
			RBX::Vector2(-1, -1),
			RBX::Vector2(+1, -1),
			RBX::Vector2(-1, +1),
			RBX::Vector2(+1, +1),
			RBX::Vector2(0, 0),
		};

        void TypesetterDynamic::loadResources(RBX::Graphics::TextureManager* textureManager, RBX::Graphics::TextureAtlas* glyphAtlas)
        {
            glyphProvider->setAtlas(glyphAtlas);
        }

        void TypesetterDynamic::releaseResources()
        {
            glyphProvider->releaseAtlas();
        }

		const shared_ptr<Texture>& TypesetterDynamic::getTexture() const
		{
			return glyphProvider->getTexture();
		}

        void TypesetterDynamic::drawCursor(Adorn* adorn, bool useClipping, const Rect2D& clippingRect, const Rotation2D& rotation, const Rect2D& glyphRect, const Color4& color) const
        {
            adorn->setIgnoreTexture(true);
            drawRect(adorn, useClipping, clippingRect, rotation, glyphRect, Vector2::one(), Vector2::one(), color);
            adorn->setIgnoreTexture(false);
        }

        void TypesetterDynamic::render(Adorn* adorn, 
            const Color4& color, const Color4& outline, float alpha, RBX::Text::XAlign xalign, std::vector<GlyphLine>& lines, float scale, 
            unsigned measureSize, unsigned renderSize, unsigned ascender,
            float startx, float starty, float height, 
            const Rect2D& clippingRect, const Rotation2D& rotation) const
        {
            shared_ptr<Texture> texture = glyphProvider->getTexture();
            if (!texture)
                return;
            bool useClipping = (clippingRect != RBX::Rect2D::xyxy(-1,-1,-1,-1));
            Vector2 uvOffset = Vector2(FONT_PADDING / texture->getWidth(), FONT_PADDING / texture->getHeight());

            unsigned previousChar = 0;
            for (int offsetIndex = (outline.a > 0.05f ? 0 : 4); offsetIndex < 5; ++offsetIndex)
            {
                float dx = kBorderOffsets[offsetIndex].x, dy = kBorderOffsets[offsetIndex].y;
                Color4 drawColor = (offsetIndex == 4) ? color : outline;
                drawColor.a *= alpha;

                for (size_t li = 0; li < lines.size(); ++li)
                {
                    const GlyphLine& line = lines[li];

                    float linex = fontAdjustX(startx, line.width, scale, xalign);
                    float liney = starty + line.yoffset * scale;

                    float x = 0;

                    for (size_t i = 0; i < line.length; ++i)
                    {
                        unsigned unicodeChar = line.data[i];

                        // Cursor character
                        if (unicodeChar == '\1')
                        {
                            drawCursor(adorn, useClipping, clippingRect, rotation,
                                Rect2D::xywh(linex + dx + x , liney + dy, 1, height),
                                drawColor);

                            continue;
                        }

                        Glyph glyph = glyphProvider->getGlyphMetrics(unicodeChar, measureSize);
                        x += glyphProvider->getKerning(previousChar, unicodeChar, measureSize);

                        if (!isCharacterWhitespace(unicodeChar))
                        {
                            Rect2D rect = Rect2D::xywh(linex + dx + (x + glyph.xoffset) * scale, liney + dy + (ascender - glyph.yoffset) * scale, glyph.width * scale, glyph.height * scale);

                            rect = rect.expand(FONT_PADDING);

                            TextureRegion textureRegion = glyphProvider->getGlyphUVs(unicodeChar, renderSize);
                            Rect2D uvs = Rect2D::xyxy(textureRegion.x / (float)texture->getWidth(), textureRegion.y / (float)texture->getHeight(),
                                (textureRegion.x + textureRegion.width) / (float)texture->getWidth(), (textureRegion.y + textureRegion.height) / (float)texture->getHeight());

                            drawRect(adorn, useClipping, clippingRect, rotation,
                                rect,
                                Vector2(uvs.x0() - uvOffset.x, uvs.y0() - uvOffset.y),
                                Vector2(uvs.x1() + uvOffset.x, uvs.y1() + uvOffset.y),
                                drawColor);
                        }

                        x += glyph.xadvance;

                        previousChar = unicodeChar;
                    }
                }
            }
        }

        Vector2 TypesetterDynamic::drawImpl(Adorn* adorn, const std::vector<unsigned>& stringUnicode,
            const Vector2& position, float size, const Color4& color, const Color4& outline,
            RBX::Text::XAlign xalign, RBX::Text::YAlign yalign, const Vector2& availableSpace,
            const Rect2D& clippingRect, const Rotation2D& rotation) const
        {
            bool useAvailableSpace = availableSpace.x != 0;

            float height = size * legacyHeightScale * retinaScale;
            float scale = 1.0f / retinaScale;

            std::vector<GlyphLine> lines;

            Vector2int16 intAvailableSpace = Vector2int16(availableSpace / scale);

            Vector2int16 intSize = layout(stringUnicode, &lines, height, intAvailableSpace, useAvailableSpace, false, NULL);
            unsigned ascender = glyphProvider->getSizeData(height).ascender;

            float startx, starty;
            if (adorn->useFontSmoothScalling())
            {
                startx = (position.x);
                starty = fontAdjustY((position.y), intSize.y, scale, yalign);
            }
            else
            {
                startx = round_int(position.x) ;
                starty = fontAdjustY(round_int(position.y), intSize.y, scale, yalign);
            }

            shared_ptr<Texture> texture = glyphProvider->getTexture();
            if (!texture)
                return Vector2::zero();

            render(adorn, color, outline, 1, xalign, lines, scale, height, height, ascender, startx, starty, height, clippingRect, rotation);

            return Vector2(intSize) * scale;
        }

		Vector2 TypesetterDynamic::drawScaledImpl(Adorn* adorn, const std::vector<unsigned>& stringUnicode, const Vector2& position,
			const Color4& color, const Color4& outline, RBX::Text::XAlign xalign, RBX::Text::YAlign yalign,
			const Vector2& availableSpace, const Rect2D& clippingRect, const Rotation2D& rotation) const
		{

			float height = 48;        // 48 is our highest font scale right now

			std::vector<GlyphLine> lines;
			Vector2int16 intSize = layoutRatio(stringUnicode, &lines, height, availableSpace);

			// ratio is the smaller one from XY ratios to make text fit
			float floatIntSizeRatio = std::min((float)availableSpace.x / (float)intSize.x, (float)availableSpace.y / (float)intSize.y);
			floatIntSizeRatio = std::min(1.0f, floatIntSizeRatio);

			intSize.x = float(intSize.x) * floatIntSizeRatio;
			intSize.y = float(intSize.y) * floatIntSizeRatio;

			// recompute based on result
			float referenceSize = height;
            unsigned ascenderReference = glyphProvider->getSizeData(referenceSize).ascender;

            height = height * floatIntSizeRatio;

            // compute size of glyph that we will use as texture.
			float renderedGlyphSize = std::ceil(height / 6) * 6;

			Vector2 measuredSize = measureLining(lines, referenceSize);
			Vector2 realSize = measuredSize * floatIntSizeRatio;

			if (!adorn)
				return realSize;

            // hide when glyphs get too small
			float alpha = alpha = -2 + height * 0.5f;
			alpha = G3D::clamp(alpha, 0, 1);

			if (alpha == 0)
				return realSize;

			float startx = position.x;
			float starty = fontAdjustY(position.y, measuredSize.y, floatIntSizeRatio, yalign);

            shared_ptr<Texture> texture = glyphProvider->getTexture();
            if (!texture)
                return Vector2::zero();

            render(adorn, color, outline, alpha, xalign, lines, floatIntSizeRatio, referenceSize, renderedGlyphSize, ascenderReference, startx, starty, height, clippingRect, rotation);

			return realSize;
		}

		Vector2 TypesetterDynamic::draw(Adorn* adorn, const std::string& s,
			const Vector2& position, float size, bool autoScale, const Color4& color, const Color4& outline,
			RBX::Text::XAlign xalign, RBX::Text::YAlign yalign, const Vector2& availableSpace,
			const Rect2D& clippingRect, const Rotation2D& rotation) const
		{
			bool useAvailableSpace = availableSpace.x != 0;

            std::vector<unsigned> stringUnicode;
            decodeUTF8(s, &stringUnicode);

            if (autoScale && useAvailableSpace)
                return drawScaledImpl(adorn, stringUnicode, position, color, outline, xalign, yalign, availableSpace, clippingRect, rotation);

			return drawImpl(adorn, stringUnicode, position, size, color, outline, xalign, yalign, availableSpace, clippingRect, rotation);
		}

		int TypesetterDynamic::getCursorPositionInText(const std::string& s, const RBX::Vector2& position, float size,
			RBX::Text::XAlign xalign, RBX::Text::YAlign yalign, const RBX::Vector2& availableSpace, const Rotation2D& rotation,
			RBX::Vector2 cursorPos) const
		{
            std::vector<unsigned> stringUnicode;
            decodeUTF8(s, &stringUnicode);

			bool useAvailableSpace = availableSpace.x != 0;

			float height = size * legacyHeightScale * retinaScale;
			float scale = 1.0f / retinaScale;

			std::vector<GlyphLine> lines;

			Vector2int16 intAvailableSpace = Vector2int16(availableSpace / scale);
			Vector2int16 intSize = layout(stringUnicode, &lines, height, intAvailableSpace, useAvailableSpace, false, NULL);

			float startx = round_int(position.x);
			float starty = fontAdjustY(round_int(position.y), intSize.y, scale, yalign);

			Vector2 localCursor = rotation.inverse().rotate(cursorPos);

            unsigned previousChar = 0;
			for (size_t li = 0; li < lines.size(); ++li)
			{
				const GlyphLine& line = lines[li];

				float linex = fontAdjustX(startx, line.width, scale, xalign);
				float liney = starty + line.yoffset * scale;

				if (liney + height > localCursor.y || li + 1 == lines.size())
				{
					int lineOffset = line.data - stringUnicode.data();

					int x = 0;

					for (size_t i = 0; i < line.length; ++i)
					{
						unsigned unicodeChar = line.data[i];

                        if (isCharacterZeroWidth(unicodeChar))
                            continue;

						Glyph glyph = glyphProvider->getGlyphMetrics(unicodeChar, size);
                        x += glyphProvider->getKerning(previousChar, unicodeChar, height);
						x += glyph.xadvance;

						if (linex + x * scale > localCursor.x)
						{
							// Select current glyph if we're within 2/3 of the advance
							if (linex + x * scale - glyph.xadvance * scale * 0.33f > localCursor.x)
								return lineOffset + i;
							else
								return lineOffset + i + 1;
						}
                        previousChar = unicodeChar;
					}

					return lineOffset + line.length;
				}
			}

			return -1;
		}

		Vector2 TypesetterDynamic::measure(const std::string& s, float size, const Vector2& availableSpace, bool* textFits) const
		{
            std::vector<unsigned> stringUnicode;
            decodeUTF8(s, &stringUnicode);

			bool useAvailableSpace = availableSpace.x != 0;

			float height = size * legacyHeightScale * retinaScale;
			float scale = 1.0 / retinaScale;

			Vector2int16 intAvailableSpace = Vector2int16(availableSpace / scale);
			Vector2int16 intSize = layout(stringUnicode, NULL, height, intAvailableSpace, useAvailableSpace, DFFlag::TextScaleDontWrapInWords, textFits);

			if (DFFlag::TextScaleDontWrapInWords && textFits == false) {
				intSize = layout(stringUnicode, NULL, height, intAvailableSpace, useAvailableSpace, false, textFits);
			}

			return Vector2(intSize) * scale;
		}

		template <typename GlyphLine> static inline void pushLine(std::vector<GlyphLine>* lines, const std::vector<unsigned>& stringUnicode, int lineStart, int lineEnd, int yoffset, int width)
		{
			if (lines)
			{
				GlyphLine line = {stringUnicode.data() + lineStart, lineEnd - lineStart, yoffset, width};

				lines->push_back(line);
			}
		}

		Vector2int16 TypesetterDynamic::layoutRatio(const std::vector<unsigned>& stringUnicode, std::vector<GlyphLine>* lines, int size, const Vector2& availableSpace) const
		{
			float ratio = availableSpace.x / availableSpace.y;

			int height = size;

			float min = 10;
			float max = height * stringUnicode.size(); // this is just to have a reasonable max value to start with, height is bigger then width when talking about fonts
			static const float ratioBiasScale = 1.01f;              
			max = std::max(max, height * ratio * ratioBiasScale);  // make sure that text will fit vertically (its is at least as big as height of font). Multiply by small paddig to be really sure

			float actualRatio = -1; // real one will never be bellow zero
			static const float tolerance = 0.05f;

			bool textFits = false;
			float x = max;
			Vector2int16 availableSpaceByRatio;

			float bestRatioSoFar = FLT_MAX;
			std::vector<GlyphLine> bestLinesSoFar = *lines;
			Vector2int16 bestSizeSoFar;

			unsigned cnt = 0;
			static const unsigned kMaxIterations = 10;

			// binary search for nice fit, but do not search to long. Max kMaxIterations. In worse case we will have some solution
			// good thing is it will be same for same string and ratio
			while( cnt < kMaxIterations ) 
			{
				float y = x / ratio;
				actualRatio = x / y;

				availableSpaceByRatio = Vector2int16(x, y);

				if (lines)
					lines->clear();

				Vector2int16 intSize = layout(stringUnicode, lines, height, availableSpaceByRatio, true, true, &textFits);
				actualRatio = (float)intSize.x / (float)intSize.y;

				if (textFits)
				{
					if (std::abs(actualRatio - ratio) < std::abs(bestRatioSoFar - ratio))
					{
						bestLinesSoFar = *lines;
						bestRatioSoFar = actualRatio;
						bestSizeSoFar = intSize;
					}

					max = x;
					x = min + (x - min) / 2;

					if (actualRatio >= ratio - tolerance && actualRatio <= ratio + tolerance)
						break;
				}
				else
				{
					min = x;
					x = x + (max - x) / 2;
				}

				if (max - x <= 0 || x - min <= 0) // we can make it bigger or smaller, so lets just stop searching and go with the result we have
					break;

				++cnt;
			}

			*lines = bestLinesSoFar;
			return bestSizeSoFar;
		}

		Vector2 TypesetterDynamic::measureLining(std::vector<GlyphLine>& lines, unsigned size) const
		{
			Vector2 ret;
			int height = size;

			float y = 0;
			for (size_t li = 0; li < lines.size(); ++li)
			{
				const GlyphLine& line = lines[li];

				float x = 0;
				y += height;

                unsigned previousChar = 0;
				for (size_t i = 0; i < line.length; ++i)
				{
                    unsigned unicodeChar = line.data[i];

                    Glyph glyph = glyphProvider->getGlyphMetrics(unicodeChar, size);
					x += glyphProvider->getKerning(previousChar, unicodeChar, size);
					x += glyph.xadvance;
                 
                    previousChar = unicodeChar;
				}

				if (ret.x < x)
					ret.x = x;
				if (ret.y < y)
					ret.y = y;
			}
			return ret;
		}

        void TypesetterDynamic::decodeUTF8(const std::string& string, std::vector<unsigned>* out) const
        {
            out->clear();
            out->reserve(string.size());

            uint32_t codepoint;
            uint32_t state = 0;

            for (unsigned i = 0; i < string.size(); ++i)
                if (utf8DecodeInternal(&state, &codepoint, (unsigned char)string.c_str()[i]) == UTF8_ACCEPT)
                    out->push_back(codepoint);
        }

		Vector2int16 TypesetterDynamic::layout(const std::vector<unsigned>& stringUnicode, std::vector<GlyphLine>* lines, int size, const Vector2int16& availableSpace, bool useAvailableSpace, bool onlyProperFit, bool* textFits) const
		{
			int x = 0, y = 0;

			int height = size;

			int lastKerningOffset = 0;

			int lineStart = 0;
			int lineMaxWidth = 0;

			int wordStart = 0;
			int wordWidth = 0;
			int whitespaceWidth = 0;

            unsigned previousChar = 0;

            for (size_t i = 0; i < stringUnicode.size(); ++i)
            {
                unsigned currentUnicode = stringUnicode[i];

                if (isCharacterZeroWidth(currentUnicode))
                {
                	lastKerningOffset = 0;
                    continue;
                }

                if (currentUnicode == '\n')
                {
                	// hard line break
                	pushLine(lines, stringUnicode, lineStart, i, y, x);

                	lineStart = i + 1;
                	lineMaxWidth = std::max(lineMaxWidth, x);

                	wordStart = i + 1;
                	wordWidth = 0;
                	whitespaceWidth = 0;

                	x = 0;
                	y += height;

                    continue;
                }

                Glyph glyph = glyphProvider->getGlyphMetrics(currentUnicode, size);
                int xoffset = glyphProvider->getKerning(previousChar, currentUnicode, size) + glyph.xadvance;

                if (isCharacterWhitespace(currentUnicode))
                {
                    if (wordWidth != 0)
                        whitespaceWidth = 0;

                    wordStart = i;
                    wordWidth = 0;
                    whitespaceWidth += xoffset;
                }
                else
                {
                    if (wordWidth == 0)
                        wordStart = i;

                    wordWidth += xoffset;
                }

                x += xoffset;

                if (useAvailableSpace && x > availableSpace.x)
                {
                    // soft line break
                    if (wordWidth <= availableSpace.x)
                    {
                        // word fits on next line; move it there, discard whitespace
                        pushLine(lines, stringUnicode, lineStart, wordStart, y, x - wordWidth - whitespaceWidth);

                        lineStart = wordStart;
                        lineMaxWidth = std::max(lineMaxWidth, x - wordWidth - whitespaceWidth);

                        x = wordWidth;
                    }
                    else
                    {
                        if (onlyProperFit)
                        {
                            // early out
                            if (textFits)
                                *textFits = false;

                            return Vector2int16(lineMaxWidth, y);
                        }

                        // word does not fit, cut
                        pushLine(lines, stringUnicode, lineStart, i, y, x - xoffset);

                        lineStart = i;
                        lineMaxWidth = std::max(lineMaxWidth, x - xoffset);

                        x = xoffset;
                    }

                    wordWidth = x;
                    whitespaceWidth = 0;
                    y += height;
                }

                previousChar = currentUnicode;
            }

			// last line
			pushLine(lines, stringUnicode, lineStart, stringUnicode.size(), y, x);

			lineMaxWidth = std::max(lineMaxWidth, x);

			y += height;

			// clamp line count
			if (useAvailableSpace && y > availableSpace.y)
			{
				int clampedLineCount = std::max(1, availableSpace.y / height);

				if (lines)
					lines->resize(clampedLineCount);

				if (textFits)
					*textFits = false;

				return Vector2int16(lineMaxWidth, clampedLineCount * height);
			}
			else
			{
				if (textFits)
					*textFits = true;

				return Vector2int16(lineMaxWidth, y);
			}
		}
	}
}