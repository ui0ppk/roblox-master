#include "stdafx.h"

#include "solver/SolverConfig.h"
#include "solver/Constraint.h"
#include "solver/DebugSerializer.h"

#include "v8kernel/Body.h"
#include "v8kernel/Constants.h"
#include "util/Units.h"

FASTFLAGVARIABLE( PGSVariablePenetrationMarginFix, false )
FASTFLAGVARIABLE( PGSApplyImpulsesAtMidpoints, false )

namespace RBX
{

//
// Constraint: implementation
//
Constraint::~Constraint()
{
    delete[] cacheData;
}

void Constraint::serialize( DebugSerializer& s ) const
{
    s & type;
    s & uid;
    s & bodyA->getUID();
    s & bodyB->getUID();
    s & dimensions;
    for( boost::uint8_t i = 0; i < dimensions; i++ )
    {
        s & cacheData[ i ];
    }
}

//
// ConstraintBallInSocket
//
void ConstraintBallInSocket::buildEquation( ConstraintJacobianPair* _jacobian, 
                                           boost::uint8_t* _useBlock, 
                                           ConstraintVariables* _velocityStage, 
                                           ConstraintVariables* _positionStage, 
                                           const SolverBodyDynamicProperties& _bodyA, 
                                           const SolverBodyDynamicProperties& _bodyB, 
                                           const SolverConfig& _config, 
                                           float _dt )
{
    const CoordinateFrame& cA = getBodyA()->getPvUnsafe().position;
    CoordinateFrame cB;
    if( getBodyB() != NULL )
    {
        cB = getBodyB()->getPvUnsafe().position;
    }

    Vector3 worldSpacePointA = cA.rotation * pointA + cA.translation;
    Vector3 worldSpacePointB = cB.rotation * pointB + cB.translation;
    Vector3 worldSpaceRelPointA;
    Vector3 worldSpaceRelPointB;
    if( FFlag::PGSApplyImpulsesAtMidpoints )
    {
        Vector3 worldSpacePointMid = 0.5f * ( worldSpacePointA + worldSpacePointB );

        // Compute the pivot positions with respect to the center of mass
        worldSpaceRelPointA = worldSpacePointMid - _bodyA.position;
        worldSpaceRelPointB = worldSpacePointMid - _bodyB.position;
    }
    else
    {
        worldSpaceRelPointA = worldSpacePointA - _bodyA.position;
        worldSpaceRelPointB = worldSpacePointB - _bodyB.position;
    }

    _jacobian[0].a.lin = Vector3( 1.0f, 0.0f, 0.0f );
    _jacobian[0].b.lin = Vector3( -1.0f, 0.0f, 0.0f );
    _jacobian[0].a.ang = -Vector3( 0.0f, -worldSpaceRelPointA.z, worldSpaceRelPointA.y );
    _jacobian[0].b.ang = Vector3( 0.0f, -worldSpaceRelPointB.z, worldSpaceRelPointB.y );

    _jacobian[1].a.lin = Vector3( 0.0f, 1.0f, 0.0f );
    _jacobian[1].b.lin = Vector3( 0.0f, -1.0f, 0.0f );
    _jacobian[1].a.ang = -Vector3( worldSpaceRelPointA.z, 0.0f, -worldSpaceRelPointA.x );
    _jacobian[1].b.ang = Vector3( worldSpaceRelPointB.z, 0.0f, -worldSpaceRelPointB.x );

    _jacobian[2].a.lin = Vector3( 0.0f, 0.0f, 1.0f );
    _jacobian[2].b.lin = Vector3( 0.0f, 0.0f, -1.0f );
    _jacobian[2].a.ang = -Vector3( -worldSpaceRelPointA.y, worldSpaceRelPointA.x, 0.0f );
    _jacobian[2].b.ang = Vector3( -worldSpaceRelPointB.y, worldSpaceRelPointB.x, 0.0f );

    Vector3 vA = _bodyA.integratedLinearVelocity + _bodyA.integratedAngularVelocity.cross( worldSpaceRelPointA );
    Vector3 vB = _bodyB.integratedLinearVelocity + _bodyB.integratedAngularVelocity.cross( worldSpaceRelPointB );
    Vector3 deltaV = vB - vA;
    Vector3 deltaP = worldSpacePointB - worldSpacePointA;
    float errorLength = deltaP.length();

    float lengthToSolveByStabilization = std::min( _config.ballInSocketCorrectionDamping * errorLength, _config.ballInSocketMaxCorrectionByStabilization );

    Vector3 vectorToSolveByStabilization(0.0f, 0.0f, 0.0f);
    if( errorLength > 0.0f )
    {
        vectorToSolveByStabilization = deltaP * ( lengthToSolveByStabilization / errorLength );
    }

    _velocityStage[0].reaction = deltaV.x;
    _velocityStage[1].reaction = deltaV.y;
    _velocityStage[2].reaction = deltaV.z;

    _positionStage[0].reaction = vectorToSolveByStabilization.x;
    _positionStage[1].reaction = vectorToSolveByStabilization.y;
    _positionStage[2].reaction = vectorToSolveByStabilization.z;
}

void ConstraintBallInSocket::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & pointA;
    s & pointB;
}

Constraint::Convergence ConstraintBallInSocket::testPGSConvergence( const float* _disp, const float* _residuals, const float* _deltaResiduals, const SolverConfig& _solverConfig )
{
    float t = _residuals[0]*_residuals[0] + _residuals[1]*_residuals[1] + _residuals[2]*_residuals[2];
    if( t < _solverConfig.inconsistentConstraintBallInSocketResidualThreshold * _solverConfig.inconsistentConstraintBallInSocketResidualThreshold )
    {
        float s = _deltaResiduals[0]*_deltaResiduals[0] + _deltaResiduals[1]*_deltaResiduals[1] + _deltaResiduals[2]*_deltaResiduals[2];
        if( s < _solverConfig.inconsistentConstraintDeltaThreshold * _solverConfig.inconsistentConstraintDeltaThreshold * (_solverConfig.inconsistentConstraintBallInSocketResidualThreshold * _solverConfig.inconsistentConstraintBallInSocketResidualThreshold) )
        {
            return Convergence_Converges;
        }
    }
    // if( ( _deltaResiduals[0] * _residuals[0] > 0.0f ) || ( _deltaResiduals[1] * _residuals[1] > 0.0f ) || ( _deltaResiduals[2] * _residuals[2] > 0.0f ) )
    else
    {
        float s = _deltaResiduals[0]*_deltaResiduals[0] + _deltaResiduals[1]*_deltaResiduals[1] + _deltaResiduals[2]*_deltaResiduals[2];
        if( s < _solverConfig.inconsistentConstraintDeltaThreshold * _solverConfig.inconsistentConstraintDeltaThreshold * t )
        {
            return Convergence_Diverges;
        }
    }
    return Convergence_Undetermined;
}

