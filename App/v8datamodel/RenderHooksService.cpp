#include "stdafx.h"

#include "V8DataModel/RenderHooksService.h"
#include "Network/Players.h"
#include "V8Xml/WebParser.h"
#include "Util/Http.h"
#include "Util/Profiling.h"
#include "rbx/Log.h"

namespace RBX
{
	const char* const sRenderHooksService = "RenderHooksService";

    REFLECTION_BEGIN();
	static Reflection::BoundFuncDesc<RenderHooksService, void()> reloadShaders(&RenderHooksService::reloadShaders, "ReloadShaders", Security::LocalUser);
	static Reflection::BoundFuncDesc<RenderHooksService, void(int)> enableQueue(&RenderHooksService::enableQueue, "EnableQueue", "qId", Security::LocalUser);
	static Reflection::BoundFuncDesc<RenderHooksService, void(int)> disableQueue(&RenderHooksService::disableQueue, "DisableQueue", "qId", Security::LocalUser);
	static Reflection::BoundFuncDesc<RenderHooksService, void()> printMetrics(&RenderHooksService::captureMetrics, "CaptureMetrics", Security::LocalUser);
	static Reflection::BoundFuncDesc<RenderHooksService, void(int, int)> resizeWindow(&RenderHooksService::resizeWindow, "ResizeWindow", "width", "height", Security::LocalUser);

	static Reflection::BoundFuncDesc<RenderHooksService, double(void)> getPresentTime(&RenderHooksService::getPresentTime, "GetPresentTime", Security::LocalUser);
	static Reflection::BoundFuncDesc<RenderHooksService, double(void)> getGPUDelay(&RenderHooksService::getGPUDelay, "GetGPUDelay", Security::LocalUser);
	static Reflection::BoundFuncDesc<RenderHooksService, double(void)> getRenderAve(&RenderHooksService::getRenderAve, "GetRenderAve", Security::LocalUser);
	static Reflection::BoundFuncDesc<RenderHooksService, double(void)> getRenderConfMin(&RenderHooksService::getRenderConfMin, "GetRenderConfMin", Security::LocalUser);
	static Reflection::BoundFuncDesc<RenderHooksService, double(void)> getRenderConfMax(&RenderHooksService::getRenderConfMax, "GetRenderConfMax", Security::LocalUser);
	static Reflection::BoundFuncDesc<RenderHooksService, double(void)> getRenderStd(&RenderHooksService::getRenderStd, "GetRenderStd", Security::LocalUser);

	static Reflection::BoundFuncDesc<RenderHooksService, double(void)> getDeltaAve(&RenderHooksService::getDeltaAve, "GetDeltaAve", Security::LocalUser);

	static Reflection::BoundFuncDesc<RenderHooksService, void(bool)> enableAdorns(&RenderHooksService::enableAdorns, "EnableAdorns", "enabled", Security::LocalUser);

	static Reflection::BoundFuncDesc<RenderHooksService, void(void)> printScene(&RenderHooksService::printScene, "PrintScene", Security::LocalUser);
    REFLECTION_END();


	RenderHooksService::RenderHooksService() :
		wndHooks(NULL)
	{
		setName(sRenderHooksService);
		memset(&metrics, 0, sizeof(metrics));
	}

	void RenderHooksService::resizeWindow(int cx, int cy)
	{
		if(wndHooks)
			wndHooks->resizeWindow(cx, cy);
	}

	void RenderHooksService::captureMetrics()
	{
		if(renderHooks)
			renderHooks->captureMetrics(metrics);
	}

	void RenderHooksService::printScene()
	{
		if(renderHooks)
			renderHooks->printScene();
	}

	void RenderHooksService::enableAdorns(bool enabled)
	{
		if(renderHooks)
			renderHooks->enableAdorns(enabled);
	}
}
