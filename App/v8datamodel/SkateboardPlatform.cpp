/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/SkateboardPlatform.h"

#include "V8World/KernelJoint.h"
#include "GfxBase/IAdornable.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/CollectionService.h"
#include "V8DataModel/UserController.h"
#include "V8DataModel/SkateboardController.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/GeometryService.h"
#include "V8Datamodel/ModelInstance.h"
#include "Humanoid/Humanoid.h"
#include "V8World/World.h"
#include "V8World/Mechanism.h"
#include "V8World/RotateJoint.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"
#include "V8Kernel/Body.h"
#include "Util/Rect.h"
#include "util/standardout.h"

namespace RBX {

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<SkateboardPlatform, int> propThrottle("Throttle", "Control", &SkateboardPlatform::getThrottle, &SkateboardPlatform::setThrottle);
static const Reflection::PropDescriptor<SkateboardPlatform, int> propSteer("Steer", "Control", &SkateboardPlatform::getSteer, &SkateboardPlatform::setSteer);
static const Reflection::PropDescriptor<SkateboardPlatform, bool> propStickyWheels("StickyWheels", "Control", &SkateboardPlatform::getStickyWheels, &SkateboardPlatform::setStickyWheels);

static Reflection::EnumPropDescriptor<SkateboardPlatform, SkateboardPlatform::MoveState> propMoveState("MoveState", "Control", &SkateboardPlatform::getMoveState, &SkateboardPlatform::setMoveState, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static Reflection::EventDesc<SkateboardPlatform, void(SkateboardPlatform::MoveState, SkateboardPlatform::MoveState)>	event_MoveStateChanged(&SkateboardPlatform::moveStateChangedSignal, "MoveStateChanged", "newState", "oldState");

static Reflection::EventDesc<SkateboardPlatform, void(shared_ptr<Instance>, shared_ptr<Instance>)> event_Equipped(&SkateboardPlatform::equippedSignal, "Equipped", "humanoid", "skateboardController");
static Reflection::EventDesc<SkateboardPlatform, void(shared_ptr<Instance>, shared_ptr<Instance>)> dep_Equipped(&SkateboardPlatform::equippedSignal, "equipped", "humanoid", "skateboardController", Security::None, Reflection::Descriptor::Attributes::deprecated(event_Equipped));
static Reflection::EventDesc<SkateboardPlatform, void(shared_ptr<Instance>)> event_Unequipped(&SkateboardPlatform::unequippedSignal, "Unequipped", "humanoid");
static Reflection::EventDesc<SkateboardPlatform, void(shared_ptr<Instance>)> dep_Unequipped(&SkateboardPlatform::unequippedSignal, "unequipped", "humanoid", Security::None, Reflection::Descriptor::Attributes::deprecated(event_Unequipped));

static Reflection::RefPropDescriptor<SkateboardPlatform, SkateboardController> prop_Controller("Controller", "Control", &SkateboardPlatform::getController, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::RefPropDescriptor<SkateboardPlatform, Humanoid> prop_ControllingHumanoid("ControllingHumanoid", "Control", &SkateboardPlatform::getControllingHumanoid, NULL, Reflection::PropertyDescriptor::UI);

static Reflection::BoundFuncDesc<SkateboardPlatform, void(Vector3)> desc_ApplySpecificImpulse(&SkateboardPlatform::applySpecificImpulse, "ApplySpecificImpulse", "impulseWorld", Security::None);

static Reflection::RemoteEventDesc<SkateboardPlatform, void(shared_ptr<Instance>)> event_createPlatformMotor6D(&SkateboardPlatform::createPlatformMotor6DSignal, "RemoteCreateMotor6D", "humanoid", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY,	Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<SkateboardPlatform, void()> event_destroyPlatformMotor6D(&SkateboardPlatform::destroyPlatformMotor6DSignal, "RemoteDestroyMotor6D", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<SkateboardPlatform::MoveState>::EnumDesc()
	:EnumDescriptor("MoveState")
{
	addPair(SkateboardPlatform::STOPPED, "Stopped");
	addPair(SkateboardPlatform::COASTING, "Coasting");
	addPair(SkateboardPlatform::PUSHING, "Pushing");
	addPair(SkateboardPlatform::STOPPING, "Stopping");
	addPair(SkateboardPlatform::AIR_FREE, "AirFree");
}
}//namespace Reflection

const char* const sSkateboardPlatform = "SkateboardPlatform";

SkateboardPlatform::SkateboardPlatform() 
: world(NULL)
, moveState(STOPPED)
, motor6D(NULL)
, humanoid(NULL)
, throttle(0)
, steer(0)
, turnRate(0)
, gyroConvergeSpeed(0)
, forwardVelocity(0)
, numWheelsGrounded(0)
, stickyWheels(true)

, maxPushVelocity(40.0f)
, maxPushForce(2000.0f)
, stopForceMultiplier(1.5f)
, maxTurnVelocity(2.5f)
, maxTurnTorque(25000.0f)
, turnTorqueGain(25000.0f)		// if deltaV == 1, apply maxTorque essentially

{
	setName(sSkateboardPlatform);
	RBXASSERT(Edge::getPrimitive(0) == NULL);
	RBXASSERT(Edge::getPrimitive(1) == NULL);

	createPlatformMotor6DSignal.connect(boost::bind(static_cast<void (SkateboardPlatform::*)(shared_ptr<Instance>)>(&SkateboardPlatform::createPlatformMotor6DInternal), this, _1));
	destroyPlatformMotor6DSignal.connect(boost::bind(&SkateboardPlatform::findAndDestroyPlatformMotor6DInternal, this));
}


SkateboardPlatform::~SkateboardPlatform() 
{
	RBXASSERT(world == NULL);
	RBXASSERT(Edge::getPrimitive(0) == NULL);
	RBXASSERT(Edge::getPrimitive(1) == NULL);
}



// Creates the joint - related to whoever is simulating the seat
// Setting the primitive of the 
void SkateboardPlatform::onPlatformStandingChanged(bool platformed, Humanoid* humanoid)
{
	// 1. Clear out the controller
	if(myController)
	{
		myController->setParent(NULL);
		myController.reset();
	}

	// 2.  If local, handle camera and controller
	if (humanoid && (humanoid == Humanoid::getLocalHumanoidFromContext(this))) {
		if (platformed) {
			onLocalPlatformStanding(humanoid);
		}
		else {
			onLocalNotPlatformStanding(humanoid);
		}
		setThrottle(0);
		setSteer(0);
	}
}

void SkateboardPlatform::onLocalPlatformStanding(Humanoid* humanoid)
{
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	ControllerService* service = ServiceProvider::create<ControllerService>(this);
	myController = Creatable<Instance>::create<SkateboardController>();
	myController->setSkateboardPlatform(this);
	myController->setParent(service);


	workspace->getCamera()->setCameraSubject(this);
	workspace->getCamera()->setCameraType(Camera::CUSTOM_CAMERA);

	this->humanoid = humanoid;

	// put the board in the humanoid.
	ModelInstance* figure = Instance::fastDynamicCast<ModelInstance>(humanoid->getParent());
	ModelInstance* board = Instance::fastDynamicCast<ModelInstance>(this->getParent());
	if(board && !board->getIsParentLocked() && figure)
	{
		board->setParent(figure);

		equippedSignal(shared_from(humanoid), myController);
	}
	else
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error mounting skateboard platform. Both Humanoid and SkateboardPlatform must be direct children of a Model");
	}
}

void SkateboardPlatform::delayedReparentToWorkspace(weak_ptr<ModelInstance> weakBoard, weak_ptr<ModelInstance> weakFigure)
{
	shared_ptr<ModelInstance> board = weakBoard.lock();
	shared_ptr<ModelInstance> figure = weakFigure.lock();
	if(board && figure && board->getParent() == figure.get()) // make sure we are still parented like we though. if not, take no action.
	{
		Workspace* workspace = ServiceProvider::find<Workspace>(figure.get());

		// if not in workspace, whole tree probably deleted.
		if(workspace)
		{
			board->setParent(workspace);
		}
	}

}


void SkateboardPlatform::onLocalNotPlatformStanding(Humanoid* humanoid)
{
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	workspace->getCamera()->setCameraSubject(humanoid);
	workspace->getCamera()->setCameraType(Camera::CUSTOM_CAMERA);

	unequippedSignal(shared_from(humanoid));
	
	// make sure humanoid is not standing. this is important if the joint gets destroyed unknown to the humanoid.
	humanoid->setPlatformStanding(false);

	// remove board from figure.
	ModelInstance* figure = Instance::fastDynamicCast<ModelInstance>(humanoid->getParent());
	ModelInstance* board = Instance::fastDynamicCast<ModelInstance>(this->getParent());
	DataModel* dataModel = DataModel::get(workspace);
	if(dataModel && board && figure && board->getParent() == figure)
	{
		dataModel->submitTask(boost::bind(&delayedReparentToWorkspace, weak_from(board), weak_from(figure)), DataModelJob::Write);
	}

	this->humanoid = NULL;
}


void SkateboardPlatform::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider, newProvider);
}
// Into / out of the world
void SkateboardPlatform::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);		// Note - moved this to first

	World* newWorld = Workspace::getWorldIfInWorkspace(this);

	if (newWorld != world) 
	{
		if(myController)
		{
			myController->setParent(NULL); // should we re-link? 
			myController.reset();
			humanoid = NULL;
		}

		if (world) {
			world->removeJoint(this);
			setPrimitive(0, NULL);
			setPrimitive(1, NULL);
		}

		world = newWorld;

		if (world) {
			setPrimitive(0, getPartPrimitive());
			setPrimitive(1, world->getGroundPrimitive());
			world->insertJoint(this);
		}
	}
}