static float maxForceAdjustment = 1.0f;

//
// ConstraintLegacyBreakableBallInSocket
//
void ConstraintLegacyBreakableBallInSocket::buildEquation( ConstraintJacobianPair* _jacobian, 
                                           boost::uint8_t* _useBlock, 
                                           ConstraintVariables* _velocityStage, 
                                           ConstraintVariables* _positionStage, 
                                           const SolverBodyDynamicProperties& _bodyA, 
                                           const SolverBodyDynamicProperties& _bodyB, 
                                           const SolverConfig& _config, 
                                           float _dt )
{
    const CoordinateFrame& cA = getBodyA()->getPvUnsafe().position;
    CoordinateFrame cB;
    if( getBodyB() != NULL )
    {
        cB = getBodyB()->getPvUnsafe().position;
    }

    _useBlock[0] = false;

    Vector3 worldSpacePointA = cA.rotation * pointA + cA.translation;
    Vector3 worldSpacePointB = cB.rotation * pointB + cB.translation;

    // Compute the pivot positions with respect to the center of mass
    Vector3 worldSpaceRelPointA = worldSpacePointA - _bodyA.position;
    Vector3 worldSpaceRelPointB = worldSpacePointB - _bodyB.position;

    Vector3 worldSpaceNormal = cA.rotation * normalA;
    Vector3 worldSpaceTangent1 = cA.rotation * tangentA1;
    Vector3 worldSpaceTangent2 = cA.rotation * tangentA2;

    _jacobian[0].a.lin = worldSpaceNormal;
    _jacobian[0].b.lin = -worldSpaceNormal;
    _jacobian[0].a.ang = -worldSpaceNormal.cross( worldSpaceRelPointA );
    _jacobian[0].b.ang = worldSpaceNormal.cross( worldSpaceRelPointB );

    _jacobian[1].a.lin = worldSpaceTangent1;
    _jacobian[1].b.lin = -worldSpaceTangent1;
    _jacobian[1].a.ang = -worldSpaceTangent1.cross( worldSpaceRelPointA );
    _jacobian[1].b.ang = worldSpaceTangent1.cross( worldSpaceRelPointB );

    _jacobian[2].a.lin = worldSpaceTangent2;
    _jacobian[2].b.lin = -worldSpaceTangent2;
    _jacobian[2].a.ang = -worldSpaceTangent2.cross( worldSpaceRelPointA );
    _jacobian[2].b.ang = worldSpaceTangent2.cross( worldSpaceRelPointB );

    Vector3 vA = _bodyA.integratedLinearVelocity + _bodyA.integratedAngularVelocity.cross( worldSpaceRelPointA );
    Vector3 vB = _bodyB.integratedLinearVelocity + _bodyB.integratedAngularVelocity.cross( worldSpaceRelPointB );
    Vector3 deltaV = vB - vA;
    Vector3 deltaP = worldSpacePointB - worldSpacePointA;
    float errorLength = deltaP.length();

    float lengthToSolveByStabilization = std::min( _config.ballInSocketCorrectionDamping * errorLength, _config.ballInSocketMaxCorrectionByStabilization );

    Vector3 vectorToSolveByStabilization(0.0f, 0.0f, 0.0f);
    if( errorLength > 0.0f )
    {
        vectorToSolveByStabilization = deltaP * ( lengthToSolveByStabilization / errorLength );
    }

    ConstraintVariables::setReaction( _velocityStage, Vector3( deltaV.dot(worldSpaceNormal), deltaV.dot(worldSpaceTangent1), deltaV.dot(worldSpaceTangent2) ) );
    ConstraintVariables::setReaction( _positionStage, Vector3( vectorToSolveByStabilization.dot(worldSpaceNormal), vectorToSolveByStabilization.dot(worldSpaceTangent1), vectorToSolveByStabilization.dot(worldSpaceTangent2) ) );

    float realMaxForce = maxForceAdjustment * maxNormalForce;
    _velocityStage[0].minImpulseValue = -realMaxForce * _dt;
    static float vStageDamping = 1.0f;
    _velocityStage[0].impulse = vStageDamping * _velocityStage[0].impulse;
    static float pStageDamping = 0.95f;
    _positionStage[0].impulse = pStageDamping * _positionStage[0].impulse;

    if( broken )
    {
        ConstraintVariables::setImpulse(_velocityStage, Vector3(0.0f));
        ConstraintVariables::setImpulse(_positionStage, Vector3(0.0f));
        ConstraintVariables::setReaction(_velocityStage, Vector3(0.0f));
        ConstraintVariables::setReaction(_positionStage, Vector3(0.0f));
        ConstraintVariables::setMinImpulses(_velocityStage, Vector3(0.0f));
        ConstraintVariables::setMaxImpulses(_velocityStage, Vector3(0.0f));
        ConstraintVariables::setMinImpulses(_positionStage, Vector3(0.0f));
        ConstraintVariables::setMaxImpulses(_positionStage, Vector3(0.0f));
    }
}

void ConstraintLegacyBreakableBallInSocket::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & pointA & pointB & normalA & tangentA1 & tangentA2 & maxNormalForce;
}

void ConstraintLegacyBreakableBallInSocket::setNormalOnA( const Vector3& _normal )
{
    normalA = _normal;
    Vector3::generateOrthonormalBasis( tangentA1, tangentA2, normalA, false );
}

bool ConstraintLegacyBreakableBallInSocket::computeBrokenState(
    const ConstraintVariables* _velocityStage, 
    const ConstraintVariables* _positionStage, 
    const SolverConfig& _config ) const 
{
    return _velocityStage[0].impulse < -0.9f * maxForceAdjustment * maxNormalForce * Constants::worldDt();
}

//
// ConstraintAlign2Axes
//
#ifdef ENABLE_HINGE_FRICTION
static const int align2AxesDimension = 3;
#else
static const int align2AxesDimension = 2;
#endif
ConstraintAlign2Axes::ConstraintAlign2Axes( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_Align2Axes, _bodyA, _bodyB, align2AxesDimension ), 
    worldSpaceOrthogonalB1(0.0f,0.0f,0.0f),
    worldSpaceOrthogonalB2(0.0f,0.0f,0.0f)
{ }

