#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/function.hpp>

#include "rbx/Declarations.h"
#include "GfxBase/RenderSettings.h"

namespace RBX {

class DataModel;
class ViewBase;
class FrameRateManager;
class CRenderSettings;
class RenderStats;
class RBXInterface IMetric;
class Instance;

enum ExporterFormat
{
	ExporterFormat_Obj,
	ExporterFormat_NumFormats
};

enum ExporterSaveType
{
	ExporterSaveType_Everything,
	ExporterSaveType_Selection,
	ExporterSaveType_NumSaveTypes
};

struct OSContext
{
	void* hWnd;
	int width;
	int height;
    
	//insert OS specific stuff here.
	OSContext()
		: hWnd(0)
		, width(640)
		, height(480)
	{
	}

};

class IViewBaseFactory
{
public:
	virtual ViewBase* Create(CRenderSettings::GraphicsMode mode, 
		OSContext* context, CRenderSettings* renderSettings) = 0;
};

class ViewBase
{
	friend class Visit;

public:
	static ViewBase* CreateView(CRenderSettings::GraphicsMode mode,
		OSContext* context, CRenderSettings* renderSettings);

	static void RegisterFactory(CRenderSettings::GraphicsMode mode,
		IViewBaseFactory* factory);

	// need this because we are statically linking.
	static void InitPluginModules();

	// it is bad form to need this. phase out please.
	static void ShutdownPluginModules();

	virtual void initResources() = 0;
	virtual void bindWorkspace(boost::shared_ptr<RBX::DataModel> dataModel) = 0;

	virtual void render(IMetric* metric, double timeJobStart);
	virtual void renderPrepare(IMetric* metric) = 0;
	virtual void renderPerform(double timeJobStart) = 0;

	virtual void enableVR(bool enabled) = 0;
	virtual void updateVR() = 0;
	virtual const char* getVRDeviceName() = 0;

	virtual void onResize(int cx, int cy) = 0;
	virtual void buildGui(bool buildInGameGui = true) = 0;

	virtual void renderThumb(unsigned char* data, int width, int height, bool crop, bool allowDolly) = 0;

    virtual void garbageCollect() {}

	virtual Instance* getWorkspace() = 0;
	virtual RenderStats& getRenderStats() = 0;

	virtual DataModel* getDataModel() = 0;

	// use for pulling debug info only, please.
	virtual FrameRateManager* getFrameRateManager() { return 0; }

	virtual double getMetricValue(const std::string& s) { return -1; }

	virtual bool getAndClearDoScreenshot() = 0;

	virtual bool exportScene(const std::string& filePath, ExporterSaveType saveType, ExporterFormat format) = 0;
    virtual bool exportSceneThumbJSON(ExporterSaveType saveType, ExporterFormat format, bool encodeBase64, std::string& strOut) = 0;

    virtual void queueAssetReload(const std::string& filePath){};
	virtual void immediateAssetReload(const std::string& filePath) = 0;

	virtual void suspendView() = 0;
	virtual void resumeView() = 0;

    virtual std::pair<unsigned, unsigned> setFrameDataCallback(const boost::function<void(void*)>& callback);

	virtual ~ViewBase() {}
};

}  // namespace RBX
