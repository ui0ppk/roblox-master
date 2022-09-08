#include "rbx/SystemUtil.h"
#include "rbx/ProcessPerfCounter.h"
#include "util/RegistryUtil.h"
#include "FastLog.h"

#include <windows.h>
#include <setupapi.h>

#include <initguid.h>
#include <d3d9.h>
#include "ddraw.h"

LOGVARIABLE(DXVideoMemory, 4)


namespace RBX
{
namespace SystemUtil
{
	std::string osVer()
    {
        OSVERSIONINFO osvi = {0};
        osvi.dwOSVersionInfoSize=sizeof(osvi);
        GetVersionEx (&osvi);
        return RBX::format("%d.%d.%d.%d", osvi.dwOSVersionInfoSize, osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);
	}

    bool isCPU64Bit()
    {
#if defined(_WIN64)
    return true;
#else
    BOOL result = false;
    return IsWow64Process(GetCurrentProcess(), &result) && result;
#endif
    }

	int osPlatformId()
	{
		return VER_PLATFORM_WIN32_NT;
	}

	std::string osPlatform()
	{
		return "Win32";
	}

	std::string deviceName()
	{
		return "PC";
	}

    std::string getGPUMake()
    {
        DISPLAY_DEVICE displayDevice;
        ZeroMemory(&displayDevice, sizeof(displayDevice));
        displayDevice.cb = sizeof(displayDevice);
        EnumDisplayDevices(NULL, 0, &displayDevice, 0);
        
        return std::string(displayDevice.DeviceString);
    }


    static DWORD GetDirecXVideoMemorySize() 
    {
        DWORD totalVidSize = 0;
        HRESULT hr = CoInitialize(NULL);

        if (FAILED(hr))
        {
            FASTLOG1(FLog::DXVideoMemory, "DXVRAM: CoInitialize returned %x", hr);
        }

        IDirectDraw2* ddraw = NULL;

        if ( FAILED(hr = CoCreateInstance( CLSID_DirectDraw, NULL, CLSCTX_INPROC_SERVER, IID_IDirectDraw2, reinterpret_cast<LPVOID*>(&ddraw))) )
        {
            FASTLOG1(FLog::DXVideoMemory, "DXVRAM: CoCreateInstance returned %x", hr);
        }
        else if ( FAILED(hr = ddraw->Initialize(0)) )
        {
            FASTLOG1(FLog::DXVideoMemory, "DXVRAM: DirectDraw::Initialize returned %x", hr);
        }
        else
        {
            DWORD freeVidSize = 0;

            DDSCAPS ddsCaps;
            memset(&ddsCaps, 0, sizeof(DDSCAPS));

            ddsCaps.dwCaps = DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;

            if (FAILED(hr = ddraw->GetAvailableVidMem(&ddsCaps, &totalVidSize, &freeVidSize)))
            {
                FASTLOG1(FLog::DXVideoMemory, "DXVRAM: GetAvailableVidMem (2D) returned %x, trying again", hr);

                ddsCaps.dwCaps |= DDSCAPS_3DDEVICE;

                if (FAILED(hr = ddraw->GetAvailableVidMem(&ddsCaps, &totalVidSize, &freeVidSize)))
                {
                    FASTLOG1(FLog::DXVideoMemory, "DXVRAM: GetAvailableVidMem (3D) returned %x, giving up", hr);

                    totalVidSize = 0;
                }
                else
                {
                    FASTLOG2(FLog::DXVideoMemory, "DXVRAM: GetAvailableVidMem (3D) succeeded, total %d, free %d", totalVidSize, freeVidSize);
                }
            }
            else
            {
                FASTLOG2(FLog::DXVideoMemory, "DXVRAM: GetAvailableVidMem (2D) succeeded, total %d, free %d", totalVidSize, freeVidSize);
            }

            ddraw->Release();
        }

        CoUninitialize();
        return totalVidSize;
    }

    uint64_t getVideoMemory()
    {
        static DWORD size = GetDirecXVideoMemorySize();
        return size;
    }
}
}
