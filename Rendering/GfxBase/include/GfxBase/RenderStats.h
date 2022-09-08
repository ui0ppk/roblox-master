/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */


#pragma once
#include "boost/scoped_ptr.hpp"
#include "util/Profiling.h"

namespace RBX { 

namespace Profiling
{
	class CodeProfiler;
}

struct RenderPassStats
{
	unsigned int batches;
	unsigned int faces;
	unsigned int vertices;
	unsigned int stateChanges;
	unsigned int passChanges;
	
	RenderPassStats()
		: batches(0)
		, faces(0)
		, vertices(0)
		, stateChanges(0)
		, passChanges(0)
	{
	}
	
	RenderPassStats& operator+=(const RenderPassStats& other)
	{
		batches += other.batches;
		faces += other.faces;
		vertices += other.vertices;
		stateChanges += other.stateChanges;
		passChanges += other.passChanges;
		
		return *this;
	}

	RenderPassStats operator+(const RenderPassStats& other) const
	{
		RenderPassStats result = *this;
        result += other;
        return result;
	}
};

struct ClusterStats
{
	unsigned int clusters;
	unsigned int parts;
	
	ClusterStats()
		: clusters(0)
		, parts(0)
	{
	}
};

class RenderStats {
public:
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> cpuRenderTotal;

	boost::scoped_ptr<RBX::Profiling::CodeProfiler>	culling;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> flip;
    boost::scoped_ptr<RBX::Profiling::CodeProfiler> renderObjects;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> updateLighting;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> adorn2D;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> adorn3D;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> visualEngineSceneUpdater;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> finishRendering;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> renderTargetUpdate;

	boost::scoped_ptr<RBX::Profiling::CodeProfiler>	frameRateManager;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> textureCompositor;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> updateSceneGraph;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> updateAllInvalidParts;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> updateDynamicsAndAggregateStatics;
	boost::scoped_ptr<RBX::Profiling::CodeProfiler> updateDynamicParts;
	
	RenderPassStats		passTotal;
	RenderPassStats		passScene;
	RenderPassStats		passShadow;
	RenderPassStats		passUI;
	RenderPassStats		pass3DAdorns;
	
	ClusterStats		clusterFast;
	ClusterStats		clusterFastFW;
	ClusterStats		clusterFastHumanoid;

	ClusterStats		lastFrameFast;
	unsigned			lastFrameMegaClusterChunks;

	RenderStats();
	~RenderStats();
};

}
