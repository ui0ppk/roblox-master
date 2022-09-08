#pragma once

#include "Util/G3DCore.h"

namespace RBX
{
namespace Graphics
{

class Device;
class RenderCamera;

struct GlobalShaderData
{
	Matrix4 ViewProjection;
    Vector4 ViewRight;
    Vector4 ViewUp;
    Vector4 ViewDir;
    Vector4 CameraPosition;

	Vector4 AmbientColor;
	Vector4 Lamp0Color;
	Vector4 Lamp0Dir;
	Vector4 Lamp1Color;

	Vector4 FogColor;
	Vector4 FogParams;

	Vector4 LightBorder;
	Vector4 LightConfig0;
	Vector4 LightConfig1;
	Vector4 LightConfig2;
	Vector4 LightConfig3;

	Vector4 FadeDistance_GlowFactor;
	Vector4 OutlineBrightness_ShadowInfo;

	Vector4 ShadowMatrix0;
	Vector4 ShadowMatrix1;
	Vector4 ShadowMatrix2;

	static void define(Device* device);

	void setCamera(const RenderCamera& camera);
};

}
}