Body* SkateboardPlatform::getEngineBody()
{
	Primitive* myPrim = getPartPrimitive();
	return myPrim->getBody()->getRoot();
}


static const float minStopVelocity = 0.1f;
static const float deltaTurnRate = 0.002f;
static const float minTurnRate = 0.005f;	// 
static const float maxTurnRate = 1.0f;


bool SkateboardPlatform::isFullyGrounded()
{
	return numWheelsGrounded == wheels.size();
}


void SkateboardPlatform::countGroundedWheels()
{
	const CoordinateFrame& boardCFrame = getCoordinateFrame();

	// check if grounded
	Vector3 downforceDir = boardCFrame.normalToWorldSpace(Vector3::unitY() * -1.0f);
	GeometryService *geom = ServiceProvider::create<GeometryService>(humanoid);

	numWheelsGrounded = 0;
	if(humanoid && geom)
	{
		for(int i = 0; i < wheels.size(); ++i)
		{
			RBX::RbxRay caster( wheels[i].joint->getJointWorldCoord(1).translation, downforceDir* 2.0f); // search 2 stud
			Primitive* hitPrim = NULL;
			geom->getHitLocationFilterStairs(humanoid->getParent(), caster, &hitPrim);
			if(hitPrim)
			{
				wheels[i].grounded = true;
				numWheelsGrounded++;
			}
			else
			{
				wheels[i].grounded = false;
			}
		}
	}
}

