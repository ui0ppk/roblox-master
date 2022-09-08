/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/SelectionBox.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "AppDraw/DrawAdorn.h"
#include "AppDraw/Draw.h"

DYNAMIC_FASTFLAG(GuiBase3dReplicateColor3WithBrickColor)

namespace RBX {

const char* const sSelectionBox = "SelectionBox";

const Reflection::PropDescriptor<SelectionBox, Color3> prop_SurfaceColor("SurfaceColor3", category_Appearance, &SelectionBox::getSurfaceColor, &SelectionBox::setSurfaceColor, Reflection::PropertyDescriptor::STANDARD);
const Reflection::PropDescriptor<SelectionBox, BrickColor> prop_depSurfaceBrickColor("SurfaceColor", category_Appearance, &SelectionBox::getSurfaceBrickColor, &SelectionBox::setSurfaceBrickColor, 
																					 Reflection::PropertyDescriptor::Attributes::deprecated(prop_SurfaceColor, Reflection::PropertyDescriptor::LEGACY_SCRIPTING));

const Reflection::PropDescriptor<SelectionBox, float> prop_SurfaceTransparency("SurfaceTransparency", category_Appearance, &SelectionBox::getSurfaceTransparency, &SelectionBox::setSurfaceTransparency);
const Reflection::PropDescriptor<SelectionBox, float> prop_LineThickness("LineThickness", category_Appearance, &SelectionBox::getLineThickness, &SelectionBox::setLineThickness);

SelectionBox::SelectionBox()
	: DescribedCreatable<SelectionBox, PVAdornment, sSelectionBox>("SelectionBox")
	, surfaceColor(BrickColor::brickBlue().color3())
	, surfaceTransparency(1)
	, lineThickness(0.15f)
{
}

static void renderPart(Adorn* adorn, const Part& part, Color3 color, float transparency, Color3 surfaceColor, float surfaceTransparency, float lineThickness)
{
	if (transparency < 1)
	{
		Color4 alphaColor(color, 1.0f - transparency);

		Draw::selectionBox(part, adorn, alphaColor, lineThickness);
	}

	if (surfaceTransparency < 1)
	{
		Color4 alphaColor(surfaceColor, 1.0f - surfaceTransparency);

		adorn->setObjectToWorldMatrix(part.coordinateFrame);
		adorn->box(Extents(-part.gridSize * 0.5f, part.gridSize * 0.5f), alphaColor);
	}
}

void SelectionBox::render3dAdorn(Adorn* adorn)
{
	if (getVisible())
	{
		if (shared_ptr<PVInstance> pvInstance = adornee.lock())
		{
			if (PartInstance* partInstance = pvInstance->fastDynamicCast<PartInstance>())
				renderPart(adorn, partInstance->getPart(), color, transparency, surfaceColor, surfaceTransparency, lineThickness);
			else if (ModelInstance* modelInstance = pvInstance->fastDynamicCast<ModelInstance>())
				renderPart(adorn, modelInstance->computePart(), color, transparency, surfaceColor, surfaceTransparency, lineThickness);
		}
	}
}

void SelectionBox::setSurfaceBrickColor(BrickColor value)
{
	setSurfaceColor(value.color3());
}

void SelectionBox::setSurfaceColor(Color3 value)
{
	if (surfaceColor != value)
	{
		surfaceColor = value;
		raisePropertyChanged(prop_SurfaceColor);
	}
}

void SelectionBox::setSurfaceTransparency(float value)
{
	if (surfaceTransparency != value)
	{
		surfaceTransparency = value;
		raisePropertyChanged(prop_SurfaceTransparency);
	}
}

void SelectionBox::setLineThickness(float value)
{
	if (lineThickness != value)
	{
		lineThickness = value;
		raisePropertyChanged(prop_LineThickness);
	}
}


}
