#include "stdafx.h"

#include "solver/Solver.h"
#include "solver/SolverKernel.h"
#include "solver/SolverSerializer.h"
#include "solver/SolverProfiler.h"

#include "G3D/Matrix3.h"
#include "G3D/Vector3.h"
#include "v8world/RotateJoint.h"
#include "v8world/Primitive.h"
#include "v8kernel/Body.h"
#include "v8kernel/ContactConnector.h"

#include "RbxAssert.h"
#include "FastLog.h"
#include "g3d/g3dmath.h"
#include "rbx/ArrayDynamic.h"

#include "boost/functional/hash.hpp"

#include "rbx/Profiler.h"

FASTFLAGVARIABLE(PGSSolverFileDump, false)
DYNAMIC_FASTFLAGVARIABLE(PGSSolverSimIslandsEnabled, false)
DYNAMIC_FASTFLAGVARIABLE(PGSSolverUsesIslandizableCode, false)
DYNAMIC_FASTFLAGVARIABLE(PGSSolverIntegrateOnlyPositionsEnabled, false)
FASTFLAG(PhysicsAnalyzerEnabled)

namespace RBX
{

class ContactManifold 
{
public:
    ContactManifold(): referenceCount( 0 ) { }
    ~ContactManifold();

    ArrayDynamic< ConstraintCollision* > collisions;
    int referenceCount;

private:
    ContactManifold( const ContactManifold& );
    ContactManifold& operator=( const ContactManifold& );
};

// Given a pointer to Body, return an index to it's position in the body list
// If it doesn't exists in the body list, add it to the end.
// Those SimBodies that aren't in the indexation are necessarily anchored.
static RBX_SIMD_INLINE int getBodyIndex( 
    ArrayDynamic< SimBody* >& _anchoredBodyList,
    BodyIndexation& _allBodyIndexation,
    Body* _body )
{
    SimBody* simBody = NULL;
    if( _body != NULL )
    {
        simBody = _body->getRootSimBody();
    }
    auto it = _allBodyIndexation.find( simBody );

    int index = 0;
    // A new body that we haven't seen previously. Add it to the list of anchored bodies and add its index
    if( it == NULL )
    {
        // We can assert that it is anchored...
        index = (int)_allBodyIndexation.size();
        _allBodyIndexation[ simBody ] = index;
        _anchoredBodyList.push_back( simBody );
    }
    else
    {
        index = *it;
    }
    return index;
}

//
// From the constraint list extract information and lay it out in arrays:
// - indices of constrained body pairs
// - a list of anchored bodies interacting constrained/colliding with at least one simulated body
// - an indexation of SimBodies
// - constraint block size and offset to the constraint blocks
// - total size of the diagonal symmetric blocks
//
static boost::uint32_t gatherConstraintPairData( ArrayBase< BodyPairIndices >& _constrainedSimBodyPairs,
                                               ArrayBase< boost::uint8_t >& _dimensions, 
                                               ArrayBase< boost::uint32_t >& _offsets,
                                               ArrayDynamic< SimBody* >& _anchoredBodyList,
                                               BodyIndexation& _bodyIndexation,
                                               boost::uint32_t& _totalBlockSize,
                                               boost::uint32_t& _collisionCount,
                                               const ArrayBase< Constraint* >& _constraints )
{
    RBXPROFILER_SCOPE("Physics", "gatherConstraintPairData");

    boost::uint32_t totalDimension = 0;
    _totalBlockSize = 0;
    _collisionCount = 0;
    for( size_t i = 0; i < _constraints.size(); i++ )
    {
        boost::uint8_t dimension = _constraints[ i ]->getDimension();
        _dimensions[ i ] = dimension;
        _offsets[ i ] = totalDimension;
        totalDimension += dimension;
        _totalBlockSize += ( dimension * ( dimension + 1 ) ) / 2;

        int indexA = getBodyIndex( _anchoredBodyList, _bodyIndexation, _constraints[ i ]->getBodyA() );
        int indexB = getBodyIndex( _anchoredBodyList, _bodyIndexation, _constraints[ i ]->getBodyB() );
        _constrainedSimBodyPairs[ i ] = BodyPairIndices( indexA, indexB );
        if( _constraints[ i ]->getType() == Constraint::Types_Collision )
        {
            _collisionCount++;
        }
    }

    return totalDimension;
}

static RBX_SIMD_INLINE void integrateVelocities( SolverBodyDynamicProperties& _variableData, const SolverBodyMassAndInertia& _massAndInertia, const Vector3& _externalImpulse, const Vector3& _externalAngularImpulse, float _dt, const SolverConfig& _config )
{
    // Integrate linear velocity
    _variableData.integratedLinearVelocity = _variableData.linearVelocity + _massAndInertia.massInvVelStage * _externalImpulse;
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _variableData.integratedLinearVelocity ) );

    // Integrate angular velocity
    SymmetricMatrix invInertia;
    invInertia.diagonals = _massAndInertia.inertiaDiagonal;
    invInertia.offDiagonals = _massAndInertia.inertiaOffDiagonal;
    _variableData.integratedAngularVelocity = expf( -_config.angularDamping * _dt ) * _variableData.angularVelocity + invInertia * _externalAngularImpulse;
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _variableData.integratedAngularVelocity ) );
}

static inline float fudgeMassForPosStage( float massInv, const SolverConfig& _config )
{
    return powf( massInv, _config.stabilizationMassReductionPower );
}

//
// Init internal structures for sim bodies, and integrate velocities
//
static void integrateVelocitiesAndInitSimulatedObjects( ArrayDynamic< SolverBodyStaticProperties >& _bodyStaticData, 
                                                       ArrayDynamic< SolverBodyDynamicProperties >& _bodyVariableData, 
                                                       ArrayDynamic< SolverBodyMassAndInertia >& _massAndInertia,
                                                       ArrayDynamic< float >& _effectiveMassMultipliers,
                                                       const ArrayBase< SimBody* >& _simBodies, float _dt, const SolverConfig& _config )
{
    RBXPROFILER_SCOPE("Physics", "integrateVelocitiesAndInitSimulatedObjects");

    for( size_t i = 0; i < _simBodies.size(); i++ )
    {
        SimBody* body = _simBodies[ i ];
        body->updateIfDirty();

        SolverBodyDynamicProperties variableData;
        variableData.angularVelocity = body->getPV().velocity.rotational;
        variableData.linearVelocity = body->getPV().velocity.linear;
        variableData.orientation = body->getPV().position.rotation;
        variableData.position = body->getPV().position.translation;
        Vector3 externalImpulse = body->getForce() * _dt + body->getImpulse();
        Vector3 externalAngularImpulse = body->getTorque() * _dt + body->getRotationallmpulse();

        Matrix3 inertiaInv = body->getInverseInertiaInWorld();
        SolverBodyStaticProperties staticData;
        SolverBodyMassAndInertia massAndInertia;
        staticData.isStatic = false;
        massAndInertia.inertiaDiagonal = Vector3( inertiaInv[0][0], inertiaInv[1][1], inertiaInv[2][2] );
        massAndInertia.inertiaOffDiagonal = Vector3( inertiaInv[0][1], inertiaInv[0][2], inertiaInv[1][2] );
        massAndInertia.massInvVelStage = body->getMassRecip();
        float massInversePosStage = fudgeMassForPosStage( massAndInertia.massInvVelStage, _config );
        if( massAndInertia.massInvVelStage > 0.0f )
        {
            massAndInertia.posToVelMassRatio = massInversePosStage / massAndInertia.massInvVelStage;
        }
        else
        {
            massAndInertia.posToVelMassRatio = 0.0f;
        }
        staticData.bodyUID = body->getUID();
        
        staticData.guid = body->getBody()->getGuidIndex();
        _bodyStaticData.push_back( staticData );
        _massAndInertia.push_back( massAndInertia );

        integrateVelocities( variableData, massAndInertia, externalImpulse, externalAngularImpulse, _dt, _config );
        _bodyVariableData.push_back( variableData );

        _effectiveMassMultipliers.push_back( 1.0f );
    }

    if( DFFlag::PGSSolverIntegrateOnlyPositionsEnabled && _config.integrateOnlyPositions )
    {
        for( auto b : _bodyVariableData )
        {
            b.linearVelocity = Vector3(0.0f);
            b.angularVelocity = Vector3(0.0f);
            b.integratedLinearVelocity = Vector3(0.0f);
            b.integratedAngularVelocity = Vector3(0.0f);
        }
    }
}

