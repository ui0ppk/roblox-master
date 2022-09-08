#include "stdafx.h"
#include "TypesetterBitmap.h"

#include "GfxBase/Adorn.h"
#include "StringConv.h"
#include "TextureManager.h"
#include "GfxCore/Texture.h"

FASTFLAG(FontSmoothScalling)
DYNAMIC_FASTFLAGVARIABLE(TextScaleDontWrapInWords, false)

#define FONT_PADDING 1.0f

namespace RBX
{
namespace Graphics
{
	static void readData(std::istream& in, void* data, int size)
	{
		in.read(static_cast<char*>(data), size);

		if (in.gcount() != size)
			throw RBX::runtime_error("Unexpected end of file while reading %d bytes", size);
	}

	TypesetterBitmap::TypesetterBitmap(TextureManager* textureManager, const std::string& fontPath, const std::string& texturePath, float legacyHeightScale, bool retina)
		: legacyHeightScale(legacyHeightScale)
        , retinaScale(retina ? 2 : 1)
	{
		std::ifstream in(utf8_decode(fontPath).c_str(), std::ios::in | std::ios::binary);
		
		char header[4];
		readData(in, header, 4);
		
		int fileSize = 0;
		readData(in, &fileSize, sizeof(fileSize));
		
		if (memcmp(header, "RBXF", 4) != 0 || fileSize < 16)
			throw RBX::runtime_error("Corrupt file header");
			
		fontData.reset(new char[fileSize]);
		
		readData(in, fontData.get(), fileSize);
		
		// Setup pointers to various tables
		info = *reinterpret_cast<Info*>(fontData.get());
		
		glyphRemap = reinterpret_cast<const unsigned char*>(fontData.get() + sizeof(Info));
		sizeTable = reinterpret_cast<const float*>(reinterpret_cast<const char*>(glyphRemap) + 256);
		glyphTable = reinterpret_cast<const Glyph*>(reinterpret_cast<const char*>(sizeTable) + info.sizeCount * sizeof(float));
		kerningTable = reinterpret_cast<const char*>(glyphTable) + info.sizeCount * info.glyphCount * sizeof(Glyph);
		
		// Load font texture
		texture = textureManager->load(ContentId(texturePath), TextureManager::Fallback_BlackTransparent);
		fontTexturePath = texturePath;
	}
	
