#include "NetworkSettings.h"
#include "ReliabilityLayer.h"
#include "Util/Statistics.h"
#include "NetworkProfiler.h"
#include "util/MemoryStats.h"
#include "v8datamodel/Workspace.h"

namespace RBX
{
	const char* const sNetworkSettings = "NetworkSettings";

	namespace Reflection {
	template<> EnumDesc<NetworkSettings::PhysicsSendMethod>::EnumDesc()
	:EnumDescriptor("PhysicsSendMethod")
	{
		addPair(NetworkSettings::ErrorComputation, "ErrorComputation");
		addPair(NetworkSettings::ErrorComputation2, "ErrorComputation2");
		addPair(NetworkSettings::RoundRobin, "RoundRobin");
		addPair(NetworkSettings::TopNErrors, "TopNErrors");
	}

	template<> EnumDesc<NetworkSettings::PhysicsReceiveMethod>::EnumDesc()
	:EnumDescriptor("PhysicsReceiveMethod")
	{
		addPair(NetworkSettings::Direct, "Direct");
		addPair(NetworkSettings::Interpolation, "Interpolation");
	}

	template<> EnumDesc<PacketReliability>::EnumDesc()
	:EnumDescriptor("PacketReliability")
	{
		addPair(UNRELIABLE, "UNRELIABLE");
		addPair(UNRELIABLE_SEQUENCED, "UNRELIABLE_SEQUENCED");
		addPair(RELIABLE, "RELIABLE");
		addPair(RELIABLE_ORDERED, "RELIABLE_ORDERED");
		addPair(RELIABLE_SEQUENCED, "RELIABLE_SEQUENCED");
	}

	template<> EnumDesc<PacketPriority>::EnumDesc()
		:EnumDescriptor("PacketPriority")
	{
		addPair(IMMEDIATE_PRIORITY, "IMMEDIATE_PRIORITY");
		addPair(HIGH_PRIORITY, "HIGH_PRIORITY");
		addPair(MEDIUM_PRIORITY, "MEDIUM_PRIORITY");
		addPair(LOW_PRIORITY, "LOW_PRIORITY");
	}

	}//namespace Reflection

