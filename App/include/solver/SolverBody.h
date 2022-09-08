#pragma once

#include "G3D/Vector3.h"
#include "simd/simd.h"
#include "solver/SolverContainers.h"
#include "solver/ConstraintJacobian.h"

namespace RBX
{
    class DebugSerializer;

    //
    // Internal representation of dynamic properties of a body (for both simulated and anchored)
    //
    class SolverBodyDynamicProperties
    {
    public:
        void serialize( DebugSerializer& s ) const;

        Vector3 integratedLinearVelocity;
        Vector3 integratedAngularVelocity;
        Matrix3 orientation;
        Vector3 position;
        Vector3 linearVelocity;
        Vector3 angularVelocity;
    };

    //
    // Symmetric matrix for use as inertia matrix
    //
    class SymmetricMatrix
    {
    public:
        RBX_SIMD_INLINE Vector3 operator*( const Vector3& v ) const
        {
            Vector3 r;
            r.x = diagonals.x * v.x + offDiagonals.x * v.y + offDiagonals.y * v.z;
            r.y = offDiagonals.x * v.x + diagonals.y * v.y + offDiagonals.z * v.z;
            r.z = offDiagonals.y * v.x + offDiagonals.z * v.y + diagonals.z * v.z;
            return r;
        }

        RBX_SIMD_INLINE SymmetricMatrix operator*( float s ) const
        {
            SymmetricMatrix r;
            r.diagonals = s * diagonals;
            r.offDiagonals = s * offDiagonals;
            return r;
        }

        void serialize( DebugSerializer& s ) const;

        // [ d0 a  b ]
        // [ a  d1 c ]
        // [ b  c  d2]
        Vector3Pod diagonals;    // [d0, d1, d2]
        Vector3Pod offDiagonals; // [a, b, c]
    };

    class SymmetricMatrixPOD
    {
    public:
        simd::v4f_pod diagonals;
        simd::v4f_pod offDiagonals;
    };
    
    //
    // SymmetricMatrixSIMD
    //
    class SymmetricMatrixSIMD
    {
    public:
        RBX_SIMD_INLINE SymmetricMatrixSIMD( const float* _m )
        {
            diagonals = simd::load3( _m );
            offDiagonals = simd::load3( _m + 3 );
        }

        RBX_SIMD_INLINE SymmetricMatrixSIMD( const simd::v4f& _diagonal, const simd::v4f& _offDiagonal ): diagonals( _diagonal ), offDiagonals( _offDiagonal ) { }

        RBX_SIMD_INLINE SymmetricMatrixSIMD( const SymmetricMatrixPOD& _m ): diagonals( _m.diagonals ), offDiagonals( _m.offDiagonals ) { }

        template< int row, int column >
        RBX_SIMD_INLINE simd::v4f get() const;

        RBX_SIMD_INLINE simd::v4f operator*( const simd::v4f& v ) const
        {
            simd::v4f t0 = diagonals * v;
            simd::v4f t1 = simd::permute<0, 2, 1, 3>( offDiagonals ) * simd::permute< 1, 2, 0, 3>( v );
            simd::v4f t2 = simd::permute<1, 0, 2, 3>( offDiagonals ) * simd::permute< 2, 0, 1, 3>( v );
            return t0 + t1 + t2;
        }