	void TypesetterBitmap::loadResources(RBX::Graphics::TextureManager* textureManager, RBX::Graphics::TextureAtlas* glyphAtlas)
	{
		if (!fontTexturePath.empty() && !texture.getTexture().get())
		{
			texture = textureManager->load(ContentId(fontTexturePath), TextureManager::Fallback_BlackTransparent);
		}
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
	
	static void drawCursor(Adorn* adorn, bool useClipping, const Rect2D& clippingRect, const Rotation2D& rotation, const Rect2D& glyphRect, const Color4& color)
	{
		adorn->setIgnoreTexture(true);
		
		drawRect(adorn, useClipping, clippingRect, rotation, glyphRect, Vector2::one(), Vector2::one(), color);
		
		adorn->setIgnoreTexture(false);
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
	
	static inline bool isCharacterWhitespace(char ch)
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

	Vector2 TypesetterBitmap::drawImpl(Adorn* adorn, const std::string& s,
	const Vector2& position, float size, const Color4& color, const Color4& outline,
	RBX::Text::XAlign xalign, RBX::Text::YAlign yalign, const Vector2& availableSpace,
	const Rect2D& clippingRect, const Rotation2D& rotation) const
	{
		bool useClipping = (clippingRect != RBX::Rect2D::xyxy(-1,-1,-1,-1));
		bool useAvailableSpace = availableSpace.x != 0;
		
		float height = size * legacyHeightScale * retinaScale;
		int sizeIndex = getSizeIndex(height);
		float scale = height / sizeTable[sizeIndex] / retinaScale;
		
		std::vector<GlyphLine> lines;
		
		Vector2int16 intAvailableSpace = Vector2int16(availableSpace / scale);
		Vector2int16 intSize = layout(s, &lines, sizeIndex, intAvailableSpace, useAvailableSpace, false, NULL);
		
		const Glyph* glyphTableSize = glyphTable + info.glyphCount * sizeIndex;
		
        float startx, starty;
        if (adorn->useFontSmoothScalling())
        {
            startx = (position.x);
            starty = fontAdjustY((position.y), intSize.y, scale, yalign);
        }
        else
        {
		    startx = floorf(position.x + 0.5f);
		    starty = fontAdjustY(floorf(position.y + 0.5f), intSize.y, scale, yalign);
        }

        Vector2 uvOffset(FONT_PADDING / texture.getTexture().get()->getWidth(), FONT_PADDING / texture.getTexture().get()->getHeight());

		for (int offsetIndex = (outline.a > 0.05f ? 0 : 4); offsetIndex < 5; ++offsetIndex)
		{
			float dx = kBorderOffsets[offsetIndex].x, dy = kBorderOffsets[offsetIndex].y;
			const Color4& drawColor = (offsetIndex == 4) ? color : outline;
			
			for (size_t li = 0; li < lines.size(); ++li)
			{
				const GlyphLine& line = lines[li];
			
				float linex = fontAdjustX(startx, line.width, scale, xalign);
				float liney = starty + line.yoffset * scale;
			
				int lastKerningOffset = 0;
				int x = 0;

				for (size_t i = 0; i < line.length; ++i)
				{
					char ch = line.data[i];
				
					int glyphIndex = glyphRemap[static_cast<unsigned char>(ch)];
				
					if (glyphIndex == 255)
					{
						if (!isCharacterZeroWidth(ch))
							lastKerningOffset = 0;
						
						// Cursor character
						if (ch == '\1')
						{
							drawCursor(adorn, useClipping, clippingRect, rotation,
								Rect2D::xywh(linex + dx + x * scale, liney + dy, 1, height),
								drawColor);
						}
						
						continue;
					}
					
					const Glyph& glyph = glyphTableSize[glyphIndex];
					
					x += kerningTable[lastKerningOffset + glyphIndex];

					if (!isCharacterWhitespace(ch))
					{
                        Rect2D rect = Rect2D::xywh(linex + dx + (x + glyph.xoffset) * scale, liney + dy + glyph.yoffset * scale, glyph.width * scale, glyph.height * scale);
                        rect = rect.expand(FONT_PADDING);

						drawRect(adorn, useClipping, clippingRect, rotation,
							rect,
                            Vector2(glyph.u0 - uvOffset.x, glyph.v0 - uvOffset.y),
                            Vector2(glyph.u1 + uvOffset.x, glyph.v1 + uvOffset.y),
							drawColor);
					}
						
					x += glyph.xadvance;
					
					lastKerningOffset = glyph.kerningOffset;
				}
			}
		}
		
		return Vector2(intSize) * scale;
	}

    Vector2 TypesetterBitmap::drawScaledImpl(Adorn* adorn, const std::string& s, const Vector2& position,
        const Color4& color, const Color4& outline, RBX::Text::XAlign xalign, RBX::Text::YAlign yalign,
        const Vector2& availableSpace, const Rect2D& clippingRect, const Rotation2D& rotation) const
    {
        bool useClipping = (clippingRect != RBX::Rect2D::xyxy(-1,-1,-1,-1));

        float height = 48 * legacyHeightScale * retinaScale;        // 48 is our highest font scale right now
        int sizeIndex = getSizeIndex(height);

        std::vector<GlyphLine> lines;

        Vector2int16 intSize = layoutRatio(s, &lines, sizeIndex, availableSpace);

        // ratio is the smaller one from XY ratios to make text fit
        float floatIntSizeRatio = std::min((float)availableSpace.x / (float)intSize.x, (float)availableSpace.y / (float)intSize.y);
        floatIntSizeRatio = std::min(1.0f, floatIntSizeRatio);

        intSize.x = float(intSize.x) * floatIntSizeRatio;
        intSize.y = float(intSize.y) * floatIntSizeRatio;

        // recompute based on result
        height = height * floatIntSizeRatio;
        sizeIndex = getSizeIndex(height);

        // we will be interpolating between current size and step smaller size
        int stepSmallerSizeIndex = std::max(0, sizeIndex - 1);

        float bigScale = sizeTable[sizeIndex];
        float smallScale = sizeTable[stepSmallerSizeIndex];

        float sizeLerpVal = 1;
        if (bigScale != smallScale)
        {
            sizeLerpVal = (height - smallScale) / (bigScale - smallScale);
            sizeLerpVal = 1 - sizeLerpVal;
        }

        // this step is to assure that nothing overflows outside bounds
        Vector2 realSize1 = measureLining(lines, sizeIndex);
        Vector2 realSize2 = measureLining(lines, stepSmallerSizeIndex);
        Vector2 totallyRealSize = lerp(realSize1, realSize2, sizeLerpVal);

        if (!adorn)
            return totallyRealSize;

        float alpha = 1.0f;
        float scale = 1.0f;
        if (totallyRealSize.x > availableSpace.x )
        {
            alpha = availableSpace.x / totallyRealSize.x;
            // this will make step faster
            alpha -= 0.75f;
            alpha *= 6;
            alpha = G3D::clamp(alpha, 0, 1);

            scale *= availableSpace.x / totallyRealSize.x;
        }

        if (alpha == 0)
            return totallyRealSize;

        const Glyph* glyphTableSize = glyphTable + info.glyphCount * sizeIndex;
        const Glyph* glyphTableSizeSmaller = glyphTable + info.glyphCount * stepSmallerSizeIndex;

        float startx = position.x;
        float starty = fontAdjustY(position.y, totallyRealSize.y, scale, yalign);

        Vector2 lineSizeRatios = Vector2(totallyRealSize.x / (float)intSize.x, totallyRealSize.y / (float)intSize.y);
        Vector2 uvOffset(FONT_PADDING / texture.getTexture().get()->getWidth(), FONT_PADDING / texture.getTexture().get()->getHeight());

        for (int offsetIndex = (outline.a > 0.05f ? 0 : 4); offsetIndex < 5; ++offsetIndex)
        {
            float dx = kBorderOffsets[offsetIndex].x, dy = kBorderOffsets[offsetIndex].y;
            Color4 drawColor = (offsetIndex == 4) ? color : outline;
            drawColor.a *= alpha;

            for (size_t li = 0; li < lines.size(); ++li)
            {
                const GlyphLine& line = lines[li];

                float linex = fontAdjustX(startx, line.width * lineSizeRatios.x, scale * floatIntSizeRatio, xalign);
                float liney = starty + line.yoffset * lineSizeRatios.y * scale * floatIntSizeRatio;

                int lastKerningOffset = 0;
                int lastKerningOffset2 = 0;
                float x = 0;
                float x2 = 0;

                for (size_t i = 0; i < line.length; ++i)
                {
                    char ch = line.data[i];

                    int glyphIndex = glyphRemap[static_cast<unsigned char>(ch)];

                    if (glyphIndex == 255)
                    {
                        if (!isCharacterZeroWidth(ch))
                            lastKerningOffset = 0;

                        // Cursor character
                        if (ch == '\1')
                        {
                            drawCursor(adorn, useClipping, clippingRect, rotation,
                                Rect2D::xywh(linex + dx + x * scale, liney + dy, 1, height),
                                drawColor);
                        }

                        continue;
                    }

                    const Glyph& glyph = glyphTableSize[glyphIndex];
                    x += kerningTable[lastKerningOffset + glyphIndex];

                    const Glyph& glyph2 = glyphTableSizeSmaller[glyphIndex];
                    x2 += kerningTable[lastKerningOffset2 + glyphIndex];

                    if (!isCharacterWhitespace(ch))
                    {
                        Rect2D rect1 = Rect2D::xywh(linex + dx + (x + glyph.xoffset) * scale, liney + dy + glyph.yoffset * scale, glyph.width * scale, glyph.height * scale);
                        Rect2D rect2 = Rect2D::xywh(linex + dx + (x2 + glyph2.xoffset) * scale, liney + dy + glyph2.yoffset * scale, glyph2.width * scale, glyph2.height * scale);

                        Rect2D lerpRect = Rect2D::xyxy(
                            G3D::lerp(rect1.x0(), rect2.x0(), sizeLerpVal),
                            G3D::lerp(rect1.y0(), rect2.y0(), sizeLerpVal),
                            G3D::lerp(rect1.x1(), rect2.x1(), sizeLerpVal),
                            G3D::lerp(rect1.y1(), rect2.y1(), sizeLerpVal)
                            );

                        lerpRect = lerpRect.expand(FONT_PADDING);

                        drawRect(adorn, useClipping, clippingRect, rotation,
                            lerpRect,
                            Vector2(glyph.u0 - uvOffset.x, glyph.v0 - uvOffset.y),
                            Vector2(glyph.u1 + uvOffset.x, glyph.v1 + uvOffset.y),
                            drawColor);
                    }

                    x += glyph.xadvance;
                    x2 += glyph2.xadvance;
                    lastKerningOffset = glyph.kerningOffset;
                    lastKerningOffset2 = glyph2.kerningOffset;
                }
            }
        }

        return Vector2(intSize) * scale;
    }

	Vector2 TypesetterBitmap::draw(Adorn* adorn, const std::string& s,
		const Vector2& position, float size, bool autoScale, const Color4& color, const Color4& outline,
		RBX::Text::XAlign xalign, RBX::Text::YAlign yalign, const Vector2& availableSpace,
		const Rect2D& clippingRect, const Rotation2D& rotation) const
	{
        bool useAvailableSpace = availableSpace.x != 0;

		if (autoScale && useAvailableSpace)
            return drawScaledImpl(adorn, s, position, color, outline, xalign, yalign, availableSpace, clippingRect, rotation);

        return drawImpl(adorn, s, position, size, color, outline, xalign, yalign, availableSpace, clippingRect, rotation);
	}

	int TypesetterBitmap::getCursorPositionInText(const std::string& s, const RBX::Vector2& position, float size,
		RBX::Text::XAlign xalign, RBX::Text::YAlign yalign, const RBX::Vector2& availableSpace, const Rotation2D& rotation,
		RBX::Vector2 cursorPos) const
	{
		bool useAvailableSpace = availableSpace.x != 0;
		
		float height = size * legacyHeightScale * retinaScale;
		int sizeIndex = getSizeIndex(height);
		float scale = height / sizeTable[sizeIndex] / retinaScale;
		
		std::vector<GlyphLine> lines;
		
		Vector2int16 intAvailableSpace = Vector2int16(availableSpace / scale);
		Vector2int16 intSize = layout(s, &lines, sizeIndex, intAvailableSpace, useAvailableSpace, false, NULL);
		
		const Glyph* glyphTableSize = glyphTable + info.glyphCount * sizeIndex;
		
		float startx = floorf(position.x + 0.5f);
		float starty = fontAdjustY(floorf(position.y + 0.5f), intSize.y, scale, yalign);

        Vector2 localCursor = rotation.inverse().rotate(cursorPos);
		
		for (size_t li = 0; li < lines.size(); ++li)
		{
			const GlyphLine& line = lines[li];
		
			float linex = fontAdjustX(startx, line.width, scale, xalign);
			float liney = starty + line.yoffset * scale;
			
			if (liney + height > localCursor.y || li + 1 == lines.size())
			{
				int lineOffset = line.data - s.c_str();
				
				int lastKerningOffset = 0;
				int x = 0;

				for (size_t i = 0; i < line.length; ++i)
				{
					char ch = line.data[i];
				
					int glyphIndex = glyphRemap[static_cast<unsigned char>(ch)];
				
					if (glyphIndex == 255)
					{
						if (!isCharacterZeroWidth(ch))
							lastKerningOffset = 0;
							
						continue;
					}
					
					const Glyph& glyph = glyphTableSize[glyphIndex];
					
					x += kerningTable[lastKerningOffset + glyphIndex];
					x += glyph.xadvance;
					
					if (linex + x * scale > localCursor.x)
					{
						// Select current glyph if we're within 2/3 of the advance
						if (linex + x * scale - glyph.xadvance * scale * 0.33f > localCursor.x)
							return lineOffset + i;
						else
							return lineOffset + i + 1;
					}
						
					lastKerningOffset = glyph.kerningOffset;
				}
				
				return lineOffset + line.length;
			}
		}
		
		return -1;
	}

	Vector2 TypesetterBitmap::measure(const std::string& s, float size, const Vector2& availableSpace, bool* textFits) const
	{
		bool useAvailableSpace = availableSpace.x != 0;

		float height = size * legacyHeightScale * retinaScale;
		int sizeIndex = getSizeIndex(height);
		float scale = height / sizeTable[sizeIndex] / retinaScale;

		Vector2int16 intAvailableSpace = Vector2int16(availableSpace / scale);
		Vector2int16 intSize = layout(s, NULL, sizeIndex, intAvailableSpace, useAvailableSpace, DFFlag::TextScaleDontWrapInWords, textFits);

		if (DFFlag::TextScaleDontWrapInWords && textFits == false) {
			intSize = layout(s, NULL, sizeIndex, intAvailableSpace, useAvailableSpace, false, textFits);
		}

		return Vector2(intSize) * scale;
	}

	int TypesetterBitmap::getSizeIndex(float height) const
	{
		for (int i = 0; i < info.sizeCount; ++i)
			if (height <= sizeTable[i])
				return i;
				
		return info.sizeCount - 1;
	}
	
	template <typename GlyphLine> static inline void pushLine(std::vector<GlyphLine>* lines, const std::string& s, int lineStart, int lineEnd, int yoffset, int width)
	{
		if (lines)
		{
			GlyphLine line = {s.c_str() + lineStart, lineEnd - lineStart, yoffset, width};
			
			lines->push_back(line);
		}
	}
	
    Vector2int16 TypesetterBitmap::layoutRatio(const std::string& s, std::vector<GlyphLine>* lines, int sizeIndex, const Vector2& availableSpace) const
    {
        float ratio = availableSpace.x / availableSpace.y;
    
        int height = sizeTable[sizeIndex];

        float min = 10;
        float max = height * s.length(); // this is just to have a reasonable max value to start with, height is bigger then width when talking about fonts
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

            Vector2int16 intSize = layout(s, lines, sizeIndex, availableSpaceByRatio, true, true, &textFits);
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

    Vector2 TypesetterBitmap::measureLining(std::vector<GlyphLine>& lines, int sizeIndex) const
    {
        Vector2 ret;
        const Glyph* glyphTableSize = glyphTable + info.glyphCount * sizeIndex;
        int height = sizeTable[sizeIndex];

        float y = 0;
		for (size_t li = 0; li < lines.size(); ++li)
		{
            const GlyphLine& line = lines[li];

            int lastKerningOffset = 0;
            float x = 0;
            y += height;

            for (size_t i = 0; i < line.length; ++i)
            {
                char ch = line.data[i];

                int glyphIndex = glyphRemap[static_cast<unsigned char>(ch)];

                if (glyphIndex == 255)
                {
                    if (!isCharacterZeroWidth(ch))
                        lastKerningOffset = 0;
                    continue;
                }

                const Glyph& glyph = glyphTableSize[glyphIndex];
                x += kerningTable[lastKerningOffset + glyphIndex];
                x += glyph.xadvance;
                lastKerningOffset = glyph.kerningOffset;
			}

            if (ret.x < x)
                ret.x = x;
            if (ret.y < y)
                ret.y = y;
		}
        return ret;
    }


	Vector2int16 TypesetterBitmap::layout(const std::string& s, std::vector<GlyphLine>* lines, int sizeIndex, const Vector2int16& availableSpace, bool useAvailableSpace, bool onlyProperFit, bool* textFits) const
	{
		int x = 0, y = 0;
		
		const Glyph* glyphTableSize = glyphTable + info.glyphCount * sizeIndex;
		
		int height = sizeTable[sizeIndex];
		
		int lastKerningOffset = 0;
		
		int lineStart = 0;
		int lineMaxWidth = 0;

        int wordStart = 0;
        int wordWidth = 0;
        int whitespaceWidth = 0;
		
		for (size_t i = 0; i < s.length(); ++i)
		{
			char ch = s[i];
			
			int glyphIndex = glyphRemap[static_cast<unsigned char>(ch)];
			
			if (glyphIndex == 255)
			{
				if (!isCharacterZeroWidth(ch))
					lastKerningOffset = 0;
				
				if (ch == '\n')
				{
					// hard line break
					pushLine(lines, s, lineStart, i, y, x);
					
					lineStart = i + 1;
					lineMaxWidth = std::max(lineMaxWidth, x);

					wordStart = i + 1;
					wordWidth = 0;
					whitespaceWidth = 0;
					
					x = 0;
					y += height;
				}
			
				continue;
			}
			
			const Glyph& glyph = glyphTableSize[glyphIndex];
			
			int xoffset = kerningTable[lastKerningOffset + glyphIndex] + glyph.xadvance;

            if (isCharacterWhitespace(ch))
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
                    pushLine(lines, s, lineStart, wordStart, y, x - wordWidth - whitespaceWidth);
                    
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
                    pushLine(lines, s, lineStart, i, y, x - xoffset);
                    
                    lineStart = i;
                    lineMaxWidth = std::max(lineMaxWidth, x - xoffset);
                    
                    x = xoffset;
                }
                
                wordWidth = x;
                whitespaceWidth = 0;
                y += height;
			}
			
			lastKerningOffset = glyph.kerningOffset;
		}

		// last line
		pushLine(lines, s, lineStart, s.length(), y, x);

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