void ConstraintAlign2Axes::buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt )
{
    const CoordinateFrame& cA = getBodyA()->getPvUnsafe().position;
    CoordinateFrame cB;
    if( getBodyB() != NULL )
    {
        cB = getBodyB()->getPvUnsafe().position;
    }

    Vector3 oldWorldSpaceOrthogonalB1 = worldSpaceOrthogonalB1;
    Vector3 oldWorldSpaceOrthogonalB2 = worldSpaceOrthogonalB2;

    worldSpaceOrthogonalB1 = cB.rotation * orthogonalAxisB1;
    worldSpaceOrthogonalB2 = cB.rotation * orthogonalAxisB2;
    Vector3 worldSpaceAxisB = cB.rotation * axisB;
    Vector3 worldSpaceAxisA = cA.rotation * axisA;

    _jacobian[0].a.lin = Vector3( 0.0f );
    _jacobian[0].b.lin = Vector3( 0.0f );
    _jacobian[0].a.ang = worldSpaceOrthogonalB1;
    _jacobian[0].b.ang = -worldSpaceOrthogonalB1;

    _jacobian[1].a.lin = Vector3( 0.0f );
    _jacobian[1].b.lin = Vector3( 0.0f );
    _jacobian[1].a.ang = worldSpaceOrthogonalB2;
    _jacobian[1].b.ang = -worldSpaceOrthogonalB2;

#ifdef ENABLE_HINGE_FRICTION
    _jacobian[2].a.lin = Vector3( 0.0f );
    _jacobian[2].b.lin = Vector3( 0.0f );
    _jacobian[2].a.ang = worldSpaceAxisB;
    _jacobian[2].b.ang = -worldSpaceAxisB;
    _useBlock[2] = false;
#endif

    // Velocity stage
    Vector3 relAngularVel = _bodyB.integratedAngularVelocity - _bodyA.integratedAngularVelocity;
    Vector3 worldSpaceVelImpulse = _velocityStage[ 0 ].impulse * oldWorldSpaceOrthogonalB1 + _velocityStage[ 1 ].impulse * oldWorldSpaceOrthogonalB2;

    _velocityStage[0].impulse = worldSpaceOrthogonalB1.dot( worldSpaceVelImpulse );
    _velocityStage[0].reaction = worldSpaceOrthogonalB1.dot( relAngularVel );

    _velocityStage[1].impulse = worldSpaceOrthogonalB2.dot( worldSpaceVelImpulse );
    _velocityStage[1].reaction = worldSpaceOrthogonalB2.dot( relAngularVel );

#ifdef ENABLE_HINGE_FRICTION
    Vector2 velocityImpulse( _velocityStage[ 0 ].impulse, _velocityStage[ 1 ].impulse );
    float maxImpulse = _config.align2AxesFrictionConstant * velocityImpulse.length();
    float minImpulse = -maxImpulse;

    _velocityStage[2].maxImpulseValue = maxImpulse;
    _velocityStage[2].minImpulseValue = minImpulse;
    _velocityStage[2].reaction = worldSpaceAxisB.dot( relAngularVel );
#endif

    // Position stage
    // If both axes are parallel, this cross product should be 0
    Vector3 angularError = worldSpaceAxisA.cross( worldSpaceAxisB );

    float cosAngle = worldSpaceAxisA.dot( worldSpaceAxisB );

    // The angle between the two axes is smaller than 15deg, we can use the approximation sin x = x
    // Otherwise we'll have to compute the angle using inverse trig
    // Equivalent to angle > 15deg this path will be rarely taken, unless missalignment is common
    float maxCorrectiveAngle = _config.align2AxesMaxCorrectiveAngle * ( boost::math::constants::pi< float >() / 180.0f );
    if( cosAngle < cosf( maxCorrectiveAngle ) )
    {
        float sinAngle = angularError.magnitude();
        // Want to compute angle/sin(angle)
        float scale = 0.0f;
        if( sinAngle > 0.00001f )
        {
            scale = sinf( maxCorrectiveAngle ) / sinAngle;
        }
        else
        {
            scale = 0.0f;
        }
        angularError = scale * angularError;
    }

    angularError *= _config.align2AxesCorrectionDamping;
    Vector3 worldSpacePosImpulse = _positionStage[ 0 ].impulse * oldWorldSpaceOrthogonalB1 + _positionStage[ 1 ].impulse * oldWorldSpaceOrthogonalB2;

    _positionStage[0].impulse = worldSpaceOrthogonalB1.dot( worldSpacePosImpulse );
    _positionStage[0].reaction = worldSpaceOrthogonalB1.dot( angularError );

    _positionStage[1].impulse = worldSpaceOrthogonalB2.dot( worldSpacePosImpulse );
    _positionStage[1].reaction = worldSpaceOrthogonalB2.dot( angularError );

#ifdef ENABLE_HINGE_FRICTION
    Vector2 positionImpulse( _positionStage[ 0 ].impulse, _positionStage[ 1 ].impulse );
    float maxPosFrictionImpulse = _config.align2AxesPositionStageFrictionConstant * positionImpulse.length();

    _positionStage[2].maxImpulseValue = maxPosFrictionImpulse;
    _positionStage[2].minImpulseValue = -maxPosFrictionImpulse;
    _positionStage[2].reaction = 0.0f;
#endif
}

void ConstraintAlign2Axes::setAxisA( const Vector3& _axisA )
{
    axisA = _axisA;
}

void ConstraintAlign2Axes::setAxisB( const Vector3& _axisB )
{
    axisB = _axisB;
    Vector3::generateOrthonormalBasis( orthogonalAxisB1, orthogonalAxisB2, axisB, false );
}

void ConstraintAlign2Axes::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & axisA & axisB & orthogonalAxisB1 & orthogonalAxisB2 & worldSpaceOrthogonalB1 & worldSpaceOrthogonalB2;
}

Constraint::Convergence ConstraintAlign2Axes::testPGSConvergence( const float* _disp, const float* _residuals, const float* _deltaResiduals, const SolverConfig& _solverConfig )
{
    float t = _residuals[0]*_residuals[0] + _residuals[1]*_residuals[1];
    if( t < _solverConfig.inconsistentConstraintAlign2AxesThreshold * _solverConfig.inconsistentConstraintAlign2AxesThreshold )
    {
        float s = _deltaResiduals[0]*_deltaResiduals[0] + _deltaResiduals[1]*_deltaResiduals[1];
        if( s < _solverConfig.inconsistentConstraintDeltaThreshold * _solverConfig.inconsistentConstraintDeltaThreshold * ( _solverConfig.inconsistentConstraintAlign2AxesThreshold * _solverConfig.inconsistentConstraintAlign2AxesThreshold ) )
        {
            return Convergence_Converges;
        }
    }
    else
    {
        float s = _deltaResiduals[0]*_deltaResiduals[0] + _deltaResiduals[1]*_deltaResiduals[1];
        if( s < _solverConfig.inconsistentConstraintDeltaThreshold * _solverConfig.inconsistentConstraintDeltaThreshold * t )
        {
            return Convergence_Diverges;
        }
    }
    return Convergence_Undetermined;
}

