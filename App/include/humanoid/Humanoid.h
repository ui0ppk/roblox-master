/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/ICharacterSubject.h"
#include "V8DataModel/IModelModifier.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Tool.h"
#include "V8World/KernelJoint.h"
#include "V8World/Primitive.h"
#include "Util/SteppedInstance.h"
#include "GfxBase/IAdornable.h"
#include "Util/RunStateOwner.h"
#include "Util/ContentFilter.h"
#include "Util/HeapValue.h"
#include "Humanoid/StatusInstance.h"
#include "Humanoid/HumanoidState.h"

namespace RBX {
	class World;
	class RunService;
	class Controller;
	class Primitive;
	class PartInstance;
	class ModelInstance;
	class JointInstance;
	class DataModelMesh;
	class Decal;
	class Weld;
	class Animator;

	namespace HUMAN {
		class HumanoidState;
	}

	namespace Soundscape {
		class SoundChannel;
	}

	extern const char* const sHumanoid;
	class Humanoid 
		: public DescribedCreatable<Humanoid, Instance, sHumanoid>
		, public KernelJoint		// Implements "computeForce"
		, public IAdornable
		, public ICharacterSubject
		, public IModelModifier
		, public IStepped
	{
	public:
		enum NameOcclusion
		{
			NAME_OCCLUSION_NONE = 0,
			NAME_OCCLUSION_ENEMY = 1,
			NAME_OCCLUSION_ALL = 2,
		};

		enum HumanoidDisplayDistanceType
		{
			HUMANOID_DISPLAY_DISTANCE_TYPE_VIEWER = 0,
			HUMANOID_DISPLAY_DISTANCE_TYPE_SUBJECT = 1,
			HUMANOID_DISPLAY_DISTANCE_TYPE_NONE = 2,
		};

		enum HumanoidRigType
		{
			HUMANOID_RIG_TYPE_R6 = 0,
			HUMANOID_RIG_TYPE_R15 = 1,
		};
	private:
        friend unsigned int HUMAN::HumanoidState::checkComputeEvent(); // only used to check for exploits.

		typedef DescribedCreatable<Humanoid, Instance, sHumanoid> Super;

		/////////////////////////////////////////////
		// REFLECTED DATA
		shared_ptr<PartInstance> seatPart;		// seat the humanoid is sitting in
		shared_ptr<PartInstance> walkToPart;	// if null, then use the walk speed control
		Vector3 walkToPoint;					// in part coordinate Frame
		Vector3 walkDirection;					// x == xvalue, y == 0, z== zvalue
		Vector3 luaMoveDirection;				// unit vector, used to move humanoid continously in that direction
		Vector3 rawMovementVector;              // unit vector, stores the raw input (never world adjusted)
		Vector3 targetPoint;
		Vector3 replicatedTargetPoint;
		float walkAngleError;
		HeapValue<float> walkSpeed;
		HeapValue<float> walkSpeedShadow;  // due to exploits.
        HeapValue<float> percentWalkSpeed;                 // used to make walk speed variable (for joysticks and the like)
		HeapValue<float> health;
		HeapValue<float> maxHealth;
        mutable ObscureValue<size_t> walkSpeedErrors; // only used in const member functions...
		HeapValue<float> jumpPower;
		HeapValue<float> maxSlopeAngle;
		HeapValue<float> hipHeight;
		bool torsoArrived;
		bool jump;
		bool autoJump; 
		bool sit;
		bool touchedHard;
		bool strafe;
		bool localSimulating;					// am I simulating this humanoid?
		bool ownedByLocalPlayer;				// is this my own humanoid?
		bool typing;
		bool autorotate;
		HumanoidRigType rigType;
		HeapValue<bool> platformStanding;

		bool autoJumpEnabled;

		bool activatePhysics;
		Vector3 activatePhysicsImpulse;			//

		int ragdollCriteria;
		int numContacts;						// Used to signal nearlyTouched event
		NameOcclusion nameOcclusion;
		HumanoidDisplayDistanceType displayDistanceType;
		float nameDisplayDistance;
		float healthDisplayDistance;
		Vector3 cameraOffset;					// When this humanoid is used as a camera target, offset the camera target by this vector

		bool isWalkingFromStudioTouchEmulation;
		
		std::string displayText;
		ContentFilter::FilterResult filterState;

		///////////////////////////////////////////////
		// INTERNAL DATA
		// Controller interface - server side
		bool isWalking;
		double walkTimer;
		Vector3 getDeltaToGoal() const;
		void setWalkMode(bool walking);
		void stepWalkMode(double gameDt);
		
		bool clickToWalkEnabled;

        bool stateTransitionEnabled[HUMAN::NUM_STATE_TYPES];

		Vector3 lastFloorNormal;
		// Humanoid Network Floor Platforms
		boost::shared_ptr<PartInstance> lastFloorPart;
		boost::shared_ptr<PartInstance> rootFloorMechPart;
		int lastFilterPhase;

		// hack - easiest place to update is on query of had neck
		bool hadNeck;							// has this humanoid ever had a neck?  Only break joints on death if so
		bool hadHealth;							// has this humanoid ever had health > 0?  Only break joints on death if so

		Vector3 pos0; // for looking for movement spikes
		Vector3 pos1;
		Vector3 pos2;

		void updateHadHealth() {
			hadHealth = hadHealth || (health > 0.0f);
		}

		typedef enum {TORSO, HEAD, RIGHT_ARM, LEFT_ARM, RIGHT_LEG, LEFT_LEG, VISIBLE_TORSO, APPENDAGE_COUNT} AppendageType;

		shared_ptr<PartInstance> appendageCache[APPENDAGE_COUNT];
		shared_ptr<PartInstance> baseInstance;

		rbx::signals::scoped_connection characterChildAdded;		
		rbx::signals::scoped_connection characterChildRemoved;				
		void onEvent_ChildModified(shared_ptr<Instance> child);

		boost::unordered_map<shared_ptr<PartInstance>, rbx::signals::scoped_connection> siblingMap;
		void updateSiblingPropertyListener(shared_ptr<PartInstance> sibling);
		void onEvent_SiblingPropertyChanged(const RBX::Reflection::PropertyDescriptor* desc);

		shared_ptr<StatusInstance> status;

		rbx::signals::scoped_connection onCFrameChangedConnection;
		rbx::signals::scoped_connection humanoidEquipConnection;

		void setCachePointerByType(AppendageType appendage, PartInstance *part);
		void updateBaseInstance();

		const PartInstance* getConstAppendageSlow(AppendageType appendage) const;
		PartInstance* getAppendageFast(AppendageType appendage, shared_ptr<PartInstance>& appendagePart);
		PartInstance* getAppendageSlow(AppendageType appendage);

		World* world;
		shared_ptr<HUMAN::HumanoidState> currentState;
		HUMAN::StateType previousState;

		shared_ptr<Animator> animator;
		Animator* getAnimator();

		void updateLocalSimulating();					// Do I need to simulate this humanoid?

		void onLocalHumanoidEnteringWorkspace();

		void onCFrameChangedFromReflection();

		bool hasWalkToPoint(Vector3& worldPosition) const;
	
		bool canClickToWalk() const;

		void setLocalTransparencyModifier(float transparencyModifier) const;

		void setWalkDirectionInternal(const Vector3& value, bool raiseSignal);
		void setJumpInternal(bool value, bool replicate);


		///////////////////////////////////////////////////////////////////////////
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const;
		/*override*/ void onAncestorChanged(const AncestorChanged& event);
		/*override*/ void setName(const std::string& value);
		/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
		/*override*/ void onDescendantAdded(Instance* instance);
		/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);

