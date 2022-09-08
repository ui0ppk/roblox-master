/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/Primitive.h"
#include "V8World/ContactManager.h"

#include "Util/IndexArray.h"
#include "rbx/rbxTime.h"
#include "rbx/signal.h"
#include "Util/SpatialRegion.h"
#include "Util/HeapValue.h"
#include "util/PhysicalProperties.h"

class btCollisionDispatcher;
class btCollisionWorld;
class btDefaultCollisionConfiguration;
namespace RBX {

	class JointInstance;
	class Joint;
	class Primitive;
	class Clump;
	class Assembly;
	class Region2;
    class PartInstance;

	namespace Profiling
	{
		class CodeProfiler;
	}

	struct RootPrimitiveOwnershipData
	{
		RBX::SystemAddress ownerAddress;
		bool ownershipManual;
		Primitive* prim;
	};

	class Edge;
	class Contact;
	class MotorJoint;

    // In Assembly.cpp
    void notifyAssemblyPrimitiveMoved(Primitive* p, bool resetContacts);
    
	class EThrottle {
	public:
		typedef enum { ThrottleDefaultAuto, ThrottleDisabled, ThrottleAlways, Skip2, Skip4, Skip8, Skip16} EThrottleType;

	private:
		int requestedSkip;
		int usedSkip;
		static int const throttleSetting[];
		int throttleIndex;

	public:
		static EThrottleType globalDebugEThrottle;

		EThrottle();

		bool computeThrottle(int step);

		bool increaseLoad(bool increase);

		float getEnvironmentSpeed() const;

		int getThrottleIndex() const { return throttleIndex; }

		void setThrottleIndex(int index) { throttleIndex = index; }

	};


	class World
	{
	friend class ContactManager;
	public:
		rbx::signal<void(Joint*, Primitive*, std::vector<Primitive*>&)> postInsertJointSignal;
		rbx::signal<void(Joint*, std::vector<Primitive*>&, std::vector<Primitive*>&)> postRemoveJointSignal;
		rbx::signal<void(Joint*)> autoJoinSignal;
		rbx::signal<void(Joint*)> autoDestroySignal;
		rbx::signal<void(std::pair<Primitive*, Primitive*>)> primitiveCollideSignal;

		struct TouchInfo
		{
			Primitive* p1;
			Primitive* p2;
			shared_ptr<PartInstance> pi1;
			shared_ptr<PartInstance> pi2;
			typedef enum { Touch, Untouch } Type;
			Type type;
		};
        
		struct OnPrimitiveMovingVisitor
		{
			ContactManager* ptr;
			OnPrimitiveMovingVisitor(ContactManager* p) : ptr(p) { }
			void operator()(Primitive* p) {
                notifyAssemblyPrimitiveMoved(p, true);
                ptr->onPrimitiveExtentsChanged(p);
            }
		};
        
	private:

		int							frmThrottle;
		EThrottle					eThrottle;

		G3D::Array<TouchInfo>		touchReporting;

		bool						inStepCode;	// debugging
		Joint*						inJointNotification;

		int							worldSteps;
		int							worldStepId;	// two years at 1/30 second dt
		float						worldStepAccumulated;  // Time unaccounted for in the last step

		HeapValue<float>			fallenPartDestroyHeight;

		// defining objects
		IndexArray<Primitive, &Primitive::worldIndexFunc>	primitives;
		Primitive*					groundPrimitive;		// for now, only used by kernel joints

		btCollisionDispatcher*	bulletDispatcher;
		btDefaultCollisionConfiguration* bulletCollisionConfiguration;

        // Generates unique ids for objects registered with this World
        boost::uint64_t             UIDGenerator;
		bool						usingPGSSolver;
        bool                        physicsAnalyzerEnabled;

		// Material Properties
		PhysicalPropertiesMode      physicalMaterialsMode;

		// Networked Interpolation Sync
		Time lastFrameTimeStamp;    // Timestamp of current frame beginning
		Time lastSendTimeStamp;     // Timestamp of the frame where we last sent physics
		int lastNumWorldSteps;
		double worldStepOffset;

	public:
		float getUpdateExpectedStepDelta();

		btCollisionDispatcher* getBulletCollisionDispatcher(void) { return bulletDispatcher; }

	private:

		// redundant data
		G3D::Array<Primitive*>		movingPrimitives;
		std::set<Joint*>			breakableJoints;
		int							numJoints;
		int							numContacts;
		int							numLinkCalls;

		// motion analytic
		double errorCount;
		double passCount;
		double frameinfosSize;
		double frameinfosTarget;
		double targetDelayTenths;
		double infosSizeTenths;
		double maxDelta;
		double frameinfosCount;


