#pragma once

#include "solver/SolverConfig.h"
#include "solver/SolverContainers.h"
#include "G3D/Vector3.h"

#include "boost/limits.hpp"
#include "boost/math/constants/constants.hpp"
#include "boost/container/vector.hpp"

#include <utility>

#include "simd/simd.h"
#include "rbx/ArrayDynamic.h"

namespace RBX
{

class DebugSerializer;
class ConstraintJacobianPair;

//
// Use class definition rather than typedef so we can forward declare
//
class BodyPairIndices: public std::pair< int, int >
{
public:
    inline BodyPairIndices() { }
    inline BodyPairIndices( int a, int b ): std::pair< int, int >( a, b ) { }
};

//
// VirtualDisplacement: http://en.wikipedia.org/wiki/Virtual_displacement
// A 6-dimensional vector with a linear part and angular part
//

// This is a version that allows conversion from non-simd to simd types.
// The non-simd is still used by integration.
class VirtualDisplacementPOD
{
public:
    void serialize( DebugSerializer& s ) const;
    union
    {
        simd::v4f_pod linV4;
        Vector3Pod lin;
    };
    union
    {
        simd::v4f_pod angV4;
        Vector3Pod ang;
    };
};

// This is a version that can be worked with in simd
// It's only ever used on the stack
class VirtualDisplacement
{
public:
    RBX_SIMD_INLINE VirtualDisplacement( ){ }
    RBX_SIMD_INLINE VirtualDisplacement( const simd::v4f& linear, const simd::v4f& angular ): lin( linear ), ang( angular ) { }
    RBX_SIMD_INLINE VirtualDisplacement( const VirtualDisplacementPOD& _v ): lin( _v.linV4 ), ang( _v.angV4 ) { }
    RBX_SIMD_INLINE operator VirtualDisplacementPOD()
    {
        VirtualDisplacementPOD r;
        r.linV4 = lin;
        r.angV4 = ang;
        return r;
    }

    RBX_SIMD_INLINE void reset()
    {
        lin = simd::zerof();
        ang = simd::zerof();
    }

    RBX_SIMD_INLINE simd::v4f getLin() const { return lin; }
    RBX_SIMD_INLINE simd::v4f getAng() const { return ang; }

    void serialize( DebugSerializer& s ) const;

private:
    simd::v4f lin;
    simd::v4f ang;
};

//
// Array of virtual displacements
//
class VirtualDisplacementArray
{
public:
    RBX_SIMD_INLINE VirtualDisplacementArray( size_t _size, size_t alignment ): size( _size ), data( _size, ArrayNoInit(), alignment )
    { }

    inline void reset()
    {
        VirtualDisplacement z( simd::zerof(), simd::zerof() );
        for( size_t i = 0; i < size; i++ )
        {
            data[ i ] = z;
        }
    }

    const VirtualDisplacementPOD* getData() const { return data.data(); }
    VirtualDisplacementPOD* getData() { return data.data(); }
    RBX_SIMD_INLINE size_t getSize() const { return size; }

    RBX_SIMD_INLINE VirtualDisplacementPOD operator[]( int i ) const
    {
        RBXASSERT_VERY_FAST( (size_t)i < size );
        return data[ i ];
    }

    RBX_SIMD_INLINE VirtualDisplacementPOD& operator[]( int i )
    {
        RBXASSERT_VERY_FAST( (size_t)i < size );
        return data[ i ];
    }

    void serialize( DebugSerializer& s ) const;

private:
    size_t size;
    ArrayDynamic< VirtualDisplacementPOD > data;
};

//
// Effective mass vectors: inverse mass matrix * jacobian
//
class EffectiveMass
{
public:
    RBX_SIMD_INLINE EffectiveMass() { }
    RBX_SIMD_INLINE EffectiveMass( const simd::v4f& linear, const simd::v4f& angular ): lin( linear ), ang( angular ) { }
    RBX_SIMD_INLINE void applyMultiplier( const simd::v4f& m )
    {
        lin = m * lin;
        ang = m * ang;
    }
    RBX_SIMD_INLINE void reset()
    {
        lin = simd::zerof();
        ang = simd::zerof();
    }
    RBX_SIMD_INLINE simd::v4f getLin() const { return lin; }
    RBX_SIMD_INLINE simd::v4f getAng() const { return ang; }

private:
    simd::v4f lin;
    simd::v4f ang;
};

class EffectiveMassPair
{
public:
    RBX_SIMD_INLINE EffectiveMassPair( const EffectiveMass& _a, const EffectiveMass& _b ): a( _a ), b( _b ) { }

