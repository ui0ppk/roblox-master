#include "DeviceD3D9.h"

#include "GeometryD3D9.h"
#include "ShaderD3D9.h"
#include "TextureD3D9.h"
#include "FramebufferD3D9.h"
#include "D3dx9math.h"

#include <d3d9.h>
#include "GfxCore/pix.h"

FASTFLAG(GraphicsDebugMarkersEnable)

namespace RBX
{
namespace Graphics
{

static const D3DCULL gCullModeD3D9[RasterizerState::Cull_Count] =
{
    D3DCULL_NONE,
    D3DCULL_CW,
    D3DCULL_CCW
};

static const D3DBLEND gBlendFactorsD3D9[BlendState::Factor_Count] =
{
	D3DBLEND_ONE, 
	D3DBLEND_ZERO,
	D3DBLEND_DESTCOLOR,
	D3DBLEND_SRCALPHA,
	D3DBLEND_INVSRCALPHA,
	D3DBLEND_DESTALPHA,
	D3DBLEND_INVDESTALPHA
};

static const D3DCMPFUNC gDepthFuncD3D9[DepthState::Function_Count] =
{
	D3DCMP_ALWAYS,
	D3DCMP_LESS,
	D3DCMP_LESSEQUAL
};

struct TextureFilterD3D9
{
    D3DTEXTUREFILTERTYPE min, mag, mip;
};

static const TextureFilterD3D9 gSamplerFilterD3D9[SamplerState::Filter_Count] =
{
	{ D3DTEXF_POINT, D3DTEXF_POINT, D3DTEXF_POINT },
	{ D3DTEXF_LINEAR, D3DTEXF_LINEAR, D3DTEXF_LINEAR },
	{ D3DTEXF_ANISOTROPIC, D3DTEXF_LINEAR, D3DTEXF_LINEAR },
};

static const D3DTEXTUREADDRESS gSamplerAddressD3D9[SamplerState::Address_Count] =
{
	D3DTADDRESS_WRAP,
	D3DTADDRESS_CLAMP
};

template <typename Map> static const typename Map::mapped_type* findKey(const Map& map, const typename Map::key_type& key)
{
    typename Map::const_iterator it = map.find(key);
    return (it == map.end()) ? NULL : &it->second;
}

static void transposeMatrix(float* dest, const float* source, const float* lastColumn)
{
    dest[0] = source[0]; dest[1] = source[4]; dest[2] = source[8]; dest[3] = lastColumn[0];
    dest[4] = source[1]; dest[5] = source[5]; dest[6] = source[9]; dest[7] = lastColumn[1];
    dest[8] = source[2]; dest[9] = source[6]; dest[10] = source[10]; dest[11] = lastColumn[2];
    dest[12] = source[3]; dest[13] = source[7]; dest[14] = source[11]; dest[15] = lastColumn[3];
}

DeviceContextD3D9::DeviceContextD3D9(Device* device)
    : device(device)
	, device9(static_cast<DeviceD3D9*>(device)->getDevice9())
	, globalDataSize(0)
	, defaultAnisotropy(1)
    , cachedFramebufferSurfaces(0)
    , cachedProgram(NULL)
	, cachedVertexLayout(NULL)
	, cachedGeometry(NULL)
	, cachedRasterizerState(RasterizerState::Cull_None)
    , cachedBlendState(BlendState::Mode_None)
	, cachedDepthState(DepthState::Function_Always, false)
    , d3d9(NULL)
{
    pfn_D3DPERF_BeginEvent = 0;
    pfn_D3DPERF_EndEvent = 0;
    pfn_D3DPERF_SetMarker = 0;

    DWORD (WINAPI *pfn_D3DPERF_GetStatus)();

    if(PIX_ENABLED)
    {
        d3d9 = LoadLibraryA("d3d9.dll");
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

DeviceContextD3D9::~DeviceContextD3D9()
{
    if (d3d9)
    {
        FreeLibrary(d3d9);
        d3d9 = NULL;
    }
}

void DeviceContextD3D9::defineGlobalConstants(size_t dataSize)
{
	RBXASSERT(globalDataSize == 0);
    RBXASSERT(dataSize > 0);

	globalDataSize = dataSize;
}

void DeviceContextD3D9::clearStates()
{
    const DeviceCapsD3D9& caps = static_cast<DeviceD3D9*>(device)->getCapsD3D9();

    // Clear framebuffer cache
	cachedFramebufferSurfaces = 0;

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

    // Clear states to invalid values to guarantee a cache miss on the next setup
	cachedRasterizerState = RasterizerState(RasterizerState::Cull_Count);
    cachedBlendState = BlendState(BlendState::Mode_Count);
	cachedDepthState = DepthState(DepthState::Function_Count, false);
}

void DeviceContextD3D9::invalidateCachedProgram()
{
    cachedProgram = NULL;
}

void DeviceContextD3D9::invalidateCachedVertexLayout()
{
    cachedVertexLayout = NULL;
}

void DeviceContextD3D9::invalidateCachedGeometry()
{
    cachedGeometry = NULL;
}

void DeviceContextD3D9::invalidateCachedTexture(Texture* texture)
{
	for (unsigned int stage = 0; stage < ARRAYSIZE(cachedTextureUnits); ++stage)
	{
		TextureUnit& u = cachedTextureUnits[stage];

        if (u.texture == texture)
			u.texture = NULL;
	}
}

void DeviceContextD3D9::setDefaultAnisotropy(unsigned int value)
{
    defaultAnisotropy = value;
}

void DeviceContextD3D9::updateGlobalConstants(const void* data, size_t dataSize)
{
    RBXASSERT(dataSize == globalDataSize);

    device9->SetVertexShaderConstantF(0, static_cast<const float*>(data), globalDataSize / 16);
    device9->SetPixelShaderConstantF(0, static_cast<const float*>(data), globalDataSize / 16);
}

void DeviceContextD3D9::bindFramebuffer(Framebuffer* buffer)
{
	FramebufferD3D9* buffer9 = static_cast<FramebufferD3D9*>(buffer);

	const std::vector<shared_ptr<Renderbuffer> >& color = buffer9->getColor();
	const shared_ptr<Renderbuffer>& depth = buffer9->getDepth();

	for (size_t i = 1; i < cachedFramebufferSurfaces; ++i)
        device9->SetRenderTarget(i, NULL);

    for (size_t i = 0; i < color.size(); ++i)
		device9->SetRenderTarget(i, static_cast<RenderbufferD3D9*>(color[i].get())->getObject());

	device9->SetDepthStencilSurface(depth ? static_cast<RenderbufferD3D9*>(depth.get())->getObject() : NULL);

	cachedFramebufferSurfaces = color.size();
}

static int colorComponentToInt(float value)
{
    return static_cast<int>(std::max(std::min(value, 1.f), 0.f) * 255.f + 0.5f);
}

static unsigned int colorToD3D(const float color[4])
{
	return D3DCOLOR_RGBA(colorComponentToInt(color[0]), colorComponentToInt(color[1]), colorComponentToInt(color[2]), colorComponentToInt(color[3]));
}

void DeviceContextD3D9::clearFramebuffer(unsigned int mask, const float color[4], float depth, unsigned int stencil)
{
    unsigned int flags = 0;

	if (mask & Buffer_Color)
		flags |= D3DCLEAR_TARGET;

	if (mask & Buffer_Depth)
		flags |= D3DCLEAR_ZBUFFER;

	if (mask & Buffer_Stencil)
		flags |= D3DCLEAR_STENCIL;

	device9->Clear(0, NULL, flags, (mask & Buffer_Color) ? colorToD3D(color) : 0, depth, stencil);
}

void DeviceContextD3D9::copyFramebuffer(Framebuffer* buffer, Texture* texture)
{
	RBXASSERT(texture->getType() == Texture::Type_2D);
	RBXASSERT(texture->getMipLevels() == 1);
	RBXASSERT(buffer->getWidth() == texture->getWidth() && buffer->getHeight() == texture->getHeight());

	IDirect3DSurface9* tempSurface = static_cast<FramebufferD3D9*>(buffer)->grabCopy();
    IDirect3DTexture9* textureObject = static_cast<IDirect3DTexture9*>(static_cast<TextureD3D9*>(texture)->getObject());

    D3DSURFACE_DESC surfaceDesc;
    tempSurface->GetDesc(&surfaceDesc);

    D3DSURFACE_DESC textureDesc;
    textureObject->GetLevelDesc(0, &textureDesc);

	RBXASSERT(textureDesc.Format == surfaceDesc.Format && textureDesc.Width == surfaceDesc.Width && textureDesc.Height == surfaceDesc.Height);

	D3DLOCKED_RECT surfaceRect = {};
	tempSurface->LockRect(&surfaceRect, NULL, 0);

	D3DLOCKED_RECT textureRect = {};
	textureObject->LockRect(0, &textureRect, NULL, 0);

	for (unsigned int y = 0; y < textureDesc.Height; ++y)
	{
		memcpy(static_cast<char*>(textureRect.pBits) + textureRect.Pitch * y, static_cast<char*>(surfaceRect.pBits) + surfaceRect.Pitch * y, std::min(textureRect.Pitch, surfaceRect.Pitch));
	}

	textureObject->UnlockRect(0);
	tempSurface->UnlockRect();

	unsigned int rc = tempSurface->Release();
    RBXASSERT(rc == 0);
}

void DeviceContextD3D9::resolveFramebuffer(Framebuffer* msaaBuffer, Framebuffer* buffer, unsigned int mask)
{
	RBXASSERT(msaaBuffer->getSamples() > 1);
	RBXASSERT(buffer->getSamples() == 1);
	RBXASSERT(msaaBuffer->getWidth() == buffer->getWidth() && msaaBuffer->getHeight() == buffer->getHeight());

	FramebufferD3D9* msaaBuffer9 = static_cast<FramebufferD3D9*>(msaaBuffer);
	FramebufferD3D9* buffer9 = static_cast<FramebufferD3D9*>(buffer);

    RBXASSERT(msaaBuffer9->getColor().size() == 1);
    RBXASSERT(buffer9->getColor().size() == 1);
	RBXASSERT(mask == Buffer_Color);

    RenderbufferD3D9* msaaBufferColor9 = static_cast<RenderbufferD3D9*>(msaaBuffer9->getColor()[0].get());
    RenderbufferD3D9* bufferColor9 = static_cast<RenderbufferD3D9*>(buffer9->getColor()[0].get());

    device9->StretchRect(msaaBufferColor9->getObject(), NULL, bufferColor9->getObject(), NULL, D3DTEXF_LINEAR);
}

void DeviceContextD3D9::discardFramebuffer(Framebuffer* buffer, unsigned int mask)
{
}

void DeviceContextD3D9::bindProgram(ShaderProgram* program)
{
    if (cachedProgram != program)
    {
        cachedProgram = static_cast<ShaderProgramD3D9*>(program);

        cachedProgram->bind();
    }
}

void DeviceContextD3D9::setWorldTransforms4x3(const float* data, size_t matrixCount)
{
    cachedProgram->setWorldTransforms4x3(data, matrixCount);
}

void DeviceContextD3D9::setConstant(int handle, const float* data, size_t vectorCount)
{
    cachedProgram->setConstant(handle, data, vectorCount);
}

void DeviceContextD3D9::bindTexture(unsigned int stage, Texture* texture, const SamplerState& state)
{
	SamplerState realState =
		(state.getFilter() == SamplerState::Filter_Anisotropic && state.getAnisotropy() == 0)
		? SamplerState(state.getFilter(), state.getAddress(), defaultAnisotropy)
		: state;

	RBXASSERT(stage < device->getCaps().maxTextureUnits);
    RBXASSERT(stage < ARRAYSIZE(cachedTextureUnits));

    TextureUnit& u = cachedTextureUnits[stage];

    static_cast<TextureD3D9*>(texture)->flushChanges();

	if (u.texture != texture)
	{
		u.texture = static_cast<TextureD3D9*>(texture);

		device9->SetTexture(stage, u.texture->getObject());
	}

	if (u.samplerState != realState)
	{
		u.samplerState = realState;

		device9->SetSamplerState(stage, D3DSAMP_ADDRESSU, gSamplerAddressD3D9[realState.getAddress()]);
		device9->SetSamplerState(stage, D3DSAMP_ADDRESSV, gSamplerAddressD3D9[realState.getAddress()]);
		device9->SetSamplerState(stage, D3DSAMP_MINFILTER, gSamplerFilterD3D9[realState.getFilter()].min);
		device9->SetSamplerState(stage, D3DSAMP_MAGFILTER, gSamplerFilterD3D9[realState.getFilter()].mag);
		device9->SetSamplerState(stage, D3DSAMP_MIPFILTER, gSamplerFilterD3D9[realState.getFilter()].mip);

		if (realState.getFilter() == SamplerState::Filter_Anisotropic)
            device9->SetSamplerState(stage, D3DSAMP_MAXANISOTROPY, realState.getAnisotropy());
	}
}

void DeviceContextD3D9::setRasterizerState(const RasterizerState& state)
{
    if (cachedRasterizerState != state)
	{
        cachedRasterizerState = state;

		device9->SetRenderState(D3DRS_CULLMODE, gCullModeD3D9[state.getCullMode()]);

		float bias = static_cast<float>(state.getDepthBias()) / static_cast<float>(1 << 24);
		float slopeBias = static_cast<float>(state.getDepthBias()) / 32.f; // do we need explicit control over slope or just a better formula since these numbers are magic anyway?

		device9->SetRenderState(D3DRS_DEPTHBIAS, *reinterpret_cast<DWORD*>(&bias));
		device9->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *reinterpret_cast<DWORD*>(&slopeBias));
	}
}

void DeviceContextD3D9::setBlendState(const BlendState& state)
{
	if (cachedBlendState != state)
	{
		cachedBlendState = state;

		if (!state.blendingNeeded())
		{
            device9->SetRenderState(D3DRS_ALPHABLENDENABLE, false);
		}
		else
		{
            device9->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
			device9->SetRenderState(D3DRS_SRCBLEND, gBlendFactorsD3D9[state.getColorSrc()]);
			device9->SetRenderState(D3DRS_DESTBLEND, gBlendFactorsD3D9[state.getColorDst()]);

			if (state.separateAlphaBlend())
			{
				device9->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, true);
				device9->SetRenderState(D3DRS_SRCBLENDALPHA, gBlendFactorsD3D9[state.getAlphaSrc()]);
				device9->SetRenderState(D3DRS_DESTBLENDALPHA, gBlendFactorsD3D9[state.getAlphaDst()]);
			}
			else
				device9->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE, false);
		}

        unsigned int colorMask = 0;

		if (state.getColorMask() & BlendState::Color_R)
			colorMask |= D3DCOLORWRITEENABLE_RED;

		if (state.getColorMask() & BlendState::Color_G)
			colorMask |= D3DCOLORWRITEENABLE_GREEN;

		if (state.getColorMask() & BlendState::Color_B)
			colorMask |= D3DCOLORWRITEENABLE_BLUE;

		if (state.getColorMask() & BlendState::Color_A)
			colorMask |= D3DCOLORWRITEENABLE_ALPHA;

		device9->SetRenderState(D3DRS_COLORWRITEENABLE, colorMask);
	}
}

void DeviceContextD3D9::setDepthState(const DepthState& state)
{
	if (cachedDepthState != state)
	{
		cachedDepthState = state;

		if (state.getFunction() == DepthState::Function_Always && state.getWrite() == false)
		{
			device9->SetRenderState(D3DRS_ZENABLE, false);
		}
		else
		{
			device9->SetRenderState(D3DRS_ZENABLE, true);
			device9->SetRenderState(D3DRS_ZFUNC, gDepthFuncD3D9[state.getFunction()]);
			device9->SetRenderState(D3DRS_ZWRITEENABLE, state.getWrite());
		}

		switch (state.getStencilMode())
		{
		case DepthState::Stencil_None:
			device9->SetRenderState(D3DRS_STENCILENABLE, false);
            break;

		case DepthState::Stencil_IsNotZero:
			device9->SetRenderState(D3DRS_STENCILENABLE, true);
			device9->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, false);
			device9->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_NOTEQUAL);
			device9->SetRenderState(D3DRS_STENCILREF, 0);
			device9->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
			device9->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP);
			device9->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
            break;

