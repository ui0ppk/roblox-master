#include "GL/DeviceGL.h"

#ifdef _WIN32
#include "D3D9/DeviceD3D9.h"
#include "D3D11/DeviceD3D11.h"
#endif

namespace RBX
{
namespace Graphics
{

Device* Device::create(API api, void* windowHandle)
{
#ifdef _WIN32
#if !defined(RBX_PLATFORM_DURANGO)
	if (api == API_Direct3D9)
        return new DeviceD3D9(windowHandle);
#endif

    if (api == API_Direct3D11)
        return new DeviceD3D11(windowHandle);
#endif

#if !defined(RBX_PLATFORM_DURANGO)
	if (api == API_OpenGL)
        return new DeviceGL(windowHandle);
#endif

	throw RBX::runtime_error("Unsupported API: %d", api);
}

}
}
