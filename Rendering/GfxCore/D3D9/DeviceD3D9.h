#pragma once

#include "GfxCore/Device.h"
#include "GfxCore/States.h"

#include <map>

struct IDirect3D9;
struct IDirect3DDevice9;
struct IDirect3DSurface9;
struct IDirect3DQuery9;

namespace RBX
{
namespace Graphics
{

class FramebufferD3D9;
class ShaderProgramD3D9;
class TextureD3D9;
class VertexLayoutD3D9;
class GeometryD3D9;

struct DeviceCapsD3D9: DeviceCaps
{
    enum Vendor
	{
        Vendor_Unknown,
        Vendor_NVidia,
        Vendor_AMD,
        Vendor_Intel
	};

    Vendor vendor;
};

class DeviceContextD3D9: public DeviceContext
{
public:
	DeviceContextD3D9(Device* device);
    ~DeviceContextD3D9();

    void defineGlobalConstants(size_t dataSize);

    void clearStates();

    void invalidateCachedProgram();
    void invalidateCachedVertexLayout();
    void invalidateCachedGeometry();
    void invalidateCachedTexture(Texture* texture);

    virtual void updateGlobalConstants(const void* data, size_t dataSize);

    virtual void setDefaultAnisotropy(unsigned int value);

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

protected:
    struct TextureUnit
	{
        TextureD3D9* texture;
        SamplerState samplerState;

        TextureUnit()
			: texture(NULL)
			, samplerState(SamplerState::Filter_Point)
		{
		}
	};

    Device* device;

	IDirect3DDevice9* device9;

    size_t globalDataSize;

    unsigned int defaultAnisotropy;

    unsigned int cachedFramebufferSurfaces;

	ShaderProgramD3D9* cachedProgram;
	VertexLayoutD3D9* cachedVertexLayout;
    GeometryD3D9* cachedGeometry;

    TextureUnit cachedTextureUnits[16];

    RasterizerState cachedRasterizerState;
	BlendState cachedBlendState;
    DepthState cachedDepthState;

    // functions
    HMODULE d3d9;
    int (WINAPI *pfn_D3DPERF_BeginEvent)( DWORD col, LPCWSTR wszName);
    int (WINAPI *pfn_D3DPERF_EndEvent)();
    void (WINAPI *pfn_D3DPERF_SetMarker)( DWORD col, LPCWSTR wszName );
};

class DeviceContextFFPD3D9: public DeviceContextD3D9
{
public:
	DeviceContextFFPD3D9(Device* device);
    ~DeviceContextFFPD3D9();

    virtual void updateGlobalConstants(const void* data, size_t dataSize);

    virtual void bindProgram(ShaderProgram* program);
    virtual void setWorldTransforms4x3(const float* data, size_t matrixCount);
    virtual void setConstant(int handle, const float* data, size_t vectorCount);
};

class QueryD3D9: public Resource
{
public:
    QueryD3D9(Device* device, unsigned int type);
    ~QueryD3D9();

    virtual void onDeviceLost();
    virtual void onDeviceRestored();

	IDirect3DQuery9* getObject() const { return object; }

private:
    unsigned int type;
	IDirect3DQuery9* object;
};

class DeviceD3D9: public Device
{
public:
    DeviceD3D9(void* windowHandle);
    ~DeviceD3D9();

    virtual bool validate();

    virtual DeviceContext* beginFrame();
    virtual void endFrame();

	virtual Framebuffer* getMainFramebuffer();

	virtual DeviceVR* getVR();
	virtual void setVR(bool enabled);

    virtual void defineGlobalConstants(size_t dataSize, const std::vector<ShaderGlobalConstant>& constants);

    virtual std::string getAPIName() { return "DirectX 9"; }
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

	DeviceContextD3D9* getImmediateContextD3D9() { return immediateContext.get(); }

	const DeviceCapsD3D9& getCapsD3D9() const { return caps; }

	IDirect3DDevice9* getDevice9() { return device9; }

	size_t getGlobalDataSize() const { return globalDataSize; }
	const std::map<std::string, ShaderGlobalConstant>& getGlobalConstants() const { return globalConstants; }

private:
    void* windowHandle;
	IDirect3D9* d3d9;
    IDirect3DDevice9* device9;

    bool deviceLost;

    DeviceCapsD3D9 caps;

    scoped_ptr<DeviceContextD3D9> immediateContext;

	scoped_ptr<FramebufferD3D9> mainFramebuffer;

	scoped_ptr<QueryD3D9> frameEventQuery;
    bool frameEventQueryIssued;

	scoped_ptr<QueryD3D9> frameTimeBeginQuery, frameTimeEndQuery, frameTimeFreqQuery;
    bool frameTimeQueryIssued;

    float gpuTime;

    size_t globalDataSize;
    std::map<std::string, ShaderGlobalConstant> globalConstants;

	DeviceFrameDataCallback frameCallback;

    std::string featureLvlStr;

    void createMainFramebuffer();
    bool validateDevice(unsigned int width, unsigned int height);
    bool resetDevice(unsigned int width, unsigned int height);
	void handleDeviceLost();
    void getFeatureLevelDX11(std::string& strOut);

    enum DX11_FEATURE_LEVEL { 
        D3D_FEATURE_LEVEL_9_1   = 0x9100,
        D3D_FEATURE_LEVEL_9_2   = 0x9200,
        D3D_FEATURE_LEVEL_9_3   = 0x9300,
        D3D_FEATURE_LEVEL_10_0  = 0xa000,
        D3D_FEATURE_LEVEL_10_1  = 0xa100,
        D3D_FEATURE_LEVEL_11_0  = 0xb000,
        D3D_FEATURE_LEVEL_11_1  = 0xb100
    };

    typedef HRESULT (WINAPI *DX11_CreateDevice)(int*, unsigned, HMODULE, unsigned, const unsigned*, unsigned, unsigned, int **, unsigned*, int **);
};

}
}
