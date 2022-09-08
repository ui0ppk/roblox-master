#if !defined(RBX_PLATFORM_DURANGO) && !defined(RBX_PLATFORM_UWP)
#include "DeviceD3D11.h"

#include "ShaderD3D11.h"

#include "HeadersD3D11.h"

FASTFLAG(DebugD3D11DebugMode)

FASTFLAG(RenderVR)

namespace RBX
{
namespace Graphics
{

    typedef HRESULT (WINAPI *TypeD3D11CreateDeviceAndSwapChain)(IDXGIAdapter *, D3D_DRIVER_TYPE, HMODULE, unsigned, const D3D_FEATURE_LEVEL*, unsigned, unsigned, const DXGI_SWAP_CHAIN_DESC *, IDXGISwapChain **, ID3D11Device **, D3D_FEATURE_LEVEL *, ID3D11DeviceContext **);
    
    static TypeD3D11CreateDeviceAndSwapChain getDeviceCreationFunction()
    {
        HMODULE module = LoadLibraryA("d3d11.dll");
        if (!module) return NULL;

        return (TypeD3D11CreateDeviceAndSwapChain)GetProcAddress(module, "D3D11CreateDeviceAndSwapChain");
    }

	static DeviceVRD3D11* createVR(IDXGIAdapter** outAdapter)
	{
		if (DeviceVRD3D11* result = DeviceVRD3D11::createOculus(outAdapter))
			return result;

		if (DeviceVRD3D11* result = DeviceVRD3D11::createOpenVR(outAdapter))
			return result;

		return NULL;
	}

    void DeviceD3D11::createDevice()
    {
		IDXGIAdapter* adapter = NULL;

		if (FFlag::RenderVR)
			vr.reset(createVR(&adapter));
		
        TypeD3D11CreateDeviceAndSwapChain createDeviceAndSwapChain = getDeviceCreationFunction();
        if (!createDeviceAndSwapChain)
            throw std::runtime_error("Unable to load d3d11.dll");

        this->windowHandle = windowHandle;

        unsigned deviceCreationFlags = FFlag::DebugD3D11DebugMode ? D3D11_CREATE_DEVICE_DEBUG : 0;

        D3D_FEATURE_LEVEL requestedFeatureLevel = D3D_FEATURE_LEVEL_11_0;

        D3D_FEATURE_LEVEL featureLevelOut;

		std::pair<unsigned int, unsigned int> dimensions = getFramebufferSize();

        DXGI_SWAP_CHAIN_DESC sd = {};
        sd.BufferCount = 1;
        sd.BufferDesc.Width = dimensions.first;
        sd.BufferDesc.Height = dimensions.second;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 0;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = (HWND)windowHandle;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = true;
        sd.Flags = 0;
        sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

        ID3D11DeviceContext* deviceContext = NULL;
        HRESULT hr = createDeviceAndSwapChain(adapter, D3D_DRIVER_TYPE_HARDWARE, NULL, deviceCreationFlags, &requestedFeatureLevel, 1,
            D3D11_SDK_VERSION, &sd, &swapChain11, &device11, &featureLevelOut, &deviceContext);
        
        if (FAILED(hr))
            throw RBX::runtime_error("Unable to create D3D device: %x", hr);

        shaderProfile = featureLevelOut == D3D_FEATURE_LEVEL_11_0 ? shaderProfile_DX11 : shaderProfile_DX11_level_9_3;

        HMODULE shaderCompiler = ShaderProgramD3D11::loadShaderCompilerDLL();
        if (!shaderCompiler)
            throw std::runtime_error("Unable to load shader compiler dll");

        immediateContext.reset(new DeviceContextD3D11(this, deviceContext));
    }

    void DeviceD3D11::present()
    {
        swapChain11->Present(0,0);
    }

    void DeviceD3D11::resizeSwapchain()
    {
        HRESULT hr = swapChain11->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
        RBXASSERT(SUCCEEDED(hr));
    }

    std::pair<unsigned int, unsigned int> DeviceD3D11::getFramebufferSize()
    {
        RECT rect = {};
        GetClientRect((HWND)windowHandle, &rect);

        unsigned int width = std::max(rect.right - rect.left, 1l);
        unsigned int height = std::max(rect.bottom - rect.top, 1l);

        return std::make_pair(width, height);
    }

}
}
#endif
