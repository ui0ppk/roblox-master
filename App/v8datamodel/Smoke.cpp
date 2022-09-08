/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Smoke.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"

namespace RBX {

const char* const sSmoke = "Smoke";


REFLECTION_BEGIN();
Reflection::BoundProp<bool> Smoke::prop_Enabled("Enabled", "Data", &Smoke::enabled);
static const Reflection::PropDescriptor<Smoke, Color3> prop_Color("Color", category_Data, &Smoke::getColor, &Smoke::setColor);

static const Reflection::PropDescriptor<Smoke, float> prop_SizeUi("Size", category_Data, &Smoke::getSizeRaw, &Smoke::setSizeUi, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Smoke, float> prop_OpacityUi("Opacity", category_Data, &Smoke::getOpacityRaw, &Smoke::setOpacityUi, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Smoke, float> prop_RiseVelocityUi("RiseVelocity", category_Data, &Smoke::getRiseVelocityRaw, &Smoke::setRiseVelocityUi, Reflection::PropertyDescriptor::UI);

static const Reflection::PropDescriptor<Smoke, float> prop_Size("size_xml", category_Data, &Smoke::getSizeRaw, &Smoke::setSize, Reflection::PropertyDescriptor::STREAMING);
static const Reflection::PropDescriptor<Smoke, float> prop_Opacity("opacity_xml", category_Data, &Smoke::getOpacityRaw, &Smoke::setOpacity, Reflection::PropertyDescriptor::STREAMING);
static const Reflection::PropDescriptor<Smoke, float> prop_RiseVelocity("riseVelocity_xml", category_Data, &Smoke::getRiseVelocityRaw, &Smoke::setRiseVelocity, Reflection::PropertyDescriptor::STREAMING);
REFLECTION_END();

const float Smoke::MaxRiseVelocity = 25.0f;
const float Smoke::MaxSize = 100.0f;

Smoke::Smoke() 
	: DescribedCreatable<Smoke, Instance, sSmoke>("Smoke")
	, enabled(true)
	, size(1)
	, opacity(0.5)
	, riseVelocity(1)
	, color(G3D::Color3::white())
{

}

Smoke::~Smoke()
{
}


void Smoke::setColor(Color3 value)
{
	if (color != value) {
		color = value;
		raisePropertyChanged(prop_Color);
	}
}

float Smoke::getClampedSize() const
{
	return G3D::clamp(getSizeRaw(), 0.1f, MaxSize);
}

void Smoke::setSizeUi(float value)
{
	float clampedSize = G3D::clamp(value, 0.1f, MaxSize);
	if(clampedSize != getSizeRaw()){
		setSize(clampedSize);
	}
	else if(clampedSize != value){
		raisePropertyChanged(prop_SizeUi);
	}
}
void Smoke::setSize(float value)
{
	if (size != value) {
		size = value;
		raisePropertyChanged(prop_Size);
		raisePropertyChanged(prop_SizeUi);
	}
}

float Smoke::getClampedOpacity() const
{
	return G3D::clamp(getOpacityRaw(), 0, 1);
}
void Smoke::setOpacityUi(float value)
{
	float clampedOpacity = G3D::clamp(value, 0, 1);
	if(clampedOpacity != getOpacityRaw()){
		setOpacity(clampedOpacity);
	}
	else if(clampedOpacity != value){
		raisePropertyChanged(prop_OpacityUi);
	}
}
void Smoke::setOpacity(float value)
{
	if (opacity != value) {
		opacity = value;
		raisePropertyChanged(prop_Opacity);
		raisePropertyChanged(prop_OpacityUi);
	}
}

float Smoke::getClampedRiseVelocity() const
{
	return G3D::clamp(getRiseVelocityRaw(), -MaxRiseVelocity, MaxRiseVelocity);
}
void Smoke::setRiseVelocityUi(float value)
{
	float clampedRiseVelocity = G3D::clamp(value, -MaxRiseVelocity, MaxRiseVelocity);
	if(clampedRiseVelocity != getRiseVelocityRaw()){
		setRiseVelocity(clampedRiseVelocity);
	}
	else if(clampedRiseVelocity != value){
		raisePropertyChanged(prop_RiseVelocityUi);
	}
}

void Smoke::setRiseVelocity(float value)
{
	if (riseVelocity != value) {
		riseVelocity = value;
		raisePropertyChanged(prop_RiseVelocity);
		raisePropertyChanged(prop_RiseVelocityUi);
	}
}



} // namespace
