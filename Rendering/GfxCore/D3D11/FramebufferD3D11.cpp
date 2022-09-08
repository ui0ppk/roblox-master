#include "FramebufferD3D11.h"

#include "TextureD3D11.h"
#include "DeviceD3D11.h"

#include "HeadersD3D11.h"

namespace RBX
{
namespace Graphics
{
    static ID3D11View* createRenderTargetView(ID3D11Device* device11, Texture::Format format, Texture::Type type, ID3D11Resource* texture, unsigned cubeIndex, unsigned int mipIndex, unsigned int samples)
    {
        HRESULT hr = E_FAIL;

        if (Texture::isFormatDepth(format))
        {
			RBXASSERT(type == Texture::Type_2D);

            ID3D11DepthStencilView* depthStencilView = NULL;

			D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};

			if (samples > 1)
			{
				RBXASSERT(mipIndex == 0);
				descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;
			}
			else
			{
				descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
				descDSV.Texture2D.MipSlice = mipIndex;
			}

            descDSV.Flags = 0;
            descDSV.Format = DXGI_FORMAT_UNKNOWN;

            hr = device11->CreateDepthStencilView(texture, &descDSV, &depthStencilView);
            RBXASSERT(SUCCEEDED(hr));

            return depthStencilView;
        }
        else
        {
            D3D11_RENDER_TARGET_VIEW_DESC descRTV = {};

            switch (type)
            {
            case Texture::Type_2D: 
                {
					if (samples > 1)
					{
						RBXASSERT(mipIndex == 0);
						descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
					}
					else
					{
						descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
						descRTV.Texture2D.MipSlice = mipIndex;
					}

                    descRTV.Format = DXGI_FORMAT_UNKNOWN;
                    break;
                }
            case Texture::Type_Cube:
                {
					RBXASSERT(samples == 1);

                    descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;
                    descRTV.Texture2DArray.ArraySize = 1;
                    descRTV.Texture2DArray.FirstArraySlice = cubeIndex;
                    descRTV.Texture2DArray.MipSlice = mipIndex;
                    descRTV.Format = DXGI_FORMAT_UNKNOWN;
                    break;
                }
            default:
                RBXASSERT(false);
                break;
            }

            ID3D11RenderTargetView* renderTargetView = NULL;

            hr = device11->CreateRenderTargetView(texture, &descRTV, &renderTargetView);
            if (FAILED(hr))
                throw RBX::runtime_error("Error creating render target view: %x", hr);

            return renderTargetView;
        }
    }

    ID3D11Texture2D* createRenderTexture(ID3D11Device* device11, unsigned width, unsigned height, unsigned int samples, Texture::Format format)
    {
        bool isDepth = Texture::isFormatDepth(format);

        D3D11_TEXTURE2D_DESC descDepth = {};
        descDepth.Width = width;
        descDepth.Height = height;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = static_cast<DXGI_FORMAT>(TextureD3D11::getInternalFormat(format));
        descDepth.SampleDesc.Count = samples;
        descDepth.SampleDesc.Quality = 0;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = isDepth ? D3D11_BIND_DEPTH_STENCIL : D3D11_BIND_RENDER_TARGET;
        descDepth.CPUAccessFlags = 0;
        descDepth.MiscFlags = 0;

        ID3D11Texture2D* rtTexture = NULL;
        HRESULT hr = device11->CreateTexture2D( &descDepth, NULL, &rtTexture );
        if (FAILED(hr))
            throw RBX::runtime_error("Error creating render target texture: %x", hr);

        return rtTexture;
    }

    RenderbufferD3D11::RenderbufferD3D11(Device* device, const shared_ptr<TextureD3D11>& owner, unsigned cubeIndex, unsigned mipIndex)
        : Renderbuffer(device, owner->getFormat(), owner->getWidth(), owner->getHeight(), 1)
        , object(0)
        , owner(owner)
    {
        ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

        object = createRenderTargetView(device11, owner->getFormat(), owner->getType(), owner->getObject(), cubeIndex, mipIndex, 1);
        
        // Destructor does not Release() the object, no need to AddRef()
        texture = owner->getObject();
    }