		///////////////////////////////////////////////////////////////////////////
		// IAdornable
		/*override*/ bool shouldRender3dAdorn() const {return true;}
		/*override*/ bool shouldRender3dSortedAdorn() const {return true;}
		/*override*/ Vector3 render3dSortedPosition() const;
		/*override*/ void render3dAdorn(Adorn* adorn);
		/*override*/ void render3dSortedAdorn(Adorn* adorn);

		void renderMultiplayer(Adorn* adorn, const RBX::Camera& camera);

		void renderBillboard(Adorn* adorn, const RBX::Camera& camera);
		void renderBillboardImpl(Adorn* adorn, const Vector2& screenLoc, float fontSize, const Color3& nameTagColor, float nameAlpha, float healthAlpha);

		///////////////////////////////////////////////////////////////////////////
		// SteppedInstance
		/*override*/ void onStepped(const Stepped& event);

		///////////////////////////////////////////////////////////////////////////
		// KernelJoint / Connector
		/*override*/ void computeForce(bool throttling);
		/*override*/ Body* getEngineBody()						{return getRootBodyFast();}
		/*override*/ virtual KernelType getConnectorKernelType() const { return Connector::HUMANOID; }

		///////////////////////////////////////////////////////////////////////////
		// ILocation
		/*override*/ const CoordinateFrame getLocation();

