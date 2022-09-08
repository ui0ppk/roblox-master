#include "stdafx.h"

#include "V8DataModel/Light.h"

#include "V8DataModel/PartInstance.h"

#include "Util/RobloxGoogleAnalytics.h"

static void sendLightingObjectsStats()
{
	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "LightingObjects");
}

namespace RBX {

const char* const sLight = "Light";
const char* const sPointLight = "PointLight";
const char* const sSpotLight = "SpotLight";
const char* const sSurfaceLight = "SurfaceLight";

Reflection::PropDescriptor<Light, bool> Light::prop_Enabled("Enabled", category_Appearance, &Light::getEnabled, &Light::setEnabled);
Reflection::PropDescriptor<Light, Color3> Light::prop_Color("Color", category_Appearance, &Light::getColor, &Light::setColor);
Reflection::PropDescriptor<Light, float> Light::prop_Brightness("Brightness", category_Appearance, &Light::getBrightness, &Light::setBrightness);
Reflection::PropDescriptor<Light, bool> Light::prop_Shadows("Shadows", category_Appearance, &Light::getShadows, &Light::setShadows);

Reflection::PropDescriptor<PointLight, float> PointLight::prop_Range("Range", category_Appearance, &PointLight::getRange, &PointLight::setRange);

Reflection::PropDescriptor<SpotLight, float> SpotLight::prop_Range("Range", category_Appearance, &SpotLight::getRange, &SpotLight::setRange);
Reflection::PropDescriptor<SpotLight, float> SpotLight::prop_Angle("Angle", category_Appearance, &SpotLight::getAngle, &SpotLight::setAngle);
Reflection::EnumPropDescriptor<SpotLight, NormalId> SpotLight::prop_Face("Face", category_Appearance, &SpotLight::getFace, &SpotLight::setFace);

Reflection::PropDescriptor<SurfaceLight, float> SurfaceLight::prop_Range("Range", category_Appearance, &SurfaceLight::getRange, &SurfaceLight::setRange);
Reflection::PropDescriptor<SurfaceLight, float> SurfaceLight::prop_Angle("Angle", category_Appearance, &SurfaceLight::getAngle, &SurfaceLight::setAngle);
Reflection::EnumPropDescriptor<SurfaceLight, NormalId> SurfaceLight::prop_Face("Face", category_Appearance, &SurfaceLight::getFace, &SurfaceLight::setFace);

Light::Light(const char* name) 
	: DescribedNonCreatable<Light, Instance, sLight>(name)
	, enabled(true)
	, shadows(false)
	, color(Color3::white())
	, brightness(1.f)
{
    static boost::once_flag flag = BOOST_ONCE_INIT;
    boost::call_once(&sendLightingObjectsStats, flag);
}

Light::~Light()
{
}

void Light::setEnabled(bool value)
{
	if (enabled != value) {
		enabled = value;
		raisePropertyChanged(prop_Enabled);
	}
}

void Light::setShadows(bool value)
{
	if (shadows != value) {
		shadows = value;
        raisePropertyChanged(prop_Shadows);
	}
}

void Light::setColor(Color3 value)
{
	if (color != value) {
		color = value;
		raisePropertyChanged(prop_Color);
	}
}

void Light::setBrightness(float value)
{
	value = std::max(value, 0.f);
	
	if (brightness != value) {
		brightness = value;
		raisePropertyChanged(prop_Brightness);
	}
}

bool Light::askSetParent(const Instance* parent) const
{
	return Instance::isA<PartInstance>(parent);
}

bool Light::askAddChild(const Instance* instance) const
{
	return true;
}

PointLight::PointLight() 
	: DescribedCreatable<PointLight, Light, sPointLight>("PointLight")
	, range(8.f)
{
}

PointLight::~PointLight()
{
}

void PointLight::setRange(float value)
{
	// Limit range due to rendering constraints
	value = std::min(std::max(value, 0.f), 60.f);

	if (range != value) {
		range = value;
		raisePropertyChanged(prop_Range);
	}
}

SpotLight::SpotLight() 
	: DescribedCreatable<SpotLight, Light, sSpotLight>("SpotLight")
	, range(16.f)
	, angle(90.f)
	, face(NORM_Z_NEG)
{
}

SpotLight::~SpotLight()
{
}

void SpotLight::setRange(float value)
{
	// Limit range due to rendering constraints
	value = std::min(std::max(value, 0.f), 60.f);

	if (range != value) {
		range = value;
		raisePropertyChanged(prop_Range);
	}
}
        
void SpotLight::setAngle(float value)
{
	value = std::min(std::max(value, 0.f), 180.f);
	
	if (angle != value) {
		angle = value;
		raisePropertyChanged(prop_Angle);
	}
}

void SpotLight::setFace(NormalId value)
{
	if (face != value) {
		face = value;
        raisePropertyChanged(prop_Face);
	}
}

SurfaceLight::SurfaceLight() 
	: DescribedCreatable<SurfaceLight, Light, sSurfaceLight>("SurfaceLight")
	, range(16.f)
	, angle(90.f)
	, face(NORM_Z_NEG)
{
}

SurfaceLight::~SurfaceLight()
{
}

void SurfaceLight::setRange(float value)
{
	// Limit range due to rendering constraints
	value = std::min(std::max(value, 0.f), 60.f);

	if (range != value) {
		range = value;
		raisePropertyChanged(prop_Range);
	}
}
        
void SurfaceLight::setAngle(float value)
{
	value = std::min(std::max(value, 0.f), 180.f);
	
	if (angle != value) {
		angle = value;
		raisePropertyChanged(prop_Angle);
	}
}

void SurfaceLight::setFace(NormalId value)
{
	if (face != value) {
		face = value;
        raisePropertyChanged(prop_Face);
	}
}

} // namespace
