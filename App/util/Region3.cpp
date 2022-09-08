#include "stdafx.h"

#include "Util/Region3.h"
#include "Util/Extents.h"
#include "Reflection/Type.h"
namespace RBX {
	
namespace Reflection
{
	template<>
	const Type& Type::getSingleton<Region3>()
	{
		static TType<Region3> type("Region3");
		return type;
	}
}
Region3::Region3()
{}

Region3::Region3(const Vector3& min, const Vector3& max)
{	
	init(Extents(min, max));
}

Region3::Region3(const Extents &extents)
{
	init(extents);
}

void Region3::init(const Extents &extents)
{
	cframe = CoordinateFrame(extents.center());
	size = extents.size();
}

Vector3 Region3::minPos() const
{
	return cframe.translation - (size * 0.5f);
}
Vector3 Region3::maxPos() const
{
	return cframe.translation + (size * 0.5f);
}

}