#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
#include "DeviceGL.h"

#include "GfxCore/Framebuffer.h"

#include "G3D/Quat.h"
#include "G3D/Matrix4.h"

#include "rbx/threadsafe.h"
#include "rbx/CEvent.h"

#include "rbx/Profiler.h"

#include "HeadersGL.h"

#include <math.h>

#ifdef RBX_PLATFORM_IOS
#import <CoreMotion/CMMotionManager.h>
#import <UIKit/UIApplication.h>
#import <UIKit/UIScreen.h>
#import <QuartzCore/CABase.h>
#endif

#ifdef __ANDROID__
#include <android/sensor.h>
#include <EGL/egl.h>
#include <time.h>
#endif

LOGGROUP(VR)

FASTFLAGVARIABLE(CardboardVR, false)

namespace RBX
{
namespace Graphics
{

// Configuration (data is kept globally accessible for ease of use from the app)
struct CardboardConfiguration
{
	float fieldOfView;
	float interLensDistance;
	float trayToLensCenterDistance; // aka verticalDistanceToLensCenter
	float screenToLensDistance;

	float distortionCoeffs[2];
	
	float screenWidth;
	float screenHeight;
	float screenBorderSize;
};

// Google Cardboard - Technical Specification version 2.0 - September 2015
static CardboardConfiguration gConfiguration = { 120, 0.0639f, 0.035f, 0.039f, { 0.34f, 0.55f }, 0, 0, 0.003f };
static float gScreenOrientation = 1.f;

// Implementation
const bool kUseVsync = true;
const bool kUseTimeWarp = true;
const bool kUseAdaptivePrediction = true;
const bool kUseNeckModel = true;

using G3D::Quat;
using G3D::Vector3;
using G3D::Matrix4;

const Vector3 kNeckOffset(0, 0.075f, -0.0805f);

struct DistortionVertex
{
    float x, y;
    float u, v;
    float fade;
};

static float clamp(float t, float a, float b)
{
    return std::min(std::max(t, a), b);
}

static float distort(const CardboardConfiguration& configuration, float radius)
{
    float result = 1.0f;
    float factor = 1.0f;

    for (size_t i = 0; i < ARRAYSIZE(configuration.distortionCoeffs); ++i)
    {
        factor *= radius * radius;
        result += configuration.distortionCoeffs[i] * factor;
    }

    return radius * result;
}

static float distortInverse(const CardboardConfiguration& configuration, float radius)
{
    float r0 = radius / 0.9f;
    float r = radius * 0.9f;
    float dr0 = radius - distort(configuration, r0);
    while (fabsf(r - r0) > 0.0001f)
    {
        float dr = radius - distort(configuration, r);
        float r2 = r - dr * ((r - r0) / (dr - dr0));
        r0 = r;
        r = r2;
        dr0 = dr;
    }
    return r;
}

static void computeFOVPort(const CardboardConfiguration& configuration, float fovPort[4])
{
    float fovTan = tanf(configuration.fieldOfView / 2 * (3.1415926f / 180));

    float outerDistance = (configuration.screenWidth - configuration.interLensDistance) / 2.0f;
    float innerDistance = configuration.interLensDistance / 2.0f;
    float bottomDistance = configuration.trayToLensCenterDistance - configuration.screenBorderSize;
    float topDistance = configuration.screenHeight + configuration.screenBorderSize - configuration.trayToLensCenterDistance;

    fovPort[0] = std::min(fovTan, distort(configuration, topDistance / configuration.screenToLensDistance));
    fovPort[1] = std::min(fovTan, distort(configuration, bottomDistance / configuration.screenToLensDistance));
    fovPort[2] = std::min(fovTan, distort(configuration, outerDistance / configuration.screenToLensDistance));
    fovPort[3] = std::min(fovTan, distort(configuration, innerDistance / configuration.screenToLensDistance));
}

static shared_ptr<Geometry> createDistortionMesh(Device* device, const CardboardConfiguration& configuration, int eye, unsigned int* outIndices, float outUVScaleOffset[4])
{
	float eyeSign = (eye == 0) ? -1 : 1;

	float fovPort[4];
	computeFOVPort(configuration, fovPort);

	float screenWidth = configuration.screenWidth / configuration.screenToLensDistance;
	float screenHeight = configuration.screenHeight / configuration.screenToLensDistance;
	float xEyeOffsetScreen = (configuration.screenWidth / 2.0f + eyeSign * configuration.interLensDistance / 2.0f) / configuration.screenToLensDistance;
	float yEyeOffsetScreen = (configuration.trayToLensCenterDistance - configuration.screenBorderSize) / configuration.screenToLensDistance;

	float textureWidth = fovPort[2] + fovPort[3];
	float textureHeight = fovPort[0] + fovPort[1];
	float xEyeOffsetTexture = (eye == 0) ? fovPort[2] : fovPort[3];
	float yEyeOffsetTexture = fovPort[1];

	float vignetteSizeTanAngle = 0.05f;

    const int rows = 40;
    const int cols = 40;
    const unsigned int vertexCount = rows * cols;
    const unsigned int indexCount = (rows - 1) * cols * 2 + (rows - 2);

    std::vector<VertexLayout::Element> elements;
    elements.push_back(VertexLayout::Element(0, offsetof(DistortionVertex, x), VertexLayout::Format_Float2, VertexLayout::Semantic_Position));
    elements.push_back(VertexLayout::Element(0, offsetof(DistortionVertex, u), VertexLayout::Format_Float3, VertexLayout::Semantic_Texture));
    
    shared_ptr<VertexLayout> layout = device->createVertexLayout(elements);
    
    shared_ptr<VertexBuffer> vb = device->createVertexBuffer(sizeof(DistortionVertex), vertexCount, GeometryBuffer::Usage_Static);
    
    DistortionVertex* vbptr = static_cast<DistortionVertex*>(vb->lock());
    
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            float uTexture = col / float(cols - 1);
            float vTexture = row / float(rows - 1);
            
            float xTexture = uTexture * textureWidth - xEyeOffsetTexture;
            float yTexture = vTexture * textureHeight - yEyeOffsetTexture;
            float rTexture = sqrtf(xTexture * xTexture + yTexture * yTexture);
            
            float textureToScreen = (rTexture > 0.0f) ? distortInverse(configuration, rTexture) / rTexture : 1.0f;
            
            float xScreen = xTexture * textureToScreen;
            float yScreen = yTexture * textureToScreen;
            
            float uScreen = (xScreen + xEyeOffsetScreen) / screenWidth;
            float vScreen = (yScreen + yEyeOffsetScreen) / screenHeight;
            
            float vignetteSizeTexture = vignetteSizeTanAngle / textureToScreen;
            
            float dxTexture = xTexture + xEyeOffsetTexture - clamp(xTexture + xEyeOffsetTexture, vignetteSizeTexture, textureWidth - vignetteSizeTexture);
            float dyTexture = yTexture + yEyeOffsetTexture - clamp(yTexture + yEyeOffsetTexture, vignetteSizeTexture, textureHeight - vignetteSizeTexture);
            float drTexture = sqrtf(dxTexture * dxTexture + dyTexture * dyTexture);
            
            float vignette = 1.0f - clamp(drTexture / vignetteSizeTexture, 0.0f, 1.0f);

            vbptr->x = 2.0f * uScreen - 1.0f;
            vbptr->y = 2.0f * vScreen - 1.0f;
            vbptr->u = xTexture;
            vbptr->v = yTexture;
            vbptr->fade = vignette;
            vbptr++;
        }
    }

