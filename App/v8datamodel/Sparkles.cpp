/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Sparkles.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "GfxBase/Adorn.h"

namespace RBX {

const char* const sSparkles = "Sparkles";

REFLECTION_BEGIN();
Reflection::BoundProp<bool> Sparkles::prop_Enabled("Enabled", "Data", &Sparkles::enabled);
Reflection::PropDescriptor<Sparkles, Color3> Sparkles::prop_Color("SparkleColor", category_Data, &Sparkles::getColor, &Sparkles::setColor);
static const Reflection::PropDescriptor<Sparkles, Color3> prop_LegacyColor("Color", category_Data, &Sparkles::getLegacyColor, &Sparkles::setLegacyColor, Reflection::PropertyDescriptor::LEGACY_SCRIPTING);
REFLECTION_END();

Sparkles::Sparkles() 
	: DescribedCreatable<Sparkles, Instance, sSparkles>("Sparkles")
	,enabled(true)
	,color(Color3(144, 25, 255)/255.0f)
{

}

void Sparkles::setColor(Color3 value)
{
	if (color != value) {
		color = value;
		raisePropertyChanged(prop_Color);
	}
}

void Sparkles::setLegacyColor(Color3 color)
{
	setColor(Color3(color.r * 144 / 255, color.g * 25 / 255, color.b));
}

Color3 Sparkles::getLegacyColor() const
{
	return Color3(color.r *255 / 144, color.g * 255 / 25, color.b);
}

} // namespace