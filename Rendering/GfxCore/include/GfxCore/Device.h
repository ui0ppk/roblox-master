#pragma once

#include "GfxCore/Geometry.h"
#include "GfxCore/Shader.h"
#include "GfxCore/Texture.h"
#include "GfxCore/pix.h"

#include <string>
#include <boost/function.hpp>


namespace RBX
{
namespace Graphics
{

class RasterizerState;
class BlendState;
class DepthState;
class SamplerState;

class Framebuffer;

class DeviceContext
{
public:
    enum BufferMask
	{
        Buffer_Color = 1 << 0,
        Buffer_Depth = 1 << 1,
        Buffer_Stencil = 1 << 2
	};

    virtual ~DeviceContext();

    virtual void setDefaultAnisotropy(unsigned int value) = 0;

    virtual void updateGlobalConstants(const void* data, size_t dataSize) = 0;

    virtual void bindFramebuffer(Framebuffer* buffer) = 0;
    virtual void clearFramebuffer(unsigned int mask, const float color[4], float depth, unsigned int stencil) = 0;

    virtual void copyFramebuffer(Framebuffer* buffer, Texture* texture) = 0;
    virtual void resolveFramebuffer(Framebuffer* msaaBuffer, Framebuffer* buffer, unsigned int mask) = 0;
    virtual void discardFramebuffer(Framebuffer* buffer, unsigned int mask) = 0;

    virtual void bindProgram(ShaderProgram* program) = 0;
    virtual void setWorldTransforms4x3(const float* data, size_t matrixCount) = 0;
    virtual void setConstant(int handle, const float* data, size_t vectorCount) = 0;

    virtual void bindTexture(unsigned int stage, Texture* texture, const SamplerState& state) = 0;

    virtual void setRasterizerState(const RasterizerState& state) = 0;
    virtual void setBlendState(const BlendState& state) = 0;
    virtual void setDepthState(const DepthState& state) = 0;

    void draw(Geometry* geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd);
    void draw(const GeometryBatch& geometryBatch);

    virtual void pushDebugMarkerGroup(const char* text) = 0;
    virtual void popDebugMarkerGroup() = 0;
    virtual void setDebugMarker(const char* text) = 0;
		
protected:
    DeviceContext();

    virtual void drawImpl(Geometry* geometry, Geometry::Primitive primitive, unsigned int offset, unsigned int count, unsigned int indexRangeBegin, unsigned int indexRangeEnd) = 0;
};

struct DeviceCaps
{
    bool supportsFramebuffer;
    bool supportsShaders;
    bool supportsFFP;
    bool supportsStencil;
    bool supportsIndex32;

    bool supportsTextureDXT;
    bool supportsTexturePVR;
    bool supportsTextureHalfFloat;
    bool supportsTexture3D;
    bool supportsTextureNPOT;
    bool supportsTextureETC1;

    bool supportsTexturePartialMipChain;

    unsigned int maxDrawBuffers;
    unsigned int maxSamples;
    unsigned int maxTextureSize;
    unsigned int maxTextureUnits;

    bool colorOrderBGR;
    bool needsHalfPixelOffset;
    bool requiresRenderTargetFlipping;
    
    bool retina;

    void dumpToFLog(int channel) const;
};

struct DeviceStats
{
    float gpuFrameTime;
};

class DeviceVR
{
public:
	struct Pose
	{
		bool valid;
		float position[3];
		float orientation[4];
	};

	struct State
	{
		Pose headPose;
		Pose handPose[2];

		float eyeOffset[2][3];
		float eyeFov[2][4]; // up-down-left-right
		
		bool needsMirror;
	};

	virtual ~DeviceVR();

	virtual void update() = 0;

	virtual void recenter() = 0;

	virtual Framebuffer* getEyeFramebuffer(int eye) = 0;

	virtual State getState() = 0;
};

typedef boost::function<void(void*)> DeviceFrameDataCallback;

class Device
{
    friend class Resource;

public:
    enum API
	{
        API_OpenGL,
        API_Direct3D9,
        API_Direct3D11
	};

    static Device* create(API api, void* windowHandle);

    virtual ~Device();

    virtual bool validate() = 0;

    virtual DeviceContext* beginFrame() = 0;
    virtual void endFrame() = 0;

	virtual Framebuffer* getMainFramebuffer() = 0;

	virtual DeviceVR* getVR() = 0;
	virtual void setVR(bool enabled) = 0;

    virtual void defineGlobalConstants(size_t dataSize, const std::vector<ShaderGlobalConstant>& constants) = 0;

    virtual std::string getAPIName() = 0;
    virtual std::string getFeatureLevel() = 0;
    virtual std::string getShadingLanguage() = 0;
    virtual std::string createShaderSource(const std::string& path, const std::string& defines, boost::function<std::string (const std::string&)> fileCallback) = 0;
    virtual std::vector<char> createShaderBytecode(const std::string& source, const std::string& target, const std::string& entrypoint) = 0;

    virtual shared_ptr<VertexShader> createVertexShader(const std::vector<char>& bytecode) = 0;
    virtual shared_ptr<FragmentShader> createFragmentShader(const std::vector<char>& bytecode) = 0;
    virtual shared_ptr<ShaderProgram> createShaderProgram(const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader) = 0;
    virtual shared_ptr<ShaderProgram> createShaderProgramFFP() = 0;

    virtual shared_ptr<VertexBuffer> createVertexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage) = 0;
    virtual shared_ptr<IndexBuffer> createIndexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage) = 0;
    virtual shared_ptr<VertexLayout> createVertexLayout(const std::vector<VertexLayout::Element>& elements) = 0;

    shared_ptr<Geometry> createGeometry(const shared_ptr<VertexLayout>& layout, const shared_ptr<VertexBuffer>& vertexBuffer, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex = 0);
    shared_ptr<Geometry> createGeometry(const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex = 0);

	virtual shared_ptr<Texture> createTexture(Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Texture::Usage usage) = 0;

	virtual shared_ptr<Renderbuffer> createRenderbuffer(Texture::Format format, unsigned int width, unsigned int height, unsigned int samples) = 0;

    shared_ptr<Framebuffer> createFramebuffer(const shared_ptr<Renderbuffer>& color, const shared_ptr<Renderbuffer>& depth = shared_ptr<Renderbuffer>());
    shared_ptr<Framebuffer> createFramebuffer(const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth = shared_ptr<Renderbuffer>());

	virtual const DeviceCaps& getCaps() const = 0;

    virtual DeviceStats getStatistics() const = 0;

	virtual void suspend() { RBXASSERT(false); /* only DX11-Durango*/ }
	virtual void resume()  { RBXASSERT(false); /* only DX11-Durango*/ }

protected:
    Resource* resourceListHead;
    Resource* resourceListTail;

    Device();

    virtual shared_ptr<Geometry> createGeometryImpl(const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex) = 0;
    virtual shared_ptr<Framebuffer> createFramebufferImpl(const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth) = 0;

    void fireDeviceLost();
    void fireDeviceRestored();
};

}
}
