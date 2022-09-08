#include "stdafx.h"

#include "v8datamodel/lighting.h"
#include "v8datamodel/sky.h"

#include "Util/RobloxGoogleAnalytics.h"

static void sendLightingShadowsStats()
{
	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "LightingShadows");
}

using namespace RBX;

REFLECTION_BEGIN();
static Reflection::EventDesc<Lighting, void(bool)> event_LightingChanged(&Lighting::lightingChangedSignal, "LightingChanged", "skyboxChanged");
static Reflection::PropDescriptor<Lighting, std::string> prop_Time("TimeOfDay", category_Data, &Lighting::getTimeStr, &Lighting::setTimeStr);
static Reflection::PropDescriptor<Lighting, float> prop_ClockTime("ClockTime", category_Data, &Lighting::getTimeFloat, &Lighting::setTimeFloat);

static Reflection::PropDescriptor<Lighting, float> prop_GeographicLatitude("GeographicLatitude", category_Data, &Lighting::getGeographicLatitude, &Lighting::setGeographicLatitude);
static Reflection::BoundFuncDesc<Lighting, float()> prop_GetMoonPhase(&Lighting::getMoonPhase, "GetMoonPhase", Security::None);
static Reflection::BoundFuncDesc<Lighting, G3D::Vector3()> prop_GetMoonPosition(&Lighting::getMoonPosition, "GetMoonDirection", Security::None);
static Reflection::BoundFuncDesc<Lighting, G3D::Vector3()> prop_GetSunPosition(&Lighting::getSunPosition, "GetSunDirection", Security::None);

static Reflection::BoundFuncDesc<Lighting, double()> func_GetMinutesAfterMidnight(&Lighting::getMinutesAfterMidnight, "GetMinutesAfterMidnight", Security::None);
static Reflection::BoundFuncDesc<Lighting, void(double)> func_SetMinutesAfterMidnight(&Lighting::setMinutesAfterMidnight, "SetMinutesAfterMidnight", "minutes", Security::None);
static Reflection::BoundFuncDesc<Lighting, double()> func_dep_GetMinutesAfterMidnight(&Lighting::getMinutesAfterMidnight, "getMinutesAfterMidnight", Security::None, Reflection::Descriptor::Attributes::deprecated(func_GetMinutesAfterMidnight));
static Reflection::BoundFuncDesc<Lighting, void(double)> func_dep_SetMinutesAfterMidnight(&Lighting::setMinutesAfterMidnight, "setMinutesAfterMidnight", "minutes", Security::None, Reflection::Descriptor::Attributes::deprecated(func_SetMinutesAfterMidnight));

static Reflection::PropDescriptor<Lighting, G3D::Color3> desc_ShadowColor("ShadowColor", category_Appearance, &Lighting::getShadowColor, &Lighting::setShadowColor);
static Reflection::PropDescriptor<Lighting, G3D::Color3> desc_FogColor("FogColor", "Fog", &Lighting::getFogColor, &Lighting::setFogColor);
static Reflection::PropDescriptor<Lighting, float> desc_FogStart("FogStart", "Fog", &Lighting::getFogStart, &Lighting::setFogStart);
static Reflection::PropDescriptor<Lighting, float> desc_FogEnd("FogEnd", "Fog", &Lighting::getFogEnd, &Lighting::setFogEnd);

static Reflection::PropDescriptor<Lighting, bool> desc_GlobalShadows("GlobalShadows", category_Appearance, &Lighting::getGlobalShadows, &Lighting::setGlobalShadows);

Reflection::BoundProp<float> Lighting::desc_GlobalBrightness("Brightness", category_Appearance, &Lighting::globalBrightness, &Lighting::onPropChanged);
Reflection::BoundProp<G3D::Color3> Lighting::desc_TopColorShift("ColorShift_Top", category_Appearance, &Lighting::topColorShift, &Lighting::onPropChanged);
Reflection::BoundProp<G3D::Color3> Lighting::desc_BottomColorShift("ColorShift_Bottom", category_Appearance, &Lighting::bottomColorShift, &Lighting::onPropChanged);
Reflection::BoundProp<G3D::Color3> Lighting::desc_GlobalAmbient("Ambient", category_Appearance, &Lighting::globalAmbient, &Lighting::onPropChanged);
Reflection::BoundProp<G3D::Color3> Lighting::desc_GlobalOutdoorAmbient("OutdoorAmbient", category_Appearance, &Lighting::globalOutdoorAmbient, &Lighting::onPropChanged);

Reflection::BoundProp<bool> Lighting::desc_Outlines("Outlines", category_Appearance, &Lighting::outlines, &Lighting::onPropChanged);
REFLECTION_END();

const char* const RBX::sLighting = "Lighting";

G3D::Color3 fromRGB(unsigned char r, unsigned char g, unsigned char b)
{
	return G3D::Color3(((float)r)/255.0f, ((float)g)/255.0f, ((float)b)/255.0f);
}

Lighting::Lighting(void)
:clearColor(G3D::Color3::white())
,clearAlpha(1.f)
,shadowColor(G3D::Color3(0.7f, 0.7f, 0.72f))
,timeOfDay(boost::posix_time::duration_from_string("14:00:00"))
,topColorShift(G3D::Color3(0,0,0.0f))
,bottomColorShift(G3D::Color3(0,0,0))
,globalBrightness(1)
,globalAmbient(G3D::Color3(0.5,0.5,0.5))
,hasSky(true)
,fogColor(G3D::Color3(0.75f, 0.75f, 0.75f))
,fogStart(0.0f)
,fogEnd(100000.0f)
,globalShadows(false)
,globalOutdoorAmbient(G3D::Color3(0.5,0.5,0.5))
,outlines(true)
{
	skyParameters.lightColor = fromRGB(152, 137, 102);
	skyParameters.setTime(getGameTime());
	setName("Lighting");
}