    vb->unlock();
    
    shared_ptr<IndexBuffer> ib = device->createIndexBuffer(sizeof(unsigned short), indexCount, GeometryBuffer::Usage_Static);
    
    unsigned short* ibptr = static_cast<unsigned short*>(ib->lock());
    
    unsigned int vertexOffset = 0;

    for (int row = 0; row < rows-1; ++row)
    {
        if (row > 0)
        {
            int last = ibptr[-1];
            
            *ibptr++ = last;
        }

        for (int col = 0; col < cols; ++col)
        {
            if (col > 0)
            {
                if (row % 2 == 0)
                    vertexOffset++;
                else
                    vertexOffset--;
            }

            *ibptr++ = vertexOffset;
            *ibptr++ = vertexOffset + 40;
        }

        vertexOffset += 40;
    }

    ib->unlock();
    
    *outIndices = indexCount;

    outUVScaleOffset[0] = 1 / textureWidth;
    outUVScaleOffset[1] = 1 / textureHeight;
    outUVScaleOffset[2] = xEyeOffsetTexture / textureWidth;
    outUVScaleOffset[3] = yEyeOffsetTexture / textureHeight;
    
    return device->createGeometry(layout, vb, ib);
}

static shared_ptr<ShaderProgram> createDistortionProgram(Device* device)
{
    std::string vertexSourceCommon = "\
uniform vec4 UVScaleOffset;\
uniform vec4 Warp;\
vec3 qtransform(vec4 q, vec3 v) { return v + 2.0*cross(cross(v, q.xyz) + q.w*v, q.xyz); } \
void main() {\
gl_Position = vertex;\
highp vec3 uvw = qtransform(Warp, vec3(uv0.xy, -1.0));\
texcoord = vec3((uvw.xy / -uvw.z) * UVScaleOffset.xy + UVScaleOffset.zw, uv0.z);\
}";
    
    std::string vertexSourceGL2 = "attribute highp vec4 vertex;\nattribute highp vec3 uv0;\nvarying highp vec3 texcoord;\n" + vertexSourceCommon;
    std::string vertexSourceGL3 = "#version 300 es\nin highp vec4 vertex;\nin highp vec3 uv0;\nout highp vec3 texcoord;\n" + vertexSourceCommon;
    std::string fragmentSourceGL2 = "uniform sampler2D buffer;\nvarying highp vec3 texcoord;\nvoid main() { gl_FragData[0] = texture2D(buffer, texcoord.xy) * texcoord.z; }";
    std::string fragmentSourceGL3 = "#version 300 es\nuniform sampler2D buffer;\nout lowp vec4 _glFragData[1];\nin highp vec3 texcoord;\nvoid main() { _glFragData[0] = texture(buffer, texcoord.xy) * texcoord.z; }";
    
    std::string vertexSource = (device->getShadingLanguage() == "glsles") ? vertexSourceGL2 : vertexSourceGL3;
    std::string fragmentSource = (device->getShadingLanguage() == "glsles") ? fragmentSourceGL2 : fragmentSourceGL3;
    
    return device->createShaderProgram(
        device->createVertexShader(device->createShaderBytecode(vertexSource, "", "main")),
        device->createFragmentShader(device->createShaderBytecode(fragmentSource, "", "main")));
}

