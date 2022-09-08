/*
 *  RenderStatsItem.h
 *  RobloxStudio
 *  Copied from old Roblox app directory
 *
 *  Created by Vinod on 10/18/11.
 *  Copyright 2011 Roblox. All rights reserved.
 *
 */
#pragma once

#include "V8Datamodel/Stats.h"
#include "GfxBase/RenderStats.h"
#include "GfxBase/FrameRateManager.h"
#include "boost/shared_ptr.hpp"

namespace RBX
{
	namespace Stats
	{
		class Item;
		class StatsService;
	}
	namespace Profiling
	{
		class BucketProfile;
	}
}

class RenderStatsItem : public RBX::Stats::Item
{
public:
	RenderStatsItem(const RBX::RenderStats& renderStats);
	void update();

	static boost::shared_ptr<RenderStatsItem> create(const RBX::RenderStats& renderStats);
	static void addBucketToItem(RBX::Stats::Item* item, const RBX::Profiling::BucketProfile& bucketProfile);

private:
	RBX::Stats::Item* m_videoMemory;
	RBX::Stats::Item* m_majorStateChanges;
	RBX::Stats::Item* m_minorStateChanges;
	const RBX::RenderStats& m_renderStats;
};
