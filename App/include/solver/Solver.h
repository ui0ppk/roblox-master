#pragma once

#include "solver/SolverConfig.h"
#include "solver/Constraint.h"
#include "solver/SolverContainers.h"
#include "solver/SolverProfiler.h"
#include "solver/SolverSerializer.h"

#include "v8kernel/SimBody.h"

#include "boost/unordered/unordered_map.hpp"
#include "boost/unordered/unordered_set.hpp"
#include "boost/container/vector.hpp"
#include <map>

namespace RBX
{

class ContactConnector;
class RotateJoint;
class ContactManifold;
class Body;

typedef std::pair< boost::uint64_t, boost::uint64_t > BodyUIDPair;

class InconsistentBodyPair
{
public:
    bool operator<( const InconsistentBodyPair& pair ) const
    {
        return bodyPair < pair.bodyPair;
    }

    Body* bodyA;
    Body* bodyB;
    BodyUIDPair bodyPair;
    Constraint::Convergence convergence;
};

class OrderedConnector
{
public:
    ContactConnector* connector;
    bool swap;
};

class BadConnector
{
public:
    Vector3 position;
    Constraint::Convergence convergence;
};

class PGSSolver
{
public:
    PGSSolver();

    // Add/remove SimBodies
    // High priority bodies will be simulated even if throttled
    void addSimBody( SimBody* body, bool highPriority );
    void removeSimBody( SimBody* body );

    // Add/remove constraints
    void addConstraint( Constraint* _constraint );
    void removeConstraint( Constraint* _constraint );

    // Solver
    // If throttled is set to true, only high priority bodies will be simulated.
    void solve( const std::vector< ContactConnector* >& connectors, float dt, boost::uint64_t debugTime, bool throttled );
    void solvePositions( const std::vector< ContactConnector* >& _contactConnectors );
    // Exactly like it was before the physics analyzer (sleeping islands) were submitted
    void solveLegacy( const std::vector< ContactConnector* >& _contactConnectors, float _dt, boost::uint64_t debugTime, bool _throttled );

    // Create and delete contact manifolds
    // The parameters are uid's of both Object instances.
    void addContactManifold( boost::uint64_t _uidA, boost::uint64_t _uidB );
    void removeContactManifold( boost::uint64_t _uidA, boost::uint64_t _uidB );

    // Clear body cache
    void clearBodyCache( boost::uint64_t _uid );

    // Switch inconsistent constraint detector
    void setInconsistentConstraintDetectorEnabled( bool value ) { inconsistentConstraintDetectorEnabled = value; }
    void setPhysicsAnalyzerBreakOnIssue( bool value ) { physicsAnalyzerBreakOnIssue = value; }
    bool getPhysicsAnalyzerBreakOnIssue( ) const { return physicsAnalyzerBreakOnIssue; }

    const ArrayBase< InconsistentBodyPair >& getInconsistentBodyPairs() const { return inconsistentBodyPairs; }
    const ArrayBase< ArrayDynamic< boost::uint64_t > >& getInconsistentBodies() const { return inconsistentBodies; }

