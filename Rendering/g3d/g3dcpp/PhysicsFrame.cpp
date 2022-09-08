/**
 @file PhysicsFrame.cpp

 @maintainer Morgan McGuire, http://graphics.cs.williams.edu
 
 @created 2002-07-09
 @edited  2010-03-25
*/

#include "G3D/platform.h"
#include "G3D/stringutils.h"
#include "G3D/PhysicsFrame.h"

namespace G3D {

PhysicsFrame::PhysicsFrame() {
    translation = Vector3::zero();
    rotation    = Quat();
}


PhysicsFrame::PhysicsFrame(
    const CoordinateFrame& coordinateFrame) {

    translation = coordinateFrame.translation;
    rotation    = Quat(coordinateFrame.rotation);
}


PhysicsFrame PhysicsFrame::operator*(const PhysicsFrame& other) const {
    PhysicsFrame result;

    result.rotation = rotation * other.rotation;
    result.translation = translation + rotation.toRotationMatrix() * other.translation;

    return result;
}


PhysicsFrame::operator CoordinateFrame() const {
    CoordinateFrame f;
    
    f.translation = translation;
    f.rotation    = rotation.toRotationMatrix();

    return f;
}


PhysicsFrame PhysicsFrame::lerp(
    const PhysicsFrame&     other,
    float                   alpha) const {

    PhysicsFrame result;

    result.translation = translation.lerp(other.translation, alpha);
    result.rotation    = rotation.slerp(other.rotation, alpha);

    return result;
}


}; // namespace

