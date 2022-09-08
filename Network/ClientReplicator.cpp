/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#include "ClientReplicator.h"
#include "Client.h"
#include "Util.h"
#include "Marker.h"

#include "network/Players.h"
#include "ConcurrentRakPeer.h"
#include "FastLog.h"
#include "Util/ProgramMemoryChecker.h"
#include "V8DataModel/HackDefines.h"
#include "Util/ScopedAssign.h"
#include "V8DataModel/HackDefines.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/JointsService.h"
#include "V8DataModel/TimerService.h"
#include "V8DataModel/message.h"
#include "V8World/Joint.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/PlayerGui.h"
#include "script/ScriptContext.h"
#include "RoundRobinPhysicsSender.h"
#include "DirectPhysicsReceiver.h"


// TODO - eliminate this - better place for distributed physics switch
#include "V8DataModel/Workspace.h"
#include "v8datamodel/DataModel.h"
#include "V8DataModel/PartInstance.h"
#include "v8datamodel/TeleportService.h"
#include "v8datamodel/ReplicatedFirst.h"
#include "V8World/Assembly.h"
#include "Replicator.StatsItem.h"
#include "Replicator.GCJob.h"
#include "NetworkProfiler.h"
#include "NetworkFilter.h"
#include "humanoid/Humanoid.h"

// For security
#include "humanoid/HumanoidState.h"
#include "humanoid/FallingDown.h"

// TODO remove this
#include "v8world/World.h"

#include <boost/scoped_ptr.hpp>
#include "VMProtect/VMProtectSDK.h"
#include "Replicator.StreamJob.h"
#include "Replicator.HashItem.h"
#include "Replicator.TagItem.h"
#include "Replicator.ChangePropertyItem.h"
#include "Replicator.RockyItem.h"
#include "util/PhysicalProperties.h"
#include "util/ProtectedString.h"
#include "util/UDim.h"
#include "util/Axes.h"

#include "rbx/Profiler.h"

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
#include "util/CheatEngine.h"
#include "security/ApiSecurity.h"
#endif

FASTFLAG(DebugLocalRccServerConnection)
DYNAMIC_LOGVARIABLE(PartStreamingRequests, 0)
DYNAMIC_FASTINTVARIABLE(ClientInstanceQuotaCap, 10000)
DYNAMIC_FASTINTVARIABLE(ClientInstanceQuotaInitial, 2000)
FASTFLAG(DebugProtocolSynchronization)
FASTINT(StreamingCriticalLowMemWatermarkMB)
FASTFLAG(RemoveUnusedPhysicsSenders)

FASTFLAG(ClientABTestingEnabled)
FASTFLAGVARIABLE(CopyArrayReferences, true)

FASTFLAG(FilterSinglePass)

namespace{
    static RBX::Network::MccReport mccReport = {};
}

namespace RBX { namespace Network {

	class ClientReplicator::ClientStatsItem : public Replicator::Stats
	{
		Item* itemCount;
		Item* pendingInstanceRequests;
		Stats::Item* streamPacketsReceived;
		Stats::Item* streamReceiverAvgTimePerPacket;

		Item* numRegionsToGC;
		Item* gcDistance;
		Item* numStreamedRegions;

	public:
		ClientStatsItem(const shared_ptr<const ClientReplicator>& replicator)
			:Replicator::Stats(replicator)
		{
			Item* item = createChildItem("PropSync");
			itemCount = item->createChildItem("ItemCount");
			item->createBoundChildItem("AckCount", replicator->propSync.ackCount);

			Item* receivedStreamData = createChildItem("Received Stream Data");
			receivedStreamData->createBoundChildItem("Avg Read Time per item", replicator->avgStreamDataReadTime);
			receivedStreamData->createBoundChildItem("Avg Instances per item", replicator->avgInstancesPerStreamData);
			receivedStreamData->createBoundChildItem("Avg Request count", replicator->avgRequestCount);
			pendingInstanceRequests = receivedStreamData->createChildItem("Pending Request count");
			numRegionsToGC = receivedStreamData->createChildItem("Num Regions To GC");
			gcDistance = receivedStreamData->createChildItem("GC Distance");
			numStreamedRegions = receivedStreamData->createChildItem("Num Regions");
		}

		/*override*/ void update() 
		{
			Replicator::Stats::update();

			if(shared_ptr<const ClientReplicator> locked = shared_static_cast<const ClientReplicator>(replicator.lock()))
			{
				pendingInstanceRequests->formatValue(locked->pendingInstanceRequests - locked->numInstancesRead);
				itemCount->formatValue(locked->propSync.itemCount());

				numRegionsToGC->formatValue(locked->getNumRegionsToGC());
				gcDistance->formatValue((int)locked->getGCDistance());
				numStreamedRegions->formatValue(locked->getNumStreamedRegions());
			}
		}
	};

#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD) && !defined(RBX_PLATFORM_DURANGO)
    // Periodically check that the program memory hashing job is still running
    class ClientReplicator::BadAppCheckerJob : public DataModelJob {
        shared_ptr<ClientReplicator> clientReplicator;
        Time lastRunTime;
        HwndScanner hwndScanner;
        FileScanner fileScanner;
        DbvmCanary dbvmScanner;
        SpeedhackDetect speedhackScanner;

        unsigned char scanCounter;
        static const unsigned int kNumScans = 7;
        static const unsigned int kCeHwndScanMask   = 1<<0;
        static const unsigned int kCeTitleScanMask  = 1<<1;
        static const unsigned int kCeAttachScanMask = 1<<2;
        static const unsigned int kCeLogScanMask    = 1<<3;
        static const unsigned int kCeDbvmGtx        = 1<<4;
        static const unsigned int kCeDbvmStx        = 1<<5;
        static const unsigned int kCeDll            = 1<<6;
        static const unsigned int kCeAllScansMask   = (1<<kNumScans) - 1;

        void doScans(unsigned int scanMask)
        {
#if defined(_WIN32) && !defined(_NOOPT) && !defined(_DEBUG)
            VMProtectBeginMutation(NULL);
            bool isCeDetectedLocal = false;
            DataModel* dataModel = DataModel::get(clientReplicator.get());
            if (scanMask & kCeDll)
            {
                isCeDetectedLocal = isCeDetectedLocal || (isCeBadDll());
            }
            if (scanMask & kCeDbvmGtx)
            {
                dbvmScanner.checkAndLocalUpdate();
            }
            if (scanMask & kCeDbvmStx)
            {
                dbvmScanner.kernelUpdate();
            }
            if (ceHwndChecks && (kCeHwndScanMask & scanMask))
            {
                hwndScanner.scan();
            }
            isCeDetectedLocal = isCeDetectedLocal || (ceHwndChecks 
                && (scanMask & kCeTitleScanMask) && hwndScanner.detectTitle());
            isCeDetectedLocal = isCeDetectedLocal || (ceHwndChecks
                && (scanMask & kCeAttachScanMask) && hwndScanner.detectFakeAttach());
            isCeDetectedLocal = isCeDetectedLocal || (ceHwndChecks
                && (scanMask * kCeAttachScanMask) && hwndScanner.detectEarlyKick());
#if 0
            // This code results in false positives, but it isn't clear why.
            // This should be debugged.
            if (speedhackScanner.isSpeedhack())
            {
                RBX::Tokens::simpleToken |= HATE_SPEEDHACK;
            }
#endif
            VMProtectEnd();
            VMProtectBeginVirtualization(NULL);
            if(ceDetected || isCeDetectedLocal)
            {
                dataModel->addHackFlag(HATE_CHEATENGINE_OLD);
                RBX::Tokens::sendStatsToken.addFlagSafe(HATE_CHEATENGINE_OLD);
            }
            VMProtectEnd();
            #endif
        }

    public:
        BadAppCheckerJob(shared_ptr<ClientReplicator> clientReplicator) :
            DataModelJob("BatteryProfiler", DataModelJob::None, false,
            shared_from(DataModel::get(clientReplicator.get())), Time::Interval(0)),
            clientReplicator(clientReplicator),
            lastRunTime(Time::nowFast()),
            scanCounter(0) {}

        virtual Time::Interval sleepTime(const Stats& stats)
        {
            return computeStandardSleepTime(stats, 1); 
        }

        virtual Error error(const Stats& stats)
        {
            return computeStandardErrorCyclicExecutiveSleeping(stats, 1); 
        }

        virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) 
        { 
            VMProtectBeginMutation("27");
            doScans(1<<scanCounter);
            lastRunTime = Time::nowFast();
            scanCounter = (scanCounter + 1) % kNumScans;
            VMProtectEnd();
            return TaskScheduler::Stepped;
        }

        Time getLastRunTime() const
        {
            return lastRunTime;
        }

    };
#endif // #ifdef _WIN32



#if !defined(RBX_STUDIO_BUILD) && !defined(RBX_PLATFORM_DURANGO)
	// Periodically hash program memory, and raise an alert if the hash changes
	class ClientReplicator::MemoryCheckerJob : public DataModelJob {

		boost::shared_ptr<ClientReplicator> clientReplicator;
		boost::scoped_ptr<ProgramMemoryChecker> memoryChecker;
		unsigned int firstHash;
		Time lastRunTime;
        Time lastHashTime;
#ifdef _WIN32
        NtApiCaller ntApi;
#endif
        Humanoid hsceFakeHumanoid;
        shared_ptr<HUMAN::Dead> hsceFakeHumanoidState;
        static const unsigned int kTimeToCompleteHash = 3;
        static const unsigned int kStepFrequency = ProgramMemoryChecker::kSteps/kTimeToCompleteHash;

	public:
        rbx::signal<void()> hashReadySignal;

		MemoryCheckerJob(shared_ptr<ClientReplicator> clientReplicator) :
			DataModelJob("US14116", DataModelJob::None, false,
				shared_from(DataModel::get(clientReplicator.get())), Time::Interval(0)),
			clientReplicator(clientReplicator),
			lastRunTime(Time::nowFast()),
            lastHashTime(Time::nowFast()) 
        {
            memoryChecker.reset(new ProgramMemoryChecker());
            // after memory checker is confirmed to work, make this into a const again.
            firstHash = memoryChecker->getLastCompletedHash();
            hsceFakeHumanoidState.reset(new HUMAN::Dead(&hsceFakeHumanoid, HUMAN::DEAD));
        }

		virtual Time::Interval sleepTime(const Stats& stats)
		{
			return computeStandardSleepTime(stats, kStepFrequency); 
		}

		virtual Error error(const Stats& stats)
		{
			return computeStandardError(stats, kStepFrequency); 
		}

		virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) 
		{
			VMProtectBeginMutation(NULL);
            bool isHsceHashFail = false;
            bool isLuaLockFail = false;
            bool isHsceUnitFail = false;
            unsigned int codeChanged = 0;
            unsigned int goldCodeChanged = 0;
            DataModel* thisModel = DataModel::get(clientReplicator.get());

#ifdef _WIN32
            // debugger check
	        CONTEXT context;
	        context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
            HANDLE thisThread = reinterpret_cast<HANDLE>(~(size_t)(1));
	        if (ntApi.getThreadContext(thisThread, &context) && (context.Dr7 & 0xff) != 0)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag9, HATE_DEBUGGER);
				return TaskScheduler::Stepped;
			}
#endif
			unsigned int sendSignal = memoryChecker->step();
            // firstHash, initialProgramHash, and lastCompletedHash should all be equal
            int diff = (firstHash != initialProgramHash)
                    | (memoryChecker->getLastCompletedHash() != initialProgramHash);
			codeChanged = static_cast<unsigned int>(diff | -diff) >> 31;
#ifdef _WIN32
            unsigned int thisHsceHash = memoryChecker->updateHsceHash();
            isHsceHashFail = ((thisHsceHash != memoryChecker->getHsceAndHash()) || thisHsceHash != memoryChecker->getHsceOrHash());
#else
            const unsigned int mask = codeChanged * HATE_OSX_MEMORY_HASH_CHANGED;
            thisModel->addHackFlag(mask);
#endif
            
            #if !defined(_DEBUG) && !defined(_NOOPT) && defined(_WIN32)
            // The golden hash is a char* to make it identifable to be patched in the exe
            // Underlying data is actually an unsigned int.
            int goldDiff = (memoryChecker->getLastGoldenHash() != RBX::Security::rbxGoldHash);
            goldCodeChanged = static_cast<unsigned int>(goldDiff | -goldDiff) >> 31;
            #endif

            lastHashTime = memoryChecker->getLastCompletedTime();
            lastRunTime = Time::nowFast();

            if (sendSignal == ProgramMemoryChecker::kAllDone)
            {
                hashReadySignal();
#if defined(_WIN32) && !defined(RBX_PLATFORM_DURNAGO)
                isLuaLockFail = ((memoryChecker->isLuaLockOk() != ProgramMemoryChecker::kLuaLockOk));

                MEMORY_BASIC_INFORMATION trapInfo;
                ntApi.virtualQuery(&RBX::writecopyTrap, &trapInfo, sizeof(trapInfo));
                if (trapInfo.Protect != PAGE_WRITECOPY)
                {
                    // report
                    RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag3, HATE_CE_ASM);
                }
#endif
            }

            // this is to detect DBVM's changes to this part of the program.
            isHsceUnitFail = (hsceFakeHumanoidState->checkComputeEvent() != HUMAN::HumanoidState::kCorrectCheckValue);

			VMProtectEnd();
            VMProtectBeginVirtualization(NULL);
            if (codeChanged)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag1, HATE_MEMORY_HASH_CHANGED);
            }
            if (isHsceHashFail)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag2, HATE_HSCE_HASH_CHANGED);
            }
            if (goldCodeChanged)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag3, HATE_MEMORY_HASH_CHANGED);
            }
            if (isLuaLockFail)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag4, HATE_DLL_INJECTION);
            }
            if (isHsceUnitFail)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag5, HATE_HSCE_HASH_CHANGED);
            }
			VMProtectEnd();
			return TaskScheduler::Stepped;
		}

		Time getLastRunTime() const {
			return lastRunTime;
		}

        Time getLastHashTime() const {
            return lastHashTime;
        }
	};
