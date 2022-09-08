#include "DeviceGL.h"

#include "GeometryGL.h"
#include "ShaderGL.h"
#include "TextureGL.h"
#include "FramebufferGL.h"

#include "HeadersGL.h"

FASTFLAGVARIABLE(GraphicsGLUseDiscard, false)

namespace RBX
{
namespace Graphics
{

static const GLenum gCullModeGL[RasterizerState::Cull_Count] =
{
    GL_NONE,
    GL_BACK,
    GL_FRONT
};

struct BlendFuncGL
{
    GLenum src, dst;
};

static const GLenum gBlendFactorsGL[BlendState::Factor_Count] =
{
	GL_ONE,
	GL_ZERO,
	GL_DST_COLOR,
	GL_SRC_ALPHA,
	GL_ONE_MINUS_SRC_ALPHA,
	GL_DST_ALPHA,
	GL_ONE_MINUS_DST_ALPHA
};

static const GLenum gDepthFuncGL[DepthState::Function_Count] =
{
    GL_ALWAYS,
    GL_LESS,
	GL_LEQUAL
};

DeviceContextGL::DeviceContextGL(DeviceGL* dev)
	: globalDataVersion(0)
    , device(dev)
	, defaultAnisotropy(1)
    , cachedProgram(0) 
	, cachedRasterizerState(RasterizerState::Cull_None)
    , cachedBlendState(BlendState::Mode_None)
	, cachedDepthState(DepthState::Function_Always, false)
{
	for (size_t i = 0; i < ARRAYSIZE(cachedTextures); ++i)
		cachedTextures[i] = NULL;
}

DeviceContextGL::~DeviceContextGL()
{
}

void DeviceContextGL::clearStates()
{
    // Clear framebuffer cache
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Clear program cache
    cachedProgram = 0;
	glUseProgram(0);
    
    // Clear texture cache
	for (size_t i = 0; i < ARRAYSIZE(cachedTextures); ++i)
		cachedTextures[i] = NULL;

    // Clear states to invalid values to guarantee a cache miss on the next setup
	cachedRasterizerState = RasterizerState(RasterizerState::Cull_Count);
    cachedBlendState = BlendState(BlendState::Mode_Count);
	cachedDepthState = DepthState(DepthState::Function_Count, false);

    // Setup state we never touch once
#ifndef GLES
	glEnable(GL_PROGRAM_POINT_SIZE);
#endif
}

void DeviceContextGL::invalidateCachedProgram()
{
    cachedProgram = 0;
}

void DeviceContextGL::invalidateCachedTexture(Texture* texture)
{
	for (unsigned int stage = 0; stage < ARRAYSIZE(cachedTextures); ++stage)
        if (cachedTextures[stage] == texture)
            cachedTextures[stage] = NULL;
}

void DeviceContextGL::invalidateCachedTextureStage(unsigned int stage)
{
    RBXASSERT(stage < ARRAYSIZE(cachedTextures));
	cachedTextures[stage] = NULL;
}

void DeviceContextGL::defineGlobalConstants(size_t dataSize)
{
	RBXASSERT(globalData.empty());
    RBXASSERT(dataSize > 0);

	globalData.resize(dataSize);
}

void DeviceContextGL::setDefaultAnisotropy(unsigned int value)
{
    defaultAnisotropy = value;
}

void DeviceContextGL::updateGlobalConstants(const void* data, size_t dataSize)
{
    RBXASSERT(dataSize == globalData.size());

    memcpy(&globalData[0], data, dataSize);
    globalDataVersion++;
}

void DeviceContextGL::bindFramebuffer(Framebuffer* buffer)
{
    unsigned int drawId = static_cast<FramebufferGL*>(buffer)->getId();

    glBindFramebuffer(GL_FRAMEBUFFER, drawId);
    glViewport(0, 0, buffer->getWidth(), buffer->getHeight());

#ifndef GLES
	unsigned int drawBuffers = static_cast<FramebufferGL*>(buffer)->getDrawBuffers();

	if (drawId == 0)
	{
		glDrawBuffer(GL_BACK);
	}
	else if (glDrawBuffers)
	{
        GLenum buffers[16];
		RBXASSERT(drawBuffers < ARRAYSIZE(buffers));

		for (unsigned int i = 0; i < drawBuffers; ++i)
			buffers[i] = GL_COLOR_ATTACHMENT0 + i;

		glDrawBuffers(drawBuffers, buffers);
	}
#endif
}

void DeviceContextGL::clearFramebuffer(unsigned int mask, const float color[4], float depth, unsigned int stencil)
{
    unsigned int maskGl = 0;

	if (mask & Buffer_Color)
	{
        // Need color writes for color clear to work
		setBlendState(BlendState(BlendState::Mode_None, BlendState::Color_All));

		maskGl |= GL_COLOR_BUFFER_BIT;
        glClearColor(color[0], color[1], color[2], color[3]);
	}

	if (mask & Buffer_Depth)
	{
        // Need depth writes for depth clear to work
        setDepthState(DepthState(DepthState::Function_Always, true));

		maskGl |= GL_DEPTH_BUFFER_BIT;
        glClearDepth(depth);
	}

	if (mask & Buffer_Stencil)
	{
        // Need stencil writes for stencil clear to work
		glStencilMask(~0u);

		maskGl |= GL_STENCIL_BUFFER_BIT;
		glClearStencil(stencil);
	}

    RBXASSERT(maskGl);
	glClear(maskGl);
}

void DeviceContextGL::copyFramebuffer(Framebuffer* buffer, Texture* texture)
{
	RBXASSERT(texture->getType() == Texture::Type_2D);
	RBXASSERT(buffer->getWidth() == texture->getWidth() && buffer->getHeight() == texture->getHeight());

	invalidateCachedTextureStage(0);

    GLint oldfb = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldfb);

