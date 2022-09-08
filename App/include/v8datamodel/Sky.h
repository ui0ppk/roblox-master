#pragma once

//#include "V8dataModel/FaceInstance.h"
#include "V8Tree/instance.h"
#include "Util/TextureId.h"

namespace RBX {

	extern const char* const sSky;
	class Sky : public DescribedCreatable<Sky, Instance, sSky>
	{
	public:
		TextureId skyUp;
		TextureId skyLf;
		TextureId skyRt;
		TextureId skyBk;
		TextureId skyFt;
		TextureId skyDn;
		bool drawCelestialBodies;
		TextureId sunTexture;
		TextureId moonTexture;
		float sunAngularSize;
		float moonAngularSize;
	private:
		int numStars;
	public:
		Sky();
		int getNumStars() const { return numStars; }
		void setNumStars(int value);

		void setSkyboxUp(const TextureId& texId);
		void setSkyboxLf(const TextureId& texId);
		void setSkyboxRt(const TextureId& texId);
		void setSkyboxBk(const TextureId& texId);
		void setSkyboxDn(const TextureId& texId);
		void setSkyboxFt(const TextureId& texId);
		void setSunTexture(const TextureId& texId);
		void setMoonTexture(const TextureId& texId);
		void setSunAngularSize(float angularSize);
		void setMoonAngularSize(float angularSize);

		const TextureId& getSkyboxUp()        const { return skyUp;           }
		const TextureId& getSkyboxLf()        const { return skyLf;           }
		const TextureId& getSkyboxRt()        const { return skyRt;           }
		const TextureId& getSkyboxBk()        const { return skyBk;           }
		const TextureId& getSkyboxDn()        const { return skyDn;           }
		const TextureId& getSkyboxFt()        const { return skyFt;           }
		const TextureId& getSunTexture()      const { return sunTexture;      }
		const TextureId& getMoonTexture()     const { return moonTexture;     }
		const float&     getSunAngularSize()  const { return sunAngularSize;  }
		const float&     getMoonAngularSize() const { return moonAngularSize; }

		static Reflection::PropDescriptor<Sky, int> prop_StarCount;
		static Reflection::PropDescriptor<Sky, TextureId> prop_SkyUp;
		static Reflection::PropDescriptor<Sky, TextureId> prop_SkyLf;
		static Reflection::PropDescriptor<Sky, TextureId> prop_SkyRt;
		static Reflection::PropDescriptor<Sky, TextureId> prop_SkyBk;
		static Reflection::PropDescriptor<Sky, TextureId> prop_SkyFt;
		static Reflection::PropDescriptor<Sky, TextureId> prop_SkyDn;
		static Reflection::BoundProp<bool> prop_CelestialBodiesShown;
		static Reflection::PropDescriptor<Sky, TextureId> prop_SunTexture;
		static Reflection::PropDescriptor<Sky, TextureId> prop_MoonTexture;
		static Reflection::PropDescriptor<Sky, float> prop_SunAngularSize;
		static Reflection::PropDescriptor<Sky, float> prop_MoonAngularSize;
	};
}