#endif

#if defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO) && !defined(RBX_STUDIO_BUILD)
	// Periodically check that the program memory hashing job is still running
	class ClientReplicator::MemoryCheckerCheckerJob : public DataModelJob {

		shared_ptr<ClientReplicator> clientReplicator;
		Time lastKnownValidXXHashRun;
        Time lastCheckTime;
        int runSlips;
        int hashSlips;
        NtApiCaller ntApi;
        int runCount;
        unsigned int pageSize;
        unsigned int encodedReport;

        __forceinline bool memPermissionChanged(uintptr_t regionBase, size_t regionSize, size_t protection)
        {
            MEMORY_BASIC_INFORMATION memInfo;
            if (0 == ntApi.virtualQuery(((void*)(regionBase)), &memInfo, sizeof(MEMORY_BASIC_INFORMATION))) {
                unsigned int sizeDiff = memInfo.RegionSize - regionSize; // makes use of the overflow for the "too small" case.
                if ((sizeDiff > pageSize) || (memInfo.Protect != protection))
                {
                    memset(&memInfo, 0, sizeof(memInfo));
                    return true;
                }
            }
            memset(&memInfo, 0, sizeof(memInfo));
            return false;
        }

        __forceinline void populateMccReport(RBX::Network::MccReport& report)
        {
            report.localChecksEncoded = encodedReport;
            report.memcheckRunTime = static_cast<unsigned int>(clientReplicator->memoryCheckerJob->getLastRunTime().timestampSeconds());
            report.memcheckDoneTime = static_cast<unsigned int>(clientReplicator->memoryCheckerJob->getLastHashTime().timestampSeconds());
            report.badAppRunTime = static_cast<unsigned int>(clientReplicator->badAppCheckerJob->getLastRunTime().timestampSeconds());
            report.mccRunTime = static_cast<unsigned int>(Time::nowFast().timestampSeconds());
        }

	public:
        rbx::signal<void()> reportReadySignal;

		MemoryCheckerCheckerJob(shared_ptr<ClientReplicator> clientReplicator) :
			DataModelJob("US14116_pt2", DataModelJob::None, false,
				shared_from(DataModel::get(clientReplicator.get())), Time::Interval(0)),
			clientReplicator(clientReplicator),
            runSlips(0),
            hashSlips(0),
			lastKnownValidXXHashRun(Time::nowFast()),
            lastCheckTime(Time::nowFast()),
            runCount(0),
            encodedReport(kGf2EncodeLut[MCC_INIT_IDX])
        {
            SYSTEM_INFO info;
            GetSystemInfo(&info);
            pageSize = info.dwPageSize;
        }
		
		virtual Time::Interval sleepTime(const Stats& stats)
		{
			return computeStandardSleepTime(stats, 1); 
		}

		virtual Error error(const Stats& stats)
		{
			return computeStandardErrorCyclicExecutiveSleeping(stats, 1); 
		}


		virtual TaskScheduler::StepResult stepDataModelJob(const Stats& stats) 
		{
			VMProtectBeginMutation(NULL);
            bool isBadTextSection = false;
            bool isBadVmpSection = false;
            bool isBadRdataSection = false;
            bool isHwbpSet = false;
            bool isGtxHook = false;
            bool isVehUnhook = false;
            bool isFreeConsoleHooked = false;
            ++runCount;
#if (!defined(NOOPT) && !defined(DEBUG)) && !defined(RBX_PLATFORM_DURANGO)
            // a switch statement might result in a jump table, which is not compatible with VMProtect.
            if (runCount % 8 == 0)
            {
                isBadTextSection =(memPermissionChanged(RBX::Security::rbxTextBase, RBX::Security::rbxTextSize, PAGE_EXECUTE_READ));
            }
            else if (runCount % 8 == 2)
            {
                isBadVmpSection = (memPermissionChanged(RBX::Security::rbxVmpBase, RBX::Security::rbxVmpSize, PAGE_EXECUTE_READ));
            }
            else if (runCount % 8 == 4)
            {
                isBadRdataSection = (memPermissionChanged(RBX::Security::rbxRdataBase, RBX::Security::rbxRdataSize, PAGE_READONLY));
            }
            else if (runCount % 8 == 6)
            {
                // Debugger Check
                CONTEXT ctx;
                ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                HANDLE thisThread = reinterpret_cast<HANDLE>(~(size_t)(1));
                isHwbpSet = (ntApi.getThreadContext(thisThread, &ctx) && ((ctx.Dr7 & 0xFF) != 0));
            }
#endif
            const unsigned short kHotPatchProlog = 0xFF8B;
            isGtxHook = (*((unsigned short*)(&::GetThreadContext)) != kHotPatchProlog);        // check for early hook
            if (vehHookLocationHv)
            {
                uintptr_t hookLoc = vehHookLocationHv;
                uintptr_t relJump = *reinterpret_cast<uintptr_t*>(hookLoc);
                uintptr_t jumpTarget = vehStubLocationHv;
                isVehUnhook = ((hookLoc + sizeof(void*) + relJump) != jumpTarget)
                    || (!ntApi.isNtdllAddress(reinterpret_cast<uintptr_t>(vehHookContinue))); // check for basic hook
            }
            if (runCount % 8 == 7)
            {
                populateMccReport(::mccReport);
                reportReadySignal();

                // this is just to troll exploit developers who are too lazy to write a GUI.
                if (FFlag::CopyArrayReferences)
                {
                    FreeConsole();
                }
                if (*reinterpret_cast<uint16_t*>(&FreeConsole) != 0xFF8B)
                {
                    isFreeConsoleHooked =  true;
                }
            }
			VMProtectEnd();
            VMProtectBeginVirtualization(NULL);
            if (isBadTextSection)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag4, HATE_NEW_AV_CHECK);
                encodedReport ^= kGf2EncodeLut[MCC_TEXT_IDX];
            }
            if (isBadVmpSection)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag5, HATE_NEW_AV_CHECK);
                encodedReport ^= kGf2EncodeLut[MCC_VMP_IDX];
            }
            if (isBadRdataSection)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag6, HATE_NEW_AV_CHECK);
                encodedReport ^= kGf2EncodeLut[MCC_RDATA_IDX];
            }
            else if (runCount % 8 == 5)
            {
                encodedReport ^= kGf2EncodeLut[MCC_NULL0_IDX];
            }
            else
            {
                encodedReport ^= kGf2EncodeLut[MCC_NULL1_IDX];
            }
            if (isHwbpSet)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag3, HATE_NEW_HWBP);
                encodedReport ^= kGf2EncodeLut[MCC_HWBP_IDX];
            }
            if (isGtxHook)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag11, HATE_HOOKED_GTX); // change this later
                encodedReport ^=  kGf2EncodeLut[MCC_GTX_IDX];
            }
            if (isVehUnhook)
            {
                RBX::Security::setHackFlagVmp<LINE_RAND4>(RBX::Security::hackFlag1, HATE_UNHOOKED_VEH); // change this later
                encodedReport ^= kGf2EncodeLut[MCC_VEH_IDX];
            }
            if (isFreeConsoleHooked)
            {
                encodedReport ^= kGf2EncodeLut[MCC_FREECONSOLE_IDX];
            }
            if ( (runCount % 8 == 6) && !FFlag::FilterSinglePass)
            {
                encodedReport ^= kGf2EncodeLut[MCC_FAKE_FFLAG_IDX];
            }
			VMProtectEnd();
			return TaskScheduler::Stepped;
        }
	};
#endif
}}

using namespace RBX;
using namespace RBX::Network;

const char* const RBX::Network::sClientReplicator = "ClientReplicator";

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<ClientReplicator, void(bool)> func_requestServerStats(&ClientReplicator::requestServerStats, "RequestServerStats", "request", Security::RobloxScript);
static Reflection::EventDesc<ClientReplicator, void(shared_ptr<const Reflection::ValueTable>)> event_StatsReceived(&ClientReplicator::statsReceivedSignal, "StatsReceived", "stats", Security::RobloxScript);
REFLECTION_END();

shared_ptr<Replicator::Stats> ClientReplicator::createStatsItem()
{
	return Creatable<Instance>::create<ClientStatsItem>(shared_from(this));
};

class CFrameAcknowledgementItem : public PooledItem
{
	ClientReplicator* client;
	const shared_ptr<const PartInstance> instance;
public:
	CFrameAcknowledgementItem(ClientReplicator* client, shared_ptr<const PartInstance> instance):PooledItem(*client),instance(instance),client(client)
	{}

	/*implement*/ bool write(RakNet::BitStream& bitStream) 
	{
		client->writePropAcknowledgementIfNeeded(instance.get(), PartInstance::prop_CFrame, bitStream);
		return true;
	}
};

ClientReplicator::ClientReplicator(RakNet::SystemAddress systemAddress, Client* client, RakNet::SystemAddress clientAddress, NetworkSettings* networkSettings)
: Super( systemAddress, client->rakPeer, networkSettings, /*ClusterDebounce*/true)
, clientAddress(clientAddress)
, receivedGlobals(false)
, numInstancesRead(0)
, pendingInstanceRequests(0)
, loggedLowMemWarning(false)
, avgInstancesPerStreamData(0.05, 25) // estimated default value 25
, avgStreamDataReadTime(0.1, 0.05)
, avgRequestCount(0.1)
, clientInstanceQuota(0)
, sampleTimer(-1.0f) // guarantee fire on first call
, memoryLevel(RBX::MemoryStats::MEMORYLEVEL_OK)
{
	setName("ClientReplicator");
	cframePool.reset(new AutoMemPool(sizeof(CFrameAcknowledgementItem)));

	// overwrite replicator default, at start up we want to process all the packets from server asap to allow faster join
	processAllPacketsPerStep = true;

	canTimeout = false;
}

ClientReplicator::~ClientReplicator()
{
    sendDisconnectionSignal("", false);
}

Player* ClientReplicator::findTargetPlayer() const
{
	return players ? players->getLocalPlayer() : 0;
}

bool ClientReplicator::isLegalSendInstance(const Instance* instance)
{
	if (Instance::fastDynamicCast<Message>(instance))
		return false;

	if (strictFilter && (strictFilter->filterNew(instance, instance->getParent()) == Reject))
	{
		if (settings().printDataFilters)
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Filtering is enabled. New Instance %s will not be replicated.", instance->getFullName().c_str());

		return false;
	}

	return true;
}

bool ClientReplicator::isLegalSendProperty(Instance* instance, const Reflection::PropertyDescriptor& desc)
{
	if (strictFilter)
	{
		bool isLegal = true;
		if (desc == Instance::propParent)
			isLegal = strictFilter->filterParent(instance, instance->getParent()) == Accept;
		else
			isLegal = strictFilter->filterChangedProperty(instance, desc) == Accept;

		if (!isLegal && (desc.canReplicate() || (desc == Instance::propParent)) && settings().printDataFilters)
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Filtering is enabled. Property %s change for instance %s will not be replicated.", desc.name.c_str(), instance->getFullName().c_str()); 
		
		return isLegal;
	}

	if (desc == Script::prop_EmbeddedSourceCode || desc == ModuleScript::prop_Source)
	{
		if (isCloudEdit())
		{
			if (LuaSourceContainer* lsc = Instance::fastDynamicCast<LuaSourceContainer>(instance))
			{
				return lsc->getCurrentEditor() == NULL;
			}
			else
			{
				RBXASSERT(false);
			}
		}
	}

	return true;
}

bool ClientReplicator::isLegalSendEvent(Instance* instance, const Reflection::EventDescriptor& desc)
{
	if (strictFilter && (strictFilter->filterEvent(instance, desc) == Reject))
	{
		if (settings().printDataFilters)
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Filtering is enabled. Event %s for instance %s will not be replicated.", desc.name.c_str(), instance->getFullName().c_str());

		return false;
	}

	return true;
}

bool ClientReplicator::canSendItems()
{ 
	return receivedGlobals; 
}

bool ClientReplicator::isCloudEdit() const
{
	if (const Client *client = Instance::fastDynamicCast<const Client>(getParent()))
	{
		return client->isCloudEdit();
	}
	return false;
}

class ClientReplicator::RequestCharacterItem : public Item
{
public:
    RequestCharacterItem(Replicator* replicator):Item(*replicator)
    {}

    /*implement*/ bool write(RakNet::BitStream& bitStream)
    {
        Player* targetPlayer = replicator.findTargetPlayer();

        // NOTE: We assume that by now the local player has a replication ID
        if (!targetPlayer)
            throw std::runtime_error("Attempting to send a Character request without a local Player");

        writeItemType(bitStream, ItemTypeRequestCharacter);

        // protocol version 12 -- using int for send stats instead of char
#if !defined(LOVE_ALL_ACCESS)
        unsigned int sendStats = DataModel::sendStats |
            DataModel::get(&replicator)->allHackFlagsOredTogether();
#else
        unsigned int sendStats = 0;
#endif

        bitStream << sendStats;

        bitStream << TeleportService::GetSpawnName();

        replicator.serializeId(bitStream, targetPlayer);

        if (replicator.settings().printInstances)
        {
            RBX::Guid::Data id;
            targetPlayer->getGuid().extract(id);

			StandardOut::singleton()->printf(MESSAGE_SENSITIVE, "%s: Requesting character for %s",
				RakNetAddressToString(replicator.remotePlayerId).c_str(), 
				id.readableString().c_str());
        }

        return true;
    }
};