		// redundant data - performance - prevent allocation
		G3D::Array<Primitive*>							tempPrimitives;
		G3D::Array<boost::shared_ptr<JointInstance> >	tempJoints;

        boost::unordered_map< boost::uint64_t, Primitive* > primitiveIndexation;

		boost::scoped_ptr<Profiling::CodeProfiler> profilingBreak;
		boost::scoped_ptr<Profiling::CodeProfiler> profilingAssembly;
		boost::scoped_ptr<Profiling::CodeProfiler> profilingFilter;
		boost::scoped_ptr<Profiling::CodeProfiler> profilingWorldStep;
		boost::scoped_ptr<Profiling::CodeProfiler> profilingUiStep;

		void createAutoJoints(Primitive* p, std::set<Primitive*>* ignoreGroup, std::set<Primitive*>* joinGroup);	// if joinGroup, only connect with them
		void destroyAutoJoints(Primitive* p, std::set<Primitive*>* ignoreGroup, bool includeExplicit = true, bool includeAuto = true);	// if group, then keep joints between group members

		void destroyJoint(Joint* j);

		void removeFromBreakable(Joint* j);

		// these functions are in the main world->step() loop
		void doBreakJoints();			// goes through the breakableJoints, breaks if necessary;

		void uiStep(bool longStep, double distributedGameTime);
		void doWorldStep(bool throttling, int uiStepId, int numThreads, boost::uint64_t debugTime);

		void notifyMovingAssemblies();

		ContactManager*					contactManager;
		class SendPhysics*				sendPhysics;
		class CleanStage*				cleanStage;

		class GroundStage*				getGroundStage();
		class SleepStage*				getSleepStage();
		class TreeStage*				getTreeStage();
		const class SpatialFilter*		getSpatialFilter() const;
		class AssemblyStage*			getAssemblyStage();
		const AssemblyStage*			getAssemblyStage() const;
		class MovingAssemblyStage*		getMovingAssemblyStage();
		class StepJointsStage*			getStepJointsStage();
		const StepJointsStage*			getStepJointsStage() const;
		const class SleepStage*			getSleepStage() const;
		class SimulateStage*			getSimulateStage();

	public:
		World();
		~World();

		void assertNotInStep() { RBXASSERT(!inStepCode); }
		void assertInStep() { RBXASSERT(inStepCode); }

		class SendPhysics*				getSendPhysics();
		class SimSendFilter&			getSimSendFilter();
		SpatialFilter*					getSpatialFilter();
		ContactManager*					getContactManager()			{return contactManager;}
		const ContactManager*			getContactManager() const	{return contactManager;}
		class HumanoidStage*			getHumanoidStage();
		class Kernel*					getKernel();
		const Kernel*					getKernel() const;

		const G3D::Array<TouchInfo>&	getTouchInfoFromLastStep()			{return touchReporting;}
		void							clearTouchInfoFromLastStep()		{touchReporting.fastClear();}

		void							computeFallen(G3D::Array<Primitive*>& fallen) const;

		const G3D::Array<Primitive*>&	getPrimitives() const	{return primitives.underlyingArray();}

		// engine interface
		int updateStepsRequiredForCyclicExecutive(float desiredInterval);
		float step(bool longStep, double distributedGameTime, float desiredInterval, int numThreads);	// 10-100 frames per second
		void assemble();																	// on heartbeat, before collision detection
		bool isAssembled();
		void reset()								{RBXASSERT(!inStepCode); worldStepId = 0;}
		int getWorldStepId()						{return worldStepId;}
		float getWorldStepsAccumulated() { return worldStepAccumulated; }
		int getUiStepId();
		int getLongUiStepId();
		void sendClumpChangedMessage(Primitive* childPrim);
        
		EThrottle& getEThrottle()					{return eThrottle;}
		int getFRMThrottle() { return frmThrottle;}
		void setFRMThrottle(int value);

		Primitive* getGroundPrimitive()	{return groundPrimitive;}

		// PRIMITIVE - Runtime geometry manipulation
		void insertPrimitive(Primitive* p);
		void removePrimitive(Primitive* p, bool isStreamingRemove);
		void ticklePrimitive(Primitive* p, bool recursive);		// simulates a touch, wakes up
        Primitive* getPrimitiveFromBodyUID( boost::uint64_t uid ) const;

		// Auto Joining / Unjoining functions
		void joinAll();

		void createAutoJoints(Primitive* p);
		void createAutoJointsToWorld(const G3D::Array<Primitive*>& primitives);		// ignores joints between them
		void createAutoJointsToPrimitives(const G3D::Array<Primitive*>& primitives);		// only join each other in this group

