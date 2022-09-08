#include "stdafx.h"

#include "solver/SolverKernel.h"
#include "solver/Constraint.h"
#include "solver/ConstraintJacobian.h"
#include "solver/SolverBody.h"

#include "G3D/Matrix2.h"
#include "G3D/Matrix3.h"

#include "simd/simd.h"

#include "boost/limits.hpp"

#include "rbx/Profiler.h"

namespace RBX
{
using namespace simd;

//
// Core of the PGS solver. Uses the Block Projected Gauss Seidel for constraints, and a modified PGS for collisions to take into account
// the non-linear dependence between the friction impulse boundaries and the normal impulse
// It solves a linear system: K * I = R with boundary constraints in each coordinate of I, where
//  K = J * W * J^t
//  I - impulses / Lagrangian multipliers
//  R - reaction vector
// It uses a factorization of K * I that allows for an optimization of the product of the matrix row against vector: K_i * I
// This factorization is K * I = J * ( W * J^t * I ) = J * D
// Where D = W * J^t * I is called the virtual displacement vector. It's been precomupted in PGSPrepareVirtualDisplacements.
// Effectively it reduces the computation of K_i * I to a sum of 4 dot products.
// The assumption being that a constraint is bilinear (i.e. affects only two rigid bodies)
// The PGS works is similar to Jacobi Method.
// The iteration Step for Jacobi is:
// I' = I + P^-1 * ( R - K * I )
// (For GS it is: I_i = I_i + [ P^-1 * ( R - K * I ) ]_i for i running over all constrained dimensions).
// where P is a pre-conditioner, that is an approximation to K that is easy to inverse.
// In regular GS P is the diagonal of K.
// For block GS, we choose P to be block diagonal, where the blocks are of dimension 1 to 3 corresponding to the linear/angular part of a constraint.
// We can do larger blocks, but requires implementing a numerical inverse computation of P.
// 
// The iteration step is rewritten as:
// I' = I + P^-1 * R - ( P^-1 * J ) * D
// and P^-1 * R and P^-1 * J are precomputed outside the main iteration loop.
// 

typedef ConstraintJacobianPair Jac;
typedef VirtualDisplacement VD;

//---------------------------------------------------------------------------------------------
//
// Pre-compute effective masses: Inverse Mass Matrix times the Jacobian transpose, W J^t
//
//---------------------------------------------------------------------------------------------

// Helper method
template< class StageSelector >
static RBX_SIMD_INLINE EffectiveMassPair computeEffectiveMassPair( 
    const SolverBodyMassAndInertia& _bodyA, 
    const SolverBodyMassAndInertia& _bodyB,
    const ConstraintJacobianPair& _j, 
    const SolverConfig& _config )
{
    EffectiveMass emA( 
        _bodyA.getInvMass< StageSelector >() * _j.getLinA(), 
        _bodyA.getInvInertia< StageSelector >( _config.stabilizationInertiaScale ) * _j.getAngA() );
    EffectiveMass emB( 
        _bodyB.getInvMass< StageSelector >() * _j.getLinB(), 
        _bodyB.getInvInertia< StageSelector >( _config.stabilizationInertiaScale ) * _j.getAngB() );
    return EffectiveMassPair( emA, emB );
}

// Pre-compute effective masses
void PGSComputeEffectiveMasses( 
    EffectiveMassPair* __restrict _effectiveMassesVelStage,
    EffectiveMassPair* __restrict _effectiveMassesPosStage,
    size_t _constraintCount, 
    const boost::uint8_t* _dimensions, 
    const ConstraintJacobianPair* _jacobians, 
    const BodyPairIndices* _pairs,
    const SolverBodyMassAndInertia* _massAndIntertia,
    const SolverConfig& _config )
{
    RBXPROFILER_SCOPE("Physics", "PGSComputeEffectiveMasses");

    boost::uint32_t offset = 0;
    for( unsigned c = 0; c < _constraintCount; c++ )
    {
        boost::uint8_t d = _dimensions[ c ];
        const ConstraintJacobianPair* currentJacobian = _jacobians + offset;
        const SolverBodyMassAndInertia& bodyA = _massAndIntertia[ _pairs[c].first ];
        const SolverBodyMassAndInertia& bodyB = _massAndIntertia[ _pairs[c].second ];

        EffectiveMassPair* __restrict currentEffectiveMassVelStage = _effectiveMassesVelStage + offset;
        EffectiveMassPair* __restrict currentEffectiveMassPosStage = _effectiveMassesPosStage + offset;
        for( boost::uint8_t i = 0; i < d; i++ )
        {
            currentEffectiveMassVelStage[ i ] = computeEffectiveMassPair< SolverBodyMassAndInertia::VelStage >( bodyA, bodyB, currentJacobian[ i ], _config );
            currentEffectiveMassPosStage[ i ] = computeEffectiveMassPair< SolverBodyMassAndInertia::PosStage >( bodyA, bodyB, currentJacobian[ i ], _config );
        }

        offset += d;
    }
}

//---------------------------------------------------------------------------------------------
//
// Applying pre-conditioner
// This involves computing the pre-conditioner which is the inverse of the diagonal block corresponding to each constraint,
// and multiplying it with the Jacobian and reaction vectors.
//
//---------------------------------------------------------------------------------------------

// Helper methods
template< int DIM >
static RBX_SIMD_INLINE void applyPreconditionerToStage( 
    ConstraintVariables* __restrict _vars,
    ConstraintJacobianPair* __restrict _preJacobians,
    const boost::uint8_t* _useBlock,
    const ConstraintJacobianPair* __restrict _jacobian,
    const float* __restrict _sor,
    const EffectiveMassPair* __restrict _effectiveMasses );

//
// Dim 1 pre-conditioner
//

// Helper methods
static RBX_SIMD_INLINE void applyPreconditionerToJacobian( ConstraintJacobianPair* __restrict _r, const v4f& _preconditioner, const ConstraintJacobianPair* __restrict _j, const v4f& _sor )
{
    _r[0].setLinA( ( _sor * _preconditioner ) * _j[0].getLinA() );
    _r[0].setAngA( ( _sor * _preconditioner ) * _j[0].getAngA() );
    _r[0].setLinB( ( _sor * _preconditioner ) * _j[0].getLinB() );
    _r[0].setAngB( ( _sor * _preconditioner ) * _j[0].getAngB() );
}

// Dim 1 pre-conditioner
// For a constraint of dimension 1, the pre-conditioner is just the inverse of the corresponding diagonal element.
template< >
RBX_SIMD_INLINE void applyPreconditionerToStage< 1 >(
    ConstraintVariables* __restrict _vars,
    ConstraintJacobianPair* __restrict _preJacobians,
    const boost::uint8_t* _useBlock,
    const ConstraintJacobianPair* __restrict _jacobian,
    const float* __restrict _sor,
    const EffectiveMassPair* __restrict _effectiveMasses )
{
    // Compute the inverse of the diagonal element of the constraint matrix, which is the dot product of the jacobian with the effective mass
    v4f precond = ( splat(1.0f) / _jacobian[0].dot( _effectiveMasses[ 0 ] ) );
    v4f sor = simd::splat( _sor[ 0 ] );

    // Apply the pre-conditioner to the Jacobian
    applyPreconditionerToJacobian( _preJacobians, precond, _jacobian, sor );

    // Apply the pre-conditioner to the reaction
    v4f reactions = splat( _vars[0].reaction );
    reactions = ( sor * precond ) * reactions;
    storeSingle( &_vars[0].reaction, reactions );
}

//
// Dim 2 pre-conditioner
//

// Helper methods
template< int row, class SubMatrixSelect >
static RBX_SIMD_INLINE void multiplyRowByColumn( ConstraintJacobianPair* __restrict _r, const SymmetricMatrix2SIMD& _preconditioner, const ConstraintJacobianPair* __restrict _j, const v4f& _sor )
{
    v4f r = _preconditioner.get<row,0>() * _j[0].get< SubMatrixSelect >() 
          + _preconditioner.get<row,1>() * _j[1].get< SubMatrixSelect >();
    r = splat<row>( _sor ) * r;
    _r[row].set< SubMatrixSelect >( r );
}

template< class SubMatrixSelect >
static RBX_SIMD_INLINE void applyPreconditionerToSubMatrix( ConstraintJacobianPair* __restrict _r, const SymmetricMatrix2SIMD& _preconditioner, const ConstraintJacobianPair* __restrict _j, const v4f& _sor )
{
    multiplyRowByColumn< 0, SubMatrixSelect >(_r, _preconditioner, _j, _sor);
    multiplyRowByColumn< 1, SubMatrixSelect >(_r, _preconditioner, _j, _sor);
}

static RBX_SIMD_INLINE void applyPreconditionerToJacobian( ConstraintJacobianPair* __restrict _r, const SymmetricMatrix2SIMD& _preconditioner, const ConstraintJacobianPair* __restrict _j, const v4f& _sor )
{
    applyPreconditionerToSubMatrix< Jac::LinA >( _r, _preconditioner, _j, _sor );
    applyPreconditionerToSubMatrix< Jac::AngA >( _r, _preconditioner, _j, _sor );
    applyPreconditionerToSubMatrix< Jac::LinB >( _r, _preconditioner, _j, _sor );
    applyPreconditionerToSubMatrix< Jac::AngB >( _r, _preconditioner, _j, _sor );
}

static RBX_SIMD_INLINE SymmetricMatrix2SIMD constraintMatrixDiagBlockInverse2( const boost::uint8_t* _useBlock, const ConstraintJacobianPair* _jacobian, const EffectiveMassPair* _massVectors )
{
    v4f d01 = _jacobian[0].dot( _massVectors[1] );
    if( !_useBlock[0] )
    {
        d01 = zerof();
    }

    if( !_useBlock[1] )
    {
        d01 = zerof();
    }

    v4f d0 = _jacobian[0].dot( _massVectors[0] );
    v4f d1 = _jacobian[1].dot( _massVectors[1] );

    SymmetricMatrix2SIMD r( d0, d01, d1 );
    r.invert();

    return r;
}

// Dim 2 pre-conditioner
// For a constraint of dimension 2, the pre-conditioner is the inverse of the corresponding 2x2 diagonal block in the constraint matrix.
template< >
RBX_SIMD_INLINE void applyPreconditionerToStage< 2 >(
    ConstraintVariables* __restrict _vars,
    ConstraintJacobianPair* __restrict _preJacobians,
    const boost::uint8_t* _useBlock,
    const ConstraintJacobianPair* __restrict _jacobian,
    const float* __restrict _sor,
    const EffectiveMassPair* __restrict _effectiveMasses )
{
    // Compute the inverse of the 2x2 diagonal block of the constraint matrix, whose entries are all the dot product of the jacobians with the effective masses in all dimensions
    SymmetricMatrix2SIMD precond = constraintMatrixDiagBlockInverse2( _useBlock, _jacobian, _effectiveMasses );
    v4f sor = form( _sor[0], _sor[1] );

    // Apply the pre-conditioner to the Jacobian
    applyPreconditionerToJacobian( _preJacobians, precond, _jacobian, sor );

    // Apply the pre-conditioner to the reactions
    v4f reactions = form( _vars[0].reaction, _vars[1].reaction );
    reactions = sor * ( precond * reactions );
    storeSingle( &_vars[0].reaction, splat< 0 >( reactions ) );
    storeSingle( &_vars[1].reaction, splat< 1 >( reactions ) );
}

//
// Dim 3 pre-conditioner
//

// Helper methods
template< int row, class SubMatrixSelect >
static RBX_SIMD_INLINE void multiplyRowByColumn( ConstraintJacobianPair* __restrict _r, const SymmetricMatrixSIMD& _preconditioner, const ConstraintJacobianPair* __restrict _j, const v4f& _sor )
{
    v4f r = _preconditioner.get<row,0>() * _j[0].get< SubMatrixSelect >() 
          + _preconditioner.get<row,1>() * _j[1].get< SubMatrixSelect >() 
          + _preconditioner.get<row,2>() * _j[2].get< SubMatrixSelect >();
    r = splat<row>( _sor ) * r;
    _r[row].set< SubMatrixSelect >( r );
}

template< class SubMatrixSelect >
static RBX_SIMD_INLINE void applyPreconditionerToSubMatrix( ConstraintJacobianPair* __restrict _r, const SymmetricMatrixSIMD& _preconditioner, const ConstraintJacobianPair* __restrict _j, const v4f& _sor )
{
    multiplyRowByColumn< 0, SubMatrixSelect >(_r, _preconditioner, _j, _sor);
    multiplyRowByColumn< 1, SubMatrixSelect >(_r, _preconditioner, _j, _sor);
    multiplyRowByColumn< 2, SubMatrixSelect >(_r, _preconditioner, _j, _sor);
}

// Multiplying a symmetric 3x3 matrix (pre-conditioner) by a 3x12 matrix (Jacobian)
// We breakdown this matrix multiply into 4 multiplies of the 3x3 symmetric matrix with a 3x3 sub-matrix of the Jacobian
static RBX_SIMD_INLINE void applyPreconditionerToJacobian( ConstraintJacobianPair* __restrict _r, const SymmetricMatrixSIMD& _preconditioner, const ConstraintJacobianPair* __restrict _j, const v4f& _sor )
{
    applyPreconditionerToSubMatrix< Jac::LinA >( _r, _preconditioner, _j, _sor );
    applyPreconditionerToSubMatrix< Jac::AngA >( _r, _preconditioner, _j, _sor );
    applyPreconditionerToSubMatrix< Jac::LinB >( _r, _preconditioner, _j, _sor );
    applyPreconditionerToSubMatrix< Jac::AngB >( _r, _preconditioner, _j, _sor );
}

static RBX_SIMD_INLINE SymmetricMatrixSIMD constraintMatrixDiagBlockInverse3( const boost::uint8_t* _useBlock, const ConstraintJacobianPair* _jacobian, const EffectiveMassPair* _massVectors )
{
    v4f d01 = _jacobian[0].dot( _massVectors[1] );
    v4f d02 = _jacobian[0].dot( _massVectors[2] );
    v4f d12 = _jacobian[1].dot( _massVectors[2] );
    if( !_useBlock[0] )
    {
        d01 = zerof();
        d02 = zerof();
    }

    if( !_useBlock[1] )
    {
        d01 = zerof();
        d12 = zerof();
    }

    if( !_useBlock[2] )
    {
        d12 = zerof();
        d02 = zerof();
    }

    v4f offDiagonal = gatherX( d01, d02, d12 );

    v4f d0 = _jacobian[0].dot( _massVectors[0] );
    v4f d1 = _jacobian[1].dot( _massVectors[1] );
    v4f d2 = _jacobian[2].dot( _massVectors[2] );
    v4f diagonal = gatherX( d0, d1, d2 );

    SymmetricMatrixSIMD r( diagonal, offDiagonal );
    r.invert();

    return r;
}

// Dim 3 pre-conditioner
template< >
RBX_SIMD_INLINE void applyPreconditionerToStage< 3 >(
    ConstraintVariables* __restrict _vars,
    ConstraintJacobianPair* __restrict _preJacobians,
    const boost::uint8_t* _useBlock,
    const ConstraintJacobianPair* __restrict _jacobian,
    const float* __restrict _sor,
    const EffectiveMassPair* __restrict _effectiveMasses )
{
    // Compute the inverse of the diagonal block of the constraint matrix.
    SymmetricMatrixSIMD precond = constraintMatrixDiagBlockInverse3( _useBlock, _jacobian, _effectiveMasses );

    // Load sor into a useful form
    v4f sor = form( _sor[0], _sor[1], _sor[2] );

    // Multiply the inverse of the diagonal block with the Jacobian
    applyPreconditionerToJacobian( _preJacobians, precond, _jacobian, sor );

    // Grab the reaction and multiply with the block inverse
    v4f reactions = form( _vars[0].reaction, _vars[1].reaction, _vars[2].reaction );
    reactions = sor * ( precond * reactions );
    storeSingle( &_vars[0].reaction, splat< 0 >( reactions ) );
    storeSingle( &_vars[1].reaction, splat< 1 >( reactions ) );
    storeSingle( &_vars[2].reaction, splat< 2 >( reactions ) );
}

template< int DIM >
static void applyPreconditioner( 
    ConstraintVariables* __restrict _velocityStageVariables,
    ConstraintVariables* __restrict _positionStageVariables,
    ConstraintJacobianPair* __restrict _preconditionedJacobiansVelStage,
    ConstraintJacobianPair* __restrict _preconditionedJacobiansPosStage,
    const boost::uint8_t* _useBlock,
    const ConstraintJacobianPair* __restrict _jacobians,
    const float* __restrict _sorVel,
    const float* __restrict _sorPos,
    const EffectiveMassPair* __restrict _effectiveMassesVelStage,
    const EffectiveMassPair* __restrict _effectiveMassesPosStage,
    boost::uint32_t offset )
{
    const ConstraintJacobianPair* __restrict currentJacobian = _jacobians + offset;
    const EffectiveMassPair* __restrict currentEffectiveMassesVelStage = _effectiveMassesVelStage + offset;
    const EffectiveMassPair* __restrict currentEffectiveMassesPosStage = _effectiveMassesPosStage + offset;
    const boost::uint8_t* currentUseBlock = _useBlock + offset;
    const float* __restrict sorVel = _sorVel + offset;
    const float* __restrict sorPos = _sorPos + offset;
    ConstraintVariables* __restrict currentVelStage = _velocityStageVariables + offset;
    ConstraintVariables* __restrict currentPosStage = _positionStageVariables + offset;
    ConstraintJacobianPair* __restrict currentPreJacobianVelStage = _preconditionedJacobiansVelStage + offset;
    ConstraintJacobianPair* __restrict currentPreJacobianPosStage = _preconditionedJacobiansPosStage + offset;

    applyPreconditionerToStage< DIM >( currentVelStage, currentPreJacobianVelStage, currentUseBlock, currentJacobian, sorVel, currentEffectiveMassesVelStage );
    applyPreconditionerToStage< DIM >( currentPosStage, currentPreJacobianPosStage, currentUseBlock, currentJacobian, sorPos, currentEffectiveMassesPosStage );
}

//
// Uses a block pre-conditioner
// Computes P^-1 * J and P^-1 * R
//
void PGSPreconditionConstraintEquations( 
    ConstraintJacobianPair* __restrict _preconditionedJacobiansVelStage, 
    ConstraintJacobianPair* __restrict _preconditionedJacobiansPosStage, 
    ConstraintVariables* __restrict _velocityStageVariables, 
    ConstraintVariables* __restrict _positionStageVariables,
    size_t _constraintCount, 
    const boost::uint8_t* __restrict _dimensions, 
    const boost::uint8_t* __restrict _useBlock,
    const float* __restrict _sorVel,
    const float* __restrict _sorPos,
    const ConstraintJacobianPair* __restrict _jacobians, 
    const EffectiveMassPair* __restrict _effectiveMassesVelStage,
    const EffectiveMassPair* __restrict _effectiveMassesPosStage )
{
    RBXPROFILER_SCOPE("Physics", "PGSPreconditionConstraintEquations");

    boost::uint32_t offset = 0;

    static const int maxConstraintDimension = 3;
    (void)maxConstraintDimension;
    for( size_t c = 0; c < _constraintCount; c++ )
    {
        auto d = _dimensions[ c ];
        RBXASSERT_VERY_FAST( d <= maxConstraintDimension );

        switch ( d )
        {
        case 1:
            applyPreconditioner<1>(_velocityStageVariables, _positionStageVariables, _preconditionedJacobiansVelStage, _preconditionedJacobiansPosStage, _useBlock, _jacobians, _sorVel, _sorPos, _effectiveMassesVelStage, _effectiveMassesPosStage, offset);
            break;
        case 2:
            applyPreconditioner<2>(_velocityStageVariables, _positionStageVariables, _preconditionedJacobiansVelStage, _preconditionedJacobiansPosStage, _useBlock, _jacobians, _sorVel, _sorPos, _effectiveMassesVelStage, _effectiveMassesPosStage, offset);
            break;
        case 3:
            applyPreconditioner<3>(_velocityStageVariables, _positionStageVariables, _preconditionedJacobiansVelStage, _preconditionedJacobiansPosStage, _useBlock, _jacobians, _sorVel, _sorPos, _effectiveMassesVelStage, _effectiveMassesPosStage, offset);
            break;
        default:
            break;
        }

        offset += d;
    }
}

//-----------------------------------------------------------------------------------------------------------
//
// Effective Mass multipliers values are 0.0f or 1.0f depending on weather the object is simulated or not.
// Apply these coefficients to effective masses.
// This is done after the pre-conditioner have been computed, right before the Kernel runs 
// (and the virtual displacements are initialized).
// So an object interacts with a sleeping object as if that sleeping object had a finite mass, 
// but the sleeping objects doesn't react to this interaction (as if it had an infinite mass).
// So to sum up:
// - pre-conditioner is computed with real masses whether the object is asleep or not
// - the kernel is run with collapsed masses, where object asleep are considered infinite mass
//
//-----------------------------------------------------------------------------------------------------------
void PGSApplyEffectiveMassMultipliers( 
    EffectiveMassPair* __restrict _effectiveMassesVelStage,
    EffectiveMassPair* __restrict _effectiveMassesPosStage,
    size_t _constraintCount, 
    const boost::uint8_t* _dimensions, 
    const float* _multipliers, 
    const BodyPairIndices* _pairs,
    const SolverConfig& _config )
{
    RBXPROFILER_SCOPE("Physics", "PGSApplyEffectiveMassMultipliers");

    boost::uint32_t offset = 0;
    for( unsigned c = 0; c < _constraintCount; c++ )
    {
        auto d = _dimensions[ c ];

        auto* __restrict effMasVel = _effectiveMassesVelStage + offset;
        auto* __restrict effMasPos = _effectiveMassesPosStage + offset;
        int a = _pairs[c].first;
        int b = _pairs[c].second;
        for( boost::uint8_t i = 0; i < d; i++ )
        {
            effMasVel[ i ].applyMultipliers( splat( _multipliers[ a ] ), splat( _multipliers[ b ] ) );
            effMasPos[ i ].applyMultipliers( splat( _multipliers[ a ] ), splat( _multipliers[ b ] ) );
        }

        offset += d;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
//
// Initialize the virtual displacements
//
// Initialize the virtual displacements using cached impulses.
// Virtual displacements are an optimization to the Kernel.
// They can always be recomputed from the impulses, but the kernel can make its computations faster if they are kept up to date.
// Virtual displacements are given by the effective masses times the impulses: Eff * I, where Eff = W * J^t.
//
//----------------------------------------------------------------------------------------------------------------------------------

// Helper method
// Apply an impulse to a virtual displacement:
// for velocity stage it is adding the change in velocities due to the constraint impulse
// for position stage it is adding the change in positions due to the constraint correction impulse
static RBX_SIMD_INLINE VirtualDisplacement applyImpulse( const VirtualDisplacement& virD, const simd::v4f& impulse, const EffectiveMass& eff )
{
    return VirtualDisplacement( simd::mulAdd( virD.getLin(), impulse, eff.getLin() ), simd::mulAdd( virD.getAng(), impulse, eff.getAng() ) );
}

// Main method
void PGSInitVirtualDisplacements( 
    VirtualDisplacementArray& _virDVel, 
    VirtualDisplacementArray& _virDPos, 
    const EffectiveMassPair* __restrict _effectiveMassesVelStage,
    const EffectiveMassPair* __restrict _effectiveMassesPosStage,
    size_t _constraintCount, 
    const boost::uint8_t* _dimensions, 
    const ConstraintVariables* __restrict _velStage,
    const ConstraintVariables* __restrict _posStage,
    const BodyPairIndices* _pairs,
    const SolverConfig& _config )
{
    RBXPROFILER_SCOPE("Physics", "PGSInitVirtualDisplacements");

    boost::uint32_t offset = 0;
    for( unsigned c = 0; c < _constraintCount; c++ )
    {
        auto d = _dimensions[ c ];

        // Get the address of the variables containing the computed impulses
        int bodyA = _pairs[c].first;
        int bodyB = _pairs[c].second;

        // Velocity stage
        {
            const ConstraintVariables* velVars = _velStage + offset;
            const EffectiveMassPair* effMasVel = _effectiveMassesVelStage + offset;

            // Load the currently accumulated virtual displacements for the pair of objects on this constraint
            VirtualDisplacement virDA( _virDVel[bodyA] );
            VirtualDisplacement virDB( _virDVel[bodyB] );

            // Add the contribution due to this constraint impulses to the pair of virtual displacements
            for( boost::uint8_t i = 0; i < d; i++ )
            {
                v4f impulse = splat( velVars[ i ].impulse );
                virDA = applyImpulse( virDA, impulse, effMasVel[ i ].getPartA() );
                virDB = applyImpulse( virDB, impulse, effMasVel[ i ].getPartB() );
            }

            // Store the results
            _virDVel[ bodyA ] = virDA;
            _virDVel[ bodyB ] = virDB;
        }

        // Position stage
        {
            const ConstraintVariables* posVars = _posStage + offset;
            const EffectiveMassPair* effMasPos = _effectiveMassesPosStage + offset;

            // Load the currently accumulated virtual displacements for the pair of objects on this constraint
            VirtualDisplacement virDA( _virDPos[bodyA] );
            VirtualDisplacement virDB( _virDPos[bodyB] );

            // Add the contribution due to this constraint impulses to the pair of virtual displacements
            for( boost::uint8_t i = 0; i < d; i++ )
            {
                v4f impulse = splat( posVars[ i ].impulse );
                virDA = applyImpulse( virDA, impulse, effMasPos[ i ].getPartA() );
                virDB = applyImpulse( virDB, impulse, effMasPos[ i ].getPartB() );
            }

            // Store the results
            _virDPos[ bodyA ] = virDA;
            _virDPos[ bodyB ] = virDB;
        }

        offset += d;
    }
}

//----------------------------------------------------------------------------------------------------------------------------------
//
// Kernel: this code needs to be as optimal as possible as it is the bottleneck of the solver (60% of time is spent in the main kernel loop).
//
// Implementations of the single kernel update for each constraint.
// These implementations are optimized for 1, 2 and 3 dimensional constraints.
// Each constraint update will update all the internal impulses for this constraints
// and will update the virtual displacements of the two bodies involved in this constraint.
// Keeping the virtual displacements updated is an important optimization, 
// but in theory they can be recomputed from the impulses (see PGSInitVirtualDisplacements).
//
//----------------------------------------------------------------------------------------------------------------------------------

// Helper method
// This is a partial dot product: the full dot product is the sum of components of the result of 'partiallyProjectOntoJacobian'
static RBX_SIMD_INLINE v4f partiallyProjectOntoJacobian( const ConstraintJacobianPair& _j, const VirtualDisplacement& _va, const VirtualDisplacement& _vb )
{
    simd::v4f partA = _j.getLinA() * _va.getLin() + _j.getAngA() * _va.getAng();
    simd::v4f partB = _j.getLinB() * _vb.getLin() + _j.getAngB() * _vb.getAng();
    return partA + partB;
}

// Helper method
// Kernel update constraint dimension 1
static void updateConstraintDim1( 
    ConstraintVariables* __restrict _velStage,
    ConstraintVariables* __restrict _posStage,
    VirtualDisplacementPOD* __restrict _virDVel,
    VirtualDisplacementPOD* __restrict _virDPos,
    const ConstraintJacobianPair* __restrict _preconditionedJacobiansVel,
    const ConstraintJacobianPair* __restrict _preconditionedJacobiansPos,
    const EffectiveMassPair* __restrict _effectiveMassesVel,
    const EffectiveMassPair* __restrict _effectiveMassesPos,
    const BodyPairIndices& _pair )
{
    // Project the virtual displacements onto the preconditioned Jacobians
    // This is equivalent to 'multiplying a row of the constraint matrix with the impulse vector and dividing by the diagonal element of the row'.
    // Physically the result of this is the relative change in velocity due to currently computed constraint impulses.
    // We do it for both velocity stage and position stage.
    // We store the results as: projectedVirD = ( projection vel stage, projection position stage, *, * )
    v4f t0 = partiallyProjectOntoJacobian( _preconditionedJacobiansVel[ 0 ], _virDVel[ _pair.first ], _virDVel[ _pair.second ] );
    v4f t1 = partiallyProjectOntoJacobian( _preconditionedJacobiansPos[ 0 ], _virDPos[ _pair.first ], _virDPos[ _pair.second ] );
    v4f projectedVirD = sumAcross3( t0, t1 );

    // Load the variables and gather the individual components in separate vectors
    // oldImpulses = (_velStage[0].impulse, _posStage[0].impulse, *, * }
    // reactions = (_velStage[0].reaction, _posStage[0].reaction, *, * }
    // ...
    // This is done purely so that we can use the simd vectors to parallel process.
    v4f oldImpulses, preconditionedReactions, minImpulses, maxImpulses;
    ConstraintVariables::gatherComponents(oldImpulses, preconditionedReactions, minImpulses, maxImpulses, _velStage[0], _posStage[0] );

    // Compute and clamp new impulses
    // The new impulse is the correction of the old impulse by the difference of the desired change in relative velocities with current change in relative velocities due to constraint impulses,
    // multiplied by the pre-conditioner (inverse of the diagonal element)
    v4f newImpulses = oldImpulses + ( preconditionedReactions - projectedVirD );
    newImpulses = max( newImpulses, minImpulses );
    newImpulses = min( newImpulses, maxImpulses );

    // Compute the change in impulses
    v4f dImpulses = newImpulses - oldImpulses;

    // Store new impulses
    storeSingle( &_velStage[0].impulse, splat<0>( newImpulses ) );
    storeSingle( &_posStage[0].impulse, splat<1>( newImpulses ) );

    // Update virtual displacements due to change in impulses
    // Change in virtual displacement is the change in impulses for the constraint times the effective mass of the constraint
    _virDVel[ _pair.first ] = applyImpulse( _virDVel[ _pair.first ], splat<0>(dImpulses), _effectiveMassesVel[0].getPartA() );
    _virDVel[ _pair.second ] = applyImpulse( _virDVel[ _pair.second ], splat<0>(dImpulses), _effectiveMassesVel[0].getPartB() );
    _virDPos[ _pair.first ] = applyImpulse( _virDPos[ _pair.first ], splat<1>(dImpulses), _effectiveMassesPos[0].getPartA() );
    _virDPos[ _pair.second ] = applyImpulse( _virDPos[ _pair.second ], splat<1>(dImpulses), _effectiveMassesPos[0].getPartB() );
}

// Helper method
// Kernel update constraint dimension 2
static void updateConstraintDim2( 
    ConstraintVariables* __restrict _velStage,
    ConstraintVariables* __restrict _posStage,
    VirtualDisplacementPOD* __restrict _virDVel,
    VirtualDisplacementPOD* __restrict _virDPos,
    const ConstraintJacobianPair* __restrict _preconditionedJacobiansVel,
    const ConstraintJacobianPair* __restrict _preconditionedJacobiansPos,
    const EffectiveMassPair* __restrict _effectiveMassesVel,
    const EffectiveMassPair* __restrict _effectiveMassesPos,
    const BodyPairIndices& _pair )
{
    // Project the virtual displacements onto the preconditioned Jacobians
    // We first 'partially project' and then complete the projection with the 'sumAcross'.
    // This is an optimization: originally this was 2 full projects and a gather.
    // This is equivalent to 'multiplying a row of the constraint matrix with the impulse vector and dividing by the diagonal element of the row'.
    // But because we do block PGS, this is 'multiplying a group of row (2 of them int is case) of the constraint matrix with the impulse vector and multiplying by the inverse of the diagonal block'.
    // Physically the result of this is the relative change in velocity due to currently computed constraint impulses.
    // We store the result as: projectedVirD = ( projection vel stage dim 0, projection pos stage dim 0, projection vel stage dim 1, projection pos stage dim 1 )
    v4f t0 = partiallyProjectOntoJacobian( _preconditionedJacobiansVel[ 0 ], _virDVel[ _pair.first ], _virDVel[ _pair.second ] );
    v4f t2 = partiallyProjectOntoJacobian( _preconditionedJacobiansVel[ 1 ], _virDVel[ _pair.first ], _virDVel[ _pair.second ] );
    v4f t02 = sumAcross3( t0, t2 ); // ( projection vel stage dim 0, projection vel stage dim 1, *, * )

    v4f t1 = partiallyProjectOntoJacobian( _preconditionedJacobiansPos[ 0 ], _virDPos[ _pair.first ], _virDPos[ _pair.second ] );
    v4f t3 = partiallyProjectOntoJacobian( _preconditionedJacobiansPos[ 1 ], _virDPos[ _pair.first ], _virDPos[ _pair.second ] );
    v4f t13 = sumAcross3( t1, t3 ); // ( projection pos stage dim 0, projection pos stage dim 1, *, * )

    v4f projectedVirD = zipLow( t02, t13 );

    // Load the variables and gather the individual components in separate vectors
    // oldImpulses = (_velStage[0].impulse, _posStage[0].impulse, _velStage[1].impulse, _posStage[1].impulse }
    // reactions = (_velStage[0].reaction, _posStage[0].reaction, _velStage[1].reaction, _posStage[1].reaction }
    // ...
    v4f oldImpulses, preconditionedReactions, minImpulses, maxImpulses;
    ConstraintVariables::gatherComponents(oldImpulses, preconditionedReactions, minImpulses, maxImpulses, _velStage[0], _posStage[0], _velStage[1], _posStage[1] );

    // Compute and clamp new impulses
    // The new impulse is the correction of the old impulse by the difference of the desired change in relative velocities with current change in relative velocities due to constraint impulses,
    // multiplied by the pre-conditioner (inverse of the diagonal block)
    v4f newImpulses = oldImpulses + ( preconditionedReactions - projectedVirD );
    newImpulses = max( newImpulses, minImpulses );
    newImpulses = min( newImpulses, maxImpulses );

    // Compute the change in impulses
    v4f dImpulses = newImpulses - oldImpulses;

    // Store new impulses
    storeSingle( &_velStage[0].impulse, splat<0>( newImpulses ) );
    storeSingle( &_posStage[0].impulse, splat<1>( newImpulses ) );
    storeSingle( &_velStage[1].impulse, splat<2>( newImpulses ) );
    storeSingle( &_posStage[1].impulse, splat<3>( newImpulses ) );

    // Update virtual displacements due to change in impulses
    // Change in virtual displacement is the change in impulses for the constraint times the effective mass of the constraint
    {
        VirtualDisplacement virD = _virDVel[ _pair.first ];
        virD = applyImpulse( virD, splat<0>(dImpulses), _effectiveMassesVel[0].getPartA() );
        virD = applyImpulse( virD, splat<2>(dImpulses), _effectiveMassesVel[1].getPartA() );
        _virDVel[ _pair.first ] = virD;
    }

    {
        VirtualDisplacement virD = _virDVel[ _pair.second ];
        virD = applyImpulse( virD, splat<0>(dImpulses), _effectiveMassesVel[0].getPartB() );
        virD = applyImpulse( virD, splat<2>(dImpulses), _effectiveMassesVel[1].getPartB() );
        _virDVel[ _pair.second ] = virD;
    }

    {
        VirtualDisplacement virD = _virDPos[ _pair.first ];
        virD = applyImpulse( virD, splat<1>(dImpulses), _effectiveMassesPos[0].getPartA() );
        virD = applyImpulse( virD, splat<3>(dImpulses), _effectiveMassesPos[1].getPartA() );
        _virDPos[ _pair.first ] = virD;
    }

    {
        VirtualDisplacement virD = _virDPos[ _pair.second ];
        virD = applyImpulse( virD, splat<1>(dImpulses), _effectiveMassesPos[0].getPartB() );
        virD = applyImpulse( virD, splat<3>(dImpulses), _effectiveMassesPos[1].getPartB() );
        _virDPos[ _pair.second ] = virD;
    }
}

// Kernel update constraint dimension 3
class CollisionType;
class ConstraintType;

// Helper method
// We reuse the code for constraints of dim 3 and collisions, but there is a small difference:
// the max/min impulses are computed differently for constraints vs collisions.
// For performance reasons - to avoid branches - using compile time polymorphism
// Default implementation does nothing (constraint type)
template<class Type >
static RBX_SIMD_INLINE void modifyImpulseBounds( v4f& minBound, v4f& maxBound, const v4f& oldImpulses )
{ }

// For collisions we need to do something different to simulate the friction cone
template<>
RBX_SIMD_INLINE void modifyImpulseBounds< CollisionType >( v4f& minBound, v4f& maxBound, const v4f& oldImpulses )
{
    v4f normalImpulse = splat< 0 >( oldImpulses );
    // For collisions, component 1 and 2 contains friction constant. To get bounds on friction impulses we multiply by the current normal impulse.
    minBound = replace< 0 >( minBound * normalImpulse, minBound ); // ( 0, Fs, Fs ) or ( 0, 0, 0 )
    maxBound = replace< 0 >( maxBound * normalImpulse, maxBound ); // ( +inf, Fs, Fs ) or ( +inf, Fd, 0 )
}

// Common implementation for a 3-dimensional constraint or collision
// Type is either CollisionType or ConstraintType
template< class Type >
static RBX_SIMD_INLINE void updateConstraintDim3OrCollisionSingleStage( 
    ConstraintVariables* __restrict _vars,
    VirtualDisplacementPOD* __restrict _virD,
    const ConstraintJacobianPair* __restrict _preconditionedJacobians,
    const EffectiveMassPair* __restrict _effectiveMasses,
    const BodyPairIndices& _pair )
{
    // Project the virtual displacements onto the preconditioned Jacobians.
    // We first 'partially project' and then complete the projection with the 'sumAcross'.
    // This is an optimization: originally this was 3 full projects and a gather.
    // This is equivalent to 'multiplying a row of the constraint matrix with the impulse vector and dividing by the diagonal element of the row'.
    // But because we do block PGS, this is 'multiplying a group of row (3 of them int is case) of the constraint matrix with the impulse vector and multiplying by the inverse of the diagonal block'.
    // Physically the result of this is the relative change in velocity due to currently computed constraint impulses.
    // We store the result as: projectedVirD = ( projection vel stage dim 0, projection vel stage dim 1, projection vel stage dim 2, * )
    v4f a = partiallyProjectOntoJacobian( _preconditionedJacobians[ 0 ], _virD[ _pair.first ], _virD[ _pair.second ] );
    v4f b = partiallyProjectOntoJacobian( _preconditionedJacobians[ 1 ], _virD[ _pair.first ], _virD[ _pair.second ] );
    v4f c = partiallyProjectOntoJacobian( _preconditionedJacobians[ 2 ], _virD[ _pair.first ], _virD[ _pair.second ] );
    v4f projectedVirD = sumAcross3( a, b, c );

    // Load the variables and gather the individual components in separate vectors
    // oldImpulses = (_vars[0].impulse, _vars[1].impulse, _vars[2].impulse, * )
    // reactions = (_vars[0].reaction, _vars[1].reaction, _vars[2].reaction, * )
    // ...
    v4f oldImpulses, preconditionedReactions, minImpulses, maxImpulses;
    ConstraintVariables::gatherComponents( oldImpulses, preconditionedReactions, minImpulses, maxImpulses, _vars[0], _vars[1], _vars[2] );

    // Compute the new impulses
    // The new impulse is the correction of the old impulse by the difference of the desired change in relative velocities with current change in relative velocities due to constraint impulses,
    // multiplied by the pre-conditioner (inverse of the diagonal block)
    v4f newImpulses = oldImpulses + ( preconditionedReactions - projectedVirD );

    // In case of collision, min/max impulses for friction need to be computed
    modifyImpulseBounds< Type >( minImpulses, maxImpulses, oldImpulses );

    // Clamp to bounds
    newImpulses = max( newImpulses, minImpulses );
    newImpulses = min( newImpulses, maxImpulses );

    // Compute the change in impulses
    v4f dImpulses = newImpulses - oldImpulses;

    // Store the results
    storeSingle( &_vars[ 0 ].impulse, splat< 0 >( newImpulses ) );
    storeSingle( &_vars[ 1 ].impulse, splat< 1 >( newImpulses ) );
    storeSingle( &_vars[ 2 ].impulse, splat< 2 >( newImpulses ) );

    // Modify the virtual displacements by the change in impulses
    // Change in virtual displacement is the change in impulses for the constraint times the effective mass of the constraint
    {
        VirtualDisplacement virD = _virD[ _pair.first ];
        virD = applyImpulse( virD, splat<0>(dImpulses), _effectiveMasses[0].getPartA() );
        virD = applyImpulse( virD, splat<1>(dImpulses), _effectiveMasses[1].getPartA() );
        virD = applyImpulse( virD, splat<2>(dImpulses), _effectiveMasses[2].getPartA() );
        _virD[ _pair.first ] = virD;
    }

    {
        VirtualDisplacement virD = _virD[ _pair.second ];
        virD = applyImpulse( virD, splat<0>(dImpulses), _effectiveMasses[0].getPartB() );
        virD = applyImpulse( virD, splat<1>(dImpulses), _effectiveMasses[1].getPartB() );
        virD = applyImpulse( virD, splat<2>(dImpulses), _effectiveMasses[2].getPartB() );
        _virD[ _pair.second ] = virD;
    }
}

// Template argument can be: CollisionType or ConstraintType
template< class Type >
static void updateConstraintDim3OrCollision( 
    ConstraintVariables* __restrict _velStage,
    ConstraintVariables* __restrict _posStage,
    VirtualDisplacementPOD* __restrict _virDVel,
    VirtualDisplacementPOD* __restrict _virDPos,
    const ConstraintJacobianPair* __restrict _preconditionedJacobiansVel,
    const ConstraintJacobianPair* __restrict _preconditionedJacobiansPos,
    const EffectiveMassPair* __restrict _effectiveMassesVel,
    const EffectiveMassPair* __restrict _effectiveMassesPos,
    const BodyPairIndices& _pair )
{
    updateConstraintDim3OrCollisionSingleStage< Type >(_velStage, _virDVel, _preconditionedJacobiansVel, _effectiveMassesVel, _pair);
    updateConstraintDim3OrCollisionSingleStage< Type >(_posStage, _virDPos, _preconditionedJacobiansPos, _effectiveMassesPos, _pair);
}

//
// Kernel main loop
//
void PGSSolveKernel(
    ConstraintVariables* __restrict _velStage,
    ConstraintVariables* __restrict _posStage,
    VirtualDisplacementArray& _virDVel,
    VirtualDisplacementArray& _virDPos,
    size_t _constraintCount, 
    size_t _collisionCount,
    const boost::uint8_t* _dimensions, 
    const BodyPairIndices* _pairs,
    const ConstraintJacobianPair* _preconditionedJacobiansVelStage, 
    const ConstraintJacobianPair* _preconditionedJacobiansPosStage, 
    const EffectiveMassPair* _effectiveMassesVelStage,
    const EffectiveMassPair* _effectiveMassesPosStage,
    const SolverConfig& _config )
{
    RBXPROFILER_SCOPE("Physics", "PGSSolveKernel");

    size_t pureConstraintCount = _constraintCount - _collisionCount;

    for( boost::uint32_t k = 0; k < _config.pgsIterations; k++ )
    {
        boost::uint32_t offset = 0;

        // Solving constraints
        for( size_t c = 0; c < pureConstraintCount; c++ )
        {
            const ConstraintJacobianPair* precondJacobianVel = _preconditionedJacobiansVelStage + offset;
            const ConstraintJacobianPair* precondJacobianPos = _preconditionedJacobiansPosStage + offset;
            const EffectiveMassPair* effMassesVel = _effectiveMassesVelStage + offset;
            const EffectiveMassPair* effMassesPos = _effectiveMassesPosStage + offset;
            ConstraintVariables* velStage = _velStage + offset;
            ConstraintVariables* posStage = _posStage + offset;

            auto dimC = _dimensions[ c ];

            // All these cases do the same thing, but have been optimized to reuse some data per constraint
            // It is equivalent to:
            // for( i = 0; i < dimC; i++ )
            //   PGSComputeConstraint1( velStage+i, posStage+i, _virDVel.data.data(), _virDPos.data.data(), precondJacobianVel+i, precondJacobianPos+i, effMassesVel+i, effMassesPos+i, _pairs[c] );
            switch ( dimC )
            {
            case 1:
                updateConstraintDim1( velStage, posStage, _virDVel.getData(), _virDPos.getData(), precondJacobianVel, precondJacobianPos, effMassesVel, effMassesPos, _pairs[c] );
                break;
            case 2:
                updateConstraintDim2( velStage, posStage, _virDVel.getData(), _virDPos.getData(), precondJacobianVel, precondJacobianPos, effMassesVel, effMassesPos, _pairs[c] );
                break;
            case 3:
                updateConstraintDim3OrCollision< ConstraintType >( velStage, posStage, _virDVel.getData(), _virDPos.getData(), precondJacobianVel, precondJacobianPos, effMassesVel, effMassesPos, _pairs[c] );
                break;
            default:
                break;
            }

            offset += dimC;
        }

        // Solving collisions
        for( size_t c = pureConstraintCount; c < _constraintCount; c++ )
        {
            const ConstraintJacobianPair* precondJacobianVel = _preconditionedJacobiansVelStage + offset;
            const ConstraintJacobianPair* precondJacobianPos = _preconditionedJacobiansPosStage + offset;
            const EffectiveMassPair* effMassesVel = _effectiveMassesVelStage + offset;
            const EffectiveMassPair* effMassesPos = _effectiveMassesPosStage + offset;
            ConstraintVariables* velStage = _velStage + offset;
            ConstraintVariables* posStage = _posStage + offset;

            updateConstraintDim3OrCollision< CollisionType >(velStage, posStage, _virDVel.getData(), _virDPos.getData(), precondJacobianVel, precondJacobianPos, effMassesVel, effMassesPos, _pairs[c] );

            offset += 3;
        }
    }
}

//
// Single stage inconsistency detector
//

// Store deltas
class StoreDeltaImpulses
{
public:
    static SIMD_FORCE_INLINE void store1( float* _dst, const v4f& _delta )
    {
        storeSingle( _dst, splat<0>(_delta) );
    }

    static SIMD_FORCE_INLINE void store1( float* _dst, const v4f& _delta, float _smoothing )
    {
        v4f s = splat(_smoothing);
        storeSingle( _dst, ( splat(1.0f) - s ) * splat<0>(_delta) + s * loadSingle( _dst ) );
    }

    static SIMD_FORCE_INLINE void storeSecondDerivative( float* _dst, const float* _oldDelta, const v4f& _delta )
    {
        storeSingle( _dst, splat<0>( _delta -  loadSingle(_oldDelta) ) );
    }

    static SIMD_FORCE_INLINE void storeSecondDerivative( float* _dst, const float* _oldDelta, const v4f& _delta, float _smoothing )
    {
        v4f s = splat(_smoothing);
        storeSingle( _dst, ( splat(1.0f) - s ) * splat<0>( _delta -  loadSingle(_oldDelta) ) + s * loadSingle( _dst ) );
    }
};

class DontStoreDeltaImpulses
{
public:
    static SIMD_FORCE_INLINE void store1( float* _dst, const v4f& _delta ) { }
    static SIMD_FORCE_INLINE void store1( float* _dst, const v4f& _delta, float _smoothing ) { }
    static SIMD_FORCE_INLINE void storeSecondDerivative( float* _dst, const float* _oldDelta, const v4f& _delta ) { }
    static SIMD_FORCE_INLINE void storeSecondDerivative( float* _dst, const float* _oldDelta, const v4f& _delta, float _smoothing ) { }
};

// Kernel update constraint dimension 1
template< class StoreDeltasSelector >
static SIMD_FORCE_INLINE void updateConstraintSingleStageDim1( 
    ConstraintVariables* __restrict _vars,
    float* __restrict _solutionDeltas,
    float* __restrict _solutionDeltaDeltas,
    VirtualDisplacementPOD* __restrict _virD,
    const ConstraintJacobianPair* __restrict _preconditionedJacobians,
    const EffectiveMassPair* __restrict _effectiveMasses,
    const BodyPairIndices& _pair,
    float _smoothingD,
    float _smoothingDD )
{
    // Project the virtual displacements onto the preconditioned Jacobians
    // This is equivalent to 'multiplying a row of the constraint matrix with the impulse vector and dividing by the diagonal element of the row'.
    // Physically the result of this is the relative change in velocity due to currently computed constraint impulses.
    // We do it for both velocity stage and position stage.
    // We store the results as: projectedVirD = ( projection vel stage, projection position stage, *, * )
    v4f t0 = partiallyProjectOntoJacobian( _preconditionedJacobians[ 0 ], _virD[ _pair.first ], _virD[ _pair.second ] );
    v4f projectedVirD = sumAcross3( t0, zerof() );

    // Load the variables and gather the individual components in separate vectors
    // oldImpulses = (_velStage[0].impulse, _posStage[0].impulse, *, * }
    // reactions = (_velStage[0].reaction, _posStage[0].reaction, *, * }
    // ...
    // This is done purely so that we can use the simd vectors to parallel process.
    v4f oldImpulses, preconditionedReactions, minImpulses, maxImpulses;
    ConstraintVariables::gatherComponents( oldImpulses, preconditionedReactions, minImpulses, maxImpulses, _vars[0] );

    // Compute and clamp new impulses
    // The new impulse is the correction of the old impulse by the difference of the desired change in relative velocities with current change in relative velocities due to constraint impulses,
    // multiplied by the pre-conditioner (inverse of the diagonal element)
    v4f newImpulses = oldImpulses + ( preconditionedReactions - projectedVirD );
    newImpulses = max( newImpulses, minImpulses );
    newImpulses = min( newImpulses, maxImpulses );

    // Compute the change in impulses
    v4f dImpulses = newImpulses - oldImpulses;
    StoreDeltasSelector::storeSecondDerivative( _solutionDeltaDeltas, _solutionDeltas, dImpulses, _smoothingDD );
    StoreDeltasSelector::store1( _solutionDeltas, dImpulses, _smoothingD );


    // Store new impulses
    storeSingle( &_vars[0].impulse, splat<0>( newImpulses ) );

    // Update virtual displacements due to change in impulses
    // Change in virtual displacement is the change in impulses for the constraint times the effective mass of the constraint
    _virD[ _pair.first ] = applyImpulse( _virD[ _pair.first ], splat<0>(dImpulses), _effectiveMasses[0].getPartA() );
    _virD[ _pair.second ] = applyImpulse( _virD[ _pair.second ], splat<0>(dImpulses), _effectiveMasses[0].getPartB() );
}

template< class StoreDeltasSelector >
static void singleStageNoFrictionKernelInternal(
    ConstraintVariables* __restrict _vars,
    VirtualDisplacementArray& _virD,
    float* __restrict _solutionDeltas,
    float* __restrict _solutionDeltaDeltas,
    size_t _constraintCount, 
    size_t _collisionCount,
    const boost::uint8_t* _dimensions, 
    const BodyPairIndices* _pairs,
    const ConstraintJacobianPair* _preconditionedJacobians, 
    const EffectiveMassPair* _effectiveMasses,
    const SolverConfig& _config,
    float _smoothingD,
    float _smoothingDD )
{
    size_t pureConstraintCount = _constraintCount - _collisionCount;

    for( boost::uint32_t k = 0; k < _config.pgsIterations; k++ )
    {
        boost::uint32_t offset = 0;

        // Solving constraints
        for( size_t c = 0; c < pureConstraintCount; c++ )
        {
            const ConstraintJacobianPair* precondJacobian = _preconditionedJacobians + offset;
            const EffectiveMassPair* effMasses = _effectiveMasses + offset;
            ConstraintVariables* vars = _vars + offset;
            float* solutionDeltas = _solutionDeltas + offset;
            float* solutionDeltaDeltas = _solutionDeltaDeltas + offset;
            auto dimC = _dimensions[ c ];

            for( size_t i = 0; i < dimC; i++ )
            {
                updateConstraintSingleStageDim1< StoreDeltasSelector >( vars+i, solutionDeltas+i, solutionDeltaDeltas+i, _virD.getData(), precondJacobian+i, effMasses+i, _pairs[c], _smoothingD, _smoothingDD );
            }

            offset += dimC;
        }

        // Solving collisions
        for( size_t c = pureConstraintCount; c < _constraintCount; c++ )
        {
            const ConstraintJacobianPair* precondJacobian = _preconditionedJacobians + offset;
            const EffectiveMassPair* effMasses = _effectiveMasses + offset;
            ConstraintVariables* vars = _vars + offset;
            float* solutionDeltas = _solutionDeltas + offset;
            float* solutionDeltaDeltas = _solutionDeltaDeltas + offset;

            updateConstraintSingleStageDim1< StoreDeltasSelector >( vars, solutionDeltas, solutionDeltaDeltas, _virD.getData(), precondJacobian, effMasses, _pairs[c], _smoothingD, _smoothingDD );

            StoreDeltasSelector::store1( solutionDeltas+1, zerof() );
            StoreDeltasSelector::store1( solutionDeltas+2, zerof() );
            StoreDeltasSelector::store1( solutionDeltaDeltas+1, zerof() );
            StoreDeltasSelector::store1( solutionDeltaDeltas+2, zerof() );

            offset += 3;
        }
    }
}

//
// Error computation
//
void PGSSolveKernelComputeErrors(
    ArrayBase< float >& _impulseDeltas,
    ArrayBase< float >& _impulseDeltaDeltas,
    ArrayBase< ConstraintVariables >& _vars,
    VirtualDisplacementArray& _virD,
    size_t _constraintCount, 
    size_t _collisionCount,
    size_t _bodyCount,
    const boost::uint8_t* _dimensions, 
    const BodyPairIndices* _pairs,
    const ConstraintJacobianPair* _jacobians, 
    const ConstraintJacobianPair* _preconditionedJacobians, 
    const EffectiveMassPair* _effectiveMasses,
    const SolverConfig& _config )
{
    RBXPROFILER_SCOPE("Physics", "PGSSolveKernelComputeErrors");

    SolverConfig localConfig = _config;
    static float smoothingD = 0.95f;
    static float smoothingDD = 0.0f;

    singleStageNoFrictionKernelInternal< StoreDeltaImpulses >(
        _vars.data(), _virD, _impulseDeltas.data(), _impulseDeltaDeltas.data(),
        _constraintCount, _collisionCount, _dimensions, _pairs, _preconditionedJacobians, _effectiveMasses, localConfig, smoothingD, smoothingDD);
}

}