RakNet::PluginReceiveResult ClientReplicator::OnReceive(RakNet::Packet *packet)
{
	if (packet->systemAddress!=remotePlayerId)
		return Replicator::OnReceive(packet);

	switch ((unsigned char) packet->data[0])
	{
	case ID_CONNECTION_REQUEST_ACCEPTED:
		{
            sendDictionaries();
		}
        return RR_CONTINUE_PROCESSING;

    case ID_DICTIONARY_FORMAT:
        {
            RakNet::BitStream bitStream(packet->data, packet->length, false);
            bitStream.IgnoreBits(8); // Ignore the packet id
            bitStream >> protocolSyncEnabled;
            bitStream >> apiDictionaryCompression;
#ifdef NETWORK_DEBUG
            StandardOut::singleton()->printf(MESSAGE_INFO, "Protocol sync enabled: %s", protocolSyncEnabled?"true":"false");
            StandardOut::singleton()->printf(MESSAGE_INFO, "API dictionary compression enabled: %s", apiDictionaryCompression?"true":"false");
#endif
        }
        return RR_CONTINUE_PROCESSING;

    case ID_PROTOCAL_MISMATCH:
		{
            std::string mismatchString = RBX::format("Protocol mismatch from %s", RakNetAddressToString(packet->systemAddress).c_str());
            
			StandardOut::singleton()->print(MESSAGE_SENSITIVE, mismatchString.c_str());
            // Also log this so we can see in output (primarily for iOS)
            FASTLOGS(FLog::Network,"Protocol mismatch %s",mismatchString.c_str());
            
			Client *client = getParent()->fastDynamicCast<Client>();
			RBXASSERT(client);
			client->connectionFailedSignal(RakNetAddressToString(packet->systemAddress), (int) packet->data[0], "Network protocol mismatch. Please upgrade.");
			requestDisconnect(DisconnectReason_ProtocolMismatch);
			return RR_CONTINUE_PROCESSING;
		}
    case ID_PLACEID_VERIFICATION:
        {
            RakNet::BitStream bitStream(packet->data, packet->length, false);
            bitStream.IgnoreBits(8); // Ignore the packet id
            bool retry;
            bitStream >> retry;
            if (retry)
            {
                // server is still verifying the placeID, request again
                StandardOut::singleton()->printf(MESSAGE_INFO, "Waiting for server to authenticate the placeID before spawning...");
                pendingItems.push_back(new RequestCharacterItem(this));
            }
            else
            {
                StandardOut::singleton()->printf(MESSAGE_ERROR, "Place ID verification failed.");
                Client *client = getParent()->fastDynamicCast<Client>();
                RBXASSERT(client);
                client->connectionFailedSignal(RakNetAddressToString(packet->systemAddress), (int) packet->data[0], "Illegal teleport destination.");
                requestDisconnect(DisconnectReason_IllegalTeleport);
            }
        }
        return RR_STOP_PROCESSING_AND_DEALLOCATE;
	default:
		return Replicator::OnReceive(packet);
	}
}

void ClientReplicator::processPacket(Packet *packet)
{
	RBXPROFILER_SCOPE("Network", "processPacket");
    RBXPROFILER_LABELF("Network", "ID %d (%d bytes)", packet->data[0], packet->length);

	switch (packet->data[0])
	{
    case ID_SCHEMA_SYNC:
        {
            RakNet::BitStream inBitstream(packet->data, packet->length, false);
            inBitstream.IgnoreBits(8); // Ignore the packet id
            learnSchema(inBitstream);
        }
        break;
	case ID_SET_GLOBALS:
		{
			RakNet::BitStream inBitstream(packet->data, packet->length, false);

			inBitstream.IgnoreBits(8); // Ignore the packet id

			{
				bool distributedPhysicsEnabled;
				inBitstream >> distributedPhysicsEnabled;
				NetworkSettings::prop_DistributedPhysics.setValue(networkSettings, distributedPhysicsEnabled);
				if (distributedPhysicsEnabled)
				{
					if(FFlag::RemoveUnusedPhysicsSenders)
					{
						physicsSender.reset(new RoundRobinPhysicsSender(*this));
						PhysicsSender::start(physicsSender);
					}
					else
					{
						createPhysicsSender(NetworkSettings::RoundRobin); // Always sends round robin to the server
					}
				}
				if(FFlag::RemoveInterpolationReciever)
				{
					physicsReceiver.reset(new DirectPhysicsReceiver(this, false));
					physicsReceiver->start(physicsReceiver);
				}
				else
				{
					createPhysicsReceiver(NetworkSettings::Direct, false);
				}

                // create GC job if server is stream data
                inBitstream >> streamingEnabled;
                if (streamingEnabled)
                {
                    gcJob.reset(new ClientReplicator::GCJob(*this));
                    TaskScheduler::singleton().add(gcJob);
                }

                bool networkFilterEnabled;
                inBitstream >> networkFilterEnabled;

                if (Workspace* workspace = ServiceProvider::find<Workspace>(this))
                {
                    workspace->setNetworkFilteringEnabled(networkFilterEnabled);
                    if (canUseProtocolVersion(32))
                    {
                        bool allowThirdPartySales;
                        inBitstream >> allowThirdPartySales;
                        workspace->setAllowThirdPartySales(allowThirdPartySales);
                    }
                }
                else
                {
                    RBXASSERT(false);
                }

                if (networkFilterEnabled)
                    strictFilter.reset(new StrictNetworkFilter(this));

				bool characterAutoLoad;
				inBitstream >> characterAutoLoad;
				if (players)
				{
					players->setCharacterAutoSpawnProperty(characterAutoLoad);
				}
            }

			std::string scopeName;
			inBitstream >> scopeName;
			this->serverScope.set(scopeName);

			if (LuaVM::useSecureReplication() || FFlag::DebugLocalRccServerConnection)
			{
				unsigned int scriptKey;
				inBitstream >> scriptKey;

				unsigned int coreScriptModKey;
				inBitstream >> coreScriptModKey;

				if (ScriptContext* scriptContext = ServiceProvider::find<ScriptContext>(this))
				{
					unsigned int xorKey = boost::hash_value(DataModel::get(this)->getPlaceID());

					scriptContext->setKeys(scriptKey ^ xorKey, coreScriptModKey ^ xorKey);
				}
				else
					RBXASSERT(false);
			}

			uint8_t numTopRepContainers;
			inBitstream >> numTopRepContainers;

			for (unsigned int i = 0; i < numTopRepContainers; i ++)
			{
				const Reflection::ClassDescriptor* classDescriptor;
				classDictionary.receive(inBitstream, classDescriptor, false /*we don't do version check for class here, if the properties or events of the class are changed, they will be handled later*/);

				RBX::Guid::Data id;
				deserializeId(inBitstream, id);

				TopReplContsMap::iterator iter = topReplicationContainersMap.find(classDescriptor);
				if (iter != topReplicationContainersMap.end())
				{
					Instance* inst = *(iter->second);
					guidRegistry->assignGuid(inst, id);
					resolvePendingReferences(inst, id);

					topReplicationContainers.erase(iter->second);
				}
			}

			// disconnect replication data for left over top containers, server does not have these
			for (TopReplConts::iterator iter = topReplicationContainers.begin(); iter != topReplicationContainers.end(); iter++)
			{
				disconnectReplicationData(shared_from(*iter));				
			}

			// no longer need top replication containers list
			topReplicationContainers.clear();
			topReplicationContainersMap.clear();

            receivedGlobals = true;
			receivedGlobalsSignal();

			enableDeserializePacketThread();
		}
		break;
    default:
        Super::processPacket(packet);
        break;
	}
}

void ClientReplicator::receiveCluster(RakNet::BitStream& inBitstream, Instance* instance, bool usingOneQuarterIterator)
{
	if (streamingEnabled) {
		FASTLOG(DFLog::PartStreamingRequests, "Received 1 terrain region.");
		++numInstancesRead; // safe to do here because cluster data is always sent before parts data
	}
	Super::receiveCluster(inBitstream, instance, usingOneQuarterIterator);
}

void ClientReplicator::terrainCellChanged(const Voxel::CellChangeInfo& info)
{
	if (!clusterDebounce && strictFilter && strictFilter->filterTerrainCellChange() == Reject)
	{
		if (settings().printDataFilters)
			StandardOut::singleton()->printf(MESSAGE_WARNING, "Filtering is enabled, terrain cell change will not be replicated.");

		return;
	}

	Super::terrainCellChanged(info);
}

void ClientReplicator::onTerrainRegionChanged(const Voxel2::Region& region)
{
	if (!clusterDebounce && strictFilter && strictFilter->filterTerrainCellChange() == Reject)
	{
		if (settings().printDataFilters)
			StandardOut::singleton()->printf(MESSAGE_WARNING, "Filtering is enabled, terrain cell change will not be replicated.");

		return;
	}

	Super::onTerrainRegionChanged(region);
}

bool ClientReplicator::wantReplicate(const Instance* source) const
{
	if (strictFilter)
	{
		if (Super::wantReplicate(source))
		{
			Player* player = findTargetPlayer();
			PlayerGui* playerGui = player->findFirstChildOfType<PlayerGui>();
			
			// don't replicate objects under PlayerGui
			return !source->isDescendantOf(playerGui);
		}

		return false;
	}

	return Super::wantReplicate(source);
}

void ClientReplicator::postProcessPacket()
{
	if (streamingEnabled) {
		updateClientCapacity();
	}
}

shared_ptr<DeserializedItem> ClientReplicator::deserializeItem(RakNet::BitStream& inBitstream, RBX::Network::Item::ItemType itemType)
{
    RBXPROFILER_SCOPE("Network", "deserializeItem");

	shared_ptr<DeserializedItem> item = shared_ptr<DeserializedItem>();

	switch (itemType)
	{	
	default:
		item = Super::deserializeItem(inBitstream, itemType);
		break;
	case Item::ItemTypeStreamData:
		NETPROFILE_START("readStreamData", &inBitstream);
		item = StreamJob::StreamDataItem::read(*this, inBitstream);
		NETPROFILE_END("readStreamData", &inBitstream);
		break;
	case Item::ItemTypeTag:
		NETPROFILE_START("readTag", &inBitstream);
		item = TagItem::read(*this, inBitstream);
		NETPROFILE_END("readTag", &inBitstream);
		break;
	case Item::ItemTypeStats:
		NETPROFILE_START("readStats", &inBitstream);
		item = StatsItem::read(*this, inBitstream);
		NETPROFILE_END("readStats", &inBitstream);
		break;
    case Item::ItemTypeRocky:
		NETPROFILE_START("readBonus", &inBitstream);
		item = RockyItem::read(*this, inBitstream);
		NETPROFILE_END("readBonus", &inBitstream);
		break;
	}

	return item;
}

void ClientReplicator::readItem(RakNet::BitStream& inBitstream, RBX::Network::Item::ItemType itemType)
{
	switch (itemType)
	{	
	default:
		Super::readItem(inBitstream, itemType);
		break;
	case Item::ItemTypeStreamData:
        NETPROFILE_START("readStreamData", &inBitstream);
		readStreamData(inBitstream);
        NETPROFILE_END("readStreamData", &inBitstream);
		break;
	case Item::ItemTypeTag:
		NETPROFILE_START("readTag", &inBitstream);
		readTag(inBitstream);
		NETPROFILE_END("readTag", &inBitstream);
		break;
	case Item::ItemTypeStats:
		NETPROFILE_START("readStats", &inBitstream);
		statsReceivedSignal(readStats(inBitstream));
		NETPROFILE_END("readStats", &inBitstream);
		break;
    case Item::ItemTypeRocky:
		NETPROFILE_START("readBonus", &inBitstream);
        processRockyItem(inBitstream);
		NETPROFILE_END("readBonus", &inBitstream);
        break;
	}
}

void ClientReplicator::processTag(int tag)
{
	// Alert the ReplicatedFirst service it has received all its descendants
	// This means the service can start running local scripts
	if (tag == REPLICATED_FIRST_FINISHED_TAG)
	{
		if (ReplicatedFirst* repFirst = RBX::ServiceProvider::find<ReplicatedFirst>(this) )
		{
			repFirst->setAllInstancesHaveReplicated();
		}
	}
	else if (tag == TOP_REPLICATION_CONTAINER_FINISHED_TAG)
	{
		// we signal game loading to the client/lua here... we should formalize these client start up functions and move this with it at some point
		if(DataModel* dataModel = DataModel::get(this))
		{
			dataModel->gameLoaded();
		}

		processAllPacketsPerStep = false;

		gameLoadedSignal();

		// reset ping timers
		canTimeout = true;
		replicatorStats.lastReceivedPingTime = RakNet::GetTimeMS();
	}
}

void ClientReplicator::readTag(RakNet::BitStream& inBitstream)
{
	int id;
	inBitstream >> id;
	if (settings().printInstances) {
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE,
			"Received tag %d from %s", id,
			RakNetAddressToString(remotePlayerId).c_str());
	}

	processTag(id);
}

void ClientReplicator::readTagItem(DeserializedTagItem* item)
{
	processTag(item->id);
}

