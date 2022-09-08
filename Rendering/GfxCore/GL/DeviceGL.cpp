#include "DeviceGL.h"

#include "GeometryGL.h"
#include "ShaderGL.h"
#include "TextureGL.h"
#include "FramebufferGL.h"

#include "ContextGL.h"

#include "HeadersGL.h"

#include "rbx/Profiler.h"

#include <set>
#include <sstream>

LOGGROUP(Graphics)

FASTFLAGVARIABLE(DebugGraphicsGL, false)

FASTFLAGVARIABLE(GraphicsGL3, false)

FASTFLAG(RenderVR)

FASTFLAGVARIABLE(GraphicsGLReduceLatency, false)

namespace RBX
{
namespace Graphics
{

static std::set<std::string> getExtensions()
{
    std::set<std::string> result;
    
    if (const char* extensionString = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS)))
    {
        std::istringstream ext(extensionString);
        
        std::string str;
        while (ext >> str)
            result.insert(str);
        
        return result;
    }
    
#ifndef GLES
    if (FFlag::GraphicsGL3 && GLEW_VERSION_3_0)
    {
        int extensionCount = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &extensionCount);
        
        for (int i = 0; i < extensionCount; ++i)
            result.insert(reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i)));
    }
#endif
    
    return result;
}
    
#ifdef GLES
static DeviceCapsGL createDeviceCaps(ContextGL* context, const std::set<std::string>& extensions)
{
    const char* renderer = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    
    bool version3 = FFlag::GraphicsGL3 && strncmp(version, "OpenGL ES ", 10) == 0 && version[10] >= '3';
    
    DeviceCapsGL caps;
    
    GLint texSize;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);
    
    GLint stencilBits;
    glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    
	caps.supportsFramebuffer = true;
    caps.supportsShaders = true;
	caps.supportsFFP = false;
    caps.supportsStencil = stencilBits >= 8;
    caps.supportsIndex32 = version3 || extensions.count("GL_OES_element_index_uint");
    
    caps.supportsTextureDXT = false;
    caps.supportsTexturePVR = extensions.count("GL_IMG_texture_compression_pvrtc");
    caps.supportsTextureHalfFloat = version3 || extensions.count("GL_OES_texture_half_float");
    caps.supportsTexture3D = version3;
    caps.supportsTextureNPOT = version3;
    caps.supportsTextureETC1 = extensions.count("GL_OES_compressed_ETC1_RGB8_texture");

	caps.supportsTexturePartialMipChain = version3 || extensions.count("GL_APPLE_texture_max_level") || (!FFlag::GraphicsGL3 && strstr(version, "OpenGL ES 3"));

    caps.maxDrawBuffers = 1;

	if (version3)
	{
		GLint samples = 1;
		glGetIntegerv(GL_MAX_SAMPLES, &samples);

		caps.maxSamples = samples;
	}
	else
	{
		caps.maxSamples = 1;
	}

    caps.maxTextureSize = texSize;
	caps.maxTextureUnits = version3 ? 16 : 8;
    
    caps.colorOrderBGR = false;
    caps.needsHalfPixelOffset = false;
    caps.requiresRenderTargetFlipping = true;

    caps.retina = context->isMainFramebufferRetina();
    
    caps.ext3 = version3;
    
    caps.extVertexArrayObject = version3 || (extensions.count("GL_OES_vertex_array_object") && !strstr(renderer, "Adreno"));
    caps.extTextureStorage = version3 || extensions.count("GL_EXT_texture_storage");
    caps.extMapBuffer = version3 || extensions.count("GL_OES_mapbuffer");
    caps.extMapBufferRange = version3 || extensions.count("GL_EXT_map_buffer_range");
    caps.extTimerQuery = false;
    caps.extDebugMarkers = extensions.count("GL_EXT_debug_marker");
    caps.extSync = version3;
    
    return caps;
}
#else
static DeviceCapsGL createDeviceCapsOld(ContextGL* context)
{
	DeviceCapsGL caps;

    GLint texSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);

    GLint stencilBits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencilBits);

	caps.supportsFramebuffer = GLEW_VERSION_3_0 || GLEW_ARB_framebuffer_object || GLEW_EXT_framebuffer_object;
    caps.supportsShaders = GLEW_VERSION_2_0 || (GLEW_ARB_shading_language_100 && GLEW_ARB_shader_objects && GLEW_ARB_fragment_shader && GLEW_ARB_vertex_shader);
	caps.supportsFFP = false;
	caps.supportsStencil = GLEW_VERSION_2_0 && stencilBits >= 8; // we need full two-sided stencil support
    caps.supportsIndex32 = true;

	caps.supportsTextureDXT = (GLEW_VERSION_1_3 || GLEW_ARB_texture_compression) && GLEW_EXT_texture_compression_s3tc;
	caps.supportsTexturePVR = false;
	caps.supportsTextureHalfFloat = !!GLEW_ARB_half_float_pixel;
	caps.supportsTexture3D = GLEW_VERSION_1_2 || GLEW_EXT_texture3D;
    caps.supportsTextureETC1 = false;

	caps.supportsTexturePartialMipChain = true;

    // According to http://aras-p.info/blog/2012/10/17/non-power-of-two-textures/, GL extensions lie
	caps.supportsTextureNPOT = GLEW_ARB_texture_non_power_of_two && texSize >= 8192;

	if (caps.supportsFramebuffer && (GLEW_VERSION_3_0 || GLEW_ARB_draw_buffers || GLEW_ATI_draw_buffers))
	{
        GLint drawBuffers = 0;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &drawBuffers);

        caps.maxDrawBuffers = drawBuffers;
	}
	else
	{
		caps.maxDrawBuffers = 1;
	}

	if (caps.supportsFramebuffer && (GLEW_VERSION_3_3 || GLEW_EXT_framebuffer_multisample))
	{
        GLint samples = 0;
		glGetIntegerv(GL_MAX_SAMPLES, &samples);

		caps.maxSamples = samples;
	}
	else
	{
        caps.maxSamples = 1;
	}

	caps.maxTextureSize = texSize;
	caps.maxTextureUnits = 16;

    caps.colorOrderBGR = false;
    caps.needsHalfPixelOffset = false;
	caps.requiresRenderTargetFlipping = true;

    caps.retina = context->isMainFramebufferRetina();

    caps.ext3 = false;
    
	caps.extVertexArrayObject = (GLEW_VERSION_3_0 || GLEW_ARB_vertex_array_object || GLEW_APPLE_vertex_array_object);
	caps.extTextureStorage = (GLEW_VERSION_4_2 || GLEW_ARB_texture_storage);
	caps.extMapBuffer = true;
	caps.extMapBufferRange = (GLEW_VERSION_3_0 || GLEW_ARB_map_buffer_range);
	caps.extTimerQuery = !!GLEW_EXT_timer_query;
    caps.extDebugMarkers = false;
    caps.extSync = (GLEW_VERSION_3_2 || GLEW_ARB_sync);