        RBX_SIMD_INLINE void invert()
        {
            simd::v4f x00x00x02x01 = simd::shuffle< 0, 0, 1, 0 >( diagonals, offDiagonals );
            simd::v4f x11x00x01x12 = simd::shuffle< 1, 0, 0, 2 >( diagonals, offDiagonals );
            simd::v4f x22x00x12x02 = simd::shuffle< 2, 0, 2, 1 >( diagonals, offDiagonals );

            simd::v4f x00x00x11x22 = simd::permute< 0, 0, 1, 2 >( diagonals );
            simd::v4f x12x12x02x01 = simd::permute< 2, 2, 1, 0 >( offDiagonals );

            simd::v4f r = x00x00x02x01 * x11x00x01x12 * x22x00x12x02 - x00x00x11x22 * x12x12x02x01 * x12x12x02x01;
            simd::v4f d = simd::splat< 0 >( r ) + simd::splat< 2 >( r ) + simd::splat< 3 >( r );
            simd::v4f dInv = ( simd::splat( 1.0f ) / d );

            simd::v4f x11x22x00 = simd::permute< 1, 2, 0, 3 >( diagonals );
            simd::v4f x22x00x11 = simd::permute< 2, 0, 1, 3 >( diagonals );
            simd::v4f x12x02x01 = simd::permute< 2, 1, 0, 3 >( offDiagonals );
            simd::v4f newDiagonals = dInv * ( x11x22x00 * x22x00x11 - x12x02x01 * x12x02x01 );

            simd::v4f x12x01x02 = simd::permute< 2, 0, 1, 3 >( offDiagonals );
            simd::v4f x02x12x01 = simd::permute< 1, 2, 0, 3 >( offDiagonals );
            simd::v4f x01x02x12 = offDiagonals;
            simd::v4f x22x11x00 = simd::permute< 2, 1, 0, 3 >( diagonals );
            simd::v4f newOffdiagonals = dInv * ( x12x01x02 * x02x12x01 - x01x02x12 * x22x11x00 );

            diagonals = newDiagonals;
            offDiagonals = newOffdiagonals;
        }

        simd::v4f diagonals;
        simd::v4f offDiagonals;
    };

    template< >
    RBX_SIMD_INLINE simd::v4f SymmetricMatrixSIMD::get<0,0>() const { return simd::splat<0>( diagonals ); }
    template< > 
    RBX_SIMD_INLINE simd::v4f SymmetricMatrixSIMD::get<1,1>() const { return simd::splat<1>( diagonals ); }
    template< > 
    RBX_SIMD_INLINE simd::v4f SymmetricMatrixSIMD::get<2,2>() const { return simd::splat<2>( diagonals ); }
    template< >
    RBX_SIMD_INLINE simd::v4f SymmetricMatrixSIMD::get<0,1>() const { return simd::splat<0>( offDiagonals ); }
    template< > 
    RBX_SIMD_INLINE simd::v4f SymmetricMatrixSIMD::get<0,2>() const { return simd::splat<1>( offDiagonals ); }
    template< > 
    RBX_SIMD_INLINE simd::v4f SymmetricMatrixSIMD::get<1,2>() const { return simd::splat<2>( offDiagonals ); }
    template< >
    RBX_SIMD_INLINE simd::v4f SymmetricMatrixSIMD::get<1,0>() const { return get<0,1>(); }
    template< > 
    RBX_SIMD_INLINE simd::v4f SymmetricMatrixSIMD::get<2,0>() const { return get<0,2>(); }
    template< > 
    RBX_SIMD_INLINE simd::v4f SymmetricMatrixSIMD::get<2,1>() const { return get<1,2>(); }

    static RBX_SIMD_INLINE SymmetricMatrixSIMD operator*( const simd::v4f& s, const SymmetricMatrixSIMD& m )
    {
        return SymmetricMatrixSIMD( s * m.diagonals, s* m.offDiagonals );
    }

    //
    // SymmetricMatrix2SIMD
    //
    class SymmetricMatrix2SIMD
    {
    public:
        RBX_SIMD_INLINE SymmetricMatrix2SIMD( ) { }

        RBX_SIMD_INLINE SymmetricMatrix2SIMD( const simd::v4f& d00, const simd::v4f& d11, const simd::v4f& d01 )
        {
            m = simd::gatherX( d00, d01, d01, d11 );
        }

        RBX_SIMD_INLINE void load( const float* _m )
        {
            m = simd::form( _m[0], _m[2], _m[2], _m[1] );
        }

        RBX_SIMD_INLINE void form( const simd::v4f& d00, const simd::v4f& d11, const simd::v4f& d01 )
        {
            m = simd::gatherX( d00, d01, d01, d11 );
        }

