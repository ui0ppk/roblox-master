#pragma once

#include "v8tree/Verb.h"
#include "rbx/CEvent.h"

namespace RBX {

class Game;
class View;
class ViewBase;
class Document;
class VideoControl;
class WebBrowserDialog;
class Application;

// Request to leave the game.  Results in process shutdown.
class LeaveGameVerb : public Verb
{
	View& v;

public:
	LeaveGameVerb(View& v, VerbContainer* container);
    virtual ~LeaveGameVerb(){}
	virtual void doIt(IDataState* dataState);
};

// Request to take screenshot of current game
class ScreenshotVerb : public RBX::Verb
{
	Game* game;
	ViewBase* view;
	const Document& doc;

	void screenshotFinished(const std::string &filename);
	void askUploadScreenshot(std::string filename);
	void uploadScreenshot(const std::string &filename);

public:
	ScreenshotVerb(const Document& doc, ViewBase* view, Game* game);
	virtual void doIt(IDataState* dataState);
	virtual bool isEnabled() const { return true; }
};

// Request to record gameplay video
class RecordToggleVerb : public Verb
{
	const Document& doc;
    View* view;
	Game* game;
	boost::scoped_ptr<VideoControl> videoControl;
	
	// Path to where the file video is being saved.
	std::string fileName;

	bool videoUploadingEnabled;

	bool stop;
	boost::scoped_ptr<boost::thread> helper;
	boost::function<void()> job;

	CEvent jobWait;
	CEvent jobDone;
	CEvent threadDone;

	void action();

	// Indicates whether the a video is being uploaded.
	bool isUploadingVideo() const { return videoUploadingEnabled; }
	void EnableVideUpload(bool enable) { videoUploadingEnabled = enable; }

	// Uploads a video to the interwebs.
	void uploadVideo();

    // stops the recording, does not prompt or signal
    void abortCapture();
public:
	RecordToggleVerb(const Document& doc, View* view, Game* game);

	~RecordToggleVerb();

	virtual bool isEnabled() const;
	virtual bool isChecked() const;
	virtual bool isSelected() const;

	void startAction();
	void stopAction();

	virtual void doIt(IDataState* dataState);

	VideoControl* GetVideoControl();
};

// Request to toggle fullscreen
class ToggleFullscreenVerb : public RBX::Verb
{

private:
	// Needed because toggle fullscreen is disabled while recording
	VideoControl* videoControl;
	View& view;

public:
	ToggleFullscreenVerb(View& view, VerbContainer* container, VideoControl* videoControl);
	virtual bool isChecked() const;
	virtual bool isEnabled() const;
	virtual void doIt(RBX::IDataState* dataState);
};


}  // namespace RBX
