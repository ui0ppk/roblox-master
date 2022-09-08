#pragma once

#include "solver/SolverConfig.h"
#include "solver/ConstraintJacobian.h"
#include "solver/SolverBody.h"
#include "v8kernel/SimBody.h"
#include "v8world/RotateJoint.h"

#include "simd/simd.h"

namespace RBX
{

class PGSSolver;
class DebugSerializer;

//
// ConstraintVariables: inputs to the solver, initialized by the constraint interface
//
class ConstraintVariables
{
public:
    void serialize( DebugSerializer&  s ) const;

    static RBX_SIMD_INLINE void setReaction( ConstraintVariables* _vars, const Vector3& _r )
    {
        _vars[0].reaction = _r.x;
        _vars[1].reaction = _r.y;
        _vars[2].reaction = _r.z;
    }

    static RBX_SIMD_INLINE void setReaction( ConstraintVariables* _vars, float x, float y )
    {
        _vars[0].reaction = x;
        _vars[1].reaction = y;
    }

    static RBX_SIMD_INLINE void setImpulse( ConstraintVariables* _vars, const Vector3& _i )
    {
        _vars[0].impulse = _i.x;
        _vars[1].impulse = _i.y;
        _vars[2].impulse = _i.z;
    }

    static RBX_SIMD_INLINE void setImpulse( ConstraintVariables* _vars, float x, float y )
    {
        _vars[0].impulse = x;
        _vars[1].impulse = y;
    }

    static RBX_SIMD_INLINE void setMinImpulses( ConstraintVariables* _vars, const Vector3& _min )
    {
        _vars[0].minImpulseValue = _min.x;
        _vars[1].minImpulseValue = _min.y;
        _vars[2].minImpulseValue = _min.z;
    }

    static RBX_SIMD_INLINE void setMinImpulses( ConstraintVariables* _vars, float x, float y )
    {
        _vars[0].minImpulseValue = x;
        _vars[1].minImpulseValue = y;
    }

    static RBX_SIMD_INLINE void setMaxImpulses( ConstraintVariables* _vars, const Vector3& _max )
    {
        _vars[0].maxImpulseValue = _max.x;
        _vars[1].maxImpulseValue = _max.y;
        _vars[2].maxImpulseValue = _max.z;
    }

    static RBX_SIMD_INLINE void setMaxImpulses( ConstraintVariables* _vars, float x, float y )
    {
        _vars[0].maxImpulseValue = x;
        _vars[1].maxImpulseValue = y;
    }

    static RBX_SIMD_INLINE void gatherComponents( simd::v4f& _impulses, simd::v4f& _reactions, simd::v4f& _min, simd::v4f& _max, const ConstraintVariables& _vars0 )
    {
        _min = simd::splat< 0 >( simd::v4f( _vars0.v ) );
        _max = simd::splat< 1 >( simd::v4f( _vars0.v ) );
        _reactions = simd::splat< 2 >( simd::v4f( _vars0.v ) );
        _impulses = simd::splat< 3 >( simd::v4f( _vars0.v ) );
    }

    static RBX_SIMD_INLINE void gatherComponents( simd::v4f& _impulses, simd::v4f& _reactions, simd::v4f& _min, simd::v4f& _max, const ConstraintVariables& _vars0, const ConstraintVariables& _vars1 )
    {
        transpose2x4( _min, _max, _reactions, _impulses, simd::v4f(_vars0.v), simd::v4f(_vars1.v) );
    }

    static RBX_SIMD_INLINE void gatherComponents( simd::v4f& _impulses, simd::v4f& _reactions, simd::v4f& _min, simd::v4f& _max, const ConstraintVariables& _vars0, const ConstraintVariables& _vars1, const ConstraintVariables& _vars2 )
    {
        transpose3x4( _min, _max, _reactions, _impulses, (simd::v4f)_vars0.v, (simd::v4f)_vars1.v, (simd::v4f)_vars2.v );
    }

    static RBX_SIMD_INLINE void gatherComponents( simd::v4f& _impulses, simd::v4f& _reactions, simd::v4f& _min, simd::v4f& _max, const ConstraintVariables& _vars0, const ConstraintVariables& _vars1, const ConstraintVariables& _vars2, const ConstraintVariables& _vars3 )
    {
        transpose( _min, _max, _reactions, _impulses, (simd::v4f)_vars0.v, (simd::v4f)_vars1.v, (simd::v4f)_vars2.v, (simd::v4f)_vars3.v );
    }

