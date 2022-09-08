#include "DeviceD3D11.h"

#include "GeometryD3D11.h"
#include "ShaderD3D11.h"
#include "TextureD3D11.h"
#include "FramebufferD3D11.h"

#include "HeadersD3D11.h"

FASTFLAG(GraphicsDebugMarkersEnable)

namespace RBX
{
namespace Graphics
{
    static const D3D11_CULL_MODE gCullModeD3D11[RasterizerState::Cull_Count] =
    {
        D3D11_CULL_NONE,
        D3D11_CULL_BACK,
        D3D11_CULL_FRONT
    };

    struct BlendFuncD3D11
    {
        D3D11_BLEND src, dst;
    };

	static const D3D11_BLEND gBlendFactors[BlendState::Factor_Count] =
	{
		D3D11_BLEND_ONE, 
		D3D11_BLEND_ZERO,
		D3D11_BLEND_DEST_COLOR,
		D3D11_BLEND_SRC_ALPHA,
		D3D11_BLEND_INV_SRC_ALPHA,
		D3D11_BLEND_DEST_ALPHA,
		D3D11_BLEND_INV_DEST_ALPHA
	};

    static const D3D11_COMPARISON_FUNC gDepthFuncD3D11[DepthState::Function_Count] =
    {
        D3D11_COMPARISON_ALWAYS,
        D3D11_COMPARISON_LESS,
        D3D11_COMPARISON_LESS_EQUAL
    };

    static const D3D11_FILTER gSamplerFilterD3D11[SamplerState::Filter_Count] =
    {
        D3D11_FILTER_MIN_MAG_MIP_POINT,
        D3D11_FILTER_MIN_MAG_MIP_LINEAR,
        D3D11_FILTER_ANISOTROPIC,
    };

    static const D3D11_TEXTURE_ADDRESS_MODE gSamplerAddressD3D11[SamplerState::Address_Count] =
    {
        D3D11_TEXTURE_ADDRESS_WRAP,
        D3D11_TEXTURE_ADDRESS_CLAMP
    };

    DeviceContextD3D11::DeviceContextD3D11(Device* device, ID3D11DeviceContext* deviceContext11)
        : device(device)
        , device11(static_cast<DeviceD3D11*>(device)->getDevice11())
        , globalDataSize(0)
        , defaultAnisotropy(1)
        , cachedProgram(NULL)
        , cachedVertexLayout(NULL)
        , cachedGeometry(NULL)
        , cachedFramebuffer(NULL)
        , cachedRasterizerState(RasterizerState::Cull_None)
        , cachedBlendState(BlendState::Mode_None)
        , cachedDepthState(DepthState::Function_Always, false)
        , globalsConstantBuffer(NULL)
        , d3d9(NULL)
    {
        immediateContext11 = deviceContext11;

        // markers, we are going oldschool here. It works for RenderDoc too
        pfn_D3DPERF_BeginEvent = 0;
        pfn_D3DPERF_EndEvent = 0;
        pfn_D3DPERF_SetMarker = 0;

        DWORD (WINAPI *pfn_D3DPERF_GetStatus)() = 0;

        if(PIX_ENABLED)
        {
            d3d9 = LoadLibraryW(L"d3d9.dll");
            if (d3d9)
            {
                (void*&)pfn_D3DPERF_BeginEvent = GetProcAddress(d3d9, "D3DPERF_BeginEvent");
                (void*&)pfn_D3DPERF_EndEvent   = GetProcAddress(d3d9, "D3DPERF_EndEvent");
                (void*&)pfn_D3DPERF_SetMarker  = GetProcAddress(d3d9, "D3DPERF_SetMarker");
                (void*&)pfn_D3DPERF_GetStatus  = GetProcAddress(d3d9, "D3DPERF_GetStatus");
            }
        }

        if( !pfn_D3DPERF_GetStatus || !pfn_D3DPERF_GetStatus() )
        {
            FFlag::GraphicsDebugMarkersEnable = false; // no use
        }
    }