static Quat predictRotation(double displayTimestamp, double sensorTimestamp, const Vector3& rotationRate)
{
    float predictionDt = std::min(std::max(displayTimestamp - sensorTimestamp, 0.0), 0.1);

    if (kUseAdaptivePrediction)
    {
        float angularSpeed = rotationRate.length();
        float candidateDt = angularSpeed * 0.2; // The rate at which the dynamic prediction interval varies
        
        predictionDt = (angularSpeed > 0.001f) ? std::min(predictionDt, candidateDt) : 0;
    }
    
    return Quat::fromRotation(predictionDt * rotationRate);
}

static DeviceVR::Pose getPose(const Vector3& position, const Quat& orientation)
{
	DeviceVR::Pose result = {};

	result.valid = true;

	result.position[0] = position.x;
	result.position[1] = position.y;
	result.position[2] = position.z;

	result.orientation[0] = orientation.x;
	result.orientation[1] = orientation.y;
	result.orientation[2] = orientation.z;
	result.orientation[3] = orientation.w;

	return result;
}

#ifdef RBX_PLATFORM_IOS
class HeadTracker
{
public:
    HeadTracker()
    : gyroTimestamp(0)
    , orientationTimestamp(0)
    {
        motionManager = [[CMMotionManager alloc] init];
        queue = [[NSOperationQueue alloc] init];
        
		motionManager.gyroUpdateInterval = 1 / 100.0;
		[motionManager startGyroUpdatesToQueue:queue withHandler:^(CMGyroData* gyroData, NSError*) {
            RBXPROFILER_SCOPE("VR", "updateSensors");
            RBXPROFILER_LABELF("VR", "gyro %.1f ms", (gyroData.timestamp - CACurrentMediaTime()) * 1000);
 
            rbx::spin_mutex::scoped_lock lock(mutex);
            
            gyro = Vector3(gyroData.rotationRate.x, gyroData.rotationRate.y, gyroData.rotationRate.z);
            gyroTimestamp = gyroData.timestamp;
        }];

		motionManager.deviceMotionUpdateInterval = 1 / 100.0;
		[motionManager startDeviceMotionUpdatesUsingReferenceFrame:CMAttitudeReferenceFrameXArbitraryZVertical toQueue:queue withHandler:^(CMDeviceMotion* motion, NSError*) {
            RBXPROFILER_SCOPE("VR", "updateSensors");
            RBXPROFILER_LABELF("VR", "orientation %.1f ms", (motion.timestamp - CACurrentMediaTime()) * 1000);
        
            rbx::spin_mutex::scoped_lock lock(mutex);
            
            orientation = Quat(motion.attitude.quaternion.x, motion.attitude.quaternion.y, motion.attitude.quaternion.z, motion.attitude.quaternion.w);
            orientationTimestamp = motion.timestamp;
        }];
    }
    
