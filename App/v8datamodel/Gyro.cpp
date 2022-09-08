#include "stdafx.h"

#include "v8datamodel/Gyro.h"
#include "v8datamodel/Partinstance.h"
#include "V8DataModel/Workspace.h"
#include "V8World/World.h"
#include "V8World/Assembly.h"
#include "V8Kernel/Kernel.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Constants.h"
#include "Network/Players.h"
#include "solver/Solver.h"

DYNAMIC_FASTFLAGVARIABLE(PGSWakePrimitivesWithBodyMoverPropertyChanges, false)
FASTFLAGVARIABLE(PGSUsesConstraintBasedBodyMovers, false)


const char* const  RBX::sBodyMover = "BodyMover";
const char* const  RBX::sBodyPosition = "BodyPosition";
const char* const  RBX::sBodyVelocity = "BodyVelocity";
const char* const  RBX::sBodyGyro = "BodyGyro";
const char* const  RBX::sBodyAngularVelocity = "BodyAngularVelocity";
const char* const  RBX::sBodyForce = "BodyForce";
const char* const  RBX::sBodyThrust = "BodyThrust";
const char* const  RBX::sRocket = "RocketPropulsion";

RBX_REGISTER_CLASS(RBX::BodyGyro);
RBX_REGISTER_CLASS(RBX::BodyForce);
RBX_REGISTER_CLASS(RBX::BodyThrust);
RBX_REGISTER_CLASS(RBX::BodyPosition);
RBX_REGISTER_CLASS(RBX::BodyVelocity);
RBX_REGISTER_CLASS(RBX::BodyAngularVelocity);
RBX_REGISTER_CLASS(RBX::Rocket);


static const float PGSSolverSpringConstantScale = 1.0f/19.0f;

void RBX::registerBodyMovers()
{
	BodyGyro::className();
	BodyPosition::className();
	BodyVelocity::className();
	BodyAngularVelocity::className();
	BodyForce::className();
	BodyThrust::className();
	Rocket::className();
}

using namespace RBX;

BodyMover::BodyMover(const char* name)
	: world(NULL)
{
	setName(name);
}

BodyMover::~BodyMover()
{
	RBXASSERT(world==NULL);
	RBXASSERT(!part.lock());
}

void BodyMover::putInKernel(Kernel* _kernel)
{
	KernelJoint::putInKernel(_kernel);
}


void BodyMover::removeFromKernel()
{
	KernelJoint::removeFromKernel();
}

void BodyMover::computeForce(bool throttling)
{
	RBXASSERT(world);
	RBXASSERT(part.lock());
	RBXASSERT(part.lock()->getPartPrimitive()->getBody());
//	RBXASSERT(part.lock()->getPrimitive()->getAssembly()->inKernel());

	Vector3 force;
	Vector3 torque;
	Body* root;
	computeForce(throttling, root, force, torque);

	//RBXASSERT(root->inKernel());
	root->accumulateForceAtBranchCofm(force);
	root->accumulateTorque(torque);
}

void BodyMover::computeForce(bool throttling, Body* &root, Vector3& force, Vector3& torque)
{
    shared_ptr<PartInstance> pi = part.lock();

	RBXASSERT(force == Vector3::zero());
	if ( !pi || !(pi->getPartPrimitive()) ||
		 !(pi->getPartPrimitive()->getBody()) )
		return;

	Body* body = pi->getPartPrimitive()->getBody();

	root = body->getRoot();

	computeForceImpl(throttling, body, root, force, torque);
}

bool different5percent(const Vector3& v0, const Vector3& v1)
{
	RBXASSERT(Math::fuzzyEq(Vector3(1,1,1), Vector3(1.04,1,1), 0.05f));
	RBXASSERT(!Math::fuzzyEq(Vector3(1,1,1), Vector3(1.2,1,1), 0.05f));

	return (!Math::fuzzyEq(v0, v1, 0.05f));	
}

void BodyMover::stepWorld()
{
	Vector3 force;
	Vector3 torque;
	Body* root;
	computeForce(false, root, force, torque);

	if (different5percent(force, lastWakeForce) || different5percent(torque, lastWakeTorque))
	{
		lastWakeForce = force;
		lastWakeTorque = torque;
		if(world->getUsingPGSSolver())
			world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
		else
			world->ticklePrimitive(part.lock()->getPartPrimitive(), false);
	}
}

Body* BodyMover::getEngineBody() 
{
	if (part.lock()) {
		return part.lock()->getPartPrimitive()->getBody();
	}
	else {
		RBXASSERT(0);
		return NULL;
	}
}




bool BodyMover::duplicateBodyMoverExists(Primitive* p0, Primitive* p1)
{
	int pairId = 0;
	Joint* joint = NULL;
	do
	{
		joint = Primitive::getJoint(p0, p1, pairId);
		if (joint && (typeid(*joint) == typeid(*this)))
		{
			//RBXASSERT(0);	// show to DB, then delete
			return true;
		}
		pairId++;
	}
	while (joint);

	return false;
}

void BodyMover::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);		// Note - moved this to first

	PartInstance* partInstance = NULL;
	World* newWorld = NULL;

	// 1. Update Part
	if (event.child==this) {
		// Immediate parent of the BodyMover changed
		partInstance = Instance::fastDynamicCast<PartInstance>(event.newParent);
		if (partInstance != NULL) {     // getEngineBody() need part during removeJoint()
			part = shared_from<PartInstance>(partInstance);
			newWorld = part.lock() ? Workspace::getWorldIfInWorkspace(this) : NULL;
		}
	} else
		newWorld = part.lock() ? Workspace::getWorldIfInWorkspace(this) : NULL;

	if (world == newWorld && world != NULL)
	{
		if (this->inPipeline()) {
			world->removeJoint(this);
		}
	}

	if (world != NULL && newWorld == NULL) {
		RBXASSERT(world != NULL); 
		if (this->inPipeline()) {
			if (world) {						// TODO - remove this test after a while - asserting now for safety
				world->removeJoint(this);
			}
		}
		this->setPrimitive(0, NULL);
		this->setPrimitive(1, NULL);
	}

	world = newWorld;

	if (newWorld != NULL)
	{
		RBXASSERT(!this->inPipeline());
		Primitive* p0 = part.lock()->getPartPrimitive();
		Primitive* p1 = world->getGroundPrimitive();
		this->setPrimitive(0, p0);
		this->setPrimitive(1, p1);
		if (!duplicateBodyMoverExists(p0, p1)) {
			world->insertJoint(this);
			RBXASSERT(this->inPipeline());
			if (p0->getWorld()) 
				world->ticklePrimitive(p0, false);
		}
	}

	if (event.child==this && partInstance == NULL)			// immediate parent changed to NULL
		part = shared_from<PartInstance>(partInstance);  
}