shared_ptr<Reflection::ValueTable> ClientReplicator::readStats(RakNet::BitStream& inBitstream)
{
	shared_ptr<Reflection::ValueTable> jobs(new Reflection::ValueTable());
	shared_ptr<Reflection::ValueTable> scripts(new Reflection::ValueTable());


	// version 2
	// read job info
	bool end;
	inBitstream >> end;
	while (!end)
	{
		std::string name;
		inBitstream >> name;

		float dutyCycle, stepsPerSec, stepTime;
		inBitstream >> dutyCycle;
		inBitstream >> stepsPerSec;
		inBitstream >> stepTime;

		shared_ptr<Reflection::ValueArray> jobInfo(new Reflection::ValueArray());
		jobInfo->push_back(dutyCycle);
		jobInfo->push_back(stepsPerSec);
		jobInfo->push_back(stepTime);

		jobs->insert(std::make_pair(name, shared_ptr<const Reflection::ValueArray>(jobInfo)));

		inBitstream >> end;
	}

	// read script info
	inBitstream >> end;
	while (!end)
	{
		std::string name;
		inBitstream >> name;
		float activity;
		inBitstream >> activity;
		int invocationCount;
		inBitstream >> invocationCount;

		shared_ptr<Reflection::ValueArray> scriptInfo(new Reflection::ValueArray());
		scriptInfo->push_back(activity);
		scriptInfo->push_back(invocationCount);

		scripts->insert(std::make_pair(name, shared_ptr<const Reflection::ValueArray>(scriptInfo)));

		inBitstream >> end;
	}

	// version 1
	float avgPing;
	inBitstream >> avgPing;

	float avgPhysicsSenderFPS;
	inBitstream >> avgPhysicsSenderFPS;

	float totalDataKbps;
	inBitstream >> totalDataKbps;

	float totalPhysicsKbps;
	inBitstream >> totalPhysicsKbps;

	float dataThroughput;
	inBitstream >> dataThroughput;

	shared_ptr<Reflection::ValueTable> stats(new Reflection::ValueTable());
	stats->insert(std::make_pair("Avg Ping ms", avgPing));
	stats->insert(std::make_pair("Avg Physics Sender Pkt/s", avgPhysicsSenderFPS));
	stats->insert(std::make_pair("Total Data KB/s", totalDataKbps));
	stats->insert(std::make_pair("Total Physics KB/s", totalPhysicsKbps));
	stats->insert(std::make_pair("Data Throughput ratio", dataThroughput));

	stats->insert(std::make_pair("Jobs", shared_ptr<const Reflection::ValueTable>(jobs)));
	stats->insert(std::make_pair("Scripts", shared_ptr<const Reflection::ValueTable>(scripts)));

	return stats;
}

void ClientReplicator::readRockyItem(RakNet::BitStream& inBitstream, uint8_t& idx, RBX::Security::NetPmcChallenge& key)
{
    uint8_t subtype;
    inBitstream >> subtype;
    inBitstream >> idx;
    inBitstream >> key.base;
    inBitstream >> key.size;    
    inBitstream >> key.seed; 
    inBitstream >> key.result;
}

void ClientReplicator::doNetPmcCheck(shared_ptr<ClientReplicator> rep, uint8_t idx, RBX::Security::NetPmcChallenge challenge)
{
#if defined(_WIN32) && !defined(RBX_STUDIO_BUILD)
    challenge ^= *const_cast<const RBX::Security::NetPmcChallenge*>(&RBX::Security::kChallenges[idx]); 
    uint32_t result = netPmcHashCheck(challenge);
	rep->pendingItems.push_back(new Replicator::NetPmcResponseItem(rep.get(), result, challenge.result, idx));
#endif
}

void ClientReplicator::processRockyItem(RakNet::BitStream& inBitstream)
{
    uint8_t idx;
    RBX::Security::NetPmcChallenge challenge;
    readRockyItem(inBitstream, idx, challenge);
    DataModel::get(this)->submitTask(
        boost::bind(&ClientReplicator::doNetPmcCheck, shared_from(this), idx, challenge), DataModelJob::Write);
}


void ClientReplicator::markerReceived()
{
}

shared_ptr<Instance> ClientReplicator::sendMarker()
{
	shared_ptr<Instance> superMarker = Super::sendMarker();

	if (Marker* marker = Instance::fastDynamicCast<Marker>(superMarker.get()))
	{
		marker->receivedSignal.connect(boost::bind(&ClientReplicator::markerReceived, this));
	}

	return superMarker;
}

void ClientReplicator::processStreamDataRegionId(Replicator::StreamJob::RegionIteratorSuccessor successorBitMask, StreamRegion::Id id)
{
	if (successorBitMask == Replicator::StreamJob::ITER_INCX)
	{
		id = lastReadStreamId + Vector3int32(1,0,0);
	}
	else if (successorBitMask == Replicator::StreamJob::ITER_INCY)
	{
		id = lastReadStreamId + Vector3int32(0,1,0);
	}
	else if (successorBitMask == Replicator::StreamJob::ITER_INCZ)
	{
		id = lastReadStreamId + Vector3int32(0,0,1);
	}
	else if (successorBitMask != Replicator::StreamJob::ITER_NONE)
	{
		RBXASSERT(false);
	}

	lastReadStreamId = id;

	if (gcJob)
		gcJob->insertRegion(id);
}

void ClientReplicator::readStreamDataItem(DeserializedStreamDataItem* item)
{
	Timer<Time::Precise> timer;

	processStreamDataRegionId((Replicator::StreamJob::RegionIteratorSuccessor)item->successorBitMask, item->id);

	if (item->deserializedJoinDataItem)
	{
		int numRead = readJoinDataItem(item->deserializedJoinDataItem.get());
		numInstancesRead += numRead;

		if (numRead > 0)
		{
			avgInstancesPerStreamData.sample(numRead);
			avgStreamDataReadTime.sample(timer.delta().seconds());
		}
	}
}

void ClientReplicator::readStreamData(RakNet::BitStream& bitStream)
{
    NETPROFILE_START("readStreamDataHeader", &bitStream);
    Timer<Time::Precise> timer;
	// stream data item is just join data plus an extra StreamRegion::Id
    StreamRegion::Id id;

    bool lowBit, highBit;
    bitStream >> lowBit;
    bitStream >> highBit;
    Replicator::StreamJob::RegionIteratorSuccessor successorBitMask = (Replicator::StreamJob::RegionIteratorSuccessor)((int)lowBit + (((int)highBit)<<1));

	if (successorBitMask == Replicator::StreamJob::ITER_NONE)
	{
		bitStream >> id;
	}

	NETPROFILE_END("readStreamDataHeader", &bitStream);

	processStreamDataRegionId(successorBitMask, id);

    NETPROFILE_START("readJoinData", &bitStream);
	int numRead = readJoinData(bitStream);
    NETPROFILE_END("readJoinData", &bitStream);
    numInstancesRead += numRead;

	if (numRead > 0)
	{
		avgInstancesPerStreamData.sample(numRead);
        avgStreamDataReadTime.sample(timer.delta().seconds());
	}

	FASTLOG2(DFLog::PartStreamingRequests, "Received %d instances, %d pending requests remaining",
		numRead,
		pendingInstanceRequests - numInstancesRead);
}

bool ClientReplicator::checkDistributedReceive(PartInstance* part)
{
	bool clientIsOwner = (part->getNetworkOwner() == RakNetToRbxAddress(clientAddress));
    return !clientIsOwner;
}

bool ClientReplicator::checkDistributedSend(const PartInstance* part)
{
    return true;
}

// assumes part is root
bool ClientReplicator::checkDistributedSendFast(const PartInstance* part)
{
    return true;
}

class ClientReplicator::ClientCapacityUpdateItem : public Item
{
	int numInstanceDiff;
    short maxRegionRadius;

public:
	ClientCapacityUpdateItem(Replicator* replicator, int _numInstanceDiff, short _maxRegionRadius):Item(*replicator), numInstanceDiff(_numInstanceDiff), maxRegionRadius(_maxRegionRadius)
	{}

	/*implement*/ bool write(RakNet::BitStream& bitStream)
	{
		writeItemType(bitStream, ItemTypeUpdateClientQuota);
		bitStream << numInstanceDiff;
        bitStream << maxRegionRadius;
		return true;
	}
};



void ClientReplicator::requestCharacter()
{
	requestCharacterImpl();
}

void ClientReplicator::requestCharacterImpl()
{
	pendingItems.push_back(new RequestCharacterItem(this));
				
	// we signal game loading to the client/lua here... we should formalize these client start up functions and move this with it at some point
	if(DataModel* dataModel = DataModel::get(this))
	{
		dataModel->gameLoaded();
	}

	if (Player* targetPlayer = findTargetPlayer())
		playerCharacterAddedConnection = targetPlayer->characterAddedSignal.connect(boost::bind(&ClientReplicator::onPlayerCharacterAdded, this));
	else
	{
		RBXASSERT(false);
		processAllPacketsPerStep = false;
	}
}

void ClientReplicator::requestServerStats(bool request)
{
	RakNet::BitStream bitStream;
	bitStream << (unsigned char) ID_REQUEST_STATS;
	bitStream << request;
	if (request)
	{
		bitStream << STATS_ITEM_VERSION;
	}

	rakPeer->rawPeer()->Send(&bitStream, networkSettings->getDataSendPriority(), DATAMODEL_RELIABILITY, DATA_CHANNEL, remotePlayerId, false);	
}

bool ClientReplicator::hasEnoughMemoryToReceiveInstances()
{
    return (!gcJob->pendingGC()) && memoryLevel >= RBX::MemoryStats::MEMORYLEVEL_OK;
}

bool ClientReplicator::needGC()
{
    return gcJob->pendingGC() || memoryLevel <= RBX::MemoryStats::MEMORYLEVEL_ALL_LOW;
}

bool ClientReplicator::canUpdateClientCapacity() 
{
    double nowTime = Time::nowFastSec();
    if (nowTime - sampleTimer > kMaxIncomePacketWaitTime/2)
    {
        sampleTimer = nowTime;
        return true;
    }
    else
    {
        return false;
    }
}

void ClientReplicator::updateMemoryStats()
{
    memoryLevel = RBX::MemoryStats::slowCheckMemoryLevel(((RBX::MemoryStats::memsize_t)NetworkSettings::singleton().getExtraMemoryUsedInMB())*1024*1024);
    if (memoryLevel == RBX::MemoryStats::MEMORYLEVEL_ONLY_PHYSICAL_CRITICAL_LOW)
    {
        // if the physical memory level is critical while pool memory is ample, release all the pool memory
        MemoryStats::releaseAllPoolMemory();
    }
}

void ClientReplicator::updateClientCapacity()
{
	if (canUpdateClientCapacity())
	{
        const bool haveEnoughMemory = hasEnoughMemoryToReceiveInstances();

        if (!haveEnoughMemory) {
            if (!loggedLowMemWarning) {
                FASTLOG2(DFLog::PartStreamingRequests, "Not enough memory to request more parts: %u free, %u additional required",
                    MemoryStats::freeMemoryBytes(), (FInt::StreamingCriticalLowMemWatermarkMB*1024*1024) - MemoryStats::freeMemoryBytes());
                loggedLowMemWarning = true;
            }
        } else {
            loggedLowMemWarning = false;
        }

        int lastClientInstanceQuota = clientInstanceQuota;
        float predictedTotalInstanceProcessTime = 0.0f;
        if (!haveEnoughMemory)
        {
            clientInstanceQuota = 0; // this will clear the existing server pending queue
        }
        else
        {
            if (!avgStreamDataReadTime.hasSampled())
            {
                // no sample yet, use initial value
                clientInstanceQuota = DFInt::ClientInstanceQuotaInitial;
            }
            else
            {
                float avgInstanceProcessTime = avgStreamDataReadTime.value() / avgInstancesPerStreamData.value();
                predictedTotalInstanceProcessTime = avgInstanceProcessTime * avgInstancesPerStreamData.value() * incomingPacketsCount();
                // evaluate the client capacity
                clientInstanceQuota = (RBX::Network::kMaxIncomePacketWaitTime - predictedTotalInstanceProcessTime) / avgInstanceProcessTime;
                if (clientInstanceQuota < 1)
                {
                    clientInstanceQuota = 1; // this will not clear the existing server pending queue
                }
                else if (clientInstanceQuota > DFInt::ClientInstanceQuotaCap)
                {
                    clientInstanceQuota = DFInt::ClientInstanceQuotaCap;
                }
            }
        }
        bool maxRegionDistanceChanged = gcJob->updateMaxRegionDistance();
        int instanceQuotaDiff = clientInstanceQuota - lastClientInstanceQuota;
        if (instanceQuotaDiff != 0 || maxRegionDistanceChanged)
        {
            if (settings().printStreamInstanceQuota)
            {
                StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "clientInstanceQuota %d, packet in queue %d, predictedTotalInstanceProcessTime %f, avgStreamDataReadTime %f, avgInstancesPerStreamData %f", clientInstanceQuota, (int)incomingPacketsCount(), predictedTotalInstanceProcessTime, avgStreamDataReadTime.value(), avgInstancesPerStreamData.value());
            }
            pendingItems.push_back(new ClientCapacityUpdateItem(this, instanceQuotaDiff, gcJob->getMaxRegionDistance()));
        }
        numInstancesRead = 0;
        avgRequestCount.sample(clientInstanceQuota);
	}
}

void ClientReplicator::onPlayerCharacterAdded()
{
	// we are only interested in first time this happen to signify the player has successfully joined the game
	playerCharacterAddedConnection.disconnect();

	processAllPacketsPerStep = false;
}

void ClientReplicator::dataOutStep()
{
	propSync.expireItems();
	Super::dataOutStep();
}