//
// Initialize internal structures for anchored bodies that are being interacted with
//
void PGSSolver::initAnchoredObjects( 
    ArrayDynamic< SolverBodyDynamicProperties >& _dynamicProps,
    ArrayDynamic< SolverBodyStaticProperties >& _staticProps,
    ArrayDynamic< SolverBodyMassAndInertia >& _massAndInertia,
    ArrayDynamic< float >& _effectiveMassMultipliers,
    VirtualDisplacementArray& _velStageVirDisplacements,
    VirtualDisplacementArray& _posStageVirDisplacements,
    int offsetToAnchoredObjects,
    const ArrayBase< SimBody* >& _anchoredBodyList,
    const SolverConfig& _config ) const
{
    RBXPROFILER_SCOPE("Physics", "initAnchoredObjects");

    for( int i = 0; i < (int)_anchoredBodyList.size(); i++ )
    {
        auto body = _anchoredBodyList[ i ];
        if( body != NULL )
        {
            body->updateIfDirty();
        }

        SolverBodyDynamicProperties dynamicProps;
        SolverBodyStaticProperties staticProps;
        SolverBodyMassAndInertia massAndInertia;
        VirtualDisplacement velStageVirD;
        VirtualDisplacement posStageVirD;
        staticProps.isStatic = true;
        boost::uint64_t uid = body != NULL ? body->getUID() : 0;
        auto bodyCacheIt = bodyCache.find( uid );
        if( ( body != NULL ) && ( bodyCacheIt != bodyCache.end() ) && _config.virtualMassesEnabled )
        {
            const SolverBodyCache& bodyCache = bodyCacheIt->second;
            dynamicProps.linearVelocity = bodyCache.linearVelocity;
            dynamicProps.angularVelocity = bodyCache.angularVelocity;
            dynamicProps.integratedLinearVelocity = bodyCache.integratedLinearVelocity;
            dynamicProps.integratedAngularVelocity = bodyCache.integratedAngularVelocity;

            velStageVirD = VirtualDisplacement( simd::load3( &bodyCache.virDVelStageLin.x ), simd::load3( &bodyCache.virDVelStageAng.x ) );
            posStageVirD = VirtualDisplacement( simd::load3( &bodyCache.virDPosStageLin.x ), simd::load3( &bodyCache.virDPosStageAng.x ) );

            Matrix3 inertiaInv = body->getInverseInertiaInWorld();
            massAndInertia.inertiaDiagonal = Vector3( inertiaInv[0][0], inertiaInv[1][1], inertiaInv[2][2] );
            massAndInertia.inertiaOffDiagonal = Vector3( inertiaInv[0][1], inertiaInv[0][2], inertiaInv[1][2] );
            massAndInertia.massInvVelStage = body->getMassRecip();
            float massInversePosStage = fudgeMassForPosStage(massAndInertia.massInvVelStage, _config);
            if( massAndInertia.massInvVelStage > 0.0f )
            {
                massAndInertia.posToVelMassRatio = massInversePosStage / massAndInertia.massInvVelStage;
            }
            else
            {
                massAndInertia.posToVelMassRatio = 0.0f;
            }
        }
        else
        {
            Vector3 zero( 0.0f, 0.0f, 0.0f );

            if( body != NULL )
            {
                // Conveyor belt
                dynamicProps.linearVelocity = body->getBodyConst()->getPvFast().velocity.linear;
                dynamicProps.angularVelocity = body->getBodyConst()->getPvFast().velocity.rotational;
            }
            else
            {
                dynamicProps.linearVelocity = zero;
                dynamicProps.angularVelocity = zero;
            }
            dynamicProps.integratedLinearVelocity = dynamicProps.linearVelocity;
            dynamicProps.integratedAngularVelocity = dynamicProps.integratedAngularVelocity;

            velStageVirD.reset();
            posStageVirD.reset();

            massAndInertia.inertiaDiagonal = zero;
            massAndInertia.inertiaOffDiagonal = zero;
            massAndInertia.massInvVelStage = 0.0f;
            massAndInertia.posToVelMassRatio = 0.0f;
        }
        
        if( body != NULL )
        {
            dynamicProps.orientation = body->getPV().position.rotation;
            dynamicProps.position = body->getPV().position.translation;
        }
        else
        {
            dynamicProps.orientation = dynamicProps.orientation.identity();
            dynamicProps.position = Vector3(0.0f, 0.0f, 0.0f);
        }

        staticProps.guid = 0;
        staticProps.bodyUID = uid;
        if( body != NULL )
        {
            staticProps.guid = body->getBody()->getGuidIndex();
        }

        _dynamicProps.push_back(dynamicProps);
        _staticProps.push_back(staticProps);
        _massAndInertia.push_back(massAndInertia);
        _effectiveMassMultipliers.push_back( 0.0f );
        _velStageVirDisplacements[ offsetToAnchoredObjects + i ] = velStageVirD;
        _posStageVirDisplacements[ offsetToAnchoredObjects + i ] = posStageVirD;
    }
}

static RBX_SIMD_INLINE void integratePositions( SolverBodyDynamicProperties& _dynamicProps, const SolverBodyStaticProperties& _staticProps, 
                                                   const VirtualDisplacementPOD& _velStageVirD, const VirtualDisplacementPOD& _posStageVirD, float _dt )
{
    RBXASSERT_VERY_FAST(!RBX::Math::isNanInfVector3(_velStageVirD.lin));
    RBXASSERT_VERY_FAST(!RBX::Math::isNanInfVector3(_velStageVirD.ang));
    RBXASSERT_VERY_FAST(!RBX::Math::isNanInfVector3(_posStageVirD.lin));
    RBXASSERT_VERY_FAST(!RBX::Math::isNanInfVector3(_posStageVirD.ang));

    // Applying constraint impulses
    _dynamicProps.linearVelocity = _dynamicProps.integratedLinearVelocity + _velStageVirD.lin;
    _dynamicProps.angularVelocity = _dynamicProps.integratedAngularVelocity + _velStageVirD.ang;

    // Applying error correction impulse
    _dynamicProps.position += _posStageVirD.lin;
    Matrix3 changeInOrientation = Matrix3::fillRotation( _posStageVirD.ang );
    _dynamicProps.orientation = changeInOrientation * _dynamicProps.orientation;

    // Integrate positions
    _dynamicProps.position += _dynamicProps.linearVelocity * _dt;

    // Integrate orientation
    Vector3 infinitessimalRotation = _dynamicProps.angularVelocity * _dt;
    changeInOrientation = Matrix3::fillRotation( infinitessimalRotation );
    _dynamicProps.orientation = changeInOrientation * _dynamicProps.orientation;

    _dynamicProps.orientation.orthonormalize();
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _dynamicProps.position ) );
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _dynamicProps.orientation.row(0) ) );
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _dynamicProps.orientation.row(1) ) );
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _dynamicProps.orientation.row(2) ) );
}

void PGSSolver::integratePositionsAndUpdateSimBodies( 
    SimBody* const * _simBodies, 
    SolverBodyDynamicProperties* const _bodyVariableData,
    const SolverBodyStaticProperties* const _bodyStaticData, 
    size_t _simBodyCount,
    const VirtualDisplacementArray& _velocityDeltas,
    const VirtualDisplacementArray& _positionDeltas,
    float _dt )
{
    RBXPROFILER_SCOPE("Physics", "integratePositionsAndUpdateSimBodies");

    for( int index = 0; index < (int)_simBodyCount; index++ )
    {
        integratePositions( _bodyVariableData[ index ], _bodyStaticData[ index ], _velocityDeltas[ index ], _positionDeltas[ index ], _dt );

        SolverBodyCache& cache = bodyCache[ _simBodies[ index ]->getUID() ];
        cache.virDPosStageLin = _positionDeltas[ index ].lin;
        cache.virDPosStageAng = _positionDeltas[ index ].ang;
        cache.virDVelStageLin = _velocityDeltas[ index ].lin;
        cache.virDVelStageAng = _velocityDeltas[ index ].ang;
        cache.linearVelocity = _simBodies[ index ]->getPV().velocity.linear; //  _bodyVariableData[ index ].linearVelocity;
        cache.angularVelocity = _simBodies[ index ]->getPV().velocity.rotational; // _bodyVariableData[ index ].angularVelocity;
        cache.integratedLinearVelocity =  _bodyVariableData[ index ].integratedLinearVelocity;
        cache.integratedAngularVelocity =  _bodyVariableData[ index ].integratedAngularVelocity;
        cache.simBodyDebug = _simBodies[ index ];

        _simBodies[ index ]->updateFromSolver( 
            _bodyVariableData[ index ].position, _bodyVariableData[ index ].orientation, 
            _bodyVariableData[ index ].linearVelocity, _bodyVariableData[ index ].angularVelocity );
    }
}

static RBX_SIMD_INLINE void integratePositionsIgnoreVelocities( SolverBodyDynamicProperties& _dynamicProps, const SolverBodyStaticProperties& _staticProps, 
                                               const VirtualDisplacementPOD& _posStageVirD, float _dt )
{
    RBXASSERT_VERY_FAST(!RBX::Math::isNanInfVector3(_posStageVirD.lin));
    RBXASSERT_VERY_FAST(!RBX::Math::isNanInfVector3(_posStageVirD.ang));

    // Applying error correction impulse
    _dynamicProps.position += _posStageVirD.lin;
    Matrix3 changeInOrientation = Matrix3::fillRotation( _posStageVirD.ang );
    _dynamicProps.orientation = changeInOrientation * _dynamicProps.orientation;

    _dynamicProps.orientation.orthonormalize();
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _dynamicProps.position ) );
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _dynamicProps.orientation.row(0) ) );
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _dynamicProps.orientation.row(1) ) );
    RBXASSERT_VERY_FAST( !RBX::Math::isNanInfVector3( _dynamicProps.orientation.row(2) ) );
}