bool BodyMover::askSetParent(const Instance* instance) const
{
	return Instance::fastDynamicCast<PartInstance>(instance)!=0;
}

REFLECTION_BEGIN();
Reflection::RefPropDescriptor<Rocket, PartInstance> Rocket::prop_Target("Target", "Goals", &Rocket::getTargetDangerous, &Rocket::setTarget);
Reflection::BoundProp<Vector3> Rocket::prop_targetOffset("TargetOffset", "Goals", &Rocket::targetOffset, &Rocket::onGoalChanged);
Reflection::BoundProp<float> Rocket::prop_targetRadius("TargetRadius", "Goals", &Rocket::targetRadius, &Rocket::onGoalChanged);
Reflection::BoundProp<float> Rocket::prop_MaxSpeed("MaxSpeed", "Thrust", &Rocket::maxSpeed);
Reflection::BoundProp<float> Rocket::prop_MaxThrust("MaxThrust", "Thrust", &Rocket::maxThrust);
Reflection::BoundProp<float> Rocket::prop_ThrustP("ThrustP", "Thrust", &Rocket::kThrustP);
Reflection::BoundProp<float> Rocket::prop_ThrustD("ThrustD", "Thrust", &Rocket::kThrustD);
Reflection::BoundProp<float> Rocket::prop_TurnP("TurnP", "Turn", &Rocket::kTurnP);
Reflection::BoundProp<float> Rocket::prop_TurnD("TurnD", "Turn", &Rocket::kTurnD);
Reflection::BoundProp<Vector3> Rocket::prop_MaxTorque("MaxTorque", "Turn", &Rocket::maxTorque);
Reflection::BoundProp<float> Rocket::prop_CartoonFactor("CartoonFactor", "Goals", &Rocket::cartoonFactor);
Reflection::BoundProp<bool> Rocket::prop_Active("Active", "Internal", &Rocket::active, Reflection::PropertyDescriptor::REPLICATE_ONLY);

Reflection::BoundFuncDesc<Rocket, void()> Rocket::func_Fire(&Rocket::fire, "Fire", Security::None);
static Reflection::BoundFuncDesc<Rocket, void()> func_fire(&Rocket::fire, "fire", Security::None, Reflection::Descriptor::Attributes::deprecated(Rocket::func_Fire));
Reflection::BoundFuncDesc<Rocket, void()> Rocket::func_Abort(&Rocket::abort, "Abort", Security::None);

static Reflection::RemoteEventDesc<Rocket, void()> event_ReachedTarget(&Rocket::reachedTargetSignal, "ReachedTarget", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::BROADCAST);
REFLECTION_END();

Rocket::Rocket()
	:DescribedCreatable<Rocket, BodyMover, sRocket>(sRocket)
	,maxThrust(4e+003)
	,maxSpeed(30)
	,kThrustP(5.0f)
	,kThrustD(0.001f)
	,kTurnP(3000.0f)
	,kTurnD(500.0f)
	,maxTorque(4e5f, 4e5f, 0)
	,targetOffset(0,0,0)
	,targetRadius(4)
	,firedEvent(false)
	,cartoonFactor(0.7)
	,active(false)
{
	FASTLOG1(FLog::ISteppedLifetime, "Rocket created - %p", this);
}

Rocket::~Rocket()
{
	FASTLOG1(FLog::ISteppedLifetime, "Rocket destroyed - %p", this);
}

void Rocket::onStepped(const Stepped& event)
{
	if (world && part.lock() && active && !firedEvent)
	{
		G3D::Vector3 pos = part.lock()->getCoordinateFrame().translation;
		G3D::Vector3 targetPos = target ? target->getCoordinateFrame().pointToWorldSpace(targetOffset) : targetOffset;
		float d2 = (pos-targetPos).squaredLength();
		if (d2<=targetRadius*targetRadius)
		{
			reachedTargetSignal();
			firedEvent = true;
		}
	}
}

void Rocket::fire()
{
	prop_Active.setValue(this, true);
}

void Rocket::abort()
{
	prop_Active.setValue(this, false);
}

void Rocket::setTarget(PartInstance* value)
{
	if (target.get()!=value)
	{
		target = shared_from(value);
		firedEvent = false;
		this->raisePropertyChanged(prop_Target);
	}
}