    // Values that must be set by the Constraint::buildEquation
    // Inputs expected by the solver
    union
    {
        struct
        {
            float minImpulseValue;
            float maxImpulseValue;

            // Constraint must set this to the desired reaction
            float reaction;

            // The Constraint must set this to the impulse computed in the previous frame or 0.0f if it is not available.
            // This will contain the result.
            float impulse;
        };
        simd::v4f_pod v;
    };
};

//
// MovingRegression: Fit best 2nd degree curve to the last few data points
//
class MovingRegression
{
public:
    MovingRegression()
    {
        lastPoint = 0.0f;
        lastTangent = 0.0f;
        lastCurvature = 0.0f;
        confidence = 0.0f;
    }

    inline float testFitNextDataPointZeroOrder( float y ) const
    {
        float predicted = lastPoint;
        return confidence * std::abs( y - predicted ) / ( std::max( std::abs( y ), std::abs( predicted ) ) + 0.00001f ) ;
    }

    inline float testFitNextDataPointFirstOrder( float y ) const
    {
        float predicted = lastPoint + lastTangent;
        return confidence * std::abs( y - predicted ) / ( std::max( std::abs( y ), std::abs( predicted ) ) + 0.00001f ) ;
    }

    inline float testFitNextDataPointSecondOrder( float y ) const
    {
        float predicted = lastPoint + lastTangent + lastCurvature;
        return confidence * std::abs( y - predicted ) / ( std::max( std::abs( y ), std::abs( predicted ) ) + 0.00001f ) ;
    }

    float predict( ) const
    {
        return lastPoint;
    }

    void addDataPoint( float y, float weight )
    {
        float newTangent = y - lastPoint;
        float newCurvature = newTangent - lastTangent;
        lastCurvature = newCurvature;
        lastTangent = newTangent;
        lastPoint = y;
        confidence += 0.1f * ( 1.0f - confidence );
    }

    void serialize( DebugSerializer& s) const;

    float confidence;
    float lastPoint;
    float lastTangent;
    float lastCurvature;
};

//
// Cached values for each constraint equation
//
class ConstraintCache
{
public:
    ConstraintCache(): 
        velocityImpulse( 0.0f ), 
        velocityReaction( 0.0f ), 
        positionImpulse( 0.0f ), 
        positionReaction( 0.0f ), 
        // These need to be initialized to the values in SolverConfig!
        velocitySor( 1.9f ), 
        positionSor( 1.9f ), 
        velocityCacheDamping( 1.0f ), 
        positionCacheDamping( 1.0f ) { }

    void cache( const ConstraintVariables& _velocityStage, const ConstraintVariables& _positionStage, float _sorVel, float _sorPos, bool _isCollision, const SolverConfig& config );

    inline void readCache( ConstraintVariables& _velocityStage, ConstraintVariables& _positionStage, float& _sorVel, float& _sorPos ) const
    {
        _velocityStage.impulse = velocityImpulse;
        _sorVel = velocitySor;
        _velocityStage.reaction = velocityReaction;
        _positionStage.impulse = positionImpulse;
        _sorPos = positionSor;
        _positionStage.reaction = positionReaction;
    }

    void serialize( DebugSerializer& s ) const;

    float velocityImpulse;
    float velocityReaction;
    float velocitySor;
    float velocityCacheDamping;
    float positionImpulse;
    float positionReaction;
    float positionSor;
    float positionCacheDamping;

    MovingRegression velocityImpulseRegression;
    MovingRegression positionImpulseRegression;
};

//
// Constraint definition: Base class for all constraints and collision classes
//
class Constraint
{
public:
    enum Types
    {
        Types_Collision, // Special constraint type: only generated inside the solver from ContactConnectors
        Types_Align2Axes,
        Types_BallInSocket,
        Types_AngularVelocity,
        Types_LinearVelocity,
        Types_AchievePosition,
        Types_BodyAngularVelocity,
        Types_LinearSpring,
		Types_LegacyBreakableBallInSocket,
        Types_LegacyAngularVelocity,
        Types_Count
    };

    // Number of degrees of freedom constrained
    inline unsigned getDimension() const { return dimensions; }

