#pragma once

#include "V8Tree/Instance.h"
#include "V8DataModel/Effect.h"

namespace RBX
{
	extern const char* const sLight;
	class Light : public DescribedNonCreatable<Light, Instance, sLight>
					, public Effect

	{
	public:
		explicit Light(const char* name);
		virtual ~Light();
		
		bool getEnabled() const { return enabled; }
		void setEnabled(bool enabled);

		bool getShadows() const { return shadows; }
		void setShadows(bool shadows);

		Color3 getColor() const { return color; }
		void setColor(Color3 color);

		float getBrightness() const { return brightness; }
        void setBrightness(float brightness);

		static Reflection::PropDescriptor<Light, bool> prop_Enabled;
		static Reflection::PropDescriptor<Light, bool> prop_Shadows;
		static Reflection::PropDescriptor<Light, Color3> prop_Color;
		static Reflection::PropDescriptor<Light, float> prop_Brightness;

	protected:
		bool askSetParent(const Instance* parent) const;
		bool askAddChild(const Instance* instance) const;
	
	private:
		bool enabled;
		bool shadows;
		Color3 color;
        float brightness;
	};
	
	extern const char* const sPointLight;
	class PointLight : public DescribedCreatable<PointLight, Light, sPointLight>
	{
	public:
		PointLight();
		virtual ~PointLight();

		float getRange() const { return range; }
        void setRange(float range);

		static Reflection::PropDescriptor<PointLight, float> prop_Range;

	private:
        float range;
	};
	
	extern const char* const sSpotLight;
	class SpotLight : public DescribedCreatable<SpotLight, Light, sSpotLight>
	{
	public:
		SpotLight();
		virtual ~SpotLight();

		float getRange() const { return range; }
        void setRange(float range);
        
        float getAngle() const { return angle; }
        void setAngle(float angle);
        
		NormalId getFace() const { return face; }
		void setFace(RBX::NormalId value);

		static Reflection::PropDescriptor<SpotLight, float> prop_Range;
		static Reflection::PropDescriptor<SpotLight, float> prop_Angle;
		static Reflection::EnumPropDescriptor<SpotLight, NormalId> prop_Face;

	private:
        float range;
        float angle;
        NormalId face;
	};

	extern const char* const sSurfaceLight;
	class SurfaceLight : public DescribedCreatable<SurfaceLight, Light, sSurfaceLight>
	{
	public:
		SurfaceLight();
		virtual ~SurfaceLight();

		float getRange() const { return range; }
        void setRange(float range);
        
        float getAngle() const { return angle; }
        void setAngle(float angle);
        
		NormalId getFace() const { return face; }
		void setFace(RBX::NormalId value);

		static Reflection::PropDescriptor<SurfaceLight, float> prop_Range;
		static Reflection::PropDescriptor<SurfaceLight, float> prop_Angle;
		static Reflection::EnumPropDescriptor<SurfaceLight, NormalId> prop_Face;

	private:
        float range;
        float angle;
        NormalId face;
	};

}	// namespace RBX