void Rocket::computeForceImpl(	bool throttling, 
								Body* body, 
								Body* root, 
								Vector3& force, 
								Vector3& torque)
{
	RBXASSERT(world);

	if (!active) {
		return;
	}

	// First determine where we want to go:
	Vector3 targetPos = target ? target->getCoordinateFrame().pointToWorldSpace(targetOffset) : targetOffset;
	Vector3 targetDeltaP = targetPos - body->getCoordinateFrame().translation;
	Vector3 targetDir = targetDeltaP.fastDirection();

    if( world && world->getUsingPGSSolver() )
    {
        // Apply thrust in the direction opposite to other forces
		force = -root->getBranchForce();

        // Compute the left-over available thrust
        float s = targetDir.dot( force );
        float t = maxThrust * maxThrust - force.dot(force) + s * s;
        if( t > 0.0f )
        {
            t = sqrtf( t ) - s;
        }
        else
        {
            t = 0.0f;
        }

        // Apply thrust in the direction of the target
        force += t * targetDir;

        // Clamp the user set maxSpeed to reasonable values
        float clampedMaxSpeed = std::min( 10000.0f, std::max( maxSpeed, 1.0f ) );

        // Compute drag constant so that the limit velocity is equal to maxSpeed
        float dragK = maxThrust / ( clampedMaxSpeed * clampedMaxSpeed );

        // Drag force is proportional to the square of the velocity
        Vector3 vel = root->getVelocity().linear;
        force -= ( dragK * vel.magnitude() ) * vel;

		force = force.clamp(G3D::Vector3(-maxThrust, -maxThrust, -maxThrust), G3D::Vector3(maxThrust, maxThrust, maxThrust));
    }
    else
    {
	    // P control system
	    const Vector3 pAccel = kThrustP * targetDeltaP;

	    force = root->getBranchMass() * pAccel;

	    // Honor the maxSpeed property
	    double speed2 = root->getVelocity().linear.squaredMagnitude();
	    if (speed2>maxSpeed*maxSpeed)
	    {
		    G3D::Vector3 rocketDirection = root->getVelocity().linear.fastDirection();
		    float component = force.dot(rocketDirection);
		    if (component>0)
		    {
			    // crop the component of force along the rocket's direction of travel
			    force -= rocketDirection * component;
			    // Apply a braking impulse
			    force -= root->getBranchMass() * (root->getVelocity().linear - maxSpeed * rocketDirection) / Constants::kernelDt();
		    }
	    }

	    // D control system	
	    force -= kThrustD * root->getBranchMass() * root->getVelocity().linear;

	    // Overcome current forces on the part:
		force -= root->getBranchForce();

	    Math::fixDenorm(force);

	    force = force.clamp(G3D::Vector3(-maxThrust, -maxThrust, -maxThrust), G3D::Vector3(maxThrust, maxThrust, maxThrust));
    }

	///////////////////////////////////////////////////////////////////////////
	// Now turn the rocket to point in the direction of thrust

	// TODO: Can this be done better?
	Vector3 forceDirection = (force.squaredLength() > 1e-6f) ? force.fastDirection() : Vector3::zero();

	Vector3 dir = cartoonFactor * targetDir + (1.0f - cartoonFactor) * forceDirection;

	torque = computeTorque(body, root, dir.fastDirection());
}


// This is called frequently during simulation, so try to keep it fast!!!!
Vector3 Rocket::computeTorque(Body* body, Body* root, const G3D::Vector3& targetDir)
{
	G3D::Vector3 localZAxis = body->getCoordinateFrame().vectorToObjectSpace(targetDir);

	const Vector3 oldTorqueWorld = root->getBranchTorque();

    float newKP = kTurnP;
    if( world && world->getUsingPGSSolver() )
    {
        newKP = kTurnP * PGSSolverSpringConstantScale;
    }

	// P control system
	const float desiredTorqueX = newKP * root->getBranchIBodyV3().x * localZAxis.y;
	const float desiredTorqueY = - newKP * root->getBranchIBodyV3().y * localZAxis.x;

	Vector3 torqueBody = body->getCoordinateFrame().vectorToObjectSpace(oldTorqueWorld);

	if (fabs(desiredTorqueX - torqueBody.x) < (maxTorque.x * root->getBranchIBodyV3().x))
		torqueBody.x = desiredTorqueX;
	else
		torqueBody.x += desiredTorqueX;

	if (fabs(desiredTorqueY - torqueBody.y) < (maxTorque.y * root->getBranchIBodyV3().y))
		torqueBody.y = desiredTorqueY;
	else
		torqueBody.y += desiredTorqueY;

    float newKD = kTurnD;
    if( world && world->getUsingPGSSolver() )
    {
        newKD = kTurnD * PGSSolverSpringConstantScale;
    }

	// D control system
	const Vector3 angVelBody = body->getCoordinateFrame().vectorToObjectSpace(body->getVelocity().rotational);
	torqueBody.x -= newKD * root->getBranchIBodyV3().x * angVelBody.x;
	torqueBody.y -= newKD * root->getBranchIBodyV3().y * angVelBody.y;

	const Vector3 desiredTorqueWorld = body->getCoordinateFrame().vectorToWorldSpace(torqueBody);

	return desiredTorqueWorld - oldTorqueWorld;
}


Reflection::BoundProp<float> BodyGyro::prop_kP("P", "Goals", &BodyGyro::kP, &BodyGyro::onPDChanged);
Reflection::BoundProp<float> BodyGyro::prop_kD("D", "Goals", &BodyGyro::kD, &BodyGyro::onPDChanged);
Reflection::PropDescriptor<BodyGyro, G3D::Vector3> BodyGyro::prop_maxTorque("MaxTorque", "Goals", &BodyGyro::getMaxTorque, &BodyGyro::setMaxTorque);
Reflection::PropDescriptor<BodyGyro, G3D::Vector3> BodyGyro::prop_maxTorqueDeprecated("maxTorque", "Goals", &BodyGyro::getMaxTorque, &BodyGyro::setMaxTorque, Reflection::PropertyDescriptor::Attributes::deprecated(BodyGyro::prop_maxTorque));
Reflection::PropDescriptor<BodyGyro, CoordinateFrame> BodyGyro::prop_cframe("CFrame", "Goals", &BodyGyro::getCFrame, &BodyGyro::setCFrame);
Reflection::PropDescriptor<BodyGyro, CoordinateFrame> BodyGyro::prop_cframeDeprecated("cframe", "Goals", &BodyGyro::getCFrame, &BodyGyro::setCFrame, Reflection::PropertyDescriptor::Attributes::deprecated(BodyGyro::prop_cframe));

BodyGyro::BodyGyro(void)
	:DescribedCreatable<BodyGyro, BodyMover, sBodyGyro>(sBodyGyro)
	,kP(3000.0f)
	,kD(500.0f)
	,maxTorque(4e5f, 0, 4e5f)
	,angularVelocityConstraint(0)
{
}

BodyGyro::~BodyGyro()
{
    if (angularVelocityConstraint)
    {
        delete angularVelocityConstraint;
    }
}

void BodyGyro::stepWorld()
{
    if (!world->getUsingPGSSolver() || !FFlag::PGSUsesConstraintBasedBodyMovers)
    {
        BodyMover::stepWorld();
    }
}

void BodyGyro::putInKernel(Kernel* _kernel)
{
	BodyMover::putInKernel(_kernel);

	if (FFlag::PGSUsesConstraintBasedBodyMovers && getKernel()->getUsingPGSSolver())
    {
        Body* b0 = getPrimitive(0)->getBody();
        Body* b1 = getPrimitive(1)->getBody();
        if( angularVelocityConstraint != NULL && ( angularVelocityConstraint->getBodyA()->getUID() != b0->getUID() || angularVelocityConstraint->getBodyB()->getUID() != b1->getUID() ) )
        {
            delete angularVelocityConstraint;
            angularVelocityConstraint = NULL;
        }
        update();
        _kernel->pgsSolver.addConstraint( angularVelocityConstraint );
    }
}