//
// ConstraintAngularVelocity
//
void ConstraintAngularVelocity::buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt )
{
    CoordinateFrame cB;
    if( getBodyB() ) 
    {
        cB = getBodyB()->getPvUnsafe().position;
    }

    Vector3 worldSpaceAxisB = cB.rotation * axisB;

    _jacobian[0].a.lin = Vector3( 0.0f, 0.0f, 0.0f );
    _jacobian[0].b.lin = Vector3( 0.0f, 0.0f, 0.0f );
    _jacobian[0].a.ang = worldSpaceAxisB;
    _jacobian[0].b.ang = -worldSpaceAxisB;

    Vector3 relAngularVel = _bodyB.integratedAngularVelocity - _bodyA.integratedAngularVelocity;
    float rotVel = worldSpaceAxisB.dot( relAngularVel ) - desiredAngularVelocity;
    float maxImpulse = maxForce * _dt;

    _velocityStage[0].maxImpulseValue = maxImpulse;
    _velocityStage[0].minImpulseValue = -maxImpulse;
    _velocityStage[0].reaction = rotVel;

    _positionStage[0].maxImpulseValue = 0.0f;
    _positionStage[0].minImpulseValue = 0.0f;
    _positionStage[0].reaction = 0.0f;
}

void ConstraintAngularVelocity::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & axisA & axisB & maxForce & desiredAngularVelocity;
}

//
// ConstraintLinearVelocity
//
void ConstraintLinearVelocity::buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt )
{
    _useBlock[0]=0;
    _useBlock[1]=0;
    _useBlock[2]=0;

    Vector3 zeroV( 0.0f );

    _jacobian[0].a.lin = Vector3( 1.0f, 0.0f, 0.0f );
    _jacobian[0].b.lin = Vector3( -1.0f, 0.0f, 0.0f );
    _jacobian[0].a.ang = zeroV;
    _jacobian[0].b.ang = zeroV;

    _jacobian[1].a.lin = Vector3( 0.0f, 1.0f, 0.0f );
    _jacobian[1].b.lin = Vector3( 0.0f, -1.0f, 0.0f );
    _jacobian[1].a.ang = zeroV;
    _jacobian[1].b.ang = zeroV;

    _jacobian[2].a.lin = Vector3( 0.0f, 0.0f, 1.0f );
    _jacobian[2].b.lin = Vector3( 0.0f, 0.0f, -1.0f );
    _jacobian[2].a.ang = zeroV;
    _jacobian[2].b.ang = zeroV;

    Vector3 relLinVel = ( _bodyB.integratedLinearVelocity - _bodyA.integratedLinearVelocity );

    ConstraintVariables::setReaction(_velocityStage, desiredVelocity + relLinVel );
    ConstraintVariables::setMinImpulses(_velocityStage, -maxForce * _dt);
    ConstraintVariables::setMaxImpulses(_velocityStage, maxForce * _dt);

    ConstraintVariables::setReaction(_positionStage, zeroV);
    ConstraintVariables::setMinImpulses(_positionStage, zeroV);
    ConstraintVariables::setMaxImpulses(_positionStage, zeroV);
}

void ConstraintLinearVelocity::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & maxForce & desiredVelocity;
}

static inline Vector3 fabsv( const Vector3& a )
{
    return Vector3( fabsf(a.x), fabsf(a.y), fabsf(a.z) );
}

static inline Vector3 fselv( const Vector3& a, const Vector3& b, const Vector3& sel )
{
    Vector3 r;
    r.x = sel.x >= 0 ? a.x : b.x;
    r.y = sel.y >= 0 ? a.y : b.y;
    r.z = sel.z >= 0 ? a.z : b.z;
    return r;
}

//
// ConstraintLinearVelocity
//
void ConstraintLinearSpring::buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt )
{
    const CoordinateFrame& cA = getBodyA()->getPvUnsafe().position;
    CoordinateFrame cB;
    if( getBodyB() != NULL )
    {
        cB = getBodyB()->getPvUnsafe().position;
    }

    _useBlock[0]=0;
    _useBlock[1]=0;
    _useBlock[2]=0;

    Vector3 worldSpacePointA = cA.rotation * pivotA + cA.translation;
    Vector3 worldSpacePointB = cB.rotation * pivotB + cB.translation;

    // Compute the pivot positions with respect to the center of mass
    Vector3 worldSpaceRelPointA = worldSpacePointA - _bodyA.position;
    Vector3 worldSpaceRelPointB = worldSpacePointB - _bodyB.position;

    Vector3 zeroV( 0.0f );

    _jacobian[0].a.lin = Vector3( 1.0f, 0.0f, 0.0f );
    _jacobian[0].b.lin = Vector3( -1.0f, 0.0f, 0.0f );
    _jacobian[0].a.ang = -Vector3( 0.0f, -worldSpaceRelPointA.z, worldSpaceRelPointA.y );
    _jacobian[0].b.ang = Vector3( 0.0f, -worldSpaceRelPointB.z, worldSpaceRelPointB.y );

    _jacobian[1].a.lin = Vector3( 0.0f, 1.0f, 0.0f );
    _jacobian[1].b.lin = Vector3( 0.0f, -1.0f, 0.0f );
    _jacobian[1].a.ang = -Vector3( worldSpaceRelPointA.z, 0.0f, -worldSpaceRelPointA.x );
    _jacobian[1].b.ang = Vector3( worldSpaceRelPointB.z, 0.0f, -worldSpaceRelPointB.x );

    _jacobian[2].a.lin = Vector3( 0.0f, 0.0f, 1.0f );
    _jacobian[2].b.lin = Vector3( 0.0f, 0.0f, -1.0f );
    _jacobian[2].a.ang = -Vector3( -worldSpaceRelPointA.y, worldSpaceRelPointA.x, 0.0f );
    _jacobian[2].b.ang = Vector3( -worldSpaceRelPointB.y, worldSpaceRelPointB.x, 0.0f );

    Vector3 vA = _bodyA.integratedLinearVelocity + _bodyA.integratedAngularVelocity.cross( worldSpaceRelPointA );
    Vector3 vB = _bodyB.integratedLinearVelocity + _bodyB.integratedAngularVelocity.cross( worldSpaceRelPointB );

    Vector3 deltaV = vA - vB;
    Vector3 deltaP = worldSpacePointA - worldSpacePointB;
    Vector3 reaction = -( p * deltaP + d * deltaV ) * Constants::worldDt();

    ConstraintVariables::setReaction(_velocityStage, reaction );
    ConstraintVariables::setMinImpulses(_velocityStage, -maxForce * _dt);
    ConstraintVariables::setMaxImpulses(_velocityStage, maxForce * _dt);

    ConstraintVariables::setReaction(_positionStage, zeroV);
    ConstraintVariables::setMinImpulses(_positionStage, zeroV);
    ConstraintVariables::setMaxImpulses(_positionStage, zeroV);
}

