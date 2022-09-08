
#include "DeviceD3D11.h"

#include "GeometryD3D11.h"
#include "ShaderD3D11.h"
#include "TextureD3D11.h"
#include "FramebufferD3D11.h"

#include "rbx/rbxTime.h"
#include "rbx/Profiler.h"

#include "StringConv.h"

#include "HeadersD3D11.h"

LOGGROUP(Graphics)
LOGGROUP(VR)

FASTFLAGVARIABLE(DebugD3D11DebugMode, false)

namespace RBX
{
namespace Graphics
{
    static unsigned int getMaxSamplesSupported(ID3D11Device* device11)
    {
        unsigned int result = 1;

        for (unsigned int mode = 2; mode <= D3D11_MAX_MULTISAMPLE_SAMPLE_COUNT; mode *= 2)
        {
            unsigned maxQualityLevel;
            HRESULT hr = device11->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, mode, &maxQualityLevel);
            RBXASSERT(SUCCEEDED(hr));
            if (maxQualityLevel <= 0)
                break;
            
            result = mode;
        }

        return result;        
    }

	static DXGI_ADAPTER_DESC getAdapterDesc(IDXGIDevice* device)
	{
		IDXGIAdapter* adapter = NULL;
		device->GetAdapter(&adapter);

		DXGI_ADAPTER_DESC desc = {};
		adapter->GetDesc(&desc);

		adapter->Release();

		return desc;
	}

	template <typename T, typename P> T* queryInterface(P* object)
	{
		void* result = 0;
		if (SUCCEEDED(object->QueryInterface(__uuidof(T), &result)))
			return static_cast<T*>(result);

		return 0;
	}
    
    DeviceD3D11::DeviceD3D11(void* windowHandle)
		: windowHandle(windowHandle)
        , device11(NULL)
        , swapChain11(NULL)
        , immediateContext(NULL)
        , frameTimeQueryIssued(false)
        , gpuTime(0)
        , beginQuery(NULL)
        , endQuery(NULL)
        , disjointQuery(NULL)
		, vrEnabled(true)
    {
        createDevice();

        caps = DeviceCaps();

        caps.supportsFramebuffer = true;
        caps.supportsShaders = true;
        caps.supportsFFP = false;
        caps.supportsStencil = true;
        caps.supportsIndex32 = true;

        caps.supportsTextureDXT = true;
        caps.supportsTexturePVR = false;
        caps.supportsTextureHalfFloat = true; //DXGI_FORMAT_R16G16B16A16_FLOAT is always supported on DX11 HW (including featureLvl 10 - 9.3)
        caps.supportsTexture3D = true;
        caps.supportsTextureNPOT = shaderProfile == shaderProfile_DX11_level_9_3 ? false : true;
        caps.supportsTextureETC1 = false;

		caps.supportsTexturePartialMipChain = true;

        caps.maxDrawBuffers = shaderProfile == shaderProfile_DX11_level_9_3 ? 4 : 8;
        caps.maxSamples = getMaxSamplesSupported(device11);
        caps.maxTextureSize = D3D11_REQ_TEXTURE2D_U_OR_V_DIMENSION;
		caps.maxTextureUnits = 16;

        caps.colorOrderBGR = false;
        caps.needsHalfPixelOffset = false;
        caps.requiresRenderTargetFlipping = false;

        caps.retina = false;

        std::pair<unsigned int, unsigned int> dimensions = getFramebufferSize();
        createMainFramebuffer(dimensions.first, dimensions.second);

        // queries
        HRESULT hr;

        D3D11_QUERY_DESC queryDesc;
        queryDesc.MiscFlags = 0;

        queryDesc.Query = D3D11_QUERY_TIMESTAMP;
        hr = device11->CreateQuery(&queryDesc, &beginQuery);
        RBXASSERT(SUCCEEDED(hr));
        hr = device11->CreateQuery(&queryDesc, &endQuery);
        RBXASSERT(SUCCEEDED(hr));

        queryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        hr = device11->CreateQuery(&queryDesc, &disjointQuery);
        RBXASSERT(SUCCEEDED(hr));

		if (IDXGIDevice1* deviceDXGI1 = queryInterface<IDXGIDevice1>(device11))
		{
			deviceDXGI1->SetMaximumFrameLatency(1);
			deviceDXGI1->Release();
		}

		if (IDXGIDevice* deviceDXGI = queryInterface<IDXGIDevice>(device11))
		{
			DXGI_ADAPTER_DESC desc = getAdapterDesc(deviceDXGI);

			FASTLOGS(FLog::Graphics, "D3D11 Adapter: %s", utf8_encode(desc.Description));
			FASTLOG2(FLog::Graphics, "D3D11 Adapter: Vendor %04x Device %04x", desc.VendorId, desc.DeviceId);

			// Do not use GPU profiling on Intel cards to avoid a crash in Intel driver when releasing resources in gpuShutdown
			// Also, this vendor id is SO COOL!
			if (desc.VendorId != 0x8086)
				Profiler::gpuInit(getImmediateContext11());

			deviceDXGI->Release();
		}

		if (vr)
		{
			try
			{
				vr->setup(this);
			}
			catch (RBX::base_exception& e)
			{
				FASTLOGS(FLog::VR, "VR ERROR during setup: %s", e.what());
				vr.reset();
			}
		}
    }