void PGSSolver::integratePositionsIgnoreVelocitiesAndUpdateSimBodies( 
    SimBody* const * _simBodies, 
    SolverBodyDynamicProperties* const _bodyVariableData,
    const SolverBodyStaticProperties* const _bodyStaticData, 
    size_t _simBodyCount,
    const VirtualDisplacementArray& _positionDeltas,
    float _dt )
{
    RBXPROFILER_SCOPE("Physics", "integratePositionsIgnoreVelocitiesAndUpdateSimBodies");

    for( int index = 0; index < (int)_simBodyCount; index++ )
    {
        integratePositionsIgnoreVelocities( _bodyVariableData[ index ], _bodyStaticData[ index ], _positionDeltas[ index ], _dt );

        _simBodies[ index ]->updateFromSolver( _bodyVariableData[ index ].position, _bodyVariableData[ index ].orientation, Vector3(0.0f), Vector3(0.0f) );
    }
}

static const int sampleCount = 240;
PGSSolver::PGSSolver(): constraintUIDGenerator( 0 ),
    inconsistentConstraintDetectorEnabled( false ),
    physicsAnalyzerBreakOnIssue( true ),
    gatherCollisionsProfiler( sampleCount, "  PGS Gather collisions & constraints: %.3fms" ),
    islandSplitProfiler( sampleCount, "  PGS Split into islands: %.3fms"),
    integrateVelocitiesProfiler( sampleCount, "  PGS Integrate velocities: %.3fms" ),
    initAnchoredBodiesProfiler( sampleCount, "  PGS Init anchored bodies: %.3fms" ),
    buildEquationsProfiler( sampleCount, "  PGS Build equations: %.3fms" ),
    computeEffectiveMassesProfiler( sampleCount, "  PGS Compute effective masses: %.3fms" ),
    preconditioningProfiler( sampleCount, "  PGS Preconditioning: %.3fms" ),
    multiplyEffectiveMassMultipliersProfiler( sampleCount, "  PGS Pre-multiply mass multipliers: %.3fms" ),
    initVirDProfiler( sampleCount, "  PGS Init Virtual Displacements: %.3fms" ),
    kernelProfiler( sampleCount, "  PGS Kernel: %.3fms" ),
    integratePositionsProfiler( sampleCount, "  PGS Integrate positions: %.3fms" ),
    writeCacheProfiler( sampleCount, "  PGS Write constraint cache: %.3fms" ),
    solverProfiler( sampleCount, "PGS Solver total: %.2fms" ),
    dumpLogSwitch( false ),
    userId( 0 )
{
}


void PGSSolver::detectInconsistentConstraints( 
    VirtualDisplacementArray& _positionDeltas,
    ArrayBase< ConstraintVariables >& _positionStage,
    const ArrayBase< ConstraintJacobianPair >& _jacobians,
    const ArrayBase< ConstraintJacobianPair >& _preconditionedJacobiansPosStage,
    const ArrayBase< EffectiveMassPair >& _effectiveMassesPosStage,
    const ArrayBase< float >& _sorPos,
    const ArrayBase< boost::uint8_t >& _dimensions,
    const ArrayBase< boost::uint32_t >& _offsets,
    const ArrayBase< BodyPairIndices >& _simBodyPairs,
    const ArrayBase< Constraint* >& _constraints,
    size_t _collisionCount,
    const SolverConfig& _solverConfig )
{
    ArrayDynamic< float > impulseDeltas( _positionStage.size() );
    impulseDeltas.assign(_positionStage.size(), 0.0f);
    ArrayDynamic< float > impulseDeltaDeltas( _positionStage.size() );
    impulseDeltaDeltas.assign(_positionStage.size(), 0.0f);

    ArrayDynamic< float > displacements( _positionStage.size() );
    ArrayDynamic< float > residuals( _positionStage.size() );
    ArrayDynamic< float > deltaResiduals( _positionStage.size() );

    _positionDeltas.reset();

//#define ENABLE_INCONSISTENT_CONSTRAINT_DETECTOR_DEBUG

#ifdef ENABLE_INCONSISTENT_CONSTRAINT_DETECTOR_DEBUG
    std::stringstream out;
    out << "Progression: ";
#endif

    static int minSteadyIterations = 5;
    int divergingIterations = 0;
    size_t divergingConstraints = 0;
    size_t convergingConstraints = 0;

    double oldEnergy = 0.0f;
    
    double energy = 0.0f;
    double dEnergy = 0.0f;
    double d2Energy = 0.0f;
    int iteration = 0;
    for( ; ; iteration++ )
    {
        // Run the PGS for a few iterations
        PGSSolveKernelComputeErrors( impulseDeltas, impulseDeltaDeltas, 
            _positionStage, 
            _positionDeltas,
            _constraints.size(), _collisionCount, _positionDeltas.getSize(), 
            _dimensions.data(), _simBodyPairs.data(),
            _jacobians.data(),
            _preconditionedJacobiansPosStage.data(), 
            _effectiveMassesPosStage.data(), _solverConfig );

        oldEnergy = energy;
        energy = 0.0f;
        dEnergy = 0.0f;
        d2Energy = 0.0f;

        // Project the impulses onto displacements
        for( size_t i = 0; i < residuals.size(); i++ )
        {
            simd::v4f d = _jacobians[i].dot(_effectiveMassesPosStage[i]);
            simd::storeSingle( &displacements[i], simd::splat(_positionStage[i].impulse) * d );
            simd::storeSingle( &residuals[i], simd::splat(impulseDeltas[i]) * d );
            simd::storeSingle( &deltaResiduals[i], simd::splat(impulseDeltaDeltas[i]) * d );
            energy += 0.5f * _positionStage[i].impulse * displacements[i];
            dEnergy += _positionStage[i].impulse * residuals[i];
            d2Energy += _positionStage[i].impulse * deltaResiduals[i];
        }

        // Test the convergence of the constraints
        divergingConstraints = 0;
        convergingConstraints = 0;
        for( size_t i = 0; i < _constraints.size(); i++ )
        {
            boost::uint32_t offset = _offsets[ i ];
            switch( _constraints[ i ]->testPGSConvergence(displacements.data() + offset, residuals.data() + offset, deltaResiduals.data() + offset, _solverConfig) )
            {
            case Constraint::Convergence_Converges:
                convergingConstraints++;
                break;
            case Constraint::Convergence_Diverges:
                divergingConstraints++;
                break;
            default:
                break;
            }
        }

        if( convergingConstraints == _constraints.size() )
        {
#ifdef ENABLE_INCONSISTENT_CONSTRAINT_DETECTOR_DEBUG
            out << "Converged! ";
#endif
            break;
        }
        if( divergingConstraints + convergingConstraints == _constraints.size() )
        {
            divergingIterations++;
            if( divergingIterations == minSteadyIterations )
            {
#ifdef ENABLE_INCONSISTENT_CONSTRAINT_DETECTOR_DEBUG
                out << "Diverged! ";
#endif
                break;
            }
        }
        else
        {
            divergingIterations = 0;
        }
        if( ( iteration+1 ) * _solverConfig.pgsIterations >= _solverConfig.inconsistentConstraintMaxIterations )
        {
#ifdef ENABLE_INCONSISTENT_CONSTRAINT_DETECTOR_DEBUG
            out << "Undetermined! ";
#endif
            break;
        }
    }

#ifdef ENABLE_INCONSISTENT_CONSTRAINT_DETECTOR_DEBUG
    if( divergingConstraints + convergingConstraints != _constraints.size() )
    {
        if( divergingConstraints >= convergingConstraints / 2 )
        {
            out << "Probably diverging";
        }
        else
        {
            out << "Probably converging";
        }
    }
#endif

#ifdef ENABLE_INCONSISTENT_CONSTRAINT_DETECTOR_DEBUG
    float dEnergyAverage = ( energy - oldEnergy ) / _solverConfig.pgsIterations;
    unsigned iterations = ( iteration + 1 ) * _solverConfig.pgsIterations;
    out << "Iterations: " << iterations << ". ";
    out << "E = " << energy << ", dE = " << dEnergy << ", gamma = " << 100.0 * dEnergy / dEnergyAverage - 100.0 << "%%, " << 100.0 * d2Energy / dEnergyAverage << "%% ";
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, out.str().c_str() );
#endif

    serializer & residuals & deltaResiduals;

    if( convergingConstraints != _constraints.size() )
    {
        inconsistentBodies.push_back( ArrayDynamic< boost::uint64_t >() );
    }

    boost::unordered_set< boost::uint64_t > alreadyRegisteredBodies;
    for( size_t i = 0; i < _constraints.size(); i++ )
    {
        boost::uint32_t offset = _offsets[ i ];
        Constraint::Convergence convergence = _constraints[ i ]->testPGSConvergence(displacements.data() + offset, residuals.data() + offset, deltaResiduals.data() + offset, _solverConfig);

        if( convergence == Constraint::Convergence_Converges )
        {
            continue;
        }

        if( simBodies.find( _constraints[i]->getBodyA()->getRootSimBody() ) != simBodies.end() 
            && alreadyRegisteredBodies.find( _constraints[i]->getBodyA()->getUID() ) == alreadyRegisteredBodies.end() )
        {
            alreadyRegisteredBodies.insert( _constraints[i]->getBodyA()->getUID() );
            inconsistentBodies.back().push_back( _constraints[i]->getBodyA()->getUID() );
        }
        if( _constraints[i]->getBodyB() && simBodies.find( _constraints[i]->getBodyB()->getRootSimBody() ) != simBodies.end()
            && alreadyRegisteredBodies.find( _constraints[i]->getBodyB()->getUID() ) == alreadyRegisteredBodies.end() )
        {
            alreadyRegisteredBodies.insert( _constraints[i]->getBodyB()->getUID() );
            inconsistentBodies.back().push_back( _constraints[i]->getBodyB()->getUID() );
        }

        InconsistentBodyPair pair;
        pair.convergence = convergence;
        pair.bodyA = _constraints[i]->getBodyA();
        pair.bodyB = _constraints[i]->getBodyB();
        pair.bodyPair.first = _constraints[i]->getBodyA()->getUID();
        pair.bodyPair.second = _constraints[i]->getBodyB()->getUID();
        inconsistentBodyPairs.push_back( pair );
    }
}