    inline bool isBroken() const { return broken; }

    // Read from the constraint cache, and call the overloaded build equation
    // This should only be called by the solver
    inline void restoreCacheAndBuildEquation( 
        ConstraintJacobianPair* _jacobian, 
        ConstraintVariables* _velocityStage, 
        ConstraintVariables* _positionStage, 
        float* _sorVel,
        float* _sorPos,
        boost::uint8_t* _useBlock,
        const SolverBodyDynamicProperties& _bodyA, 
        const SolverBodyDynamicProperties& _bodyB, 
        const SolverConfig& _solverConfig, 
        float _dt );

    // Write into the constraint cache
    // This should only be called by the solver
    inline void cache( 
        const ConstraintVariables* _velocityStage, 
        const ConstraintVariables* _positionStage, 
        const float* _sorVel, const float* _sorPos, 
        const SolverConfig& _config );

    // Each breakable constraint will need to implement this, and return /true/ if the constraint changes state to broken
    // This should only be called by the solver
    inline void updateBrokenState(
        const ConstraintVariables* _velocityStage, 
        const ConstraintVariables* _positionStage, 
        const SolverConfig& _config );

    void setBodyA( Body* _a ) { bodyA = _a; }
    void setBodyB( Body* _b ) { bodyB = _b; }
    const Body* getBodyA() const { return bodyA; }
    const Body* getBodyB() const { return bodyB; }
    Body* getBodyA() { return bodyA; }
    Body* getBodyB() { return bodyB; }
    Types getType() const { return type; }

    virtual ~Constraint();

    void setUID( boost::uint64_t _index ) { uid = _index; }
    bool hasValidUID() const { return uid != 0; }
    boost::uint64_t getUID() const { return uid; }
    
    // After the PGS has updated all i  ts iterations, the last iteration reaction delta is passed in as parameter
    enum Convergence
    {
        Convergence_Converges,
        Convergence_Diverges,
        Convergence_Undetermined
    };
    virtual Convergence testPGSConvergence( const float* _disp, const float* _residuals, const float* _deltaResiduals, const SolverConfig& _solverConfig ) { return Convergence_Converges; }
    
    virtual void serialize( DebugSerializer& s ) const;

protected:
    const ConstraintCache& getCache( unsigned d ) const { return cacheData[ d ]; }
    ConstraintCache& getCache( unsigned d ) { return cacheData[ d ]; }

    inline Constraint( Types _type, Body* _bodyA, Body* _bodyB, uint8_t _dimensions );

private:
    // Main constraint interface to the solver
    // Initializes the Jacobian and ConstraintVariables for the two stages
    virtual void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) = 0;

    // Each breakable constraint will need to implement this, and return /true/ if the constraint changes state to broken
    virtual bool computeBrokenState(
        const ConstraintVariables* _velocityStage, 
        const ConstraintVariables* _positionStage, 
        const SolverConfig& _config ) const { return false; }

    // Disable copy constructs
    Constraint( const Constraint& );
    Constraint& operator=( const Constraint& );

protected:
    uint8_t dimensions;
    Types type : 8;
    bool broken : 1;
private:
    Body* bodyA;
    Body* bodyB;
    ConstraintCache* cacheData;
    boost::uint64_t uid; // Current index if registered in the solver
};

//
// Constraint: inline implementation
//
inline Constraint::Constraint( Types _type, Body* _bodyA, Body* _bodyB, uint8_t _dimensions ): type( _type ), dimensions( _dimensions ), bodyA( _bodyA ), bodyB( _bodyB ), uid( 0 ), broken( false )
{
    RBXASSERT( _bodyA != NULL );
    cacheData = new ConstraintCache[ dimensions ];
}

#ifdef __RBX_NOT_RELEASE
static inline void checkConstraintVariables( const ConstraintVariables& _vars )
{
    RBXASSERT( !RBX::Math::isNanInf( _vars.impulse ) );
    RBXASSERT( !RBX::Math::isNanInf( _vars.reaction ) );
    RBXASSERT( !RBX::Math::isNan( _vars.minImpulseValue ) );
    RBXASSERT( !RBX::Math::isNan( _vars.maxImpulseValue ) );
}