    ~HeadTracker()
    {
		[motionManager stopGyroUpdates];
		[motionManager stopDeviceMotionUpdates];

        [motionManager release];
        [queue release];
    }
    
	double getTime()
	{
		return CACurrentMediaTime();
	}
    
    bool isValid()
    {
		return orientationTimestamp > 0;
    }
    
    Quat predictOrientation(double time)
    {
		rbx::spin_mutex::scoped_lock lock(mutex);
    
        if (!isValid()) return Quat();
        
		return orientation * predictRotation(time, orientationTimestamp, gyro);
    }

private:
    CMMotionManager* motionManager;
    NSOperationQueue* queue;
    
   	rbx::spin_mutex mutex;

	Vector3 gyro;
	double gyroTimestamp;

	Quat orientation;
	double orientationTimestamp;
};
#endif

#ifdef __ANDROID__
enum
{
	ASENSOR_TYPE_GAME_ROTATION = 15,
};

class HeadTracker
{
public:
    HeadTracker()
	: looper(NULL)
	, looperReady(false)
	, gyroTimestamp(0)
	, gameRotationTimestamp(0)
    {
		boost::thread(boost::bind(processThread, this)).swap(thread);
    }
    
    ~HeadTracker()
    {
		looperReady.Wait();
		ALooper_wake(looper);
		thread.join();
    }
    
	double getTime()
	{
		timespec tv;
		clock_gettime(CLOCK_BOOTTIME, &tv);

		return tv.tv_sec + tv.tv_nsec / 1e9;
	}
    
    bool isValid()
    {
		return gameRotationTimestamp > 0;
    }