void ConstraintLinearSpring::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & pivotA & pivotB & maxForce & p & d;
}

//
// ConstraintAchievePosition
//
void ConstraintAchievePosition::buildEquation( ConstraintJacobianPair* _jacobian, 
                                           boost::uint8_t* _useBlock, 
                                           ConstraintVariables* _velocityStage, 
                                           ConstraintVariables* _positionStage, 
                                           const SolverBodyDynamicProperties& _bodyA, 
                                           const SolverBodyDynamicProperties& _bodyB, 
                                           const SolverConfig& _config, 
                                           float _dt )
{
    const CoordinateFrame& cA = getBodyA()->getPvUnsafe().position;
    CoordinateFrame cB;
    if( getBodyB() != NULL )
    {
        cB = getBodyB()->getPvUnsafe().position;
    }

    _useBlock[0]=0;
    _useBlock[1]=0;
    _useBlock[2]=0;

    Vector3 worldSpacePointA = cA.rotation * pivotA + cA.translation;
    Vector3 worldSpacePointB = cB.rotation * pivotB + cB.translation;

    // Compute the pivot positions with respect to the center of mass
    Vector3 worldSpaceRelPointA;
    Vector3 worldSpaceRelPointB;

    if( FFlag::PGSApplyImpulsesAtMidpoints )
    {
        Vector3 worldSpacePointMid = 0.5f * ( worldSpacePointA + worldSpacePointB );

        // Compute the pivot positions with respect to the center of mass
        worldSpaceRelPointA = worldSpacePointMid - _bodyA.position;
        worldSpaceRelPointB = worldSpacePointMid - _bodyB.position;
    }
    else
    {
        worldSpaceRelPointA = worldSpacePointA - _bodyA.position;
        worldSpaceRelPointB = worldSpacePointB - _bodyB.position;
    }

    _jacobian[0].a.lin = Vector3( 1.0f, 0.0f, 0.0f );
    _jacobian[0].b.lin = Vector3( -1.0f, 0.0f, 0.0f );
    _jacobian[0].a.ang = -Vector3( 0.0f, -worldSpaceRelPointA.z, worldSpaceRelPointA.y );
    _jacobian[0].b.ang = Vector3( 0.0f, -worldSpaceRelPointB.z, worldSpaceRelPointB.y );

    _jacobian[1].a.lin = Vector3( 0.0f, 1.0f, 0.0f );
    _jacobian[1].b.lin = Vector3( 0.0f, -1.0f, 0.0f );
    _jacobian[1].a.ang = -Vector3( worldSpaceRelPointA.z, 0.0f, -worldSpaceRelPointA.x );
    _jacobian[1].b.ang = Vector3( worldSpaceRelPointB.z, 0.0f, -worldSpaceRelPointB.x );

    _jacobian[2].a.lin = Vector3( 0.0f, 0.0f, 1.0f );
    _jacobian[2].b.lin = Vector3( 0.0f, 0.0f, -1.0f );
    _jacobian[2].a.ang = -Vector3( -worldSpaceRelPointA.y, worldSpaceRelPointA.x, 0.0f );
    _jacobian[2].b.ang = Vector3( -worldSpaceRelPointB.y, worldSpaceRelPointB.x, 0.0f );

    Vector3 vA = _bodyA.integratedLinearVelocity + _bodyA.integratedAngularVelocity.cross( worldSpaceRelPointA );
    Vector3 vB = _bodyB.integratedLinearVelocity + _bodyB.integratedAngularVelocity.cross( worldSpaceRelPointB );
    Vector3 deltaV = vB - vA;
    Vector3 deltaP = worldSpacePointB - worldSpacePointA;

    // Interpolate between all velocity based correction to all position based.
    static float thresholdMinCoef = 0.1f;
    static float thresholdMaxCoef = 1.0f;
    Vector3 thresholdMin = thresholdMinCoef * fabsv( targetVelocity ) * _dt;
    Vector3 thresholdMax = thresholdMaxCoef * fabsv( targetVelocity ) * _dt;
    Vector3 absoluteDeltaP = fabsv( deltaP );
    Vector3 t = ( absoluteDeltaP - thresholdMin ) / ( thresholdMax - thresholdMin );
    t = fselv( Vector3(1.0f), fselv( Vector3(0.0f), t, thresholdMin - absoluteDeltaP ), absoluteDeltaP - thresholdMax );

    RBXASSERT( t.x>=0.0f && t.x<=1.0f );
    RBXASSERT( t.y>=0.0f && t.y<=1.0f );
    RBXASSERT( t.z>=0.0f && t.z<=1.0f );

    Vector3 stabilizedVel = t * targetVelocity;
    Vector3 vectorToSolveByStabilization = ( Vector3(1.0f) - t ) * deltaP;

    deltaV += stabilizedVel;
    ConstraintVariables::setReaction(_velocityStage, deltaV);
    Vector3 maxVelImpulse = maxForce * _dt;
    ConstraintVariables::setMinImpulses(_velocityStage,-maxVelImpulse);
    ConstraintVariables::setMaxImpulses(_velocityStage,maxVelImpulse);

    ConstraintVariables::setReaction(_positionStage, vectorToSolveByStabilization);
    Vector3 maxPosImpulse = ( Vector3(1.0f) - t ) * maxVelImpulse * _dt;
    ConstraintVariables::setMinImpulses(_positionStage, -maxPosImpulse);
    ConstraintVariables::setMaxImpulses(_positionStage, maxPosImpulse);
}

void ConstraintAchievePosition::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & pivotA;
    s & pivotB;
    s & minForce;
    s & maxForce;
    s & targetVelocity;
}