void BodyGyro::removeFromKernel()
{
	if (FFlag::PGSUsesConstraintBasedBodyMovers && getKernel()->getUsingPGSSolver())
    {
        if (angularVelocityConstraint)
        {
            getKernel()->pgsSolver.removeConstraint( angularVelocityConstraint );
        }
    }

	BodyMover::removeFromKernel();
}

void BodyGyro::update()
{
    if (FFlag::PGSUsesConstraintBasedBodyMovers && angularVelocityConstraint == NULL)
    {
        Body* b0 = getPrimitive(0)->getBody();
        Body* b1 = getPrimitive(1)->getBody();
        angularVelocityConstraint = new ConstraintBodyAngularVelocity( b0, b1 );
    }
}

void BodyGyro::computeForceImpl(	bool throttling, 
									Body* body, 
									Body* root, 
									Vector3& force, 
									Vector3& torque)
{
	RBXASSERT(world);
	RBXASSERT(part.lock());

	torque = computeBalanceTorque(body, root);
	torque += computeOrientationTorque(body, root);

    if(FFlag::PGSUsesConstraintBasedBodyMovers && world && world->getUsingPGSSolver() && angularVelocityConstraint != NULL)
    {
        Vector3 torqueBody = body->getCoordinateFrame().vectorToObjectSpace( torque );
        Vector3 velBody = body->getCoordinateFrame().vectorToObjectSpace( body->getVelocity().rotational );
        Vector3 deltaAngularVelocity = ( torqueBody / root->getBranchIBodyV3() ) * Constants::worldDt();
        Vector3 actualMaxTorque = root->getBranchIBodyV3() * maxTorque;

        angularVelocityConstraint->setTarget( deltaAngularVelocity + velBody );
        angularVelocityConstraint->setMaxTorque( actualMaxTorque );
        angularVelocityConstraint->setMinTorque( -actualMaxTorque);
        torque = Vector3(0.0f);
    }
}

static void correctPDValuesForTimeStep( float &p, float &d )
{
    float discriminant = 4.0f * p - d * d;
    if( discriminant >= 0.0f )
    {
        float w = 0.5f * sqrtf( discriminant );

        // Clamp the frequency to maximum we can handle
        static float thresholdAngularStep = 3.14159f/6.0f;
        if( w * Constants::worldDt() > thresholdAngularStep )
        {
            float s = thresholdAngularStep / ( w * Constants::worldDt() );
            d = d * s;
            p = p * ( s * s );
        }
    }
    else
    {
        // Here is the theory behind this condition:
        // If x0 and v0 are initial position and velocity then the solution to the over-damped spring equation is:
        // v(t) = ( v0-w1*x0 ) * w0 * exp( w0*t ) / D + ( x0*w0-v0 ) * w1 * exp( w1*t ) / D
        // where 
        // w0 = ( -d + sqrt(d^2-4p) ) / 2
        // w1 = ( -d - sqrt(d^2-4p) ) / 2
        // D^2 = d^2-4p
        // Taking the Taylor series expansion of this around 0, we have:
        // v(t) = v0( 1 - d*t + 0.5*(d^2 - p)*t^2 +... ) + x0( -p*t + 0.5*d*p*t^2 +... )
        // So our integration step is precisely the first degree terms.
        // For this to be stable, we need the higher degree terms to be smaller than the first degree term:
        // d >> 0.5*(d^2-p)*t
        // p >> 0.5*d*p*t
        // or:
        // 1 >> 0.5*(d-p/d)*t
        // 1 >> 0.5*d*t
        // But since d*t > (d-p/d)*t we only need:
        // 1 >> 0.5*d*t
        //
        // We rescale p and d in such a way to preserve the the property of being over-damped: d->s*d, p->s^2*p

        // Clamp the step to the maximum we can handle
        static float threshold = 0.8f;
        float w = 0.5f * d;
        if( w > threshold / Constants::worldDt() )
        {
            float s = threshold / ( w * Constants::worldDt() );
            d = d * s;
            p = p * ( s * s );
        }
    }
}

// This is called frequently during simulation, so try to keep it fast!!!!
Vector3 BodyGyro::computeBalanceTorque(Body* body, Body* root)
{
	if (G3D::fuzzyEq(maxTorque.x, 0) && G3D::fuzzyEq(maxTorque.z, 0)) {
		return Vector3::zero();
	}

    const Vector3 oldTorqueWorld = root->getBranchTorque();

    // 1. Compute yAxis
    const Vector3 localYAxis = body->getCoordinateFrame().vectorToObjectSpace(cframe.upVector());

    float newKP = kP;
    float newKD = kD;
    if( world && world->getUsingPGSSolver() )
    {
        if( FFlag::PGSUsesConstraintBasedBodyMovers )
        {
            correctPDValuesForTimeStep(newKP, newKD);

            float fitnessX = 1.0f;
            float fitnessZ = 1.0f;
            static float fitnessSensitivity = 2.0f;

            float t = instabilityDetectorX.testFitNextDataPointSecondOrder(localYAxis.x);
            fitnessZ = expf( -fitnessSensitivity * t );
            instabilityDetectorX.addDataPoint(localYAxis.x, 1.0f);

            t = instabilityDetectorZ.testFitNextDataPointSecondOrder(localYAxis.z);
            fitnessX = expf( -fitnessSensitivity * t );
            instabilityDetectorZ.addDataPoint(localYAxis.z, 1.0f);

            const Vector3 angVelBody = body->getCoordinateFrame().vectorToObjectSpace(body->getVelocity().rotational);

            // Control system
            float desiredAccelerationX =  newKP * localYAxis.z - newKD * angVelBody.x;
            float desiredAccelerationZ = -newKP * localYAxis.x - newKD * angVelBody.z;

            // Computing the torque
            float desiredTorqueX = desiredAccelerationX * fitnessX * root->getBranchIBodyV3().x;
            float desiredTorqueZ = desiredAccelerationZ * fitnessZ * root->getBranchIBodyV3().z;

            // Transforming into world space
            return body->getCoordinateFrame().vectorToWorldSpace(Vector3(desiredTorqueX, 0.0f, desiredTorqueZ));
        }
        else
        {
            newKP = kP * PGSSolverSpringConstantScale;
            newKD = kD * PGSSolverSpringConstantScale;
        }
    }

    // P control system
    const float desiredTorqueX = newKP * root->getBranchIBodyV3().x * localYAxis.z;
    const float desiredTorqueZ = - newKP * root->getBranchIBodyV3().z * localYAxis.x;

    Vector3 torqueBody = body->getCoordinateFrame().vectorToObjectSpace(oldTorqueWorld);

    if (fabs(desiredTorqueX - torqueBody.x) < (maxTorque.x * root->getBranchIBodyV3().x))
        torqueBody.x = desiredTorqueX;
    else
        torqueBody.x += desiredTorqueX;

    if (fabs(desiredTorqueZ - torqueBody.z) < (maxTorque.z * root->getBranchIBodyV3().z))
        torqueBody.z = desiredTorqueZ;
    else
        torqueBody.z += desiredTorqueZ;

    // D control system
    const Vector3 angVelBody = body->getCoordinateFrame().vectorToObjectSpace(body->getVelocity().rotational);
    torqueBody.x -= newKD * root->getBranchIBodyV3().x * angVelBody.x;
    torqueBody.z -= newKD * root->getBranchIBodyV3().z * angVelBody.z;

    const Vector3 desiredTorqueWorld = body->getCoordinateFrame().vectorToWorldSpace(torqueBody);
    const Vector3 addedTorque = desiredTorqueWorld - oldTorqueWorld;

    return addedTorque;
}