		case DepthState::Stencil_UpdateZFail:
			device9->SetRenderState(D3DRS_STENCILENABLE, true);
			device9->SetRenderState(D3DRS_TWOSIDEDSTENCILMODE, true);
			device9->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
			device9->SetRenderState(D3DRS_CCW_STENCILFUNC, D3DCMP_ALWAYS);
			device9->SetRenderState(D3DRS_STENCILFAIL, D3DSTENCILOP_KEEP);
			device9->SetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_DECR);
			device9->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
			device9->SetRenderState(D3DRS_CCW_STENCILFAIL, D3DSTENCILOP_KEEP);
			device9->SetRenderState(D3DRS_CCW_STENCILZFAIL, D3DSTENCILOP_INCR);
			device9->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
            break;

		default:
            RBXASSERT(false);
		}
	}
}

void DeviceContextD3D9::drawImpl(Geometry* geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd)
{
	static_cast<GeometryD3D9*>(geometry)->draw(primitive, offset, count, indexRangeBegin, indexRangeEnd, &cachedVertexLayout, &cachedGeometry);
}

//////////////////////////////////////////////////////////////////////////
// Markers

static void ascii2unicode(wchar_t* dest, const char* src, int max)
{
    while( (*dest++ = *src++) && max--) {}
}

