#include "TextureD3D11.h"

#include "FramebufferD3D11.h"
#include "DeviceD3D11.h"

#include "HeadersD3D11.h"

LOGGROUP(Graphics)

namespace RBX
{
namespace Graphics
{
    static const DXGI_FORMAT gTextureFormatD3D11[Texture::Format_Count] =
    {
        DXGI_FORMAT_R8_UNORM,
        DXGI_FORMAT_R8G8_UNORM,
        DXGI_FORMAT_B5G5R5A1_UNORM,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        DXGI_FORMAT_R16G16_UNORM,
        DXGI_FORMAT_R16G16B16A16_FLOAT,
        DXGI_FORMAT_BC1_UNORM,
        DXGI_FORMAT_BC3_UNORM,
        DXGI_FORMAT_BC3_UNORM,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_UNKNOWN,
        DXGI_FORMAT_D16_UNORM,
        DXGI_FORMAT_D24_UNORM_S8_UINT,
    };

    struct TextureUsageD3D11
    {
        D3D11_USAGE usage;
        unsigned    cpuAccess;
        unsigned    bindFlags;
        unsigned    misc;
    };

    static const TextureUsageD3D11 gTextureUsageD3D11[Texture::Usage_Count] =
    {
        { D3D11_USAGE_DEFAULT, 0, D3D11_BIND_SHADER_RESOURCE , 0},
        { D3D11_USAGE_DEFAULT, 0, D3D11_BIND_SHADER_RESOURCE , 0}, // dynamic doesn't support updateSubResource and map cannot lock just part of resource
        { D3D11_USAGE_DEFAULT, 0, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, D3D11_RESOURCE_MISC_GENERATE_MIPS}
    };

    static ID3D11Resource* createTexture(ID3D11Device * device11, Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, const TextureUsageD3D11& textureUsage)
    {
        ID3D11Resource* result = NULL;

        DXGI_FORMAT format11 = gTextureFormatD3D11[format];
     
        HRESULT hr;
        switch (type)
        {
        case Texture::Type_2D:
        case Texture::Type_Cube:
            {
                D3D11_TEXTURE2D_DESC desc = {};

                desc.Width = width;
                desc.Height = height;
                desc.MipLevels = mipLevels;
                desc.Format = format11;
                desc.SampleDesc.Count = 1;
                desc.SampleDesc.Quality = 0;
                desc.ArraySize = type == Texture::Type_Cube ? 6 : 1;
                desc.Usage = textureUsage.usage;
                desc.BindFlags = textureUsage.bindFlags;
                desc.CPUAccessFlags = textureUsage.cpuAccess;
                desc.MiscFlags = textureUsage.misc | (type == Texture::Type_Cube ? D3D11_RESOURCE_MISC_TEXTURECUBE : 0);

                hr = device11->CreateTexture2D(&desc, NULL, reinterpret_cast<ID3D11Texture2D**>(&result));
                break;
            }
           
        case Texture::Type_3D:
            {
                D3D11_TEXTURE3D_DESC desc;
                desc.Width = width;
                desc.Height = height;
                desc.Depth = depth;
                desc.MipLevels = mipLevels;
                desc.Format = format11;
                desc.Usage = textureUsage.usage;
                desc.BindFlags = textureUsage.bindFlags;
                desc.CPUAccessFlags = textureUsage.cpuAccess;
                desc.MiscFlags = textureUsage.misc;

                hr = device11->CreateTexture3D(&desc, NULL, reinterpret_cast<ID3D11Texture3D**>(&result)); 
                break;
            }
        default:
            RBXASSERT(false);
        }

        if (FAILED(hr))
            throw RBX::runtime_error("Error creating texture: %x", hr);

        return result;
    }