	glBindFramebuffer(GL_FRAMEBUFFER, static_cast<FramebufferGL*>(buffer)->getId());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, static_cast<TextureGL*>(texture)->getId());

#ifndef GLES
    glReadBuffer(GL_COLOR_ATTACHMENT0);
#endif
    
    glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, texture->getWidth(), texture->getHeight());

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, oldfb);
}

void DeviceContextGL::resolveFramebuffer(Framebuffer* msaaBuffer, Framebuffer* buffer, unsigned int mask)
{
	RBXASSERT(msaaBuffer->getSamples() > 1);
	RBXASSERT(buffer->getSamples() == 1);
	RBXASSERT(msaaBuffer->getWidth() == buffer->getWidth() && msaaBuffer->getHeight() == buffer->getHeight());

    GLint oldfb = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldfb);

    glBindFramebuffer(GL_READ_FRAMEBUFFER, static_cast<FramebufferGL*>(msaaBuffer)->getId());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, static_cast<FramebufferGL*>(buffer)->getId());

	if (mask & Buffer_Color)
	{
	#ifndef GLES
        glReadBuffer(GL_COLOR_ATTACHMENT0);
	#endif

        glBlitFramebuffer(0, 0, buffer->getWidth(), buffer->getHeight(), 0, 0, buffer->getWidth(), buffer->getHeight(), GL_COLOR_BUFFER_BIT, GL_LINEAR);
	}

	if (mask & (Buffer_Depth | Buffer_Stencil))
	{
        unsigned int maskGl = 0;

        if (mask & Buffer_Depth)
			maskGl |= GL_DEPTH_BUFFER_BIT;

        if (mask & Buffer_Stencil)
			maskGl |= GL_STENCIL_BUFFER_BIT;

		glBlitFramebuffer(0, 0, buffer->getWidth(), buffer->getHeight(), 0, 0, buffer->getWidth(), buffer->getHeight(), maskGl, GL_NEAREST);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, oldfb);
}

void DeviceContextGL::discardFramebuffer(Framebuffer* buffer, unsigned int mask)
{
    if (!device->getCapsGL().ext3)
		return;

	if (!FFlag::GraphicsGLUseDiscard)
		return;

	unsigned int drawBuffers = static_cast<FramebufferGL*>(buffer)->getDrawBuffers();

	GLenum attachments[16];
	unsigned int index = 0;

	if (mask & Buffer_Color)
		for (unsigned int i = 0; i < drawBuffers; ++i)
			attachments[index++] = GL_COLOR_ATTACHMENT0 + i;

	if (mask & Buffer_Depth)
		attachments[index++] = GL_DEPTH_ATTACHMENT;
		
	if (mask & Buffer_Stencil)
		attachments[index++] = GL_STENCIL_ATTACHMENT;

	if (index > 0)
	{
		GLint oldfb = 0;
		glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldfb);

		glBindFramebuffer(GL_FRAMEBUFFER, static_cast<FramebufferGL*>(buffer)->getId());

		glInvalidateFramebuffer(GL_FRAMEBUFFER, index, attachments);

		glBindFramebuffer(GL_FRAMEBUFFER, oldfb);
	}
}

void DeviceContextGL::bindProgram(ShaderProgram* program)
{
    static_cast<ShaderProgramGL*>(program)->bind(&globalData[0], globalDataVersion, &cachedProgram);
}

void DeviceContextGL::setWorldTransforms4x3(const float* data, size_t matrixCount)
{
	cachedProgram->setWorldTransforms4x3(data, matrixCount);
}

void DeviceContextGL::setConstant(int handle, const float* data, size_t vectorCount)
{
	cachedProgram->setConstant(handle, data, vectorCount);
}

void DeviceContextGL::bindTexture(unsigned int stage, Texture* texture, const SamplerState& state)
{
	SamplerState realState =
		(state.getFilter() == SamplerState::Filter_Anisotropic && state.getAnisotropy() == 0)
		? SamplerState(state.getFilter(), state.getAddress(), defaultAnisotropy)
		: state;

	RBXASSERT(stage < device->getCaps().maxTextureUnits);
    RBXASSERT(stage < ARRAYSIZE(cachedTextures));

	static_cast<TextureGL*>(texture)->bind(stage, realState, &cachedTextures[stage]);
}

