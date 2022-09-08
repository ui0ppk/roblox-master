#include "GamePerfMonitor.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/Stats.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/TimerService.h"
#include "v8world/World.h"
#include "util/RunStateOwner.h"

#include "Network/Players.h"

using namespace RBX;


FASTINTVARIABLE(GamePerfMonitorPercentage,2)
FASTINTVARIABLE(GamePerfMonitorReportTimer, 10) // minutes

GamePerfMonitor::~GamePerfMonitor()
{
}

void GamePerfMonitor::collectAndPostStats(boost::weak_ptr<DataModel> dataModel)
{
	boost::shared_ptr<DataModel> sharedDM = dataModel.lock(); 
	if (!sharedDM)
		return;

	if (sharedDM->isClosed())
		return;

    postDiagStats(sharedDM);

	if (TimerService* timer = sharedDM->create<TimerService>()) {
		timer->delay(boost::bind(&GamePerfMonitor::collectAndPostStats, this, dataModel), FInt::GamePerfMonitorReportTimer*60);
	}	
}

void GamePerfMonitor::postDiagStats(boost::shared_ptr<DataModel> dataModel)
{
	if (!shouldPostDiagStats)
		return;

	Stats::StatsService* statsService = dataModel->find<Stats::StatsService>();
	if (statsService)
	{
		if (Instance* networkStats = statsService->findFirstChildByName("Network"))
		{
			if (networkStats->numChildren() > 1)
			{
				// child 0 is "Packet Thread"
				Instance* stats = networkStats->getChild(1);

				if (Stats::Item* ping = Instance::fastDynamicCast<Stats::Item>(stats->findFirstChildByName("Data Ping")))
					Analytics::EphemeralCounter::reportStats("ClientDataPing", ping->getValue());

				if (Stats::Item* raknetPing = Instance::fastDynamicCast<Stats::Item>(stats->findFirstChildByName("Ping")))
					Analytics::EphemeralCounter::reportStats("ClientPing", raknetPing->getValue());

				if (Stats::Item* packetLossPercent = Instance::fastDynamicCast<Stats::Item>(stats->findFirstChildByName("MaxPacketLoss")))
					Analytics::EphemeralCounter::reportStats("ClientPacketLossPercent_"+DebugSettings::singleton().osPlatform(), packetLossPercent->getValue());
			}
		}

		if (Instance* workspaceStats = statsService->findFirstChildByName("Workspace"))
		{
			if (Stats::Item* fps = Instance::fastDynamicCast<Stats::Item>(workspaceStats->findFirstChildByName("FPS")))
				Analytics::EphemeralCounter::reportStats("ClientPhysicsFPS", fps->getValue());

			if (Stats::Item* envSpeed = Instance::fastDynamicCast<Stats::Item>(workspaceStats->findFirstChildByName("Environment Speed %")))
				Analytics::EphemeralCounter::reportStats("ClientPhysicsEnvSpeed", envSpeed->getValue());
		}

		if (!frmStats)
			frmStats = shared_from(statsService->findFirstChildByName("FrameRateManager"));

		if (frmStats)
		{
			if (Stats::Item* fps = Instance::fastDynamicCast<Stats::Item>(frmStats->findFirstChildByName("AverageFPS")))
			{
				Analytics::EphemeralCounter::reportStats("ClientFPS", fps->getValue());
				Analytics::EphemeralCounter::reportStats("ClientFPS_"+DebugSettings::singleton().osPlatform(), fps->getValue());
			}
		}
	}
}

void GamePerfMonitor::onGameClose(boost::weak_ptr<DataModel> dataModel)
{
	boost::shared_ptr<DataModel> sharedDM = dataModel.lock();
	if (!sharedDM)
		return;

	if (frmStats)
	{
		if (Stats::Item* fps = Instance::fastDynamicCast<Stats::Item>(frmStats->findFirstChildByName("AverageFPS")))
		{
			Analytics::EphemeralCounter::reportStats("ClientFPS", fps->getValue());
			Analytics::EphemeralCounter::reportStats("ClientFPS_"+DebugSettings::singleton().osPlatform(), fps->getValue());
		}
	}
}

void GamePerfMonitor::start(DataModel* dataModel)
{
	if (!dataModel)
		return;

	Stats::StatsService* statsService = dataModel->create<Stats::StatsService>();
	if (statsService)
		frmStats = shared_from(statsService->findFirstChildByName("FrameRateManager"));

	bool shouldPost = (abs(userId) % 100) < FInt::GamePerfMonitorPercentage;
	dataModel->setJobsExtendedStatsWindow(30);

	if (shouldPost)
	{
		if (TimerService* timer = dataModel->create<TimerService>()) {
			timer->delay(boost::bind(&GamePerfMonitor::collectAndPostStats, this, weak_from(dataModel)), 60);
		}
	}

	if (shouldPost)
	{
		dataModel->closingSignal.connect(boost::bind(&GamePerfMonitor::onGameClose, this, weak_from(dataModel)));
	}
}