    DeviceContextD3D11::~DeviceContextD3D11()
    {
        if (d3d9)
        {
            FreeLibrary(d3d9);
            d3d9 = NULL;
        }

        ReleaseCheck(immediateContext11);
        ReleaseCheck(globalsConstantBuffer);

        for (RasterizerStateHash::iterator it = rasterizerStateHash.begin(); it != rasterizerStateHash.end(); ++it)
            ReleaseCheck(it->second);
        for (BlendStateHash::iterator it = blendStateHash.begin(); it != blendStateHash.end(); ++it)
            ReleaseCheck(it->second);
        for (DepthStateHash::iterator it = depthStateHash.begin(); it != depthStateHash.end(); ++it)
            ReleaseCheck(it->second);
        for (SamplerStateHash::iterator it = samplerStateHash.begin(); it != samplerStateHash.end(); ++it)
            ReleaseCheck(it->second);

        rasterizerStateHash.clear();
        samplerStateHash.clear();
        depthStateHash.clear();
        blendStateHash.clear();
    }

    void DeviceContextD3D11::defineGlobalConstants(size_t dataSize)
    {
        RBXASSERT(globalsConstantBuffer == NULL);
        if (globalsConstantBuffer)
            return;

        D3D11_BUFFER_DESC bd = {};
        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = dataSize;
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        bd.StructureByteStride = 0;
        bd.MiscFlags = 0;

        globalDataSize = dataSize;

        HRESULT hr = device11->CreateBuffer( &bd, NULL, &globalsConstantBuffer );
        RBXASSERT(SUCCEEDED(hr));
    }

    void DeviceContextD3D11::updateGlobalConstants(const void* data, size_t dataSize)
    {
        RBXASSERT(dataSize == globalDataSize);

        immediateContext11->UpdateSubresource(globalsConstantBuffer, 0, NULL, data, 0, 0);

        immediateContext11->VSSetConstantBuffers( 0, 1, &globalsConstantBuffer);
        immediateContext11->PSSetConstantBuffers( 0, 1, &globalsConstantBuffer);
    }

    void DeviceContextD3D11::bindFramebuffer(Framebuffer* buffer)
    {
        FramebufferD3D11* buffer11 = static_cast<FramebufferD3D11*>(buffer);
        
        const std::vector<shared_ptr<Renderbuffer> >& color = buffer11->getColor();
        const shared_ptr<Renderbuffer>& depth = buffer11->getDepth();

        ID3D11RenderTargetView* rtArray[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT];
        for(size_t i = 0; i < color.size(); ++i)
        {
            RenderbufferD3D11* rb = static_cast<RenderbufferD3D11*>(color[i].get());
            rtArray[i] = static_cast<ID3D11RenderTargetView*>(rb->getObject());

            // unbind texture if bound as SRV
            const shared_ptr<TextureD3D11>& ownerTex = rb->getOwner();

            if (ownerTex)
            {
                for (size_t i = 0; i < ARRAYSIZE(cachedTextureUnits); ++i)
                {
                    TextureUnit& u = cachedTextureUnits[i];
                    if (u.texture == ownerTex.get())
                    {
                        ID3D11ShaderResourceView* nullSRV = NULL;
                        immediateContext11->PSSetShaderResources(i, 1, &nullSRV);
                        u.texture = NULL;
                    }
                }
            }
        }

        RenderbufferD3D11* dsBuffer = static_cast<RenderbufferD3D11*>(depth.get());
        ID3D11DepthStencilView* dsv = dsBuffer ? static_cast<ID3D11DepthStencilView*>(dsBuffer->getObject()) : NULL;

        immediateContext11->OMSetRenderTargets( color.size(), rtArray, dsv);

        cachedFramebuffer = buffer;

        D3D11_VIEWPORT vp = {};
        vp.Width = (FLOAT)buffer->getWidth();
        vp.Height = (FLOAT)buffer->getHeight();
        vp.MinDepth = 0.0f;
        vp.MaxDepth = 1.0f;
        vp.TopLeftX = 0;
        vp.TopLeftY = 0;
        immediateContext11->RSSetViewports( 1, &vp );
    }

    void DeviceContextD3D11::setWorldTransforms4x3(const float* data, size_t matrixCount)
    {
        RBXASSERT(cachedProgram);

        cachedProgram->setWorldTransforms4x3(data, matrixCount);
    }

    void DeviceContextD3D11::setConstant(int handle, const float* data, size_t vectorCount)
    {
        RBXASSERT(cachedProgram);

        cachedProgram->setConstant(handle, data, vectorCount);
    }