    void dumpLog( bool enable );
    void setUserId( int id ) { userId = id; }

private:
    void solveInternal( const std::vector< ContactConnector* >& connectors, float dt, boost::uint64_t debugTime, bool throttled, const SolverConfig& _solverConfig );
    void solveIsland( const ArrayDynamic< Constraint* >& constraints, const ArrayDynamic< SimBody* >& selectedSimBodies,
        float _dt, const SolverConfig& _solverConfig );
    ContactManifold* updateContactManifold( const BodyUIDPair& _pairId, const ArrayBase< OrderedConnector >& _manifold );
    size_t addContactConnectors( ArrayDynamic< ContactManifold* >& _activeManifolds, const std::vector< ContactConnector* >& _connectors, const boost::unordered_set< SimBody* >& _simBodies );
    void initAnchoredObjects( 
        ArrayDynamic< SolverBodyDynamicProperties >& _bodyVariableData,
        ArrayDynamic< SolverBodyStaticProperties >& _bodyStaticData,
        ArrayDynamic< SolverBodyMassAndInertia >& _massAndInertia,
        ArrayDynamic< float >& _effectiveMassMultipliers,
        VirtualDisplacementArray& _velocityDeltas,
        VirtualDisplacementArray& _positionDeltas,
        int offsetToAnchoredObjects,
        const ArrayBase< SimBody* >& _anchoredBodyList,
        const SolverConfig& _config ) const;
    void integratePositionsAndUpdateSimBodies( 
        SimBody* const * _simBodies, 
        SolverBodyDynamicProperties* const _bodyVariableData,
        const SolverBodyStaticProperties* const _bodyStaticData, 
        size_t _simBodyCount,
        const VirtualDisplacementArray& _velocityDeltas,
        const VirtualDisplacementArray& _positionDeltas,
        float _dt );
    void integratePositionsIgnoreVelocitiesAndUpdateSimBodies( 
        SimBody* const * _simBodies, 
        SolverBodyDynamicProperties* const _bodyVariableData,
        const SolverBodyStaticProperties* const _bodyStaticData, 
        size_t _simBodyCount,
        const VirtualDisplacementArray& _positionDeltas,
        float _dt );
    void detectInconsistentConstraints(
        VirtualDisplacementArray& positionDeltasSIMD,
        ArrayBase< ConstraintVariables >& positionStage,
        const ArrayBase< ConstraintJacobianPair >& jacobians,
        const ArrayBase< ConstraintJacobianPair >& preconditionedJacobiansPosStage,
        const ArrayBase< EffectiveMassPair >& effectiveMassesPos,
        const ArrayBase< float >& sorPos,
        const ArrayBase< boost::uint8_t >& dimensions,
        const ArrayBase< boost::uint32_t >& offsets,
        const ArrayBase< BodyPairIndices >& simBodyPairs,
        const ArrayBase< Constraint* >& constraints,
        size_t collisionCount,
        const SolverConfig& solverConfig );


    boost::uint64_t constraintUIDGenerator;
    // Pure constraints - not including the Collision constraints
    SolverOrderedMap< boost::uint64_t, Constraint* >::Type pureConstraintSet;

    SolverUnorderedMap< BodyUIDPair, ContactManifold* >::Type contactManifolds;

    class SolverBodyCache
    {
    public:
        void serialize( DebugSerializer& s ) const;
        Vector3 virDPosStageLin;
        Vector3 virDPosStageAng;
        Vector3 virDVelStageLin;
        Vector3 virDVelStageAng;
        Vector3 linearVelocity;
        Vector3 angularVelocity;
        Vector3 integratedLinearVelocity;
        Vector3 integratedAngularVelocity;
        SimBody* simBodyDebug;
    };

    SolverUnorderedMap< boost::uint64_t, SolverBodyCache >::Type bodyCache;
    boost::unordered_set< SimBody* > simBodies;
    boost::unordered_set< SimBody* > highPrioritySimBodies;

    // Inconsistent constraint detector
    bool inconsistentConstraintDetectorEnabled;
    bool physicsAnalyzerBreakOnIssue;
    ArrayDynamic< InconsistentBodyPair > inconsistentBodyPairs;
    ArrayDynamic< ArrayDynamic< boost::uint64_t > > inconsistentBodies;

    // Serializer
    SolverSerializer serializer;

    // Profilers
    SolverProfiler gatherCollisionsProfiler;
    SolverProfiler islandSplitProfiler;
    SolverProfiler integrateVelocitiesProfiler;
    SolverProfiler initAnchoredBodiesProfiler;
    SolverProfiler buildEquationsProfiler;
    SolverProfiler computeEffectiveMassesProfiler;
    SolverProfiler preconditioningProfiler;
    SolverProfiler multiplyEffectiveMassMultipliersProfiler;
    SolverProfiler initVirDProfiler;
    SolverProfiler kernelProfiler;
    SolverProfiler integratePositionsProfiler;
    SolverProfiler writeCacheProfiler;
    SolverProfiler solverProfiler;

    bool dumpLogSwitch;
    int userId;
};

}