bool ClientReplicator::processChangedParentPropertyForStreaming(const Guid::Data& parentId, Reflection::Property prop)
{
	RBXASSERT(streamingEnabled && prop.getDescriptor() == Instance::propParent);

	Instance* instance = static_cast<Instance*>(prop.getInstance());
	shared_ptr<Instance> parent;
	bool recognizedId = guidRegistry->lookupByGuid(parentId, parent);

	// ships in the night:
	// client sent a GC notice for Part A
	// simultaneously server sent a reparent message setting parent to Part A
	// resolution: GC reparented instance (and its descendants)
	RBXASSERT(gcJob);
	if (gcJob && instance && !recognizedId)
	{
		// TODO: add an ACK mechanism to GC instance list, and only allow unknown
		// parents to be interpreted as ships-in-the-night with gc if we know the
		// parent in question has a GC notice in flight
		gcJob->notifyServerGcingInstanceAndDescendants(shared_from(instance));
		streamOutInstance(instance, true);

		// done processing this property change
		return true;
	}

	return false;
}

void ClientReplicator::readChangedProperty(RakNet::BitStream& bitStream, Reflection::Property prop)
{
	// This is the mirror image of ServerReplicator::writeChangedProperty
    // also, if you make changes, please also take care of ClientReplicator::skipChangedProperty
	bool versionReset;
	bitStream >> versionReset;
	propSync.onReceivedPropertyChanged(prop, versionReset);

	// if we are streaming, check to see if we are setting parent to an unknown guid
	if (streamingEnabled && prop.getDescriptor() == Instance::propParent)
	{
		// Remember the read offset before we read out the id from the stream.
		RakNet::BitSize_t currentReadOffset = bitStream.GetReadOffset();

		RBX::Guid::Data parentId;
		deserializeId(bitStream, parentId);

		if (processChangedParentPropertyForStreaming(parentId, prop))
			return;
		else
		{
			// undo reading the property value, and let the normal property update path continue.
			bitStream.SetReadOffset(currentReadOffset);
		}
	}

	if (prop.getDescriptor() == PartInstance::prop_CFrame)
	{
		// special-case CFrames. Send acknowledgement right away so that physics data won't get filtered
		const PartInstance* p = boost::polymorphic_downcast<const PartInstance*>(prop.getInstance());
		if (p)
			pendingItems.push_back(new (cframePool.get()) CFrameAcknowledgementItem(this, shared_from(p)));
	}

	Super::readChangedProperty(bitStream, prop);
}

void ClientReplicator::readChangedPropertyItem(DeserializedChangePropertyItem* item, Reflection::Property prop)
{
	propSync.onReceivedPropertyChanged(prop, item->versionReset);

	if (streamingEnabled && prop.getDescriptor() == Instance::propParent)
	{
		Guid::Data parentId = item->value.get<Guid::Data>();
		if (processChangedParentPropertyForStreaming(parentId, prop))
			return;
	}
	else if (prop.getDescriptor() == PartInstance::prop_CFrame)
	{
		// special-case CFrames. Send acknowledgement right away so that physics data won't get filtered
		const PartInstance* p = boost::polymorphic_downcast<const PartInstance*>(prop.getInstance());
		if (p)
			pendingItems.push_back(new (cframePool.get()) CFrameAcknowledgementItem(this, shared_from(p)));
	}

	Super::readChangedPropertyItem(item, prop);
	
}

void ClientReplicator::writePropAcknowledgementIfNeeded(const Instance* instance, const Reflection::PropertyDescriptor& desc, RakNet::BitStream& outBitStream)
{
    DescriptorSender<RBX::Reflection::PropertyDescriptor>::IdContainer idContainer = propDictionary.getId(&desc);
    if (idContainer.outdated)
    {
        return;
    }

	int version;
	if (propSync.onPropertySend(Reflection::ConstProperty(desc, instance), version) == PropSync::Slave::DontSendAcknowledgement)
		return;

	Item::writeItemType(outBitStream, Item::ItemTypePropAcknowledgement);
	outBitStream << version;
	propDictionary.send(outBitStream, idContainer.id);
	serializeId(outBitStream, instance);
}

bool ClientReplicator::isLimitedByOutgoingBandwidthLimit() const {
	if (const RakNet::RakNetStatistics* rakStats = getRakNetStats()) {
		return rakStats->isLimitedByOutgoingBandwidthLimit;
	}
	return true;
}

void ClientReplicator::writeChangedProperty(const Instance* instance, const Reflection::PropertyDescriptor& desc, RakNet::BitStream& outBitStream)
{
	// Write the ItemTypePropAcknowledgement message before sending the property
	writePropAcknowledgementIfNeeded(instance, desc, outBitStream);

	Super::writeChangedProperty(instance, desc, outBitStream);
}

void ClientReplicator::writeChangedRefProperty(const Instance* instance,
	const Reflection::RefPropertyDescriptor& desc, const Guid::Data& newRefGuid,
	RakNet::BitStream& outBitStream)
{
	// Write the ItemTypePropAcknowledgement message before sending the property
	writePropAcknowledgementIfNeeded(instance, desc, outBitStream);

	Super::writeChangedRefProperty(instance, desc, newRefGuid, outBitStream);
}

FilterResult ClientReplicator::filterChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc)
{
    FilterResult result = Super::filterChangedProperty(instance, desc);
    if (streamingEnabled && result == Accept)
    {
        if (Humanoid* humanoid = instance->fastDynamicCast<Humanoid>())
        {
            if (!humanoid->getTorsoSlow())
            {
                return Reject;
            }
        }
    }
    return result;
}

FilterResult ClientReplicator::filterReceivedParent(Instance* instance, Instance* parent)
{
	FilterResult result = Super::filterReceivedParent(instance, parent);
	if (result == Accept)
	{
		// Can't set if parent is locked
		if (instance->getIsParentLocked())
		{
			StandardOut::singleton()->printf(MESSAGE_WARNING, "trying to set locked parent!");
			return Reject;
		}
	}
	return result;
}

void ClientReplicator::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
#if !defined(RBX_STUDIO_BUILD) && !defined(RBX_PLATFORM_DURANGO)
#ifdef _WIN32
    TaskScheduler::singleton().remove(memoryCheckerCheckerJob);
    memoryCheckerCheckerJob.reset();
#endif

#if defined(_WIN32) || (defined(__APPLE__) && !defined(RBX_PLATFORM_IOS))
    TaskScheduler::singleton().remove(memoryCheckerJob);
    memoryCheckerJob.reset();
#endif

#ifdef _WIN32
    TaskScheduler::singleton().remove(badAppCheckerJob);
    badAppCheckerJob.reset();
#endif
#endif

	if (gcJob)
	{
		TaskScheduler::singleton().remove(gcJob);
        gcJob->unregisterCoarseMovementCallback();
		gcJob.reset();
	}

	Super::onServiceProvider(oldProvider, newProvider);
	
	hashReadyConnection.disconnect();
	mccReadyConnection.disconnect();

	if (newProvider)
	{ 
#if !defined(RBX_STUDIO_BUILD)
        VMProtectBeginMutation("30");
#if !defined(LOVE_ALL_ACCESS) && defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
        badAppCheckerJob.reset(new BadAppCheckerJob(shared_from(this)));
        TaskScheduler::singleton().add(badAppCheckerJob);
#endif

#if !defined(LOVE_ALL_ACCESS) && (defined(_WIN32) || (defined(__APPLE__) && !defined(RBX_PLATFORM_IOS))) && !defined(RBX_PLATFORM_DURANGO)
        memoryCheckerJob.reset(new MemoryCheckerJob(shared_from(this)));
        TaskScheduler::singleton().add(memoryCheckerJob);
#endif

#if !defined(LOVE_ALL_ACCESS) && defined(_WIN32) && !defined(RBX_PLATFORM_DURANGO)
        memoryCheckerCheckerJob.reset(new MemoryCheckerCheckerJob(shared_from(this)));
        TaskScheduler::singleton().add(memoryCheckerCheckerJob);
        hashReadyConnection = memoryCheckerJob->hashReadySignal.connect(boost::bind(&ClientReplicator::onHashReady, this));
        mccReadyConnection = memoryCheckerCheckerJob->reportReadySignal.connect(boost::bind(&ClientReplicator::onMccReady, this));
#endif
        VMProtectEnd();
#endif
	}
}

void ClientReplicator::onHashReady()
{
    unsigned long long thisTag = Tokens::apiToken.crypt();
    unsigned long long prevTag = Tokens::apiToken.getPrev();
    pendingItems.push_back(new HashItem(this, &pmcHash, Tokens::sendStatsToken.crypt(), thisTag, prevTag));
}

void ClientReplicator::onMccReady()
{
    pendingItems.push_back(new RockyItem(this, ::mccReport));
}

bool ClientReplicator::canUseProtocolVersion(int protocolVersion) const {
	return protocolVersion <= NETWORK_PROTOCOL_VERSION;
}

void ClientReplicator::deserializeSFFlags(RakNet::BitStream& inBitStream)
{
    unsigned short numOfSFFlags;
    inBitStream.Read(numOfSFFlags);
    while (numOfSFFlags--)
    {
        RakNet::RakString name;
        RakNet::RakString varValue;
        inBitStream.Read(name);
        inBitStream.Read(varValue);
        //RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, 
        //    "Received FFLag: %s: %s", name.C_String(), varValue.C_String());
        std::string stdName(name.C_String());
        std::string stdValue(varValue.C_String());
        FLog::SetValueFromServer(stdName, stdValue);
    }
}

void ClientReplicator::streamOutPartHelper(const Guid::Data& data,
		PartInstance* part, shared_ptr<Instance> descendant) {

	if (shared_ptr<JointInstance> jointInstance =
			Instance::fastSharedDynamicCast<JointInstance>(descendant)) {

		const Reflection::RefPropertyDescriptor* refPropDescriptor = NULL;
		if (jointInstance->getPart0() == part) {
			refPropDescriptor = &JointInstance::prop_Part0;
		} else if (jointInstance->getPart1() == part) {
			refPropDescriptor = &JointInstance::prop_Part1;
		} else {
			return;
		}

		// HACK: re-use the "removingInstance" functionality to suppress
		// replication of events and property changes while we set the joint's
		// part reference (to the streamed out part) to NULL.
		{
			RBX::ScopedAssign<Instance*> assign(removingInstance, jointInstance.get());
			refPropDescriptor->setRefValue(jointInstance.get(), NULL);
		}

		addPendingRef(refPropDescriptor, jointInstance, data);
	}
}

void ClientReplicator::streamOutAutoJointHelper(std::vector<shared_ptr<PartInstance> > pendingRemovalParts, shared_ptr<Instance> instance)
{
    if (shared_ptr<JointInstance> jointInstance =
        Instance::fastSharedDynamicCast<JointInstance>(instance))
    {
        RBXASSERT(Joint::isManualJoint(jointInstance->getJoint()) == false);

        PartInstance* partInstance0 = jointInstance->getPart0();
        PartInstance* partInstance1 = jointInstance->getPart1();

        if (partInstance0 == NULL && partInstance1 == NULL)
            return;
        for (std::vector<shared_ptr<PartInstance> >::iterator iter = pendingRemovalParts.begin(); iter != pendingRemovalParts.end(); iter++)
        {
            shared_ptr<PartInstance> pendingRemovalPart = *iter;
            if (pendingRemovalPart)
            {
                const Reflection::RefPropertyDescriptor* refPropDescriptor = NULL;
                Guid::Data data;
                if (partInstance0 == pendingRemovalPart.get())
                {
                    refPropDescriptor = &JointInstance::prop_Part0;
                    partInstance0->getGuid().extract(data);
                }
                else if (partInstance1 == pendingRemovalPart.get())
                {
                    refPropDescriptor = &JointInstance::prop_Part1;
                    partInstance1->getGuid().extract(data);
                }
                else
                {
                    continue;
                }
                {
                    RBX::ScopedAssign<Instance*> assign(removingInstance, jointInstance.get());
                    refPropDescriptor->setRefValue(jointInstance.get(), NULL);
                }
                addPendingRef(refPropDescriptor, jointInstance, data);
            }
        }
    }
}

void unregisterHelper(GuidItem<Instance>::Registry* registry, const shared_ptr<Instance>& instance)
{
	registry->tryUnregister(instance.get());
}

void ClientReplicator::streamOutTerrain(const Vector3int16 &cellPos) {
	ScopedAssign<bool> assignment(clusterDebounce, true); // prevent replication of garbage collection
    if (megaClusterInstance)
    {
	    megaClusterInstance->getVoxelGrid()->setCell(cellPos, Voxel::Constants::kUniqueEmptyCellRepresentation, Voxel::CELL_MATERIAL_Unspecified);
    }
}

