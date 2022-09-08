/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */


#include "GfxBase/RenderStats.h"
#include "Util/Profiling.h"

using namespace RBX;

RenderStats::RenderStats() :
	cpuRenderTotal(new RBX::Profiling::CodeProfiler("3D CPU Total"))

	, culling(new RBX::Profiling::CodeProfiler("Culling"))
	, flip(new RBX::Profiling::CodeProfiler("Flipping Backbuffer"))
	, renderObjects(new RBX::Profiling::CodeProfiler("Render Objects"))
	, updateLighting(new RBX::Profiling::CodeProfiler("Update Lighting"))
	, adorn2D(new RBX::Profiling::CodeProfiler("Adorn 2D"))
	, adorn3D(new RBX::Profiling::CodeProfiler("Adorn 3D"))
	, visualEngineSceneUpdater(new RBX::Profiling::CodeProfiler("Visual Engine Scene Updater"))
   	, finishRendering(new RBX::Profiling::CodeProfiler("Finish Rendering"))
	, renderTargetUpdate(new RBX::Profiling::CodeProfiler("RenderTarget Update"))

	, frameRateManager(new RBX::Profiling::CodeProfiler("Frame Rate Manager"))

	, textureCompositor(new RBX::Profiling::CodeProfiler("Texture Compositor"))
	, updateSceneGraph(new RBX::Profiling::CodeProfiler("Update SceneGraph"))
	, updateAllInvalidParts(new RBX::Profiling::CodeProfiler("updateAllInvalidParts"))
	, updateDynamicsAndAggregateStatics(new RBX::Profiling::CodeProfiler("updateDynamicsAndAggregateStatics"))
	, updateDynamicParts(new RBX::Profiling::CodeProfiler("updateDynamicParts"))
{
	
}

RenderStats::~RenderStats()
{
}