    RenderbufferD3D11::RenderbufferD3D11(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples, ID3D11Texture2D* texture)
        : Renderbuffer(device, format, width, height, samples)
        , object(0)
        , texture(texture)
    {
        ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

		if (Texture::isFormatDepth(format))
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
			descDSV.Format = static_cast<DXGI_FORMAT>(TextureD3D11::getInternalFormat(format));
			descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

			ID3D11DepthStencilView* depthStencilView = NULL;
			HRESULT hr = device11->CreateDepthStencilView(texture, &descDSV, &depthStencilView);
			RBXASSERT(SUCCEEDED(hr));

			object = depthStencilView;
		}
		else
		{
			D3D11_RENDER_TARGET_VIEW_DESC descRTV = {};
			descRTV.Format = static_cast<DXGI_FORMAT>(TextureD3D11::getInternalFormat(format));
			descRTV.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

			ID3D11RenderTargetView* renderTargetView = NULL;
			HRESULT hr = device11->CreateRenderTargetView(texture, &descRTV, &renderTargetView);
			RBXASSERT(SUCCEEDED(hr));

			object = renderTargetView;
		}
    }

    RenderbufferD3D11::RenderbufferD3D11(Device* device, Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
        : Renderbuffer(device, format, width, height, samples)
    {
        ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

        texture = createRenderTexture(device11, width, height, samples, format);
        
        object = createRenderTargetView(device11, format, Texture::Type_2D, texture, 0, 0, samples);
    }

    RenderbufferD3D11::~RenderbufferD3D11()
    {
        ReleaseCheck(object);
        if (!owner)
            ReleaseCheck(texture);
    }

    FramebufferD3D11::FramebufferD3D11(Device* device, const std::vector<shared_ptr<Renderbuffer>>& color, const shared_ptr<Renderbuffer>& depth)
        : Framebuffer(device, 0, 0, 0)
        , color(color)
        , depth(depth)
    {
        RBXASSERT(!color.empty());

        if (color.size() > device->getCaps().maxDrawBuffers)
            throw RBX::runtime_error("Unsupported framebuffer configuration: too many buffers (%d)", color.size());

        for (size_t i = 0; i < color.size(); ++i)
        {
            RenderbufferD3D11* buffer = static_cast<RenderbufferD3D11*>(color[i].get());
            RBXASSERT(buffer);
            RBXASSERT(!Texture::isFormatDepth(buffer->getFormat()));

            if (i == 0)
            {
                width = buffer->getWidth();
                height = buffer->getHeight();
                samples = buffer->getSamples();
            }
            else
            {
                RBXASSERT(width == buffer->getWidth());
                RBXASSERT(height == buffer->getHeight());
                RBXASSERT(samples == buffer->getSamples());
            }
        }

        if (depth)
        {
            RenderbufferD3D11* buffer = static_cast<RenderbufferD3D11*>(depth.get());
            RBXASSERT(Texture::isFormatDepth(buffer->getFormat()));

            RBXASSERT(width == buffer->getWidth());
            RBXASSERT(height == buffer->getHeight());
            RBXASSERT(samples == buffer->getSamples());
        }
    }

    void FramebufferD3D11::download(void* data, unsigned int size)
    {
        RBXASSERT(size == width * height * 4);

        DeviceD3D11* device11 = static_cast<DeviceD3D11*>(device);
        ID3D11DeviceContext* context11 = device11->getImmediateContext11();

        D3D11_TEXTURE2D_DESC desc = {};
        desc.Width = width;
        desc.Height = height;
        desc.MipLevels = 1;
        desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
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
            throw RBX::runtime_error("Download frame buffer cant create temp texture %x", hr);

        // get resource bound to view
        ID3D11Resource* resource;
        RenderbufferD3D11* color0 = static_cast<RenderbufferD3D11*>(color[0].get());
        color0->getObject()->GetResource(&resource);
        RBXASSERT(color0->getFormat() == Texture::Format_RGBA8);

        // copy resource to tex
        context11->CopyResource(tempTex, resource);

        // copy texture to provided memory
        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = context11->Map(tempTex, 0, D3D11_MAP_READ, 0, &mappedResource);
        RBXASSERT(SUCCEEDED(hr));

        for (unsigned int y = 0; y < height; ++y)
        {
            char* dataRow = static_cast<char*>(data) + y * width * 4;
            memcpy(dataRow, static_cast<char*>(mappedResource.pData) + y * mappedResource.RowPitch, width * 4);
        }

        // release all the stuff
        context11->Unmap(tempTex, 0);
        resource->Release();
        ReleaseCheck(tempTex);
    }

    FramebufferD3D11::~FramebufferD3D11()
    {
    }
}
}
