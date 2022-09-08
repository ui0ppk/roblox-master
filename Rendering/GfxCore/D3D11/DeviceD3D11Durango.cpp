#if defined(RBX_PLATFORM_DURANGO)
#include "DeviceD3D11.h"

#include "HeadersD3D11.h"
#include <wrl.h>

FASTFLAG(DebugD3D11DebugMode)

// This is a hacky bridge to get overscan-aware resolution from XboxClient
// There's no simple way to refactor this :(
std::pair<unsigned int, unsigned int> xboxPlatformGetRenderSize_Hack(void* h);

namespace RBX {
namespace Graphics {

    IDXGISwapChain* createSwapchain(ID3D11Device* dev, unsigned w, unsigned h)
    {
        HRESULT hr;
        IDXGISwapChain1* swapch = 0;

        IDXGIDevice1* spdxgiDevice = 0;
        dev->QueryInterface( IID_IDXGIDevice1, reinterpret_cast<void**>(&spdxgiDevice) );

        IDXGIAdapter* spdxgiAdapter = 0;
        spdxgiDevice->GetAdapter( &spdxgiAdapter );

        IDXGIFactory2* spdxgiFactory = 0;
        spdxgiAdapter->GetParent( IID_IDXGIFactory2, reinterpret_cast<void**>(&spdxgiFactory) );


        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
        swapChainDesc.Width  = w;
        swapChainDesc.Height = h;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = false; 
        swapChainDesc.SampleDesc.Count = 1;                         // don't use multi-sampling
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2;            // use two buffers to enable flip effect
        swapChainDesc.Scaling    = DXGI_SCALING_STRETCH;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
        hr = spdxgiFactory->CreateSwapChainForCoreWindow(dev, (IUnknown*)Windows::UI::Core::CoreWindow::GetForCurrentThread(), &swapChainDesc, nullptr, &swapch);

        spdxgiFactory->Release();
        spdxgiAdapter->Release();
        spdxgiDevice->Release();

        if (FAILED(hr))
            throw RBX::runtime_error("failed to create swapchain: %x", hr);

        return swapch;
    }

    void DeviceD3D11::suspend()
    {
        ID3D11DeviceContextX* contextx = static_cast<ID3D11DeviceContextX*>(immediateContext->getContextDX11());
        contextx->Suspend(0);
    }

    void DeviceD3D11::resume()
    {
        ID3D11DeviceContextX* contextx = static_cast<ID3D11DeviceContextX*>(immediateContext->getContextDX11());
        contextx->Resume();
    }

    void DeviceD3D11::createDevice()
    {
        ID3D11DeviceX* dev;
        ID3D11DeviceContextX* ctx;

        UINT flags = 0;
        if( FFlag::DebugD3D11DebugMode )
        {
            flags |= D3D11_CREATE_DEVICE_INSTRUMENTED | D3D11_CREATE_DEVICE_VALIDATED | D3D11_CREATE_DEVICE_DEBUG;
        }

        D3D11X_CREATE_DEVICE_PARAMETERS params = D3D11X_CREATE_DEVICE_PARAMETERS();
        params.Flags = flags;
        params.Version = D3D11_SDK_VERSION;
        params.DeferredDeletionThreadAffinityMask = 0x20; // default value

        HRESULT hr = D3D11XCreateDeviceX(&params, &dev, &ctx);

        if (FAILED(hr))
            throw RBX::runtime_error("Unable to create D3D device: %x", hr);

        std::pair<unsigned int, unsigned int> fbsize = getFramebufferSize();

        this->device11 = dev;
        this->immediateContext.reset(new DeviceContextD3D11(this, ctx));
        this->shaderProfile = shaderProfile_DX11;
        this->swapChain11 = createSwapchain(dev, fbsize.first, fbsize.second);
    }

    void DeviceD3D11::present()
    {
        unsigned centerX = 1920/2, centerY = 1080/2; // for now, assume that the native resolution of the TV is fullhd and we're simply dealing with overscan
        std::pair<unsigned, unsigned> sz = getFramebufferSize();
        D3D11_RECT src = { 0, 0, sz.first, sz.second };
        POINT dest = { centerX - sz.first/2, centerY - sz.second/2 };

        DXGIX_PRESENTARRAY_PARAMETERS pp = {};
        pp.SourceRect = src;
        pp.DestRectUpperLeft = dest;
        pp.ScaleFactorHorz = 1.0f;
        pp.ScaleFactorVert = 1.0f;
        pp.Flags = 0;

        DXGIXPresentArray(1, 0, 0, 1, (IDXGISwapChain1**)&swapChain11, &pp);
    }

    void DeviceD3D11::resizeSwapchain()
    {
        std::pair<unsigned int, unsigned int> dimensions = getFramebufferSize();
        ReleaseCheck(swapChain11);
        swapChain11 = createSwapchain(device11, dimensions.first, dimensions.second );
        RBXASSERT(swapChain11);
    }

    std::pair<unsigned int, unsigned int> DeviceD3D11::getFramebufferSize()
    {
        return xboxPlatformGetRenderSize_Hack(windowHandle);
    }

}}
#endif