static inline void checkJacobian( const ConstraintJacobianPair& _j )
{
    RBXASSERT( !RBX::Math::isNanInfVector3( _j.a.lin ) );
    RBXASSERT( !RBX::Math::isNanInfVector3( _j.b.lin ) );
    RBXASSERT( !RBX::Math::isNanInfVector3( _j.a.ang ) );
    RBXASSERT( !RBX::Math::isNanInfVector3( _j.b.ang ) );
}
#endif

RBX_SIMD_INLINE void Constraint::restoreCacheAndBuildEquation( 
    ConstraintJacobianPair* __restrict _jacobian, 
    ConstraintVariables* __restrict _varsVel, 
    ConstraintVariables* __restrict _varsPos, 
    float* __restrict _sorVel,
    float* __restrict _sorPos,
    boost::uint8_t* _useBlock,
    const SolverBodyDynamicProperties& _bodyA, 
    const SolverBodyDynamicProperties& _bodyB, 
    const SolverConfig& _solverConfig, 
    float _dt )
{
    for( unsigned i = 0; i < dimensions; i++ )
    {
        // Initialize to some reasonable default values
        _varsVel[i].minImpulseValue = -std::numeric_limits<float>::infinity();
        _varsVel[i].maxImpulseValue = std::numeric_limits<float>::infinity();

        _varsPos[i].minImpulseValue = -std::numeric_limits<float>::infinity();
        _varsPos[i].maxImpulseValue = std::numeric_limits<float>::infinity();

        _jacobian[i].reset();
        
        // Unless specified by the constraint, use the entire constraint as a Block in the Gauss-Seidel
        _useBlock[i] = _solverConfig.blockPGSEnabled;

        // Read previous frame impulse/SOR/reaction values
        getCache(i).readCache(_varsVel[i],_varsPos[i], _sorVel[i], _sorPos[i]);

        // Optionally disable the cache
        _varsVel[i].impulse = _solverConfig.velCacheDamping * _varsVel[i].impulse;
        _varsPos[i].impulse = _solverConfig.posCacheDamping * _varsPos[i].impulse;
    }

    buildEquation( _jacobian, _useBlock, _varsVel, _varsPos, _bodyA, _bodyB, _solverConfig, _dt );

    // Run some sanity checks
#ifdef __RBX_NOT_RELEASE
    for( unsigned i = 0; i < dimensions; i++ )
    {
        checkConstraintVariables( _varsVel[ i ] );
        checkConstraintVariables( _varsPos[ i ] );
        checkJacobian( _jacobian[ i ] );
    }
#endif
}

RBX_SIMD_INLINE void Constraint::updateBrokenState(
    const ConstraintVariables* _velocityStage, 
    const ConstraintVariables* _positionStage, 
    const SolverConfig& _config ) 
{
    if( !broken )
    {
        broken = computeBrokenState(_velocityStage, _positionStage, _config);
    }
}

inline void Constraint::cache( const ConstraintVariables* _velocityStage, const ConstraintVariables* _positionStage, const float* _sorVel, const float* _sorPos, const SolverConfig& _config )
{
    // Cache Constraint base class data
    for( unsigned i = 0; i < dimensions; i++ )
    {
        cacheData[ i ].cache( _velocityStage[ i ], _positionStage[ i ], _sorVel[ i ], _sorPos[ i ], getType() == Types_Collision, _config );
    }
}

//
// ConstraintBallInSocket
//
class ConstraintBallInSocket: public Constraint
{
public:
    ConstraintBallInSocket( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_BallInSocket, _bodyA, _bodyB, 3 ) { }
    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock,  ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;

    inline void setPivotA( const Vector3& _pivotA ) { pointA = _pivotA; }
    inline void setPivotB( const Vector3& _pivotB ) { pointB = _pivotB; }

    Convergence testPGSConvergence( const float* _disp, const float* _residuals, const float* _deltaResiduals, const SolverConfig& _solverConfig ) override;
    void serialize( DebugSerializer& s ) const override;

private:
    // Points on object A and B in object space, relative to center of mass
    Vector3 pointA;
    Vector3 pointB;
};

//
// ConstraintLegacyBreakableBallInSocket
//
class ConstraintLegacyBreakableBallInSocket: public Constraint
{
public:
    ConstraintLegacyBreakableBallInSocket( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_LegacyBreakableBallInSocket, _bodyA, _bodyB, 3 ), pointA(0.0f), pointB(0.0f), broken( false ), maxNormalForce( std::numeric_limits<float>::infinity() )
    {
        setNormalOnA(  Vector3(1.0f, 0.0f, 0.0f ) );
    }
    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock,  ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;