template< class T >
class MultiIndexList
{
public:
    MultiIndexList( size_t listCount, size_t storageSize )
    {
        next.assign( storageSize, -1 );
        first.assign( listCount, -1 );
        storage.reserve( storageSize );
    }

    void addToList( size_t list, T value )
    {
        boost::int32_t element = ( boost::int32_t )storage.size();
        storage.push_back( value );
        if( first[ list ] == -1 )
        {
            first[ list ] = element;
            return;
        }
        next[ element ] = first[ list ];
        first[ list ] = element;
    }

    size_t getFirst( size_t list ) const
    {
        return first[ list ];
    }

    size_t getNext( size_t element ) const
    {
        return next[ element ];
    }

    bool isValid( boost::int32_t index ) const
    {
        return index != -1;
    }

    T getValue( boost::int32_t index ) const
    {
        RBXASSERT_VERY_FAST( index != -1 );
        return storage[ index ];
    }

private:
    ArrayDynamic< boost::int32_t > first;
    ArrayDynamic< boost::int32_t > next;
    ArrayDynamic< T > storage;
};

static void addAdjacentBodies( ArrayDynamic< boost::uint32_t >& _islandSimBodies, 
                              ArrayDynamic< boost::uint32_t >& _islandConstraints,
                              ArrayBase< boost::uint8_t >& _visitedBodies,
                              ArrayBase< boost::uint8_t >& _visitedConstraints,
                              ArrayDynamic< boost::int32_t >& _simBodyRecursionStack,
                              boost::int32_t _rootBodyIndex, 
                              const ArrayBase< std::pair< boost::int32_t, boost::int32_t > >& _bodyIndexPairs,
                              const MultiIndexList< boost::uint32_t >& _adjacencyLists )
{
    // Push the root body index onto the stack
    for( _simBodyRecursionStack.push_back( _rootBodyIndex ); _simBodyRecursionStack.size() > 0;)
    {
        boost::int32_t bodyIndex = _simBodyRecursionStack.back();
        _simBodyRecursionStack.pop_back();

        // Iterate over the list of constraints involving this body
        for( size_t it = _adjacencyLists.getFirst( bodyIndex ); 
            _adjacencyLists.isValid( it ); 
            it = _adjacencyLists.getNext( it ) )
        {
            size_t cIndex = _adjacencyLists.getValue( it );

            // Ignore if this constraint has been visited
            if( _visitedConstraints[ cIndex ] )
            {
                continue;
            }
            _visitedConstraints[ cIndex ] = 1;
            _islandConstraints.push_back( cIndex );

            // Push the second body of the constraint onto the stack
            {
                boost::int32_t index = _bodyIndexPairs[ cIndex ].first;
                if( index != -1 && !_visitedBodies[ index ] )
                {
                    _visitedBodies[ index ] = 1;
                    _islandSimBodies.push_back( index );
                    _simBodyRecursionStack.push_back( index );
                }
            }

            {
                boost::int32_t index = _bodyIndexPairs[ cIndex ].second;
                if( index != -1 && !_visitedBodies[ index ] )
                {
                    _visitedBodies[ index ] = 1;
                    _islandSimBodies.push_back( index );
                    _simBodyRecursionStack.push_back( index );
                }
            }
        }
    }
}

static void breakIntoIslands( ArrayDynamic< SimBody* >& _islandSimBodies, 
                             ArrayDynamic< Constraint* >& _islandConstraints,
                             ArrayDynamic< std::pair< boost::uint32_t, boost::uint32_t > >& _islandSizes,
                            const boost::unordered_set< SimBody* >& _simBodies, 
                            const ArrayBase< Constraint* >& _constraints )
{
    RBXPROFILER_SCOPE("Physics", "breakIntoIslands");
    // Index the bodies
    DenseHashMap< SimBody*, boost::int32_t > bodyIndexation( NULL, G3D::ceilPow2( 2 * _simBodies.size() ) );
    ArrayDynamic< SimBody* > simBodyList( _simBodies.size(), ArrayNoInit() );
    boost::int32_t index = 0;
    for( auto b : _simBodies )
    {
        bodyIndexation[ b ] = index;
        simBodyList[ index ] = b;
        index++;
    }

    // Form the adjacency map
    MultiIndexList< boost::uint32_t > adjacencyLists( _simBodies.size(), 2 * _constraints.size() );
    ArrayDynamic< std::pair< boost::int32_t, boost::int32_t > > bodyIndexPairs( _constraints.size(), ArrayNoInit() );
    for( boost::uint32_t i = 0; i < ( boost::uint32_t )_constraints.size(); i++ )
    {
        auto c = _constraints[ i ];
        std::pair< boost::int32_t, boost::int32_t > pair( -1, -1 );
        boost::int32_t* indexA = bodyIndexation.find( c->getBodyA()->getRootSimBody() );
        if( indexA != NULL )
        {
            adjacencyLists.addToList( *indexA, i );
            pair.first = *indexA;
        }

        if( c->getBodyB() != NULL )
        {
            boost::int32_t* indexB = bodyIndexation.find( c->getBodyB()->getRootSimBody() );
            if( indexB != NULL )
            {
                adjacencyLists.addToList( *indexB, i );
                pair.second = *indexB;
            }
        }
        bodyIndexPairs[ i ] = pair;
    }

    // Reserve for the worst case scenario
    _islandSimBodies.reserve( _simBodies.size() );
    _islandConstraints.reserve( _constraints.size() );
    _islandSizes.reserve( _simBodies.size() );

    // Temporary buffers
    // These are all the memory allocations necessary
    ArrayDynamic< boost::uint8_t > visitedBodies;
    visitedBodies.assign( _simBodies.size(), 0 );
    ArrayDynamic< boost::uint8_t > visitedConstraints;
    visitedConstraints.assign( _constraints.size(), 0 );
    ArrayDynamic< boost::uint32_t > indexedIslandConstraints;
    indexedIslandConstraints.reserve( _constraints.size() );
    ArrayDynamic< boost::uint32_t > indexedIslandBodies;
    indexedIslandBodies.reserve( _simBodies.size() );
    ArrayDynamic< boost::int32_t > tempBuffer;
    tempBuffer.reserve( _simBodies.size() );
    
    for( boost::uint32_t bodyIndex = 0; bodyIndex < simBodyList.size(); bodyIndex++ )
    {
        if( !visitedBodies[ bodyIndex ] )
        {
            visitedBodies[ bodyIndex ] = 1;

            indexedIslandBodies.push_back( bodyIndex );
            addAdjacentBodies( indexedIslandBodies, indexedIslandConstraints, visitedBodies, visitedConstraints, tempBuffer, bodyIndex, bodyIndexPairs, adjacencyLists );

            // Store the island size
            _islandSizes.push_back( std::pair< boost::uint32_t, boost::uint32_t >( indexedIslandBodies.size(), indexedIslandConstraints.size() ) );

            // Sort and add constraints
            std::sort( indexedIslandConstraints.begin(), indexedIslandConstraints.end() );
            for( auto p : indexedIslandConstraints )
            {
                _islandConstraints.push_back( _constraints[ p ] );
            }

            // Add the bodies
            for( auto p : indexedIslandBodies )
            {
                _islandSimBodies.push_back( simBodyList[ p ] );
            }

            // Cleanup
            indexedIslandBodies.clear();
            indexedIslandConstraints.clear();
        }
    }
}

void PGSSolver::solve( const std::vector< ContactConnector* >& _contactConnectors, float _dt, boost::uint64_t debugTime, bool _throttled )
{
    inconsistentBodyPairs.clear();
    inconsistentBodies.clear();

    if( FFlag::PhysicsAnalyzerEnabled && inconsistentConstraintDetectorEnabled )
    {
        static SolverConfig solverConfigInconsitencyDetector( SolverConfig::Type_InconsistencyDetector );
        PGSSolver::solveInternal(_contactConnectors, _dt, debugTime, _throttled, solverConfigInconsitencyDetector);
    }

    // Solve only if there are no inconsistent constraints detected
    if( !physicsAnalyzerBreakOnIssue || ( inconsistentBodyPairs.size() == 0 ) )
    {
        if( !DFFlag::PGSSolverUsesIslandizableCode )
        {
            solveLegacy(_contactConnectors, _dt, debugTime, _throttled);
        }
        else
        {
            static SolverConfig solverConfigDefault( SolverConfig::Type_Default );
            solverConfigDefault.useSimIslands = DFFlag::PGSSolverSimIslandsEnabled;
            PGSSolver::solveInternal(_contactConnectors, _dt, debugTime, _throttled, solverConfigDefault);
        }
    }
}