void ClientReplicator::streamOutInstance(Instance* instance, bool deleteImmediately)
{
	RBXASSERT(instance);

    CPUPROFILER_START(RBX::Network::NetworkProfiler::PROFILER_streamOutPart);

	disconnectReplicationData(shared_from(instance));

    CPUPROFILER_STEP(RBX::Network::NetworkProfiler::PROFILER_streamOutPart);
	// Unregistration happens in the instance destructor. Perform it here
	// to be on the safe side. This ensures the streamed out instance is not
	// used if the guid is seen later, possibly before all of the shared
	// pointers to the instance are released.
    Guid::Data data;
    instance->getGuid().extract(data);
    instance->visitDescendants(boost::bind(&unregisterHelper, guidRegistry.get(), _1));
    guidRegistry->unregister(instance);

	CPUPROFILER_STEP(RBX::Network::NetworkProfiler::PROFILER_streamOutPart);

	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance)) 
	{
		part->setIsCurrentlyStreamRemovingPart();

		// Clean manual joints
		Primitive* prim = part->getPartPrimitive();
		for (int i = 0; i < prim->getNumJoints(); ++i) {
			Joint* joint = prim->getJoint(i);
			if (Joint::isManualJoint(joint)) {
				JointInstance* jointInstance =
						static_cast<JointInstance*>(joint->getJointOwner());
				// Don't worry about joints that are descendents of the part, they
				// should be streamed back when the part comes back.
				if (!jointInstance->isDescendantOf(part)) {
					streamOutPartHelper(data, part, shared_from(jointInstance));
				}
			}
		}

        if (deleteImmediately)
        {
            // Clean auto joints
            // This mechanism can't rely on primitive->getJoint() because that function
            // only returns active joints (where both part0 and part1 are set). If the
            // second part of a joint is streamed out, we still want to null out the
            // part reference in that joint.
            if (JointsService* jointsService = ServiceProvider::find<JointsService>(this)) {
                jointsService->visitDescendants(
                    boost::bind(&ClientReplicator::streamOutPartHelper, this, data, part, _1));
            }
        }
	}
    else
    {
        // not a part instance, we can safely delete it
        deleteImmediately = true;
    }

    CPUPROFILER_STEP(RBX::Network::NetworkProfiler::PROFILER_streamOutPart);

	// Normally removingInstance is set after the replicator sees the
	// setParent(NULL) property change event. Set it before hand to
	// suppress any associated events and property changes. This makes the
	// removal local (not replicated).

	// TODO: destroy can throw if the part or any of its children have
	// locked parents. Is it possible to stream out an instance with a 
	// locked parent?
    if (deleteImmediately)
    {
	    RBX::ScopedAssign<Instance*> assign(removingInstance, instance);
	    instance->setParent(NULL);
    }

    CPUPROFILER_STEP(RBX::Network::NetworkProfiler::PROFILER_streamOutPart);
}

int ClientReplicator::getNumRegionsToGC() const
{ 
	return gcJob ? gcJob->getNumRegionsToGC() : 0; 
}

short ClientReplicator::getGCDistance() const
{ 
	return gcJob ? gcJob->getGCDistance() : 0; 
}

void ClientReplicator::renderStreamedRegions(Adorn* adorn)
{
	if (gcJob)
		gcJob->render3dAdorn(adorn);
}

int ClientReplicator::getNumStreamedRegions() const
{
	return gcJob ? gcJob->getNumRegions() : 0;
}

void ClientReplicator::renderPartMovementPath(Adorn* adorn)
{
    if (physicsReceiver)
    {
        physicsReceiver->renderPartMovementPath(adorn);
    }
}

using namespace RBX::Reflection;

void ClientReplicator::learnSchema(RakNet::BitStream& inBitStream)
{
#ifdef NETWORK_DEBUG
    StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Syncing schema from server..");
#endif
    RakNet::BitStream bitStream;
    decompressBitStream(inBitStream, bitStream);

    unsigned numTotalEnum, enumMSB, numClass, numProp, numEvent, numArg;
    unsigned int classId, propId, typeId, eventId;
    RakNet::RakString enumName, className, propName, propType, eventName;
    unsigned char replicationLevel;
    bool canReplicate, isEnum;

    // --- Enums
    bitStream >> numTotalEnum;
    for (size_t i=0; i<numTotalEnum; i++)
    {
        bitStream.Read(enumName);
        bitStream >> enumMSB;

        // can we find the enum locally?
        const Name& enumNameName = Name::declare(enumName.C_String());

        serverEnums.insert(std::make_pair(&enumNameName, ReflectionEnumContainer(enumMSB)));
    }

    // --- Classes
    bitStream >> numClass;
    for (size_t i=0; i<numClass; i++)
    {
        // read class desc
        bitStream >> classId;
        bitStream.Read(className);
        bitStream >> replicationLevel;

        const Name& classNameName = Name::declare(className.C_String());

        shared_ptr<ReflectionClassContainer> classContainer(new ReflectionClassContainer(classId, classNameName, (Reflection::ReplicationLevel)replicationLevel));

        const ClassDescriptor* classDesc;
        classDictionary.getValue(classId, classDesc);

        if (!classDesc)
        {
            classContainer->needSync = true;
        }
        
        bool classIsDesync = false;
        // --- Properties
        bitStream >> numProp;
        for (size_t j=0; j<numProp; j++)
        {
            // read prop desc
            bitStream >> propId;
            bitStream.Read(propName);
            bitStream >> typeId;
            bitStream.Read(propType);
            bitStream >> canReplicate;
            bitStream >> isEnum;
            if (isEnum)
            {
                bitStream >> enumMSB;
            }
            else
            {
                enumMSB = 0;
            }
            const Reflection::Type* type;
            typeDictionary.getValue(typeId, type);
            shared_ptr<ReflectionPropertyContainer> propertyContainer(new ReflectionPropertyContainer(propId, Name::declare(propName.C_String()), typeId, std::string(propType.C_String()), type, canReplicate, enumMSB));
            if (!type)
            {
                if (!isEnum)
                {
                    StandardOut::singleton()->printf(MESSAGE_WARNING, "A new type (%s) is used for property '%s', please make sure this property will not be replicated to or from the legacy clients.", propType.C_String(), propName.C_String());
                }
                // we are using a new type for this property, we should always use server schema for it
                propertyContainer->needSync = true;
            }

            const Reflection::PropertyDescriptor* propDesc;
            propDictionary.getValue(propId, propDesc);
            if (propDesc)
            {
                // has the property attributes changed?
                if (propDesc->canReplicate() != canReplicate
                    || strcmp(propType.C_String(), propDesc->type.name.c_str()) != 0)
                {
                    // property attribute changed
                    propertyContainer->needSync = true;
                }
#ifdef NETWORK_DEBUG
                if (FFlag::DebugProtocolSynchronization)
                {
                    // This is to simulate property adding / changing scenario
                    if (classContainer->name == "WedgePart" && propertyContainer->name == "BrickColor")
                    {
                        StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Simulating property adding / changing on WedgePart.BrickColor");
                        propertyContainer->needSync = true;
                        ClassDescriptor::ClassDescriptors::const_iterator iter = ClassDescriptor::all_begin();
                        while (iter!=ClassDescriptor::all_end())
                        {
                            if ((*iter)->name == classDesc->name)
                            {
                                *((*iter)->isOutdated) = true;
                                break;
                            }
                            ++iter;
                        }
                    }
                }
#endif
            }
            else
            {
                //StandardOut::singleton()->printf(RBX::MESSAGE_WARNING, "Unknown property %s (%s)", propName, propType);
                propertyContainer->needSync = true;
            }
            if (classContainer->needSync == true)
            {
                // if the class is new or removed, we invalidate all its properties
                propertyContainer->needSync = true;
            }

            classContainer->properties.push_back(propertyContainer);
            if (propertyContainer->needSync == true)
            {
                // if a class' property is new, removed, or modified, we mark the class as changed
                classIsDesync = true;
            }
        }
        // --- Events
        bitStream >> numEvent;
        for (size_t j=0; j<numEvent; j++)
        {
            // read event desc
            bitStream >> eventId;
            bitStream.Read(eventName);
            bitStream >> numArg;
            shared_ptr<ReflectionEventContainer> eventContainer(new ReflectionEventContainer(eventId, Name::declare(eventName.C_String())));
            for (size_t k=0; k<numArg; k++)
            {
                bitStream >> typeId;
                const Reflection::Type* type;
                typeDictionary.getValue(typeId, type);
                RBXASSERT(type);
                eventContainer->argTypes.push_back(type);
            }
            //StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Event %s, %d", eventContainer->name.c_str(), eventContainer->argTypes.size());

            const Reflection::EventDescriptor* eventDesc;
            eventDictionary.getValue(eventId, eventDesc);
            if (eventDesc)
            {
                if (eventDesc->getSignature().arguments.size() != numArg)
                {
                    eventContainer->needSync = true;
                }
                else
                {
                    size_t l = 0;
                    for(std::list<Reflection::SignatureDescriptor::Item>::const_iterator argIter = eventDesc->getSignature().arguments.begin(); 
                        argIter != eventDesc->getSignature().arguments.end(); 
                        ++argIter)
                    {
                        const Reflection::SignatureDescriptor::Item arg = *argIter;
                        if (arg.type->name != eventContainer->argTypes[l++]->name)
                        {
                            eventContainer->needSync = true;
                        }
                        // we don't perform enum check here because we want to give some tolerance to enum changes
                        // e.g., if we add a new value to the end of enum, we should still expect the legacy client behave normally
                        // In this case, changing or removing enum values should be prohibited or customized code should be added to handle legacy clients
                        //else if (const Reflection::EnumDescriptor* enumDesc = Reflection::EnumDescriptor::lookupDescriptor(*typeIter->type))
                        //{
                        //    size_t newMSB = 0;
                        //    if (hasEnumChanged(*enumDesc, newMSB))
                        //    {
                        //        eventContainer->needSync = true;
                        //    }
                        //}
                    }
                }
#ifdef NETWORK_DEBUG
                if (FFlag::DebugProtocolSynchronization)
                {
                    // This is to simulate event adding / changing scenario
                    if (classContainer->name == "Player" && eventContainer->name == "OnTeleport")
                    {
                        StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Simulating event adding / changing on Player.OnTeleport");
                        eventContainer->needSync = true;
                        ClassDescriptor::ClassDescriptors::const_iterator iter = ClassDescriptor::all_begin();
                        while (iter!=ClassDescriptor::all_end())
                        {
                            if ((*iter)->name == classDesc->name)
                            {
                                *((*iter)->isOutdated) = true;
                                break;
                            }
                            ++iter;
                        }
                    }
                }
#endif
            }
            else
            {
                // event added to newer version
                eventContainer->needSync = true;
            }

            if (classContainer->needSync)
            {
                // if the class is new or removed, we invalidate all its events
                eventContainer->needSync = true;
            }

            classContainer->events.push_back(eventContainer);
            if (eventContainer->needSync)
            {
                classIsDesync = true;
            }
        }

        if (classIsDesync || (classDesc && *(classDesc->isOutdated)))
        {
            classContainer->needSync = true;
        }

        serverClasses.insert(std::make_pair(&classNameName, classContainer));
    }
}

void ClientReplicator::writeProperties(const Instance* instance, RakNet::BitStream& outBitstream, PropertyCacheType cacheType, bool useDictionary)
{
    if (*(instance->getDescriptor().isOutdated))
    {
        // class is outdated, serialize using server schema
        bool cacheable = (cacheType != PropertyCacheType_NonCacheable);

        ReflectionClassMap::const_iterator classIter = serverClasses.find(&instance->getClassName());

        if (classIter != serverClasses.end())
        {
            const shared_ptr<ReflectionClassContainer>& reflectionClass = classIter->second;

            if (reflectionClass->needSync == true)
            {
                // the content of this class has changed, we need use server scheme to deserialize the properties
                ReflectionPropertyList::const_iterator propIter = reflectionClass->properties.begin();
                RBX::Reflection::ConstPropertyIterator iter = instance->properties_begin();
                for (; propIter != reflectionClass->properties.end(); propIter++)
                {
                    const shared_ptr<ReflectionPropertyContainer>& reflectionProperty = *propIter;
                    if (reflectionProperty->needSync == true)
                    {
                        if (reflectionProperty->canReplicate)
                        {
                            // new property or modified property, let's use the default value
                            if (cacheType == PropertyCacheType_All || isPropertyCacheable(reflectionProperty->type, reflectionProperty->enumMSB > 0) == cacheable)
                            {
                                // optimization for bool properties
                                if (settings().printBits) {
                                    std::string str = RBX::format(
                                        "   write %s %s, 1 bit (using server schema)", 
                                        reflectionProperty->type ? reflectionProperty->type->name.c_str() : "Unknown",
                                        reflectionProperty->name.c_str()
                                        );

                                    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, "%s", str.c_str());

                                }
                                outBitstream << true;
                            }
                        }
                    }
                    else
                    {
                        Reflection::ConstProperty property = *iter;
#ifdef NETWORK_DEBUG
                        if (FFlag::DebugProtocolSynchronization)
                        {
                            // Cope with property adding / changing simulation
                            if (instance->getDescriptor().name == "WedgePart" && property.getName().toString() == "BrickColor")
                            {
                                ++iter;
                                property = *iter;
                            }
                        }
#endif
                        while (property.getName() != reflectionProperty->name && iter != instance->properties_end())
                        {
                            ++iter;
                            property = *iter;
                        }

						writePropertiesInternal(instance, property, outBitstream, cacheType, useDictionary);

                        ++iter;
                    }
                }
                RBXASSERT(iter == instance->properties_end());
            }
            else
            {
                // client schema is same as server's, why we get here?
                RBXASSERT(false);
            }
        }
        else
        {
            // server does not know about our type
        }
    }
    else
    {
        Super::writeProperties(instance, outBitstream, cacheType, useDictionary);
    }
}