    void setPivotA( const Vector3& _pivotA ) { pointA = _pivotA; }
    void setPivotB( const Vector3& _pivotB ) { pointB = _pivotB; }
    void setNormalOnA( const Vector3& _normal );
    void setMaxNormalForce( float _maxForce ) { maxNormalForce = _maxForce; }
    bool computeBrokenState(
        const ConstraintVariables* _velocityStage, 
        const ConstraintVariables* _positionStage, 
        const SolverConfig& _config ) const override;

    void serialize( DebugSerializer& s ) const override;

private:
    // Points on object A and B in object space, relative to center of mass
    Vector3 pointA;
    Vector3 pointB;
    Vector3 normalA;
    Vector3 tangentA1;
    Vector3 tangentA2;
    float maxNormalForce;
    bool broken;
};

//
// ConstraintAlign2Axes
//
class ConstraintAlign2Axes: public Constraint
{
public:
    ConstraintAlign2Axes( Body* _bodyA, Body* _bodyB );
    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;

    void setAxisA( const Vector3& a );
    void setAxisB( const Vector3& b );
    Vector3 getAxisA() const { return axisA; }
    Vector3 getAxisB() const { return axisB; }

    Convergence testPGSConvergence( const float* _disp, const float* _residuals, const float* _deltaResiduals, const SolverConfig& _solverConfig ) override;
    void serialize( DebugSerializer& s ) const override;

private:
    // Axis on body A in object space
    Vector3 axisA;

    // 2 normal axes to the axis on body B
    // In object space
    Vector3 axisB;
    Vector3 orthogonalAxisB1;
    Vector3 orthogonalAxisB2;

    // Cache
    Vector3 worldSpaceOrthogonalB1;
    Vector3 worldSpaceOrthogonalB2;
};

//
// ConstraintAngularVelocity
//
class ConstraintAngularVelocity: public Constraint
{
public:
    ConstraintAngularVelocity( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_AngularVelocity, _bodyA, _bodyB, 1 ), 
        maxForce( 0.0f ), 
        desiredAngularVelocity( 0.0f ) { }
    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;

    void setAxisA( const Vector3& a ) { axisA = a; }
    void setAxisB( const Vector3& b ) { axisB = b; }
    void setDesiredAngularVelocity( float _v ) { desiredAngularVelocity = _v; }
    void setMaxForce( float _f ) { maxForce = _f; }

    void serialize( DebugSerializer& s ) const override;

private:
    Vector3 axisA;
    Vector3 axisB;
    float maxForce;
    float desiredAngularVelocity;
};

//
// ConstraintLinearVelocity
//
class ConstraintLinearVelocity: public Constraint
{
public:
    ConstraintLinearVelocity( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_LinearVelocity, _bodyA, _bodyB, 3 ), maxForce(0.0f), desiredVelocity(0.0f, 0.0f, 0.0f) { }

    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;

    void setDesiredVelocity( const Vector3& _v ) { desiredVelocity = _v; }
    void setMaxForce( const Vector3& _f ) { maxForce = _f; }

    void serialize( DebugSerializer& s ) const override;

private:
    Vector3 maxForce;
    Vector3 desiredVelocity;
};

class ConstraintLinearSpring: public Constraint
{
public:
    ConstraintLinearSpring( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_LinearSpring, _bodyA, _bodyB, 3 ), pivotA(0.0f), pivotB(0.0f), maxForce(0.0f), p(0.0f), d(0.0f) { }

    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;

    void setPivotA( const Vector3& _pivotA ) { pivotA = _pivotA; }
    void setPivotB( const Vector3& _pivotB ) { pivotB = _pivotB; }
    void setMaxForce( const Vector3& _f ) { maxForce = _f; }
    void setPD( float _p, float _d ) { p = _p; d = _d; }

    void serialize( DebugSerializer& s ) const override;

private:
    Vector3 pivotA;
    Vector3 pivotB;
    Vector3 maxForce;
    float p, d;
};