Vector3 BodyGyro::computeOrientationTorque(Body* body, Body* root)
{
	if (G3D::fuzzyEq(maxTorque.y, 0)) {
		return Vector3::zero();
	}

    // 1. Compute znegAxis
    const Vector3 localZNegAxis = body->getCoordinateFrame().vectorToObjectSpace(cframe.lookVector());

    float newKP = kP;
    float newKD = kD;
    if( world && world->getUsingPGSSolver() )
    {
        if( FFlag::PGSUsesConstraintBasedBodyMovers )
        {
            float fitness = 1.0f;
            static float fitnessSensitivity = 2.0f;

            correctPDValuesForTimeStep(newKP, newKD);
            float t = instabilityDetectorY.testFitNextDataPointSecondOrder(localZNegAxis.x);
            fitness = expf( -fitnessSensitivity * t );
            instabilityDetectorY.addDataPoint(localZNegAxis.x, 1.0f);

            const Vector3 angVelBody = body->getCoordinateFrame().vectorToObjectSpace(body->getVelocity().rotational);
            float desiredTorqueY = - fitness * root->getBranchIBodyV3().y * ( newKP * localZNegAxis.x + newKD * angVelBody.y );
            return body->getCoordinateFrame().vectorToWorldSpace( Vector3(0.0f,desiredTorqueY, 0.0f));
        }
        else
        {
            newKP = kP * PGSSolverSpringConstantScale;
            newKD = kD * PGSSolverSpringConstantScale;
        }
    }

    const Vector3 oldTorqueWorld = root->getBranchTorque();

    // P control system
    const float desiredTorqueY = - newKP * root->getBranchIBodyV3().y * localZNegAxis.x;

    Vector3 torqueBody = body->getCoordinateFrame().vectorToObjectSpace(oldTorqueWorld);

    if (fabs(desiredTorqueY - torqueBody.y) < (maxTorque.y * root->getBranchIBodyV3().y))
        torqueBody.y = desiredTorqueY;
    else
        torqueBody.y += desiredTorqueY;

    // D control system
    const Vector3 angVelBody = body->getCoordinateFrame().vectorToObjectSpace(body->getVelocity().rotational);
    torqueBody.y -= newKD * root->getBranchIBodyV3().y * angVelBody.y;

    const Vector3 desiredTorqueWorld = body->getCoordinateFrame().vectorToWorldSpace(torqueBody);
    const Vector3 addedTorque = desiredTorqueWorld - oldTorqueWorld;

    return addedTorque;
}

void BodyGyro::setMaxTorque(Vector3 value)
{
	if (value != maxTorque)
	{
		maxTorque = value;
		raisePropertyChanged(BodyGyro::prop_maxTorque);

		if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
			world->ticklePrimitive(part.lock()->getPartPrimitive(), true);

	}
}

void BodyGyro::setCFrame(CoordinateFrame value)
{
	if (value != cframe)
	{
		cframe = value;
		raisePropertyChanged(BodyGyro::prop_cframe);

		if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
			world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
	}
}

void BodyGyro::onPDChanged(const Reflection::PropertyDescriptor&)
{
	if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
		world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
}

Reflection::BoundProp<float> BodyPosition::prop_kP("P", "Goals", &BodyPosition::kP, &BodyPosition::onPDChanged);
Reflection::BoundProp<float> BodyPosition::prop_kD("D", "Goals", &BodyPosition::kD, &BodyPosition::onPDChanged);
Reflection::PropDescriptor<BodyPosition, G3D::Vector3> BodyPosition::prop_maxForce("MaxForce", "Goals", &BodyPosition::getMaxForce, &BodyPosition::setMaxForce);
Reflection::PropDescriptor<BodyPosition, G3D::Vector3> BodyPosition::prop_maxForceDeprecated("maxForce", "Goals", &BodyPosition::getMaxForce, &BodyPosition::setMaxForce, Reflection::PropertyDescriptor::Attributes::deprecated(BodyPosition::prop_maxForce));
Reflection::PropDescriptor<BodyPosition, G3D::Vector3> BodyPosition::prop_position("Position", "Goals", &BodyPosition::getPosition, &BodyPosition::setPosition);
Reflection::PropDescriptor<BodyPosition, G3D::Vector3> BodyPosition::prop_positionDeprecated("position", "Goals", &BodyPosition::getPosition, &BodyPosition::setPosition, Reflection::PropertyDescriptor::Attributes::deprecated(BodyPosition::prop_position));

// Query for debugging:
// TODO: Can we fix Reflection to allow const member functions???
RBX::Reflection::BoundFuncDesc<BodyPosition, G3D::Vector3()> func_getLastForce(&BodyPosition::getLastForce, "GetLastForce", Security::None);
RBX::Reflection::BoundFuncDesc<BodyPosition, G3D::Vector3()> func_getLastForceOld(&BodyPosition::getLastForce, "lastForce", Security::None, Reflection::PropertyDescriptor::Attributes::deprecated(func_getLastForce));

