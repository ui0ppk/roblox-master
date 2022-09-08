#include "stdafx.h"
#include "GlobalShaderData.h"

#include "GfxCore/Device.h"

#include "RenderCamera.h"

namespace RBX
{
namespace Graphics
{

void GlobalShaderData::define(Device* device)
{
    bool useG = (device->getShadingLanguage() == "hlsl");

	std::vector<ShaderGlobalConstant> ctab;

#define MEMBER(name) ctab.push_back(ShaderGlobalConstant(useG ? "_G." #name : #name, offsetof(GlobalShaderData, name), sizeof(static_cast<GlobalShaderData*>(0)->name)))

	MEMBER(ViewProjection);
    MEMBER(ViewRight);
    MEMBER(ViewUp);
    MEMBER(ViewDir);
	MEMBER(CameraPosition);

	MEMBER(AmbientColor);
	MEMBER(Lamp0Color);
	MEMBER(Lamp0Dir);
	MEMBER(Lamp1Color);

	MEMBER(FogColor);
	MEMBER(FogParams);

	MEMBER(LightBorder);
	MEMBER(LightConfig0);
	MEMBER(LightConfig1);
	MEMBER(LightConfig2);
	MEMBER(LightConfig3);

	MEMBER(FadeDistance_GlowFactor);
	MEMBER(OutlineBrightness_ShadowInfo);

	MEMBER(ShadowMatrix0);
	MEMBER(ShadowMatrix1);
	MEMBER(ShadowMatrix2);

#undef MEMBER

	device->defineGlobalConstants(sizeof(GlobalShaderData), ctab);
}

void GlobalShaderData::setCamera(const RenderCamera& camera)
{
    Matrix3 camMat = camera.getViewMatrix().upper3x3().transpose();

	ViewProjection = camera.getViewProjectionMatrix();
    ViewRight = Vector4(camMat.column(0), 0);
    ViewUp = Vector4(camMat.column(1), 0);
    ViewDir = Vector4(camMat.column(2),0);
	CameraPosition = Vector4(camera.getPosition(), 1);
}

}
}