bool ClientReplicator::ProcessOutdatedChangedProperty(RakNet::BitStream& inBitstream, const RBX::Guid::Data& id, const Instance* instance, const Reflection::PropertyDescriptor* propertyDescriptor, unsigned int propId)
{
    if (instance && !(*(instance->getDescriptor().isOutdated)))
    {
        // if the class is not outdated, its properties won't be too because the class checksum is an accumulation of its property and event checksums
        return false;
    }
    bool skipProperty = false;
    if (propertyDescriptor == NULL)
    {
        // we don't have the property definition locally, let's skip the property
        skipProperty = true;
    }
    else
    {
        if (instance)
        {
            // do client and server agree on the property attributes (e.g., type)?
            shared_ptr<ReflectionPropertyContainer> serverBasedProperty;
            if (getServerBasedProperty(instance->getClassName(), propId, serverBasedProperty))
            {
                if (serverBasedProperty->needSync)
                {
                    // they don't match, skip it..
                    skipProperty = true;
                }
            }
            else
            {
                RBXASSERT(false);
            }
        }
        else
        {
            skipProperty = true;
        }
    }

    if (skipProperty)
    {
        const Name* className = &Name::getNullName();

        if (instance) // instance could be NULL because the legacy client might not recognize the class and have not created the instance
        {
            className = &instance->getClassName();
        }
        else
        {
            InstanceClassMap::const_iterator findIter = serverInstanceClassMap.find(id);
            if (findIter != serverInstanceClassMap.end())
            {
                className = &findIter->second->name;
            }
            else
            {
                // This must be a null instance with a known class, we proceed with regular deserialization
                return false;
            }
        }
        shared_ptr<ReflectionPropertyContainer> serverBasedProperty;
        if (getServerBasedProperty(*className, propId, serverBasedProperty))
        {
            skipChangedProperty(inBitstream, serverBasedProperty);
            return true;
        }
        else
        {
            RBXASSERT(false);
        }
    }
    return false;
}

bool ClientReplicator::ProcessOutdatedProperties(RakNet::BitStream& inBitstream, Instance* instance, PropertyCacheType cacheType, bool useDictionary, bool preventBounceBack, std::vector<PropValuePair>* valueArray)
{
    bool cacheable = (cacheType != PropertyCacheType_NonCacheable);

    if (!(*(instance->getDescriptor().isOutdated)))
    {
        // if the class is not outdated, its properties won't be too because the class checksum is an accumulation of its property and event checksums
        return false;
    }

    ReflectionClassMap::const_iterator classIter = serverClasses.find(&instance->getClassName());

    if (classIter != serverClasses.end())
    {
        const shared_ptr<ReflectionClassContainer>& reflectionClass = classIter->second;

        if (reflectionClass->needSync == true)
        {
            // the content of this class has changed, we need use server scheme to deserialize the properties
            ReflectionPropertyList::const_iterator propIter = reflectionClass->properties.begin();
            for (; propIter != reflectionClass->properties.end(); propIter++)
            {
                const shared_ptr<ReflectionPropertyContainer>& reflectionProperty = *propIter;
                if (!reflectionProperty->canReplicate)
                {
                    continue;
                }
                if (reflectionProperty->needSync == true)
                {
					if ((cacheType == PropertyCacheType_All || isPropertyCacheable(reflectionProperty->type, reflectionProperty->enumMSB > 0) == cacheable)
                        && (reflectionProperty->name != Instance::propParent.name)) // don't read parent for now
                    {
                        // skip the new/changed properties
                        skipPropertiesInternal(reflectionProperty, inBitstream, useDictionary);
                    }
                }
                else
                {
                    // for each recognizable property, we iterate through the instance property collection and set the value
                    RBX::Reflection::PropertyIterator iter = instance->properties_begin();
                    RBX::Reflection::PropertyIterator end = instance->properties_end();
                    while (iter!=end)
                    {
                        Reflection::Property property = *iter;
                        const Reflection::PropertyDescriptor& descriptor = property.getDescriptor();
                        if (descriptor.name == reflectionProperty->name)
                        {
                            if (descriptor.canReplicate()
                                && (cacheType == PropertyCacheType_All || isPropertyCacheable(descriptor.type) == cacheable)
                                && (!cacheable || descriptor != Instance::propParent)) // don't read parent if it's cacheable (PropertyCacheType_Cacheable) or potentially cacheable (PropertyCacheType_All)
                            {
                                if (valueArray)
                                {
                                    Reflection::Variant value;
                                    readPropertiesInternal(property, inBitstream, useDictionary, preventBounceBack, &value);
                                    if (!value.isVoid())
                                        valueArray->push_back(PropValuePair(property.getDescriptor(), value));
                                }
                                else
                                    readPropertiesInternal(property, inBitstream, useDictionary, preventBounceBack, NULL);
                            }
                            break;
                        }
                        ++iter;
                    }
                }
            }

            return true;
        }
        else
        {
            // client schema is same as server's, why are we here?
            RBXASSERT(false);
        }
    }
	else
	{
		// we got the object from the server, how can it not be in the map?
		RBXASSERT(false);
	}

    return false;
}

bool ClientReplicator::ProcessOutdatedInstance(RakNet::BitStream& inBitstream, bool isJoinData, const RBX::Guid::Data& id, const Reflection::ClassDescriptor* classDescriptor, unsigned int classId)
{
#ifdef NETWORK_DEBUG
    if (FFlag::DebugProtocolSynchronization)
    {
        // This is to simulate class adding scenario
        if (classDescriptor && classDescriptor->name.toString() == "VehicleSeat")
        {
            StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Simulating class adding on %s", classDescriptor->name.c_str());
            classDescriptor = NULL;
        }
    }
#endif

    if (FFlag::DebugProtocolSynchronization)
    {
        if (classDescriptor && classDescriptor->name.toString() == "TheNewClass")
        {
            classDescriptor = NULL;
        }
    }

    if (classDescriptor == NULL)
    {
        // we don't have the class definition locally, let's read through the instance creation message and do nothing
        ReflectionClassMap::const_iterator classIter = serverClasses.begin();
        for (; classIter != serverClasses.end(); classIter++)
        {
            const shared_ptr<ReflectionClassContainer>& reflectionClass = classIter->second;
            if (reflectionClass->id == classId)
            {
                // we found the class defined at the server side
                // let's map the instance GUID to the class definition so we can deserialize property changes about this instance properly
                serverInstanceClassMap.insert(std::pair<RBX::Guid::Data, shared_ptr<ReflectionClassContainer> >(id, reflectionClass));

                if (settings().printInstances) {
                    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_SENSITIVE, 
                        "Skipping: %s:%s << %s",								// note: remove player always on the left
                        reflectionClass->name.c_str(), 
                        id.readableString().c_str(), 
                        RakNetAddressToString(remotePlayerId).c_str()
                        );
                }
                bool deleteOnDisconnect;
                inBitstream >> deleteOnDisconnect;
                RBX::Guid::Data parentId;
                if (!isJoinData)
                {
                    // read all non-cachable properties first
                    ReflectionPropertyList::const_iterator propIter = reflectionClass->properties.begin();
                    for (; propIter != reflectionClass->properties.end(); propIter++)
                    {
                        const shared_ptr<ReflectionPropertyContainer> reflectionProp = *propIter;
                        const Reflection::Type* propType;
                        typeDictionary.getValue(reflectionProp->typeId, propType);
                        if (reflectionProp->canReplicate & !isPropertyCacheable(*propType))
                        {
                            skipPropertiesInternal(reflectionProp, inBitstream, true);
                        }
                    }

                    // read all cachable properties except for parent
                    propIter = reflectionClass->properties.begin();
                    for (; propIter != reflectionClass->properties.end(); propIter++)
                    {
                        const shared_ptr<ReflectionPropertyContainer> reflectionProp = *propIter;
                        const Reflection::Type* propType;
                        typeDictionary.getValue(reflectionProp->typeId, propType);
                        if (reflectionProp->canReplicate & isPropertyCacheable(*propType))
                        {
                            // Parent are never sent here
                            if (reflectionProp->name != Instance::propParent.name.toString())
                            {
                                skipPropertiesInternal(reflectionProp, inBitstream, true);
                            }
                        }
                    }

                    deserializeId(inBitstream, parentId);
                }
                else
                {
                    // read all properties except for parent
                    ReflectionPropertyList::const_iterator propIter = reflectionClass->properties.begin();
                    for (; propIter != reflectionClass->properties.end(); propIter++)
                    {
                        const shared_ptr<ReflectionPropertyContainer> reflectionProp = *propIter;
                        const Reflection::Type* propType;
                        typeDictionary.getValue(reflectionProp->typeId, propType);
                        if (reflectionProp->canReplicate)
                        {
                            // don't write the parent yet
                            if (reflectionProp->name != Instance::propParent.name.toString())
                            {
                                skipPropertiesInternal(reflectionProp, inBitstream, false);
                            }
                        }
                    }
                    deserializeIdWithoutDictionary(inBitstream, parentId);
                }
                break;
            }
        }
        return true;
    }
    return false;
}

bool ClientReplicator::ProcessOutdatedEventInvocation(RakNet::BitStream& inBitstream, const RBX::Guid::Data& id, const Instance* instance, const Reflection::EventDescriptor* eventDescriptor, unsigned int eventId)
{
    if (instance && !(*(instance->getDescriptor().isOutdated)))
    {
        // if the class is not outdated, its events won't be too because the class checksum is an accumulation of its property and event checksums
        return false;
    }

    bool skipEvent = false;
    if (eventDescriptor == NULL)
    {
        // we don't have the event definition locally, let's skip it
        skipEvent = true;
    }
    else
    {
        if (instance)
        {
            // do client and server agree on the event attributes?
            shared_ptr<ReflectionEventContainer> serverBasedEvent;
            if (getServerBasedEvent(instance->getClassName(), eventId, serverBasedEvent))
            {
                if (serverBasedEvent->needSync)
                {
                    skipEvent = true;
                }
            }
            else
            {
                RBXASSERT(false);
            }
        }
        else
        {
            skipEvent = true;
        }
    }
    if (skipEvent)
    {
        const Name* className = &Name::getNullName();

        if (instance)
        {
            className = &instance->getClassName();
        }
        else
        {
            InstanceClassMap::const_iterator findIter = serverInstanceClassMap.find(id);
            if (findIter != serverInstanceClassMap.end())
            {
                className = &findIter->second->name;
            }
            else
            {
                RBXASSERT(false);
            }
        }
        shared_ptr<ReflectionEventContainer> serverBasedEvent;
        if (getServerBasedEvent(*className, eventId, serverBasedEvent))
        {
            skipEventInvocation(inBitstream, serverBasedEvent);
            return true;
        }
        else
        {
            RBXASSERT(false);
        }
    }
    return false;
}

bool ClientReplicator::ProcessOutdatedEnumSerialization(const Reflection::Type& type, const Reflection::Variant& value, RakNet::BitStream& outBitStream)
{
    if (const Reflection::EnumDescriptor* enumDesc = Reflection::EnumDescriptor::lookupDescriptor(type))
    {
        size_t newMSB = 0;
        if (hasEnumChanged(*enumDesc, newMSB))
        {
            // the enum definition has changed at server side, let's use server schema to serialize
            serializeEnum(enumDesc, value, outBitStream, newMSB);
            return true;
        }
    }
    return false;
}

bool ClientReplicator::ProcessOutdatedEnumDeserialization(RakNet::BitStream& inBitStream, const Reflection::Type& type, Reflection::Variant& value)
{
    size_t newMSB = 0;
    if (const Reflection::EnumDescriptor* enumDesc = Reflection::EnumDescriptor::lookupDescriptor(type))
    {
        if (hasEnumChanged(*enumDesc, newMSB))
        {
            // the enum definition has changed at server side, let's use server schema to deserialize
            // Note, we try to be very tolerant to enum changes so adding a new value at the end of enum will still work most of the time
            // However, it will break if:
            // 1) enum values get removed
            // 2) order of enum values gets changed
            // Also, overflowed new enum values will be discarded on old clients
            deserializeEnum(enumDesc, value, inBitStream, newMSB);
            return true;
        }
    }
    else
    {
        // we can't find the enum locally, skip the value using server schema
        auto enumIter = serverEnums.find(&type.name);

        if (enumIter != serverEnums.end())
        {
            inBitStream.IgnoreBits(enumIter->second.enumMSB+1);
            return true;
        }

        RBXASSERT(false);
    }
    return false;
}

bool ClientReplicator::ProcessOutdatedPropertyEnumSerialization(const Reflection::ConstProperty& property, RakNet::BitStream& outBitStream)
{
    const EnumPropertyDescriptor& enumDesc = static_cast<const EnumPropertyDescriptor&>(property.getDescriptor());
    size_t newMSB = 0;
    if (hasEnumChanged(enumDesc.enumDescriptor, newMSB))
    {
        // the enum definition has changed at server side, let's use server schema to serialize
        serializeEnumProperty(property, outBitStream, newMSB);
        return true;
    }
    return false;
}

bool ClientReplicator::ProcessOutdatedPropertyEnumDeserialization(Reflection::Property& property, RakNet::BitStream& inBitStream)
{
    size_t newMSB = 0;
    const EnumPropertyDescriptor& enumDesc = static_cast<const EnumPropertyDescriptor&>(property.getDescriptor());
    if (hasEnumChanged(enumDesc.enumDescriptor, newMSB))
    {
        // the enum definition has changed at server side, let's use server schema to deserialize
        // Note, we try to be very tolerant to enum changes so adding a new value at the end of enum will still work most of the time
        // However, it will break if:
        // 1) enum values get removed
        // 2) order of enum values gets changed
        // Also, overflowed new enum values will be discarded on old clients
        deserializeEnumProperty(property, inBitStream, newMSB);
        return true;
    }
    return false;
}

