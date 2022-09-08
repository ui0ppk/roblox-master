#include "StdAfx.h"
#include "ThumbnailGenerator.h"

#include "v8datamodel/lighting.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/TextureContentProvider.h"
#include "v8datamodel/MeshContentProvider.h"
#include "v8datamodel/SolidModelContentProvider.h"
#include "g3d/BinaryOutput.h"
#include "g3d/GImage.h"
#include "v8DataModel/DataModel.h"
#include "v8DataModel/workspace.h"
#include "util/base64.hpp"
#include "util/StandardOut.h"
#include "gui/guidraw.h"
#include <sstream>
#include "LogManager.h"
#include "GfxBase/RenderSettings.h"
#include "DummyWindow.h"
#include "util/FileSystem.h"
#include "reflection/property.h"
#include "v8datamodel/Stats.h"
#include "v8datamodel/FastLogSettings.h"
#include "GfxBase/ViewBase.h"

using namespace RBX;

struct ThumbnailRenderSettings : public RBX::CRenderSettings
{
	ThumbnailRenderSettings()
	{
		antialiasingMode = CRenderSettings::AntialiasingOn;
		eagerBulkExecution = true;
		enableFRM = false;
	}
};

struct ThumbnailRenderRequest
{
	ThumbnailGenerator* generator;
	
	std::string fileType;
	int cx;
	int cy;
	bool hideSky;
	bool crop;
	
    std::string* strOutput;
	std::string* errorOutput;
	
	RBX::CEvent* doneEvent;
};

static rbx::safe_queue<ThumbnailRenderRequest> gThumbRenderQueue;
static RBX::CEvent gThumbRenderQueueNotEmpty(/* manualReset= */ false);

static boost::scoped_ptr<boost::thread> gThumbRenderThread;
static boost::once_flag gThumbRenderThreadInit = BOOST_ONCE_INIT;

static void thumbRenderWorker()
{
	// Create a dummy 1x1 window that holds the GL context and Ogre state alive
	DummyWindow dummyWindow(1, 1);
	boost::scoped_ptr<RBX::ViewBase> dummyView;
	ThumbnailRenderSettings dummySettings;

	OSContext dummyContext;
	dummyContext.hWnd = dummyWindow.handle;
	dummyContext.width = 1;
	dummyContext.height = 1;

    ViewBase::InitPluginModules();

    dummyView.reset(ViewBase::CreateView(CRenderSettings::OpenGL, &dummyContext, &dummySettings));
	
	while (true)
	{
		gThumbRenderQueueNotEmpty.Wait();
		
		ThumbnailRenderRequest request;
		while (gThumbRenderQueue.pop_if_present(request))
		{
			RBXASSERT(request.generator && request.strOutput && request.errorOutput && request.doneEvent);

			try
			{
				// The requesting thread holds a lock for us and waits for doneEvent to be set
				DataModel::scoped_write_transfer writeTransfer(request.generator);

                // filetype==OBJ
                if (strcmpi(request.fileType.c_str(), "obj") == 0)
                {
                    request.generator->exportScene(dummyView.get(), request.strOutput);
                }
				else
				{
					request.generator->renderThumb(dummyView.get(), dummyWindow.handle, request.fileType, request.cx, request.cy, request.hideSky, request.crop, request.strOutput);
				}
				
				*request.errorOutput = std::string();
			}
			catch (std::exception& e)
			{
				*request.errorOutput = std::string("renderThumb failed: ") + e.what();
			}
			
			request.doneEvent->Set();
		}
	}
}

static void thumbRenderInit()
{
	RBXASSERT(!gThumbRenderThread);
	
	gThumbRenderThread.reset(new boost::thread(thumbRenderWorker));
}

const char* const sThumbnailGenerator = "ThumbnailGenerator";

REFLECTION_BEGIN();
static RBX::Reflection::BoundFuncDesc<ThumbnailGenerator, shared_ptr<const RBX::Reflection::Tuple>(std::string, int, int, bool, bool)> clickFunction(&ThumbnailGenerator::click, "Click", "fileType", "width", "height", "hideSky", "crop", false, RBX::Security::LocalUser);
static RBX::Reflection::BoundFuncDesc<ThumbnailGenerator, shared_ptr<const RBX::Reflection::Tuple>(std::string, std::string, int, int)> clickTextureFunction(&ThumbnailGenerator::clickTexture, "ClickTexture", "textureId", "fileType", "width", "height", RBX::Security::LocalUser);
RBX::Reflection::BoundProp<int>			  prop_HeadColor("GraphicsMode", "Settings", &ThumbnailGenerator::graphicsMode);
REFLECTION_END();