    RBX_SIMD_INLINE EffectiveMassPair( ) { }

    RBX_SIMD_INLINE void reset()
    {
        a.reset();
        b.reset();
    }

    RBX_SIMD_INLINE void applyMultipliers( const simd::v4f& mA, const simd::v4f& mB )
    {
        a.applyMultiplier( mA );
        b.applyMultiplier( mB );
    }

    RBX_SIMD_INLINE simd::v4f getLinA() const { return a.getLin(); }
    RBX_SIMD_INLINE simd::v4f getLinB() const { return b.getLin(); }
    RBX_SIMD_INLINE simd::v4f getAngA() const { return a.getAng(); }
    RBX_SIMD_INLINE simd::v4f getAngB() const { return b.getAng(); }

    RBX_SIMD_INLINE EffectiveMass getPartA() const { return a; }
    RBX_SIMD_INLINE EffectiveMass getPartB() const { return b; }

    void serialize( DebugSerializer& s ) const;

private:
    EffectiveMass a;
    EffectiveMass b;
};

//
// Jacobian of a binary constraint
//
class ConstraintJacobian
{
public:
    RBX_SIMD_INLINE void reset()
    {
        linV4 = simd::zerof();
        angV4 = simd::zerof();
    }

    union
    {
        Vector3Pod lin;
        simd::v4f_pod linV4;
    };
    union
    {
        Vector3Pod ang;
        simd::v4f_pod angV4;
    };
};

class ConstraintJacobianPair
{
public:
    class LinA;
    class LinB;
    class AngA;
    class AngB;

    template< class PartSelect >
    RBX_SIMD_INLINE simd::v4f get() const;

    RBX_SIMD_INLINE simd::v4f getLinA() const { return a.linV4; }
    RBX_SIMD_INLINE simd::v4f getLinB() const { return b.linV4; }
    RBX_SIMD_INLINE simd::v4f getAngA() const { return a.angV4; }
    RBX_SIMD_INLINE simd::v4f getAngB() const { return b.angV4; }

    template< class PartSelect >
    RBX_SIMD_INLINE void set( const simd::v4f& v );

    RBX_SIMD_INLINE void setLinA( const simd::v4f& v ) { a.linV4 = v; }
    RBX_SIMD_INLINE void setLinB( const simd::v4f& v ) { b.linV4 = v; }
    RBX_SIMD_INLINE void setAngA( const simd::v4f& v ) { a.angV4 = v; }
    RBX_SIMD_INLINE void setAngB( const simd::v4f& v ) { b.angV4 = v; }

    RBX_SIMD_INLINE void reset()
    {
        a.reset();
        b.reset();
    }

    RBX_SIMD_INLINE simd::v4f dot( const EffectiveMassPair& _v ) const
    {
        simd::v4f partA = getLinA() * _v.getLinA() + getAngA() * _v.getAngA();
        simd::v4f partB = getLinB() * _v.getLinB() + getAngB() * _v.getAngB();
        simd::v4f r = partA + partB;
        return simd::splat<0>(r) + simd::splat<1>(r) + simd::splat<2>(r);
    }

    void serialize( DebugSerializer& s ) const;

    ConstraintJacobian a;
    ConstraintJacobian b;
};

template< >
RBX_SIMD_INLINE simd::v4f ConstraintJacobianPair::get< ConstraintJacobianPair::LinA >() const { return a.linV4; }
template< >
RBX_SIMD_INLINE simd::v4f ConstraintJacobianPair::get< ConstraintJacobianPair::LinB >() const { return b.linV4; }
template< >
RBX_SIMD_INLINE simd::v4f ConstraintJacobianPair::get< ConstraintJacobianPair::AngA >() const { return a.angV4; }
template< >
RBX_SIMD_INLINE simd::v4f ConstraintJacobianPair::get< ConstraintJacobianPair::AngB >() const { return b.angV4; }

template< >
RBX_SIMD_INLINE void ConstraintJacobianPair::set< ConstraintJacobianPair::LinA >( const simd::v4f& v ) { a.linV4 = v; }
template< >
RBX_SIMD_INLINE void ConstraintJacobianPair::set< ConstraintJacobianPair::LinB >( const simd::v4f& v ) { b.linV4 = v; }
template< >
RBX_SIMD_INLINE void ConstraintJacobianPair::set< ConstraintJacobianPair::AngA >( const simd::v4f& v ) { a.angV4 = v; }
template< >
RBX_SIMD_INLINE void ConstraintJacobianPair::set< ConstraintJacobianPair::AngB >( const simd::v4f& v ) { b.angV4 = v; }

}
