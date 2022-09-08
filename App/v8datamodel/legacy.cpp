#include "stdafx.h"

#include "V8DataModel/legacy.h"
#include "Util/SurfaceType.h"
#include "Reflection/EnumConverter.h"

namespace RBX {
namespace Reflection {
	template<>
	EnumDesc<Legacy::SurfaceConstraint>::EnumDesc()
		:EnumDescriptor("SurfaceConstraint")
	{
		addPair(Legacy::NO_CONSTRAINT, "None");
		addPair(Legacy::ROTATE_LEGACY, "Hinge");
		addPair(Legacy::ROTATE_P_LEGACY, "SteppingMotor");
		addPair(Legacy::ROTATE_V_LEGACY, "Motor");
	}
}//namespace Reflection
}//namespace RBX
