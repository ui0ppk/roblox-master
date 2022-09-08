/**
 * VideoControl.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once 

// Standard C/C++ Headers
#include <string>

// Roblox Headers
#include "util/SoundService.h"

struct IDirect3DDevice9;
struct IDirect3DSwapChain9;

namespace RBX {

class FrameRateManager;
class ViewBase;

	/*
	 * Stores functions required by the recording loop to perform audio recording
	 * Also stores whether sound recording is to be done or not.
	 */
	struct SoundState
	{

	public:
		SoundState()
		{
		}
		~SoundState() {}

		boost::function<FMOD::DSP*(FMOD_DSP_DESCRIPTION&)> createDSPFunction;
		boost::function<int()> getSampleRateFunction;
		boost::function<bool()> enabledFunction;
	};

	class IVideoCapture
	{
	public:
		virtual ~IVideoCapture() {};
		virtual bool start(int cx, int cy, SoundState  *s) = 0;
		virtual bool stop() = 0;
		virtual bool isRunning() = 0;
		virtual void setVideoQuality(int vq) = 0;
        virtual void pushNextFrame(void* device, Verb *cancelAction) = 0;
		virtual std::string &getFileName() = 0;
	};

	class VideoControl
	{
	private:
		bool recorded;

		int videoQuality;

		boost::scoped_ptr<IVideoCapture> capture;
		boost::scoped_ptr<SoundState> soundState;
		RBX::ViewBase *rbxView;
		FrameRateManager *frameRateManager;
		Verb *verb;

        void onFrameData(void* device);
	public:
		VideoControl(IVideoCapture *capture, RBX::ViewBase *rbxView, FrameRateManager *frameRateManager, Verb *verb);

		void unPause();
		void pause();
		void stopRecording();
		void startRecording(RBX::Soundscape::SoundService *soundservice);
		bool isReadyToUpload();
		bool isVideoPaused();
		bool isVideoRecording();
		bool isVideoRecordingStopped();
		void setVideoQuality(int vq);
		std::string &getFileName() { return capture->getFileName(); }
	};

}