    static ID3D11ShaderResourceView* createSRV(ID3D11Device * device11, ID3D11Resource* resource, Texture::Type type, Texture::Format format, unsigned int mipLevels)
    {
        D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        switch (type)
        {
        case Texture::Type_2D:
            {            
                srvDesc.Format = gTextureFormatD3D11[format];
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Texture2D.MipLevels = mipLevels;
                srvDesc.Texture2D.MostDetailedMip = 0;
                break;
            }
        case Texture::Type_Cube:
            {            
                srvDesc.Format = gTextureFormatD3D11[format];
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;
                srvDesc.TextureCube.MipLevels = mipLevels;
                srvDesc.TextureCube.MostDetailedMip = 0;
                break;
            }
        case Texture::Type_3D:
            {            
                srvDesc.Format = gTextureFormatD3D11[format];
                srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;
                srvDesc.Texture3D.MipLevels = mipLevels;
                srvDesc.Texture3D.MostDetailedMip = 0;
                break;
            }
        }

        ID3D11ShaderResourceView* SRV; 
        HRESULT hr = device11->CreateShaderResourceView(resource, &srvDesc, &SRV);
        
        if (FAILED(hr))
            throw RBX::runtime_error("Error creating shader resource view: %x", hr);

        return SRV;
    }

    TextureD3D11::TextureD3D11(Device* device, Type type, Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Usage usage)
        : Texture(device, type, format, width, height, depth, mipLevels, usage)
        , object(NULL)
        , objectSRV(NULL)
    {
        ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

        object = createTexture(device11, type, format, width, height, depth, mipLevels, gTextureUsageD3D11[usage]);
        objectSRV = createSRV(device11, object, type, format, mipLevels);
    }

    void TextureD3D11::upload(unsigned int index, unsigned int mip, const TextureRegion& region, const void* data, unsigned int size)
    {
        unsigned int mipWidth = getMipSide(width, mip);
        unsigned int mipHeight = getMipSide(height, mip);
        unsigned int mipDepth = getMipSide(depth, mip);

        RBXASSERT(mip < mipLevels);
        RBXASSERT(region.x + region.width <= mipWidth);
        RBXASSERT(region.y + region.height <= mipHeight);
        RBXASSERT(region.z + region.depth <= mipDepth);

        RBXASSERT(size == getImageSize(format, region.width, region.height) * region.depth);

        bool partialUpload = (region.width != mipWidth || region.height != mipHeight || region.depth != mipDepth);
        
        ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

        unsigned int dataRowPitch = Texture::getImageSize(format, region.width, 1);
        unsigned int dataSlicePitch = (type == Type_3D) ? Texture::getImageSize(format, region.width, region.height) : 0;

        D3D11_BOX box = {};
        if (partialUpload)
        {
            box.left = region.x;
            box.right = region.x + region.width;
            box.top = region.y;
            box.bottom = region.y + region.height;
            box.front = region.z;
            box.back = region.z + region.depth;
        }

        UINT res = D3D11CalcSubresource(mip, index, mipLevels);
        context11->UpdateSubresource(object, res, partialUpload ? &box : NULL, data, dataRowPitch, dataSlicePitch);
    }

    static void copyHelper(void* targetData, unsigned int targetRowPitch, unsigned int targetSlicePitch,
        const void* sourceData, unsigned int sourceRowPitch, unsigned int sourceSlicePitch,
        unsigned int width, unsigned int height, unsigned int depth, Texture::Format format)
    {
        unsigned int heightBlocks = Texture::isFormatCompressed(format) ? (height + 3) / 4 : height;
        unsigned int lineSize = Texture::getImageSize(format, width, 1);

        RBXASSERT(lineSize <= sourceRowPitch && lineSize <= targetRowPitch);

        if (targetRowPitch == sourceRowPitch && targetSlicePitch == sourceSlicePitch)
        {
            // Fast path: memory layout is the same
            unsigned int size = (sourceSlicePitch == 0) ? depth * heightBlocks * sourceRowPitch : depth * sourceSlicePitch;

            memcpy(targetData, sourceData, size);
        }
        else
        {
            // Slow path: need to copy line by line
            for (unsigned int z = 0; z < depth; ++z)
                for (unsigned int yb = 0; yb < heightBlocks; ++yb)
                {
                    void* target = static_cast<char*>(targetData) + z * targetSlicePitch + yb * targetRowPitch;
                    const void* source = static_cast<const char*>(sourceData) + z * sourceSlicePitch + yb * sourceRowPitch;

                    memcpy(target, source, lineSize);
                }
        }
    }

