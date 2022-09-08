/**
 * VideoControl.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "VideoControl.h"

// System Headers
#define NOMINMAX        // need to define this before windows.h
#include <d3d9.h>
#include <d3dx9.h>

// Roblox Headers
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/GameSettings.h"
#include "util/FileSystem.h"
#include "util/standardout.h"
#include "GfxBase/FrameRateManager.h"
#include "GfxBase/ViewBase.h"

using namespace RBX;

namespace RBX{

	
	static void logError(std::string errorString)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "%s\r\n", errorString.c_str());
	}

	VideoControl::VideoControl(IVideoCapture *capture, RBX::ViewBase *rbxView, FrameRateManager *frameRateManager, Verb *verb)
	{
		this->capture.reset(capture);
		RBXASSERT(verb);
		this->verb = verb;

		RBXASSERT(rbxView);
		this->rbxView = rbxView;
		this->frameRateManager = frameRateManager;

		recorded = false;
		videoQuality =  -1;
		setVideoQuality(GameSettings::singleton().getVideoQualitySetting() );
	}

	bool VideoControl::isVideoRecordingStopped() 	
	{
		return !capture->isRunning();
	}

	bool VideoControl::isVideoRecording()
	{	
		return capture->isRunning();
	}

	bool VideoControl::isVideoPaused()
	{
		return false;
	}

	bool VideoControl::isReadyToUpload()
	{
		return recorded && !capture->isRunning();
	}

	void VideoControl::startRecording(RBX::Soundscape::SoundService *soundservice)
	{
		setVideoQuality(GameSettings::singleton().getVideoQualitySetting());
		RBXASSERT(soundservice);

		if(soundservice)
		{
			soundState.reset(new SoundState());

			soundState->createDSPFunction = boost::bind(&RBX::Soundscape::SoundService::createDSP, soundservice, _1);
			soundState->getSampleRateFunction = boost::bind(&RBX::Soundscape::SoundService::getSampleRate, soundservice);
			soundState->enabledFunction = boost::bind(&RBX::Soundscape::SoundService::enabled, soundservice);
		}	

        std::pair<unsigned, unsigned> dimensions = rbxView->setFrameDataCallback(boost::bind(&VideoControl::onFrameData, this, _1));

        bool captureStarted = dimensions.first && dimensions.second && capture->start(dimensions.first, dimensions.second, soundState.get());
        RBXASSERT(captureStarted);

		if (captureStarted)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,  "Video recording started");
			frameRateManager->PauseAutoAdjustment();
			recorded = false;
		}
	}

	void VideoControl::stopRecording()
	{
		recorded = true;

		capture->stop();
		frameRateManager->ResumeAutoAdjustment();

        rbxView->setFrameDataCallback(boost::function<void(void*)>());
	}

	void VideoControl::pause()
	{
	}

	void VideoControl::unPause()
	{
	}

	void VideoControl::setVideoQuality(int vq)
	{	
		capture->setVideoQuality(vq);
	}

    void VideoControl::onFrameData(void* device)
    {
		if (capture->isRunning())
		{
			capture->pushNextFrame(device, verb);
		}
    }
}