bool SkateboardPlatform::stepUi(double distributedGameTime)
{
	if ((throttle != 0) || (steer != 0)) {	
		world->ticklePrimitive(getPartPrimitive(), true);
	}

	loadWheels();
	countGroundedWheels();

	///////////////// simple set state here to get started
	if (getControllingHumanoid())
	{
		if(isFullyGrounded())
		{
			if (throttle > 0) {
				setMoveState(PUSHING);
			}
			else if (throttle < 0) {
				setMoveState(STOPPING);
			}
			else if (fabs(forwardVelocity) > 1.0f) {
				setMoveState(COASTING);
			}
			else {
				setMoveState(STOPPED);
			}
		}
		else
		{
			setMoveState(AIR_FREE);
		}
	}

	motor6D = findPlatformMotor6D();
	if (motor6D) {
		RBXASSERT(humanoidFromMotor6D(motor6D) == this->humanoid);
		RBXASSERT(humanoidFromMotor6D(motor6D) == this->humanoid);
	}

/*	if(false)
	{
		Body* root = getEngineBody();
		if (root->getCoordinateFrame().rotation.getColumn(1).y < 0.2) {
			if (Motor6D* motor6D = findPlatformMotor6D()) {
				if (Humanoid* humanoid = humanoidFromMotor6D(motor6D)) {
					humanoid->setJump(1);								// jump off
				}
			}
		}
	}*/


	/*Motor6D* motor6D = findPlatformMotor6D();
	if(motor6D)
	{
		Vector3 torqueObj = boardCFrame.normalToObjectSpace(torque);
		Vector3 torqueFace = motor6D->getC0().normalToObjectSpace(torqueObj);

		float angle = asin(torqueFace.unitize() / fabs(gravityForce.y));
		{
			Vector3 axis;
			if(angle > 0.001)
			{
				axis = torqueFace;
				axis.unitize();
			}
			else
			{
				axis = Vector3::UNIT_Z;
				angle = 0;
			}

			angle = std::min(0.5f, angle); // max lean.
			
			CachedPose pose(Vector3::ZERO, axis * angle);
			motor6D->applyPose(pose);
		}
	}*/

	return false;
}