static Reflection::RemoteEventDesc<BodyPosition, void()> event_BodyPositionReachedTarget(&BodyPosition::reachedTargetSignal, "ReachedTarget", 
																						 Security::None, Reflection::RemoteEventCommon::SCRIPTING,
																						 Reflection::RemoteEventCommon::BROADCAST);

BodyPosition::BodyPosition(void)
	:DescribedCreatable<BodyPosition, BodyMover, sBodyPosition>(sBodyPosition)
	,kP(1e4f)
	,kD(1250.0f)
	,maxForce(4000.0f, 4000.0f, 4000.0f)
	,position(Vector3(0, 50.0f, 0))
	,firedEvent(false)
	,spring(0)
{
}

BodyPosition::~BodyPosition()
{
    if (spring)
    {
        delete spring;
    }
}

void BodyPosition::stepWorld()
{
    if (!world->getUsingPGSSolver() || !FFlag::PGSUsesConstraintBasedBodyMovers)
    {
        BodyMover::stepWorld();
    }
}

void BodyPosition::putInKernel(Kernel* _kernel)
{
	BodyMover::putInKernel(_kernel);

	if ( FFlag::PGSUsesConstraintBasedBodyMovers && world && world->getUsingPGSSolver() )
    {
        Body* b0 = getPrimitive(0)->getBody();
        Body* b1 = getPrimitive(1)->getBody();

        if( spring != NULL && ( spring->getBodyA()->getUID() != b0->getUID() || spring->getBodyB()->getUID() != b1->getUID() ) )
        {
            delete spring;
            spring = NULL;
        }
        update();
        _kernel->pgsSolver.addConstraint(spring);
    }
}


void BodyPosition::removeFromKernel()
{
    if ( FFlag::PGSUsesConstraintBasedBodyMovers && world && world->getUsingPGSSolver() )
    {
        if (spring)
        {
            getKernel()->pgsSolver.removeConstraint(spring);
        }
    }
	BodyMover::removeFromKernel();
}

void BodyPosition::update()
{
    if ( FFlag::PGSUsesConstraintBasedBodyMovers && world && world->getUsingPGSSolver() && !spring )
    {
        Body* b0 = getPrimitive(0)->getBody();
        Body* b1 = getPrimitive(1)->getBody();
        spring = new ConstraintLinearSpring( b0, b1 );
    }
}

// This is called frequently during simulation, so try to keep it fast!!!!
void BodyPosition::computeForceImpl(
									bool throttling, 
									Body* body, 
									Body* root, 
									Vector3& force, 
									Vector3& torque)
{
	RBXASSERT(world);
	RBXASSERT(part.lock());

	if (FFlag::PGSUsesConstraintBasedBodyMovers && world && world->getUsingPGSSolver())
	{
        float p = kP;
        float d = kD;
        correctPDValuesForTimeStep(p,d);

        spring->setMaxForce( maxForce );
        spring->setPD(p, d);
        spring->setPivotA(spring->getBodyA()->getCoordinateFrame().pointToObjectSpace(spring->getBodyA()->getRootSimBody()->getPV().position.translation));
        spring->setPivotB(spring->getBodyB()->getCoordinateFrame().pointToObjectSpace(position + spring->getBodyA()->getRootSimBody()->getPV().position.translation - spring->getBodyA()->getCoordinateFrame().translation ));
	}
	else
    {
		float newKP = kP;
		float newKD = kD;
		if( world && world->getUsingPGSSolver() )
		{
			newKP = kP * PGSSolverSpringConstantScale;
			newKD = kD * PGSSolverSpringConstantScale;
		}

		// P control system
		const Vector3 pAccel = newKP * (position - body->getCoordinateFrame().translation);

		// D control system
		const Vector3 dAccel = -newKD * body->getVelocity().linear;

		lastForce = body->getRoot()->getBranchMass() * (pAccel + dAccel);
		lastForce = lastForce.clamp(-maxForce, maxForce);
	
		force = lastForce;
	}
}

void BodyPosition::onStepped( const Stepped& event )
{
	static const float BODY_POSITION_TARGET_RADIUS = 0.1;
	if (world && part.lock() && !firedEvent)
	{
		G3D::Vector3 pos = part.lock()->getCoordinateFrame().translation;
		float d2 = (pos - position).squaredLength();
		if (d2 <= BODY_POSITION_TARGET_RADIUS * BODY_POSITION_TARGET_RADIUS)
		{
			reachedTargetSignal();
			firedEvent = true;
		}
	}
}

void BodyPosition::setMaxForce(Vector3 value)
{
	if (value != maxForce)
	{
		maxForce = value;
		raisePropertyChanged(BodyPosition::prop_maxForce);

		if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
			world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
	}
}

void BodyPosition::setPosition(Vector3 value)
{
	if (value != position)
	{
		position = value;
		firedEvent = false;
		raisePropertyChanged(BodyPosition::prop_position);

		if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
			world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
	}
}

void BodyPosition::onPDChanged(const Reflection::PropertyDescriptor&)
{
	if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
		world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
}


Reflection::BoundProp<float> BodyVelocity::prop_kP("P", "Goals", &BodyVelocity::kP, &BodyVelocity::onPChanged);
Reflection::PropDescriptor<BodyVelocity, G3D::Vector3> BodyVelocity::prop_maxForce("MaxForce", "Goals", &BodyVelocity::getMaxForce, &BodyVelocity::setMaxForce);
Reflection::PropDescriptor<BodyVelocity, G3D::Vector3> BodyVelocity::prop_maxForceDeprecated("maxForce", "Goals", &BodyVelocity::getMaxForce, &BodyVelocity::setMaxForce, Reflection::PropertyDescriptor::Attributes::deprecated(BodyVelocity::prop_maxForce));
Reflection::PropDescriptor<BodyVelocity, G3D::Vector3> BodyVelocity::prop_velocity("Velocity", "Goals", &BodyVelocity::getVelocity, &BodyVelocity::setVelocity);
Reflection::PropDescriptor<BodyVelocity, G3D::Vector3> BodyVelocity::prop_velocityDeprecated("velocity", "Goals", &BodyVelocity::getVelocity, &BodyVelocity::setVelocity, Reflection::PropertyDescriptor::Attributes::deprecated(BodyVelocity::prop_velocity));

