/**
  Quaternion.inl
 
 */

namespace RBX {

inline Quaternion& Quaternion::operator+= (const Quaternion& rkQuaternion) {
    x += rkQuaternion.x;
    y += rkQuaternion.y;
    z += rkQuaternion.z;
    w += rkQuaternion.w;
    return *this;
}

inline Quaternion& Quaternion::operator*= (float fScalar) {
    x *= fScalar;
    y *= fScalar;
    z *= fScalar;
    w *= fScalar;
    return *this;
}

} // namespace
