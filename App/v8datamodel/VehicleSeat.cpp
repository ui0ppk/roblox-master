/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/VehicleSeat.h"

#include "V8World/KernelJoint.h"
#include "GfxBase/IAdornable.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/CollectionService.h"
#include "V8DataModel/UserController.h"
#include "V8DataModel/Camera.h"
#include "Humanoid/Humanoid.h"
#include "V8World/World.h"
#include "V8World/Mechanism.h"
#include "V8World/RotateJoint.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"
#include "V8Kernel/Body.h"
#include "Util/Rect.h"

DYNAMIC_FASTFLAGVARIABLE(SmootherVehicleSeatControlSystem, false)
FASTFLAG(UseInGameTopBar)

namespace RBX {

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<VehicleSeat, bool> propDisabled("Disabled", "Control", &VehicleSeat::getDisabled, &VehicleSeat::setDisabled);
static const Reflection::RefPropDescriptor<VehicleSeat, Humanoid> propOccupant("Occupant", "Control", &VehicleSeat::getOccupant, NULL, Reflection::PropertyDescriptor::SCRIPTING);
static const Reflection::PropDescriptor<VehicleSeat, int> propThrottle("Throttle", "Control", &VehicleSeat::getThrottle, &VehicleSeat::setThrottle);
static const Reflection::PropDescriptor<VehicleSeat, int> propSteer("Steer", "Control", &VehicleSeat::getSteer, &VehicleSeat::setSteer);
static const Reflection::PropDescriptor<VehicleSeat, float> propMaxSpeed("MaxSpeed", "Control", &VehicleSeat::getMaxSpeed, &VehicleSeat::setMaxSpeed);
static const Reflection::PropDescriptor<VehicleSeat, float> propTurnSpeed("TurnSpeed", "Control", &VehicleSeat::getTurnSpeed, &VehicleSeat::setTurnSpeed);
static const Reflection::PropDescriptor<VehicleSeat, float> propTorque("Torque", "Control", &VehicleSeat::getTorque, &VehicleSeat::setTorque);
static const Reflection::PropDescriptor<VehicleSeat, bool> propEnableHud("HeadsUpDisplay", "Control", &VehicleSeat::getEnableHud, &VehicleSeat::setEnableHud);
static const Reflection::PropDescriptor<VehicleSeat, int> propNumHinges("AreHingesDetected", "Control", &VehicleSeat::getNumHinges, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::RemoteEventDesc<VehicleSeat, void(shared_ptr<Instance>)> event_createSeatWeld(&VehicleSeat::createSeatWeldSignal, "RemoteCreateSeatWeld", "humanoid", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY,	Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<VehicleSeat, void()> event_destroySeatWeld(&VehicleSeat::destroySeatWeldSignal, "RemoteDestroySeatWeld", Security::None, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::CLIENT_SERVER);
REFLECTION_END();

const char* const sVehicleSeat = "VehicleSeat";

VehicleSeat::VehicleSeat() 
: world(NULL)
, throttle(0)
, steer(0)
, maxSpeed(25.0)
, turnSpeed(1.0)
, torque(10)
, enableHud(true)
{
	setName(sVehicleSeat);
	setDisabled(false);
	RBXASSERT(Edge::getPrimitive(0) == NULL);
	RBXASSERT(Edge::getPrimitive(1) == NULL);

	createSeatWeldSignal.connect(boost::bind(static_cast<void (VehicleSeat::*)(shared_ptr<Instance>)>(&VehicleSeat::createSeatWeldInternal), this, _1));
	destroySeatWeldSignal.connect(boost::bind(&VehicleSeat::findAndDestroySeatWeldInternal, this));
}


VehicleSeat::~VehicleSeat() 
{
	RBXASSERT(world == NULL);
	RBXASSERT(Edge::getPrimitive(0) == NULL);
	RBXASSERT(Edge::getPrimitive(1) == NULL);
}

bool VehicleSeat::shouldRender2d() const
{
	return false;
}

void VehicleSeat::render2d(Adorn* adorn)
{
	if(!enableHud) return;

	if (FFlag::UseInGameTopBar)
	{
		return;
	}

	Rect2D viewPort = adorn->getViewport();
	Vector2 center = viewPort.center();
	center.y += 60.0;

	float speed = getPartPrimitive()->getPV().velocity.linear.magnitude();
	float speedDraw = std::min(speed, 100.0f) * 10;
	Rect speedo = Rect::fromCenterSize(center, Vector2(speedDraw, 20));
	adorn->rect2d(speedo.toRect2D(), Color3::blue());	// note RBX color

	std::string speedText = "Speed: " + StringConverter<int>::convertToString(static_cast<int>(speed));
	adorn->drawFont2D(
					speedText,
					speedo.positionPoint(Rect::CENTER, Rect::BOTTOM),
					12,
                    false,
					Color3::blue(),
					Color4::clear(),
					Text::FONT_LEGACY,
					Text::XALIGN_CENTER,
					Text::YALIGN_TOP);
}

// Creates the joint - related to whoever is simulating the seat
// Setting the primitive of the 
void VehicleSeat::onSeatedChanged(bool seated, Humanoid* humanoid)
{
	// If local, handle camera and controller
	if (humanoid && (humanoid == Humanoid::getLocalHumanoidFromContext(this))) {
		if (seated) {
			onLocalSeated(humanoid);
		}
		else {
			onLocalUnseated(humanoid);
		}
		setThrottle(0);
		setSteer(0);
	}

	// Flag that we might need to recompute whether canRender2d
	shouldRenderSetDirty();
}


Humanoid* VehicleSeat::getLocalHumanoid()
{
	return Humanoid::getLocalHumanoidFromContext(this);
}

void VehicleSeat::onLocalSeated(Humanoid* humanoid)
{
	Workspace* workspace = ServiceProvider::find<Workspace>(this);

	workspace->getCamera()->setCameraType(Camera::CUSTOM_CAMERA);
	workspace->getCamera()->setCameraSubject(this);

	// Position camera so it is looking the same direction as the humanoid's torso, but about 10 studs above and 15 studs behind
	if(humanoid)
		if(PartInstance* torso = humanoid->getTorsoFast())
		{
			CoordinateFrame torsoCframe = torso->getCoordinateFrame();
			workspace->getCamera()->setCameraCoordinateFrame(CoordinateFrame((-torsoCframe.lookVector() * 15) 
				+ torsoCframe.translation
				+ Vector3(0,10,0)));
		}
}


void VehicleSeat::onLocalUnseated(Humanoid* humanoid)
{
	Workspace* workspace = ServiceProvider::find<Workspace>(this);
	workspace->getCamera()->setCameraSubject(humanoid);
	workspace->getCamera()->setCameraType(Camera::CUSTOM_CAMERA);

	// make sure humanoid is not steated
	humanoid->setSit(false);
}


void VehicleSeat::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if(oldProvider){
		oldProvider->create<CollectionService>()->removeInstance(shared_from(this));
	}

	Super::onServiceProvider(oldProvider, newProvider);

	if(newProvider){
		newProvider->create<CollectionService>()->addInstance(shared_from(this));
	}
}
// Into / out of the world
void VehicleSeat::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);		// Note - moved this to first

