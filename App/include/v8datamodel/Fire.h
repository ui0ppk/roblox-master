/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "V8DataModel/Effect.h"
#include "V8DataModel/PartInstance.h"

namespace RBX
{
	extern const char* const sFire;
	class Fire : public DescribedCreatable<Fire, Instance, sFire>
					, public Effect

	{
	private:
		bool enabled;

		Color3 color;
		Color3 secondaryColor;
		float  size;
		float  heat;
		static const float MaxHeat;
		static const float MaxSize;

	public:
		Fire();
		virtual ~Fire();

		static Reflection::BoundProp<bool> prop_Enabled;
		bool getEnabled() const { return enabled; }

		void onChangedEnabled(const Reflection::PropertyDescriptor&);

		void setColor(Color3 Color);
		Color3 getColor() const		{return color;}

		void setSecondaryColor(Color3 secondaryColor);
		Color3 getSecondaryColor() const	{return secondaryColor;}

		void setSizeUi(float);
		void setSize(float);
		float getSizeRaw() const		{return size;}
		float getClampedSize() const;

		void setHeatUi(float);
		void setHeat(float);
		float getHeatRaw() const		{return heat;}
		float getClampedHeat() const;

		static float getMaxSize()		{return MaxSize;}

	protected:
		bool askSetParent(const Instance* parent) const {return Instance::fastDynamicCast<PartInstance>(parent) != NULL;}
		bool askAddChild(const Instance* instance) const {return true;}
	};
}	// namespace RBX