void SkateboardPlatform::loadWheels() 
{
	wheels.fastClear();

	Primitive* primitive = getPartPrimitive();
	if (Assembly* assembly = primitive->getAssembly()) {
		if (!assembly->computeIsGrounded()) {
			assembly->visitPrimitives(boost::bind(&SkateboardPlatform::doLoadWheels, this, _1));
		}
	}
}

// only test all primitives and their child joints in the assembly
void SkateboardPlatform::doLoadWheels(Primitive* primitive)
{
	for (int i = 0; i < primitive->numChildren(); ++i) {
		Primitive* child = primitive->getTypedChild<Primitive>(i);
		SpanningEdge* edge = child->getEdgeToParent();
		Joint* joint = rbx_static_cast<Joint*>(edge);
		if (Joint::getJointType(joint) == Joint::ROTATE_JOINT) {
			Wheel w;
			w.joint = rbx_static_cast<RotateJoint*>(joint);
			wheels.append(w);
		}
	}
}


void SkateboardPlatform::computeForce(bool throttling)
{
	Body* root = getEngineBody();
	const CoordinateFrame& boardCFrame = getCoordinateFrame(); // is it okay to be calling this in computeForce? if we can garantee that the root body is the board, we could use the root body cframe instead.
	const SimBody* rootBody = root->getConstRootSimBody();

	PV pvInWorld = root->getPvUnsafe();
	Velocity velInRoot = Velocity::toObjectSpace(pvInWorld.velocity, pvInWorld.position);
	forwardVelocity = -velInRoot.linear.z;
	float yRotVelocity = velInRoot.rotational.y;

	int steerY = -steer;

	if (throttle > 0) {
		doPush(forwardVelocity);
	}
	else if (throttle < 0) {
		doStop(forwardVelocity);
	}

	// if we flick the steer input - do an immediate half over
	if (steerY > 0) {
		turnRate = G3D::clamp(turnRate + deltaTurnRate, minTurnRate, maxTurnRate);
	}
	else if (steerY < 0) {
		turnRate = G3D::clamp(turnRate - deltaTurnRate, -maxTurnRate, -minTurnRate);
	}
	else if (turnRate > deltaTurnRate) {
		turnRate -= deltaTurnRate;
	}
	else if (turnRate < -deltaTurnRate) {
		turnRate += deltaTurnRate;
	}

	doTurn(yRotVelocity);

	if(!specificImpulseAccumulator.isZero())
	{
		Vector3 impulse = specificImpulseAccumulator * root->getBranchMass();
		root->accumulateImpulseAtBranchCofm(impulse);
		specificImpulseAccumulator = Vector3::zero();
	}

	if(isFullyGrounded() && stickyWheels)
	{
		// cancel torque caused by gravity/surface normal. todo: should we be using ground contact point center instead of board center?
		// advantage of this method: torque is zero automatically if nothing is attached to the board. 
		Vector3 gravityForce = rootBody->getWorldGravityForce();
		Vector3 gravityTorque = SimBody::computeTorqueFromOffsetForce(gravityForce, rootBody->getPV().position.translation, boardCFrame.translation);
		root->accumulateTorque(gravityTorque);

		Vector3 downforceWorld = boardCFrame.normalToWorldSpace(gravityForce);
		root->accumulateForceAtBranchCofm(gravityForce * -.5f); // cancel

		root->accumulateForceAtBranchCofm(downforceWorld); // stick board to ground
	}

}