// Query for debugging:
// TODO: Can we fix Reflection to allow const member functions???
RBX::Reflection::BoundFuncDesc<BodyVelocity, G3D::Vector3()> func_getLastForceVOld(&BodyVelocity::getLastForce, "lastForce", Security::None);
RBX::Reflection::BoundFuncDesc<BodyVelocity, G3D::Vector3()> func_getLastForceV(&BodyVelocity::getLastForce, "GetLastForce", Security::None);

BodyVelocity::BodyVelocity(void)
	:DescribedCreatable<BodyVelocity, BodyMover, sBodyVelocity>(sBodyVelocity)
	,kP(1250.0)
	,maxForce(4000.0f, 4000.0f, 4000.0f)
	,velocity(Vector3(0, 2.0f, 0))
    ,linearVelocity(NULL)
{
}

void BodyVelocity::putInKernel(Kernel* _kernel)
{
    BodyMover::putInKernel(_kernel);

    if (FFlag::PGSUsesConstraintBasedBodyMovers && getKernel()->getUsingPGSSolver())
    {
        Body* b0 = getPrimitive(0)->getBody();
        Body* b1 = getPrimitive(1)->getBody();
        if( linearVelocity != NULL && ( linearVelocity->getBodyA()->getUID() != b0->getUID() || linearVelocity->getBodyB()->getUID() != b1->getUID() ) )
        {
            delete linearVelocity;
            linearVelocity = NULL;
        }
        update();
        _kernel->pgsSolver.addConstraint( linearVelocity );
    }
}

void BodyVelocity::removeFromKernel()
{
    if (FFlag::PGSUsesConstraintBasedBodyMovers && getKernel()->getUsingPGSSolver())
    {
        if (linearVelocity)
        {
            getKernel()->pgsSolver.removeConstraint( linearVelocity );
        }
    }

    BodyMover::removeFromKernel();
}

void BodyVelocity::update()
{
    if (FFlag::PGSUsesConstraintBasedBodyMovers && linearVelocity == NULL)
    {
        Body* b0 = getPrimitive(0)->getBody();
        Body* b1 = getPrimitive(1)->getBody();
        linearVelocity = new ConstraintLinearVelocity( b0, b1 );
    }
}

void BodyVelocity::computeForceImpl(
									bool throttling, 
									Body* body, 
									Body* root, 
									Vector3& force, 
									Vector3& torque)
{
    float newKP = kP;
    if( world && world->getUsingPGSSolver() )
    {
        if( FFlag::PGSUsesConstraintBasedBodyMovers )
        {
            // Clamp newKP to a maximum that's reasonable (90 degrees / step)
            // newKP = std::min( newKP, 0.5f / Constants::worldDt() );
            linearVelocity->setDesiredVelocity( velocity );
            linearVelocity->setMaxForce( maxForce );
            return;
        }
        else
        {
            newKP = kP * PGSSolverSpringConstantScale;
        }
    }

    // P control system
	RBXASSERT(body->getRoot());
	Vector3 pAccel = newKP * (velocity - body->getRoot()->getBranchVelocity().linear);

	lastForce = body->getRoot()->getBranchMass() * pAccel;
	lastForce = lastForce.clamp(-maxForce, maxForce);
	
	// Note - now accumulates force at the Branch (assembly) cofm to prevent rotational torque
	force = lastForce;
}

void BodyVelocity::setMaxForce(Vector3 value)
{
	if (value != maxForce)
	{
		maxForce = value;
		raisePropertyChanged(BodyVelocity::prop_maxForce);

		if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
			world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
	}
}

void BodyVelocity::setVelocity(Vector3 value)
{
	if (value != velocity)
	{
		velocity = value;
		raisePropertyChanged(BodyVelocity::prop_velocity);

		if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
			world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
	}
}

void BodyVelocity::onPChanged(const Reflection::PropertyDescriptor&)
{
	if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
		world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
}

Reflection::BoundProp<float> BodyAngularVelocity::prop_kP("P", "Goals", &BodyAngularVelocity::kP, &BodyAngularVelocity::onPChanged);
Reflection::PropDescriptor<BodyAngularVelocity, Vector3> BodyAngularVelocity::prop_maxTorque("MaxTorque", "Goals", &BodyAngularVelocity::getMaxTorque, &BodyAngularVelocity::setMaxTorque);
Reflection::PropDescriptor<BodyAngularVelocity, Vector3> BodyAngularVelocity::prop_maxTorqueDeprecated("maxTorque", "Goals", &BodyAngularVelocity::getMaxTorque, &BodyAngularVelocity::setMaxTorque, Reflection::PropertyDescriptor::Attributes::deprecated(BodyAngularVelocity::prop_maxTorque));
Reflection::PropDescriptor<BodyAngularVelocity, G3D::Vector3> BodyAngularVelocity::prop_angularvelocity("AngularVelocity", "Goals", &BodyAngularVelocity::getAngularVelocity, &BodyAngularVelocity::setAngularVelocity);
Reflection::PropDescriptor<BodyAngularVelocity, G3D::Vector3> BodyAngularVelocity::prop_angularvelocityDeprecated("angularvelocity", "Goals", &BodyAngularVelocity::getAngularVelocity, &BodyAngularVelocity::setAngularVelocity, Reflection::PropertyDescriptor::Attributes::deprecated(BodyAngularVelocity::prop_angularvelocity));

BodyAngularVelocity::BodyAngularVelocity(void)
	:DescribedCreatable<BodyAngularVelocity, BodyMover, sBodyAngularVelocity>(sBodyAngularVelocity)
	,kP(1250.0)
	,maxTorque(4000.0f, 4000.0f, 4000.0f)
	,angularvelocity(Vector3(0, 2.0f, 0))
    ,angularVelocityConstraint(NULL)
{
}


