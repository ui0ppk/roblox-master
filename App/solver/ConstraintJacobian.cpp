#include "stdafx.h"
#include "solver/ConstraintJacobian.h"
#include "solver/DebugSerializer.h"

namespace RBX
{

void VirtualDisplacementArray::serialize( DebugSerializer& s ) const
{
    s & data;
}

void VirtualDisplacementPOD::serialize( DebugSerializer& s ) const
{
    s & lin & ang;
}

void ConstraintJacobianPair::serialize( DebugSerializer& s ) const
{
    s & a.lin & a.ang & b.lin & b.ang;
}

static inline void serializeVector3( DebugSerializer& s, const simd::v4f& v )
{
    s & simd::extractSlow( v, 0 ) & simd::extractSlow( v, 1 ) & simd::extractSlow( v, 2 );
}

void EffectiveMassPair::serialize( DebugSerializer& s ) const
{
    serializeVector3( s, a.getLin() );
    serializeVector3( s, a.getAng() );
    serializeVector3( s, b.getLin() );
    serializeVector3( s, b.getAng() );
}

}