//
// ConstraintCollisions
//
void ConstraintCollision::buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt )
{
    Vector3 worldSpaceNormalB = normal;
    Vector3 relativePositionA;
    Vector3 relativePositionB;
    
    if( FFlag::PGSApplyImpulsesAtMidpoints )
    {
        Vector3 midPoint = pointA + 0.5f * ( depth * worldSpaceNormalB );
        relativePositionA = midPoint - _bodyA.position;
        relativePositionB = midPoint - _bodyB.position;
    }
    else
    {
        relativePositionA = pointA - _bodyA.position;
        relativePositionB = pointA + depth * worldSpaceNormalB - _bodyB.position;
    }

    _jacobian[0].a.lin = -worldSpaceNormalB;
    _jacobian[0].b.lin = worldSpaceNormalB;
    _jacobian[0].a.ang = -relativePositionA.cross( worldSpaceNormalB );
    _jacobian[0].b.ang = relativePositionB.cross( worldSpaceNormalB );

    _useBlock[0] = false;
    _useBlock[1] = false;
    _useBlock[2] = false;

    Vector3 integratedDeltaV = ( _bodyA.integratedLinearVelocity + _bodyA.integratedAngularVelocity.cross( relativePositionA ) )
        - ( _bodyB.integratedLinearVelocity + _bodyB.integratedAngularVelocity.cross( relativePositionB ) );
    float integratedNormalVelocity = integratedDeltaV.dot( worldSpaceNormalB );

    Vector3 previousDeltaV = ( _bodyA.linearVelocity + _bodyA.angularVelocity.cross( relativePositionA ) )
        - ( _bodyB.linearVelocity + _bodyB.angularVelocity.cross( relativePositionB ) );
    float previousNormalVelocity = previousDeltaV.dot( worldSpaceNormalB );

    // Restitute using solved velocities to avoid adding energy
    _velocityStage[ 0 ].reaction = integratedNormalVelocity + restitution * ( previousNormalVelocity < _config.collisionRestitutionThreshold ? 0.0f : previousNormalVelocity );
    _velocityStage[ 0 ].minImpulseValue = 0.0f;
    _velocityStage[ 0 ].maxImpulseValue = std::numeric_limits< float >::infinity(); // change this if you want soft collisions

    float penetrationMargin = _config.collisionPenetrationMargin;

    Vector3 integratedTangentialVelocity = integratedDeltaV - integratedNormalVelocity * worldSpaceNormalB;

    Vector3 previousTangentialVelocity = previousDeltaV - previousNormalVelocity * worldSpaceNormalB;
    float previousTangentialVelocitySquared = previousTangentialVelocity.squaredLength();

    Vector3 cachedTangent2 = cachedTangent1.cross( worldSpaceNormalB );
    if( previousTangentialVelocitySquared < _config.collisionFrictionStaticToDynamicThreshold * _config.collisionFrictionStaticToDynamicThreshold )
    {
        // Static friction
        Vector3 worldSpaceTangent1;
        Vector3 worldSpaceTangent2;
        Vector3::generateOrthonormalBasis( worldSpaceTangent1, worldSpaceTangent2, worldSpaceNormalB, true );

        _jacobian[1].a.lin = -worldSpaceTangent1;
        _jacobian[1].b.lin = worldSpaceTangent1;
        _jacobian[1].a.ang = -relativePositionA.cross( worldSpaceTangent1 );
        _jacobian[1].b.ang = relativePositionB.cross( worldSpaceTangent1 );

        _jacobian[2].a.lin = -worldSpaceTangent2;
        _jacobian[2].b.lin = worldSpaceTangent2;
        _jacobian[2].a.ang = -relativePositionA.cross( worldSpaceTangent2 );
        _jacobian[2].b.ang = relativePositionB.cross( worldSpaceTangent2 );

        float frictionBound = _config.collisionFrictionStaticScale * friction;

        {
            Vector3 tangentVelocityCachedImpulse = _velocityStage[ 1 ].impulse * cachedTangent1 + _velocityStage[ 2 ].impulse * cachedTangent2;

            _velocityStage[ 1 ].reaction = integratedTangentialVelocity.dot( worldSpaceTangent1 );
            _velocityStage[ 1 ].impulse = tangentVelocityCachedImpulse.dot( worldSpaceTangent1 );
            _velocityStage[ 1 ].minImpulseValue = -frictionBound;
            _velocityStage[ 1 ].maxImpulseValue = frictionBound;

            _velocityStage[ 2 ].reaction = integratedTangentialVelocity.dot( worldSpaceTangent2 );
            _velocityStage[ 2 ].impulse = tangentVelocityCachedImpulse.dot( worldSpaceTangent2 );
            _velocityStage[ 2 ].minImpulseValue = -frictionBound;
            _velocityStage[ 2 ].maxImpulseValue = frictionBound;
        }

        {
            Vector3 tangentPositionCachedImpulse = _positionStage[ 1 ].impulse * cachedTangent1 + _positionStage[ 2 ].impulse * cachedTangent2;

            _positionStage[ 1 ].reaction = 0.0f;
            _positionStage[ 1 ].impulse = tangentPositionCachedImpulse.dot( worldSpaceTangent1 );
            _positionStage[ 1 ].minImpulseValue = -frictionBound;
            _positionStage[ 1 ].maxImpulseValue = frictionBound;

            _positionStage[ 2 ].reaction = 0.0f;
            _positionStage[ 2 ].impulse = tangentPositionCachedImpulse.dot( worldSpaceTangent2 );
            _positionStage[ 2 ].minImpulseValue = -frictionBound;
            _positionStage[ 2 ].maxImpulseValue = frictionBound;
        }

        cachedTangent1 = worldSpaceTangent1;

        if( FFlag::PGSVariablePenetrationMarginFix )
        {
            float gravity = fabsf( Units::kmsAccelerationToRbx( Constants::getKmsGravity() ) );

            // Adding an epsilon to the gravity to avoid 0/0 = #nan in case gravity is 0.0f
            // Since gravity is already ~200, an epsilon of 0.1 will do
            // and we want d -> inf as angularVelocity -> 0
            // Here is the math: given a sphere of radius r, embedded inside a plane, with a shallow depth d,
            // the radius of the cross section of the sphere on the plane is
            // l = sqrt( d*( 2*r - d ) ) ~ sqrt( 2*d*r )
            // As d is assumed to be small compare to the radius of the sphere: d << r
            // If the angular velocity of the sphere is a
            // The vertical motion at the intersection circle is at most:
            // v = r * a * l / r = a * sqrt( 2*d*r )
            // The max height of an object moving at velocity v is h = v^2/ ( 2*g ), where g is the gravity, so
            // epsilon = h / r = a^2 * d / g
            // therefore d = epsilon * g / a^2
            float dA = _config.collisionPenetrationMarginMaxBumpProportions * ( gravity + 0.1f ) / _bodyA.angularVelocity.squaredLength();
            float dB = _config.collisionPenetrationMarginMaxBumpProportions * ( gravity + 0.1f ) / _bodyB.angularVelocity.squaredLength();

            // Clamping to max and min
            penetrationMargin = std::max( _config.collisionPenetrationMarginMin, std::min( _config.collisionPenetrationMarginMax, std::min( dA, dB ) ) );
        }
    }
    else
    {
        // Dynamic friction
        Vector3 worldSpaceTangent1 = previousTangentialVelocity.direction();
        Vector3 worldSpaceTangent2 = worldSpaceTangent1.cross( worldSpaceNormalB );

        _jacobian[1].a.lin = -worldSpaceTangent1;
        _jacobian[1].b.lin = worldSpaceTangent1;
        _jacobian[1].a.ang = -relativePositionA.cross( worldSpaceTangent1 );
        _jacobian[1].b.ang = relativePositionB.cross( worldSpaceTangent1 );

        _jacobian[2].a.lin = -worldSpaceTangent2;
        _jacobian[2].b.lin = worldSpaceTangent2;
        _jacobian[2].a.ang = -relativePositionA.cross( worldSpaceTangent2 );
        _jacobian[2].b.ang = relativePositionB.cross( worldSpaceTangent2 );

        {
            Vector3 tangentVelocityCachedImpulse = _velocityStage[ 1 ].impulse * cachedTangent1 + _velocityStage[ 2 ].impulse * cachedTangent2;

            _velocityStage[ 1 ].reaction = integratedTangentialVelocity.dot( worldSpaceTangent1 );
            _velocityStage[ 1 ].impulse = tangentVelocityCachedImpulse.dot( worldSpaceTangent1 );
            _velocityStage[ 1 ].minImpulseValue = 0.0f;
            _velocityStage[ 1 ].maxImpulseValue = _config.collisionFrictionDynamicScale * friction;

            _velocityStage[ 2 ].reaction = 0.0f;
            _velocityStage[ 2 ].impulse = 0.0f;
            _velocityStage[ 2 ].minImpulseValue = 0.0f;
            _velocityStage[ 2 ].maxImpulseValue = 0.0f;
        }

        _positionStage[ 1 ].reaction = 0.0f;
        _positionStage[ 1 ].impulse = 0.0f;
        _positionStage[ 1 ].minImpulseValue = 0.0f;
        _positionStage[ 1 ].maxImpulseValue = 0.0f;

        _positionStage[ 2 ].reaction = 0.0f;
        _positionStage[ 2 ].impulse = 0.0f;
        _positionStage[ 2 ].minImpulseValue = 0.0f;
        _positionStage[ 2 ].maxImpulseValue = 0.0f;

        cachedTangent1 = worldSpaceTangent1;

        if( FFlag::PGSVariablePenetrationMarginFix )
        {
            float dynamicVelocityExcess = std::min( _config.collisionPenetrationVelocityForMinMargin, sqrtf( previousTangentialVelocitySquared ) - _config.collisionFrictionStaticToDynamicThreshold );
            float t = dynamicVelocityExcess / ( _config.collisionPenetrationVelocityForMinMargin - _config.collisionFrictionStaticToDynamicThreshold );
            penetrationMargin = t * _config.collisionPenetrationMarginMin + ( 1.0f - t ) * _config.collisionPenetrationMarginMax;
        }
    }

    _positionStage[ 0 ].reaction = -_config.collisionPenetrationResolutionDamping * ( depth + penetrationMargin );
    _positionStage[ 0 ].minImpulseValue = 0.0f;
    _positionStage[ 0 ].maxImpulseValue = std::numeric_limits< float >::infinity(); // change this if you want soft collisions
}

