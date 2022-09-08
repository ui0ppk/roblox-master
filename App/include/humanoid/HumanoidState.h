/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once


#include "Util/HitTestFilter.h"
#include "GfxBase/IAdornable.h"
#include "Util/Name.h"
#include "rbx/Debug.h"
#include "Util/Velocity.h"
#include "rbx/boost.hpp"
#include "Reflection/Event.h"
#include "G3D/Array.h"
#include "util/PartMaterial.h"

#include <vector>

#define CHARACTER_FORCE_DEBUG	0

namespace RBX
{
	class Humanoid;
	class PartInstance;
	class Controller;
	class Assembly;
	class GeometryService;

	namespace HUMAN
	{
		typedef enum  {	FALLING_DWN = 0,
						RAGDOLL,				// 1
						GETTING_UP,
						JUMPING,
						SWIMMING,
						FREE_FALL,				// 4: balancing, no thrust
						FLYING,
						LANDED,					// 6: can't jump
						RUNNING,				// 7
						RUNNING_SLAVE,			// 8: slave side - lock to running mode to do accurate physics when touched slave side
						RUNNING_NO_PHYS,		// 9
						STRAFING_NO_PHYS,
						CLIMBING,
						SEATED,
						PLATFORM_STANDING,
						DEAD,
						PHYSICS,
						NUM_STATE_TYPES,
						xx					}	StateType;		// XX == NO change

		typedef enum {	NO_HEALTH = 0,		// Humanoid Commands
						NO_NECK,
						JUMP_CMD,
						STRAFE_CMD,
						NO_STRAFE_CMD,
						SIT_CMD,
						NO_SIT_CMD,
						PLATFORM_STAND_CMD,
						NO_PLATFORM_STAND_CMD,
						TIPPED,				// Tilting
						UPRIGHT,
						FACE_LDR,			// Ladder
						AWAY_LDR,
						OFF_FLOOR,			// Floor
						OFF_FLOOR_GRACE,	// Floor w/ Grace Period
						ON_FLOOR,
						TOUCHED,			// Other Objects
						NEARLY_TOUCHED,		// Other Objects
						TOUCHED_HARD,
						ACTIVATE_PHYSICS,
						FINISHED,
						TIMER_UP,			//	FINISHED_FALLING, READY_TO_JUMP,
						NO_TOUCH_ONE_SECOND,
						HAS_GYRO,
						HAS_BUOYANCY,
						NO_BUOYANCY,
						NUM_EVENT_TYPES		}	EventType;

#if CHARACTER_FORCE_DEBUG
		class DebugRay 
		{
		public:
			RbxRay ray;
			Color3 color;

			DebugRay(const RbxRay& _ray, const Color3& _color)
			{
				ray = _ray;
				color = _color;
			}

			void Draw(Adorn* adorn);
		};
#endif

		class HumanoidState :	public INamed,
								public HitTestFilter