#ifdef _WIN32
    const char* vendorString = reinterpret_cast<const char*>(glGetString(GL_VENDOR));

    if (strstr(vendorString, "Intel"))
    {
        // Intel drivers don't store IB state as part of VAO (http://stackoverflow.com/questions/8973690/vao-and-element-array-buffer-state)
        caps.extVertexArrayObject = false;
    }

	if (strstr(vendorString, "ATI Technologies"))
    {
        // Some AMD drivers can't create cubemaps with TS and can create 3D textures that they can't update afterwards...
		caps.extTextureStorage = false;
    }
#endif

    return caps;
}

static DeviceCapsGL createDeviceCaps(ContextGL* context, const std::set<std::string>& extensions)
{
    if (!FFlag::GraphicsGL3)
        return createDeviceCapsOld(context);
    
	const char* version = reinterpret_cast<const char*>(glGetString(GL_VERSION));

    bool version3 = isdigit(version[0]) && version[0] >= '3';

	DeviceCapsGL caps;

    GLint texSize = 0;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &texSize);

    GLint stencilBits = 0;
	glGetIntegerv(GL_STENCIL_BITS, &stencilBits);
    
	caps.supportsFramebuffer = version3 || (extensions.count("GL_ARB_framebuffer_object") || extensions.count("GL_EXT_framebuffer_object"));
    caps.supportsShaders =
        version3 || GLEW_VERSION_2_0 ||
        // need this for Mac OpenGL 1.4 :-/
        (extensions.count("GL_ARB_shading_language_100") && extensions.count("GL_ARB_shader_objects") && extensions.count("GL_ARB_fragment_shader") && extensions.count("GL_ARB_vertex_shader"));

	caps.supportsFFP = false;
	caps.supportsStencil = GLEW_VERSION_2_0 && stencilBits >= 8; // we need full two-sided stencil support
    caps.supportsIndex32 = true;

	caps.supportsTextureDXT = !!extensions.count("GL_EXT_texture_compression_s3tc");
	caps.supportsTexturePVR = false;
	caps.supportsTextureHalfFloat = version3;
	caps.supportsTexture3D = true;
    caps.supportsTextureETC1 = false;

	caps.supportsTexturePartialMipChain = true;

	caps.supportsTextureNPOT = version3;

	if (version3)
	{
        GLint drawBuffers = 0;
        glGetIntegerv(GL_MAX_DRAW_BUFFERS, &drawBuffers);

        GLint samples = 0;
		glGetIntegerv(GL_MAX_SAMPLES, &samples);

        caps.maxDrawBuffers = drawBuffers;
		caps.maxSamples = samples;
	}
	else
	{
		caps.maxDrawBuffers = 1;
        caps.maxSamples = 1;
	}

	caps.maxTextureSize = texSize;
	caps.maxTextureUnits = 16;

    caps.colorOrderBGR = false;
    caps.needsHalfPixelOffset = false;
	caps.requiresRenderTargetFlipping = true;

    caps.retina = context->isMainFramebufferRetina();

    caps.ext3 = version3;
    
	caps.extVertexArrayObject = version3;
	caps.extTextureStorage = GLEW_VERSION_4_2 || (version3 && extensions.count("GL_ARB_texture_storage"));
	caps.extMapBuffer = true;
	caps.extMapBufferRange = version3;
	caps.extTimerQuery = !!GLEW_VERSION_3_3;
    caps.extDebugMarkers = false;
    caps.extSync = !!GLEW_VERSION_3_2;

    return caps;
}

