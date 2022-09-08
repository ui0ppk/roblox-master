/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Fire.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"

FASTFLAGVARIABLE(RenderNewParticles2Enable,true);

namespace RBX {

const char* const sFire = "Fire";


REFLECTION_BEGIN();
Reflection::BoundProp<bool> Fire::prop_Enabled("Enabled", "Data", &Fire::enabled);
static const Reflection::PropDescriptor<Fire, Color3> prop_Color("Color", category_Data, &Fire::getColor, &Fire::setColor);
static const Reflection::PropDescriptor<Fire, Color3> prop_SecondaryColor("SecondaryColor", category_Data, &Fire::getSecondaryColor, &Fire::setSecondaryColor);

static const Reflection::PropDescriptor<Fire, float> prop_SizeUi("Size", category_Data, &Fire::getSizeRaw, &Fire::setSizeUi, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Fire, float> dep_SizeUi("size", category_Data, &Fire::getSizeRaw, &Fire::setSizeUi, Reflection::PropertyDescriptor::Attributes::deprecated(prop_SizeUi));
static const Reflection::PropDescriptor<Fire, float> prop_HeatUi("Heat", category_Data, &Fire::getHeatRaw, &Fire::setHeatUi, Reflection::PropertyDescriptor::UI);

static const Reflection::PropDescriptor<Fire, float> prop_Size("size_xml", category_Data, &Fire::getSizeRaw, &Fire::setSize, Reflection::PropertyDescriptor::STREAMING);
static const Reflection::PropDescriptor<Fire, float> prop_Heat("heat_xml", category_Data, &Fire::getHeatRaw, &Fire::setHeat, Reflection::PropertyDescriptor::STREAMING);
REFLECTION_END();

const float Fire::MaxHeat = 25.0f;
const float Fire::MaxSize = 30.0f;

Fire::Fire() 
	:
	DescribedCreatable<Fire, Instance, sFire>("Fire")
	,enabled(true)
	, size(5)
	, heat(9)
	, color(G3D::Color3::orange())
	, secondaryColor(G3D::Color3::red())
{
    if (FFlag::RenderNewParticles2Enable) 
    {{{
        color = G3D::Color3(236, 139, 70)/255.f;
        secondaryColor = G3D::Color3(139, 80, 55)/255.f;
    }}}
}

Fire::~Fire()
{
}


void Fire::setColor(Color3 value)
{
	if (color != value) {
		color = value;
		raisePropertyChanged(prop_Color);
	}
}

void Fire::setSecondaryColor(Color3 value)
{
	if (secondaryColor != value) {
		secondaryColor = value;
		raisePropertyChanged(prop_SecondaryColor);
	}
}

float Fire::getClampedSize() const
{
	return G3D::clamp(getSizeRaw(), 2.0f, MaxSize);
}

void Fire::setSizeUi(float value)
{
	float clampedSize = G3D::clamp(value, 2.0f, MaxSize);
	if(clampedSize != getSizeRaw()){
		setSize(clampedSize);
	}
	else if(clampedSize != value){
		raisePropertyChanged(prop_SizeUi);
	}
}
void Fire::setSize(float value)
{
	if (size != value) {
		size = value;
		raisePropertyChanged(prop_Size);
		raisePropertyChanged(prop_SizeUi);
	}
}


float Fire::getClampedHeat() const
{
	return G3D::clamp(getHeatRaw(), -MaxHeat, MaxHeat);
}
void Fire::setHeatUi(float value)
{
	float clampedHeat = G3D::clamp(value, -MaxHeat, MaxHeat);
	if(clampedHeat != getHeatRaw()){
		setHeat(clampedHeat);
	}
	else if(clampedHeat != value){
		raisePropertyChanged(prop_HeatUi);
	}
}

void Fire::setHeat(float value)
{
	if (heat != value) {
		heat = value;
		raisePropertyChanged(prop_Heat);
		raisePropertyChanged(prop_HeatUi);
	}
}

} // namespace
