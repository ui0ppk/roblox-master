#include "stdafx.h"
#include "Image.h"

#include "g3d/BinaryInput.h"
#include "g3d/GImage.h"

namespace RBX
{
namespace Graphics
{

template <typename T> class LinearResampler
{
public:
	static void scale(unsigned char* target, unsigned int targetWidth, unsigned int targetHeight, const unsigned char* source, unsigned int sourceWidth, unsigned int sourceHeight)
	{
		if (targetWidth * 2 == sourceWidth && targetHeight * 2 == sourceHeight)
            downscaleFast2x2(target, source, sourceWidth, sourceHeight);
		else
			scaleSlow(target, targetWidth, targetHeight, source, sourceWidth, sourceHeight);
	}

private:
    static void downscaleFast2x2(unsigned char* target, const unsigned char* source, unsigned int sourceWidth, unsigned int sourceHeight)
    {
        RBXASSERT(sourceWidth > 1 && sourceWidth % 2 == 0);
        RBXASSERT(sourceHeight > 1 && sourceHeight % 2 == 0);
        RBXASSERT(reinterpret_cast<uintptr_t>(target) % sizeof(T) == 0);
        RBXASSERT(reinterpret_cast<uintptr_t>(source) % sizeof(T) == 0);
        
        const T* srcData = reinterpret_cast<const T*>(source);
        T* dstData = reinterpret_cast<T*>(target);
        
        unsigned int width = sourceWidth / 2;
        unsigned int height = sourceHeight / 2;
        
        for (unsigned int y = 0; y < height; ++y)
        {
            const T* srcRow = srcData;
            T* dstRow = dstData;
            
            for (unsigned int x = 0; x < width; ++x)
            {
                T c0 = srcRow[0];
                T c1 = srcRow[1];
                T c2 = srcRow[sourceWidth + 0];
                T c3 = srcRow[sourceWidth + 1];
                
                T t0 = (c0 | c1) - (((c0 ^ c1) >> 1) & 0x7f7f7f7f);
                T t1 = (c2 | c3) - (((c2 ^ c3) >> 1) & 0x7f7f7f7f);
                T rs = (t0 | t1) - (((t0 ^ t1) >> 1) & 0x7f7f7f7f);
                
                *dstRow = rs;
            
                srcRow += 2;
                dstRow += 1;
            }
            
            srcData += sourceWidth * 2;
            dstData += width;
        }
    }

	static void scaleSlow(unsigned char* target, unsigned int targetWidth, unsigned int targetHeight, const unsigned char* source, unsigned int sourceWidth, unsigned int sourceHeight)
	{
        typedef unsigned long long uint64;

		// srcdata stays at beginning of slice, pdst is a moving pointer
		const unsigned char* srcdata = source;
		unsigned char* pdst = target;

		// sx_48,sy_48 represent current position in source
		// using 16/48-bit fixed precision, incremented by steps
		uint64 stepx = ((uint64)sourceWidth << 48) / targetWidth;
		uint64 stepy = ((uint64)sourceHeight << 48) / targetHeight;
		
		// bottom 28 bits of temp are 16/12 bit fixed precision, used to
		// adjust a source coordinate backwards by half a pixel so that the
		// integer bits represent the first sample (eg, sx1) and the
		// fractional bits are the blend weight of the second sample
		unsigned int temp;
		
		uint64 sy_48 = (stepy >> 1) - 1;
		for (unsigned int y = 0; y < targetHeight; y++, sy_48+=stepy)
		{
			temp = static_cast<unsigned int>(sy_48 >> 36);
			temp = (temp > 0x800) ? temp - 0x800 : 0;
			unsigned int syf = temp & 0xFFF;
			unsigned int sy1 = temp >> 12;
			unsigned int sy2 = std::min(sy1+1, sourceHeight-1);
			unsigned int syoff1 = sy1 * sourceWidth;
			unsigned int syoff2 = sy2 * sourceWidth;

			uint64 sx_48 = (stepx >> 1) - 1;
			for (unsigned int x = 0; x < targetWidth; x++, sx_48+=stepx)
			{
				temp = static_cast<unsigned int>(sx_48 >> 36);
				temp = (temp > 0x800) ? temp - 0x800 : 0;
				unsigned int sxf = temp & 0xFFF;
				unsigned int sx1 = temp >> 12;
				unsigned int sx2 = std::min(sx1+1, sourceWidth-1);

				unsigned int sxfsyf = sxf*syf;
				for (unsigned int k = 0; k < sizeof(T); k++)
				{
					unsigned int accum =
						srcdata[(sx1 + syoff1)*sizeof(T)+k]*(0x1000000-(sxf<<12)-(syf<<12)+sxfsyf) +
						srcdata[(sx2 + syoff1)*sizeof(T)+k]*((sxf<<12)-sxfsyf) +
						srcdata[(sx1 + syoff2)*sizeof(T)+k]*((syf<<12)-sxfsyf) +
						srcdata[(sx2 + syoff2)*sizeof(T)+k]*sxfsyf;

					// accum is computed using 8/24-bit fixed-point math
					// (maximum is 0xFF000000; rounding will not cause overflow)
					*pdst++ = static_cast<unsigned char>((accum + 0x800000) >> 24);
				}
			}
		}
	}
};

struct DDS_PIXELFORMAT
{
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwFourCC;
	unsigned int dwRGBBitCount;
	unsigned int dwRBitMask;
	unsigned int dwGBitMask;
	unsigned int dwBBitMask;
	unsigned int dwABitMask;
};

struct DDS_HEADER
{
	unsigned int dwSize;
	unsigned int dwFlags;
	unsigned int dwHeight;
	unsigned int dwWidth;
	unsigned int dwPitchOrLinearSize;
	unsigned int dwDepth;
	unsigned int dwMipMapCount;
	unsigned int dwReserved1[11];
	DDS_PIXELFORMAT ddspf;
	unsigned int dwCaps;
	unsigned int dwCaps2;
	unsigned int dwCaps3;
	unsigned int dwCaps4;
	unsigned int dwReserved2;
};

static const unsigned int DDS_MAGIC = 0x20534444;

static const unsigned int DDSD_CAPS = 0x1;
static const unsigned int DDSD_HEIGHT = 0x2;
static const unsigned int DDSD_WIDTH = 0x4;
static const unsigned int DDSD_PIXELFORMAT = 0x1000;
static const unsigned int DDSD_MIPMAPCOUNT = 0x20000;

static const unsigned int DDSCAPS2_CUBEMAP = 0x200;
static const unsigned int DDSCAPS2_CUBEMAP_FACES = 0xFC00;
static const unsigned int DDSCAPS2_VOLUME = 0x200000;

static const unsigned int DDPF_ALPHAPIXELS = 0x1;
static const unsigned int DDPF_FOURCC = 0x4;
static const unsigned int DDPF_RGB = 0x40;

static const unsigned int DDS_FOURCC_RGBA16F = 113;
static const unsigned int DDS_FOURCC_DXT1 = 0x31545844;
static const unsigned int DDS_FOURCC_DXT3 = 0x33545844;
static const unsigned int DDS_FOURCC_DXT5 = 0x35545844;
static const unsigned int DDS_FOURCC_DX10 = 0x30315844;

struct PVRTCTexHeader2
{
    unsigned int headerLength;
    unsigned int height;
    unsigned int width;
    unsigned int numMipmaps;
    unsigned int flags;
    unsigned int dataLength;
    unsigned int bpp;
    unsigned int bitmaskRed;
    unsigned int bitmaskGreen;
    unsigned int bitmaskBlue;
    unsigned int bitmaskAlpha;
    unsigned int pvrTag;
    unsigned int numSurfs;
};

static const unsigned int kPVRTextureFlagMask = 0xff;
static const unsigned int kPVRTextureFlagTypePVRTC_2 = 24;
static const unsigned int kPVRTextureFlagTypePVRTC_4 = 25;
static const unsigned int kPVRTextureFlagTypeETC1 = 54;
static const unsigned int kPVRTextureFlagTypeRGBA = 18;

static void readData(std::istream& in, void* data, unsigned int size)
{
    in.read(static_cast<char*>(data), size);

    if (in.gcount() != size)
        throw RBX::runtime_error("Unexpected end of file while reading %d bytes", size);
}

static void writeData(std::ostream& out, const void* data, unsigned int size)
{
    out.write(static_cast<const char*>(data), size);
}

static unsigned int getMipCountToSkip(unsigned int width, unsigned int height, unsigned int depth, unsigned int maxSide)
{
    unsigned int result = 0;

    while (width > maxSide || height > maxSide || depth > maxSide)
	{
        result++;

        width /= 2;
        height /= 2;
        depth /= 2;
	}

    return result;
}

static Texture::Type getTypeDDS(const DDS_HEADER& header)
{
	if (header.dwCaps2 & DDSCAPS2_VOLUME)
	{
		if (header.dwDepth == 0)
			throw RBX::runtime_error("Invalid DDS header: depth is zero");

		return Texture::Type_3D;
	}
	else if (header.dwCaps2 & DDSCAPS2_CUBEMAP)
	{
		if ((header.dwCaps2 & DDSCAPS2_CUBEMAP_FACES) == DDSCAPS2_CUBEMAP_FACES)
			return Texture::Type_Cube;
		else
			throw RBX::runtime_error("Unsupported DDS: not all cubemap faces are present");
	}
	else
		return Texture::Type_2D;
}

static Texture::Format getFormatDDS(const DDS_HEADER& header)
{
	const DDS_PIXELFORMAT& f = header.ddspf;

	if (f.dwFourCC == 0)
	{
		if (f.dwRGBBitCount == 8 && f.dwRBitMask == 0xff && f.dwGBitMask == 0 && f.dwBBitMask == 0 && f.dwABitMask == 0)
			return Texture::Format_L8;
		else if (f.dwRGBBitCount == 16 && f.dwRBitMask == 0xff && f.dwGBitMask == 0 && f.dwBBitMask == 0 && f.dwABitMask == 0xff00)
            return Texture::Format_LA8;
		else if (f.dwRGBBitCount == 32)
			return Texture::Format_RGBA8;
		else
			throw RBX::runtime_error("Unsupported DDS: custom non-compressed %d-bit format", f.dwRGBBitCount);
	}
	else
	{
		if (f.dwFourCC == 113)
			return Texture::Format_RGBA16F;
		else if (f.dwFourCC == DDS_FOURCC_DXT1)
			return Texture::Format_BC1;
		else if (f.dwFourCC == DDS_FOURCC_DXT3)
			return Texture::Format_BC2;
		else if (f.dwFourCC == DDS_FOURCC_DXT5)
			return Texture::Format_BC3;
		else if (f.dwFourCC == DDS_FOURCC_DX10)
			throw RBX::runtime_error("Unsupported DDS: DX10 FourCC");
		else
			throw RBX::runtime_error("Unsupported DDS: unknown FourCC %d", f.dwFourCC);
	}
}

/*
    Copyright (c) 2006 Simon Brown                          si@sjbrown.co.uk

    Permission is hereby granted, free of charge, to any person obtaining
    a copy of this software and associated documentation files (the 
    "Software"), to deal in the Software without restriction, including
    without limitation the rights to use, copy, modify, merge, publish,
    distribute, sublicense, and/or sell copies of the Software, and to 
    permit persons to whom the Software is furnished to do so, subject to 
    the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
    CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
    TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
    SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

namespace squish
{
    typedef unsigned char u8;

    static int Unpack565( u8 const* packed, u8* colour )
    {
            // build the packed value
            int value = ( int )packed[0] | ( ( int )packed[1] << 8 );
            
            // get the components in the stored range
            u8 red = ( u8 )( ( value >> 11 ) & 0x1f );
            u8 green = ( u8 )( ( value >> 5 ) & 0x3f );
            u8 blue = ( u8 )( value & 0x1f );

            // scale up to 8 bits
            colour[0] = ( red << 3 ) | ( red >> 2 );
            colour[1] = ( green << 2 ) | ( green >> 4 );
            colour[2] = ( blue << 3 ) | ( blue >> 2 );
            colour[3] = 255;
            
            // return the value
            return value;
    }

    void DecompressColour( u8* rgba, void const* block, bool isDxt1 )
    {
        // get the block bytes
        u8 const* bytes = reinterpret_cast< u8 const* >( block );
        
        // unpack the endpoints
        u8 codes[16];
        int a = Unpack565( bytes, codes );
        int b = Unpack565( bytes + 2, codes + 4 );
        
        // generate the midpoints
        for( int i = 0; i < 3; ++i )
        {
                int c = codes[i];
                int d = codes[4 + i];

                if( isDxt1 && a <= b )
                {
                        codes[8 + i] = ( u8 )( ( c + d )/2 );
                        codes[12 + i] = 0;
                }
                else
                {
                        codes[8 + i] = ( u8 )( ( 2*c + d )/3 );
                        codes[12 + i] = ( u8 )( ( c + 2*d )/3 );
                }
        }
        
        // fill in alpha for the intermediate values
        codes[8 + 3] = 255;
        codes[12 + 3] = ( isDxt1 && a <= b ) ? 0 : 255;
        
        // unpack the indices
        u8 indices[16];
        for( int i = 0; i < 4; ++i )
        {
                u8* ind = indices + 4*i;
                u8 packed = bytes[4 + i];
                
                ind[0] = packed & 0x3;
                ind[1] = ( packed >> 2 ) & 0x3;
                ind[2] = ( packed >> 4 ) & 0x3;
                ind[3] = ( packed >> 6 ) & 0x3;
        }

        // store out the colours
        for( int i = 0; i < 16; ++i )
        {
                u8 offset = 4*indices[i];
                for( int j = 0; j < 4; ++j )
                        rgba[4*i + j] = codes[offset + j];
        }
	}

    void DecompressAlphaDxt3( u8* rgba, void const* block )
    {
        u8 const* bytes = reinterpret_cast< u8 const* >( block );

        // unpack the alpha values pairwise
        for( int i = 0; i < 8; ++i )
        {
            // quantise down to 4 bits
            u8 quant = bytes[i];

            // unpack the values
            u8 lo = quant & 0x0f;
            u8 hi = quant & 0xf0;

            // convert back up to bytes
            rgba[8*i + 3] = lo | ( lo << 4 );
            rgba[8*i + 7] = hi | ( hi >> 4 );
        }
    }

    void DecompressAlphaDxt5( u8* rgba, void const* block )
    {
        // get the two alpha values
        u8 const* bytes = reinterpret_cast< u8 const* >( block );
        int alpha0 = bytes[0];
        int alpha1 = bytes[1];
        
        // compare the values to build the codebook
        u8 codes[8];
        codes[0] = ( u8 )alpha0;
        codes[1] = ( u8 )alpha1;
        if( alpha0 <= alpha1 )
        {
                // use 5-alpha codebook
                for( int i = 1; i < 5; ++i )
                        codes[1 + i] = ( u8 )( ( ( 5 - i )*alpha0 + i*alpha1 )/5 );
                codes[6] = 0;
                codes[7] = 255;
        }
        else
        {
                // use 7-alpha codebook
                for( int i = 1; i < 7; ++i )
                        codes[1 + i] = ( u8 )( ( ( 7 - i )*alpha0 + i*alpha1 )/7 );
        }
        
        // decode the indices
        u8 indices[16];
        u8 const* src = bytes + 2;
        u8* dest = indices;
        for( int i = 0; i < 2; ++i )
        {
                // grab 3 bytes
                int value = 0;
                for( int j = 0; j < 3; ++j )
                {
                        int byte = *src++;
                        value |= ( byte << 8*j );
                }
                
                // unpack 8 3-bit values from it
                for( int j = 0; j < 8; ++j )
                {
                        int index = ( value >> 3*j ) & 0x7;
                        *dest++ = ( u8 )index;
                }
        }
        
        // write out the indexed codebook values
        for( int i = 0; i < 16; ++i )
                rgba[4*i + 3] = codes[indices[i]];
    }
}

static void decodeDXTBlock(unsigned char* target, const unsigned char* source, Texture::Format format)
{
    switch (format)
	{
	case Texture::Format_BC1:
		squish::DecompressColour(target, source, true);
        break;

	case Texture::Format_BC2:
		squish::DecompressColour(target, source + 8, false);
		squish::DecompressAlphaDxt3(target, source);
        break;

	case Texture::Format_BC3:
		squish::DecompressColour(target, source + 8, false);
		squish::DecompressAlphaDxt5(target, source);
        break;

	default:
        RBXASSERT(false);
	}
}

void Image::decodeDXT(unsigned char* target, const unsigned char* source, unsigned int width, unsigned int height, unsigned int depth, Texture::Format format)
{
    unsigned char buffer[4][4][4];

	unsigned int blockSize = Texture::getImageSize(format, 4, 4);

    const unsigned char* block = source;

    for (unsigned int z = 0; z < depth; z++)
	{
        unsigned char* targetSlice = target + z * (width * height * 4);

        for (unsigned int y = 0; y < height; y += 4)
            for (unsigned int x = 0; x < width; x += 4)
			{
				decodeDXTBlock(&buffer[0][0][0], block, format);

                block += blockSize;

                for (unsigned int yi = 0; yi < 4; ++yi)
                    for (unsigned int xi = 0; xi < 4; ++xi)
                        if (x + xi < width && y + yi < height)
						{
                            unsigned char* targetPixel = targetSlice + 4 * ((x + xi) + width * (y + yi));

                            targetPixel[0] = buffer[yi][xi][0];
                            targetPixel[1] = buffer[yi][xi][1];
                            targetPixel[2] = buffer[yi][xi][2];
                            targetPixel[3] = buffer[yi][xi][3];
                        }
			}
	}
}

static void flipRGB(unsigned char* data, unsigned int width, unsigned int height, unsigned int depth)
{
    unsigned int count = width * height * depth;

    for (unsigned int i = 0; i < count; i++)
	{
        std::swap(data[i * 4 + 0], data[i * 4 + 2]);
	}
}

static unsigned int adjustMipLevels(unsigned int mipLevels, unsigned int maxMipLevels, bool forceFullMipChain)
{
	if (forceFullMipChain && mipLevels != 1)
		return maxMipLevels;

	return mipLevels;
}

static Image::LoadResult loadFromDDS(std::istream& in, unsigned int maxTextureSize, unsigned int flags)
{
    unsigned int magic;
    readData(in, &magic, sizeof(magic));

	DDS_HEADER header;
    readData(in, &header, sizeof(header));

    if (magic != DDS_MAGIC)
		throw RBX::runtime_error("Invalid DDS header");

	if (header.dwWidth == 0 || header.dwHeight == 0)
		throw RBX::runtime_error("Invalid DDS header: width/height is zero");

	Texture::Type type = getTypeDDS(header);

    Texture::Format fileFormat = getFormatDDS(header);
	Texture::Format format = (Texture::isFormatCompressed(fileFormat) && (flags & Image::Load_DecodeDXT) != 0) ? Texture::Format_RGBA8 : fileFormat;

	unsigned int faces = (type == Texture::Type_Cube) ? 6 : 1;
	unsigned int depth = (type == Texture::Type_3D) ? header.dwDepth : 1;
	unsigned int maxMipLevels = Texture::getMaxMipCount(header.dwWidth, header.dwHeight, depth);
	unsigned int mipLevels = header.dwMipMapCount == 0 ? 1 : adjustMipLevels(header.dwMipMapCount, maxMipLevels, (flags & Image::Load_ForceFullMipChain) != 0);

	if ((flags & Image::Load_RoundToPOT) && !(G3D::isPow2(header.dwWidth) && G3D::isPow2(header.dwHeight) && G3D::isPow2(depth)))
		throw RBX::runtime_error("Unsupported DDS: non-power-of-two image needs to be resized");

	unsigned int skipMips = getMipCountToSkip(header.dwWidth, header.dwHeight, depth, maxTextureSize);

    if (skipMips >= mipLevels)
		throw RBX::runtime_error("Unsupported DDS: %d out of %d mipmap levels need to be skipped", skipMips, mipLevels);

	shared_ptr<Image> image(new Image(type, format, Texture::getMipSide(header.dwWidth, skipMips), Texture::getMipSide(header.dwHeight, skipMips), Texture::getMipSide(depth, skipMips), mipLevels - skipMips));

    for (unsigned int face = 0; face < faces; ++face)
	{
		for (unsigned int mip = 0; mip < mipLevels; ++mip)
		{
			unsigned int mipWidth = Texture::getMipSide(header.dwWidth, mip);
			unsigned int mipHeight = Texture::getMipSide(header.dwHeight, mip);
			unsigned int mipDepth = Texture::getMipSide(depth, mip);

			unsigned int fileMipSize = Texture::getImageSize(fileFormat, mipWidth, mipHeight) * mipDepth;

            if (mip < skipMips)
			{
				in.seekg(fileMipSize, std::ios::cur);
			}
			else
			{
                unsigned char* mipData = image->getMipData(face, mip - skipMips);

                // Read mip
                if (format == fileFormat)
                {
                    // Read raw data & flip if necessary
                    readData(in, mipData, fileMipSize);

                    if (format == Texture::Format_RGBA8)
                    {
                        RBXASSERT(header.ddspf.dwRGBBitCount == 32);

                        bool sourceBGR = (header.ddspf.dwBBitMask == 0xff);
                        bool targetBGR = (flags & Image::Load_OutputBGR) != 0;

                        if (sourceBGR != targetBGR)
                            flipRGB(mipData, mipWidth, mipHeight, mipDepth);
                    }
                }
                else
                {
                    // Read & decode DXT
                    boost::scoped_array<unsigned char> buffer(new unsigned char[fileMipSize]);
                    readData(in, buffer.get(), fileMipSize);

                    Image::decodeDXT(mipData, buffer.get(), mipWidth, mipHeight, mipDepth, fileFormat);

                    if (flags & Image::Load_OutputBGR)
                        flipRGB(mipData, mipWidth, mipHeight, mipDepth);
                }
			}
		}
	}

	return Image::LoadResult(image, ImageInfo(header.dwWidth, header.dwHeight, true));
}

static Texture::Format getFormatPVR(const PVRTCTexHeader2& header)
{
	unsigned int format = header.flags & kPVRTextureFlagMask;

	if (format == kPVRTextureFlagTypePVRTC_2)
		return header.bitmaskAlpha ? Texture::Format_PVRTC_RGBA2 : Texture::Format_PVRTC_RGB2;
	else if (format == kPVRTextureFlagTypePVRTC_4)
		return header.bitmaskAlpha ? Texture::Format_PVRTC_RGBA4 : Texture::Format_PVRTC_RGB4;
	else if (format == kPVRTextureFlagTypeETC1)
        return Texture::Format_ETC1;
    else if (format == kPVRTextureFlagTypeRGBA)
        return Texture::Format_RGBA8;
    else
		throw RBX::runtime_error("Unsupported PVR: unknown format %d", format);
}

static Image::LoadResult loadFromPVR(std::istream& in, unsigned int maxTextureSize, unsigned int flags)
{
    PVRTCTexHeader2 header;
	readData(in, &header, sizeof(header));

	if (header.pvrTag != 0x21525650)
		throw RBX::runtime_error("Invalid PVR header");

	if (header.width == 0 || header.height == 0)
		throw RBX::runtime_error("Invalid PVR header: width/height is zero");

	if ((flags & Image::Load_RoundToPOT) && !(G3D::isPow2(header.width) && G3D::isPow2(header.height)))
		throw RBX::runtime_error("Unsupported PVR: non-power-of-two image needs to be resized");

	Texture::Format format = getFormatPVR(header);

	unsigned int maxMipLevels = Texture::getMaxMipCount(header.width, header.height, 1);
	unsigned int mipLevels = adjustMipLevels(header.numMipmaps + 1, maxMipLevels, (flags & Image::Load_ForceFullMipChain) != 0);

	unsigned int skipMips = getMipCountToSkip(header.width, header.height, 1, maxTextureSize);

    if (skipMips >= mipLevels)
		throw RBX::runtime_error("Unsupported PVR: %d out of %d mipmap levels need to be skipped", skipMips, mipLevels);

	shared_ptr<Image> image(new Image(Texture::Type_2D, format, Texture::getMipSide(header.width, skipMips), Texture::getMipSide(header.height, skipMips), 1, mipLevels - skipMips));

    for (unsigned int mip = 0; mip < mipLevels; ++mip)
    {
		unsigned int mipWidth = Texture::getMipSide(header.width, mip);
		unsigned int mipHeight = Texture::getMipSide(header.height, mip);
        unsigned int mipSize = Texture::getImageSize(format, mipWidth, mipHeight);

        if (mip < skipMips)
        {
            in.seekg(mipSize, std::ios::cur);
        }
        else
        {
            unsigned char* mipData = image->getMipData(0, mip - skipMips);

            readData(in, mipData, mipSize);
		}
	}

	return Image::LoadResult(image, ImageInfo(header.width, header.height, true));
}

static void decodeImage(G3D::GImage& image, G3D::GImage::Format format, std::istream& in)
{
    // Unfortunately, we need to copy the entire source data out of the stream and into BinaryInput since BinaryInput is not flexible
    in.seekg(0, std::ios::end);
    std::streamoff pos = in.tellg();
    in.seekg(0);

	if (in.fail() || pos < 0)
		throw RBX::runtime_error("Input stream is not seekable");

    unsigned int length = static_cast<unsigned int>(pos);

	boost::scoped_array<unsigned char> data(new unsigned char[length]);
    readData(in, data.get(), length);

	G3D::BinaryInput bin(data.get(), length, G3D::G3D_LITTLE_ENDIAN, false, false);

    try
	{
        image.decode(bin, format);
	}
	catch (const G3D::GImage::Error& e)
	{
		throw std::runtime_error(e.reason);
	}
}

static unsigned int adjustSide(unsigned int side, unsigned int maxTextureSize, unsigned int flags)
{
	unsigned int rounded = (flags & Image::Load_RoundToPOT) ? G3D::ceilPow2(side) : side;

	return std::min(rounded, maxTextureSize);
}

static Texture::Format getFormatAndConvertImage(G3D::GImage& gimage)
{
    switch (gimage.channels())
	{
	case 1:
    case 2:
    case 3:
		gimage.convertToRGBA();
		RBXASSERT(gimage.channels() == 4);
		return Texture::Format_RGBA8;

	case 4:
		return Texture::Format_RGBA8;

	default:
		throw RBX::runtime_error("Unsupported image: can't decode image with %d channels", gimage.channels()); 
	}
}

static void scaleImage(unsigned char* target, unsigned int targetWidth, unsigned int targetHeight, const unsigned char* source, unsigned int sourceWidth, unsigned int sourceHeight, Texture::Format format)
{
    switch (format)
	{
	case Texture::Format_L8:
		LinearResampler<unsigned char>::scale(target, targetWidth, targetHeight, source, sourceWidth, sourceHeight);
        break;

	case Texture::Format_LA8:
		LinearResampler<unsigned short>::scale(target, targetWidth, targetHeight, source, sourceWidth, sourceHeight);
        break;

	case Texture::Format_RGBA8:
		LinearResampler<unsigned int>::scale(target, targetWidth, targetHeight, source, sourceWidth, sourceHeight);
        break;

	default:
		throw RBX::runtime_error("Unsupported image: can't scale image with format %d", format);
	}
}

static bool hasTransparentPixels(const G3D::GImage& gimage)
{
    if (gimage.channels() == 2 || gimage.channels() == 4)
    {
        const unsigned char* data = gimage.byte();
        unsigned int size = gimage.width() * gimage.height() * gimage.channels();
        unsigned int channels = gimage.channels();

        for (unsigned int i = 0; i < size; i += channels)
            if (data[i + (channels - 1)] != 255)
                return true;

        return false;
    }

    return false;
}

static Image::LoadResult loadGeneric(std::istream& in, G3D::GImage::Format gformat, unsigned int maxTextureSize, unsigned int flags)
{
    // Load base image
	G3D::GImage gimage;
	decodeImage(gimage, gformat, in);

	if (gimage.width() <= 0 || gimage.height() <= 0)
		throw RBX::runtime_error("Invalid image: width/height is zero");

	bool hasAlpha = hasTransparentPixels(gimage);
	Texture::Format format = getFormatAndConvertImage(gimage);

    // Flip RGB in the base image
	if (format == Texture::Format_RGBA8 && (flags & Image::Load_OutputBGR))
		flipRGB(gimage.byte(), gimage.width(), gimage.height(), 1);

    // Determine final texture dimensions
	unsigned int width = adjustSide(gimage.width(), maxTextureSize, flags);
	unsigned int height = adjustSide(gimage.height(), maxTextureSize, flags);
    unsigned int mipLevels = Texture::getMaxMipCount(width, height, 1);

	shared_ptr<Image> image(new Image(Texture::Type_2D, format, width, height, 1, mipLevels));

    // Load and generate mip level
    for (unsigned int mip = 0; mip < mipLevels; ++mip)
	{
		unsigned int mipWidth = Texture::getMipSide(width, mip);
		unsigned int mipHeight = Texture::getMipSide(height, mip);

        unsigned char* mipData = image->getMipData(0, mip);

        if (mip == 0)
		{
			if (mipWidth == gimage.width() && mipHeight == gimage.height())
				memcpy(mipData, gimage.byte(), Texture::getImageSize(format, mipWidth, mipHeight));
			else
                scaleImage(mipData, mipWidth, mipHeight, gimage.byte(), gimage.width(), gimage.height(), format);
		}
		else
		{
            unsigned int prevMipWidth = Texture::getMipSide(width, mip - 1);
            unsigned int prevMipHeight = Texture::getMipSide(height, mip - 1);

            unsigned char* prevMipData = image->getMipData(0, mip - 1);

			scaleImage(mipData, mipWidth, mipHeight, prevMipData, prevMipWidth, prevMipHeight, format);
		}
	}

	return Image::LoadResult(image, ImageInfo(gimage.width(), gimage.height(), hasAlpha));
}

Image::LoadResult::LoadResult(const shared_ptr<Image>& image, const ImageInfo& info)
	: image(image)
    , info(info)
{
}

Image::Image(Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels)
	: type(type)
    , format(format)
    , width(width)
    , height(height)
    , depth(depth)
    , mipLevels(mipLevels)
{
    RBXASSERT(width > 0 && height > 0 && depth > 0);
	RBXASSERT(mipLevels > 0 && mipLevels <= Texture::getMaxMipCount(width, height, depth));
	RBXASSERT(type == Texture::Type_3D || depth == 1);

	unsigned int faces = (type == Texture::Type_Cube) ? 6 : 1;

	mipOffsets.reserve(faces * mipLevels);

    unsigned int totalSize = 0;

    for (unsigned int index = 0; index < faces; ++index)
	{
        for (unsigned int mip = 0; mip < mipLevels; ++mip)
		{
			mipOffsets.push_back(totalSize);

			totalSize += Texture::getImageSize(format, Texture::getMipSide(width, mip), Texture::getMipSide(height, mip)) * Texture::getMipSide(depth, mip);
		}
	}

	data.reset(new unsigned char[totalSize]);
    dataSize = totalSize;
}

Image::~Image()
{
}

unsigned char* Image::getMipData(unsigned int index, unsigned int level)
{
	return const_cast<unsigned char*>(static_cast<const Image*>(this)->getMipData(index, level));
}

const unsigned char* Image::getMipData(unsigned int index, unsigned int level) const
{
	unsigned int faces = (type == Texture::Type_Cube) ? 6 : 1;

    RBXASSERT(index < faces);
    RBXASSERT(level < mipLevels);

	return data.get() + mipOffsets[index * mipLevels + level];
}

Image::LoadResult Image::load(std::istream& stream, unsigned int maxTextureSize, unsigned int flags)
{
    // Determine image format based on the header
    char header[64];
	stream.read(header, sizeof(header));

	G3D::GImage::Format format = G3D::GImage::resolveFormat("", reinterpret_cast<unsigned char*>(header), static_cast<unsigned int>(stream.gcount()), G3D::GImage::AUTODETECT);

	if (format == G3D::GImage::UNKNOWN)
		throw std::runtime_error("Failed to resolve texture format");

    // Reset the stream (clear failbit if file is <64 bytes)
	stream.clear();
	stream.seekg(0);

    if (format == G3D::GImage::DDS)
		return loadFromDDS(stream, maxTextureSize, flags);
	else if (format == G3D::GImage::PVR)
		return loadFromPVR(stream, maxTextureSize, flags);
	else
		return loadGeneric(stream, format, maxTextureSize, flags);
}

static void setFormatDDS(DDS_PIXELFORMAT& pf, Texture::Format format)
{
	pf.dwSize = sizeof(DDS_PIXELFORMAT);

    switch (format)
	{
	case Texture::Format_L8:
		pf.dwFlags = DDPF_RGB;
		pf.dwRGBBitCount = 8;
		pf.dwRBitMask = 0xff;
        break;

	case Texture::Format_LA8:
		pf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		pf.dwRGBBitCount = 16;
		pf.dwRBitMask = 0xff;
		pf.dwABitMask = 0xff00;
        break;

	case Texture::Format_RGBA8:
		pf.dwFlags = DDPF_RGB | DDPF_ALPHAPIXELS;
		pf.dwRGBBitCount = 32;
		pf.dwRBitMask = 0xff0000;
		pf.dwGBitMask = 0xff00;
		pf.dwBBitMask = 0xff;
		pf.dwABitMask = 0xff000000;
        break;

	case Texture::Format_RGBA16F:
		pf.dwFlags = DDPF_FOURCC;
		pf.dwFourCC = DDS_FOURCC_RGBA16F;
        break;

	case Texture::Format_BC1:
		pf.dwFlags = DDPF_FOURCC;
		pf.dwFourCC = DDS_FOURCC_DXT1;
        break;

	case Texture::Format_BC2:
		pf.dwFlags = DDPF_FOURCC;
		pf.dwFourCC = DDS_FOURCC_DXT3;
        break;

	case Texture::Format_BC3:
		pf.dwFlags = DDPF_FOURCC;
		pf.dwFourCC = DDS_FOURCC_DXT5;
        break;

	default:
        RBXASSERT(false);
	}
}

void Image::saveDDS(std::ostream& stream)
{
    unsigned int magic = DDS_MAGIC;
    writeData(stream, &magic, sizeof(magic));

	DDS_HEADER header = {};
	header.dwSize = sizeof(DDS_HEADER);
	header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT | DDSD_MIPMAPCOUNT;
	header.dwHeight = height;
	header.dwWidth = width;
	header.dwDepth = depth;
	header.dwMipMapCount = mipLevels;
	header.dwCaps2 = (type == Texture::Type_3D) ? DDSCAPS2_VOLUME : (type == Texture::Type_Cube) ? (DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_FACES) : 0;

	setFormatDDS(header.ddspf, format);

    writeData(stream, &header, sizeof(header));

	writeData(stream, data.get(), dataSize);
}

}
}
