#pragma once

#include "V8Tree/Instance.h"

#include "Util/NormalId.h"
#include "GfxBase/IAdornable.h"

namespace RBX {

	// An instance that is designed to "attach" itself to a face of its parent PartInstance
	extern const char* const sFaceInstance;
	class FaceInstance 
		: public Reflection::Described<FaceInstance, sFaceInstance, Instance>
		, public IAdornable
	{	
	private:
		typedef Reflection::Described<FaceInstance, sFaceInstance, Instance> Super;
		NormalId face;
		
		//Instance
		/*override*/ bool askSetParent(const Instance* instance) const;

		//IAdornable
		/*override*/ void render3dSelect(Adorn* adorn, SelectState selectState);
	public:
		FaceInstance(void);

		static const Reflection::EnumPropDescriptor<FaceInstance, NormalId > prop_Face;
		NormalId getFace() const { return face; }
		void setFace(RBX::NormalId value);
	};

}
