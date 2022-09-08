#include "stdafx.h"

#include "Util/SurfaceType.h"
#include "Reflection/EnumConverter.h"


namespace RBX {
namespace Reflection {
	//////////////////////////////////////////////////////////
	template<>
	EnumDesc<SurfaceType>::EnumDesc()
		:EnumDescriptor("SurfaceType")
	{
		addPair(NO_SURFACE, "Smooth");
		addPair(GLUE, "Glue");
		addPair(WELD, "Weld");
		addPair(STUDS, "Studs");
		addPair(INLET, "Inlet");
		addPair(UNIVERSAL, "Universal");
		addLegacyName("Spawn", NO_SURFACE);
		addPair(ROTATE, "Hinge");
		addPair(ROTATE_V, "Motor");
		addPair(ROTATE_P, "SteppingMotor");
		addPair(NO_JOIN, "Unjoinable");
		addPair(NO_SURFACE_NO_OUTLINES, "SmoothNoOutlines");

		addLegacyName("Bumps", GLUE);
	}
}//namespace Reflection
}// namespace RBX