class ConstraintAchievePosition: public Constraint
{
public:
    ConstraintAchievePosition( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_AchievePosition, _bodyA, _bodyB, 3 ), maxForce( 0.0f ), targetVelocity( 0.0f ) { }

    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;

    void setPivotA( const Vector3& _pivotA ) { pivotA = _pivotA; }
    void setPivotB( const Vector3& _pivotB ) { pivotB = _pivotB; }
    void setTargetVelocity( const Vector3& _v ) { targetVelocity = _v; }
    void setMaxForce( const Vector3& _f ) { maxForce = _f; }
    void setMinForce( const Vector3& _f ) { minForce = _f; }

    void serialize( DebugSerializer& s ) const override;

private:
    Vector3 pivotA;
    Vector3 pivotB;
    Vector3 maxForce;
    Vector3 minForce;
    Vector3 targetVelocity;
};

class ConstraintBodyAngularVelocity: public Constraint
{
public:
    ConstraintBodyAngularVelocity( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_BodyAngularVelocity, _bodyA, _bodyB, 3 ), targetAngularVelocity( 0.0f ), maxTorque( 0.0f ), minTorque( 0.0f ), useIntegratedVelocities( false ) { }

    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;

    void serialize( DebugSerializer& s ) const override;

    // The vector values need to be provided in object space of body A
    void setTarget( const Vector3& v ) { targetAngularVelocity = v; }
    void setMaxTorque( const Vector3& v ) { maxTorque = v; }
    void setMinTorque( const Vector3& v ) { minTorque = v; }
    void setUseIntegratedVelocities( bool flag ) { useIntegratedVelocities = flag; }

private:
    Vector3 targetAngularVelocity;
    Vector3 maxTorque;
    Vector3 minTorque;
    bool useIntegratedVelocities;
};

class ConstraintLegacyAngularVelocity: public Constraint
{
public:
    ConstraintLegacyAngularVelocity( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_LegacyAngularVelocity, _bodyA, _bodyB, 3 ), targetAngularVelocity( 0.0f ), maxTorque( 0.0f ), minTorque( 0.0f ), useIntegratedVelocities( false ) { }

    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;

    void serialize( DebugSerializer& s ) const override;

    // The vector values need to be provided in object space of body A
    void setTarget( const Vector3& v ) { targetAngularVelocity = v; }
    void setMaxTorque( const Vector3& v ) { maxTorque = v; }
    void setMinTorque( const Vector3& v ) { minTorque = v; }
    void setUseIntegratedVelocities( bool flag ) { useIntegratedVelocities = flag; }

private:
    Vector3 targetAngularVelocity;
    Vector3 maxTorque;
    Vector3 minTorque;
    bool useIntegratedVelocities;
};

//
// ConstraintCollision
//
class ConstraintCollision: public Constraint
{
public:
    ConstraintCollision( Body* _bodyA, Body* _bodyB ): Constraint( Constraint::Types_Collision, _bodyA, _bodyB, 3 )
    {
        cachedTangent1 = Vector3( 1.0f, 0.0f, 0.0f );
        getCache(0).positionSor = 1.0f;
        getCache(0).velocitySor = 1.0f;
        getCache(1).positionSor = 1.0f;
        getCache(1).velocitySor = 1.0f;
        getCache(2).positionSor = 1.0f;
        getCache(2).velocitySor = 1.0f;
    }

    void buildEquation( ConstraintJacobianPair* _jacobian, boost::uint8_t* _useBlock, ConstraintVariables* _velocityStage, ConstraintVariables* _positionStage, const SolverBodyDynamicProperties& _bodyA, const SolverBodyDynamicProperties& _bodyB, const SolverConfig& _config, float _dt ) override;
    void setNormal( const Vector3& _normal ) { normal = _normal; }
    void setPointA( const Vector3& _pointA ) { pointA = _pointA; }
    void setDepth( float _d ) { depth = _d; }
    void setFriction( float _f ) { friction = _f; }
    void setResititution( float _r ) { restitution = _r; }

    Convergence testPGSConvergence( const float* _disp, const float* _residuals, const float* _deltaResiduals, const SolverConfig& _solverConfig ) override;
    void serialize( DebugSerializer& s ) const override;
private:
    Vector3 normal;
    Vector3 pointA;
    float depth;
    float friction;
    float restitution;

    // Cache
    Vector3 cachedTangent1;
};

}
