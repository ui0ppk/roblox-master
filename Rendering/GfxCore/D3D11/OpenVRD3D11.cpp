#if !defined(RBX_PLATFORM_DURANGO) && !defined(RBX_PLATFORM_UWP)
#include "DeviceD3D11.h"

#include "FramebufferD3D11.h"

#include "G3D/Quat.h"

#include "rbx/Profiler.h"

#include <d3d11.h>

LOGGROUP(VR)

FASTFLAGVARIABLE(OpenVR, true)

#include "../Rendering/OpenVR/headers/openvr.h"

using namespace vr;

#define OVR_CHECK(call) \
	do { \
		int vrResult = call; \
		if (vrResult) FASTLOG1(FLog::VR, "VR ERROR: " #call " returned %d", vrResult); \
	} while (0)

namespace RBX
{
namespace Graphics
{
	using G3D::Quat;
	using G3D::Vector3;
	using G3D::Matrix3;

	typedef HRESULT (WINAPI *TypeCreateDXGIFactory)(REFIID riid, void **ppFactory);

	static TypeCreateDXGIFactory getFactoryCreationFunction()
    {
        HMODULE module = LoadLibraryA("d3d11.dll");
        if (!module) return NULL;

		return (TypeCreateDXGIFactory)GetProcAddress(module, "CreateDXGIFactory");
    }

	template <typename F> static F getOpenVRFunction(const char* name)
	{
        HMODULE module = LoadLibraryA("openvr_api.dll");
		if (!module) return NULL;

		return F(GetProcAddress(module, name));
	}

	static IDXGIAdapter* getAdapterForIndex(int index)
	{
		TypeCreateDXGIFactory createFactory = getFactoryCreationFunction();
		if (!createFactory)
			return NULL;

		// Try to find adapter by LUID
		IDXGIFactory* factory = NULL;
		if (FAILED(createFactory(__uuidof(IDXGIFactory), (void**)&factory)))
			return NULL;

		IDXGIAdapter* adapter = NULL;
		factory->EnumAdapters(index, &adapter);

		ReleaseCheck(factory);

		return adapter;
    }

	static std::string getStringProperty(IVRSystem* system, TrackedDeviceIndex_t device, TrackedDeviceProperty property)
	{
		char buf[128] = {};
		system->GetStringTrackedDeviceProperty(device, property, buf, sizeof(buf), NULL);

		return buf;
	}

	static DeviceVR::Pose getPose(const TrackedDevicePose_t& pose)
	{
		const HmdMatrix34_t& m = pose.mDeviceToAbsoluteTracking;

		Quat orientation(Matrix3(m.m[0][0], m.m[0][1], m.m[0][2], m.m[1][0], m.m[1][1], m.m[1][2], m.m[2][0], m.m[2][1], m.m[2][2]));

		DeviceVR::Pose result = {};

		result.valid = pose.bPoseIsValid;

		result.position[0] = m.m[0][3];
		result.position[1] = m.m[1][3];
		result.position[2] = m.m[2][3];

		result.orientation[0] = orientation.x;
		result.orientation[1] = orientation.y;
		result.orientation[2] = orientation.z;
		result.orientation[3] = orientation.w;

		return result;
	}

	static DeviceVR::Pose getPoseForIndex(IVRCompositor* compositor, TrackedDeviceIndex_t index)
	{
		TrackedDevicePose_t pose;
		OVR_CHECK(compositor->GetLastPoseForTrackedDeviceIndex(index, &pose, NULL));

		return getPose(pose);
	}

	static DeviceVR::Pose getPoseForRole(IVRSystem* system, IVRCompositor* compositor, ETrackedControllerRole role)
	{
		TrackedDeviceIndex_t index = system->GetTrackedDeviceIndexForControllerRole(role);
		if (index == k_unTrackedDeviceIndexInvalid)
			return DeviceVR::Pose();

		return getPoseForIndex(compositor, index);
	}

	struct OpenVRD3D11: DeviceVRD3D11
	{
		IVRSystem* system;
		IVRCompositor* compositor;

		shared_ptr<Framebuffer> fb[2];
		shared_ptr<Texture> textures[2];

		OpenVRD3D11(): system(NULL), compositor(NULL)
		{
		}

		~OpenVRD3D11()
		{
			auto VR_ShutdownInternal = getOpenVRFunction<void (*)()>("VR_ShutdownInternal");

			if (VR_ShutdownInternal)
				VR_ShutdownInternal();
		}

		void update() override
		{
			compositor->SetTrackingSpace(TrackingUniverseSeated);
		}

		void recenter() override
		{
			system->ResetSeatedZeroPose();
		}

		Framebuffer* getEyeFramebuffer(int eye) override
		{
			RBXASSERT(eye == 0 || eye == 1);

			return fb[eye].get();
		}

		State getState() override
		{
			State result = {};

			result.headPose = getPoseForIndex(compositor, k_unTrackedDeviceIndex_Hmd);
			result.handPose[0] = getPoseForRole(system, compositor, TrackedControllerRole_LeftHand);
			result.handPose[1] = getPoseForRole(system, compositor, TrackedControllerRole_RightHand);

			for (int eye = 0; eye < 2; ++eye)
			{
				HmdMatrix34_t eyeToHead = system->GetEyeToHeadTransform(EVREye(eye));

				result.eyeOffset[eye][0] = eyeToHead.m[0][3];
				result.eyeOffset[eye][1] = eyeToHead.m[1][3];
				result.eyeOffset[eye][2] = eyeToHead.m[2][3];

				float upTan, downTan, leftTan, rightTan;
				system->GetProjectionRaw(EVREye(eye), &leftTan, &rightTan, &upTan, &downTan);

				result.eyeFov[eye][0] = -upTan;
				result.eyeFov[eye][1] = downTan;
				result.eyeFov[eye][2] = -leftTan;
				result.eyeFov[eye][3] = rightTan;
			}

			result.needsMirror = true;

			return result;
		}

		void submitFrame(DeviceContext* context) override
		{
			RBXPROFILER_SCOPE("VR", "submitFrame");

			for (int eye = 0; eye < 2; ++eye)
			{
				Texture_t texture = { static_cast<TextureD3D11*>(textures[eye].get())->getObject(), API_DirectX, ColorSpace_Gamma };
				OVR_CHECK(compositor->Submit(EVREye(eye), &texture));
			}

			OVR_CHECK(compositor->WaitGetPoses(NULL, 0, NULL, 0));
		}

		void setup(Device* device) override
		{
			unsigned int width, height;
			system->GetRecommendedRenderTargetSize(&width, &height);

			shared_ptr<Renderbuffer> depthStencil = device->createRenderbuffer(Texture::Format_D24S8, width, height, 1);

			for (int eye = 0; eye < 2; ++eye)
			{
				textures[eye] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
				fb[eye] = device->createFramebuffer(textures[eye]->getRenderbuffer(0, 0), depthStencil);
			}
		}
	};

	DeviceVRD3D11* DeviceVRD3D11::createOpenVR(IDXGIAdapter** outAdapter)
	{
		if (!FFlag::OpenVR)
			return NULL;

		auto VR_InitInternal = getOpenVRFunction<uint32_t (*)(EVRInitError *peError, EVRApplicationType eApplicationType)>("VR_InitInternal");
		auto VR_ShutdownInternal = getOpenVRFunction<void (*)()>("VR_ShutdownInternal");
		auto VR_GetGenericInterface = getOpenVRFunction<void* (*)(const char *pchInterfaceVersion, EVRInitError *peError)>("VR_GetGenericInterface");

		if (!VR_InitInternal || !VR_ShutdownInternal || !VR_GetGenericInterface)
			return NULL;

		EVRInitError error;
		VR_InitInternal(&error, VRApplication_Scene);

		if (error)
		{
			FASTLOG1(FLog::VR, "VR: VR_InitInternal returned %d", error);
			return NULL;
		}

		IVRSystem* system = static_cast<IVRSystem*>(VR_GetGenericInterface(IVRSystem_Version, &error));

		if (!system)
		{
			FASTLOG1(FLog::VR, "VR: Error creating system: %d", error);
			VR_ShutdownInternal();
			return NULL;
		}

		IVRCompositor* compositor = static_cast<IVRCompositor*>(VR_GetGenericInterface(IVRCompositor_Version, &error));

		if (!compositor)
		{
			FASTLOG1(FLog::VR, "VR: Error creating compositor: %d", error);
			VR_ShutdownInternal();
			return NULL;
		}

		int adapterIndex = 0;
		system->GetDXGIOutputInfo(&adapterIndex);

		OpenVRD3D11* vr = new OpenVRD3D11();

		vr->system = system;
		vr->compositor = compositor;

		if (outAdapter)
			*outAdapter = getAdapterForIndex(adapterIndex);

		FASTLOGS(FLog::VR, "VR: Connected to %s", getStringProperty(system, k_unTrackedDeviceIndex_Hmd, Prop_ModelNumber_String));

		return vr;
	}

}
}
#endif