G3D::Vector3 Lighting::getMoonPosition()
{
	return skyParameters.physicallyCorrect ? skyParameters.trueMoonPosition : skyParameters.moonPosition;
}

G3D::Vector3 Lighting::getSunPosition()
{
	return skyParameters.physicallyCorrect ? skyParameters.trueSunPosition : skyParameters.sunPosition;
}

double Lighting::getMinutesAfterMidnight()
{
	return timeOfDay.total_milliseconds() / 60000.0;
}

void Lighting::setMinutesAfterMidnight(double value)
{
	setTime(boost::posix_time::seconds((int)(60*value)));
}

G3D::GameTime Lighting::getGameTime() const
{
	return timeOfDay.total_seconds();
}

std::string Lighting::getTimeStr() const
{
	return boost::posix_time::to_simple_string(timeOfDay);
}

void Lighting::setTimeStr(const std::string& value)
{
	setTime(boost::posix_time::duration_from_string(value));
}

float Lighting::getTimeFloat() const
{
	return (timeOfDay.total_seconds() / 3600);
}

void Lighting::setTimeFloat(float value)
{
	value = G3D::clamp(value, 0.0f, 24.0f);
	int seconds = ceil(value * 3600);

	if (timeOfDay.total_seconds() != seconds)
	{
		timeOfDay = boost::posix_time::seconds(seconds);

		skyParameters.setTime(getGameTime());
		this->raisePropertyChanged(prop_ClockTime);
		fireLightingChanged(false);
	}
}

void Lighting::setTime(const boost::posix_time::time_duration& value)
{
	// wrap to 24-hour clock
	int seconds = value.total_seconds();
	seconds %= 60*60*24;
	
	if (timeOfDay.total_seconds()!=seconds)
	{
		timeOfDay = boost::posix_time::seconds(seconds);

		skyParameters.setTime(getGameTime());
		this->raisePropertyChanged(prop_Time);
		fireLightingChanged(false);
	}
}

//void Lighting::setAmbientTop(Color3 value)
//{
//	if (value!=ambientTop)
//	{
//		ambientTop = value;
//		this->raisePropertyChanged(desc_AmbientTop);
//		fireLightingChanged(false);
//	}
//}

void Lighting::setGeographicLatitude(float value)
{
	if (value!=skyParameters.geoLatitude)
	{
		skyParameters.setLatitude(value);
		skyParameters.setTime(getGameTime());

		this->raisePropertyChanged(prop_GeographicLatitude);
		fireLightingChanged(false);
	}
}

bool Lighting::askAddChild(const Instance* instance) const
{
	return Instance::fastDynamicCast<Sky>(instance)!=0;
}

void Lighting::fireLightingChanged(bool skyboxChanged)
{
	lightingChangedSignal(skyboxChanged);
}

void Lighting::setClearColor(const Color3& value)
{
	if (value!=clearColor)
	{
		clearColor = value;
		fireLightingChanged(false);
	}
}

void Lighting::setClearAlpha(float value)
{
	if (value!=clearAlpha)
	{
		clearAlpha = value;
		fireLightingChanged(false);
	}
}

void Lighting::setGlobalAmbient(const Color3& value)
{
	if (value!=globalAmbient)
	{
		globalAmbient = value;
		this->raisePropertyChanged(desc_GlobalAmbient);
		fireLightingChanged(false);
	}
}

void Lighting::setShadowColor(const Color3& value)
{
	if (value!=shadowColor)
	{
		shadowColor = value;
		this->raisePropertyChanged(desc_ShadowColor);
		fireLightingChanged(false);
	}
}


void Lighting::setFogColor(const G3D::Color3& value)
{
	if (value!=fogColor)
	{
		fogColor = value;
		this->raisePropertyChanged(desc_FogColor);
	}
}

void Lighting::setFogStart( float value )
{
	if (value!=fogStart)
	{
		fogStart = value;
		this->raisePropertyChanged(desc_FogStart);
	}
}

void Lighting::setFogEnd( float value )
{
	if (value!=fogEnd)
	{
		fogEnd = value;
		this->raisePropertyChanged(desc_FogEnd);
	}
}

void Lighting::setGlobalShadows(bool value)
{
	if (value != globalShadows)
	{
		globalShadows = value;
		this->raisePropertyChanged(desc_GlobalShadows);

        if (value)
        {
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(&sendLightingShadowsStats, flag);
        }
	}
}

void Lighting::replaceSky(Sky* newSky)
{
	while (Sky* oldSky = this->findFirstChildOfType<Sky>())
	{
		oldSky->setParent(NULL);
	}
	newSky->setParent(this);
}


void Lighting::onChildRemoving(Instance* child)
{
	if (sky.get()==child)
	{
		sky.reset();
		fireLightingChanged(true);
	}
	Super::onChildRemoving(child);
}

void Lighting::onChildAdded(Instance* child)
{
	Super::onChildAdded(child);
	if (Sky* sky = Instance::fastDynamicCast<Sky>(child))
	{
		this->sky = shared_from(sky);
		fireLightingChanged(true);
	}
}

void Lighting::onChildChanged(Instance* instance, const PropertyChanged& event)
{
	Super::onChildChanged(instance, event);
	if (sky.get()==instance)
		fireLightingChanged(true);
}
