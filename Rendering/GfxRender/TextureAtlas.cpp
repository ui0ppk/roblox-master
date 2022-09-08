#include "stdafx.h"
#include "TextureAtlas.h"
#include "VisualEngine.h"
#include "Image.h"

#include "GfxCore/Device.h"
#include "GfxCore/Texture.h"

DYNAMIC_FASTINTVARIABLE(TexAtlasUpdateLineHeight, 150)

namespace RBX
{
namespace Graphics
{
    TextureAtlas::TextureAtlas(VisualEngine* visualEngine, unsigned width, unsigned height)
        : textureSize(width, height)
        , uploadRect(UINT_MAX, 0)
        , lastFailedSize(0x7FFF, 0x7FFF)
    {
        reset();

        RBXASSERT((height & (height - 1)) == 0); // glyph texture height has to be power of two!
        moduloParam = height - 1;

        updateWindow = new unsigned char[textureSize.x * 2 * DFInt::TexAtlasUpdateLineHeight];
        texture = visualEngine->getDevice()->createTexture(Texture::Type_2D, Texture::Format_L8, textureSize.x, textureSize.y, 1, 1, Texture::Usage_Dynamic);
    }

    TextureAtlas::~TextureAtlas()
    {
        delete[] updateWindow;
    }

    bool TextureAtlas::getRegion(const boost::uint64_t& id, const TextureRegion*& region)
    {
        AtlasEntries::const_iterator it = atlasEntries.find(id);

        if (it != atlasEntries.end())
        {
            region = &it->second;
            int distance = (int)updateLineY - (int)region->y;

            if (distance > ((int)textureSize.y - DFInt::TexAtlasUpdateLineHeight))
                return false;
            return true;
        }

        return false;
    }

    void TextureAtlas::reset()
    {
        skyline.clear();
        skyline.push_back(Vector2uint32(0,0));
        skyline.push_back(Vector2uint32(textureSize.x-1, 0)); // sentinel

        atlasEntries.clear();

        updateWindowPosition = 0;
        updateLineY = 0;
    }

    void TextureAtlas::upload()
    {
        updateLineY = skyline.begin()->y;
        unsigned newUpdateWindowPos = UINT_MAX;
        for (SkylineArray::iterator it = skyline.begin(); it != skyline.end(); ++it)
        {
            updateLineY = std::max(updateLineY, it->y);
            newUpdateWindowPos = std::min(newUpdateWindowPos, it->y);
        }

        // update skyline to fit to update window next frame
        newUpdateWindowPos = std::max((int)updateLineY - DFInt::TexAtlasUpdateLineHeight, (int)newUpdateWindowPos);
        for (SkylineArray::iterator it = skyline.begin(); it != skyline.end(); ++it)
        {
            it->y = std::max(it->y, newUpdateWindowPos);
        }
        skyline[skyline.size()-1].y = newUpdateWindowPos;

        if (uploadRect.min < uploadRect.max)
        { 
            unsigned texureMin = uploadRect.min & moduloParam;
            unsigned textureMax = uploadRect.max & moduloParam;

            unsigned uploadWindowStartIdx = (uploadRect.min - updateWindowPosition) * textureSize.x;
            unsigned char* dataPtr = &updateWindow[uploadWindowStartIdx];
            unsigned height = textureMax > texureMin ? textureMax - texureMin : textureSize.y - texureMin;
            texture.getTexture()->upload(0, 0, RBX::Graphics::TextureRegion(0, texureMin, textureSize.x, height), dataPtr, textureSize.x * (height));

            if (textureMax < texureMin)
            {
                dataPtr = &updateWindow[uploadWindowStartIdx + height * textureSize.x];
                texture.getTexture()->upload(0, 0, RBX::Graphics::TextureRegion(0, 0, textureSize.x, textureMax), dataPtr, textureSize.x * textureMax);
            }

            texture.getTexture()->commitChanges();
        }

        unsigned updateLineShift = newUpdateWindowPos - updateWindowPosition;
        unsigned srcIdx = updateLineShift * textureSize.x;
        memmove(updateWindow, &updateWindow[srcIdx], (DFInt::TexAtlasUpdateLineHeight*2 - updateLineShift - 1) * textureSize.x);
        updateWindowPosition = newUpdateWindowPos;


        uploadRect = Interval1D(UINT_MAX, 0);
        
        lastFailedSize = Vector2int16(0x7FFF, 0x7FFF);

        if (updateLineY >= UINT_MAX - DFInt::TexAtlasUpdateLineHeight)
            reset();
    }