    Quat predictOrientation(double time)
    {
		rbx::spin_mutex::scoped_lock lock(mutex);

        if (!isValid()) return Quat();

		Quat orientation(gameRotation, sqrtf(std::max(0.f, 1 - gameRotation.squaredLength())));
            
		return orientation * predictRotation(time, gameRotationTimestamp, gyro);
    }

private:
	static void processThread(HeadTracker* self)
	{
		Profiler::onThreadCreate("SensorUpdate");

		self->looper = ALooper_prepare(ALOOPER_PREPARE_ALLOW_NON_CALLBACKS);

		self->looperReady.Set();

		ASensorManager* manager = ASensorManager_getInstance();
		ASensorEventQueue* eventQueue = ASensorManager_createEventQueue(manager, self->looper, 0, NULL, NULL);
        
		FASTLOG(FLog::VR, "VR: Sensor thread started");

		setupSensor(eventQueue, ASensorManager_getDefaultSensor(manager, ASENSOR_TYPE_GYROSCOPE));
		setupSensor(eventQueue, ASensorManager_getDefaultSensor(manager, ASENSOR_TYPE_GAME_ROTATION));

		for (;;)
		{
			int rc = ALooper_pollOnce(-1, NULL, NULL, NULL);
			if (rc != 0)
			{
				FASTLOG1(FLog::VR, "VR: Stopping sensor thread (pollOnce returned %d)", rc);
				break;
			}

			RBXPROFILER_SCOPE("VR", "updateSensors");

			for (;;)
			{
				ASensorEvent eventBuffer[16];

				ssize_t numEvents = ASensorEventQueue_getEvents(eventQueue, eventBuffer, ARRAYSIZE(eventBuffer));
				if (numEvents <= 0) break;

				rbx::spin_mutex::scoped_lock lock(self->mutex);
                
                double time = self->getTime();

				for (ssize_t i = 0; i < numEvents; ++i)
				{
					const ASensorEvent& e = eventBuffer[i];
                    
					if (e.type == ASENSOR_TYPE_GYROSCOPE)
					{
						self->gyro = Vector3(e.data);
						self->gyroTimestamp = e.timestamp / 1e9;

						RBXPROFILER_LABELF("VR", "gyro %.1f ms", (time - e.timestamp / 1e9) * 1000);
					}
					else if (e.type == ASENSOR_TYPE_GAME_ROTATION)
					{
						self->gameRotation = Vector3(e.data);
						self->gameRotationTimestamp = e.timestamp / 1e9;

						RBXPROFILER_LABELF("VR", "orientation %.1f ms", (time - e.timestamp / 1e9) * 1000);
					}
				}
			}
		}

		ASensorManager_destroyEventQueue(manager, eventQueue);

		Profiler::onThreadExit();
	}
    
	static void setupSensor(ASensorEventQueue* eventQueue, const ASensor* sensor)
	{
		if (sensor)
        {
			int rate = ASensor_getMinDelay(sensor);

			ASensorEventQueue_enableSensor(eventQueue, sensor);
			ASensorEventQueue_setEventRate(eventQueue, sensor, rate);

			FASTLOG2(FLog::VR, "VR: Setup sensor %d with rate %d us", ASensor_getType(sensor), ASensor_getMinDelay(sensor));
			FASTLOGS(FLog::VR, "VR: Sensor %s", ASensor_getName(sensor));
		}
	}

	ALooper* looper;
	CEvent looperReady;

	boost::thread thread;
	rbx::spin_mutex mutex;

	Vector3 gyro;
	double gyroTimestamp;

	Vector3 gameRotation;
	double gameRotationTimestamp;
};
#endif

struct CardboardVRGL: DeviceVRGL
{
    Framebuffer* mainFramebuffer;

    shared_ptr<Framebuffer> fb[2];
    shared_ptr<Texture> textures[2];
    
    shared_ptr<ShaderProgram> distortionProgram;

    shared_ptr<Geometry> distortionMesh[2];
    unsigned int distortionMeshIndices;
	float distortionUVScaleOffset[2][4];
    
    HeadTracker headTracker;
    CardboardConfiguration configuration;

	double displayTime;
    Quat headOrientation;
    Vector3 headPosition;
    
    float headingRef;
    bool headingSet;

	CardboardVRGL()
	{
        mainFramebuffer = NULL;

		distortionMeshIndices = 0;
        memset(distortionUVScaleOffset, 0, sizeof(distortionUVScaleOffset));

		displayTime = 0;
        
        headingRef = 0;
        headingSet = 0;
	}

	~CardboardVRGL()
	{
	}
    
    Quat getHeadOrientation(const Quat& orientation)
    {
        float pi = G3D::pif();

        float uiOrientationFlip = gScreenOrientation;
        Quat displayOrientation = Quat::fromAxisAngleRotation(Vector3::unitZ(), uiOrientationFlip * pi / 2);

        return
            Quat::fromAxisAngleRotation(Vector3::unitX(), -pi / 2) *
            orientation * displayOrientation;
    }

