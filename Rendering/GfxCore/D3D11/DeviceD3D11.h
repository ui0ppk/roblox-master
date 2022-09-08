#pragma once

#include "GfxCore/Device.h"
#include "GfxCore/States.h"

#include <map>

#include <boost/unordered_map.hpp>

struct ID3D11Device;
struct ID3D11DeviceContext;
struct IDXGISwapChain;
struct ID3D11Buffer;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11DepthStencilState;
struct ID3D11SamplerState;
struct ID3D11ShaderResourceView;
struct ID3D11Resource;
struct ID3D11Query;
struct IDXGIAdapter;
struct IDXGIDevice1;
struct ID3D11DeviceChild;

namespace RBX
{
namespace Graphics
{

class DeviceVRD3D11: public DeviceVR
{
public:
	virtual void setup(Device* device) = 0;
	virtual void submitFrame(DeviceContext* context) = 0;

	static DeviceVRD3D11* createOculus(IDXGIAdapter** outAdapter);
	static DeviceVRD3D11* createOpenVR(IDXGIAdapter** outAdapter);
};

class FramebufferD3D11;
class ShaderProgramD3D11;
class TextureD3D11;
class VertexLayoutD3D11;
class GeometryD3D11;

template<class Ty>
inline void ReleaseCheck(Ty*& object)
{
    if (object)
    {
        ULONG refCnt = object->Release();
#if !defined(RBX_PLATFORM_DURANGO) // on xbox, object->Release() always returns 1, just because the COM doc says Release() can return anything
        RBXASSERT(refCnt == 0);
#endif
        object = NULL;
    }
}

class DeviceContextD3D11: public DeviceContext
{
public:
	DeviceContextD3D11(Device* device, ID3D11DeviceContext* deviceContext11);
    ~DeviceContextD3D11();

    void defineGlobalConstants(size_t dataSize);
    unsigned getGlobalDataSize() { return globalDataSize; }

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

    void beginQuery(ID3D11Query* query);
    void endQuery(ID3D11Query* query);
    bool getQueryData(ID3D11Query* query, void* dataOut, size_t dataSize);

    virtual void pushDebugMarkerGroup(const char* text);
    virtual void popDebugMarkerGroup();
    virtual void setDebugMarker(const char* text);

    ID3D11DeviceContext* getContextDX11();

protected:
    struct TextureUnit
	{
        TextureD3D11* texture;
        SamplerState samplerState;

        TextureUnit()
			: texture(NULL)
			, samplerState(SamplerState::Filter_Point)
		{
		}
	};

    Device*                 device;
	ID3D11Device*           device11;
    ID3D11DeviceContext*    immediateContext11;

    ID3D11Buffer* globalsConstantBuffer;
    size_t globalDataSize;

    unsigned int defaultAnisotropy;

    Framebuffer* cachedFramebuffer;
	ShaderProgramD3D11* cachedProgram;
	VertexLayoutD3D11* cachedVertexLayout;
    GeometryD3D11* cachedGeometry;

    TextureUnit cachedTextureUnits[16];

    RasterizerState cachedRasterizerState;
	BlendState cachedBlendState;
    DepthState cachedDepthState;

    typedef boost::unordered_map<RasterizerState, ID3D11RasterizerState*, StateHasher<RasterizerState>> RasterizerStateHash;
    typedef boost::unordered_map<BlendState, ID3D11BlendState*, StateHasher<BlendState>> BlendStateHash;
    typedef boost::unordered_map<DepthState, ID3D11DepthStencilState*, StateHasher<DepthState>> DepthStateHash;
    typedef boost::unordered_map<SamplerState, ID3D11SamplerState*, StateHasher<SamplerState>> SamplerStateHash;

    RasterizerStateHash rasterizerStateHash;
    BlendStateHash blendStateHash;
    DepthStateHash depthStateHash;
    SamplerStateHash samplerStateHash;

    template <class tHash, class tState>
    void checkDuplicates(const tHash& hash, tState* state)
    {
        for (tHash::const_iterator it = hash.begin(); it != hash.end(); ++it)
		{
            RBXASSERT(state != it->second);
		}
    }

    // functions
    HMODULE d3d9;
    int (WINAPI *pfn_D3DPERF_BeginEvent)( DWORD col, LPCWSTR wszName);
    int (WINAPI *pfn_D3DPERF_EndEvent)();
    void (WINAPI *pfn_D3DPERF_SetMarker)( DWORD col, LPCWSTR wszName ); 
};

class DeviceD3D11: public Device
{
public:

    DeviceD3D11(void* windowHandle);
    ~DeviceD3D11();

    enum ShaderProfile
    {
        shaderProfile_DX11,
        shaderProfile_DX11_level_9_3
    };

    virtual bool validate();

    virtual DeviceContext* beginFrame();
    virtual void endFrame();

	virtual Framebuffer* getMainFramebuffer();

	virtual DeviceVR* getVR();
	virtual void setVR(bool enabled);

    virtual void defineGlobalConstants(size_t dataSize, const std::vector<ShaderGlobalConstant>& constants);

    virtual std::string getAPIName() { return "DirectX 11"; }
    virtual std::string getFeatureLevel(){ return shaderProfile == shaderProfile_DX11 ? "D3D11" : "D3D11_9.3"; }
    virtual std::string getShadingLanguage(){ return shaderProfile == shaderProfile_DX11 ? "hlsl11" : "hlsl11_level_9_3"; }
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

#ifdef RBX_PLATFORM_DURANGO
    virtual void suspend();
    virtual void resume();
#endif

    ID3D11Device* getDevice11() { return device11; }
    ShaderProfile getShaderProfile() const { return shaderProfile; }

    ID3D11DeviceContext* getImmediateContext11() { return immediateContext->getContextDX11(); }
    DeviceContextD3D11* getImmediateContextD3D11() { return immediateContext.get(); }

	void* getWindowHandle() const { return windowHandle; }

private:
    void* windowHandle;
    DeviceCaps caps;

    ID3D11Device* device11;
    IDXGISwapChain* swapChain11;
    scoped_ptr<DeviceContextD3D11> immediateContext;

    scoped_ptr<FramebufferD3D11> mainFramebuffer;

    void createMainFramebuffer(unsigned width, unsigned height);

    ShaderProfile shaderProfile;

    float gpuTime;

    ID3D11Query* beginQuery;
    ID3D11Query* endQuery;
    ID3D11Query* disjointQuery;
    bool frameTimeQueryIssued;

	scoped_ptr<DeviceVRD3D11> vr;
	bool vrEnabled;

    // these functions are platform-dependent
    void createDevice();
    void present();
    void resizeSwapchain();
    std::pair<unsigned int, unsigned int> getFramebufferSize();
};

}
}
