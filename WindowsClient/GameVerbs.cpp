#include "stdafx.h"

#include "GameVerbs.h"

#include "Document.h"
#include "DSVideoCaptureEngine.h"
#include "FunctionMarshaller.h"
#include "VideoControl.h"
#include "DSVideoCaptureEngine.h"
#include "RenderSettingsItem.h"
#include "DSVideoCaptureEngine.h"
#include "Resource.h"
#include "util/FileSystem.h"
#include "util/Http.h"
#include "util/Statistics.h"
#include "v8datamodel/Game.h"
#include "v8datamodel/GameBasicSettings.h"
#include "VideoControl.h"
#include "View.h"
#include "Application.h"
#include "LogManager.h"
#include "WebBrowserAxDialog.h"


namespace RBX {

LeaveGameVerb::LeaveGameVerb(View& view, VerbContainer* container)
	: Verb(container, "Exit")
	, v(view)
{
}

void LeaveGameVerb::doIt(IDataState* dataState)
{
	MainLogManager::getMainLogManager()->setLeaveGame();
    v.CloseWindow();
}

ScreenshotVerb::ScreenshotVerb(const Document& doc, 
							   ViewBase* view, 
							   Game* game)
	: Verb(game->getDataModel().get(), "Screenshot")
	, doc(doc)
	, game(game)
	, view(view)
{
	boost::shared_ptr<DataModel> dataModel = game->getDataModel();
	dataModel->screenshotReadySignal.connect(
		boost::bind(&ScreenshotVerb::screenshotFinished, this, _1));
}

void ScreenshotVerb::doIt(IDataState* dataState)
{
	boost::shared_ptr<DataModel> dataModel = game->getDataModel();

	if (!dataModel)
		return;

	dataModel->submitTask(boost::bind(&DataModel::TakeScreenshotTask,
		boost::weak_ptr<DataModel>(dataModel)), DataModelJob::Write);
}

// TODO: Why Facebook?
// TODO: Make non-static
static void PostImageFinished(std::string *response, std::exception *ex, weak_ptr<RBX::DataModel> weakDataModel)
{
	if(shared_ptr<RBX::DataModel> dataModel = weakDataModel.lock())
	{
		if ((ex == NULL) && (response->compare("ok") == 0))
			dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Image uploaded to Facebook", 2), RBX::DataModelJob::Write);
		else
		{
			dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Failed to upload image", 2), RBX::DataModelJob::Write);
			RBX::GameSettings::singleton().setPostImageSetting(RBX::GameSettings::ASK);
		}

		dataModel->submitTask(boost::bind(&RBX::DataModel::ScreenshotUploadTask, weak_ptr<RBX::DataModel>(dataModel), true), RBX::DataModelJob::Write);
	}
}

void ScreenshotVerb::screenshotFinished(const std::string &filename)
{
	view->getAndClearDoScreenshot();

	boost::shared_ptr<DataModel> dataModel = game->getDataModel();
	dataModel->submitTask(boost::bind(&DataModel::ShowMessage,
		weak_ptr<DataModel>(dataModel), 0, "Screenshot saved", 2.0),
		DataModelJob::Write);

	
	switch (RBX::GameSettings::singleton().getPostImageSetting()) {
		case RBX::GameSettings::ASK:
		{
			// TODO We should ASK user here from UI thread.
			// image upload is disabled anyway on web site, so it looks OK just to file bug to fix this later
			// deffect id - DE3515
			doc.GetMarshaller()->Submit(boost::bind(&ScreenshotVerb::askUploadScreenshot, this, filename));
			break;
		}
		case RBX::GameSettings::ALWAYS:
		{
			uploadScreenshot(filename);
			break;
		}
		case RBX::GameSettings::NEVER:
			break;
	}
	
}

void ScreenshotVerb::askUploadScreenshot(std::string filename)
{
	// Random parameter to force refresh
	char n[16];
	itoa(rand(), n, 10);
    std::string pictureDir = RBX::FileSystem::getUserDirectory(true, RBX::DirPicture).string();
	std::string url = RBX::format("%s/UploadMedia/PostImage.aspx?seostr=%s&filename=%s&screenshotdir=%s&from=client&rand=", 
		GetBaseURL().c_str(), 
		doc.GetSEOStr().c_str(), 
		filename.c_str(), 
		pictureDir.c_str());
	url += n;

	WebBrowserAxDialog dlg(url, game->getDataModel());
	dlg.DoModal();
}

void ScreenshotVerb::uploadScreenshot(const std::string& filename)
{
	boost::shared_ptr<DataModel> dataModel = game->getDataModel();

	if (!dataModel)
		return;

	try
	{
		std::string seostr = doc.GetSEOStr();

		std::string url = RBX::format("%s/UploadMedia/DoPostImage.ashx?from=client", GetBaseURL().c_str());
		RBX::Http http(url);
		// in case the seo info contains nothing but whitespaces, add a line break to prevent facebook from returning errors
		http.additionalHeaders[seostr] = seostr + "%0D%0A";
		shared_ptr<std::ifstream> in(new std::ifstream);
		in->open(filename.c_str(), std::ios::binary);
		if (in->fail())
		{
			dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Failed to upload image", 2), RBX::DataModelJob::Write);
		}
		else
		{
			dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Uploading image ...", 0), RBX::DataModelJob::Write);

			dataModel->submitTask(boost::bind(&RBX::DataModel::ScreenshotUploadTask, weak_ptr<RBX::DataModel>(dataModel), false), RBX::DataModelJob::Write);
			http.post(in, Http::kContentTypeDefaultUnspecified, false,
				boost::bind(&PostImageFinished, _1, _2, weak_ptr<RBX::DataModel>(dataModel)));
		}
	}
	catch (...)
	{
		dataModel->submitTask(boost::bind(&RBX::DataModel::ShowMessage, weak_ptr<RBX::DataModel>(dataModel), 1, "Failed to upload image", 2), RBX::DataModelJob::Write);

		dataModel->submitTask(boost::bind(&RBX::DataModel::ScreenshotUploadTask, weak_ptr<RBX::DataModel>(dataModel), true), RBX::DataModelJob::Write);
	}
}

void RecordToggleVerb::action()
{
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
	while(!stop)
	{
		jobWait.Wait();
		if (!stop)
		{
			{
				DataModel::scoped_write_transfer request(game->getDataModel().get());
				job();
			}
			jobDone.Set();
		}
	}
	CoUninitialize();
	threadDone.Set();
}

RecordToggleVerb::RecordToggleVerb(const Document& doc,
								   View* view,
								   Game* game)
	: Verb(game->getDataModel().get(), "RecordToggle")
	, doc(doc)
    , view(view)
	, game(game)
	, stop(false)
	, jobWait(false)
	, jobDone(false)
	, threadDone(false)
	, videoUploadingEnabled(true)
{
	helper.reset(new boost::thread(boost::bind(&RecordToggleVerb::action,
		this)));

    DSVideoCaptureEngine* videoCaptureEngine = new DSVideoCaptureEngine();
    VideoControl* videoControlPtr = new VideoControl(videoCaptureEngine,
        view->GetGfxView(), view->GetGfxView()->getFrameRateManager(), this);

    videoControl.reset(videoControlPtr);
}

RecordToggleVerb::~RecordToggleVerb()
{
	if( isSelected() )
		abortCapture();

	stop = true;
	jobWait.Set();
	threadDone.Wait();
}

bool RecordToggleVerb::isEnabled() const
{
	return GameSettings::singleton().videoCaptureEnabled
		&& isUploadingVideo();
}

bool RecordToggleVerb::isChecked() const
{
	return videoControl->isVideoRecording();
}

bool RecordToggleVerb::isSelected() const
{
	return videoControl->isVideoRecording();
}

void RecordToggleVerb::startAction()
{
	Soundscape::SoundService* soundService =
		ServiceProvider::create<Soundscape::SoundService>(
		game->getDataModel().get());

	videoControl->startRecording(soundService);

	fileName = videoControl->getFileName();

	RBX::GameSettings::singleton().videoRecordingSignal(true);
}

void RecordToggleVerb::stopAction()
{
	videoControl->stopRecording();

	GameBasicSettings& settings = GameBasicSettings::singleton();

	RBX::GameSettings::singleton().videoRecordingSignal(false);

	if (settings.getUploadVideoSetting() == GameSettings::NEVER)
		return;

	doc.GetMarshaller()->Submit(boost::bind(&RecordToggleVerb::uploadVideo, this));
}

void RecordToggleVerb::abortCapture()
{
    if( videoControl->isVideoRecording())
    {
        videoControl->stopRecording();
    }
}

void RecordToggleVerb::uploadVideo()
{
    std::string videoDir = RBX::FileSystem::getUserDirectory(true, RBX::DirVideo).string();
	std::string url = RBX::format("%s/UploadMedia/UploadVideo.aspx?from=client&videodir=%s&rand=", 
		GetBaseURL().c_str(),
		videoDir.c_str());

	// Random parameter to force refresh
	char n[16];
	itoa(rand(), n, 10);
	url += n;

	WebBrowserAxDialog browser(url, game->getDataModel(), boost::bind(&RecordToggleVerb::EnableVideUpload, this, _1));
	browser.SetFileName(fileName);
	browser.DoModal(view->GetHWnd());
}

void RecordToggleVerb::doIt(IDataState* dataState)
{
	OutputDebugString("RecordToggleVerb::doIt");

	if (videoControl->isVideoRecording())
	{
		job = boost::bind(&RecordToggleVerb::stopAction, this);
		jobWait.Set();
		jobDone.Wait();
		return;
	}

    job = boost::bind(&RecordToggleVerb::startAction, this);
    jobWait.Set();
    jobDone.Wait();
}

VideoControl* RecordToggleVerb::GetVideoControl()
{
	return videoControl.get();
}

ToggleFullscreenVerb::ToggleFullscreenVerb(View& view, VerbContainer* container, VideoControl* videoControl) 
	: Verb(container, "ToggleFullScreen")
	, videoControl(videoControl)
	, view(view)
{}

bool ToggleFullscreenVerb::isChecked() const 
{
	return view.IsFullscreen();
}

bool ToggleFullscreenVerb::isEnabled() const
{
	return true; 
}

void ToggleFullscreenVerb::doIt(RBX::IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:ToggleFullscreen");

	view.SetFullscreen(!view.IsFullscreen());
}


}  // namespace RBX