void PGSSolver::solvePositions( const std::vector< ContactConnector* >& _contactConnectors )
{
    static SolverConfig solverConfig( SolverConfig::Type_PositionalCorrection );

    PGSSolver::solveInternal(_contactConnectors, 0.0f, 0, false, solverConfig);
}

void PGSSolver::solveInternal( const std::vector< ContactConnector* >& _contactConnectors, float _dt, boost::uint64_t debugTime, bool _throttled, const SolverConfig& _solverConfig )
{
    RBXPROFILER_SCOPE("Physics", "PGSSolver::solve");

    // To enable add '"FFlagPGSSolverFileDump": "True"' in ClientSettings/ClientAppSettings.json
    // Press Ctrl-F8 to start recording, Ctrl-Shift-F8 to stop
    
    serializer.update( dumpLogSwitch, userId, debugTime );
    SolverConfig solverConfig( _solverConfig );

    solverProfiler.start();
    gatherCollisionsProfiler.start();

    // All objects will be indexed in this map. Uses DenseHashMap.
    const boost::unordered_set< SimBody* >& selectedSimBodies = _throttled ? highPrioritySimBodies : simBodies;

    // Form manifolds and match against existing ones
    ArrayDynamic< ContactManifold* > activeManifolds;
    activeManifolds.reserve( contactManifolds.size() );
    size_t collisionCount = addContactConnectors( activeManifolds, _contactConnectors, selectedSimBodies );

    //
    // Gather collisions and constraints into a single array
    //
    ArrayDynamic< Constraint* > constraints;
    constraints.reserve( pureConstraintSet.size() + collisionCount );

    // Iterate over the (ordered map) of constraints - linear in number of elements.
    {
        RBXPROFILER_SCOPE("Physics", "fillConstraintSet");

        if( _throttled )
        {
            // If we are throttling, ignore any constraints that aren't involved with high priority bodies
            for( const auto& it : pureConstraintSet )
            {
                if( highPrioritySimBodies.find( it.second->getBodyA()->getRootSimBody() ) == highPrioritySimBodies.end() 
                    && ( ( it.second->getBodyB() == NULL ) || highPrioritySimBodies.find( it.second->getBodyB()->getRootSimBody() ) == highPrioritySimBodies.end() ) )
                {
                    continue;
                }
                if( !it.second->isBroken() )
                {
                    constraints.push_back( it.second );
                }
            }
        }
        else
        {
            for( const auto& it : pureConstraintSet )
            {
                if( !it.second->isBroken() )
                {
                    constraints.push_back( it.second );
                }
            }
        }
    
        for( const auto manifold : activeManifolds )
        {
            constraints.insert( constraints.end(), manifold->collisions.cbegin(), manifold->collisions.cend() );
        }
    }
    gatherCollisionsProfiler.end();

    serializer & solverConfig.pgsIterations;

    serializer.serializeConstraints(constraints);

    if( solverConfig.useSimIslands )
    {
        // Break bodies and constraints into islands
        islandSplitProfiler.start();
        ArrayDynamic< SimBody* > islandBodies;
        ArrayDynamic< Constraint* > islandConstraints;
        ArrayDynamic< std::pair< boost::uint32_t, boost::uint32_t > > islandSizes;
        breakIntoIslands( islandBodies, islandConstraints, islandSizes, selectedSimBodies, constraints );
        islandSplitProfiler.end();

        if( solverConfig.inconsistentConstraintDetectorEnabled )
        {
            inconsistentBodies.reserve( islandSizes.size() );
        }

        // Dispatch the solve tasks
        size_t constraintIndex = 0;
        size_t simBodyIndex = 0;
        for( size_t i = 0; i < islandSizes.size(); i++ )
        {
            ArrayRef< SimBody* > simBodiesRef( islandBodies.begin() + simBodyIndex, islandSizes[ i ].first );
            simBodyIndex += islandSizes[ i ].first;
            ArrayRef< Constraint* > constraintsRef( islandConstraints.begin() + constraintIndex, islandSizes[ i ].second );
            constraintIndex += islandSizes[ i ].second;
            solveIsland( constraintsRef, simBodiesRef,  _dt, solverConfig );
        }
    }
    else
    {
        ArrayDynamic< SimBody* > simBodyArray;
        simBodyArray.reserve( selectedSimBodies.size() );
        for( auto b : selectedSimBodies )
        {
            simBodyArray.push_back( b );
        }
        solveIsland( constraints, simBodyArray, _dt, solverConfig );
    }

    solverProfiler.end();

    // Print profiling data
    gatherCollisionsProfiler.printStats();
    islandSplitProfiler.printStats();
    integrateVelocitiesProfiler.printStats();
    initAnchoredBodiesProfiler.printStats();
    buildEquationsProfiler.printStats();
    computeEffectiveMassesProfiler.printStats();
    preconditioningProfiler.printStats();
    multiplyEffectiveMassMultipliersProfiler.printStats();
    initVirDProfiler.printStats();
    kernelProfiler.printStats();
    writeCacheProfiler.printStats();
    integratePositionsProfiler.printStats();
    solverProfiler.printStats();
}