		///////////////////////////////////////////////////////////////////////////
		// CameraSubject
		/*override*/ const CoordinateFrame getRenderLocation();
		/*override*/ const Vector3 getRenderSize();

		///////////////////////////////////////////////////////////////////////////
		// ICharacterSubject 
		/*override*/ float getYAxisRotationalVelocity() const; // used for camera control
		/*override*/ void setFirstPersonRotationalVelocity(const Vector3& desiredLook, bool firstPersonOn);
		/*override*/ void getSelectionIgnorePrimitives(std::vector<const Primitive*>& primitives);
		/*override*/ virtual bool hasFocusCoord() const {return getHeadSlow() != NULL;}

		// Humanoid Platform Networking
		bool validateNetworkUpdateDistance(PartInstance* floorPart, CoordinateFrame& previousFloorPosition, float& netDt);
		void truncateDisplacementIfObstacle(PartInstance* floorPart, Vector3& newCharPosition, const Vector3& previousCharPosition);
		Vector3 getSimulatedFrictionVelocityOffset(PartInstance* floorPart, Vector3& newCharPosition, const Vector3& previousCharPosition, const Vector3& charPosInFloorSpace, const RBX::Velocity& previousFloorVelocity, float& netDt);
		const Velocity getHumanoidRelativeVelocityToPart(Primitive* floorPrim);
	
	public:
		void updateNetworkFloorPosition(PartInstance* floorPart, CoordinateFrame& previousFloorPosition, RBX::Velocity& lastFloorVelocity, float& netDt);
		bool shouldNotApplyFloorVelocity(Primitive* floorPrim);
		bool primitiveIsLastFloor(Primitive* prim);
		void updateFloorSimPhaseCharVelocity(Primitive* floorPrim);
		// End Humanoid Platform Networking


		/*override*/ void tellCameraNear(float distance) const;
		/*override*/ void tellCameraSubjectDidChange(shared_ptr<Instance> oldSubject, shared_ptr<Instance> newSubject) const;
		/*override*/ void tellCursorOver(float cursorOffset) const;
		/*override*/ void getCameraIgnorePrimitives(std::vector<const Primitive*>& primitives);
		/*override*/ Velocity calcDesiredWalkVelocity() const; // used for camera control

		static float autoTurnSpeed()		{return 8.0f;}

		Humanoid();
		~Humanoid();

		bool waitingForTorso() { return !torsoArrived; }
		bool isLegalForClientToChange(const Reflection::PropertyDescriptor& desc) const;
		int getPlayerId();

		enum Status
		{
			POISON_STATUS = 0,
			CONFUSION_STATUS = 1,
		};
		bool hasStatus(Status status);
		bool addStatus(Status status);
		bool removeStatus(Status status);
		bool hasCustomStatus(std::string status);
		bool addCustomStatus(std::string status);
		bool removeCustomStatus(std::string status);
		bool isLocalSimulating() const {return localSimulating;}
		bool computeNearlyTouched();

        bool getStateTransitionEnabled(HUMAN::StateType state); 
        void setStateTransitionEnabled(HUMAN::StateType state, bool enabled);

		shared_ptr<const Reflection::ValueArray> getStatuses();
		rbx::signal<void(Status)> statusAddedSignal;
		rbx::signal<void(Status)> statusRemovedSignal;
		rbx::signal<void(std::string)> customStatusAddedSignal;
		rbx::signal<void(std::string)> customStatusRemovedSignal;
		
		rbx::remote_signal<void(shared_ptr<Instance>)> serverEquipToolSignal;

		// Humanoid Network Update Connection
		rbx::signals::scoped_connection         onPositionUpdatedByNetworkConnection;

		NameOcclusion getNameOcclusion() const { return nameOcclusion; }
		void setNameOcclusion(NameOcclusion value);