volatile long ThumbnailGenerator::totalCount = 0;

ThumbnailGenerator::ThumbnailGenerator(void)
: graphicsMode(0)
{
}

ThumbnailGenerator::~ThumbnailGenerator(void)
{
}


void ThumbnailGenerator::configureCaches()
{
	if(RBX::ContentProvider* contentProvider = RBX::ServiceProvider::create<RBX::ContentProvider>(this)){
		contentProvider->setCacheSize(INT_MAX);
	}
	if(RBX::TextureContentProvider* textureContentProvider = RBX::ServiceProvider::create<RBX::TextureContentProvider>(this)){
		textureContentProvider->setCacheSize(INT_MAX);
		textureContentProvider->setImmediateMode();
	}
	if(RBX::MeshContentProvider* meshContentProvider = RBX::ServiceProvider::create<RBX::MeshContentProvider>(this)){
		meshContentProvider->setCacheSize(INT_MAX);
		meshContentProvider->setImmediateMode();
	}

	if(RBX::SolidModelContentProvider* solidModelContentProvider = RBX::ServiceProvider::create<RBX::SolidModelContentProvider>(this)){
		solidModelContentProvider->setCacheSize(INT_MAX);
		solidModelContentProvider->setImmediateMode();
	}
}

void ReadAccessKey();

shared_ptr<const RBX::Reflection::Tuple> ThumbnailGenerator::clickTexture(std::string textureId, std::string fileType, int cx, int cy)
{
	::InterlockedIncrement(&totalCount);
	try
	{
		ReadAccessKey();

		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ThumbnailGenerator::clickTexture(%s, %s, %d, %d)", textureId.c_str(), fileType.c_str(), cx, cy);

		configureCaches();
		RBX::ContentProvider* contentProvider = RBX::ServiceProvider::create<RBX::ContentProvider>(this);

		G3D::BinaryOutput binaryOutput;
		binaryOutput.setEndian(G3D::G3D_LITTLE_ENDIAN);

		const G3D::GImage::Format format = G3D::GImage::stringToFormat(fileType);

		G3D::GImage image;

        {
            boost::shared_ptr<const std::string> content = contentProvider->getContentString(RBX::ContentId(textureId));

            G3D::BinaryInput binaryInput(reinterpret_cast<const unsigned char*>(content->data()), content->size(), G3D::G3D_LITTLE_ENDIAN, false, false);

            image.decode(binaryInput, G3D::GImage::resolveFormat("", binaryInput.getCArray(), binaryInput.size(), G3D::GImage::AUTODETECT));
        }
	
		if (format != G3D::GImage::PNG)
		{
			// alpha channel not requested, strip it.
			image = image.stripAlpha();
		}
		else
		{
			image.setColorAlphaTest(G3D::Color4uint8(255, 255, 255, 0));
		}

		if (image.width() > 0 && image.height() > 0)
		{
			image = image.bilinearStretchBlt(cx, cy);
		}
		else
		{
			// Create a dummy 1x1 image that we can save (PNG encode can't handle empty images)
			image.resize(1, 1, 3);
		}
		
		image.encode(format, binaryOutput);
		
		std::string strOut;
        base64<char>::encode((const char*)binaryOutput.getCArray(), binaryOutput.length(), strOut, base64<>::noline());

		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "ThumbnailGenerator::clickTexture() success");

		shared_ptr<RBX::Reflection::Tuple> tuble(new RBX::Reflection::Tuple(2));
		tuble->values[0] = strOut;
		tuble->values[1] = contentProvider->getRequestedUrls();

		return tuble;
	}
	catch (std::exception& exp)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, exp);
		throw;
	}
	catch (const G3D::GImage::Error& e)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.reason);
		throw std::runtime_error(e.reason);
	}
};

