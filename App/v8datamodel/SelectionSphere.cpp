#include "stdafx.h"

#include "V8DataModel/SelectionSphere.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "AppDraw/DrawAdorn.h"
#include "AppDraw/Draw.h"

namespace RBX {

const char* const sSelectionSphere = "SelectionSphere";

const Reflection::PropDescriptor<SelectionSphere, Color3> prop_SurfaceColor("SurfaceColor3", category_Appearance, &SelectionSphere::getSurfaceColor, &SelectionSphere::setSurfaceColor, Reflection::PropertyDescriptor::STANDARD);
const Reflection::PropDescriptor<SelectionSphere, BrickColor> prop_depSurfaceBrickColor("SurfaceColor", category_Appearance, &SelectionSphere::getSurfaceBrickColor, &SelectionSphere::setSurfaceBrickColor, 
																						Reflection::PropertyDescriptor::Attributes::deprecated(prop_SurfaceColor, Reflection::PropertyDescriptor::LEGACY_SCRIPTING));

const Reflection::PropDescriptor<SelectionSphere, float> prop_SurfaceTransparency("SurfaceTransparency", category_Appearance, &SelectionSphere::getSurfaceTransparency, &SelectionSphere::setSurfaceTransparency);

static void renderPart(Adorn* adorn, const Part& part, Color3 color, float transparency, Color3 surfaceColor, float surfaceTransparency)
{
    float thickness = 0.2f;
	float radius = part.gridSize.min() * 0.5f;

	if (transparency < 1)
	{
		Color4 alphaColor(color, 1.0f - transparency);

		adorn->setMaterial(Adorn::Material_Outline);

		adorn->setObjectToWorldMatrix(part.coordinateFrame);
		adorn->sphere(Sphere(Vector3(), radius + thickness), alphaColor);
	}

	if (surfaceTransparency < 1)
	{
		Color4 alphaColor(surfaceColor, 1.0f - surfaceTransparency);

		adorn->setMaterial(Adorn::Material_NoLighting);

		adorn->setObjectToWorldMatrix(part.coordinateFrame);
		adorn->sphere(Sphere(Vector3(), radius), alphaColor);
	}

	adorn->setMaterial(Adorn::Material_Default);
}

SelectionSphere::SelectionSphere()
	: DescribedCreatable<SelectionSphere, PVAdornment, sSelectionSphere>("SelectionSphere")
	, surfaceColor(BrickColor::brickBlue().color3())
	, surfaceTransparency(1)
{
}

void SelectionSphere::render3dAdorn(Adorn* adorn)
{
	if (getVisible())
	{
		if (shared_ptr<PVInstance> pvInstance = adornee.lock())
		{
			if (PartInstance* partInstance = pvInstance->fastDynamicCast<PartInstance>())
				renderPart(adorn, partInstance->getPart(), color, transparency, surfaceColor, surfaceTransparency);
			else if (ModelInstance* modelInstance = pvInstance->fastDynamicCast<ModelInstance>())
				renderPart(adorn, modelInstance->computePart(), color, transparency, surfaceColor, surfaceTransparency);
		}
	}
}

void SelectionSphere::setSurfaceBrickColor(BrickColor value)
{
	setSurfaceColor(value.color3());
}

void SelectionSphere::setSurfaceColor(Color3 value)
{
	if (surfaceColor != value)
	{
		surfaceColor = value;
		raisePropertyChanged(prop_SurfaceColor);
	}
}

void SelectionSphere::setSurfaceTransparency(float value)
{
	if (surfaceTransparency != value)
	{
		surfaceTransparency = value;
		raisePropertyChanged(prop_SurfaceTransparency);
	}
}

}