		HumanoidDisplayDistanceType getDisplayDistanceType() const { return displayDistanceType; }
		void setDisplayDistanceType(HumanoidDisplayDistanceType value);

		static Humanoid* humanoidFromBodyPart(Instance* bodyPart);
		static const Humanoid* constHumanoidFromBodyPart(const Instance* bodyPart);
		static const Humanoid* constHumanoidFromDescendant(const Instance* bodyPart);

		static Humanoid* modelIsCharacter(Instance* testModel);
		static const Humanoid* modelIsConstCharacter(const Instance* testModel);

		static Humanoid* getLocalHumanoidFromContext(Instance* context);
		static const Humanoid* getConstLocalHumanoidFromContext(const Instance* context);

		static PartInstance* getLocalHeadFromContext(Instance* context);
		static const PartInstance* getConstLocalHeadFromContext(const Instance* context);

		static ModelInstance* getCharacterFromHumanoid(Humanoid* humanoid);
		static const ModelInstance* getConstCharacterFromHumanoid(const Humanoid* humanoid);

		static PartInstance* getHeadFromCharacter(ModelInstance* character);
		static const PartInstance* getConstHeadFromCharacter(const ModelInstance* character);

		static Weld* getGrip(Instance* character);

		static const Vector3 &defaultCharacterCorner() { static Vector3 corner(2.5f,2.5f,2.5f); return corner; }

		// By Definition, these are all called by HumanoidState - these are reflected
		rbx::signal<void()> diedSignal;
		rbx::signal<void(float)> swimmingSignal;
		rbx::signal<void(float)> runningSignal;				// state change scripts			
		rbx::signal<void(float)> climbingSignal;			// state change scripts
		rbx::signal<void(bool)> jumpingSignal;				// state change scripts
		rbx::signal<void(bool)> freeFallingSignal;
		rbx::signal<void(bool)> strafingSignal;
		rbx::signal<void(bool)> gettingUpSignal;
		rbx::signal<void(bool)> fallingDownSignal;
		rbx::signal<void(bool)> ragdollSignal;
		rbx::signal<void(bool, shared_ptr<Instance>)> seatedSignal;
		rbx::signal<void(bool)> platformStandingSignal;
        rbx::signal<void(RBX::HUMAN::StateType, RBX::HUMAN::StateType)> stateChangedSignal;
        rbx::signal<void(RBX::HUMAN::StateType, bool)> stateEnabledChangedSignal;
		RBX::HUMAN::StateType getCurrentStateType();
		RBX::HUMAN::StateType getPreviousStateType() { return previousState; };
		void setPreviousStateType(RBX::HUMAN::StateType newState);
		void changeState(RBX::HUMAN::StateType state);
		static bool isStateInString(const std::string& text, const RBX::HUMAN::StateType &compare, RBX::HUMAN::StateType& value);

		rbx::signal<void(float)> healthChangedSignal;

		// Internal use only - no reflection - happens both client, server
		rbx::signal<void()> doneSittingSignal;
		rbx::signal<void()> donePlatformStandingSignal;

		void equipToolInstance(shared_ptr<Instance> instance);
		void equipTool(RBX::Tool* tool);
		void unequipTools();

		void setWalkSpeed(float value);
		float getWalkSpeed() const {return walkSpeed;}
        void testWalkSpeed(float walkSpeed, float percentWalkSpeed) const
        {
            walkSpeedErrors = (walkSpeed > walkSpeedShadow) ? walkSpeedErrors + 1 : 0;
            if (walkSpeedErrors > 8)
            {
                RBX::Security::setHackFlagVs<LINE_RAND4>(RBX::Security::hackFlag11, HATE_SPEEDHACK);
            }
            if (fabs(percentWalkSpeed) > 1.01)
            {
                RBX::Security::setHackFlagVs<LINE_RAND4>(RBX::Security::hackFlag11, HATE_SPEEDHACK);
            }
        }
        
        void setPercentWalkSpeed(float value);
        float getPercentWalkSpeed() const { return percentWalkSpeed; }

		void setJumpPower(float value);
		float getJumpPower() const { return jumpPower; }

		void setMaxSlopeAngle(float value);
		float getMaxSlopeAngle() const { return maxSlopeAngle; }

		const Vector3 &getLastFloorNormal() const { return lastFloorNormal; }
		void setLastFloorNormal(const Vector3 &norm) { lastFloorNormal = norm; }