    void DeviceD3D11::createMainFramebuffer(unsigned width, unsigned height)
    {
        RBXASSERT(!mainFramebuffer);

        // Get back buffer, create view
        ID3D11Texture2D* backBuffer = NULL;
        HRESULT hr = swapChain11->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&backBuffer );
        RBXASSERT(SUCCEEDED(hr));

        shared_ptr<Renderbuffer> backBufferRB = shared_ptr<Renderbuffer>(new RenderbufferD3D11(this, Texture::Format_RGBA8, width, height, 1, backBuffer));
        std::vector<shared_ptr<Renderbuffer>> colorBuffers;
        colorBuffers.push_back(backBufferRB);

        // Create depth stencil
        shared_ptr<Renderbuffer> depthStencil = shared_ptr<Renderbuffer>(new RenderbufferD3D11(this, Texture::Format_D24S8, width, height, 1));

        // create frame buffer
        mainFramebuffer.reset(new FramebufferD3D11(this, colorBuffers, depthStencil));
    }

    DeviceD3D11::~DeviceD3D11()
    {
		Profiler::gpuShutdown();

        // we want to release all the DX references before releasing device to be able to test if nothing is hanging
		vr.reset();
        immediateContext.reset();
        mainFramebuffer.reset();

        ReleaseCheck(beginQuery);
        ReleaseCheck(endQuery);
        ReleaseCheck(disjointQuery);

        ReleaseCheck(swapChain11);
        ReleaseCheck(device11);
    }

    void DeviceD3D11::defineGlobalConstants(size_t dataSize, const std::vector<ShaderGlobalConstant>& constants)
    {
        RBXASSERT(!constants.empty());

        // Since constants are directly set to register values, we impose additional restrictions on constant data
        // The struct should be an integer number of float4 registers, and every constant has to be aligned to float4 boundary
        RBXASSERT(dataSize % 16 == 0);

        immediateContext->defineGlobalConstants(dataSize);
    }

    DeviceContext* DeviceD3D11::beginFrame()
    {
        std::pair<unsigned int, unsigned int> dimensions = getFramebufferSize();

		// Don't render anything if there is no main framebuffer or device is lost
		if (!mainFramebuffer)
			return false;

        // Don't render anything if window size changed; wait for validate
        if (dimensions.first != mainFramebuffer->getWidth() || dimensions.second != mainFramebuffer->getHeight())
            return NULL;

        immediateContext->bindFramebuffer(mainFramebuffer.get());
        immediateContext->clearStates();

        if (disjointQuery && beginQuery && endQuery)
        {
            if (!frameTimeQueryIssued)
            {
                immediateContext->beginQuery(disjointQuery);
                immediateContext->endQuery(beginQuery);
            }
        }
        
        return immediateContext.get();
    }

    bool DeviceD3D11::validate()
    {
        std::pair<unsigned int, unsigned int> dimensions = getFramebufferSize();

		// Don't change anything if window is minimized (getFramebufferSize returns 1x1)
		if (dimensions.first <= 1 && dimensions.second <= 1)
			return false;

        // Reset device if window size changed
        if (mainFramebuffer && (dimensions.first != mainFramebuffer->getWidth() || dimensions.second != mainFramebuffer->getHeight()))
        {
            immediateContext->getContextDX11()->OMSetRenderTargets(NULL, NULL, NULL);

            mainFramebuffer.reset();
            resizeSwapchain();
            createMainFramebuffer(dimensions.first, dimensions.second);
        }

        return true;
    }

    void DeviceD3D11::endFrame()
    {
        if (disjointQuery && beginQuery && endQuery)
        {
            if (!frameTimeQueryIssued)
            {
                immediateContext->endQuery(endQuery);
                immediateContext->endQuery(disjointQuery);
                frameTimeQueryIssued = true;
            }
            else
            {
                UINT64 tsBeginFrame, tsEndFrame;
                D3D11_QUERY_DATA_TIMESTAMP_DISJOINT tsDisjoint;

                bool queryFinished = immediateContext->getQueryData(disjointQuery, &tsDisjoint, sizeof(tsDisjoint));
                queryFinished &= immediateContext->getQueryData(beginQuery, &tsBeginFrame, sizeof(UINT64));
                queryFinished &= immediateContext->getQueryData(endQuery, &tsEndFrame, sizeof(UINT64));

                if (queryFinished)
                {
                    if (!tsDisjoint.Disjoint)
                        gpuTime = (float)(double(tsEndFrame - tsBeginFrame) / double(tsDisjoint.Frequency) * 1000.0);
                    frameTimeQueryIssued = false;
                }
            }
        }

		if (vr && vrEnabled)
		{
			vr->submitFrame(immediateContext.get());
        }

        present();
    }

    DeviceStats DeviceD3D11::getStatistics() const
    {
        DeviceStats result = {};

        result.gpuFrameTime = gpuTime;

        return result;
    }

    shared_ptr<Texture> DeviceD3D11::createTexture(Texture::Type type, Texture::Format format, unsigned int width, unsigned int height, unsigned int depth, unsigned int mipLevels, Texture::Usage usage)
    {      
        return shared_ptr<Texture>(new TextureD3D11(this, type, format, width, height, depth, mipLevels, usage));
    }

    shared_ptr<VertexBuffer> DeviceD3D11::createVertexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage)
    {
        return shared_ptr<VertexBuffer>(new VertexBufferD3D11(this, elementSize, elementCount, usage));
    }

    shared_ptr<IndexBuffer> DeviceD3D11::createIndexBuffer(size_t elementSize, size_t elementCount, GeometryBuffer::Usage usage)
    {
        return shared_ptr<IndexBuffer>(new IndexBufferD3D11(this, elementSize, elementCount, usage));
    }

    shared_ptr<VertexLayout> DeviceD3D11::createVertexLayout(const std::vector<VertexLayout::Element>& elements)
    {
        return shared_ptr<VertexLayout>(new VertexLayoutD3D11(this, elements));
    }

    shared_ptr<Geometry> DeviceD3D11::createGeometryImpl(const shared_ptr<VertexLayout>& layout, const std::vector<shared_ptr<VertexBuffer> >& vertexBuffers, const shared_ptr<IndexBuffer>& indexBuffer, unsigned int baseVertexIndex)
    {
        return shared_ptr<Geometry>(new GeometryD3D11(this, layout, vertexBuffers, indexBuffer, baseVertexIndex));
    }

    shared_ptr<Framebuffer> DeviceD3D11::createFramebufferImpl(const std::vector<shared_ptr<Renderbuffer> >& color, const shared_ptr<Renderbuffer>& depth)
    {
        return shared_ptr<Framebuffer>(new FramebufferD3D11(this, color, depth));
    }

    Framebuffer* DeviceD3D11::getMainFramebuffer()
    { 
        return mainFramebuffer.get(); 
    }

	DeviceVR* DeviceD3D11::getVR()
	{
		return (vr && vrEnabled) ? vr.get() : NULL;
	}

	void DeviceD3D11::setVR(bool enabled)
	{
		vrEnabled = enabled;
	}

    shared_ptr<Renderbuffer> DeviceD3D11::createRenderbuffer(Texture::Format format, unsigned int width, unsigned int height, unsigned int samples)
    {
        return shared_ptr<Renderbuffer>(new RenderbufferD3D11(this, format, width, height, samples));
    }

    std::string DeviceD3D11::createShaderSource(const std::string& path, const std::string& defines, boost::function<std::string (const std::string&)> fileCallback)
    {
        std::string dx11Defines = defines;
        dx11Defines += " DX11";

        if (getShaderProfile() == DeviceD3D11::shaderProfile_DX11_level_9_3)
            dx11Defines += " WIN_MOBILE";

        return ShaderProgramD3D11::createShaderSource(path, dx11Defines, this, fileCallback);
    }

    std::vector<char> DeviceD3D11::createShaderBytecode(const std::string& source, const std::string& target, const std::string& entrypoint)
    {
        return ShaderProgramD3D11::createShaderBytecode(source, target, this, entrypoint);
    }

    shared_ptr<VertexShader> DeviceD3D11::createVertexShader(const std::vector<char>& bytecode)
    {
        return shared_ptr<VertexShader>(new VertexShaderD3D11(this, bytecode));
    }

    shared_ptr<FragmentShader> DeviceD3D11::createFragmentShader(const std::vector<char>& bytecode)
    {
        return shared_ptr<FragmentShader>(new FragmentShaderD3D11(this, bytecode));
    }

    shared_ptr<ShaderProgram> DeviceD3D11::createShaderProgram(const shared_ptr<VertexShader>& vertexShader, const shared_ptr<FragmentShader>& fragmentShader)
    {
        return shared_ptr<ShaderProgram>(new ShaderProgramD3D11(this, vertexShader, fragmentShader));
    }

    shared_ptr<ShaderProgram> DeviceD3D11::createShaderProgramFFP()
    {
        throw RBX::runtime_error("No FFP support");
    }

}
}