void SkateboardPlatform::doTurn(float yRotVelocity)
{
	Body* root = getEngineBody();

	if (fabs(turnRate) > minTurnRate) {

		float desiredYRotVelocity = turnRate * maxTurnVelocity;
		float deltaVelocity = desiredYRotVelocity - yRotVelocity;

		float torque = G3D::clamp( deltaVelocity * turnTorqueGain, -maxTurnTorque, maxTurnTorque);

		Vector3 torqueInBody = Vector3(0.0f, torque, 0.0f);
		Vector3 torqueInWorld = root->getCoordinateFrame().vectorToWorldSpace(torqueInBody);

		root->accumulateTorque(torqueInWorld);
	}
}



void SkateboardPlatform::doPush(float forwardVelocity)
{
	if (forwardVelocity < 0.0) {
		applyForwardForce(maxPushForce * stopForceMultiplier);
	}
	else if (forwardVelocity < maxPushVelocity) {
		applyForwardForce(maxPushForce);
	}
}

void SkateboardPlatform::doStop(float forwardVelocity)
{
	if (forwardVelocity > minStopVelocity) {
		applyForwardForce(-maxPushForce * stopForceMultiplier);
	}
}

void SkateboardPlatform::applyForwardForce(float force)
{
	Body* root = getEngineBody();
	Vector3 forceInBody = Vector3(0.0f, 0.0f, -force);
	Vector3 forceInWorld = root->getCoordinateFrame().vectorToWorldSpace(forceInBody);
	root->accumulateForceAtBranchCofm(forceInWorld);
}





bool SkateboardPlatform::shouldRender2d() const
{
	return false;
}

void SkateboardPlatform::render2d(Adorn* adorn)
{
}
/*
	float speed = getPartPrimitive()->getPV().velocity.linear.magnitude();

	renderGauge(adorn, 0, "Speed", speed, -20.0f, 20.0f);
	renderGauge(adorn, 1, "TurnRate", turnRate, -1.0f, 1.0f); 
	renderGauge(adorn, 2, "TurnRate", turnRate, -1.0f, 1.0f); 
}

void SkateboardPlatform::renderGuange(
	Rect2D viewPort = adorn->getViewport();
	Vector2 center = viewPort.center();
	center.y += 60.0;

	


	float speedDraw = std::min(speed, 100.0f) * 10;
	Rect speedo = Rect::fromCenterSize(center, Vector2(speedDraw, 20));
	adorn->rect2d(speedo.toRect2D(), Color3::blue());	// note RBX color

	std::string speedText = "Speed: " + StringConverter<int>::convertToString(static_cast<int>(speed));
	adorn->drawFont2D(
					speedText,
					speedo.positionPoint(Rect::CENTER, Rect::BOTTOM),
					12,
					Color3::blue(),
					Color4::clear(),
					Text::FONT_LEGACY,
					Text::XALIGN_CENTER,
					Text::YALIGN_TOP);
}
*/


// Set primitive - already in the engine, this will turn it on/off for simulation
// Basically pulls out of the engine if no throttle
void SkateboardPlatform::setThrottle(int value)
{
	if (throttle != value) {
		throttle = G3D::iClamp(value, -1, 1);
		raisePropertyChanged(propThrottle);
	}
}

void SkateboardPlatform::setSteer(int value)
{
	if (steer != value) {
		steer = G3D::iClamp(value, -1, 1);
		raisePropertyChanged(propSteer);
	}
}

void SkateboardPlatform::setStickyWheels(bool value)
{
	if (stickyWheels != value) {
		stickyWheels = value;
		raisePropertyChanged(propStickyWheels);
	}
}

void SkateboardPlatform::setMoveState(const MoveState& value)
{
	if (moveState != value) {
		MoveState oldState = moveState;
		moveState = value;
		raisePropertyChanged(propMoveState);
		moveStateChangedSignal(moveState, oldState);
	}
}


///////////////////////////////////////////////////////////////////////////////////////////////
//  CameraSubject stuff


// Ignore everything in the same assembly if moving
//

