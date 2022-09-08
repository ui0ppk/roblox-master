#if defined(__ANDROID__)
#include "DeviceGL.h"

#include "FramebufferGL.h"
#include "TextureGL.h"

#include <EGL/egl.h>

#include "VrApi.h"
#include "VrApi_Helpers.h"

LOGGROUP(VR)

FASTFLAGVARIABLE(GearVR, false)

namespace RBX
{
namespace Graphics
{
	static ovrJava gJava;

	static DeviceVR::Pose getPose(const ovrPosef& pose, unsigned int statusFlags)
	{
		DeviceVR::Pose result = {};

		result.valid = (statusFlags & VRAPI_TRACKING_STATUS_ORIENTATION_TRACKED) != 0;

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

		ovrTextureSwapChain* swapChain;
		unsigned int swapChainIndex;

		VRTexture(): swapChain(NULL), swapChainIndex(0)
		{
		}
	};

	struct GearVRGL: DeviceVRGL
	{
		ovrMobile* session;
		float eyeFovX, eyeFovY;
		VRTexture textures[2];

		long long frameIndex;
		double sensorSampleTime;
		ovrTracking trackingState;

		GearVRGL(): session(NULL), frameIndex(0), sensorSampleTime(0)
		{
		}

		~GearVRGL()
		{
			vrapi_LeaveVrMode(session);

			for (int eye = 0; eye < 2; ++eye)
				vrapi_DestroyTextureSwapChain(textures[eye].swapChain);

			vrapi_Shutdown();
		}

		void update() override
		{
			frameIndex++;

			double frameTime = vrapi_GetPredictedDisplayTime(session, frameIndex);

			sensorSampleTime = vrapi_GetTimeInSeconds();

			ovrTracking baseState = vrapi_GetPredictedTracking(session, frameTime);

			ovrHeadModelParms headModel = vrapi_DefaultHeadModelParms();

			trackingState = vrapi_ApplyHeadModel(&headModel, &baseState);
		}

		void recenter() override
		{
			vrapi_RecenterPose(session);
		}

		Framebuffer* getEyeFramebuffer(int eye) override
		{
			RBXASSERT(eye == 0 || eye == 1);

			return textures[eye].fb[textures[eye].swapChainIndex].get();
		}

		State getState() override
		{
			State result = {};

			result.headPose = getPose(trackingState.HeadPose.Pose, trackingState.Status);

			ovrHeadModelParms headModel = vrapi_DefaultHeadModelParms();

			float tanX = tanf(eyeFovX / 2 * (3.1415926f / 180));
			float tanY = tanf(eyeFovY / 2 * (3.1415926f / 180));

			for (int eye = 0; eye < 2; ++eye)
			{
				float eyeSign = (eye == 0 ? -1 : 1);

				result.eyeOffset[eye][0] = eyeSign * headModel.InterpupillaryDistance / 2;
				result.eyeOffset[eye][1] = 0;
				result.eyeOffset[eye][2] = 0;

				result.eyeFov[eye][0] = tanY;
				result.eyeFov[eye][1] = tanY;
				result.eyeFov[eye][2] = tanX;
				result.eyeFov[eye][3] = tanX;
			}

			result.needsMirror = false;

			return result;
		}

		void submitFrame(DeviceContext* context) override
		{
			double currentTime = vrapi_GetTimeInSeconds();

			ovrMatrix4f eyeProjectionMatrix = ovrMatrix4f_CreateProjectionFov(eyeFovX, eyeFovY, 0, 0, VRAPI_ZNEAR, 0);
			ovrMatrix4f tanAngleMatrix = ovrMatrix4f_TanAngleMatrixFromProjection(&eyeProjectionMatrix);

			ovrFrameParms params = vrapi_DefaultFrameParms(&gJava, VRAPI_FRAME_INIT_DEFAULT, currentTime, NULL);
			params.FrameIndex = frameIndex;

			for (int eye = 0; eye < 2; ++eye)
			{
				params.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].ColorTextureSwapChain = textures[eye].swapChain;
				params.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].TextureSwapChainIndex = textures[eye].swapChainIndex;
				params.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].TexCoordsFromTanAngles = tanAngleMatrix;
				params.Layers[VRAPI_FRAME_LAYER_TYPE_WORLD].Textures[eye].HeadPose = trackingState.HeadPose;
			}

			vrapi_SubmitFrame(session, &params);

			for (int eye = 0; eye < 2; ++eye)
			{
				textures[eye].swapChainIndex++;
				textures[eye].swapChainIndex %= vrapi_GetTextureSwapChainLength(textures[eye].swapChain);
			}
		}

		void setup(Device* device) override
		{
			unsigned int width = vrapi_GetSystemPropertyInt(&gJava, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_WIDTH);
			unsigned int height = vrapi_GetSystemPropertyInt(&gJava, VRAPI_SYS_PROP_SUGGESTED_EYE_TEXTURE_HEIGHT);

			shared_ptr<Renderbuffer> depthStencil = device->createRenderbuffer(Texture::Format_D24S8, width, height, 1);

			for (int eye = 0; eye < 2; ++eye)
			{
				textures[eye].swapChain = vrapi_CreateTextureSwapChain(VRAPI_TEXTURE_TYPE_2D, VRAPI_TEXTURE_FORMAT_8888, width, height, 1, true);

				int textureCount = vrapi_GetTextureSwapChainLength(textures[eye].swapChain);

				RBXASSERT(textureCount <= VRTexture::kMaxCount);

				for (int i = 0; i < textureCount; ++i)
				{
					unsigned int textureId = vrapi_GetTextureSwapChainHandle(textures[eye].swapChain, i);

					shared_ptr<Texture> texture(new TextureGL(device, Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer, textureId));

					textures[eye].fb[i] = device->createFramebuffer(texture->getRenderbuffer(0, 0), depthStencil);
				}
			}
		}
	};

	DeviceVRGL* DeviceVRGL::createGear()
	{
		if (!FFlag::GearVR)
			return NULL;

		ovrInitParms initParms = vrapi_DefaultInitParms(&gJava);

		ovrInitializeStatus status = vrapi_Initialize(&initParms);

		if (status != VRAPI_INITIALIZE_SUCCESS)
		{
			FASTLOG1(FLog::VR, "VR: vrapi_Initialize returned %d", status);
			return NULL;
		}

		ovrModeParms modeParms = vrapi_DefaultModeParms(&gJava);

		ovrMobile* session = vrapi_EnterVrMode(&modeParms);

		GearVRGL* vr = new GearVRGL();

		vr->session = session;

		vr->eyeFovX = vrapi_GetSystemPropertyFloat(&gJava, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_X);
		vr->eyeFovY = vrapi_GetSystemPropertyFloat(&gJava, VRAPI_SYS_PROP_SUGGESTED_EYE_FOV_DEGREES_Y);

		return vr;
	}

}
}

void vrGearSetJava(JavaVM* vm, JNIEnv* env, jobject activity)
{
	using RBX::Graphics::gJava;

	gJava.Vm = vm;
	gJava.Env = env;
	gJava.ActivityObject = activity;
}
#endif