void ConstraintCollision::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & normal & pointA & depth & friction & restitution & cachedTangent1;
}

Constraint::Convergence ConstraintCollision::testPGSConvergence( const float* _disp, const float* _residuals, const float* _deltaResiduals, const SolverConfig& _solverConfig )
{
    if( -_solverConfig.inconsistentConstraintCollisionBaseThreshold < _residuals[0] && _residuals[0] < _solverConfig.inconsistentConstraintCollisionThreshold )
    {
        float s = fabsf( _deltaResiduals[0] );
        if( s < _solverConfig.inconsistentConstraintDeltaThreshold * _solverConfig.inconsistentConstraintCollisionThreshold )
        {
            return Convergence_Converges;
        }
    }
    
    if( _residuals[0] >= _solverConfig.inconsistentConstraintCollisionThreshold )
    {
        if( _deltaResiduals[0] > -_solverConfig.inconsistentConstraintDeltaThreshold * _residuals[0] )
        {
            return Convergence_Diverges;
        }
    }

    return Convergence_Undetermined;
}

//
// ConstraintBodyAngularVelocity
//
void ConstraintBodyAngularVelocity::buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt )
{
    const CoordinateFrame& cA = getBodyA()->getPvUnsafe().position;

    _useBlock[0]=0;
    _useBlock[1]=0;
    _useBlock[2]=0;

    Vector3 zeroV( 0.0f );

    _jacobian[0].a.lin = zeroV;
    _jacobian[0].b.lin = zeroV;
    _jacobian[0].a.ang = cA.rotation * Vector3( 1.0f, 0.0f, 0.0f );
    _jacobian[0].b.ang = cA.rotation * Vector3( -1.0f, 0.0f, 0.0f );

    _jacobian[1].a.lin = zeroV;
    _jacobian[1].b.lin = zeroV;
    _jacobian[1].a.ang = cA.rotation * Vector3( 0.0f, 1.0f, 0.0f );
    _jacobian[1].b.ang = cA.rotation * Vector3( 0.0f, -1.0f, 0.0f );

    _jacobian[2].a.lin = zeroV;
    _jacobian[2].b.lin = zeroV;
    _jacobian[2].a.ang = cA.rotation * Vector3( 0.0f, 0.0f, 1.0f );
    _jacobian[2].b.ang = cA.rotation * Vector3( 0.0f, 0.0f, -1.0f );

    Vector3 relAngVel;
    if( useIntegratedVelocities )
    {
        relAngVel = cA.vectorToObjectSpace( _bodyB.integratedAngularVelocity - _bodyA.integratedAngularVelocity );
    }
    else
    {
        relAngVel = cA.vectorToObjectSpace( _bodyB.angularVelocity - _bodyA.angularVelocity );
    }
    
    ConstraintVariables::setReaction(_velocityStage, targetAngularVelocity + relAngVel );
    ConstraintVariables::setMinImpulses(_velocityStage, minTorque * _dt);
    ConstraintVariables::setMaxImpulses(_velocityStage, maxTorque * _dt);

    ConstraintVariables::setReaction(_positionStage, zeroV);
    ConstraintVariables::setMinImpulses(_positionStage, zeroV);
    ConstraintVariables::setMaxImpulses(_positionStage, zeroV);
}

void ConstraintBodyAngularVelocity::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & targetAngularVelocity & minTorque & maxTorque;
}