static void gatherPrimitivesInSeatAssembly(Primitive* p, std::vector<const Primitive*>& primitives)
{
	primitives.push_back(p);
}

void SkateboardPlatform::getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives)
{
	Primitive* p = getPartPrimitive();
	if (Assembly* a = p->getAssembly()) {
		if (!a->computeIsGrounded()) {
			a->visitPrimitives(boost::bind(&gatherPrimitivesInSeatAssembly, _1, boost::ref(primitives)));
		}
	}
}

void SkateboardPlatform::applySpecificImpulse(Vector3 impulseWorld)
{
	specificImpulseAccumulator += impulseWorld;
}

void SkateboardPlatform::applySpecificImpulse(Vector3 impulseWorld, Vector3 worldPos)
{
	// ignore rot impulse for now.
	applySpecificImpulse(impulseWorld);
}


float SkateboardPlatform::getGroundSpeed()
{
	return 1.0f;
}

void SkateboardPlatform::createPlatformMotor6D(Humanoid *h)
{
	Network::GameMode gameMode = Network::Players::getGameMode(h);
	if (gameMode == Network::DPHYS_CLIENT || gameMode == Network::CLIENT)
	{
		if (this->debounceTimeUp())
		{
			event_createPlatformMotor6D.replicateEvent(this, shared_from(h));
			this->debounceTime = Time::now<Time::Fast>();
		}
	}
	else
		Super::createPlatformMotor6D(h);
}

void SkateboardPlatform::findAndDestroyPlatformMotor6D()
{
	event_destroyPlatformMotor6D.fireAndReplicateEvent(this);
}

/*bool SkateboardPlatform::findLip(Adorn* adorn)
{
	const CoordinateFrame& boardCoord = getCoordinateFrame();
	Vector3 rayDir = boardCoord.lookVector();

	if(getGroundSpeed() < 0)
	{
		rayDir = rayDir * -1.0f;
	}

	GeometryService *geom = ServiceProvider::create<GeometryService>(humanoid);
	
	Color3 color = Color3::orange();	// only when rendering

	float samples = {0.05f, 0.1f, 0.5f};

	for (int i = 0; i < ARRAYSIZE(samples); i++) 
	{
		float distanceFromBottom = samples[i];
		Vector3 originOnTorso(0, -lowLadderSearch() + distanceFromBottom, 0);
		Ray caster;
		caster.origin = torsoCoord.translation + originOnTorso;
		caster.direction = torsoLook * ladderSearchDistance();

		Primitive* hitPrim = NULL;
		Vector3 hitLoc = geom->getHitLocationFilterStairs(humanoid->getParent(), caster, &hitPrim);

		// make trusses climbable.
		if (hitPrim) 
		{
			PartInstance *p = PartInstance::fromPrimitive(hitPrim);
			if (p)
			{
				if (p->getPartType() == RBX::TRUSS_PART) return true;
			}
		}

		float mag = (hitLoc - caster.origin).magnitude();

		if (mag < searchDepth()) {
			// this is a step or rung
			if (look_for_space) 
			{
				// we were looking for a space and now we've found the end of it
				first_space = distanceFromBottom;
				look_for_space = false;
				look_for_step = true;
				color = Color3::green();
			}
		} else
		{
			// this is a space
			if (look_for_step)
			{
				// we were looking for a step and now we've found the end of it
				first_step = distanceFromBottom - first_space;
				look_for_step = false;
				color = Color3::red();
			}
		}
		if (adorn) {
			adorn->setObjectToWorldMatrix(torsoCoord.translation);
			Ray rayInPart = Ray::fromOriginAndDirection(	caster.origin - torsoCoord.translation, 
															hitLoc - caster.origin	);
			adorn->ray( rayInPart, color, 1.0 );
		}
	}

	if (first_space < maxClimbDistance() &&
		first_step > 0.0f &&
		first_step < maxClimbDistance()) 
	{
		if (adorn) {
			DrawAdorn::cylinder(adorn, CoordinateFrame(torsoCoord.translation + Vector3(0,5,0)), 1, 0.2, 1.0, Color3::purple());
		}
		return true;
	}

	return false;
}
*/

} // namespace