    Quat getHeadOrientationCentered(const Quat& orientation)
    {
        return
            Quat::fromAxisAngleRotation(Vector3::unitY(), -headingRef) *
            getHeadOrientation(orientation);
    }

	void update() override
	{
		// 40 ms is the estimated time to display the content
		displayTime = headTracker.getTime() + 0.040;

        Quat orientation = headTracker.predictOrientation(displayTime);

        // reset heading when we get valid orientations
        if (!headingSet && headTracker.isValid())
        {
            Vector3 heading = getHeadOrientation(orientation) * Vector3(0, 0, -1);
            
            if (fabsf(heading.y) < 0.95f)
            {
                float angle = atan2(-heading.x, -heading.z);
            
                headingRef = angle;
                headingSet = true;
            }
        }
        
        // update head orientation
		headOrientation = getHeadOrientationCentered(orientation);

        if (kUseNeckModel)
        {
            // update head position using neck model
            headPosition = headOrientation * kNeckOffset - Vector3(0, kNeckOffset.y, 0);
        }
	}

	void recenter() override
	{
		headingSet = false;
	}

	Framebuffer* getEyeFramebuffer(int eye) override
	{
		RBXASSERT(eye == 0 || eye == 1);

		return fb[eye].get();
	}

	State getState() override
	{
		State result = {};

        result.headPose = getPose(headPosition, headOrientation);

        float fovPort[4];
        computeFOVPort(configuration, fovPort);
        
		for (int eye = 0; eye < 2; ++eye)
		{
            float eyeSign = (eye == 0 ? -1 : 1);

			result.eyeOffset[eye][0] = eyeSign * configuration.interLensDistance / 2;
			result.eyeOffset[eye][1] = 0;
			result.eyeOffset[eye][2] = 0;
            
			result.eyeFov[eye][0] = fovPort[0];
			result.eyeFov[eye][1] = fovPort[1];
			result.eyeFov[eye][2] = (eye == 0) ? fovPort[2] : fovPort[3];
			result.eyeFov[eye][3] = (eye == 0) ? fovPort[3] : fovPort[2];
		}
        
        result.needsMirror = false;

		return result;
	}

