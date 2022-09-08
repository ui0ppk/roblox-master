#pragma once

#include "V8Tree/Service.h"
#include "util/g3dcore.h"
#include "G3D/LightingParameters.h"
#define BOOST_DATE_TIME_NO_LIB
#include "boost/date_time/posix_time/posix_time.hpp"

namespace RBX {

class Sky;

extern const char* const sLighting;
class Lighting 
	: public DescribedCreatable<Lighting, Instance, sLighting, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
	, public Service
{
private:
	typedef DescribedCreatable<Lighting, Instance, sLighting, Reflection::ClassDescriptor::PERSISTENT_HIDDEN> Super;
	G3D::LightingParameters skyParameters;
	bool hasSky;
	G3D::Color3 clearColor;
    float clearAlpha;
	G3D::Color3 shadowColor;
	G3D::Color3 fogColor;
	float fogStart;
	float fogEnd;
	boost::posix_time::time_duration timeOfDay;

	Color3 topColorShift, bottomColorShift;
	float globalBrightness;
	Color3 globalAmbient;
	Color3 globalOutdoorAmbient;

	bool globalShadows;

	bool outlines;

public:
	Lighting();

	void replaceSky(Sky* newSky);

	rbx::signal<void(bool)> lightingChangedSignal;		// (arg=skyboxChanged)

	bool isSkySuppressed() const { return !hasSky; }
	void suppressSky(const bool value) { hasSky = !value; }

	shared_ptr<Sky> sky;

	static Reflection::BoundProp<float> desc_GlobalBrightness;
	static Reflection::BoundProp<G3D::Color3> desc_TopColorShift;
	static Reflection::BoundProp<G3D::Color3> desc_BottomColorShift;
	static Reflection::BoundProp<G3D::Color3> desc_GlobalAmbient;
	static Reflection::BoundProp<G3D::Color3> desc_GlobalOutdoorAmbient;
	static Reflection::BoundProp<bool> desc_Outlines;

	const G3D::LightingParameters& getSkyParameters() const { return skyParameters; }

	const G3D::Color3& getClearColor() const { return clearColor; }
	void setClearColor(const G3D::Color3& value);

	float getClearAlpha() const { return clearAlpha; }
	void setClearAlpha(float value);

	const G3D::Color3& getShadowColor() const { return shadowColor; }
	void setShadowColor(const G3D::Color3& value);

	const G3D::Color3& getFogColor() const { return fogColor; }
	void setFogColor(const G3D::Color3& value);

	float getTimeFloat() const;
	void setTimeFloat(const float value);
	std::string getTimeStr() const;
	void setTimeStr(const std::string& value);
	void setTime(const boost::posix_time::time_duration& value);
	G3D::GameTime getGameTime() const;
	double getMinutesAfterMidnight();
	void setMinutesAfterMidnight(double value);
	float getFogStart() const { return fogStart; }
	void setFogStart( float value );
	float getFogEnd() const { return fogEnd; }
	void setFogEnd( float value );

	float getMoonPhase() { return (float) skyParameters.moonPhase; }
	G3D::Vector3 getMoonPosition();
	G3D::Vector3 getSunPosition();

	float getGeographicLatitude() const { return skyParameters.geoLatitude; }
	void setGeographicLatitude(float value);

	G3D::Color3 getTopColorShift() const { return topColorShift; }
	G3D::Color3 getBottomColorShift() const { return bottomColorShift; }
	float getGlobalBrightness() const { return globalBrightness; }
	G3D::Color3 getGlobalAmbient() const { return globalAmbient; }
	G3D::Color3 getGlobalOutdoorAmbient() const { return globalOutdoorAmbient; }

	G3D::Color3 getSkyAmbient() const { return (globalOutdoorAmbient - globalAmbient).max(Color3::zero()); }

	bool getGlobalShadows() const { return globalShadows; }
	void setGlobalShadows(bool value);

	bool getOutlines() const { return outlines; }

	void setGlobalAmbient(const G3D::Color3& value);

protected:
	/*override*/ void onChildAdded(Instance* child);
	/*override*/ void onChildRemoving(Instance* child);
	/*override*/ void onChildChanged(Instance* instance, const PropertyChanged& event);
	/*override*/ bool askAddChild(const Instance* instance) const;
private:
	void onPropChanged(const Reflection::PropertyDescriptor&)
	{
		fireLightingChanged(false);
	}
	void fireLightingChanged(bool skyboxChanged);
};

}
