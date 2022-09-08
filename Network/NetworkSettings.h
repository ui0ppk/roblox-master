#pragma once

#include "V8DataModel/GlobalSettings.h"
#include "RakNet/Source/PacketPriority.h"
#include "rbx/TaskScheduler.h"
#include "Util/ObscureValue.h"

namespace RBX
{
	extern const char* const sNetworkSettings;
	class NetworkSettings
		: public GlobalAdvancedSettingsItem<NetworkSettings, sNetworkSettings>
	{
	public:
		NetworkSettings();

		bool printSplitMessages;
		bool printPhysicsErrors;
		bool printInstances;
        bool printStreamInstanceQuota;
		bool printTouches;
		bool printProperties;
		bool printEvents;
		bool printPhysicsFilters;
		bool printDataFilters;
		bool printBits;
		bool trackDataTypes;
        bool trackPhysicsDetails;
		double incommingReplicationLag;
		typedef enum { ErrorComputation, ErrorComputation2, RoundRobin, TopNErrors } PhysicsSendMethod;
		typedef enum { Direct, Interpolation } PhysicsReceiveMethod;

		bool isQueueErrorComputed;

		bool usePhysicsPacketCache;
		bool useInstancePacketCache;

		bool profiling;
        bool profilecpu;
		bool profilerOneColumnPerStack;
		std::string profilerServerIp;
		int profilerServerPort;
		std::string profilerTag;
		double profilerTimedSeconds; // 0 or negative for un-timed profiling
        bool enableHeavyCompression;
        bool heavyCompressionEnabled();

	private:
		PhysicsSendMethod physicsSendMethod;
		PhysicsReceiveMethod physicsReceiveMethod;
		
		PacketPriority physicsSendPriority;
		PacketPriority dataSendPriority;

    public:

		PhysicsSendMethod getPhysicsSendMethod() const { return physicsSendMethod; }
		void setPhysicsSendMethod(const PhysicsSendMethod& value);

		PacketPriority getPhysicsSendPriority() const { return physicsSendPriority; }
		void setPhysicsSendPriority(const PacketPriority& value);

		PacketPriority getDataSendPriority() const { return dataSendPriority; }
		void setDataSendPriority(const PacketPriority& value);

		PhysicsReceiveMethod dummyGetPhysicsReceiveMethod() const { return physicsReceiveMethod; }
		void dummySetPhysicsReceiveMethod(const PhysicsReceiveMethod& value);

		const std::string getReportStatURL() const { return ""; }
		void setReportStatURL(const std::string&  value) {}

		static Reflection::BoundProp<bool> prop_DistributedPhysics;
		bool distributedPhysicsEnabled;

		int preferredClientPort;

		int getPhysicsMtuAdjust() const { return physicsMtuAdjust; }
		void setPhysicsMtuAdjust(int value);

		int getReplicationMtuAdjust() const { return replicationMtuAdjust; }
		void setReplicationMtuAdjust(int value);

		// Data-out throttle
		float getDataSendRate() const { return dataSendRate; }
		void setDataSendRate(float value);

		float getDataGCRate() const { return dataGCRate; }
		void setDataGCRate(float value);

		float getPhysicsSendRate() const { return physicsSendRate; }
		void setPhysicsSendRate(float value);

        float getClientPhysicsSendRate() const { return clientPhysicsSendRate; }
        void setClientPhysicsSendRate(float value);

		// Streaming debug
		void setExtraMemoryUsedInMB(int value);
		int getExtraMemoryUsedInMB() const { return extraMemoryUsed; }
		float getFreeMemoryMBytes() const;
        float getFreeMemoryPoolMBytes() const;

		bool getRenderStreamedRegions() const;
		void setRenderStreamedRegions(bool value);

        bool getShowPartMovementPath() const;
        void setShowPartMovementPath(bool value);
        int getTotalNumMovementWayPoint() const {return totalNumMovementWayPoint; }
        void setTotalNumMovementWayPoint(int value);
#ifdef RBX_TEST_BUILD
        int getTaskSchedulerFindJobFPS() const {return TaskScheduler::findJobFPS; }
        void setTaskSchedulerFindJobFPS(int value) {TaskScheduler::findJobFPS = value;}
        bool getTaskSchedulerUpdateJobPriorityOnWake() const {return TaskScheduler::updateJobPriorityOnWake; }
        void setTaskSchedulerUpdateJobPriorityOnWake(bool value) {TaskScheduler::updateJobPriorityOnWake = value;}
#endif

        bool getShowActiveAnimationAsset() const;
        void setShowActiveAnimationAsset(bool value);

        void printProfilingResult();

		float touchSendRate;

		int canSendPacketBufferLimit;
		int sendPacketBufferLimit;
		float networkOwnerRate;
		bool isThrottledByOutgoingBandwidthLimit;
		bool isThrottledByCongestionControl;

		// Data-in throttle
		double getReceiveRate() const { return receiveRate; }
		void setReceiveRate(double value);

		float getTouchSendRate() const { return touchSendRate; }
		void setTouchSendRate(float value);

		int deprecatedWaitingForCharacterLogRate;
	protected:
		int physicsMtuAdjust;
		int replicationMtuAdjust;

		// Data-out throttle
		ObscureValue<float> dataSendRate;
		ObscureValue<float> physicsSendRate;
        ObscureValue<float> clientPhysicsSendRate;
		float dataGCRate;

		// Data-in throttle
		ObscureValue<double> receiveRate;

		// streaming debug
		int extraMemoryUsed;

        // movement debug
        int totalNumMovementWayPoint;
	};

}