//
// ConstraintBodyAngularVelocity
//
void ConstraintLegacyAngularVelocity::buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt )
{
    _useBlock[0]=0;
    _useBlock[1]=0;
    _useBlock[2]=0;

    Vector3 zeroV( 0.0f );

    _jacobian[0].a.lin = zeroV;
    _jacobian[0].b.lin = zeroV;
    _jacobian[0].a.ang = Vector3( 1.0f, 0.0f, 0.0f );
    _jacobian[0].b.ang = Vector3( -1.0f, 0.0f, 0.0f );

    _jacobian[1].a.lin = zeroV;
    _jacobian[1].b.lin = zeroV;
    _jacobian[1].a.ang = Vector3( 0.0f, 1.0f, 0.0f );
    _jacobian[1].b.ang = Vector3( 0.0f, -1.0f, 0.0f );

    _jacobian[2].a.lin = zeroV;
    _jacobian[2].b.lin = zeroV;
    _jacobian[2].a.ang = Vector3( 0.0f, 0.0f, 1.0f );
    _jacobian[2].b.ang = Vector3( 0.0f, 0.0f, -1.0f );

    Vector3 relAngVel;
    if( useIntegratedVelocities )
    {
        relAngVel = ( _bodyB.integratedAngularVelocity - _bodyA.integratedAngularVelocity );
    }
    else
    {
        relAngVel = ( _bodyB.angularVelocity - _bodyA.angularVelocity );
    }

    ConstraintVariables::setReaction(_velocityStage, targetAngularVelocity + relAngVel );
    ConstraintVariables::setMinImpulses(_velocityStage, minTorque * _dt);
    ConstraintVariables::setMaxImpulses(_velocityStage, maxTorque * _dt);

    ConstraintVariables::setReaction(_positionStage, zeroV);
    ConstraintVariables::setMinImpulses(_positionStage, zeroV);
    ConstraintVariables::setMaxImpulses(_positionStage, zeroV);
}

void ConstraintLegacyAngularVelocity::serialize( DebugSerializer& s ) const
{
    Constraint::serialize(s);
    s & targetAngularVelocity & minTorque & maxTorque;
}

//
// ConstraintCache
//
inline float recomputeSor( float sor, float relError, const SolverConfig::ModulationParams& config )
{
#ifndef ENABLE_LOCAL_SOR_MODULATION
    return sor;
#endif
    float sorThresholdMax = config.thresholdMax;
    float sorThresholdMin = config.thresholdMin;
    float maxSor = config.aggressiveValue;
    float minSor = config.conservativeValue;
    float easingConstantUp = config.easingUpToAggressive;
    float easingConstantDown = config.easingDownToConservative;

    float newSor = maxSor;
    if( relError > sorThresholdMin )
    {
        float t = std::min( relError, sorThresholdMax );
        t = std::max( t, sorThresholdMin );
        t = ( t - sorThresholdMin ) / ( sorThresholdMax - sorThresholdMin );
        newSor = t * minSor + ( 1.0f - t ) * maxSor;
    }
    if( newSor > sor )
    {
        sor = easingConstantUp * newSor +  ( 1.0f - easingConstantUp ) * sor;
    }
    else
    {
        sor = easingConstantDown * newSor +  ( 1.0f - easingConstantDown ) * sor;
    }

    return sor;
}

static inline float calculateCacheDampingFactor( float fit, float oldFactor, const SolverConfig::ModulationParams& config )
{
#ifndef ENABLE_IMPULSE_CACHE_DAMPING_PER_EQUATION
    return oldFactor;
#endif
    float minThreshold = config.thresholdMin;
    float maxThreshold = config.thresholdMax;
    float minFactor = config.conservativeValue;
    float maxFactor = config.aggressiveValue;
    float easingDownCoef = config.easingDownToConservative;
    float easingUpCoef = config.easingUpToAggressive;

    float t = ( std::max( std::min( fit, maxThreshold ), minThreshold ) - minThreshold ) / ( maxThreshold - minThreshold );
    float factor = t * minFactor + ( 1.0f - t ) * maxFactor;
    float easing = factor < oldFactor ? easingDownCoef : easingUpCoef;
    factor = easing * factor + ( 1.0f - easing ) * oldFactor;

    return factor;
}

void ConstraintCache::cache( const ConstraintVariables& _velocityStage, const ConstraintVariables& _positionStage, float _sorVel, float _sorPos, bool _isCollision, const SolverConfig& config )
{
    float velImpulseFit = velocityImpulseRegression.testFitNextDataPointSecondOrder( _velocityStage.impulse );
    float posImpulseFit = positionImpulseRegression.testFitNextDataPointSecondOrder( _positionStage.impulse );

    velocityCacheDamping = calculateCacheDampingFactor( velImpulseFit, velocityCacheDamping, config.cacheVStageModulation );
    positionCacheDamping = calculateCacheDampingFactor( posImpulseFit, positionCacheDamping, config.cachePStageModulation );

    if( !_isCollision )
    {
        velocitySor = recomputeSor( _sorVel, velImpulseFit, config.sorConstraintsModulation );
        positionSor = recomputeSor( _sorPos, posImpulseFit, config.sorConstraintsModulation );
    }
    else
    {
        float velImpulseFit = velocityImpulseRegression.testFitNextDataPointZeroOrder( _velocityStage.impulse );
        float posImpulseFit = positionImpulseRegression.testFitNextDataPointZeroOrder( _positionStage.impulse );

        velocitySor = recomputeSor( _sorVel, velImpulseFit, config.sorCollisionsModulation );
        positionSor = recomputeSor( _sorPos, posImpulseFit, config.sorCollisionsModulation );
    }

    velocityImpulse = velocityCacheDamping * _velocityStage.impulse;
    velocityReaction = _velocityStage.reaction;
    positionImpulse = positionCacheDamping * _positionStage.impulse;
    positionReaction = _positionStage.reaction;

    velocityImpulseRegression.addDataPoint( velocityImpulse, 1.0f );
    positionImpulseRegression.addDataPoint( positionImpulse, 1.0f );
}

void ConstraintCache::serialize(  DebugSerializer& s ) const
{
    s & velocityImpulse & velocityReaction & velocitySor & velocityCacheDamping & positionImpulse & positionReaction & positionSor & positionCacheDamping & velocityImpulseRegression & positionImpulseRegression;
}

void MovingRegression::serialize( DebugSerializer& s ) const
{
    s & confidence & lastPoint & lastTangent & lastCurvature;
}

void ConstraintVariables::serialize( DebugSerializer& s ) const
{
    s & minImpulseValue & maxImpulseValue & impulse & reaction;
}

}