void DeviceContextD3D9::pushDebugMarkerGroup(const char* text)
{
    if (pfn_D3DPERF_BeginEvent)
    {
        wchar_t buffer[512];
        ascii2unicode(buffer, text, sizeof(buffer)/sizeof(buffer[0]) );
        pfn_D3DPERF_BeginEvent(0, buffer);
    }
}

void DeviceContextD3D9::popDebugMarkerGroup()
{
    if (pfn_D3DPERF_EndEvent)
    {
        pfn_D3DPERF_EndEvent();
    }
}

void DeviceContextD3D9::setDebugMarker(const char* text)
{
    if (pfn_D3DPERF_SetMarker)
    {
        wchar_t buffer[512];
        ascii2unicode(buffer, text, sizeof(buffer)/sizeof(buffer[0]));
        pfn_D3DPERF_SetMarker(0, buffer);
    }
}


DeviceContextFFPD3D9::DeviceContextFFPD3D9(Device* device)
	: DeviceContextD3D9(device)
{
}

DeviceContextFFPD3D9::~DeviceContextFFPD3D9()
{
}

void DeviceContextFFPD3D9::updateGlobalConstants(const void* data, size_t dataSize)
{
    const std::map<std::string, ShaderGlobalConstant>& globalConstants = static_cast<DeviceD3D9*>(device)->getGlobalConstants();
    
    // we use only viewprojection matrices in shaders. So we'll simple feed view by identity and projection by viewProj
    D3DXMATRIX matrix;
    D3DXMatrixIdentity(&matrix);
    device9->SetTransform(D3DTS_VIEW, &matrix);

    if (const ShaderGlobalConstant* viewProjection = findKey(globalConstants, "_G.ViewProjection"))
	{
		const float* source = reinterpret_cast<const float*>(static_cast<const char*>(data) + viewProjection->offset);

        D3DMATRIX matrix;
		transposeMatrix(&matrix._11, source, source + 12);

		device9->SetTransform(D3DTS_PROJECTION, &matrix);
	}
}

void DeviceContextFFPD3D9::bindProgram(ShaderProgram* program)
{
    device9->SetRenderState(D3DRS_LIGHTING, false);

    device9->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    device9->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    device9->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
}

void DeviceContextFFPD3D9::setWorldTransforms4x3(const float* data, size_t matrixCount)
{
    static const float identity[] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0};
    static const float lastColumn[] = {0, 0, 0, 1};

    D3DMATRIX matrix;
    transposeMatrix(&matrix._11, matrixCount > 0 ? data : identity, lastColumn);

    device9->SetTransform(D3DTS_WORLD, &matrix);
}

void DeviceContextFFPD3D9::setConstant(int handle, const float* data, size_t vectorCount)
{
    if (handle == 0)
    {
		device9->SetRenderState(D3DRS_TEXTUREFACTOR, colorToD3D(data));

        device9->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
        device9->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    }
}

}
}
