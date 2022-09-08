#include "stdafx.h"
#include "solver/SolverConfig.h"

FASTINTVARIABLE(PGSPenetrationMarginMax, 50000) //0.05f
FASTINTVARIABLE(PGSPenetrationMarginMin, 100) //0.0001f
FASTINTVARIABLE(PGSPenetrationMarginMaxBump, 5) //5%
FASTINTVARIABLE(PGSPenetrationResolutionDamping, 7) //0.7
FASTINTVARIABLE(PGSPenetrationVelocityForMinMargin, 20) // 20.0f
FASTINTVARIABLE(PGSAlign2AxesCorrectionDamping, 10) //1.0
FASTINTVARIABLE(PGSBallInSocketCorrectionDamping, 10) //1.0


namespace RBX
{

SolverConfig::SolverConfig( Type type )
{
    // Kernel
    pgsIterations = 20;

    // Collisions
    collisionRestitutionThreshold = 1.0f;
    collisionPenetrationMargin = 0.05f;
    collisionPenetrationMarginMax = float(FInt::PGSPenetrationMarginMax)/1000000.0f;
    collisionPenetrationMarginMin = float(FInt::PGSPenetrationMarginMin)/1000000.0f;
    collisionPenetrationVelocityForMinMargin = float(FInt::PGSPenetrationVelocityForMinMargin);
    collisionPenetrationMarginMaxBumpProportions = float(FInt::PGSPenetrationMarginMaxBump)/100.0f;
    collisionPenetrationResolutionDamping = float(FInt::PGSPenetrationResolutionDamping)/10.0f;

    collisionFrictionStaticToDynamicThreshold = 1.0f;
    collisionFrictionStaticScale = 1.0f;
    collisionFrictionDynamicScale = 1.0f;

    // Align2Axes
    align2AxesFrictionConstant = 0.02f;
    align2AxesPositionStageFrictionConstant = 0.01f;
    align2AxesMaxCorrectiveAngle = 3.0f;
    align2AxesCorrectionDamping = float(FInt::PGSAlign2AxesCorrectionDamping)/10.0f;

    // BallInSocket
    ballInSocketMaxCorrectionByStabilization = 0.3f;
    ballInSocketCorrectionDamping = float(FInt::PGSBallInSocketCorrectionDamping)/10.0f;

    // SOR modulation - constraints
    sorConstraintsModulation.thresholdMax = 1.0f;
    sorConstraintsModulation.thresholdMin = 0.01f;
    sorConstraintsModulation.aggressiveValue = 1.5f;
    sorConstraintsModulation.conservativeValue = 0.75f;
    sorConstraintsModulation.easingUpToAggressive = 0.0001f;
    sorConstraintsModulation.easingDownToConservative = 0.2f;

    // SOR modulation - Collisions
    sorCollisionsModulation.thresholdMax = 0.05f;
    sorCollisionsModulation.thresholdMin = 0.01f;
    sorCollisionsModulation.aggressiveValue = 1.9f;
    sorCollisionsModulation.conservativeValue = 1.0f;
    sorCollisionsModulation.easingUpToAggressive = 0.0001f;
    sorCollisionsModulation.easingDownToConservative = 0.2f;

    // Stabilization mass properties
    stabilizationMassReductionPower = 0.3f;
    stabilizationInertiaScale = 0.05f;

    // Impulse cache damping
    cacheVStageModulation.thresholdMin = 0.5f;
    cacheVStageModulation.thresholdMax = 2.0f;
    cacheVStageModulation.conservativeValue = 0.93f;
    cacheVStageModulation.aggressiveValue= 0.99f;
    cacheVStageModulation.easingDownToConservative = 0.01f;
    cacheVStageModulation.easingUpToAggressive = 0.0002f;

    cachePStageModulation.thresholdMin = 0.5f;
    cachePStageModulation.thresholdMax = 1.5f;
    cachePStageModulation.conservativeValue = 0.70f;
    cachePStageModulation.aggressiveValue = 0.99f;
    cachePStageModulation.easingDownToConservative = 0.01f;
    cachePStageModulation.easingUpToAggressive = 0.0002f;

	// Enable/disable caching
    velCacheDamping = 1.0f;
    posCacheDamping = 1.0f;
    constraintCachingEnabled = true;

    // Integration

    // Old damping was 0.99621 per step. Using d = -ln( 0.99621 ) / dt, where dt = 1/240, we get:
    angularDamping = 0.911328f;
    updateSimBodies = true;
    integrateOnlyPositions = false;

    // Block PGS
    blockPGSEnabled = true;

    // Virtual masses
    virtualMassesEnabled = true;

    // SOR
    velocityStageSOREnabled = true;
    positionStageSOREnabled = true;

    // Sim islands
    useSimIslands = false;

    // Inconsistency detector
    inconsistentConstraintDetectorEnabled = false;
    inconsistentConstraintMaxIterations = 0;
    inconsistentConstraintBallInSocketResidualThreshold = 0.02f;
    inconsistentConstraintAlign2AxesThreshold = 0.003f;
    inconsistentConstraintCollisionThreshold = 0.02f;
    inconsistentConstraintCollisionBaseThreshold = 0.001f;
    inconsistentConstraintDeltaThreshold = 0.0001f;

    // Change up things for inconsistency detector
    if( type == Type_InconsistencyDetector )
    {
        inconsistentConstraintDetectorEnabled = true;
        inconsistentConstraintMaxIterations = 100000;
        pgsIterations = 100;

        // Disable caching
        posCacheDamping = 0.0f;
        velCacheDamping = 0.0f;
        constraintCachingEnabled = false;

        // Disable block PGS
        blockPGSEnabled = false;

        // Disable SOR
        velocityStageSOREnabled = false;
        positionStageSOREnabled = false;

        // Disable friction
        collisionFrictionStaticScale = 0.0f;
        collisionFrictionDynamicScale = 0.0f;

        // Don't update body positions
        updateSimBodies = false;

        // Mass Stabilization
        stabilizationMassReductionPower = 0.1f;

        // Sim islands
        useSimIslands = true;
    }
    printConvergenceDiagnostics = false;

    if( type == Type_PositionalCorrection )
    {
        // Disable caching
        posCacheDamping = 0.0f;
        velCacheDamping = 0.0f;
        constraintCachingEnabled = false;

        // Disable block PGS
        blockPGSEnabled = false;

        // Disable SOR
        velocityStageSOREnabled = false;
        positionStageSOREnabled = false;

        // Disable friction
        collisionFrictionStaticScale = 0.0f;
        collisionFrictionDynamicScale = 0.0f;

        integrateOnlyPositions = true;
    }
}

}
