/**
 @file Vector3int16.cpp
 
 @author Morgan McGuire, http://graphics.cs.williams.edu
  
 @created 2003-04-07
 @edited  2006-01-17
 */

#include "G3D/platform.h"
#include "G3D/g3dmath.h"
#include "G3D/Vector3int16.h"
#include "G3D/Vector3.h"
#include "G3D/format.h"

namespace G3D {

Vector3int16::Vector3int16(const class Vector3& v) {
    x = (int16)iFloor(v.x + 0.5);
    y = (int16)iFloor(v.y + 0.5);
    z = (int16)iFloor(v.z + 0.5);
}

std::string Vector3int16::toString() const {
    return G3D::format("(%d, %d, %d)", x, y, z);
}

std::ostream& operator<<(std::ostream& os, const Vector3int16& v) {
    return os << v.toString();
}

const Vector3int16& Vector3int16::zero() { static const Vector3int16 v(0, 0, 0); return v; }
const Vector3int16& Vector3int16::one() { static const Vector3int16 v(1, 1, 1); return v; }

}