		void setHipHeight(float value);
		float getHipHeight() const { return hipHeight; }

		// Health / damage interface
		void setHealth(float value);
		void setHealthUi(float value);
		void zeroHealthLocal() {health = 0.0f;}		// for a non-simulating client -zero out health without bouncing back to the server
		float getHealth() const {return health;}

		void setMaxHealth(float value);
		float getMaxHealth() const {return maxHealth;}

		void setTyping(bool value) { typing = value; }
		bool getTyping() const { return typing; }

		void takeDamage(float value);		// honors explosions, etc.

		void setClickToWalkEnabled(bool value) { clickToWalkEnabled = value; }

		// Controller interface - don't set the walk direction directly
		void setWalkDirection(const Vector3& value);				// compacted - z is in the y place
		Vector3 getWalkDirection() const {return walkDirection;}	// compacted - z is in the y place
		bool allow3dWalkDirection() const;

		void move(Vector3 walkVector, bool relativeToCamera);

		Vector3 getLuaMoveDirection() const { return luaMoveDirection;}
		void setLuaMoveDirection(const Vector3& value);
		Vector3 getRawMovementVector() const { return rawMovementVector; }

		bool getAutoJumpEnabled() const { return autoJumpEnabled; };
		void setAutoJumpEnabled(bool value);

		void setWalkAngleError(const float &value);
		float getWalkAngleError() const {return walkAngleError;}
             
		void setWalkToPoint(const Vector3& value);
		const Vector3& getWalkToPoint() const {return walkToPoint;}

		void setWalkToPart(PartInstance* value);
		PartInstance* getWalkToPart() const {return walkToPart.get();}

		void setSeatPart(PartInstance* value);
		PartInstance* getSeatPart() const {return seatPart.get();}

		void setJump(bool value);
		bool getJump() const { return jump; }

		void setAutoJump(bool value); 
		bool getAutoJump() const { return autoJump;} 

		void setSit(bool value);
		bool getSit() const { return sit; }

		void setAutoRotate(bool value);
		bool getAutoRotate() const { return autorotate; }

		void setTouchedHard(bool hit) { touchedHard = hit; }

		void setActivatePhysics(bool flag, const Vector3& impulse) 
		{ 
			activatePhysics = flag;
			if (flag)
				activatePhysicsImpulse += impulse;
			else
				activatePhysicsImpulse = impulse;
		}
		bool getActivatePhysics() { return activatePhysics; }  
		const Vector3 getActivatePhysicsImpulse() { return activatePhysicsImpulse; }  
        bool getTouchedHard() { return touchedHard; }

		void setRagdollCriteria(int value);
        int getRagdollCriteria() const { return ragdollCriteria; }

		void setPlatformStanding(bool value);
		bool getPlatformStanding() const { return platformStanding; }

		void setStrafe(bool value);
		bool getStrafe() const { return strafe; }

		bool getDead() const;

		void setHadNeck() {hadNeck = true;}
		bool breakJointsOnDeath() const {return hadNeck && hadHealth;}

		void setTargetPoint(const Vector3& value);
		void setTargetPointLocal(const Vector3& value);	// does not replicate
		const Vector3& getTargetPoint() const {return targetPoint;}

		void setNameDisplayDistance(float d);
		float getNameDisplayDistance() const { return nameDisplayDistance; }

		void setHealthDisplayDistance(float d);
		float getHealthDisplayDistance() const { return healthDisplayDistance; }

		void setCameraOffset(const Vector3 &value);
		const Vector3 &getCamearaOffset() const { return cameraOffset; }

		// Humanoid Platform Update Getters and Setters;
		void setLastFloor(PartInstance* part) 
		{ 
			if(lastFloorPart)
			{
				lastFloorPart.reset();
			}
			lastFloorPart = shared_from(part); 
		}
		const shared_ptr<PartInstance>& getLastFloor() const { return lastFloorPart; }

		void setRootFloorPart(PartInstance* part)
		{
			if (rootFloorMechPart)
			{
				rootFloorMechPart.reset();
			}
			rootFloorMechPart = shared_from(part);
		}
		shared_ptr<PartInstance> getRootFloorPart() const { return rootFloorMechPart; }

