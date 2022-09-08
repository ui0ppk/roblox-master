/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "GfxBase/IAdornable.h"
#include "V8DataModel/Effect.h"
#include "V8DataModel/PartInstance.h"

namespace RBX {
	extern const char* const sSmoke;
	class Smoke : public DescribedCreatable<Smoke, Instance, sSmoke>
					, public Effect

	{
	private:
		bool enabled;

		Color3 color;
		float  size;
		float  opacity;
		float  riseVelocity;

		static const float MaxRiseVelocity;
		static const float MaxSize;

	public:
		Smoke();
		virtual ~Smoke();

		static Reflection::BoundProp<bool> prop_Enabled;
		bool getEnabled() const { return enabled; }

		void onChangedEnabled(const Reflection::PropertyDescriptor&);

		void setColor(Color3 color);
		Color3 getColor() const		{return color;}

		void setSizeUi(float);
		void setSize(float);
		float getSizeRaw() const		{return size;}
		float getClampedSize() const;

		void setOpacityUi(float);
		void setOpacity(float);
		float getOpacityRaw() const		{return opacity;}
		float getClampedOpacity() const;

		void setRiseVelocityUi(float);
		void setRiseVelocity(float);
		float getRiseVelocityRaw() const		{return riseVelocity;}
		float getClampedRiseVelocity() const;

		static float getMaxRiseVelocity()	{return MaxRiseVelocity;}
		static float getMaxSize()			{return MaxSize;}

	protected:
		bool askSetParent(const Instance* parent) const {return Instance::fastDynamicCast<PartInstance>(parent) != NULL;}
		bool askAddChild(const Instance* instance) const {return true;}
	};
}	// namespace RBX
