#if !defined(RBX_PLATFORM_DURANGO) && !defined(RBX_PLATFORM_UWP)
#include "DeviceD3D11.h"

#include "FramebufferD3D11.h"

#include <d3d11.h>

LOGGROUP(VR)

FASTFLAGVARIABLE(DebugRenderVRHUD, false)

#include "../Rendering/LibOVR/Include/OVR_CAPI_D3D.h"

#pragma comment(lib, "../Rendering/LibOVR/Lib/Windows/Win32/Release/VS2012/LibOVR.lib")

#define OVR_CHECK(call) \
	do { \
		ovrResult vrResult = call; \
		if (OVR_FAILURE(vrResult)) FASTLOG1(FLog::VR, "VR ERROR: " #call " returned %d", vrResult); \
	} while (0)

namespace RBX
{
namespace Graphics
{
	typedef HRESULT (WINAPI *TypeCreateDXGIFactory)(REFIID riid, void **ppFactory);

	static TypeCreateDXGIFactory getFactoryCreationFunction()
    {
        HMODULE module = LoadLibraryA("d3d11.dll");
        if (!module) return NULL;

		return (TypeCreateDXGIFactory)GetProcAddress(module, "CreateDXGIFactory");
    }

	static IDXGIAdapter* getAdapterForLuid(LUID luid)
	{
        // Allow to use null/default adapter for applications that may render a window without an HMD.
		if ((luid.HighPart | luid.LowPart) == 0)
			return NULL;

		TypeCreateDXGIFactory createFactory = getFactoryCreationFunction();
		if (!createFactory)
			return NULL;

		// Try to find adapter by LUID
		IDXGIFactory* factory = NULL;
		if (FAILED(createFactory(__uuidof(IDXGIFactory), (void**)&factory)))
			return NULL;

		UINT index = 0;

        for (;;)
        {
            IDXGIAdapter* adapter = NULL;

			if (SUCCEEDED(factory->EnumAdapters(index, &adapter)) && adapter)
			{
				DXGI_ADAPTER_DESC desc;

				if (SUCCEEDED(adapter->GetDesc(&desc)) && desc.AdapterLuid.HighPart == luid.HighPart && desc.AdapterLuid.LowPart == luid.LowPart)
				{
					ReleaseCheck(factory);

					return adapter;
				}
			}
			else
				break;

			index++;
		}

		ReleaseCheck(factory);

		return NULL;
    }

	static DeviceVR::Pose getPose(const ovrPosef& pose, unsigned int statusFlags)
	{
		DeviceVR::Pose result = {};

		result.valid = (statusFlags & ovrStatus_OrientationTracked) != 0;

		result.position[0] = pose.Position.x;
		result.position[1] = pose.Position.y;
		result.position[2] = pose.Position.z;

		result.orientation[0] = pose.Orientation.x;
		result.orientation[1] = pose.Orientation.y;
		result.orientation[2] = pose.Orientation.z;
		result.orientation[3] = pose.Orientation.w;

		return result;
	}

	struct VRTexture
	{
		static const int kMaxCount = 4;

		shared_ptr<Framebuffer> fb[kMaxCount];
		ovrSwapTextureSet* textureSet;

		VRTexture(): textureSet(NULL)
		{
		}
	};

	struct OculusVRD3D11: DeviceVRD3D11
	{
		ovrSession session;
		ovrHmdDesc desc;
		VRTexture textures[2];
		ovrEyeRenderDesc eyeDesc[2];

		double sensorSampleTime;
		ovrTrackingState trackingState;

		OculusVRD3D11(): session(NULL), sensorSampleTime(0)
		{
		}

		~OculusVRD3D11()
		{
			for (int eye = 0; eye < 2; ++eye)
				ovr_DestroySwapTextureSet(session, textures[eye].textureSet);

			ovr_Destroy(session);
			ovr_Shutdown();
		}

		void update() override
		{
			double frameTime = ovr_GetPredictedDisplayTime(session, 0);

			// Keeping sensorSampleTime as close to ovr_GetTrackingState as possible - fed into the layer
			sensorSampleTime = ovr_GetTimeInSeconds();
			trackingState = ovr_GetTrackingState(session, frameTime, ovrTrue);
		}

		void recenter() override
		{
			ovr_RecenterPose(session);
		}

		Framebuffer* getEyeFramebuffer(int eye) override
		{
			RBXASSERT(eye == 0 || eye == 1);

			return textures[eye].fb[textures[eye].textureSet->CurrentIndex].get();
		}

		State getState() override
		{
			State result = {};

			result.headPose = getPose(trackingState.HeadPose.ThePose, trackingState.StatusFlags);
			result.handPose[0] = getPose(trackingState.HandPoses[0].ThePose, trackingState.HandStatusFlags[0]);
			result.handPose[1] = getPose(trackingState.HandPoses[1].ThePose, trackingState.HandStatusFlags[1]);

			for (int eye = 0; eye < 2; ++eye)
			{
				result.eyeOffset[eye][0] = eyeDesc[eye].HmdToEyeViewOffset.x;
				result.eyeOffset[eye][1] = eyeDesc[eye].HmdToEyeViewOffset.y;
				result.eyeOffset[eye][2] = eyeDesc[eye].HmdToEyeViewOffset.z;

				result.eyeFov[eye][0] = eyeDesc[eye].Fov.UpTan;
				result.eyeFov[eye][1] = eyeDesc[eye].Fov.DownTan;
				result.eyeFov[eye][2] = eyeDesc[eye].Fov.LeftTan;
				result.eyeFov[eye][3] = eyeDesc[eye].Fov.RightTan;
			}

			result.needsMirror = true;

			return result;
		}

		void submitFrame(DeviceContext* context) override
		{
			ovrLayerEyeFov ld = {};
			ld.Header.Type = ovrLayerType_EyeFov;
			ld.Header.Flags = 0;

			ovrVector3f hmdToEyeViewOffset[2] = { eyeDesc[0].HmdToEyeViewOffset, eyeDesc[1].HmdToEyeViewOffset };
			ovrPosef eyeRenderPoses[2];
			ovr_CalcEyePoses(trackingState.HeadPose.ThePose, hmdToEyeViewOffset, eyeRenderPoses);

			for (int eye = 0; eye < 2; ++eye)
			{
				ld.ColorTexture[eye] = textures[eye].textureSet;
				ld.Viewport[eye].Pos.x = 0;
				ld.Viewport[eye].Pos.y = 0;
				ld.Viewport[eye].Size.w = textures[eye].fb[0]->getWidth();
				ld.Viewport[eye].Size.h = textures[eye].fb[0]->getHeight();
				ld.Fov[eye] = eyeDesc[eye].Fov;
				ld.RenderPose[eye] = eyeRenderPoses[eye];
				ld.SensorSampleTime = sensorSampleTime;
			}

			ovrViewScaleDesc viewScaleDesc;
			viewScaleDesc.HmdSpaceToWorldScaleInMeters = 1.0f;
			viewScaleDesc.HmdToEyeViewOffset[0] = eyeDesc[0].HmdToEyeViewOffset;
			viewScaleDesc.HmdToEyeViewOffset[1] = eyeDesc[1].HmdToEyeViewOffset;

			ovrLayerHeader* layers = &ld.Header;
			OVR_CHECK(ovr_SubmitFrame(session, 0, &viewScaleDesc, &layers, 1));

			for (int eye = 0; eye < 2; ++eye)
			{
				textures[eye].textureSet->CurrentIndex += 1;
				textures[eye].textureSet->CurrentIndex %= textures[eye].textureSet->TextureCount;
			}

			if (FFlag::DebugRenderVRHUD)
			{
				static bool keyWasDown = false;
				bool keyIsDown = (GetAsyncKeyState(VK_CONTROL) < 0 && GetAsyncKeyState(VK_F8) < 0);

				if (keyIsDown && !keyWasDown)
				{
					int perfHudMode = ovr_GetInt(session, OVR_PERF_HUD_MODE, 0);

					perfHudMode += 1;
					perfHudMode %= ovrPerfHud_Count;
					
					ovr_SetInt(session, OVR_PERF_HUD_MODE, perfHudMode);
				}

				keyWasDown = keyIsDown;
			}
		}

		void setup(Device* device) override
		{
			ID3D11Device* device11 = static_cast<DeviceD3D11*>(device)->getDevice11();

			// Work around a bug in LibOVR
			ShowWindow(static_cast<HWND>(static_cast<DeviceD3D11*>(device)->getWindowHandle()), SW_SHOWNORMAL);

			ovrSizei idealSizeLeft = ovr_GetFovTextureSize(session, ovrEye_Left, desc.DefaultEyeFov[ovrEye_Left], 1.0f);
			ovrSizei idealSizeRight = ovr_GetFovTextureSize(session, ovrEye_Right, desc.DefaultEyeFov[ovrEye_Right], 1.0f);

			unsigned int width = std::max(idealSizeLeft.w, idealSizeRight.w);
			unsigned int height = std::max(idealSizeLeft.h, idealSizeRight.h);

			shared_ptr<Renderbuffer> depthStencil = shared_ptr<Renderbuffer>(new RenderbufferD3D11(device, Texture::Format_D24S8, width, height, 1));

			for (int eye = 0; eye < 2; ++eye)
			{
				D3D11_TEXTURE2D_DESC dsDesc;
				dsDesc.Width = width;
				dsDesc.Height = height;
				dsDesc.MipLevels = 1;
				dsDesc.ArraySize = 1;
				dsDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				dsDesc.SampleDesc.Count = 1;
				dsDesc.SampleDesc.Quality = 0;
				dsDesc.Usage = D3D11_USAGE_DEFAULT;
				dsDesc.CPUAccessFlags = 0;
				dsDesc.MiscFlags = 0;
				dsDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;

				ovrResult result = ovr_CreateSwapTextureSetD3D11(session, device11, &dsDesc, ovrSwapTextureSetD3D11_Typeless, &textures[eye].textureSet);
				if (OVR_FAILURE(result))
					throw RBX::runtime_error("ovr_CreateSwapTextureSetD3D11 failed with %d", result);

				RBXASSERT(textures[eye].textureSet->TextureCount <= VRTexture::kMaxCount);

				for (int i = 0; i < textures[eye].textureSet->TextureCount; ++i)
				{
					ovrD3D11Texture* tex = reinterpret_cast<ovrD3D11Texture*>(&textures[eye].textureSet->Textures[i]);

					tex->D3D11.pTexture->AddRef();

					shared_ptr<Renderbuffer> colorBuffer(new RenderbufferD3D11(device, Texture::Format_RGBA8, width, height, 1, tex->D3D11.pTexture));

					textures[eye].fb[i] = device->createFramebuffer(colorBuffer, depthStencil);
				}
			}
		}
	};

	static void logCallback(uintptr_t userData, int level, const char* message)
	{
		FASTLOGS(FLog::VR, "VR: %s", message);
	}

	DeviceVRD3D11* DeviceVRD3D11::createOculus(IDXGIAdapter** outAdapter)
	{
		ovrInitParams params = {};

		params.LogCallback = logCallback;

		ovrResult vrResult = ovr_Initialize(&params);

		if (!OVR_SUCCESS(vrResult))
		{
			FASTLOG1(FLog::VR, "VR: ovr_Initialize returned %d", vrResult);
			return NULL;
		}

		ovrSession session;
		ovrGraphicsLuid vrLuid;

		vrResult = ovr_Create(&session, &vrLuid);

		if (!OVR_SUCCESS(vrResult))
		{
			FASTLOG1(FLog::VR, "VR: ovr_Create returned %d", vrResult);

			ovr_Shutdown();

			return NULL;
		}

		OculusVRD3D11* vr = new OculusVRD3D11();

		vr->session = session;
		vr->desc = ovr_GetHmdDesc(session);

		for (int eye = 0; eye < 2; ++eye)
			vr->eyeDesc[eye] = ovr_GetRenderDesc(session, (ovrEyeType)eye, vr->desc.DefaultEyeFov[eye]);

		if (outAdapter)
			*outAdapter = getAdapterForLuid(*reinterpret_cast<LUID*>(&vrLuid));

		FASTLOGS(FLog::VR, "VR: Connected to %s", vr->desc.ProductName);
		FASTLOG4(FLog::VR, "VR: Vendor %x Product %x Firmware %d.%d", vr->desc.VendorId, vr->desc.ProductId, vr->desc.FirmwareMajor, vr->desc.FirmwareMinor);

		return vr;
	}

}
}
#endif