void DeviceContextGL::setRasterizerState(const RasterizerState& state)
{
    if (cachedRasterizerState != state)
	{
        cachedRasterizerState = state;

		if (state.getCullMode() == RasterizerState::Cull_None)
		{
            glDisable(GL_CULL_FACE);
		}
		else
		{
            glEnable(GL_CULL_FACE);
			glCullFace(gCullModeGL[state.getCullMode()]);
		}

		if (state.getDepthBias() == 0)
		{
			glDisable(GL_POLYGON_OFFSET_FILL);
		}
		else
		{
			float bias = static_cast<float>(state.getDepthBias());
			float slopeBias = bias / 32.f; // do we need explicit control over slope or just a better formula since these numbers are magic anyway?

			glEnable(GL_POLYGON_OFFSET_FILL);
			glPolygonOffset(slopeBias, bias);
		}
	}
}

void DeviceContextGL::setBlendState(const BlendState& state)
{
	if (cachedBlendState != state)
	{
		cachedBlendState = state;

        unsigned int colorMask = state.getColorMask();

		glColorMask((colorMask & BlendState::Color_R) != 0, (colorMask & BlendState::Color_G) != 0, (colorMask & BlendState::Color_B) != 0, (colorMask & BlendState::Color_A) != 0);

		if (!state.blendingNeeded())
		{
            glDisable(GL_BLEND);
		}
		else
		{
            glEnable(GL_BLEND);
			
			if (!state.separateAlphaBlend())
				glBlendFunc(gBlendFactorsGL[state.getColorSrc()], gBlendFactorsGL[state.getColorDst()]);
			else
				glBlendFuncSeparate(gBlendFactorsGL[state.getColorSrc()], gBlendFactorsGL[state.getColorDst()], gBlendFactorsGL[state.getAlphaSrc()], gBlendFactorsGL[state.getAlphaDst()]);
		}
	}
}

void DeviceContextGL::setDepthState(const DepthState& state)
{
	if (cachedDepthState != state)
	{
		cachedDepthState = state;

		if (state.getFunction() == DepthState::Function_Always && !state.getWrite())
		{
			glDisable(GL_DEPTH_TEST);
		}
		else
		{
            glEnable(GL_DEPTH_TEST);
			glDepthFunc(gDepthFuncGL[state.getFunction()]);
			glDepthMask(state.getWrite());
		}

        switch (state.getStencilMode())
        {
        case DepthState::Stencil_None:
			glDisable(GL_STENCIL_TEST);
            break;

        case DepthState::Stencil_IsNotZero:
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_NOTEQUAL, 0, ~0u);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            break;

        case DepthState::Stencil_UpdateZFail:
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 0, ~0u);
            glStencilOpSeparate(GL_FRONT, GL_KEEP, GL_DECR_WRAP, GL_KEEP);
            glStencilOpSeparate(GL_BACK, GL_KEEP, GL_INCR_WRAP, GL_KEEP);
            break;

        default:
            RBXASSERT(false);
		}
	}
}

void DeviceContextGL::drawImpl(Geometry* geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd)
{
    static_cast<GeometryGL*>(geometry)->draw(primitive, offset, count);
}


//////////////////////////////////////////////////////////////////////////
// Markers:

void DeviceContextGL::pushDebugMarkerGroup(const char* text)
{
#ifdef RBX_PLATFORM_IOS
    if (device->getCapsGL().extDebugMarkers)
    {
        glPushGroupMarkerEXT(0, text);
    }
#elif defined(__ANDROID__)
    ;
#else
    if (glPushDebugGroup) // Requires GL4.3, because ARB-version does not have marker enums for type
    {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION_ARB, 0, -1, text );
    }
#endif
}

void DeviceContextGL::popDebugMarkerGroup()
{
#ifdef RBX_PLATFORM_IOS
    if (device->getCapsGL().extDebugMarkers)
    {
        glPopGroupMarkerEXT();
    }
#elif defined(__ANDROID__)
    ;
#else
    if (glPopDebugGroup) // Requires GL4.3, because ARB-version does not have marker enums for type
    {
        glPopDebugGroup();
    }
#endif
}

void DeviceContextGL::setDebugMarker(const char* text)
{
#ifdef RBX_PLATFORM_IOS
    if (device->getCapsGL().extDebugMarkers)
    {
        glInsertEventMarkerEXT(0, text);
    }
#elif defined(__ANDROID__)
    ;
#else
    if (glDebugMessageInsert) // Requires GL4.3, because ARB-version does not have marker enums for type
    {
        glDebugMessageInsert(GL_DEBUG_SOURCE_APPLICATION_ARB, GL_DEBUG_TYPE_MARKER, 0, GL_DEBUG_SEVERITY_NOTIFICATION, -1, text );
    }
#endif
}


}
}