		int getCurrentFloorFilterPhase();
		int getCurrentFloorFilterPhase(Assembly* floorAssembly);

		void setLastFloorPhase(int phase) { lastFilterPhase = phase; }
		int getLastFloorPhase() const { return lastFilterPhase; }

		// Walk Utilities
		void moveTo(const Vector3& worldPosition, PartInstance* part);
		void moveTo2(Vector3 worldPosition, shared_ptr<Instance> part);
		rbx::signal<void(bool)> moveToFinishedSignal;

        bool getUseR15() const { return (rigType != HUMANOID_RIG_TYPE_R6); }
		Humanoid::HumanoidRigType getRigType() const { return rigType; }
        void setRigType(Humanoid::HumanoidRigType type);

		// Build Joints
		void buildJoints(RBX::DataModel* dm = NULL);
		void buildJointsFromAttachments(PartInstance* part, std::vector<PartInstance*>& characterParts);

		JointInstance* getRightShoulder();
		Joint* getNeck();

		// Primitive
		void getPrimitives(std::vector<Primitive*>& primitives) const;
		
		void getParts(std::vector<PartInstance*>& primitives) const;

		PartInstance* getTorsoDangerous() const;		// reflection only
		PartInstance* getLeftLegDangerous() const;		// reflection only
		PartInstance* getRightLegDangerous() const;		// reflection only

		// TODO: No internal buffering - after refactor, rename without the word slow
		PartInstance* getTorsoSlow();		// no internal buffering
		PartInstance* getVisibleTorsoSlow();
		PartInstance* getHeadSlow();
		PartInstance* getLeftLegSlow();
		PartInstance* getRightLegSlow();
		PartInstance* getLeftArmSlow();
		PartInstance* getRightArmSlow();
		StatusInstance* getStatusSlow();

		const PartInstance* getTorsoSlow() const;		// no internal buffering
		const PartInstance* getVisibleTorsoSlow() const;
		const PartInstance* getHeadSlow() const;
		const PartInstance* getLeftLegSlow() const;
		const PartInstance* getRightLegSlow() const;
		const PartInstance* getLeftArmSlow() const;
		const PartInstance* getRightArmSlow() const;
		const StatusInstance* getStatusSlow() const;

		Primitive* getTorsoPrimitiveSlow();
		Primitive* getHeadPrimitiveSlow();

		// Internal buffering
		PartInstance* getTorsoFast();
		PartInstance* getVisibleTorsoFast();
		PartInstance* getHeadFast();
		PartInstance* getLeftLegFast();
		PartInstance* getRightLegFast();
		PartInstance* getLeftArmFast();
		PartInstance* getRightArmFast();
		StatusInstance* getStatusFast();

		Primitive* getTorsoPrimitiveFast();

		// Body
		inline Body* getTorsoBodyFast()
		{
			Primitive* prim = getTorsoPrimitiveFast();
			return prim ? prim->getBody() : NULL;
		}
		Body* getRootBodyFast();

		// Attachment Points
		CoordinateFrame getTopOfHead() const;
		CoordinateFrame getRightArmGrip() const;

		float getTorsoHeading() const;		// 0 == North == -Z.  Pi/2 = WEST == -x
		float getTorsoElevation() const;

		void setTorso(PartInstance* value);
		void setLeftLeg(PartInstance* value);
		void setRightLeg(PartInstance* value);
		
		void setHeadMesh(DataModelMesh* value);
		void setHeadDecal(Decal* value);

		World* getWorld()							{return world;};
		const World* getConstWorld() const			{return world;};

		static void renderWaypoint(Adorn* adorn, const Vector3& waypoint);

		// proxy interface to Animator object.
		shared_ptr<Instance> loadAnimation(shared_ptr<Instance> animation);
		bool CheckTorso();
		void setupAnimator();
		shared_ptr<const Reflection::ValueArray> getPlayingAnimationTracks();

		rbx::signal<void(shared_ptr<Instance>)> animationPlayedSignal;

		bool getOwnedByLocalPlayer() const { return ownedByLocalPlayer; }
		
		bool getWalkingFromStudioTouchEmulation() const { return isWalkingFromStudioTouchEmulation; }
		void setWalkingFromStudioTouchEmulation(bool value) { isWalkingFromStudioTouchEmulation = value; }
	};

}  // namespace RBX