void BodyAngularVelocity::putInKernel(Kernel* _kernel)
{
    BodyMover::putInKernel(_kernel);

    if (FFlag::PGSUsesConstraintBasedBodyMovers && getKernel()->getUsingPGSSolver())
    {
        Body* b0 = getPrimitive(0)->getBody();
        Body* b1 = getPrimitive(1)->getBody();
        if( angularVelocityConstraint != NULL && ( angularVelocityConstraint->getBodyA()->getUID() != b0->getUID() || angularVelocityConstraint->getBodyB()->getUID() != b1->getUID() ) )
        {
            delete angularVelocityConstraint;
            angularVelocityConstraint = NULL;
        }
        update();
        _kernel->pgsSolver.addConstraint( angularVelocityConstraint );
    }
}

void BodyAngularVelocity::removeFromKernel()
{
    if (FFlag::PGSUsesConstraintBasedBodyMovers && getKernel()->getUsingPGSSolver())
    {
        if (angularVelocityConstraint)
        {
            getKernel()->pgsSolver.removeConstraint( angularVelocityConstraint );
        }
    }

    BodyMover::removeFromKernel();
}

void BodyAngularVelocity::update()
{
    if (FFlag::PGSUsesConstraintBasedBodyMovers && angularVelocityConstraint == NULL)
    {
        Body* b0 = getPrimitive(0)->getBody();
        Body* b1 = getPrimitive(1)->getBody();
        angularVelocityConstraint = new ConstraintLegacyAngularVelocity( b0, b1 );
    }
}

void BodyAngularVelocity::computeForceImpl(
											bool throttling, 
											Body* body, 
											Body* root, 
											Vector3& force, 
											Vector3& torque)
{
	RBXASSERT(world);

    float newKP = kP;
    if( world && world->getUsingPGSSolver() )
    {
        if( FFlag::PGSUsesConstraintBasedBodyMovers )
        {
            // Clamp newKP to a maximum that's reasonable (90 degrees / step)
            // newKP = std::min( newKP, 0.5f / Constants::worldDt() );
            angularVelocityConstraint->setTarget( angularvelocity );
            angularVelocityConstraint->setMaxTorque(maxTorque);
            angularVelocityConstraint->setMinTorque(-maxTorque);
            angularVelocityConstraint->setUseIntegratedVelocities(true);
            return;
        }
        else
        {
            newKP = kP * PGSSolverSpringConstantScale;
        }
    }

	// P control system
	const Vector3 pAccel = newKP * (angularvelocity - body->getVelocity().rotational);

	lastTorque = body->getRoot()->getBranchMass() * pAccel;
	lastTorque = lastTorque.clamp(-maxTorque, maxTorque);
	
	torque = lastTorque;
}

void BodyAngularVelocity::setMaxTorque(Vector3 value)
{
	if (value != maxTorque)
	{
		maxTorque = value;
		raisePropertyChanged(BodyAngularVelocity::prop_maxTorque);

		if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
			world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
	}
}

void BodyAngularVelocity::setAngularVelocity(Vector3 value)
{
	if (value != angularvelocity)
	{
		angularvelocity = value;
		raisePropertyChanged(BodyAngularVelocity::prop_angularvelocity);

		if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
			world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
	}
}

void BodyAngularVelocity::onPChanged(const Reflection::PropertyDescriptor&)
{
	if (DFFlag::PGSWakePrimitivesWithBodyMoverPropertyChanges && world && world->getUsingPGSSolver())
		world->ticklePrimitive(part.lock()->getPartPrimitive(), true);
}

Reflection::PropDescriptor<BodyForce, Vector3> BodyForce::prop_Force("Force", "Goals", &BodyForce::getBodyForce, &BodyForce::setBodyForce);
Reflection::PropDescriptor<BodyForce, Vector3> BodyForce::prop_ForceDeprecated("force", "Goals", &BodyForce::getBodyForce, &BodyForce::setBodyForce, Reflection::PropertyDescriptor::Attributes::deprecated(BodyForce::prop_Force));

BodyForce::BodyForce(void)
	:DescribedCreatable<BodyForce, BodyMover, sBodyForce>(sBodyForce)
	,bodyForceValue(Vector3::unitY())
{
}


void BodyForce::computeForceImpl(
								bool throttling, 
								Body* body, 
								Body* root, 
								Vector3& force, 
								Vector3& torque)
{
	force = bodyForceValue;
}

void BodyForce::setBodyForce(Vector3 value)
{
	if (value != bodyForceValue)
	{
		bodyForceValue = value;
		raisePropertyChanged(BodyForce::prop_Force);
	}
}

Reflection::PropDescriptor<BodyThrust, Vector3> BodyThrust::prop_force("Force", "Goals", &BodyThrust::getForce, &BodyThrust::setForce);
Reflection::PropDescriptor<BodyThrust, Vector3> BodyThrust::prop_forceDeprecated("force", "Goals", &BodyThrust::getForce, &BodyThrust::setForce, Reflection::PropertyDescriptor::Attributes::deprecated(BodyThrust::prop_force));
Reflection::PropDescriptor<BodyThrust, G3D::Vector3> BodyThrust::prop_location("Location", "Goals", &BodyThrust::getLocation, &BodyThrust::setLocation);
Reflection::PropDescriptor<BodyThrust, G3D::Vector3> BodyThrust::prop_locationDeprecated("location", "Goals", &BodyThrust::getLocation, &BodyThrust::setLocation, Reflection::PropertyDescriptor::Attributes::deprecated(BodyThrust::prop_location));

BodyThrust::BodyThrust(void)
	:DescribedCreatable<BodyThrust, BodyMover, sBodyThrust>(sBodyThrust)
	,bodyThrustValue(Vector3::unitY())
	,location(Vector3::zero())
{
}


void BodyThrust::computeForceImpl(
								bool throttling, 
								Body* body, 
								Body* root, 
								Vector3& force, 
								Vector3& torque)
{
	force = body->getCoordinateFrame().vectorToWorldSpace(bodyThrustValue);
	torque = SimBody::computeTorqueFromOffsetForce(	force, 
													root->getBranchCofmPos(),									// computing torque at this location
													body->getCoordinateFrame().pointToWorldSpace(location)	);	// world position of force application

}

void BodyThrust::setForce(Vector3 value)
{
	if (value != bodyThrustValue)
	{
		bodyThrustValue = value;
		raisePropertyChanged(BodyThrust::prop_force);
	}
}

void BodyThrust::setLocation(Vector3 value)
{
	if (value != location)
	{
		location = value;
		raisePropertyChanged(BodyThrust::prop_location);
	}
}

