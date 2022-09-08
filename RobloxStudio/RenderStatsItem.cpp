/**
 * RenderStatsItem.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RenderStatsItem.h"
#include "rbx/SystemUtil.h"

// Roblox Headers
#include "util/Profiling.h"

RenderStatsItem::RenderStatsItem(const RBX::RenderStats& renderStats)
: m_renderStats(renderStats)
{
}

boost::shared_ptr<RenderStatsItem> RenderStatsItem::create(const RBX::RenderStats& renderStats)
{
	boost::shared_ptr<RenderStatsItem> pResult = RBX::Creatable<RBX::Instance>::create<RenderStatsItem, const RBX::RenderStats&>(renderStats);
	
	RBX::Stats::Item* pItem3D = pResult->createBoundChildItem(*(renderStats.cpuRenderTotal));
	
	pItem3D->createBoundChildItem(*(renderStats.culling));
	pItem3D->createBoundChildItem(*(renderStats.flip));
	pItem3D->createBoundChildItem(*(renderStats.renderObjects));

	pItem3D->createBoundChildItem(*(renderStats.updateLighting));
	pItem3D->createBoundChildItem(*(renderStats.adorn2D));
	pItem3D->createBoundChildItem(*(renderStats.adorn3D));
	pItem3D->createBoundChildItem(*(renderStats.visualEngineSceneUpdater));
	pItem3D->createBoundChildItem(*(renderStats.finishRendering));
	pItem3D->createBoundChildItem(*(renderStats.renderTargetUpdate));
	pItem3D->createBoundChildItem(*(renderStats.updateDynamicParts));
	pItem3D->createBoundChildItem(*(renderStats.frameRateManager));

	RBX::Stats::Item* updateScene = pItem3D->createBoundChildItem(*(renderStats.updateSceneGraph));
	updateScene->createBoundChildItem(*(renderStats.updateAllInvalidParts));
	updateScene->createBoundChildItem(*(renderStats.updateDynamicsAndAggregateStatics));
	
	RBX::Stats::Item* pMem = pResult->createChildItem("Memory");
	pResult->m_videoMemory = pMem->createChildItem("Video");

	RBX::Stats::Item* pStateChanges = pResult->createChildItem("State Changes");
	pResult->m_majorStateChanges = pStateChanges->createChildItem("Major");
	pResult->m_minorStateChanges = pStateChanges->createChildItem("Minor");

	return pResult;
}

void RenderStatsItem::update()
{
	RBX::Stats::Item::update();

    m_videoMemory->formatMem(size_t(RBX::SystemUtil::getVideoMemory()));
}

void RenderStatsItem::addBucketToItem(RBX::Stats::Item* item, const RBX::Profiling::BucketProfile& bucketProfile)
{
	const std::vector<int>& data = bucketProfile.getData();
	const int* bucketLimits = bucketProfile.getLimits();

	char temp[50];
	sprintf_s(temp, 50, "Less than %u", bucketLimits[0]);
	item->createBoundChildItem(temp, data[0]);
	for(unsigned i = 1; i < data.size()-1; i++)
	{
		sprintf_s(temp, 50, "%u to %u", bucketLimits[i-1], bucketLimits[i]);
		item->createBoundChildItem(temp, data[i]);
	}
	sprintf_s(temp, 50, "More than %u", bucketLimits[data.size()-2]);
	item->createBoundChildItem(temp, data[data.size()-1]);
}
