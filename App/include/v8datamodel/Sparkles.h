/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "GfxBase/IAdornable.h"
#include "V8DataModel/Effect.h"
#include "V8DataModel/PartInstance.h"

namespace RBX {
	class PartInstance;
	class ModelInstance;

	extern const char* const sSparkles;
	class Sparkles : public DescribedCreatable<Sparkles, Instance, sSparkles>
					, public Effect

	{
	private:
		bool enabled;

		Color3 color;

	public:
		Sparkles();
		virtual ~Sparkles() {}

		bool getEnabled() const { return enabled; }
		void setColor(Color3 color);
		Color3 getColor() const		{return color;}
		void setLegacyColor(Color3 color);
		Color3 getLegacyColor() const;

		static Reflection::BoundProp<bool> prop_Enabled;
		static Reflection::PropDescriptor<Sparkles, Color3> prop_Color;

		void onChangedEnabled(const Reflection::PropertyDescriptor&);

	protected:
		bool askSetParent(const Instance* parent) const {return Instance::fastDynamicCast<PartInstance>(parent) != NULL;}
		bool askAddChild(const Instance* instance) const {return true;}
	};
}	// namespace RBX