static void GLAPIENTRY debugOutputGLARB(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam)
{
	FASTLOGS(FLog::Graphics, "GL Debug: %s", message);
}
#endif

static DeviceVRGL* createVR()
{
#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
	if (DeviceVRGL* result = DeviceVRGL::createCardboard())
		return result;
#endif

	return NULL;
}
    
DeviceGL::DeviceGL(void* windowHandle)
    : frameTimeQueryId(0)
    , frameTimeQueryIssued(false)
    , frameEventQueryId(0)
    , frameEventQueryIssued(false)
    , gpuTime(0)
	, vrEnabled(true)
{
	glContext.reset(ContextGL::create(windowHandle));

	if (FFlag::RenderVR)
		vr.reset(createVR());

    std::set<std::string> extensions = getExtensions();

	caps = createDeviceCaps(glContext.get(), extensions);

	FASTLOGS(FLog::Graphics, "GL Renderer: %s", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	FASTLOGS(FLog::Graphics, "GL Version: %s", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
	FASTLOGS(FLog::Graphics, "GL Vendor: %s", reinterpret_cast<const char*>(glGetString(GL_VENDOR)));

	if (caps.supportsShaders)
        FASTLOGS(FLog::Graphics, "GLSL version: %s", reinterpret_cast<const char*>(glGetString(GL_SHADING_LANGUAGE_VERSION)));

    std::string extensionString;

    for (auto& e: extensions)
    {
        extensionString += " ";
        extensionString += e;
    }

	while (!extensionString.empty())
	{
		std::string::size_type space = extensionString.find_first_of(' ', 128);
		FASTLOGS(FLog::Graphics, "Ext:%s", extensionString.substr(0, space));
		extensionString.erase(0, space);
    }

	caps.dumpToFLog(FLog::Graphics);

	FASTLOG1(FLog::Graphics, "Caps: GL3 %d", caps.ext3);
	FASTLOG5(FLog::Graphics, "Caps: VAO %d TexStorage %d MapBuffer %d MapBufferRange %d TimerQuery %d", caps.extVertexArrayObject, caps.extTextureStorage, caps.extMapBuffer, caps.extMapBufferRange, caps.extTimerQuery);
	FASTLOG2(FLog::Graphics, "Caps: DebugMarkers %d Sync %d", caps.extDebugMarkers, caps.extSync);

	immediateContext.reset(new DeviceContextGL(this));

	mainFramebuffer.reset(new FramebufferGL(this, glContext->getMainFramebufferId()));

	std::pair<unsigned int, unsigned int> dimensions = glContext->updateMainFramebuffer(0, 0);

	mainFramebuffer->updateDimensions(dimensions.first, dimensions.second);

#ifndef GLES
	if (caps.extTimerQuery)
	{
		glGenQueries(1, &frameTimeQueryId);
		RBXASSERT(frameTimeQueryId);
	}

    if (FFlag::DebugGraphicsGL && GLEW_ARB_debug_output)
    {
        glDebugMessageCallbackARB(debugOutputGLARB, NULL);
    }
#endif

#ifndef _WIN32
	Profiler::gpuInit(0);
#endif

	if (vr)
		vr->setup(this);
}

DeviceGL::~DeviceGL()
{
#ifndef _WIN32
	Profiler::gpuShutdown();
#endif

	vr.reset();
    immediateContext.reset();
	mainFramebuffer.reset();

	glContext.reset();
}

std::string DeviceGL::getFeatureLevel()
{ 
    std::string glString = reinterpret_cast<const char*>(glGetString(GL_VERSION));
    std::string featureLevel = "OpenGL ";
    if (!glString.empty())
    {
        size_t dotId = glString.find('.');

        // this is based on GLEW implementation of determining GL version. Does it look unsafe? Dont worry, GLEW version is even worse;)
        if (dotId > 0 && dotId < (glString.length() -1) && isdigit(glString[dotId-1]) && isdigit(glString[dotId+1]))
        {
            featureLevel += glString[dotId - 1];
            featureLevel += '.';
            featureLevel += glString[dotId + 1];
            return featureLevel;
        }
    }

    featureLevel += "unknown";
    return featureLevel;
}

bool DeviceGL::validate()
{
    glContext->setCurrent();
    
	std::pair<unsigned int, unsigned int> dimensions = glContext->updateMainFramebuffer(mainFramebuffer->getWidth(), mainFramebuffer->getHeight());

	mainFramebuffer->updateDimensions(dimensions.first, dimensions.second);

    return true;
}

DeviceContext* DeviceGL::beginFrame()
{
    glContext->setCurrent();

#ifndef GLES
	if (frameTimeQueryId && !frameTimeQueryIssued)
	{
		glBeginQuery(GL_TIME_ELAPSED, frameTimeQueryId);
	}
#endif

	immediateContext->bindFramebuffer(mainFramebuffer.get());
	immediateContext->clearStates();

	return immediateContext.get();
}

void DeviceGL::endFrame()
{
#ifndef GLES
	if (frameTimeQueryId)
	{
		if (!frameTimeQueryIssued)
		{
			glEndQuery(GL_TIME_ELAPSED);

			frameTimeQueryIssued = true;
		}
		else
		{
            int available = 0;
			glGetQueryObjectiv(frameTimeQueryId, GL_QUERY_RESULT_AVAILABLE, &available);

            if (available)
			{
				GLuint64EXT elapsed = 0;
                if (FFlag::GraphicsGL3)
    				glGetQueryObjectui64v(frameTimeQueryId, GL_QUERY_RESULT, &elapsed);
                else
    				glGetQueryObjectui64vEXT(frameTimeQueryId, GL_QUERY_RESULT, &elapsed);

                gpuTime = static_cast<float>(elapsed) / 1e6f;

				frameTimeQueryIssued = false;
			}
		}
	}
#endif

    if (FFlag::GraphicsGLReduceLatency && caps.extSync)
    {
        // Wait for last frame to finish on GPU
        if (frameEventQueryIssued)
        {
            int rc = glClientWaitSync(frameEventQueryId, GL_SYNC_FLUSH_COMMANDS_BIT, GLuint64(5e9));
            RBXASSERT(rc == GL_CONDITION_SATISFIED || rc == GL_ALREADY_SIGNALED);

            glDeleteSync(frameEventQueryId);
            frameEventQueryId = 0;

            frameEventQueryIssued = false;
        }

        // Submit new query for frame wait
        if (!frameEventQueryIssued)
        {
            frameEventQueryId = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
            RBXASSERT(frameEventQueryId);

            frameEventQueryIssued = true;
        }
    }

	if (vr && vrEnabled)
	{
		vr->submitFrame(immediateContext.get());
	}

    glContext->swapBuffers();
}

Framebuffer* DeviceGL::getMainFramebuffer()
{
    return mainFramebuffer.get();
}

DeviceVR* DeviceGL::getVR()
{
	return (vr && vrEnabled) ? vr.get() : NULL;
}

void DeviceGL::setVR(bool enabled)
{
	vrEnabled = enabled;
}

void DeviceGL::defineGlobalConstants(size_t dataSize, const std::vector<ShaderGlobalConstant>& constants)
{
	RBXASSERT(globalConstants.empty());
	RBXASSERT(!constants.empty());

    globalConstants = constants;

	immediateContext->defineGlobalConstants(dataSize);
}

std::string DeviceGL::getShadingLanguage()
{
#ifdef GLES
    return caps.ext3 ? "glsles3" : "glsles";
#else
    return caps.ext3 ? "glsl3" : "glsl";
#endif
}

std::string DeviceGL::createShaderSource(const std::string& path, const std::string& defines, boost::function<std::string (const std::string&)> fileCallback)
{
    // No preprocessor support
    return fileCallback(path);
}

std::vector<char> DeviceGL::createShaderBytecode(const std::string& source, const std::string& target, const std::string& entrypoint)
{
    // No bytecode support
    RBXASSERT(entrypoint == "main");

    return std::vector<char>(source.begin(), source.end());
}

shared_ptr<VertexShader> DeviceGL::createVertexShader(const std::vector<char>& bytecode)
{
	if (!caps.supportsShaders)
        throw RBX::runtime_error("No shader support");

    std::string source(bytecode.begin(), bytecode.end());

    return shared_ptr<VertexShader>(new VertexShaderGL(this, source));
}

shared_ptr<FragmentShader> DeviceGL::createFragmentShader(const std::vector<char>& bytecode)
{
	if (!caps.supportsShaders)
        throw RBX::runtime_error("No shader support");

    std::string source(bytecode.begin(), bytecode.end());

    return shared_ptr<FragmentShader>(new FragmentShaderGL(this, source));
}

shared_ptr<ShaderProgram> DeviceGL::createShaderProgram(const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
{
	if (!caps.supportsShaders)
        throw RBX::runtime_error("No shader support");

    return shared_ptr<ShaderProgram>(new ShaderProgramGL(this, vertexShader, fragmentShader));
}

shared_ptr<ShaderProgram> DeviceGL::createShaderProgramFFP()
{
	throw RBX::runtime_error("No FFP support");
}

shared_ptr<VertexBuffer> DeviceGL::createVertexBuffer(size_t elementSize, size_t elementCount, VertexBuffer::Usage usage)
{
	return shared_ptr<VertexBuffer>(new VertexBufferGL(this, elementSize, elementCount, usage));
}

shared_ptr<IndexBuffer> DeviceGL::createIndexBuffer(size_t elementSize, size_t elementCount, VertexBuffer::Usage usage)
{
	return shared_ptr<IndexBuffer>(new IndexBufferGL(this, elementSize, elementCount, usage));
}

shared_ptr<VertexLayout> DeviceGL::createVertexLayout(const std::vector<VertexLayout::Element>& elements)
{
    return shared_ptr<VertexLayout>(new VertexLayoutGL(this, elements));
}

shared_ptr<Texture> DeviceGL::createTexture(Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Texture::Usage usage)
{
    return shared_ptr<Texture>(new TextureGL(this, type, format, width, height, depth, mipLevels, usage));
}

shared_ptr<Renderbuffer> DeviceGL::createRenderbuffer(Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
{
    return shared_ptr<Renderbuffer>(new RenderbufferGL(this, format, width, height, samples));
}

shared_ptr<Geometry> DeviceGL::createGeometryImpl(const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
{
    return shared_ptr<Geometry>(new GeometryGL(this, layout, vertexBuffers, indexBuffer, baseVertexIndex));
}

shared_ptr<Framebuffer> DeviceGL::createFramebufferImpl(const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth)
{
	return shared_ptr<Framebuffer>(new FramebufferGL(this, color, depth));
}

DeviceStats DeviceGL::getStatistics() const
{
	DeviceStats result = {};

	result.gpuFrameTime = gpuTime;

    return result;
}

}
}