		{
        public: 
            static const unsigned int kCorrectCheckValue = 2;

		private:           
            const Vector3& unitializedFloorTouch() const {
				static Vector3 v(1e15f, 1e15f, 1e15f);
				return v;
			}

			Humanoid* humanoid;
			float timer;
			float noTouchTimer;
			bool nearlyTouched;
			bool shouldRender;
			bool finished;
			bool outOfWater;
			bool headClear;
			StateType priorState;
			StateType luaState;

			G3D::Array<PartInstance*> foundParts;		// temp buffer
			bool facingLadder;
			shared_ptr<PartInstance> floorPart;	
			PartMaterial			 floorMaterial;
			Vector3 floorTouchInWorld;
			Vector3 floorTouchNormal;
			Vector3 floorHumanoidLocationInWorld;
			float noFloorTimer;

			// Cut down firing of the running event to when there are major difference in velocity
			float lastMovementVelocity;

			// signals that the state needs these updated
			bool usesEvent(EventType e) const;
			bool usesLadder() const {return (usesEvent(FACE_LDR) || usesEvent(AWAY_LDR));}
			bool usesFloor() const {return (usesEvent(OFF_FLOOR) || usesEvent(ON_FLOOR) || usesEvent(OFF_FLOOR_GRACE));}

			float computeTilt() const;
			bool computeTipped() const;
			bool computeUpright() const;
			bool computeHasGyro() const;
			bool computeJumped() const;
			float computeFloorTilt() const;

			void setLegsCanCollide(bool canCollide);
			void setArmsCanCollide(bool canCollide);
			void setHeadCanCollide(bool canCollide);
			void setTorsoCanCollide(bool canCollide);

			int ladderCheck;
			virtual int ladderCheckRate() { return 2; }
			bool findPrimitiveInLadderZone(Adorn* adorn);
			bool findLadder(Adorn* adorn);			
			void doLadderRaycast(GeometryService *geom, const RbxRay& caster,Humanoid* humanoid, Primitive** hitPrimOut, 
									Vector3* hitLocationOut);
			void doAutoJump(); 
			void findFloor(shared_ptr<PartInstance>& oldFloor);
			shared_ptr<PartInstance> tryFloor(const RbxRay& ray, Vector3& hitLocation, Vector3& hitNormal, float maxDistance, Assembly* humanoidAssembly, PartMaterial& recentFloorMaterial);
			void AverageFloorRayCast(shared_ptr<PartInstance> &floorPart, Vector3& floorPartHitLocation, Vector3& floorPartHitNormal,
										PartMaterial& floorPartHitMaterial, Vector3& hitLocationAccumulator, int& hitLocationCount, const bool UpdateFloorPart, 
										const Vector3& offset, const float maxDistance,	Assembly* humanoidAssembly, const CoordinateFrame& torsoC);
			void preStepFloor();
			void preStepCollide();
			void preStepSimulatorSide(float dt);
			void preStepSlaveSide()				{preStepCollide();}

			static void doSimulatorStateTable(shared_ptr<HumanoidState>& state, float dt);
			static void doSlaveStateTable(shared_ptr<HumanoidState>& state, StateType newType);
			static HumanoidState* create(StateType newType, StateType oldType, Humanoid* humanoid);
			static HumanoidState* createNew(StateType newType, StateType oldType, Humanoid* humanoid);
			static void changeState(shared_ptr<HumanoidState>& state, StateType newType);

			void fireEvent(StateType stateType, bool entering);

			// Override hitTestFiler
			/*override*/ Result filterResult(const Primitive* testMe) const;

		protected:

			// For debugging
			Vector3 maxTorque;
			Vector3 maxForce;
			float maxContactVel;
			Vector3 lastTorque;
			Vector3 lastForce;
			float lastContactVel;

			static float minMoveVelocity()					{return 0.5f;}
			static float maxClimbDistance()					{return 2.45f;}			// studs
			static const Vector3 maxMoveForce()				{static Vector3 m(1000.0f, 10000.0f, 1000.0f);   return m;}		//const Vector3 maxMoveAccelerationGrid(5e4f*0.02f, 5e5f*0.02f, 1e4f*0.02f);
			static const Vector3 minMoveForce()				{static Vector3 m(-1000.0f, 0.0f,	 -1000.0f); return m;}
			static const Vector3 maxSwimmingMoveForce()		{static Vector3 m(10000.0f, 1000.0f, 10000.0f); return m;}
			static const Vector3 minSwimmingMoveForce()		{static Vector3 m(-10000.0f, -10000.0f, -10000.0f); return m;}
			static float fallDelay()						{ return 0.125f; }
			static float maxLinearMoveForce()				{ return 143.0; }

		// Need public for some Physics calculations outside of Humanoid State
		public:
			float steepSlopeAngle() const;
			static float runningKMoveP();
			static float runningKMovePForPGS();
			static float maxLinearGroundMoveForce()			{ return 500.0; }

		protected:
			Assembly* filteringAssembly;

			bool computeEvent(EventType eventType);

			bool computeTouched();
			bool computeNearlyTouched();
			bool computeTouchedByMySimulation();
			bool computeTouchedHard();
			bool computeActivatePhysics();

			void setOutOfWater() { outOfWater = true; }
			bool getOutOfWater() const { return outOfWater; }

			void setTimer(float time) {timer = time;}
			float getTimer() const {return timer;}

			bool getFinished() const {return finished;}
			void setFinished(bool value) {finished = value;}

			bool getFacingLadder() const {return facingLadder;}
			bool getHeadClear() const { return headClear; }

			PartMaterial getFloorMaterial() const { return floorMaterial; }
			float getFloorFrictionProperty(Primitive* floorPrim) const;

			Primitive* getFloorPrimitive();
			const Primitive* getFloorPrimitiveConst() const;
			const Vector3& getFloorTouchInWorld() const {
				RBXASSERT(floorTouchInWorld != unitializedFloorTouch());
				return floorTouchInWorld;
			}
			const Vector3& getFloorTouchNormal() const {
				RBXASSERT(floorTouchInWorld != unitializedFloorTouch());
				return floorTouchNormal;
			}
			const Vector3& getFloorHumanoidLocationInWorld() const {
				RBXASSERT(floorHumanoidLocationInWorld != unitializedFloorTouch());
				return floorHumanoidLocationInWorld;
			}


			const Velocity getFloorPointVelocity();
			Vector3 getRelativeMovementVelocity();
			float getDesiredAltitude() const;

#if CHARACTER_FORCE_DEBUG
			std::vector<DebugRay> debugRayList;
#endif

			void fireMovementSignal(rbx::signal<void(float)>& movementSignal, float movementVelocity);

			// Tick Count - 30 FPS, state table occurs here
			virtual void onComputeForceImpl() = 0;
			virtual void onStepImpl() {}
			virtual void onSimulatorStepImpl(float stepDt) {}

		protected:
			void setCanThrottleState(bool canThrottle);		// only the Seated state can throttle - its joined to parts so it must

			// Attributes moved in assemblies
			Assembly* getAssembly();
			const Assembly* getAssemblyConst() const;
			void stateToAssembly();
			StateType stateFromAssembly();

		public:
			virtual bool armsShouldCollide()		const	{return true;}	
			virtual bool legsShouldCollide()		const	{return true;}	
			virtual bool headShouldCollide()		const	{return true;}
			virtual bool torsoShouldCollide()		const	{return true;}

			virtual bool enableAutoJump()			const	{ return true; }

			virtual void onCFrameChangedFromReflection() { preStepFloor();}	// recalculate floor part


			HumanoidState(Humanoid* humanoid, StateType priorState); 

			virtual ~HumanoidState();

			const Humanoid* getHumanoidConst() const;

			Humanoid* getHumanoid() {
				return const_cast<Humanoid*>(getHumanoidConst());
			}

			static HumanoidState* defaultState(Humanoid* humanoid);	//	new Running(this));

			static void simulate(shared_ptr<HumanoidState>& state, float dt);

			static void updateHumanoidFloorStatus(shared_ptr<HumanoidState>& state);

			static bool hasFloorChanged(shared_ptr<HumanoidState>& state, Primitive* lastFloorPrim);

			static void noSimulate(shared_ptr<HumanoidState>& state);			// for non-simulating humanoids, match states

			virtual void fireEvents();

			virtual float getYAxisRotationalVelocity() const {return 0.0f;}

			float getCharacterHipHeight() const;

			void onComputeForce();

			bool torsoHasBuoyancy, leftLegHasBuoyancy, rightLegHasBuoyancy;
			std::vector<rbx::signals::connection> buoyancyConnections;
			void setTorsoHasBuoyancy( bool value ) { torsoHasBuoyancy = value; }
			void setLeftLegHasBuoyancy( bool value ) { leftLegHasBuoyancy = value; }
			void setRightLegHasBuoyancy( bool value ) { rightLegHasBuoyancy = value; }
			bool computeHasBuoyancy();

			virtual StateType getStateType() const = 0;

			void setLuaState(StateType state);
			StateType getLuaState() { return luaState; }

			static const char *getStateNameByType(StateType state);

			bool computeHitByHighImpactObject();

			// only in debug
			void render3dAdorn(Adorn* adorn);

			void setNearlyTouched();

            // for security purposes, get the address of this code.
            // A member function pointer is a compiler defined data structure.
            static inline const void* getComputeEventBaseAddress()
            {
#ifdef _WIN32
                bool (RBX::HUMAN::HumanoidState::* hsce)(EventType) = &computeEvent;
                if(sizeof(hsce) == 8 || sizeof(hsce) == 4)
                {
                    return (const void*&)(hsce); // odd, but required syntax for this horrible conversion.
                }
#endif
                return NULL;
            }

            unsigned int checkComputeEvent(); // this was added due to exploits.
		};

	}	// namespace HUMAN
} // namespace RBX
