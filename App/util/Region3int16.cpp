#include "stdafx.h"

#include "Util/Region3int16.h"
#include "Util/ExtentsInt32.h"
#include "Reflection/Type.h"
namespace RBX {
	
namespace Reflection
{
	template<>
	const Type& Type::getSingleton<Region3int16>()
	{
		static TType<Region3int16> type("Region3int16");
		return type;
	}
}

}