    void DeviceContextD3D11::setRasterizerState(const RasterizerState& state)
    {
        bool depthBiasSupported = static_cast<DeviceD3D11*>(device)->getShaderProfile() == DeviceD3D11::shaderProfile_DX11;
        
        RasterizerState realState = state;
        if (!depthBiasSupported)
            realState = RasterizerState(state.getCullMode(), 0);
        
        if (realState != cachedRasterizerState)
        {
            ID3D11RasterizerState* state11 = rasterizerStateHash[realState];

            if (!state11)
            {
                D3D11_RASTERIZER_DESC rastStateDesc = {};

                float slopeBias = static_cast<float>(realState.getDepthBias()) / 32.f; // do we need explicit control over slope or just a better formula since these numbers are magic anyway?

                rastStateDesc.FillMode = D3D11_FILL_SOLID;
                rastStateDesc.CullMode = gCullModeD3D11[realState.getCullMode()];
                rastStateDesc.FrontCounterClockwise = true;
                rastStateDesc.DepthBias = realState.getDepthBias();;
                rastStateDesc.DepthBiasClamp = 0;
                rastStateDesc.SlopeScaledDepthBias = slopeBias;
                rastStateDesc.DepthClipEnable = true;
                rastStateDesc.ScissorEnable = false;
                rastStateDesc.MultisampleEnable = false;
                rastStateDesc.AntialiasedLineEnable = false;

                HRESULT hr = device11->CreateRasterizerState( &rastStateDesc, &state11);
                RBXASSERT(SUCCEEDED(hr));
            
                checkDuplicates(rasterizerStateHash, state11);
                rasterizerStateHash[realState] = state11;
            }

            immediateContext11->RSSetState(state11);
            cachedRasterizerState = realState;
        }
    }