    bool TextureD3D11::download(unsigned int index, unsigned int mip, void* data, unsigned int size)
    {
        unsigned int mipWidth = getMipSide(width, mip);
        unsigned int mipHeight = getMipSide(height, mip);
        unsigned int mipDepth = getMipSide(depth, mip);

        RBXASSERT(mip < mipLevels);
        RBXASSERT(size == getImageSize(format, mipWidth, mipHeight) * mipDepth);
        if (type == Type::Type_3D)
            return false;

        DeviceD3D11* device11 = static_cast<DeviceD3D11*>(device);
        ID3D11DeviceContext* context11 = device11->getImmediateContext11();

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = mipWidth;
        desc.Height = mipHeight;
        desc.MipLevels = 1;
        desc.Format = gTextureFormatD3D11[format];
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.ArraySize = 1;
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        desc.MiscFlags = 0;

        ID3D11Texture2D* tempTex = NULL;
        HRESULT hr = device11->getDevice11()->CreateTexture2D(&desc, NULL, reinterpret_cast<ID3D11Texture2D**>(&tempTex));
        if (FAILED(hr))
            throw RBX::runtime_error("Download texture cant create temp texture %x", hr);

        UINT res = D3D11CalcSubresource(mip, index, mipLevels);
        context11->CopySubresourceRegion(tempTex, 0, 0, 0, 0, object, res, NULL);

        // copy texture to provided memory
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = context11->Map(tempTex, 0, D3D11_MAP_READ, 0, &mappedResource);
        RBXASSERT(SUCCEEDED(hr));

        unsigned int dataRowPitch = Texture::getImageSize(format, mipWidth, 1);
        unsigned int dataSlicePitch = (type == Type_3D) ? Texture::getImageSize(format, mipWidth, mipHeight) : 0;

        copyHelper(data, dataRowPitch, dataSlicePitch, mappedResource.pData, mappedResource.RowPitch, 0, mipWidth, mipHeight, mipDepth, format);

        // release all the things
        context11->Unmap(tempTex, 0);
        ReleaseCheck(tempTex);

        return true;
    }

    bool TextureD3D11::supportsLocking() const
    {
         return false;
    }

    Texture::LockResult TextureD3D11::lock(unsigned int index, unsigned int mip, const TextureRegion& region)
    {
         return LockResult();
    }

    void TextureD3D11::unlock(unsigned int index, unsigned int mip)
    {
    }

    shared_ptr<Renderbuffer> TextureD3D11::getRenderbuffer(unsigned int index, unsigned int mip)
    {     
        RBXASSERT(mip < mipLevels);

        weak_ptr<Renderbuffer>& slot = renderbuffers[std::make_pair(index, mip)];
        shared_ptr<Renderbuffer> result = slot.lock();

        if (!result)
        {
            if (getType() != Type_2D && getType() != Type_Cube)
            {
                RBXASSERT(!"Renderbuffer for this kind of texture is not yet implemented."); 
                return shared_ptr<Renderbuffer>();
            }

            result.reset(new RenderbufferD3D11(device, shared_from_this(), index, mip));

            slot = result;
        }

        return result;
    }

    unsigned TextureD3D11::getInternalFormat(Texture::Format format)
    {
        return gTextureFormatD3D11[format];
    }

    void TextureD3D11::commitChanges()
	{
	}

    void TextureD3D11::generateMipmaps()
    {
        RBXASSERT(usage == Texture::Usage_Renderbuffer);

        ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();
        
        context11->GenerateMips(objectSRV);
    }

    TextureD3D11::~TextureD3D11() 
    {
        static_cast<DeviceD3D11*>(device)->getImmediateContextD3D11()->invalidateCachedTexture(this);

        ReleaseCheck(objectSRV);
        ReleaseCheck(object);
    }
}
}
