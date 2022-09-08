#pragma once

#include "solver/SolverConfig.h"
#include "boost/cstdint.hpp"
#include "rbx/ArrayDynamic.h"

namespace RBX
{

class ConstraintJacobianPair;
class BodyPairIndices;
class SolverBodyStaticProperties;
class SolverBodyMassAndInertia;
class ConstraintVariables;
class VirtualDisplacementArray;
class VirtualDisplacementArray;
class EffectiveMassPair;

void PGSComputeEffectiveMasses( 
    EffectiveMassPair* _effectiveMassesVelStage,
    EffectiveMassPair* _effectiveMassesPosStage,
    size_t _constraintCount, 
    const boost::uint8_t* _dimensions, 
    const ConstraintJacobianPair* _jacobians, 
    const BodyPairIndices* _pairs,
    const SolverBodyMassAndInertia* _massAndIntertia,
    const SolverConfig& _config );

void PGSApplyEffectiveMassMultipliers( 
    EffectiveMassPair* _effectiveMassesVelStage,
    EffectiveMassPair* _effectiveMassesPosStage,
    size_t _constraintCount, 
    const boost::uint8_t* _dimensions, 
    const float* _multipliers, 
    const BodyPairIndices* _pairs,
    const SolverConfig& _config );

void PGSPreconditionConstraintEquations( 
    ConstraintJacobianPair* _preconditionedJacobiansVelStage, 
    ConstraintJacobianPair* _preconditionedJacobiansPosStage, 
    ConstraintVariables* _velocityStageVariables, 
    ConstraintVariables* _positionStageVariables,
    size_t _constraintCount, 
    const boost::uint8_t* _dimensions, 
    const boost::uint8_t* _useBlock,
    const float* __restrict _sorVel,
    const float* __restrict _sorPos,
    const ConstraintJacobianPair* _jacobians, 
    const EffectiveMassPair* _effectiveMassesVelStage,
    const EffectiveMassPair* _effectiveMassesPosStage );

void PGSInitVirtualDisplacements( 
    VirtualDisplacementArray& _virDVel, 
    VirtualDisplacementArray& _virDPos, 
    const EffectiveMassPair* _effectiveMassesVelStage,
    const EffectiveMassPair* _effectiveMassesPosStage,
    size_t _constraintCount, 
    const boost::uint8_t* _dimensions, 
    const ConstraintVariables* __restrict _velStage,
    const ConstraintVariables* __restrict _posStage,
    const BodyPairIndices* _pairs,
    const SolverConfig& _config );

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
    const SolverConfig& _config );

void PGSSolveKernelComputeErrors(
    ArrayBase< float >& _residuals,
    ArrayBase< float >& _deltaResiduals,
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
    const SolverConfig& _config );

}