    void DeviceContextD3D11::setBlendState(const BlendState& state)
    {
        if (cachedBlendState != state)
        {
            ID3D11BlendState* state11 = blendStateHash[state];

            if (!state11)
            {
                D3D11_BLEND_DESC blendStateDesc = {};

                blendStateDesc.AlphaToCoverageEnable = false;
                blendStateDesc.IndependentBlendEnable = false;

                if (!state.blendingNeeded())
                {
                    blendStateDesc.RenderTarget[0].BlendEnable = false;
                }
                else
                {
                    blendStateDesc.RenderTarget[0].BlendEnable = true;
                    blendStateDesc.RenderTarget[0].SrcBlend = gBlendFactors[state.getColorSrc()];
                    blendStateDesc.RenderTarget[0].SrcBlendAlpha = gBlendFactors[state.getAlphaSrc()];
                    blendStateDesc.RenderTarget[0].DestBlend = gBlendFactors[state.getColorDst()];
                    blendStateDesc.RenderTarget[0].DestBlendAlpha = gBlendFactors[state.getAlphaDst()];
                    blendStateDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
                    blendStateDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
                }

                unsigned int colorMask = 0;

                if (state.getColorMask() & BlendState::Color_R)
                    colorMask |= D3D11_COLOR_WRITE_ENABLE_RED;

                if (state.getColorMask() & BlendState::Color_G)
                    colorMask |= D3D11_COLOR_WRITE_ENABLE_GREEN;

                if (state.getColorMask() & BlendState::Color_B)
                    colorMask |= D3D11_COLOR_WRITE_ENABLE_BLUE;

                if (state.getColorMask() & BlendState::Color_A)
                    colorMask |= D3D11_COLOR_WRITE_ENABLE_ALPHA;

                blendStateDesc.RenderTarget[0].RenderTargetWriteMask = colorMask;
                    
                HRESULT hr = device11->CreateBlendState(&blendStateDesc, &state11);
                RBXASSERT(SUCCEEDED(hr));

                checkDuplicates(blendStateHash, state11);
                blendStateHash[state] = state11;
            }

            float blendFactor[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
            UINT sampleMask = 0xffffffff;
            
            immediateContext11->OMSetBlendState(state11, blendFactor, sampleMask);
            cachedBlendState = state;
        }
    }

    void DeviceContextD3D11::setDepthState(const DepthState& state)
    {
        if (cachedDepthState != state)
        {
            ID3D11DepthStencilState* state11 = depthStateHash[state];

            if (!state11)
            {
                D3D11_DEPTH_STENCIL_DESC dsDesc = {};

                if (state.getFunction() == DepthState::Function_Always && state.getWrite() == false)
                {
                    dsDesc.DepthEnable = false;
                }
                else
                {
                    dsDesc.DepthEnable = true;
                    dsDesc.DepthFunc = gDepthFuncD3D11[state.getFunction()];
                    dsDesc.DepthWriteMask = state.getWrite() ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
                }

                dsDesc.StencilReadMask = 0xFF;
                dsDesc.StencilWriteMask = 0xFF;

                switch (state.getStencilMode())
                {
                case DepthState::Stencil_None:
                    dsDesc.StencilEnable = false;
                    break;

                case DepthState::Stencil_IsNotZero:
                    dsDesc.StencilEnable = true;
                    
                    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
                    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
                    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
                    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;

                    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_NOT_EQUAL;
                    dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
                    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
                    dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
                    break;

                case DepthState::Stencil_UpdateZFail:
                    dsDesc.StencilEnable = true;

                    dsDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
                    dsDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
                    dsDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
                    dsDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;

                    dsDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
                    dsDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
                    dsDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
                    dsDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
                    break;
                }

                HRESULT hr = device11->CreateDepthStencilState(&dsDesc, &state11);
                RBXASSERT(SUCCEEDED(hr));

                checkDuplicates(depthStateHash, state11);
                depthStateHash[state] = state11;   
            }

            immediateContext11->OMSetDepthStencilState(state11, 0);
            cachedDepthState = state;
        }
    }

    void DeviceContextD3D11::clearStates()
    {
        // Clear framebuffer cache
        cachedFramebuffer = NULL;

        // Clear program cache
        cachedProgram = NULL;

        // Clear vertex layout cache
        cachedVertexLayout = NULL;

        // Clear geometry cache
        cachedGeometry = NULL;

        // Clear texture cache
        for (size_t i = 0; i < ARRAYSIZE(cachedTextureUnits); ++i)
        {
            TextureUnit& u = cachedTextureUnits[i];

            u.texture = NULL;

            // Clear state to an invalid value to guarantee a cache miss on the next setup
            u.samplerState = SamplerState::Filter_Count;
        }

        // unset all textures (SRVs) from shader
        ID3D11ShaderResourceView* nullSRVs[ARRAYSIZE(cachedTextureUnits)] = {};

        immediateContext11->PSSetShaderResources(0, ARRAYSIZE(cachedTextureUnits), nullSRVs);

        // Clear states to invalid values to guarantee a cache miss on the next setup
        cachedRasterizerState = RasterizerState(RasterizerState::Cull_Count);
        cachedBlendState = BlendState(BlendState::Mode_Count);
        cachedDepthState = DepthState(DepthState::Function_Count, false);
    }

    void DeviceContextD3D11::setDefaultAnisotropy(unsigned int value)
    {
        defaultAnisotropy = value;
    }

    void DeviceContextD3D11::bindTexture(unsigned int stage, Texture* texture, const SamplerState& state)
    {
        SamplerState realState =
            (state.getFilter() == SamplerState::Filter_Anisotropic && state.getAnisotropy() == 0)
            ? SamplerState(state.getFilter(), state.getAddress(), defaultAnisotropy)
            : state;

		RBXASSERT(stage < device->getCaps().maxTextureUnits);
        RBXASSERT(stage < ARRAYSIZE(cachedTextureUnits));

        TextureUnit& u = cachedTextureUnits[stage];

        TextureD3D11* texture11 = static_cast<TextureD3D11*>(texture);
        if (u.texture != texture11)
        {
            ID3D11ShaderResourceView* srv = texture11->getSRV();
            immediateContext11->PSSetShaderResources(stage, 1, &srv);

            u.texture = texture11;
        }
        
        if (u.samplerState != realState)
        {
            
            ID3D11SamplerState* samplerState = samplerStateHash[realState];

            if (!samplerState)
            {
                D3D11_SAMPLER_DESC samplerDesc = {};

                samplerDesc.AddressU = gSamplerAddressD3D11[realState.getAddress()];
                samplerDesc.AddressV = gSamplerAddressD3D11[realState.getAddress()];
                samplerDesc.AddressW = gSamplerAddressD3D11[realState.getAddress()];
                samplerDesc.Filter = gSamplerFilterD3D11[realState.getFilter()];
                samplerDesc.MaxAnisotropy = realState.getAnisotropy();
                samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
                samplerDesc.MaxLOD = FLT_MAX;

                HRESULT hr = device11->CreateSamplerState(&samplerDesc, &samplerState);
                RBXASSERT(SUCCEEDED(hr));

                checkDuplicates(samplerStateHash, samplerState);
                samplerStateHash[realState] = samplerState;
            }

            immediateContext11->PSSetSamplers(stage, 1, &samplerState);
            u.samplerState = realState;
        }
    }

    void DeviceContextD3D11::drawImpl(Geometry* geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd)
    {
        static_cast<GeometryD3D11*>(geometry)->draw(primitive, offset, count, indexRangeBegin, indexRangeEnd, &cachedVertexLayout, &cachedGeometry, &cachedProgram);
    }

    void DeviceContextD3D11::bindProgram(ShaderProgram* program)
    {
        if (cachedProgram != program)
        {
            cachedProgram = static_cast<ShaderProgramD3D11*>(program);
            cachedProgram->bind();
            cachedVertexLayout = NULL; // VertexLayout is per vertex shader
        }
    }

    ID3D11DeviceContext* DeviceContextD3D11::getContextDX11()
    { 
        return immediateContext11; 
    }

    void DeviceContextD3D11::clearFramebuffer(unsigned int mask, const float color[4], float depth, unsigned int stencil)
    {
        RBXASSERT(cachedFramebuffer);

        FramebufferD3D11* buffer11 = static_cast<FramebufferD3D11*>(cachedFramebuffer);

        if (mask & Buffer_Color)
        {
            const std::vector<shared_ptr<Renderbuffer> >& colorBuf = buffer11->getColor();
            for(size_t i = 0; i < colorBuf.size(); ++i)
            {
                RenderbufferD3D11* rb = static_cast<RenderbufferD3D11*>(colorBuf[i].get());
                ID3D11RenderTargetView* rtv = static_cast<ID3D11RenderTargetView*>(rb->getObject());
                immediateContext11->ClearRenderTargetView(rtv, color);
            }
        }

        if (mask & (Buffer_Depth | Buffer_Stencil))
        {
            unsigned int flags = 0;
            if (mask & Buffer_Depth)
                flags |= D3D11_CLEAR_DEPTH;

            if (mask & Buffer_Stencil)
                flags |= D3D11_CLEAR_STENCIL;

            RenderbufferD3D11* dsBuffer = static_cast<RenderbufferD3D11*>(buffer11->getDepth().get());
            ID3D11DepthStencilView* dsv = static_cast<ID3D11DepthStencilView*>(dsBuffer->getObject());

            immediateContext11->ClearDepthStencilView(dsv, flags, depth, stencil);
        }
    }

    void DeviceContextD3D11::copyFramebuffer(Framebuffer* buffer, Texture* texture)
    {
        FramebufferD3D11* fb = static_cast<FramebufferD3D11*>(buffer);        
        RBXASSERT(fb->getColor().size() > 0);
        RenderbufferD3D11* color0 = static_cast<RenderbufferD3D11*>(fb->getColor()[0].get());

        RBXASSERT(texture->getType() == Texture::Type_2D);
        RBXASSERT(texture->getMipLevels() == 1);
        RBXASSERT(color0->getWidth() == texture->getWidth() && color0->getHeight() == texture->getHeight());

        ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

        context11->CopyResource(static_cast<TextureD3D11*>(texture)->getObject(), color0->getResource());
    }

    void DeviceContextD3D11::resolveFramebuffer(Framebuffer* msaaBuffer, Framebuffer* buffer, unsigned int mask)
	{
		RBXASSERT(msaaBuffer->getSamples() > 1);
		RBXASSERT(buffer->getSamples() == 1);
		RBXASSERT(msaaBuffer->getWidth() == buffer->getWidth() && msaaBuffer->getHeight() == buffer->getHeight());

		FramebufferD3D11* msaaBuffer11 = static_cast<FramebufferD3D11*>(msaaBuffer);
		FramebufferD3D11* buffer11 = static_cast<FramebufferD3D11*>(buffer);

        ID3D11DeviceContext* context11 = static_cast<DeviceD3D11*>(device)->getImmediateContext11();

		if (mask & Buffer_Color)
		{
			RBXASSERT(msaaBuffer11->getColor().size() > 0);
			RBXASSERT(msaaBuffer11->getColor().size() == buffer11->getColor().size());

			for (size_t i = 0; i < msaaBuffer11->getColor().size(); ++i)
			{
				RenderbufferD3D11* msaaColor = static_cast<RenderbufferD3D11*>(msaaBuffer11->getColor()[i].get());
				RenderbufferD3D11* color = static_cast<RenderbufferD3D11*>(buffer11->getColor()[i].get());

				RBXASSERT(msaaColor->getFormat() == color->getFormat());

				context11->ResolveSubresource(color->getResource(), 0, msaaColor->getResource(), 0, static_cast<DXGI_FORMAT>(TextureD3D11::getInternalFormat(msaaColor->getFormat())));
			}
		}

		if (mask & (Buffer_Depth | Buffer_Stencil))
		{
			RBXASSERT(msaaBuffer11->getDepth() && buffer11->getDepth());

			RenderbufferD3D11* msaaDepth = static_cast<RenderbufferD3D11*>(msaaBuffer11->getDepth().get());
			RenderbufferD3D11* depth = static_cast<RenderbufferD3D11*>(buffer11->getDepth().get());

			RBXASSERT(msaaDepth->getFormat() == depth->getFormat());

			context11->ResolveSubresource(depth->getResource(), 0, msaaDepth->getResource(), 0, static_cast<DXGI_FORMAT>(TextureD3D11::getInternalFormat(msaaDepth->getFormat())));
		}
	}

    void DeviceContextD3D11::discardFramebuffer(Framebuffer* buffer, unsigned int mask)
	{
	}

    void DeviceContextD3D11::invalidateCachedGeometry()
    {
        cachedGeometry = NULL;
    }

    void DeviceContextD3D11::invalidateCachedProgram()
    {
        cachedProgram = NULL;
    }

    void DeviceContextD3D11::invalidateCachedVertexLayout()
    {
        cachedVertexLayout = NULL;
    }

    void DeviceContextD3D11::invalidateCachedTexture(Texture* texture)
    {
        for (unsigned int stage = 0; stage < ARRAYSIZE(cachedTextureUnits); ++stage)
        {
            TextureUnit& u = cachedTextureUnits[stage];

            if (u.texture == texture)
                u.texture = NULL;
        }
    }

    static void ascii2unicode(wchar_t* dest, const char* src, int max)
    {
        while( (*dest++ = *src++) && max--) {}
    }

    void DeviceContextD3D11::pushDebugMarkerGroup(const char* text)
    {
        if (pfn_D3DPERF_BeginEvent)
        {
            wchar_t buffer[512];
            ascii2unicode(buffer, text, sizeof(buffer)/sizeof(buffer[0]) );
            pfn_D3DPERF_BeginEvent(0, buffer);
        }
    }

    void DeviceContextD3D11::popDebugMarkerGroup()
    {
        if (pfn_D3DPERF_EndEvent)
        {
            pfn_D3DPERF_EndEvent();
        }
    }

    void DeviceContextD3D11::setDebugMarker(const char* text)
    {
        if (pfn_D3DPERF_SetMarker)
        {
            wchar_t buffer[512];
            ascii2unicode(buffer, text, sizeof(buffer)/sizeof(buffer[0]));
            pfn_D3DPERF_SetMarker(0, buffer);
        }
    }


    void DeviceContextD3D11::beginQuery(ID3D11Query* query)
    {
        immediateContext11->Begin(query);
    }

    void DeviceContextD3D11::endQuery(ID3D11Query* query)
    {
        immediateContext11->End(query);
    }

    bool DeviceContextD3D11::getQueryData(ID3D11Query* query, void* dataOut, size_t dataSize)
    {
        HRESULT hr = immediateContext11->GetData(query, dataOut, dataSize, D3D11_ASYNC_GETDATA_DONOTFLUSH);
        return hr == S_OK;
    }
}
}