void PGSSolver::solveIsland( const ArrayDynamic< Constraint* >& _constraints, 
                            const ArrayDynamic< SimBody* >& _simBodies, 
                            float _dt, 
                            const SolverConfig& solverConfig )
{
    // Align to cache lines
    int defaultAlignment = 64;

    BodyIndexation bodyIndexation( (SimBody*)NULL, G3D::ceilPow2( 2 * _simBodies.size() ) );
    ArrayDynamic< SimBody* > simBodyList;
    {
        RBXPROFILER_SCOPE("Physics", "generateBodyIndices");

        simBodyList.reserve( _simBodies.size() );
        for( auto body : _simBodies )
        {
            bodyIndexation[ body ] = (int)simBodyList.size();
            simBodyList.push_back( body );
        }
    }

    //
    // Gather sim body indices for each constraint
    //
    ArrayDynamic< boost::uint8_t > dimensions( _constraints.size(), ArrayNoInit() );
    ArrayDynamic< boost::uint32_t > offsets( _constraints.size(), ArrayNoInit() );
    ArrayDynamic< BodyPairIndices > simBodyPairs( _constraints.size(), ArrayNoInit() );

    // Listing anchored bodies interacting with sim bodies
    ArrayDynamic< SimBody* > anchoredBodyList;
    boost::uint32_t totalBlockSize;
    boost::uint32_t collisionCount = 0;
    auto totalDimension = gatherConstraintPairData(simBodyPairs, dimensions, offsets, anchoredBodyList, bodyIndexation, totalBlockSize, collisionCount, _constraints );

    //
    // Init sim bodies, integrate velocities
    //
    integrateVelocitiesProfiler.start();
    ArrayDynamic< SolverBodyDynamicProperties > bodyVariableData;
    ArrayDynamic< SolverBodyStaticProperties > bodyStaticData;
    ArrayDynamic< SolverBodyMassAndInertia > massAndInertia;
    ArrayDynamic< float > effectiveMassMultipliers;
    bodyVariableData.reserve( bodyIndexation.size() );
    bodyStaticData.reserve( bodyIndexation.size() );
    massAndInertia.reserve( bodyIndexation.size() );
    effectiveMassMultipliers.reserve( bodyIndexation.size() );
    integrateVelocitiesAndInitSimulatedObjects( bodyStaticData, bodyVariableData, massAndInertia, effectiveMassMultipliers, simBodyList, _dt, solverConfig );
    integrateVelocitiesProfiler.end();

    initAnchoredBodiesProfiler.start();
    VirtualDisplacementArray velocityDeltasSIMD( bodyIndexation.size(), defaultAlignment );
    VirtualDisplacementArray positionDeltasSIMD( bodyIndexation.size(), defaultAlignment );
    velocityDeltasSIMD.reset();
    positionDeltasSIMD.reset();
    initAnchoredObjects(bodyVariableData, bodyStaticData, massAndInertia, effectiveMassMultipliers, velocityDeltasSIMD, positionDeltasSIMD, (int)simBodyList.size(), anchoredBodyList, solverConfig);

    initAnchoredBodiesProfiler.end();

    serializer.serializeForces(simBodyList, bodyStaticData.size());
    serializer.tag("Bodies");
    serializer & simBodyPairs & bodyVariableData & bodyStaticData & massAndInertia;

    //
    // Build constraint equations: setup the Jacobians and the reaction vectors, restore cached solutions
    //
    buildEquationsProfiler.start();
    ArrayDynamic< ConstraintJacobianPair > jacobians( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< ConstraintVariables > velocityStage( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< ConstraintVariables > positionStage( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< float > sorVel( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< float > sorPos( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< boost::uint8_t > useBlock( totalDimension, ArrayNoInit() );
    {
        RBXPROFILER_SCOPE("Physics", "buildConstraintEquations");

        for( size_t i = 0; i < _constraints.size(); i++ )
        {
            const auto& bodyA = bodyVariableData[ simBodyPairs[i].first ];
            const auto& bodyB = bodyVariableData[ simBodyPairs[i].second ];
            boost::uint32_t currentOffset = offsets[ i ];
            _constraints[ i ]->restoreCacheAndBuildEquation( 
                jacobians.data() + currentOffset, 
                velocityStage.data() + currentOffset, 
                positionStage.data() + currentOffset,
                sorVel.data() + currentOffset,
                sorPos.data() + currentOffset,
                useBlock.data() + currentOffset,
                bodyA, bodyB, solverConfig, _dt );
        }
        if( !solverConfig.velocityStageSOREnabled )
        {
            sorVel.assign(sorVel.size(),1.0f);
        }
        if( !solverConfig.positionStageSOREnabled )
        {
            sorPos.assign(sorPos.size(),1.0f);
        }
    }
    buildEquationsProfiler.end();

    serializer.tag("Variables");
    serializer & velocityStage & positionStage;

    //
    // Compute effective masses: M^-1 * J^t
    //
    computeEffectiveMassesProfiler.start();
    ArrayDynamic< EffectiveMassPair > effectiveMassesVel( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< EffectiveMassPair > effectiveMassesPos( totalDimension, ArrayNoInit(), defaultAlignment );
    PGSComputeEffectiveMasses(effectiveMassesVel.data(), effectiveMassesPos.data(), _constraints.size(), dimensions.data(), jacobians.data(), simBodyPairs.data(), massAndInertia.data(), solverConfig);
    computeEffectiveMassesProfiler.end();

    //
    // Pre-condition the Jacobians and reaction vectors
    //
    preconditioningProfiler.start();
    ArrayDynamic< ConstraintJacobianPair > preconditionedJacobiansVelStage( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< ConstraintJacobianPair > preconditionedJacobiansPosStage( totalDimension, ArrayNoInit(), defaultAlignment );
    PGSPreconditionConstraintEquations(preconditionedJacobiansVelStage.data(), preconditionedJacobiansPosStage.data(), 
        velocityStage.data(), positionStage.data(), _constraints.size(), dimensions.data(), useBlock.data(), 
        sorVel.data(), sorPos.data(), jacobians.data(), effectiveMassesVel.data(), effectiveMassesPos.data() );
    preconditioningProfiler.end();

    //
    // Collapse the masses of sleeping simulated objects to infinity
    // This has to be done after pre-conditioning
    //
    multiplyEffectiveMassMultipliersProfiler.start();
    PGSApplyEffectiveMassMultipliers( effectiveMassesVel.data(), effectiveMassesPos.data(), _constraints.size(), dimensions.data(), effectiveMassMultipliers.data(),
        simBodyPairs.data(), solverConfig );
    multiplyEffectiveMassMultipliersProfiler.end();

    serializer.tag("Jacobians");
    serializer & jacobians & preconditionedJacobiansVelStage & preconditionedJacobiansPosStage & effectiveMassesVel & effectiveMassesPos;

    //
    // Initialize the virtual displacements using previously computed impulses (cached)
    //
    initVirDProfiler.start();
    PGSInitVirtualDisplacements( velocityDeltasSIMD, positionDeltasSIMD, effectiveMassesVel.data(), effectiveMassesPos.data(), _constraints.size(), dimensions.data(), velocityStage.data(), positionStage.data(), simBodyPairs.data(), solverConfig );
    initVirDProfiler.end();

    //
    // Run the kernel
    //
    kernelProfiler.start();
    if( solverConfig.inconsistentConstraintDetectorEnabled )
    {
        detectInconsistentConstraints(positionDeltasSIMD, positionStage, jacobians, preconditionedJacobiansPosStage, effectiveMassesPos, sorPos, dimensions, offsets, simBodyPairs, _constraints, collisionCount, solverConfig );
    }
    else
    {
        PGSSolveKernel( 
            velocityStage.data(), positionStage.data(),
            velocityDeltasSIMD, positionDeltasSIMD,
            _constraints.size(), collisionCount,
            dimensions.data(), simBodyPairs.data(),
            preconditionedJacobiansVelStage.data(), preconditionedJacobiansPosStage.data(),
            effectiveMassesVel.data(), effectiveMassesPos.data(), solverConfig );
    }
    kernelProfiler.end();

    serializer.tag("Results");
    serializer.serializeComputedImpulse(velocityStage, positionStage);
    serializer & velocityDeltasSIMD & positionDeltasSIMD;

    //
    // Write out the constraint cache
    //
    writeCacheProfiler.start();
    if( solverConfig.constraintCachingEnabled )
    {
        RBXPROFILER_SCOPE("Physics", "updateConstraintCache");

        for( size_t i = 0; i < _constraints.size(); i++ )
        {
            auto currentOffset = offsets[ i ];
            _constraints[ i ]->updateBrokenState( velocityStage.data() + currentOffset, positionStage.data() + currentOffset, solverConfig );
            _constraints[ i ]->cache(velocityStage.data() + currentOffset, positionStage.data() + currentOffset, sorVel.data() + currentOffset, sorPos.data() + currentOffset, solverConfig);
        }
    }
    writeCacheProfiler.end();

    //
    // Integrate positions and write out into SimBodies
    //
    integratePositionsProfiler.start();
    if( solverConfig.updateSimBodies )
    {
        if( !solverConfig.integrateOnlyPositions )
        {
            integratePositionsAndUpdateSimBodies( simBodyList.data(), bodyVariableData.data(), bodyStaticData.data(), simBodyList.size(), velocityDeltasSIMD, positionDeltasSIMD, _dt );
        }
        else
        {
            integratePositionsIgnoreVelocitiesAndUpdateSimBodies( simBodyList.data(), bodyVariableData.data(), bodyStaticData.data(), simBodyList.size(), positionDeltasSIMD, _dt );
        }
    }
    integratePositionsProfiler.end();

    serializer.tag("Integrated");
    serializer & bodyVariableData;

    serializer.tag("Cache");
    serializer.serializeBodyCache(bodyCache);
}

void PGSSolver::solveLegacy( const std::vector< ContactConnector* >& _contactConnectors, float _dt, boost::uint64_t debugTime, bool _throttled )
{
    RBXPROFILER_SCOPE("Physics", "PGSSolver::solve");

    // Align to cache lines
    int defaultAlignment = 64;

    // To enable add '"FFlagPGSSolverFileDump": "True"' in ClientSettings/ClientAppSettings.json
    // Press Ctrl-F8 to start recording, Ctrl-Shift-F8 to stop
    
    serializer.update( dumpLogSwitch, userId, debugTime );

    // Static SolverConfig so we can change values from debugger
    static SolverConfig solverConfig;

    solverProfiler.start();
    gatherCollisionsProfiler.start();

    // All objects will be indexed in this map. Uses DenseHashMap.
    const boost::unordered_set< SimBody* >& selectedSimBodies = _throttled ? highPrioritySimBodies : simBodies;
    BodyIndexation bodyIndexation( (SimBody*)NULL, G3D::ceilPow2( 2 * simBodies.size() ) );
    ArrayDynamic< SimBody* > simBodyList;
    {
        RBXPROFILER_SCOPE("Physics", "generateBodyIndices");

        simBodyList.reserve( simBodies.size() );
        for( auto body : selectedSimBodies )
        {
            bodyIndexation[ body ] = (int)simBodyList.size();
            simBodyList.push_back( body );
        }
    }

    // Form manifolds and match against existing ones
    ArrayDynamic< ContactManifold* > activeManifolds;
    activeManifolds.reserve( contactManifolds.size() );
    boost::uint32_t collisionCount = ( boost::uint32_t )addContactConnectors( activeManifolds, _contactConnectors, selectedSimBodies );

    //
    // Gather collisions and constraints into a single array
    //
    ArrayDynamic< Constraint* > constraints;
    constraints.reserve( pureConstraintSet.size() + collisionCount );

    // Iterate over the (ordered map) of constraints - linear in number of elements.
    {
        RBXPROFILER_SCOPE("Physics", "fillConstraintSet");

        if( _throttled )
        {
            // If we are throttling, ignore any constraints that aren't involved with high priority bodies
            for( const auto& it : pureConstraintSet )
            {
                if( highPrioritySimBodies.find( it.second->getBodyA()->getRootSimBody() ) == highPrioritySimBodies.end() 
                    && ( ( it.second->getBodyB() == NULL ) || highPrioritySimBodies.find( it.second->getBodyB()->getRootSimBody() ) == highPrioritySimBodies.end() ) )
                {
                    continue;
                }
                if( !it.second->isBroken() )
                {
                    constraints.push_back( it.second );
                }
            }
        }
        else
        {
            for( const auto& it : pureConstraintSet )
            {
                if( !it.second->isBroken() )
                {
                    constraints.push_back( it.second );
                }
            }
        }
    
        for( const auto manifold : activeManifolds )
        {
            constraints.insert( constraints.end(), manifold->collisions.cbegin(), manifold->collisions.cend() );
        }
    }

    serializer.serializeConstraints(constraints);

    //
    // Gather sim body indices for each constraint
    //
    ArrayDynamic< boost::uint8_t > dimensions( constraints.size(), ArrayNoInit() );
    ArrayDynamic< boost::uint32_t > offsets( constraints.size(), ArrayNoInit() );
    ArrayDynamic< BodyPairIndices > simBodyPairs( constraints.size(), ArrayNoInit() );

    // Listing anchored bodies interacting with sim bodies
    ArrayDynamic< SimBody* > anchoredBodyList;
    boost::uint32_t totalBlockSize;
    auto totalDimension = gatherConstraintPairData(simBodyPairs, dimensions, offsets, anchoredBodyList, bodyIndexation, totalBlockSize, collisionCount, constraints );
    gatherCollisionsProfiler.end();

    //
    // Init sim bodies, integrate velocities
    //
    integrateVelocitiesProfiler.start();
    ArrayDynamic< SolverBodyDynamicProperties > bodyVariableData;
    ArrayDynamic< SolverBodyStaticProperties > bodyStaticData;
    ArrayDynamic< SolverBodyMassAndInertia > massAndInertia;
    ArrayDynamic< float > effectiveMassMultipliers;
    bodyVariableData.reserve( bodyIndexation.size() );
    bodyStaticData.reserve( bodyIndexation.size() );
    massAndInertia.reserve( bodyIndexation.size() );
    effectiveMassMultipliers.reserve( bodyIndexation.size() );
    integrateVelocitiesAndInitSimulatedObjects( bodyStaticData, bodyVariableData, massAndInertia, effectiveMassMultipliers, simBodyList, _dt, solverConfig );
    integrateVelocitiesProfiler.end();

    initAnchoredBodiesProfiler.start();
    VirtualDisplacementArray velocityDeltasSIMD( bodyIndexation.size(), defaultAlignment );
    VirtualDisplacementArray positionDeltasSIMD( bodyIndexation.size(), defaultAlignment );
    velocityDeltasSIMD.reset();
    positionDeltasSIMD.reset();
    initAnchoredObjects(bodyVariableData, bodyStaticData, massAndInertia, effectiveMassMultipliers, velocityDeltasSIMD, positionDeltasSIMD, (int)simBodyList.size(), anchoredBodyList, solverConfig);

    initAnchoredBodiesProfiler.end();

    serializer.serializeForces(simBodyList, bodyStaticData.size());
    serializer.tag("Bodies");
    serializer & simBodyPairs & bodyVariableData & bodyStaticData & massAndInertia;

    //
    // Build constraint equations: setup the Jacobians and the reaction vectors, restore cached solutions
    //
    buildEquationsProfiler.start();
    ArrayDynamic< ConstraintJacobianPair > jacobians( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< ConstraintVariables > velocityStage( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< ConstraintVariables > positionStage( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< float > sorVel( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< float > sorPos( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< boost::uint8_t > useBlock( totalDimension, ArrayNoInit() );
    {
        RBXPROFILER_SCOPE("Physics", "buildConstraintEquations");

        for( size_t i = 0; i < constraints.size(); i++ )
        {
            const auto& bodyA = bodyVariableData[ simBodyPairs[i].first ];
            const auto& bodyB = bodyVariableData[ simBodyPairs[i].second ];
            boost::uint32_t currentOffset = offsets[ i ];
            constraints[ i ]->restoreCacheAndBuildEquation( 
                jacobians.data() + currentOffset, 
                velocityStage.data() + currentOffset, 
                positionStage.data() + currentOffset,
                sorVel.data() + currentOffset,
                sorPos.data() + currentOffset,
                useBlock.data() + currentOffset,
                bodyA, bodyB, solverConfig, _dt );
        }
    }
    buildEquationsProfiler.end();

    serializer.tag("Variables");
    serializer & velocityStage & positionStage;

    //
    // Compute effective masses: M^-1 * J^t
    //
    computeEffectiveMassesProfiler.start();
    ArrayDynamic< EffectiveMassPair > effectiveMassesVel( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< EffectiveMassPair > effectiveMassesPos( totalDimension, ArrayNoInit(), defaultAlignment );
    PGSComputeEffectiveMasses(effectiveMassesVel.data(), effectiveMassesPos.data(), constraints.size(), dimensions.data(), jacobians.data(), simBodyPairs.data(), massAndInertia.data(), solverConfig);
    computeEffectiveMassesProfiler.end();

    //
    // Pre-condition the Jacobians and reaction vectors
    //
    preconditioningProfiler.start();
    ArrayDynamic< ConstraintJacobianPair > preconditionedJacobiansVelStage( totalDimension, ArrayNoInit(), defaultAlignment );
    ArrayDynamic< ConstraintJacobianPair > preconditionedJacobiansPosStage( totalDimension, ArrayNoInit(), defaultAlignment );
    PGSPreconditionConstraintEquations(preconditionedJacobiansVelStage.data(), preconditionedJacobiansPosStage.data(), 
        velocityStage.data(), positionStage.data(), constraints.size(), dimensions.data(), useBlock.data(), 
        sorVel.data(), sorPos.data(), jacobians.data(), effectiveMassesVel.data(), effectiveMassesPos.data() );
    preconditioningProfiler.end();

    //
    // Collapse the masses of sleeping simulated objects to infinity
    // This has to be done after pre-conditioning
    //
    multiplyEffectiveMassMultipliersProfiler.start();
    PGSApplyEffectiveMassMultipliers( effectiveMassesVel.data(), effectiveMassesPos.data(), constraints.size(), dimensions.data(), effectiveMassMultipliers.data(),
        simBodyPairs.data(), solverConfig );
    multiplyEffectiveMassMultipliersProfiler.end();

    serializer.tag("Jacobians");
    serializer & jacobians & preconditionedJacobiansVelStage & preconditionedJacobiansPosStage & effectiveMassesVel & effectiveMassesPos;

    //
    // Initialize the virtual displacements using previously computed impulses (cached)
    //
    initVirDProfiler.start();
    PGSInitVirtualDisplacements( velocityDeltasSIMD, positionDeltasSIMD, effectiveMassesVel.data(), effectiveMassesPos.data(), constraints.size(), dimensions.data(), velocityStage.data(), positionStage.data(), simBodyPairs.data(), solverConfig );
    initVirDProfiler.end();

    //
    // Run the kernel
    //
    kernelProfiler.start();
    PGSSolveKernel( 
        velocityStage.data(), positionStage.data(),
        velocityDeltasSIMD, positionDeltasSIMD,
        constraints.size(), collisionCount,
        dimensions.data(), simBodyPairs.data(),
        preconditionedJacobiansVelStage.data(), preconditionedJacobiansPosStage.data(),
        effectiveMassesVel.data(), effectiveMassesPos.data(), solverConfig );
    kernelProfiler.end();

    serializer.tag("Results");
    serializer.serializeComputedImpulse(velocityStage, positionStage);
    serializer & velocityDeltasSIMD & positionDeltasSIMD;

    //
    // Write out the constraint cache
    //
    writeCacheProfiler.start();
    {
        RBXPROFILER_SCOPE("Physics", "updateConstraintCache");

        for( size_t i = 0; i < constraints.size(); i++ )
        {
            auto currentOffset = offsets[ i ];
            constraints[ i ]->updateBrokenState( velocityStage.data() + currentOffset, positionStage.data() + currentOffset, solverConfig );
            constraints[ i ]->cache(velocityStage.data() + currentOffset, positionStage.data() + currentOffset, sorVel.data() + currentOffset, sorPos.data() + currentOffset, solverConfig);
        }
    }
    writeCacheProfiler.end();

    //
    // Integrate positions and write out into SimBodies
    //
    integratePositionsProfiler.start();
    integratePositionsAndUpdateSimBodies( simBodyList.data(), bodyVariableData.data(), bodyStaticData.data(), simBodyList.size(), velocityDeltasSIMD, positionDeltasSIMD, _dt );
    integratePositionsProfiler.end();

    serializer.tag("Integrated");
    serializer & bodyVariableData;

    serializer.tag("Cache");
    serializer.serializeBodyCache(bodyCache);

    solverProfiler.end();

    // Print profiling data
    gatherCollisionsProfiler.printStats();
    integrateVelocitiesProfiler.printStats();
    initAnchoredBodiesProfiler.printStats();
    buildEquationsProfiler.printStats();
    computeEffectiveMassesProfiler.printStats();
    preconditioningProfiler.printStats();
    multiplyEffectiveMassMultipliersProfiler.printStats();
    initVirDProfiler.printStats();
    kernelProfiler.printStats();
    writeCacheProfiler.printStats();
    integratePositionsProfiler.printStats();
    solverProfiler.printStats();
}

void PGSSolver::addSimBody( SimBody* _body, bool _highPriority )
{
    if( simBodies.find( _body ) == simBodies.end() )
    {
        simBodies.insert( _body );
    }

    if( _highPriority && highPrioritySimBodies.find( _body ) == highPrioritySimBodies.end() )
    {
        highPrioritySimBodies.insert( _body );
    }
}

void PGSSolver::removeSimBody( SimBody* _body )
{
    auto it = simBodies.find( _body );
    if( it != simBodies.end() )
    {
        simBodies.erase(it);
    }

    it = highPrioritySimBodies.find( _body );
    if( it != highPrioritySimBodies.end() )
    {
        highPrioritySimBodies.erase( it );
    }
}

void PGSSolver::addConstraint( Constraint* _constraint )
{
    // The engine shouldn't explicitly add collisions.
    RBXASSERT( _constraint->getType() != Constraint::Types_Collision );
    if( _constraint->hasValidUID() && pureConstraintSet.find( _constraint->getUID() ) == pureConstraintSet.end() )
    {
        // If the constraint was previously registered with the solver, use the same index.
        // This is to preserve the same ordering of constraints as they are registered and unregistered
        // The solver stability depends on maintaining this order
        pureConstraintSet[ _constraint->getUID() ] = _constraint;
    }
    else
    {
        // We need a new index
        constraintUIDGenerator++;
        _constraint->setUID( constraintUIDGenerator );
        pureConstraintSet[ constraintUIDGenerator ] = _constraint;
    }
}

void PGSSolver::removeConstraint( Constraint* _constraint )
{
    auto it = pureConstraintSet.find( _constraint->getUID() );
    if( it != pureConstraintSet.end() )
    {
        pureConstraintSet.erase( it );
    }
}

ContactManifold::~ContactManifold()
{
    for( size_t i = 0; i < collisions.size(); i++ )
    {
        delete collisions[ i ];
    }
}

void PGSSolver::addContactManifold( boost::uint64_t _uidA, boost::uint64_t _uidB )
{
    BodyUIDPair pair( _uidA, _uidB );
    if( pair.first > pair.second )
    {
        std::swap( pair.first, pair.second );
    }

    auto it = contactManifolds.find( pair );
    if( it == contactManifolds.end() )
    {
        ContactManifold* contactManifold = new ContactManifold();
        contactManifold->referenceCount++;
        contactManifolds[ pair ] = contactManifold;
    }
    else
    {
        it->second->referenceCount++;
    }
}

void PGSSolver::removeContactManifold( boost::uint64_t _uidA, boost::uint64_t _uidB )
{
    BodyUIDPair pair( _uidA, _uidB );
    if( pair.first > pair.second )
    {
        std::swap( pair.first, pair.second );
    }

    auto it = contactManifolds.find( pair );
    RBXASSERT( it != contactManifolds.end() );
    if( it != contactManifolds.end() )
    {
        it->second->referenceCount--;
        if( it->second->referenceCount == 0 )
        {
            delete it->second;
            contactManifolds.erase( it );
        }
    }
}

void PGSSolver::clearBodyCache( boost::uint64_t _uid )
{
    auto it = bodyCache.find( _uid );
    if( it != bodyCache.end() )
    {
        bodyCache.erase( it );
    }
}

static RBX_SIMD_INLINE void updateCollision( ConstraintCollision& _collision, const OrderedConnector& _inputConnector )
{
    ContactConnector* connector = _inputConnector.connector;
    const PairParams& params = _inputConnector.connector->getContactPoint();

    _collision.setFriction( connector->getContactParams().kFriction );
    _collision.setResititution( connector->getContactParams().kElasticity );
    _collision.setDepth( params.length );

    RBXASSERT_VERY_FAST( !RBX::Math::isNanInf( params.normal.length() ) && std::abs( 1.0f - params.normal.length() ) < 0.1f );
    if( !_inputConnector.swap )
    {
        _collision.setBodyA( connector->getBody( Connector::body0 ) );
        _collision.setBodyB( connector->getBody( Connector::body1 ) );
        _collision.setPointA( params.position );
        _collision.setNormal( params.normal );
    }
    else
    {
        _collision.setBodyB( connector->getBody( Connector::body0 ) );
        _collision.setBodyA( connector->getBody( Connector::body1 ) );
        _collision.setPointA( params.position + params.length * params.normal );
        _collision.setNormal( -params.normal );
    }
}

ContactManifold* PGSSolver::updateContactManifold( const BodyUIDPair& _pairId, const ArrayBase< OrderedConnector >& _inputManifold )
{
    // Filter out the bad connectors 
    ArrayDynamic< OrderedConnector > cleanManifold;
    cleanManifold.reserve( _inputManifold.size() );
    for( auto c : _inputManifold )
    {
        if( c.connector->getContactPoint().normal.squaredLength() > 0.8f )
        {
            cleanManifold.push_back( c );
        }
    }

    if( cleanManifold.size() == 0 )
    {
        return 0;
    }

    // Find the contact manifold in the cache
    auto cachedManifoldIt = contactManifolds.find( _pairId );

    // It should find it, but fail safe in case of bugs
    RBXASSERT( cachedManifoldIt != contactManifolds.end() );
    if( cachedManifoldIt != contactManifolds.end() )
    {
        // Match the collisions between the two manifolds
        size_t newContactIndex = 0;
        size_t cachedContactIndex = 0;
        auto& collisions = cachedManifoldIt->second->collisions;

        // Match the collisions simply using their order in the manifold
        for( ; cachedContactIndex < collisions.size(); cachedContactIndex++, newContactIndex++ )
        {
            if( newContactIndex == cleanManifold.size() )
            {
                break;
            }

            updateCollision( *collisions[ cachedContactIndex ], cleanManifold[ newContactIndex ] );
        }

        // Add the new collisions
        for( ; newContactIndex < cleanManifold.size(); newContactIndex++ )
        {
            ConstraintCollision* collision = new ConstraintCollision( 
                cleanManifold[ newContactIndex ].connector->getBody( Connector::body0 ),
                cleanManifold[ newContactIndex ].connector->getBody( Connector::body1 ) );
            constraintUIDGenerator++;
            collision->setUID(constraintUIDGenerator);
            updateCollision( *collision, cleanManifold[ newContactIndex ] );
            collisions.push_back( collision );
        }

        // Remove old collisions
        if( cleanManifold.size() < collisions.size() )
        {
            collisions.resize( cleanManifold.size() );
        }

        return cachedManifoldIt->second;
    }
    return NULL;
}

class OrderedConnectorPair: public std::pair< BodyUIDPair, OrderedConnector >
{
public:
    OrderedConnectorPair( const BodyUIDPair& p, const OrderedConnector& c ): std::pair< BodyUIDPair, OrderedConnector >( p, c ) { }
    inline bool operator<( const OrderedConnectorPair& a ) const
    {
        return first < a.first;
    }
};

size_t PGSSolver::addContactConnectors( ArrayDynamic< ContactManifold* >& _activeManifolds, 
                                       const std::vector< ContactConnector* >& _connectors,
                                       const boost::unordered_set< SimBody* >& _simBodies )
{
    RBXPROFILER_SCOPE("Physics", "addContactConnectors");

    // Ensure the collisions are always in the same order.
    // That also makes the solver more stable for static situations as it reaches more or less the same solution every frame
    ArrayDynamic< OrderedConnectorPair > orderedConnectors;
    orderedConnectors.reserve( _connectors.size() );
    for( size_t j = 0; j < _connectors.size(); j++ )
    {
        Body* _bodyA = _connectors[ j ]->getBody( Connector::body0 );
        Body* _bodyB = _connectors[ j ]->getBody( Connector::body1 );

        // If both bodies are not simulated, drop the connector.
        // This seemed to be happening, must track down why.
        if( _simBodies.find( _bodyA->getRootSimBody() ) == _simBodies.end() && _simBodies.find( _bodyB->getRootSimBody() ) == _simBodies.end() )
        {
            continue;
        }

        OrderedConnector orderedContact;
        orderedContact.connector = _connectors[ j ];
        orderedContact.swap = false;

        BodyUIDPair pair( _bodyA->getUID(), _bodyB->getUID() );
        if( pair.first > pair.second )
        {
            orderedContact.swap = true;
            std::swap( pair.first, pair.second );
        }
        orderedConnectors.push_back( OrderedConnectorPair( pair, orderedContact ) );
    }

    // Sort the array of connectors by body pair
    std::stable_sort( orderedConnectors.begin(), orderedConnectors.end() );

    size_t collisionCount = 0;

    BodyUIDPair pair( 0, 0 );
    ArrayDynamic< OrderedConnector > manifold;
    for (size_t j = 0; j < orderedConnectors.size(); ++j) 
    {
        if( pair != orderedConnectors[ j ].first )
        {
            if( manifold.size() > 0 )
            {
                ContactManifold* m = updateContactManifold( pair, manifold );
                if( m != NULL )
                {
                    collisionCount += m->collisions.size();
                    _activeManifolds.push_back( m );
                }
            }
            manifold.clear();
            pair = orderedConnectors[ j ].first;
        }
        manifold.push_back( orderedConnectors[ j ].second );
    }
    if( manifold.size() > 0 )
    {
        ContactManifold* m = updateContactManifold( pair, manifold );
        if( m != NULL )
        {
            collisionCount += m->collisions.size();
            _activeManifolds.push_back( m );
        }
    }

    return collisionCount;
}

void PGSSolver::dumpLog( bool enable )
{
    if( FFlag::PGSSolverFileDump )
    {
        dumpLogSwitch = enable;
    }
}

void PGSSolver::SolverBodyCache::serialize( DebugSerializer& s ) const
{
    // simBodyDebug->updateIfDirty();
    const CoordinateFrame& c = simBodyDebug->getPV().position;
    s & c.translation;
    s & c.rotation.row(0);
    s & c.rotation.row(1);
    s & c.rotation.row(2);
    s & simBodyDebug->getMassRecip();
}

}