    REFLECTION_BEGIN();
	static Reflection::BoundProp<int> prop_PreferredClientPort("PreferredClientPort", "Network", &NetworkSettings::preferredClientPort);
	static Reflection::PropDescriptor<NetworkSettings, float> prop_DataSendRate("DataSendRate", "Network", &NetworkSettings::getDataSendRate, &NetworkSettings::setDataSendRate);
	static Reflection::PropDescriptor<NetworkSettings, float> prop_DataGCRate("DataGCRate", "Network", &NetworkSettings::getDataGCRate, &NetworkSettings::setDataGCRate);
	static Reflection::PropDescriptor<NetworkSettings, float> prop_PhysicsSendRate("PhysicsSendRate", "Network", &NetworkSettings::getPhysicsSendRate, &NetworkSettings::setPhysicsSendRate);
    static Reflection::PropDescriptor<NetworkSettings, float> prop_ClientPhysicsSendRate("ClientPhysicsSendRate", "Network", &NetworkSettings::getClientPhysicsSendRate, &NetworkSettings::setClientPhysicsSendRate);
	static Reflection::BoundProp<float> prop_SendRate("NetworkOwnerRate", "Network", &NetworkSettings::networkOwnerRate);
	static Reflection::PropDescriptor<NetworkSettings, float> prop_TouchSendRate("TouchSendRate", "Network", &NetworkSettings::getTouchSendRate, &NetworkSettings::setTouchSendRate);
	static Reflection::BoundProp<bool> prop_isThrottledByOutgoingBandwidthLimit("IsThrottledByOutgoingBandwidthLimit", "Network", &NetworkSettings::isThrottledByOutgoingBandwidthLimit);
	static Reflection::BoundProp<bool> prop_IsThrottledByCongestionControl("IsThrottledByCongestionControl", "Network", &NetworkSettings::isThrottledByCongestionControl);
	static Reflection::PropDescriptor<NetworkSettings, double> prop_ReceiveRate("ReceiveRate", "Network", &NetworkSettings::getReceiveRate, &NetworkSettings::setReceiveRate);
	static Reflection::BoundProp<int> prop_WaitingForCharacterLogRate("WaitingForCharacterLogRate", "Network", &NetworkSettings::deprecatedWaitingForCharacterLogRate, Reflection::PropertyDescriptor::Attributes::deprecated(Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
	static Reflection::PropDescriptor<NetworkSettings, std::string> prop_ReportStatUrl("ReportStatURL", "Network", &NetworkSettings::getReportStatURL, &NetworkSettings::setReportStatURL, Reflection::PropertyDescriptor::Attributes::deprecated(Reflection::PropertyDescriptor::HIDDEN_SCRIPTING));
	static Reflection::BoundProp<bool> prop_UsePhysicsPacketCache("UsePhysicsPacketCache", "Network", &NetworkSettings::usePhysicsPacketCache);
	static Reflection::BoundProp<bool> prop_UseInstancePacketCache("UseInstancePacketCache", "Network", &NetworkSettings::useInstancePacketCache);
	static Reflection::BoundProp<bool> propIsQueueErrorComputed("IsQueueErrorComputed", "Network", &NetworkSettings::isQueueErrorComputed);

	static Reflection::BoundProp<bool> prop_TrackDataTypes("TrackDataTypes", "Diagnostics", &NetworkSettings::trackDataTypes);
    static Reflection::BoundProp<bool> prop_TrackPhysicsDetails("TrackPhysicsDetails", "Diagnostics", &NetworkSettings::trackPhysicsDetails);
	static Reflection::BoundProp<bool> prop_PrintSplitMessages("PrintSplitMessage", "Diagnostics", &NetworkSettings::printSplitMessages);
	static Reflection::BoundProp<bool> prop_PrintTouches("PrintTouches", "Diagnostics", &NetworkSettings::printTouches);
	static Reflection::BoundProp<bool> prop_PrintInstances("PrintInstances", "Diagnostics", &NetworkSettings::printInstances);
    static Reflection::BoundProp<bool> prop_PrintStreamInstanceQuota("PrintStreamInstanceQuota", "Diagnostics", &NetworkSettings::printStreamInstanceQuota);
	static Reflection::BoundProp<bool> prop_PrintProperties("PrintProperties", "Diagnostics", &NetworkSettings::printProperties);
	static Reflection::BoundProp<bool> prop_PrintPhysicsErrors("PrintPhysicsErrors", "Diagnostics", &NetworkSettings::printPhysicsErrors);
	static Reflection::BoundProp<bool> prop_PrintDataFilters("PrintFilters", "Diagnostics", &NetworkSettings::printDataFilters);
	static Reflection::BoundProp<bool> prop_ArePhysicsRejectionsReported("ArePhysicsRejectionsReported", "Diagnostics", &NetworkSettings::printPhysicsFilters);
	static Reflection::BoundProp<bool> propPrintEvents("PrintEvents", "Diagnostics", &NetworkSettings::printEvents);
	static Reflection::BoundProp<bool> propPrintBits("PrintBits", "Diagnostics", &NetworkSettings::printBits);
	static Reflection::BoundProp<double> prop_incommingReplicationLag("IncommingReplicationLag", "Diagnostics", &NetworkSettings::incommingReplicationLag);

	static Reflection::EnumPropDescriptor<NetworkSettings, NetworkSettings::PhysicsSendMethod> prop_PhysicsSend("PhysicsSend", "Physics", &NetworkSettings::getPhysicsSendMethod, &NetworkSettings::setPhysicsSendMethod);
	static Reflection::EnumPropDescriptor<NetworkSettings, NetworkSettings::PhysicsReceiveMethod> prop_PhysicsReceive("PhysicsReceive", "Physics", &NetworkSettings::dummyGetPhysicsReceiveMethod, &NetworkSettings::dummySetPhysicsReceiveMethod);
	static Reflection::EnumPropDescriptor<NetworkSettings, PacketPriority> prop_PhysicsSendPriority("PhysicsSendPriority", "Physics", &NetworkSettings::getPhysicsSendPriority, &NetworkSettings::setPhysicsSendPriority, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);

	Reflection::BoundProp<bool> NetworkSettings::prop_DistributedPhysics("ExperimentalPhysicsEnabled", "Physics", &NetworkSettings::distributedPhysicsEnabled);
	
	static Reflection::PropDescriptor<NetworkSettings, int> prop_PhysicsMtuAdjust("PhysicsMtuAdjust", "Physics", &NetworkSettings::getPhysicsMtuAdjust, &NetworkSettings::setPhysicsMtuAdjust);
	static Reflection::PropDescriptor<NetworkSettings, int> prop_ReplicationMtuAdjust("DataMtuAdjust", "Data", &NetworkSettings::getReplicationMtuAdjust, &NetworkSettings::setReplicationMtuAdjust);
	static Reflection::BoundProp<int> propCanSendPacketBufferLimit("CanSendPacketBufferLimit", "Data", &NetworkSettings::canSendPacketBufferLimit);
	static Reflection::BoundProp<int> propSendPacketBufferLimit("SendPacketBufferLimit", "Data", &NetworkSettings::sendPacketBufferLimit);
	static Reflection::BoundProp<int> propMaxDataModelSendBuffer("MaxDataModelSendBuffer", "Data", &NetworkSettings::canSendPacketBufferLimit, Reflection::PropertyDescriptor::Attributes::deprecated(propCanSendPacketBufferLimit));
	static Reflection::EnumPropDescriptor<NetworkSettings, PacketPriority> prop_DataSendPriority("DataSendPriority", "Data", &NetworkSettings::getDataSendPriority, &NetworkSettings::setDataSendPriority, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);

#ifdef NETWORK_PROFILER
	static Reflection::BoundProp<bool> prop_Profiling("Profiling", "Profiler", &NetworkSettings::profiling, Reflection::PropertyDescriptor::UI);
    static Reflection::BoundProp<bool> prop_ProfilingCpu("ProfileCpu", "Profiler", &NetworkSettings::profilecpu);
	static Reflection::BoundProp<bool> prop_ProfilerOneColumnPerStack("ProfilerOneColumnPerStack", "Profiler", &NetworkSettings::profilerOneColumnPerStack, Reflection::PropertyDescriptor::UI);
	static Reflection::BoundProp<std::string> prop_ProfilerServerIp("ProfilerServerIp", "Profiler", &NetworkSettings::profilerServerIp);
	static Reflection::BoundProp<int> prop_ProfilerServerPort("ProfilerServerPort", "Profiler", &NetworkSettings::profilerServerPort);
	static Reflection::BoundProp<std::string> prop_ProfilerTag("ProfilerTag", "Profiler", &NetworkSettings::profilerTag, Reflection::PropertyDescriptor::UI);
	static Reflection::BoundProp<double> prop_ProfilerTimedSeconds("ProfilerTimedSeconds", "Profiler", &NetworkSettings::profilerTimedSeconds);
    static Reflection::BoundFuncDesc<NetworkSettings, void()> func_printProfilingResult(&NetworkSettings::printProfilingResult, "PrintProfilingResult", Security::None);
#endif

#ifdef NETWORK_PROFILER
	static Reflection::BoundProp<bool> prop_EnableHeavyCompression("EnableHeavyCompression", "Optimization", &NetworkSettings::enableHeavyCompression, Reflection::PropertyDescriptor::UI);
#else
    static Reflection::BoundProp<bool> prop_EnableHeavyCompression("EnableHeavyCompression", "Optimization", &NetworkSettings::enableHeavyCompression, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING);
#endif

	static Reflection::PropDescriptor<NetworkSettings, int> prop_extraMemoryUsed("ExtraMemoryUsed", category_Data, &NetworkSettings::getExtraMemoryUsedInMB, &NetworkSettings::setExtraMemoryUsedInMB, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::Plugin);
	static Reflection::PropDescriptor<NetworkSettings, float> prop_freeMemoryMBytes("FreeMemoryMBytes", category_Data, &NetworkSettings::getFreeMemoryMBytes, NULL, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::Plugin);
    static Reflection::PropDescriptor<NetworkSettings, float> prop_freeMemoryPoolMBytes("FreeMemoryPoolMBytes", category_Data, &NetworkSettings::getFreeMemoryPoolMBytes, NULL, Reflection::PropertyDescriptor::HIDDEN_SCRIPTING, Security::Plugin);
	static Reflection::PropDescriptor<NetworkSettings, bool> prop_RenderRegions("RenderStreamedRegions", category_Appearance, &NetworkSettings::getRenderStreamedRegions, &NetworkSettings::setRenderStreamedRegions);
    static Reflection::PropDescriptor<NetworkSettings, bool> prop_ShowPartMovementPath("ShowPartMovementWayPoint", category_Appearance, &NetworkSettings::getShowPartMovementPath, &NetworkSettings::setShowPartMovementPath);
    static Reflection::PropDescriptor<NetworkSettings, int> prop_TotalNumMovementWayPoint("TotalNumMovementWayPoint", category_Appearance, &NetworkSettings::getTotalNumMovementWayPoint, &NetworkSettings::setTotalNumMovementWayPoint);
#ifdef RBX_TEST_BUILD
    static Reflection::PropDescriptor<NetworkSettings, int> prop_TaskSchedulerFindJobFPS("TaskSchedulerFindJobFPS", category_Appearance, &NetworkSettings::getTaskSchedulerFindJobFPS, &NetworkSettings::setTaskSchedulerFindJobFPS);
    static Reflection::PropDescriptor<NetworkSettings, bool> prop_TaskSchedulerUpdateJobPriorityOnWake("TaskSchedulerUpdateJobPriorityOnWake", category_Appearance, &NetworkSettings::getTaskSchedulerUpdateJobPriorityOnWake, &NetworkSettings::setTaskSchedulerUpdateJobPriorityOnWake);
#endif
    static Reflection::PropDescriptor<NetworkSettings, bool> prop_ShowActiveAnimationAsset("ShowActiveAnimationAsset", category_Appearance, &NetworkSettings::getShowActiveAnimationAsset, &NetworkSettings::setShowActiveAnimationAsset);
    REFLECTION_END();
    
	NetworkSettings::NetworkSettings(void)
		:printInstances(false)
        ,printStreamInstanceQuota(false)
		,printTouches(false)
		,distributedPhysicsEnabled(true)
		,printPhysicsErrors(false)
		,printPhysicsFilters(false)
		,printProperties(false)
		,printEvents(false)
		,printDataFilters(false)
		,printBits(false)
		,incommingReplicationLag(0.0)
		,isQueueErrorComputed(true)
		,usePhysicsPacketCache(false)
		,useInstancePacketCache(false)
		,canSendPacketBufferLimit(1)
		,sendPacketBufferLimit(-1)  // default is to not check it
		,physicsMtuAdjust(-200)
		,preferredClientPort(0)
		,replicationMtuAdjust(-200)
		,dataSendRate(30.0f)
		,physicsSendRate(20.0f)
        ,clientPhysicsSendRate(20.0f)
		,dataGCRate(20.0f)
		,networkOwnerRate(10.0f)
		,touchSendRate(10.0f)
		,isThrottledByOutgoingBandwidthLimit(false)
		,isThrottledByCongestionControl(false)
		,receiveRate(60.0f)
		,physicsSendMethod(TopNErrors) // default to TopNErrors as unit tests will be using the default value, if we are going to use other types of senders please make coordinating changes in PhysicsReceiver as it is expecting packets from either RoundRobin sender or TopNError sender
		,physicsSendPriority(HIGH_PRIORITY)
		,dataSendPriority(MEDIUM_PRIORITY)
		,physicsReceiveMethod(Interpolation)
		,printSplitMessages(false)
		,trackDataTypes(false)
        ,trackPhysicsDetails(false)
		,profiling(false)
        ,profilecpu(false)
		,profilerOneColumnPerStack(false)
		,profilerTag("")
		,profilerServerIp("127.0.0.1")
		,profilerServerPort(38123)
		,profilerTimedSeconds(0.f)
		,enableHeavyCompression(true)
		,extraMemoryUsed(0)
        ,totalNumMovementWayPoint(1000)
	{
		setName("Network");
	}

	template<typename A>
	static A clamp(const A& min, const A& value, const A& max)
	{
		return std::max(min, std::min(value, max));
	}

	void NetworkSettings::setPhysicsMtuAdjust(int value)
	{
		value = clamp(-1000,value,0);
		if(value != physicsMtuAdjust){
			physicsMtuAdjust = value;
			raisePropertyChanged(prop_PhysicsMtuAdjust);
		}
	}
	void NetworkSettings::setReplicationMtuAdjust(int value)
	{
		value = clamp(-1000,value,0);
		if(value != replicationMtuAdjust){
			replicationMtuAdjust = value;
			raisePropertyChanged(prop_ReplicationMtuAdjust);
		}
	}
	void NetworkSettings::setDataSendRate(float value)
	{
		value = clamp(5.0f, value, 120.0f);
		if(value != dataSendRate){
			dataSendRate = value;
			raisePropertyChanged(prop_DataSendRate);
		}
	}
	void NetworkSettings::setDataGCRate(float value)
	{
		value = clamp(2.0f, value, 60.0f);
		if(value != dataGCRate){
			dataGCRate = value;
			raisePropertyChanged(prop_DataGCRate);
		}
	}
	void NetworkSettings::setPhysicsSendRate(float value)
	{
		value = clamp(0.1f, value, 120.0f);
		if(value != physicsSendRate){
			physicsSendRate = value;
			raisePropertyChanged(prop_PhysicsSendRate);
		}
	}

    void NetworkSettings::setClientPhysicsSendRate(float value)
    {
        value = clamp(0.1f, value, 120.0f);
        if(value != clientPhysicsSendRate){
            clientPhysicsSendRate = value;
            raisePropertyChanged(prop_ClientPhysicsSendRate);
        }
    }

	void NetworkSettings::setTouchSendRate(float value)
	{
		value = clamp(1.0f, value, 120.0f);
		if(value != touchSendRate){
			touchSendRate = value;
			raisePropertyChanged(prop_TouchSendRate);
		}
	}

	void NetworkSettings::setReceiveRate(double value)
	{
		value = clamp(5.0,value,120.0);
		if(value != receiveRate){
			receiveRate = value;
			raisePropertyChanged(prop_ReceiveRate);
		}
	}

	void NetworkSettings::setPhysicsSendMethod(const PhysicsSendMethod& value)
	{
		if (value != physicsSendMethod)
		{
			physicsSendMethod = value;
			raiseChanged(prop_PhysicsSend);
		}
	}

	void NetworkSettings::setPhysicsSendPriority(const PacketPriority& value)
	{
		if (value != physicsSendPriority)
		{
			physicsSendPriority = value;
			raiseChanged(prop_PhysicsSendPriority);
		}
	}

	void NetworkSettings::setDataSendPriority(const PacketPriority& value)
	{
		if (value != dataSendPriority)
		{
			dataSendPriority = value;
			raiseChanged(prop_DataSendPriority);
		}
	}

	void NetworkSettings::dummySetPhysicsReceiveMethod(const PhysicsReceiveMethod& value)
	{
		if (value != physicsReceiveMethod)
		{
			physicsReceiveMethod = value;
			raiseChanged(prop_PhysicsReceive);
		}
	}

    bool NetworkSettings::heavyCompressionEnabled()
    {
        return enableHeavyCompression;
    }

	void NetworkSettings::setExtraMemoryUsedInMB(int value)
	{
		if (value < 0)
			value = 0;

		if (value != extraMemoryUsed)
		{
			extraMemoryUsed = value;
			raiseChanged(prop_extraMemoryUsed);
		}
	}

	float NetworkSettings::getFreeMemoryMBytes() const 
	{
		return MemoryStats::freeMemoryBytes() / 1024.0f / 1024.0f; 
	}

    float NetworkSettings::getFreeMemoryPoolMBytes() const
    {
        return MemoryStats::slowGetMemoryPoolAvailability() / 1024.0f / 1024.0f;
    }

	void NetworkSettings::setRenderStreamedRegions(bool value)
	{
		if (value != Workspace::showStreamedRegions)
		{
			Workspace::showStreamedRegions = value;
			raiseChanged(prop_RenderRegions);
		}
	}

	bool NetworkSettings::getRenderStreamedRegions() const 
	{ 
		return Workspace::showStreamedRegions; 
	}

    void NetworkSettings::setShowPartMovementPath(bool value)
    {
        if (value != Workspace::showPartMovementPath)
        {
            Workspace::showPartMovementPath = value;
         }
    }

    bool NetworkSettings::getShowPartMovementPath() const
    {
        return Workspace::showPartMovementPath;
    }

    void NetworkSettings::setTotalNumMovementWayPoint(int value)
    {
        if (value < 0)
            value = 0;
        if (value > 1000000)
            value = 1000000;

        if (value != totalNumMovementWayPoint)
        {
            totalNumMovementWayPoint = value;
        }
    }

    bool NetworkSettings::getShowActiveAnimationAsset() const
    {
        return Workspace::showActiveAnimationAsset;
    }

    void NetworkSettings::setShowActiveAnimationAsset(bool value)
    {
        if (value != Workspace::showActiveAnimationAsset)
        {
            Workspace::showActiveAnimationAsset = value;
        }
    }

    void NetworkSettings::printProfilingResult()
    {
        CPUPROFILER_OUTPUT();
    }
}