	World* newWorld = Workspace::getWorldIfInWorkspace(this);

	if (newWorld != world) 
	{
		if (world) {
			RBXASSERT(newWorld == NULL);
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

	raisePropertyChanged(propNumHinges);		// not perfect, but a start - really need to flag any change of assembly/dissassembly
}


Body* VehicleSeat::getEngineBody()
{
	Primitive* myPrim = getPartPrimitive();
	return myPrim->getBody()->getRoot();
}


void VehicleSeat::computeForce(bool throttling)
{
	stepHinges();
}

bool VehicleSeat::stepUi(double distributedGameTime)
{
	loadMotorsAndHinges();
	if ((throttle != 0) || (steer != 0)) {
		if (world) {
			for (int i = 0; i < hinges.size(); ++i) {
				RotateJoint* hinge = hinges[i];
				world->ticklePrimitive(hinge->getAxlePrim(), true);
				world->ticklePrimitive(hinge->getHolePrim(), true);
			}
		}
	}

//	stepVelocityMotors();
	return false;
}


int VehicleSeat::computeNumHinges()
{
	loadMotorsAndHinges();
	return hinges.size();
}


int VehicleSeat::getNumHinges() const
{
	VehicleSeat* meNotConst = const_cast<VehicleSeat*>(this);		// hack hack hack
	return meNotConst->computeNumHinges();
}


void VehicleSeat::loadMotorsAndHinges() 
{
	hinges.fastClear();
	onRights.fastClear();
	axlePointingIns.fastClear();
	axleVelocities.fastClear();


	Primitive* primitive = getPartPrimitive();
	if (Assembly* assembly = primitive->getAssembly()) {
		if (!assembly->computeIsGrounded()) {
			assembly->visitPrimitives(boost::bind(&VehicleSeat::doLoadHinges, this, _1));
		}
	}
}

// only test all primitives and their child joints in the assembly
void VehicleSeat::doLoadHinges(Primitive* primitive)
{
	for (int i = 0; i < primitive->numChildren(); ++i) {
		Primitive* child = primitive->getTypedChild<Primitive>(i);
		SpanningEdge* edge = child->getEdgeToParent();
		Joint* joint = rbx_static_cast<Joint*>(edge);
		if (Joint::getJointType(joint) == Joint::ROTATE_JOINT) {
			RotateJoint* rotateJoint = rbx_static_cast<RotateJoint*>(joint);

			bool aligned, onRight, axlePointingIn;
			getJointInfo(rotateJoint, aligned, onRight, axlePointingIn);
			if (aligned) {
				hinges.append(rotateJoint);
				onRights.append(onRight);
				axleVelocities.append(0.0f);
				axlePointingIns.append(axlePointingIn);
			}
		}
	}
}

void VehicleSeat::getJointInfo(RotateJoint* rotateJoint, bool& aligned, bool& onRight, bool& axlePointingIn)
{
	Primitive* myPrim = getPartPrimitive();
	Vector3 seatRight = myPrim->getCoordinateFrame().rightVector();
	Vector3 seatToJoint = rotateJoint->getJointWorldCoord(0).translation - myPrim->getCoordinateFrame().translation;
	onRight = (seatRight.dot(seatToJoint) > 0.0);

	Vector3 axleLine = rotateJoint->getAxleWorldDirection();
	float rightDotAxle = seatRight.dot(axleLine);
	aligned = (fabs(rightDotAxle) > 0.8f);
	bool negativeDot = (rightDotAxle < 0.0f);
	axlePointingIn = (onRight == negativeDot);
}

/*
void VehicleSeat::stepVelocityMotors()
{
	Primitive* primitive = PartInstance::getPrimitive();
	Vector3 seatRight = primitive->getCoordinateFrame().rightVector();
	
	int left = vehicleSeatLeftPower[throttle+1][steer+1];
	int right = vehicleSeatLeftPower[throttle+1][1-steer];

	for (int i = 0; i < velocityMotors.size(); ++i) {
		RotateVJoint* rotateVJoint = velocityMotors[i];
		Primitive* axlePrim = rotateVJoint->getAxlePrim();
		NormalId normalId = rotateVJoint->getNormalId(0);		// surface of the axle Primitive
		int id2 = (normalId + 1) % 3;
		int id3 = (normalId + 2) % 3;
		Vector3 size = axlePrim->getSize();
		float diameter = std::max(size[id2], size[id3]);
		SurfaceData surfaceData = axlePrim->getSurfaceData(normalId);
		surfaceData.inputType = LegacyController::CONSTANT_INPUT;

		float pointingRight = Math::polarity(seatRight.dot(rotateVJoint->getAxleWorldDirection()));	
		float onRight = Math::polarity(seatRight.dot(	rotateVJoint->getJointWorldCoord(0).translation - primitive->getCoordinateFrame().translation )  );

		float direction = pointingRight * (onRight > 0.0 ? right : left);

		surfaceData.paramB = -3.0f * direction / diameter;
		axlePrim->setSurfaceData(normalId, surfaceData);
	}
}
*/

void VehicleSeat::stepHinges()
{
	Primitive* primitive = getPartPrimitive();
	Velocity velocity = primitive->getPV().velocity;

	bool overMaxSpeed = (velocity.linear.magnitude() > maxSpeed);
	bool overTurnSpeed = (fabs(velocity.rotational.y) > turnSpeed);

	for (int i = 0; i < hinges.size(); ++i) 
	{
		RotateJoint* hinge = hinges[i];
		float jointVelocity = hinge->getAxleVelocity();

		if (!axlePointingIns[i]) 
		{
			jointVelocity = -jointVelocity;
		}
		int polarity = lookupFunction(throttle, steer, onRights[i], overMaxSpeed, overTurnSpeed, jointVelocity);
		if (!axlePointingIns[i]) 
		{
			polarity = -polarity;
		}

		float gain = 1.0f;
		if (DFFlag::SmootherVehicleSeatControlSystem && world && world->getUsingPGSSolver())
		{
			if (steer == 0) 
			{ 
				//Get the velocity that the hinge had previously
				float previousVelocity = axleVelocities[i];
				axleVelocities[i] = jointVelocity;

				//TurnCorrectionFactor is used to compensate for any unwanted turning. This should only be required
				//with vehicles that are "poorly" built (i.e. friction in places that shouldn't have any). 
				float turnCorrectionFactor = velocity.rotational.y * throttle * 5.0f;
				if (turnCorrectionFactor >  1.0f)
					turnCorrectionFactor =  1.0f;
				if (turnCorrectionFactor < -1.0f) 
					turnCorrectionFactor = -1.0f;

				if (onRights[i]) 
					turnCorrectionFactor *= -1; 
				if (turnCorrectionFactor < 0.0f) 
					turnCorrectionFactor = 0.0f;

				if (throttle == 0) 
				{ // with no throttle, do *not* simply set gain to 0.  Slow down more smoothly.
					gain = fabs(jointVelocity)/10.0f;
					if (gain < 0.0f) 
						gain = 0.0f;
					if (gain > 1.0f) 
						gain = 1.0f;

					if (jointVelocity * previousVelocity < 0) 
					{
						float delta = fabs(previousVelocity-jointVelocity);
						gain *= fabs(jointVelocity)/delta;
					}
				} 
				else if (maxSpeed > 0.0f) 
				{ // make it smooth once you approach max speed.
					gain = fabs(velocity.linear.magnitude() - maxSpeed)-0.5f;
					if (gain < 0.0f) gain = 0.0f;
					if (gain > 1.0f) gain = 1.0f;
				}
				gain += turnCorrectionFactor; //Full stop, 2x speed at 0.1 1/s. Reverse, 3x speed at 0.2 rad/s
			} 
			else if (turnSpeed != 0) 
			{ 
				gain = 2*(turnSpeed - fabs(velocity.rotational.y))/turnSpeed + 1;
			}
		}

		float appliedTorque = gain * torque * 1000;		// make properties nice

		Vector3 axleDirection = hinge->getAxleWorldDirection();	
		hinge->getAxlePrim()->getBody()->accumulateTorque(polarity * axleDirection * appliedTorque);
		hinge->getHolePrim()->getBody()->accumulateTorque(-polarity * axleDirection * appliedTorque);
	}
}


/*
	+/- 1: Wheel pushes forward/backward
	0:     Wheel locks
*/

const int throttleSteerRightSpeedTurn[3][3][2][2][2] = 
{
	//	Side:	Left			Right
	//	Speed:	OK		Hi   	OK		Hi
	//	Turn:	Lo  Hi  Lo  Hi  Lo  Hi  Lo  Hi
			    0, -1,  0,  0, -1, -1,  0,  0,     // Backward Left
			   -1, -1,  0,  0, -1, -1,  0,  0,     // Backward Center
			   -1, -1,  0,  0,  0,  1,  0,  0,     // Backward Right
			   -1,  0,  0,  0, 	1,  0,  0,  0,     // ZERO Left
				0,  0,  0,  0, 	0,  0,  0,  0,     // ZERO Center
			    1,  0,  0,  0, -1,  0,  0,  0,     // ZERO Right
				0,  1,  0,  0, 	1,  1,  0,  0,     // Forward Left
				1,  1,  0,  0, 	1,  1,  0,  0,     // Forward Center
				1,  1,  0,  0, 	0,  1,  0,  0      // Forward Right
};

int VehicleSeat::lookupFunction(int throttle, int steer, bool onRight, bool overMaxSpeed, bool overTurnSpeed, float jointVelocity)
{
	int th = throttle+1;
	int st = steer+1;
	int ri = onRight ? 1 : 0;
	int sp = overMaxSpeed ? 1 : 0;
	int tu = overTurnSpeed ? 1 : 0;
	int forwardBackward = throttleSteerRightSpeedTurn[th][st][ri][sp][tu];

	if (forwardBackward != 0) {
		int positiveTorque = forwardBackward;
		if (!onRight) {
			positiveTorque = -positiveTorque;
		}
		return positiveTorque;
	}
	else {
		if (jointVelocity > 0) {
			return -1;
		}
		else {
			return 1;
		}
	}
}

// Set primitive - already in the engine, this will turn it on/off for simulation
// Basically pulls out of the engine if no throttle
void VehicleSeat::setThrottle(int value)
{
	if (throttle != value) {
		throttle = G3D::iClamp(value, -1, 1);
		raisePropertyChanged(propThrottle);
	}
}

void VehicleSeat::setSteer(int value)
{
	if (steer != value) {
		steer = G3D::iClamp(value, -1, 1);
		raisePropertyChanged(propSteer);
	}
}

void VehicleSeat::setEnableHud(bool value)
{
	if(enableHud != value){
		enableHud = value;
		raisePropertyChanged(propEnableHud);
	}
}

void VehicleSeat::setMaxSpeed(float value)
{
	if (maxSpeed != value) {
		maxSpeed = value;
		raisePropertyChanged(propMaxSpeed);
	}
}

void VehicleSeat::setTurnSpeed(float value)
{
	if (turnSpeed != value) {
		turnSpeed = value;
		raisePropertyChanged(propTurnSpeed);
	}
}

void VehicleSeat::setTorque(float value)
{
	if (torque != value) {
		torque = value;
		raisePropertyChanged(propTorque);
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

void VehicleSeat::getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives)
{
	Primitive* p = getPartPrimitive();
	if (Assembly* a = p->getAssembly()) {
		if (!a->computeIsGrounded()) {
			a->visitPrimitives(boost::bind(&gatherPrimitivesInSeatAssembly, _1, boost::ref(primitives)));
		}
	}
}

void VehicleSeat::createSeatWeld(Humanoid *h)
{
	Network::GameMode gameMode = Network::Players::getGameMode(h);
	if (gameMode == Network::DPHYS_CLIENT || gameMode == Network::CLIENT)
	{
		if (this->debounceTimeUp())
		{
			event_createSeatWeld.replicateEvent(this, shared_from(h));
			this->debounceTime = Time::now<Time::Fast>();
		}
	}
	else
		Super::createSeatWeld(h);
}

void VehicleSeat::findAndDestroySeatWeld()
{
	event_destroySeatWeld.fireAndReplicateEvent(this);
}

void VehicleSeat::setOccupant(Humanoid* value)
{
	if (occupant.get() != value)
	{
		occupant = shared_from(value);
		raisePropertyChanged(propOccupant);
	}
}

} // namespace
