/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

// NOTE: both Surface.h and Surfaces.h

#include "V8World/Controller.h"
#include "Util/SurfaceType.h"
#include "Util/NormalId.h"

class XmlElement;

namespace RBX {

	class PartInstance;
	class IReferenceBinder;

	namespace Reflection
	{
		class PropertyDescriptor;
	}

	class Surface
	{
	private:
		PartInstance*				partInstance;
		NormalId					surfId;

	public:
		void flat();

		Surface();

		Surface(
			PartInstance* partInstance,
			NormalId surfId
			);

		Surface(const Surface& lhs);

		PartInstance* getPartInstance() {return partInstance;}
		const PartInstance* getConstPartInstance() const {return partInstance;}
		NormalId getNormalId() const {return surfId;}

		SurfaceType getSurfaceType() const;
		void setSurfaceType(SurfaceType type);

		LegacyController::InputType getInput() const;
		void setSurfaceInput(LegacyController::InputType inputType);

		float getParamA() const;
		float getParamB() const;
		void setParamA(float _paramA);
		void setParamB(float _paramB);

		void toggleSurfaceType();

		static const Reflection::PropertyDescriptor& getSurfaceTypeStatic(NormalId face);
		static const Reflection::PropertyDescriptor& getSurfaceInputStatic(NormalId face);
		static const Reflection::PropertyDescriptor& getParamAStatic(NormalId face);
		static const Reflection::PropertyDescriptor& getParamBStatic(NormalId face);

		static bool isSurfaceDescriptor(const Reflection::PropertyDescriptor& desc);
		static void registerSurfaceDescriptors();
	};

} // namespace