        RBX_SIMD_INLINE simd::v4f operator*( const simd::v4f& v ) const
        {
            simd::v4f t0 = m * simd::permute< 0, 0, 1, 1 >( v );
            simd::v4f t1 = simd::permute< 2, 3, 0, 1 >( t0 );
            return t0 + t1;
        }

        RBX_SIMD_INLINE void invert()
        {
            simd::v4f xt = simd::splat< 1 >( m );
            simd::v4f det = simd::splat< 0 >( m ) * simd::splat< 3 >( m ) - xt * xt;
            simd::v4f t = simd::select< 0, 1, 1, 0 >( simd::splat( 1.0f ), simd::splat( -1.0f ) ) * simd::permute< 3, 2, 1, 0 >( m );
            m = ( t / det );
        }

        template< int row, int column >
        RBX_SIMD_INLINE simd::v4f get() const;

        simd::v4f m;
    };

    template<  >
    RBX_SIMD_INLINE simd::v4f SymmetricMatrix2SIMD::get<0,0>() const { return simd::splat<0>( m ); }
    template<  >
    RBX_SIMD_INLINE simd::v4f SymmetricMatrix2SIMD::get<1,0>() const { return simd::splat<1>( m ); }
    template<  >
    RBX_SIMD_INLINE simd::v4f SymmetricMatrix2SIMD::get<0,1>() const { return simd::splat<1>( m ); }
    template<  >
    RBX_SIMD_INLINE simd::v4f SymmetricMatrix2SIMD::get<1,1>() const { return simd::splat<3>( m ); }

    class SolverBodyMassAndInertia
    {
    public:
        void serialize( DebugSerializer& s ) const;

        RBX_SIMD_INLINE SymmetricMatrixSIMD getInvInertiaVelStage() const
        {
            return inertiaSIMD;
        }

        RBX_SIMD_INLINE SymmetricMatrixSIMD getInvInertiaPosStage( float scale ) const
        {
            simd::v4f inertiaScale = simd::splat( scale ) * simd::splat( posToVelMassRatio );
            SymmetricMatrixSIMD r( inertiaSIMD );
            return inertiaScale * r;
        }

        RBX_SIMD_INLINE simd::v4f getInvMassVelStage() const
        {
            return simd::splat( massInvVelStage );
        }

        RBX_SIMD_INLINE simd::v4f getInvMassPosStage() const
        {
            return simd::splat( massInvVelStage * posToVelMassRatio );
        }

        class VelStage;
        class PosStage;

        template< class StageSelect >
        RBX_SIMD_INLINE simd::v4f getInvMass() const;

        template< class StageSelect >
        RBX_SIMD_INLINE SymmetricMatrixSIMD getInvInertia( float scale ) const;

        union
        {
            struct
            {
                Vector3Pod inertiaDiagonal;
                float massInvVelStage;
                Vector3Pod inertiaOffDiagonal;
                float posToVelMassRatio;
            };
            SymmetricMatrixPOD inertiaSIMD;
        };
    };

    template<>
    RBX_SIMD_INLINE simd::v4f SolverBodyMassAndInertia::getInvMass< SolverBodyMassAndInertia::VelStage >() const
    {
        return getInvMassVelStage();
    }

    template<>
    RBX_SIMD_INLINE simd::v4f SolverBodyMassAndInertia::getInvMass< SolverBodyMassAndInertia::PosStage >() const
    {
        return getInvMassPosStage();
    }

    template< >
    RBX_SIMD_INLINE SymmetricMatrixSIMD SolverBodyMassAndInertia::getInvInertia< SolverBodyMassAndInertia::VelStage >( float scale ) const
    {
        return getInvInertiaVelStage();
    }

    template< >
    RBX_SIMD_INLINE SymmetricMatrixSIMD SolverBodyMassAndInertia::getInvInertia< SolverBodyMassAndInertia::PosStage >( float scale ) const
    {
        return getInvInertiaPosStage( scale );
    }

    //
    // Static properties of a body
    //
    class SolverBodyStaticProperties
    {
    public:
        void serialize( DebugSerializer& s ) const;

        boost::uint64_t bodyUID;
        boost::uint32_t guid;
        bool isStatic;
    };
}