    bool TextureAtlas::insert(const boost::uint64_t& id, const Vector2int16& size, unsigned dataPitch, unsigned char* data, const TextureRegion*& outRegion)
    {
        if (getRegion(id, outRegion))
            return true;

        Vector2int16 realSize = size + Vector2int16(2,2);

        unsigned bestFitY = UINT_MAX;
        unsigned bestFitOffset = 0;

        unsigned bestFitIdx = 0;
        for (unsigned i = 0; 0 < skyline.size() || skyline[i].x >= textureSize.x-1; ++i)
        {    
            unsigned fitEnd = i + 1;
            if (fitEnd >= skyline.size()) 
                break;

            int fitWidth = 0;
            unsigned fitYOffset = 0;

            do
            {
                fitWidth = skyline[fitEnd].x - skyline[i].x;
                if (fitWidth >= realSize.x)
                {
                    // fit found
                    break;
                }

                if (skyline[fitEnd].y > skyline[i].y)
                {
                    unsigned thisOffset = skyline[fitEnd].y - skyline[i].y;
                    fitYOffset = std::max(fitYOffset, thisOffset);
                }

                ++fitEnd;
            }
            while(fitEnd < skyline.size());

            if (fitWidth >= realSize.x && skyline[i].y + fitYOffset < bestFitY)
            {
                bestFitY = skyline[i].y + fitYOffset;
                bestFitOffset = fitYOffset;
                bestFitIdx = i;
            }
        }

        Vector2uint32 borderPos = skyline[bestFitIdx];
        borderPos.y += bestFitOffset;
        Vector2uint32 dataPos = borderPos;
        dataPos.x += 1;
        dataPos.y += 1;

        // if it doesnt fit in updateLine skip it (we will render it next frame)
        if (skyline[bestFitIdx].y + realSize.y + bestFitOffset > updateLineY + DFInt::TexAtlasUpdateLineHeight)
        {
            lastFailedSize = size;
            return false;
        }

        // find have much space we need to make in skyline array
        unsigned adjustIdx = bestFitIdx;
        while(adjustIdx < skyline.size() && skyline[adjustIdx].x < borderPos.x + realSize.x)
            ++adjustIdx;

        // shift the array (and make space for 2 new values]
        int offset = bestFitIdx - adjustIdx + 2;
        if (offset > 0)
        {
            skyline.resize(skyline.size() + offset);

            for (int i = (skyline.size() - 1 - offset); i >= (int)adjustIdx; --i)
            {
                skyline[i+offset] = skyline[i];
            }
        }
        if (offset < 0)
        {
            for(unsigned i = adjustIdx + offset; i < skyline.size() + offset; ++i)
                skyline[i] = skyline[i-offset];

            skyline.resize(skyline.size() + offset);
        }
        
        skyline[bestFitIdx    ] = Vector2uint32(borderPos.x, borderPos.y + realSize.y);
        skyline[bestFitIdx + 1] = Vector2uint32(borderPos.x + realSize.x, borderPos.y);
        
        // cut duplicates at the end
        if (skyline[skyline.size() - 1].x == skyline[skyline.size()-2].x)
            skyline.resize(skyline.size() - 1);

        // clean borders (2px border to clean, we have to cleanup after previous glyph too)
        Vector2uint32 borderPosUpdateWindow = borderPos;
        borderPosUpdateWindow.y -= updateWindowPosition;
        memset(&updateWindow[getUpdateWindowIdx((int)borderPosUpdateWindow.x, (int)borderPosUpdateWindow.y - 1             )], 0, realSize.x);
        memset(&updateWindow[getUpdateWindowIdx((int)borderPosUpdateWindow.x, (int)borderPosUpdateWindow.y                 )], 0, realSize.x);
        memset(&updateWindow[getUpdateWindowIdx((int)borderPosUpdateWindow.x, (int)borderPosUpdateWindow.y + realSize.y    )], 0, realSize.x);
        memset(&updateWindow[getUpdateWindowIdx((int)borderPosUpdateWindow.x, (int)borderPosUpdateWindow.y + realSize.y - 1)], 0, realSize.x);

        for (int i = 0; i < realSize.y; ++i)
        {
            updateWindow[getUpdateWindowIdx((int)borderPosUpdateWindow.x - 1, (int)borderPosUpdateWindow.y + i)] = 0;
            updateWindow[getUpdateWindowIdx((int)borderPosUpdateWindow.x, borderPosUpdateWindow.y + i)] = 0;
            updateWindow[getUpdateWindowIdx((int)borderPosUpdateWindow.x + realSize.x - 1, (int)borderPosUpdateWindow.y + i)] = 0;
            updateWindow[getUpdateWindowIdx((int)borderPosUpdateWindow.x + realSize.x, (int)borderPosUpdateWindow.y + i)] = 0;
        }

        // insert the data to update Window
        for (int i = 0; i < size.y; ++i)
        {
            unsigned yPos = dataPos.y + i - updateWindowPosition;
            RBXASSERT((int)yPos < DFInt::TexAtlasUpdateLineHeight * 2);
            unsigned dataIdx = (yPos * textureSize.x) + dataPos.x;
            unsigned sourceIdx = (i * dataPitch);

            memcpy(&updateWindow[dataIdx], &data[sourceIdx], size.x);
        }

        uploadRect.min = std::min(uploadRect.min, (unsigned)(dataPos.y - 1)); // -1 to include 2px border cleanup
        uploadRect.max = std::max(uploadRect.max, (unsigned)(dataPos.y - 1 + realSize.y)); // +1 for same reason

        atlasEntries[id] = TextureRegion(dataPos.x, dataPos.y, size.x, size.y);
        outRegion = &atlasEntries[id];
        
        return true;
    }

    unsigned TextureAtlas::getUpdateWindowIdx(int x, int y)
    {
        return std::max(0, std::min(DFInt::TexAtlasUpdateLineHeight*2-1, y)) * textureSize.x + std::max(0, std::min(x, (int)textureSize.x - 1));
    }
}
}