	void submitFrame(DeviceContext* context) override
	{
		unsigned int screenWidth = mainFramebuffer->getWidth();
        unsigned int screenHeight = mainFramebuffer->getHeight();
    
        context->bindFramebuffer(mainFramebuffer);

        const float clearColor[] = {0, 0, 0, 0};
        context->clearFramebuffer(DeviceContext::Buffer_Color, clearColor, 0, 0);
        
		context->bindProgram(distortionProgram.get());
        context->setBlendState(BlendState::Mode_None);
        context->setRasterizerState(RasterizerState::Cull_None);
        context->setDepthState(DepthState(DepthState::Function_Always, false));

		Quat warp;

        if (kUseTimeWarp)
        {
			Quat warpOrientation = headTracker.predictOrientation(displayTime);
			Quat warpHeadOrientation = getHeadOrientationCentered(warpOrientation);

			warp = warpHeadOrientation.inverse() * headOrientation;
        }

		for (int eye = 0; eye < 2; ++eye)
		{
			context->setConstant(distortionProgram->getConstantHandle("UVScaleOffset"), distortionUVScaleOffset[eye], 1);
			context->setConstant(distortionProgram->getConstantHandle("Warp"), &warp.x, 1);

			context->bindTexture(0, textures[eye].get(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
			context->draw(distortionMesh[eye].get(), Geometry::Primitive_TriangleStrip, 0, distortionMeshIndices, 0, 0);
		}

        const int dividerWidth = 4;
        const float dividerColor[] = {1, 1, 1, 1};

        glEnable(GL_SCISSOR_TEST);
        glScissor(screenWidth / 2 - dividerWidth / 2, 0, dividerWidth, screenHeight);

        context->clearFramebuffer(DeviceContext::Buffer_Color, dividerColor, 0, 0);

        glDisable(GL_SCISSOR_TEST);
	}

	void setup(Device* device) override
	{
        mainFramebuffer = device->getMainFramebuffer();

        unsigned int width = 1024, height = 1024;

		shared_ptr<Renderbuffer> depthStencil = device->createRenderbuffer(Texture::Format_D24S8, width, height, 1);

		for (int eye = 0; eye < 2; ++eye)
    	{
            textures[eye] = device->createTexture(Texture::Type_2D, Texture::Format_RGBA8, width, height, 1, 1, Texture::Usage_Renderbuffer);
			fb[eye] = device->createFramebuffer(textures[eye]->getRenderbuffer(0, 0), depthStencil);
		}
        
		distortionMesh[0] = createDistortionMesh(device, configuration, 0, &distortionMeshIndices, distortionUVScaleOffset[0]);
		distortionMesh[1] = createDistortionMesh(device, configuration, 1, &distortionMeshIndices, distortionUVScaleOffset[1]);

        distortionProgram = createDistortionProgram(device);
	}
};

DeviceVRGL* DeviceVRGL::createCardboard()
{
	if (!FFlag::CardboardVR)
		return NULL;

	CardboardVRGL* vr = new CardboardVRGL();
    
	vr->configuration = gConfiguration;

	float fovPort[4];
	computeFOVPort(vr->configuration, fovPort);

	for (int i = 0; i < 4; ++i)
		fovPort[i] = atanf(fovPort[i]) / (3.1415926f / 180);

#ifdef __ANDROID__
	eglSwapInterval(eglGetCurrentDisplay(), kUseVsync);
#else
    (void)kUseVsync;
#endif

	FASTLOG3F(FLog::VR, "VR: HMD interLens %f trayToLensCenter %f screenToLens %f", vr->configuration.interLensDistance, vr->configuration.trayToLensCenterDistance, vr->configuration.screenToLensDistance);
	FASTLOG3F(FLog::VR, "VR: Lens FOV %f distortion0 %f distortion1 %f", vr->configuration.fieldOfView, vr->configuration.distortionCoeffs[0], vr->configuration.distortionCoeffs[1]);
	FASTLOG3F(FLog::VR, "VR: Screen width %f height %f border %f", vr->configuration.screenWidth, vr->configuration.screenHeight, vr->configuration.screenBorderSize);
	FASTLOG4F(FLog::VR, "VR: Eye FOV up %f down %f left %f right %f", fovPort[0], fovPort[1], fovPort[2], fovPort[3]);

	return vr;
}

}
}

void vrCardboardSetDeviceParams(float fieldOfView, float interLensDistance, float trayToLensCenterDistance, float screenToLensDistance, float distortionCoeff0, float distortionCoeff1)
{
	using RBX::Graphics::gConfiguration;

	gConfiguration.fieldOfView = fieldOfView;
	gConfiguration.interLensDistance = interLensDistance;
	gConfiguration.trayToLensCenterDistance = trayToLensCenterDistance;
	gConfiguration.screenToLensDistance = screenToLensDistance;
	gConfiguration.distortionCoeffs[0] = distortionCoeff0;
	gConfiguration.distortionCoeffs[1] = distortionCoeff1;
}

void vrCardboardSetPhoneParams(int screenWidth, int screenHeight, float xdpi, float ydpi)
{
	using RBX::Graphics::gConfiguration;

	float metersPerInch = 0.0254f;
	float screenWidthMeters = screenWidth * (metersPerInch / xdpi);
	float screenHeightMeters = screenHeight * (metersPerInch / ydpi);

	gConfiguration.screenWidth = std::max(screenWidthMeters, screenHeightMeters);
	gConfiguration.screenHeight = std::min(screenWidthMeters, screenHeightMeters);
	gConfiguration.screenBorderSize = 0.003f;
}

void vrCardboardSetOrientation(bool flipped)
{
	using RBX::Graphics::gScreenOrientation;

	gScreenOrientation = flipped ? -1 : 1;
}

#endif
