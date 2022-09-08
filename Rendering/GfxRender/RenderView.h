#pragma once

#include "GfxBase/ViewBase.h"
#include "v8datamodel/RenderHooksService.h"
#include "rbx/RunningAverage.h"

LOGGROUP(DeviceLost)
LOGGROUP(ViewRbxBase)
LOGGROUP(ViewRbxInit)

namespace RBX
{
	class Lighting;
    class DataModel;
    class PlatformService;
}

namespace RBX
{
namespace Graphics
{

class Device;
class VisualEngine;
class FrameRateManagerStatsItem;
class SSAO;
class Framebuffer;
class VertexStreamer;
class DeviceContext;
class Texture;
class ProfilerRenderer;

class RenderView
    : public ViewBase
    , public IRenderHooks
{
public:
    RenderView(CRenderSettings::GraphicsMode mode, OSContext* context, CRenderSettings* renderSettings);
    ~RenderView();

    void bindWorkspace(boost::shared_ptr<RBX::DataModel> dataModel);
    void initResources();

    void renderPrepare(IMetric* metric);
    void renderPerform(double timeRenderJob);

    void onResize(int cx, int cy);
    void buildGui(bool buildInGameGui = true);

    void updateVR();
	void enableVR(bool enabled);
	const char* getVRDeviceName();

    void renderThumb(unsigned char* data, int width, int height, bool crop, bool allowDolly);

    bool getAndClearDoScreenshot();

	bool exportScene(const std::string& filePath, ExporterSaveType saveType, ExporterFormat format);
    bool exportSceneThumbJSON(ExporterSaveType saveType, ExporterFormat format, bool encodeBase64, std::string& strOut);

	void suspendView();
	void resumeView();

    RBX::Instance* getWorkspace();

    RenderStats & getRenderStats();

    RBX::DataModel* getDataModel() { return dataModel.get(); }
    FrameRateManager* getFrameRateManager();

    virtual double getMetricValue(const std::string& s);

    VisualEngine& getVisualEngine() { return *visualEngine; }

    // Debug/Profiling hooks
    void reloadShaders();
    void captureMetrics(RenderMetrics& metrics);
    void enableAdorns(bool enable);
    void queueAssetReload(const std::string& filePath);
	void immediateAssetReload(const std::string& filePath);
    void printScene();
    
    std::string getRenderStatsMetric(const std::string& name);

    virtual void garbageCollect();

    virtual std::pair<unsigned, unsigned> setFrameDataCallback(const boost::function<void(void*)>& callback);

private:
    void onWorkspaceDescendantAdded(shared_ptr<RBX::Instance> descendant);
    void updateLighting(Lighting* lighting);
    void updateFog();
    void invalidateLighting(bool updateSkybox);
    void onTakeScreenshot();
    bool saveScreenshotToFile(std::string& /*output*/ filename);
    void prepareSceneGraph();
    void sendFeatureLevelStats();
    void drawRecordingFrame(DeviceContext* context);
    void drawProfiler(DeviceContext* context);
    void drawVRWindow(DeviceContext* context);

    void presetLighting(RBX::Lighting* l, const RBX::Color3& extraAmbient = RBX::Color3(0, 0, 0), float skylightFactor = 1);
    void presetPostProcess(RBX::PlatformService* platformService);

    void renderPrepareImpl(IMetric* metric, bool updateViewport);
	void renderPerformImpl(double timeRenderJob, Framebuffer* mainFramebuffer);

    scoped_ptr<Device> device;
    scoped_ptr<VisualEngine> visualEngine;

    double  deltaMs;
    double  timestampMs;
    float   prepareTime;
	float   totalRenderTime;
    double  artificialDelay;

    RBX::WindowAverage<double, double> prepareAverage;
    RBX::WindowAverage<double, double> performAverage;
    RBX::WindowAverage<double, double> presentAverage;
    RBX::WindowAverage<double, double> gpuAverage;

    rbx::signal<void(std::string)> screenshotFinishedSignal;

    boost::shared_ptr<RBX::DataModel> dataModel;

    rbx::signals::scoped_connection lightingChangedConnection;
    bool lightingValid;

    rbx::signals::scoped_connection screenshotConnection;
    bool doScreenshot;

    bool adornsEnabled;

    bool outlinesEnabled;
    float fogEndCurrentFRM;
    float fogEndTargetFRM;

    typedef boost::function<void(void*)> FrameDataCallback;
    FrameDataCallback frameDataCallback;
    scoped_ptr<VertexStreamer> videoFrameVertexStreamer;

	scoped_ptr<ProfilerRenderer> profilerRenderer;

    boost::shared_ptr<FrameRateManagerStatsItem> frameRateManagerStatsItem;

    scoped_ptr<VertexStreamer> vrVertexStreamer;
	shared_ptr<Texture> vrDebugTexture;
};
}
}
