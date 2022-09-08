#pragma once

#define ENABLE_SOR_CONSTRAINTS
#define ENABLE_SOR_COLLISIONS
#define ENABLE_LOCAL_SOR_MODULATION
#define ENABLE_HINGE_FRICTION
#define ENABLE_IMPULSE_CACHE_DAMPING_PER_EQUATION
//#define ENABLE_SOLVER_PROFILER
#define ENABLE_SOLVER_DEBUG_SERIALIZER
//#define MIN_NORM_REPROJECT
//#define PGS_MIN_NORM

//#define DISABLE_ANGULAR_CONSTRAINTS

namespace RBX
{

class SolverConfig
{
public:
    enum Type
    {
        Type_Default,
        Type_InconsistencyDetector,
        Type_PositionalCorrection,
    };
    SolverConfig( Type type = Type_Default );

    //
    // Kernel
    //
    unsigned pgsIterations;

    //
    // Collisions
    //

    // Minimum normal velocity for restitution to be applied
    float collisionRestitutionThreshold;

    // Ignore penetrations that are smaller than this parameter
    float collisionPenetrationMargin;

    // Params for variable penetration margin
    float collisionPenetrationMarginMax;
    float collisionPenetrationMarginMin;

    // The max height of a bump that a rolling object (sphere) can have, in proportion to it's size, due to hitting an edge between two primitives.
    float collisionPenetrationMarginMaxBumpProportions;

    // Damping of the penetration resolution
    float collisionPenetrationResolutionDamping;

    // The velocity at which the penetration allowed will be minimum
    float collisionPenetrationVelocityForMinMargin;

    // Threshold tangential velocity between static and dynamic friction
    float collisionFrictionStaticToDynamicThreshold;

    // Tunning constant for static friction
    float collisionFrictionStaticScale;

    // Tunning constant for dynamic friction
    float collisionFrictionDynamicScale;

    //
    // Align2Axes Constraint
    //

    // Angular friction velocity stage
    float align2AxesFrictionConstant;

    // Angular friction position stage
    float align2AxesPositionStageFrictionConstant;

    // Maximum angle for angular correction in degrees
    float align2AxesMaxCorrectiveAngle;

    float align2AxesCorrectionDamping;

    //
    // BallInSocket
    //

    // Maximum corrective distance
    float ballInSocketMaxCorrectionByStabilization;

    float ballInSocketCorrectionDamping;

    //
    // SOR Modulation
    //

    // Create a common structure for these...
    // Constraints
    struct ModulationParams
    {
        float thresholdMax;
        float thresholdMin;
        float aggressiveValue;
        float conservativeValue;
        float easingUpToAggressive;
        float easingDownToConservative;
    };

    ModulationParams sorConstraintsModulation;
    ModulationParams sorCollisionsModulation;
    ModulationParams cacheVStageModulation;
    ModulationParams cachePStageModulation;

    //
    // Stabilization
    //
    float stabilizationMassReductionPower;
    float stabilizationInertiaScale;

    //
    // Cache
    //
    float velCacheDamping;
    float posCacheDamping;
    bool constraintCachingEnabled;

    //
    // Integration
    //
    float angularDamping;
    bool updateSimBodies;
    bool integrateOnlyPositions;

    //
    // Block PGS
    //
    bool blockPGSEnabled;

    //
    // SOR
    //
    float velocityStageSOREnabled;
    float positionStageSOREnabled;

    //
    // Virtual masses
    //
    bool virtualMassesEnabled;

    //
    // Use sim islands
    //
    bool useSimIslands;

    //
    // Conflicting constraints detector
    //
    bool inconsistentConstraintDetectorEnabled;
    unsigned inconsistentConstraintMaxIterations;

    float inconsistentConstraintBallInSocketResidualThreshold;
    float inconsistentConstraintDeltaThreshold;
    float inconsistentConstraintAlign2AxesThreshold;
    float inconsistentConstraintCollisionThreshold;
    float inconsistentConstraintCollisionBaseThreshold;
    bool printConvergenceDiagnostics;
};

}