shared_ptr<const RBX::Reflection::Tuple> ThumbnailGenerator::click(std::string fileType, int cx, int cy, bool hideSky, bool crop)
{
	::InterlockedIncrement(&totalCount);
	try
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "ThumbnailGenerator::click(%s, %d, %d, %s)", fileType.c_str(), cx, cy, hideSky ? "true" : "false");

		RBX::Time startTime = RBX::Time::now<Time::Fast>();

		ReadAccessKey();

        std::string strOutput;
		
		RBX::ContentProvider* contentProvider = RBX::ServiceProvider::create<RBX::ContentProvider>(this);
		configureCaches();

        {
            // We need to reuse graphics state between runs; this means reusing Mesa GL context.
            // There are two reasons why we'd like *all* render requests to come off a single thread:
            // 1. The window that's created in a thread dies, and DC dies with it - we need to keep the main window alive
            // 2. Mesa GL context is bound to a thread that created it; if a new thread is created, graphics can't reuse old GL context.
            std::string errorOutput;
            RBX::CEvent doneEvent(/* manualReset= */ true);
            
            ThumbnailRenderRequest request = {this, fileType, cx, cy, hideSky, crop, &strOutput, &errorOutput, &doneEvent};
            
            boost::call_once(thumbRenderInit, gThumbRenderThreadInit);
            
            gThumbRenderQueue.push(request);
            gThumbRenderQueueNotEmpty.Set();
            
            doneEvent.Wait();
            
            if (!errorOutput.empty())
            {
                // render request threw an exception; "marshal" it here (we lose the stack, but the exception gets converted to Lua error anyway)
                throw std::runtime_error(errorOutput);
            }
        }

		RBX::StandardOut::singleton()->print(RBX::MESSAGE_INFO, "ThumbnailGenerator::click() success");

		// log thumbnail generation time
		// TODO: log asset/place id
		shared_ptr<Reflection::ValueTable> entry(new Reflection::ValueTable());
		(*entry)["Time"] = (RBX::Time::now<Time::Fast>() - startTime).seconds();

		RBX::Stats::StatsService* stats = RBX::ServiceProvider::create<RBX::Stats::StatsService>(this);
		stats->report("Thumbnail", entry);

        shared_ptr<RBX::Reflection::Tuple> tuble(new RBX::Reflection::Tuple(2));
        tuble->values[0] = strOutput;
        tuble->values[1] = contentProvider->getRequestedUrls();

		return tuble;
	}
	catch (std::exception& exp)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, exp);
		throw;
	}
}

void ThumbnailGenerator::exportScene(RBX::ViewBase* view, std::string* outStr)
{
    RBX::Workspace* w = RBX::ServiceProvider::find<RBX::Workspace>(this);
    // just fill it with a bunch of dummies
    bool allowDolly = w->setImageServerView(false);
    RBX::DataModel* dataModel = boost::polymorphic_downcast<RBX::DataModel*>(getParent());
    view->bindWorkspace(shared_from(dataModel));
    view->exportSceneThumbJSON(ExporterSaveType_Everything, ExporterFormat_Obj, true, *outStr);
    view->bindWorkspace(shared_ptr<DataModel>());
}

void ThumbnailGenerator::renderThumb(ViewBase* view, void* windowHandle, std::string fileType, int cx, int cy, bool hideSky, bool crop, std::string* strOutput)
{
	const G3D::GImage::Format format = G3D::GImage::stringToFormat(fileType);

	bool alphaChannel = false;
	
	// TODO: put this code into an API to Lighting???
	if (hideSky)
	{
		RBX::Lighting* lighting = RBX::ServiceProvider::create<RBX::Lighting>(this);
		lighting->suppressSky(true);
		if (format==G3D::GImage::PNG)
		{
			alphaChannel = true;
			lighting->setClearAlpha(0);
		}
		else
			lighting->setClearAlpha(1);
	}

	// TODO: put this code into an API to Camera
	RBX::Workspace* w = RBX::ServiceProvider::find<RBX::Workspace>(this);

	// Camera Adjustments:
	// use camera named "ThumbnailCamera" else:
	//	for a model / bunch of models - look from -Z
	//  for a place: not much.
	bool allowDolly = w->setImageServerView(!hideSky /* = isAPlace*/);

	// render settings;
	ThumbnailRenderSettings defaultsettings;

	{
		RBX::DataModel* dataModel = boost::polymorphic_downcast<RBX::DataModel*>(getParent());
		G3D::GImage image(cx, cy, 4);

        view->bindWorkspace(shared_from(dataModel));
		view->renderThumb(image.byte(), cx, cy, crop,allowDolly);
		view->bindWorkspace(shared_ptr<DataModel>());

		view->garbageCollect();

        if (!alphaChannel)
        {
            image.convertToRGB();
        }

        if (strOutput)
        {
            G3D::BinaryOutput binaryOutput;
            binaryOutput.setEndian(G3D::G3D_LITTLE_ENDIAN);
            image.encode(format, binaryOutput);
            base64<char>::encode((const char*)binaryOutput.getCArray(), binaryOutput.length(), *strOutput, base64<>::noline());
        }
		
		if (hideSky)
		{
			RBX::Workspace* w = RBX::ServiceProvider::find<RBX::Workspace>(this);
			// Hack to avoid double-toggle
			w->setImageServerView(false);
		}
	}
}
