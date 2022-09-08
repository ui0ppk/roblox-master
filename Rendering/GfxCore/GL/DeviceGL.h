#pragma once

#include "GfxCore/Device.h"
#include "GfxCore/States.h"

typedef struct __GLsync *GLsync;

namespace RBX
{
namespace Graphics
{

class FramebufferGL;
class ShaderProgramGL;
class TextureGL;
class ContextGL;
class DeviceGL;

class DeviceVRGL: public DeviceVR
{
public:
	virtual void setup(Device* device) = 0;
	virtual void submitFrame(DeviceContext* context) = 0;

	static DeviceVRGL* createCardboard();
};

struct DeviceCapsGL: DeviceCaps
{
    bool ext3;

    bool extVertexArrayObject;
    bool extTextureStorage;
    bool extMapBuffer;
    bool extMapBufferRange;
    bool extTimerQuery;
    bool extDebugMarkers;
    bool extSync;
};

class DeviceContextGL: public DeviceContext
{
public:
	DeviceContextGL(DeviceGL* dev);
    ~DeviceContextGL();

    void defineGlobalConstants(size_t dataSize);

    void clearStates();

    void invalidateCachedProgram();
    void invalidateCachedTexture(Texture* texture);
    void invalidateCachedTextureStage(unsigned int stage);

    virtual void setDefaultAnisotropy(unsigned int value);

    virtual void updateGlobalConstants(const void* data, size_t dataSize);

    virtual void bindFramebuffer(Framebuffer* buffer);
    virtual void clearFramebuffer(unsigned int mask, const float color[4], float depth, unsigned int stencil);

    virtual void copyFramebuffer(Framebuffer* buffer, Texture* texture);
    virtual void resolveFramebuffer(Framebuffer* msaaBuffer, Framebuffer* buffer, unsigned int mask);
    virtual void discardFramebuffer(Framebuffer* buffer, unsigned int mask);

    virtual void bindProgram(ShaderProgram* program);
    virtual void setWorldTransforms4x3(const float* data, size_t matrixCount);
    virtual void setConstant(int handle, const float* data, size_t vectorCount);

    virtual void bindTexture(unsigned int stage, Texture* texture, const SamplerState& state);

	virtual void setRasterizerState(const RasterizerState& state);
    virtual void setBlendState(const BlendState& state);
    virtual void setDepthState(const DepthState& state);

    virtual void drawImpl(Geometry* geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd);

    virtual void pushDebugMarkerGroup(const char* text);
    virtual void popDebugMarkerGroup();
    virtual void setDebugMarker(const char* text);

private:
    std::vector<char> globalData;
    unsigned int globalDataVersion;

    unsigned int defaultAnisotropy;

	ShaderProgramGL* cachedProgram;
    TextureGL* cachedTextures[16];

    RasterizerState cachedRasterizerState;
	BlendState cachedBlendState;
    DepthState cachedDepthState;
    DeviceGL*  device;
};

class DeviceGL: public Device
{
public:
    DeviceGL(void* windowHandle);
    ~DeviceGL();

    virtual bool validate();

    virtual DeviceContext* beginFrame();
    virtual void endFrame();

	virtual Framebuffer* getMainFramebuffer();

	virtual DeviceVR* getVR();
	virtual void setVR(bool enabled);

    virtual void defineGlobalConstants(size_t dataSize, const std::vector<ShaderGlobalConstant>& constants);

    virtual std::string getAPIName() { return "OpenGL"; }
    virtual std::string getFeatureLevel();
    virtual std::string getShadingLanguage();
    virtual std::string createShaderSource(const std::string& path, const std::string& defines, boost::function<std::string (const std::string&)> fileCallback);
    virtual std::vector<char> createShaderBytecode(const std::string& source, const std::string& target, const std::string& entrypoint);

    virtual shared_ptr<VertexShader> createVertexShader(const std::vector<char>& bytecode);
    virtual shared_ptr<FragmentShader> createFragmentShader(const std::vector<char>& bytecode);
    virtual shared_ptr<ShaderProgram> createShaderProgram(const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader);
    virtual shared_ptr<ShaderProgram> createShaderProgramFFP();

    virtual shared_ptr<VertexBuffer> createVertexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage);
    virtual shared_ptr<IndexBuffer> createIndexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage);
    virtual shared_ptr<VertexLayout> createVertexLayout(const std::vector<VertexLayout::Element>& elements);

	virtual shared_ptr<Texture> createTexture(Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Texture::Usage usage);

	virtual shared_ptr<Renderbuffer> createRenderbuffer(Texture::Format format, unsigned int width, unsigned int height, unsigned int samples);

    virtual shared_ptr<Geometry> createGeometryImpl(const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex);

    virtual shared_ptr<Framebuffer> createFramebufferImpl(const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth);

	virtual const DeviceCaps& getCaps() const { return caps; }

    virtual DeviceStats getStatistics() const;

	DeviceContextGL* getImmediateContextGL() { return immediateContext.get(); }

	const DeviceCapsGL& getCapsGL() const { return caps; }

	const std::vector<ShaderGlobalConstant>& getGlobalConstants() const { return globalConstants; }

private:
    DeviceCapsGL caps;

    scoped_ptr<DeviceContextGL> immediateContext;

	scoped_ptr<FramebufferGL> mainFramebuffer;

    std::vector<ShaderGlobalConstant> globalConstants;

    unsigned int frameTimeQueryId;
    bool frameTimeQueryIssued;

    GLsync frameEventQueryId;
    bool frameEventQueryIssued;

    float gpuTime;

	scoped_ptr<DeviceVRGL> vr;
	bool vrEnabled;

    scoped_ptr<ContextGL> glContext;
};

}
}