void ClientReplicator::skipPropertyValue(RakNet::BitStream& inBitStream, const shared_ptr<ClientReplicator::ReflectionPropertyContainer>& prop, bool useDictionary)
{
    const Reflection::Type& type = *prop->type;
	if (prop->enumMSB > 0) // this is an enum property
	{
		int value = 0;
		readFastN( inBitStream, value, prop->enumMSB+1 );
	}
	else if (type == Reflection::Type::singleton<RBX::ProtectedString>())
    {
        BinaryString strValue;

        if (useDictionary)
            getSharedPropertyBinaryDictionaryById(prop->id).deserializeString(strValue, inBitStream);
        else
            inBitStream >> strValue;
    }
    else if (type == Reflection::Type::singleton<std::string>())
    {
        std::string strValue;
        if (useDictionary)
        {
            getSharedPropertyDictionaryById(prop->id).deserializeString(strValue, inBitStream);
        }
        else
        {
            inBitStream >> strValue;
        }
    }
    else if (type==Reflection::Type::singleton<BinaryString>())
    {
        BinaryString strValue;
        inBitStream >> strValue;
    }
    else if (type==Reflection::Type::singleton<bool>())
    {
        bool dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<int>())
    {
        int dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<float>())
    {
        float dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<double>())
    {
        double dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<UDim>())
    {
        UDim dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<UDim2>())
    {
        UDim2 dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<RBX::RbxRay>())
    {
        RBX::RbxRay dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<Faces>())
    {
        Faces dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<Axes>())
    {
        Axes dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<BrickColor>())
    {
        BrickColor dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<G3D::Color3>())
    {
        G3D::Color3 dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<G3D::Vector2>())
    {
        G3D::Vector2 dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<G3D::Vector3>())
    {
        if (prop->name == PartInstance::prop_Size.name.toString())
        {
            G3D::Vector3 value;
            readBrickVector(inBitStream, value);
        }
        else
        {
            G3D::Vector3 dummy;
            inBitStream >> dummy;
        }
    }
    else if (type==Reflection::Type::singleton<G3D::Vector2int16>())
    {
        G3D::Vector2int16 dummy;
        inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<G3D::CoordinateFrame>())
    {
        G3D::CoordinateFrame dummy;
        inBitStream >> dummy;
    }
    else if (Reflection::RefPropertyDescriptor::isRefPropertyDescriptor(type))
    {
        RBX::Guid::Data id;
        if (useDictionary)
        {
            deserializeId(inBitStream, id);
        }
        else
        {
            deserializeIdWithoutDictionary(inBitStream, id);
        }
    }
    else if (type==Reflection::Type::singleton<RBX::ContentId>())
    {
        RBX::ContentId dummy;
		if (useDictionary)
			contentIdDictionary.receive(inBitStream, dummy);
		else
			inBitStream >> dummy;
    }
    else if (type==Reflection::Type::singleton<RBX::SystemAddress>())
    {
        RBX::SystemAddress value;
        if (useDictionary)
            systemAddressDictionary.receive(inBitStream, value);
        else
            inBitStream >> value;
    }
	else if (type==Reflection::Type::singleton<Rect2D>())
	{
		Rect2D dummy;
		inBitStream >> dummy;
	}
	else if (type==Reflection::Type::singleton<PhysicalProperties>())
	{
		PhysicalProperties dummy;
		inBitStream >> dummy;
	}
    else
    {
        StandardOut::singleton()->printf(MESSAGE_ERROR, "A new property type is not supported on this legacy client. Please flag off the replication before client and server are synchronized");
        RBXASSERT(false);
    }
}

void ClientReplicator::skipPropertiesInternal(const shared_ptr<ClientReplicator::ReflectionPropertyContainer>& prop, RakNet::BitStream& inBitstream, bool useDictionary)
{
    // in line with readPropertiesInternal
    bool boolValue;
    if (prop->type && prop->type->isType<bool>())
    {
        inBitstream >> boolValue;
        if (settings().printBits) {
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, 
                "   skip %s %s, 1 bit", 
                prop->typeName.c_str(),
                prop->name.c_str()
                );
        }
    }
    else
    {
        bool isDefault;
        inBitstream >> isDefault;
        if (isDefault)
        {
            if (settings().printBits) {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, 
                    "   skip %s %s, 1 bit (default)", 
                    prop->typeName.c_str(),
                    prop->name.c_str()
                    );
            }
        }
        else
        {
            const int start = inBitstream.GetReadOffset();

            skipPropertyValue(inBitstream, prop, useDictionary);

            if (settings().printBits) {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, 
                    "   skip %s %s, %d bits", 
                    prop->typeName.c_str(),
                    prop->name.c_str(),
                    inBitstream.GetReadOffset() - start + 1
                    );
            }
        }
    }
}

void ClientReplicator::skipChangedProperty(RakNet::BitStream& bitStream, const shared_ptr<ClientReplicator::ReflectionPropertyContainer>& prop)
{
    bool versionReset;
    bitStream >> versionReset;
    if (prop->name == Instance::propParent.name.toString())
    {
        RBX::Guid::Data parentId;
        deserializeId(bitStream, parentId);
    }
    else
    {
        skipPropertyValue(bitStream, prop, true/*useDictionary*/);
    }
}

void ClientReplicator::skipEventInvocation(RakNet::BitStream& bitStream, const shared_ptr<ReflectionEventContainer>& event)
{
    int numArgs;
    bitStream >> numArgs;

    for (int i=0; i<numArgs; i++)
    {
        const Reflection::Type& type = *event->argTypes[i];
        Reflection::Variant argument;
        if(type == Reflection::Type::singleton<std::string>())
        {
            std::string value;
            getSharedEventDictionaryById(event->id).deserializeString(value, bitStream);
        }
        else{
            deserializeValue(bitStream, type, argument);
        }
    }
}

bool ClientReplicator::getServerBasedProperty(const Name& className, int propertyId, shared_ptr<ClientReplicator::ReflectionPropertyContainer>& serverBasedProperty)
{
    ReflectionClassMap::const_iterator classIter = serverClasses.find(&className);

    if (classIter != serverClasses.end())
    {
        const shared_ptr<ReflectionClassContainer>& reflectionClass = classIter->second;

        ReflectionPropertyList::const_iterator propIter = reflectionClass->properties.begin();
        for (; propIter != reflectionClass->properties.end(); propIter++)
        {
            const shared_ptr<ReflectionPropertyContainer>& reflectionProperty = *propIter;
            if (reflectionProperty->id == propertyId)
            {
                serverBasedProperty = reflectionProperty;
                return true;
            }
        }
    }

    return false;
}

bool ClientReplicator::getServerBasedEvent(const Name& className, int eventId, shared_ptr<ClientReplicator::ReflectionEventContainer>& serverBasedEvent)
{
    ReflectionClassMap::const_iterator classIter = serverClasses.find(&className);

    if (classIter != serverClasses.end())
    {
        const shared_ptr<ReflectionClassContainer>& reflectionClass = classIter->second;

        ReflectionEventList::const_iterator eventIter = reflectionClass->events.begin();
        for (; eventIter != reflectionClass->events.end(); eventIter++)
        {
            const shared_ptr<ReflectionEventContainer>& reflectionEvent = *eventIter;
            if (reflectionEvent->id == eventId)
            {
                serverBasedEvent = reflectionEvent;
                return true;
            }
        }
    }

    return false;
}

bool ClientReplicator::isClassRemoved(const Instance* instance)
{
#ifdef NETWORK_DEBUG
    if (FFlag::DebugProtocolSynchronization)
    {
        // This is to simulate class removal scenario
        if (instance->getClassNameStr() == "TrussPart")
        {
            StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Simulating class removal TrussPart");
            return true;
        }
    }
#endif

    return !(*(instance->getDescriptor().isReplicable));
}

bool ClientReplicator::isPropertyRemoved(const Instance* instance, const Name& propertyName)
{
#ifdef NETWORK_DEBUG
    if (FFlag::DebugProtocolSynchronization)
    {
        // This is to simulate property removal scenario
        if (instance->getClassNameStr() == "WedgePart" && propertyName == "Material")
        {
            StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Simulating property removal on WedgePart.Material");
            return true;
        }
    }
#endif
    if (!(*(instance->getDescriptor().isOutdated)))
    {
        // the class is not outdated, this means nothing has been changed in the class
        return false;
    }

    ReflectionClassMap::const_iterator classIter = serverClasses.find(&instance->getClassName());

    if (classIter != serverClasses.end())
    {
        const shared_ptr<ReflectionClassContainer>& reflectionClass = classIter->second;

        ReflectionPropertyList::const_iterator propIter = reflectionClass->properties.begin();
        for (; propIter != reflectionClass->properties.end(); propIter++)
        {
            const shared_ptr<ReflectionPropertyContainer>& reflectionProperty = *propIter;
            if (reflectionProperty->name == propertyName)
            {
                return false;
            }
        }
    }

    // we can't find the property in server class schema, it should have been removed in server code
    return true;
}

bool ClientReplicator::isEventRemoved(const Instance* instance, const Name& eventName)
{
#ifdef NETWORK_DEBUG
    if (FFlag::DebugProtocolSynchronization)
    {
        // This is to simulate event removal scenario
        if (instance->getClassNameStr() == "Tool" && eventName == "Activated")
        {
            StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Simulating event removal on Tool.Activated");
            return true;
        }
    }
#endif

    if (!(*(instance->getDescriptor().isOutdated)))
    {
        // the class is not outdated, this means nothing has been changed in the class
        return false;
    }

    ReflectionClassMap::const_iterator classIter = serverClasses.find(&instance->getClassName());

    if (classIter != serverClasses.end())
    {
        const shared_ptr<ReflectionClassContainer>& reflectionClass = classIter->second;

        ReflectionEventList::const_iterator eventIter = reflectionClass->events.begin();
        for (; eventIter != reflectionClass->events.end(); eventIter++)
        {
            const shared_ptr<ReflectionEventContainer>& reflectionEvent = *eventIter;
            if (reflectionEvent->name == eventName)
            {
                return false;
            }
        }
    }

    // we can't find the event in server class schema, it should have been removed in server code
    return true;
}

bool ClientReplicator::hasEnumChanged(const Reflection::EnumDescriptor& enumDesc, size_t& newMSB)
{
#ifdef NETWORK_DEBUG
    if (FFlag::DebugProtocolSynchronization)
    {
        // This is to simulate enum change scenario
        if (!strcmp(enumDesc.name.c_str(), "TeleportState") || !strcmp(enumDesc.name.c_str(), "Material"))
        {
            StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Simulating enum modification on %s", enumDesc.name.c_str());
            newMSB = enumDesc.getEnumCountMSB();
            return true;
        }
    }
#endif

    auto enumIter = serverEnums.find(&enumDesc.name);

    if (enumIter != serverEnums.end())
    {
        newMSB = enumIter->second.enumMSB;

        return newMSB != enumDesc.getEnumCountMSB();
    }

    // we can't find the enum in server enum schema, it should have been removed in server code
    return true;
}

SharedStringProtectedDictionary& ClientReplicator::getSharedPropertyProtectedDictionaryById(unsigned int propertyId)
{
    const Reflection::PropertyDescriptor* descriptor;
    propDictionary.getValue(propertyId, descriptor);
    if (descriptor)
    {
        return getSharedPropertyProtectedDictionary(*descriptor);
    }
    else
    {
        DummyPropertyProtectedStrings::iterator iter = dummyProtectedStrings.find(propertyId);
        if (iter==dummyProtectedStrings.end())
        {
            shared_ptr<SharedStringProtectedDictionary> result(new SharedStringProtectedDictionary(isProtectedStringEnabled()));
            dummyProtectedStrings[propertyId] = result;
            return *result;
        }
        return *iter->second;
    }
}

SharedStringDictionary& ClientReplicator::getSharedPropertyDictionaryById(unsigned int propertyId)
{
    const Reflection::PropertyDescriptor* descriptor;
    propDictionary.getValue(propertyId, descriptor);
    if (descriptor)
    {
        return getSharedPropertyDictionary(*descriptor);
    }
    else
    {
        DummyPropertyStrings::iterator iter = dummyStrings.find(propertyId);
        if (iter==dummyStrings.end())
        {
            shared_ptr<SharedStringDictionary> result(new SharedStringDictionary());
            dummyStrings[propertyId] = result;
            return *result;
        }
        return *iter->second;
    }
}

SharedStringDictionary& ClientReplicator::getSharedEventDictionaryById(unsigned int eventId)
{
    const Reflection::EventDescriptor* descriptor;
    eventDictionary.getValue(eventId, descriptor);
    if (descriptor)
    {
        return getSharedEventDictionary(*descriptor);
    }
    else
    {
        DummyEventStrings::iterator iter = dummyEventStrings.find(eventId);
        if (iter==dummyEventStrings.end())
        {
            shared_ptr<SharedStringDictionary> result(new SharedStringDictionary());
            dummyEventStrings[eventId] = result;
            return *result;
        }
        return *iter->second;
    }
}

SharedBinaryStringDictionary& ClientReplicator::getSharedPropertyBinaryDictionaryById(unsigned int propertyId)
{
    const Reflection::PropertyDescriptor* descriptor;
    propDictionary.getValue(propertyId, descriptor);
    if (descriptor)
    {
        return getSharedPropertyBinaryDictionary(*descriptor);
    }
    else
    {
        DummyPropertyBinaryStrings::iterator iter = dummyBinaryStrings.find(propertyId);
        if (iter==dummyBinaryStrings.end())
        {
            shared_ptr<SharedBinaryStringDictionary> result(new SharedBinaryStringDictionary());
            dummyBinaryStrings[propertyId] = result;
            return *result;
        }
        return *iter->second;
    }
}

bool ClientReplicator::isProtectedStringEnabled()
{
    return true;
}

std::string ClientReplicator::encodeProtectedString(const ProtectedString& value, const Instance* instance, const Reflection::PropertyDescriptor& desc)
{
    if (LuaVM::useSecureReplication())
        return value.getBytecode();
    else
        return value.getSource();
}

boost::optional<ProtectedString> ClientReplicator::decodeProtectedString(const std::string& value, const Instance* instance, const Reflection::PropertyDescriptor& desc)
{
    if (LuaVM::useSecureReplication())
        return ProtectedString::fromBytecode(value);
    else
        return ProtectedString::fromTrustedSource(value);
}