		void destroyAutoJoints(Primitive* p, bool includeExplicit = true);
		void destroyAutoJointsToWorld(const G3D::Array<Primitive*>& primitives);	// ignores joints between them

        void destroyTerrainWeldJointsWithEmptyCells(Primitive* megaClusterPrim, const SpatialRegion::Id& region, Primitive* touchingPrim);
        void destroyTerrainWeldJointsNoTouch(Primitive* megaClusterPrim, Primitive* touchingPrim);

		// Joint based insert, remove
		void insertJoint(Joint* j);
		void removeJoint(Joint* j);	
		void jointCoordsChanged(Joint* j);
		void notifyMoved(Primitive* p);

		// Network Ownership API Data Gatherer
		void gatherMechDataPreJoin(Joint *j, Primitive*& unGroundedPrim, std::vector<Primitive*>& combiningRoots);
		void gatherMechDataPreSplit(Joint* j, std::vector<Primitive*>& prim0ChildRoots, std::vector<Primitive*>& prim1Roots);

		// CONTACT MANAGER - Can only be called by the contact manager;
		void insertContact(Contact* c);
		void destroyContact(Contact* c);

		// inquiry functions
		int getMetric(IWorldStage::MetricType metricType) const;
		int getNumBodies() const;
		int getNumPoints() const;
		int getNumConstraints() const;
		int getNumHashNodes() const;	
		int getMaxBucketSize() const;	
		int getNumLinkCalls() const					{return numLinkCalls;}
		int	getNumContacts() const					{return numContacts;}
		int	getNumJoints() const					{return numJoints;}
		int	getNumPrimitives() const				{return getPrimitives().size();}
		float getEnvironmentSpeed() const			{return eThrottle.getEnvironmentSpeed();}
		float getEnvironmentSpeedPercent() const	{return getEnvironmentSpeed() * 100.0f;}

		void setFallenPartDestroyHeight(float value) 		{fallenPartDestroyHeight = value;}
		float getFallenPartDestroyHeight() const			{return fallenPartDestroyHeight;}

		RBX::Profiling::CodeProfiler& getProfileWorldStep() { return *profilingWorldStep; }
		const RBX::Profiling::CodeProfiler& getProfileWorldStep() const { return *profilingWorldStep; }

		void loadProfilers(std::vector<RBX::Profiling::CodeProfiler*>& worldProfilers) const;

		// PRIMITIVE - notification on edits - only called by Primitive or Clump
		Assembly* onPrimitiveEngineChanging(Primitive* p);
		void onPrimitiveEngineChanged(Assembly* changing);

		void onPrimitiveFixedChanging(Primitive* p);		
		void onPrimitiveFixedChanged(Primitive* p);		

		void onPrimitivePreventCollideChanged(Primitive* p);
		void onPrimitiveExtentsChanged(Primitive* p);
		void onPrimitiveContactParametersChanged(Primitive* p);
		void onPrimitiveGeometryChanged(Primitive* p);
		void reportTouchInfo(const TouchInfo& info);
		void reportTouchInfo(Primitive* p0, Primitive* p1, World::TouchInfo::Type T);

		void onPrimitiveCollided(Primitive* p0, Primitive* p1);

		void onAssemblyPhysicsChanged(Assembly* a, bool physics) const;
		void onAssemblyInSimluationStage(Assembly* a);

		// JOINT - notification on edits - only called by Joint
		void onJointPrimitiveNulling(Joint* j, Primitive* p);
		void onJointPrimitiveSet(Joint* j, Primitive* p);

		void addAnimatedJointToMovingAssemblyStage(Joint* j);
		void removeAnimatedJointFromMovingAssemblyStage(Joint* j);

		bool getUsingPGSSolver();
		void setUsingPGSSolver(bool usePGS);
        void setUserId( int id );

        void setPhysicsAnalyzerEnabled(bool value) { physicsAnalyzerEnabled = value; }

		bool getUsingNewPhysicalProperties() const;

		PhysicalPropertiesMode getPhysicalPropertiesMode() const { return physicalMaterialsMode; }
		void setPhysicalPropertiesMode(PhysicalPropertiesMode mode);

		// motion analytic
		void plusErrorCount(double ec) { errorCount += ec;}
		void plusPassCount(double pc) { passCount += pc; }
		double getPassCount() {return passCount; }
		void sendAnalytics(void);
		void addFrameinfosStat(double fis, double fit, double tdt, double ist, double md, double fic);
	};

} // namespace

    
