/**
 * DSVideoCaptureEngine.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include "VideoControl.h"

// System Headers
#ifndef RBX_PLATFORM_DURANGO
#include <atlcomcli.h>
#endif // RBX_PLATFORM_DURANGO

struct IMediaControl;
struct IGraphBuilder;

namespace RBX {

	namespace DS {
		class CVideoStreamFilter;
		class CAudioStreamFilter;

		class IAudioTime
		{
		public:
			virtual LONG GetTime() = 0;
			virtual LONG GetAbsoluteTime() = 0;
		};
	}

	class DSVideoCaptureEngine : public IVideoCapture, public DS::IAudioTime
	{
	public:
		DSVideoCaptureEngine();
		virtual ~DSVideoCaptureEngine();
		virtual bool start(int cx, int cy, SoundState  *s);
		virtual bool stop();
		virtual bool isRunning();
		virtual void setVideoQuality(int vq);
        virtual void pushNextFrame(void* device, Verb *cancelAction);
		virtual std::string & getFileName() { return fullFileName; };

		virtual LONG GetTime();
		virtual LONG GetAbsoluteTime();
	private:
		const double defNx;
		const double defNy;
		const LONG MaxRecordTime;

		HRESULT BuildCaptureGraph(int cx, int cy, bool forceNoAudio);
		HRESULT BuildCaptureGraphNoThrow(int cx, int cy, bool forceNoAudio);

		void DestroyCaptureGgaph();
		int GenerateFileName(LPWSTR fileName);
		void GetSquareSizes(int cx, int cy, int &ncx, int &ncy);

		DWORD startTime;

		IGraphBuilder *graph;
		SoundState *soundState;
		IMediaControl *mediaControl;
		DS::CVideoStreamFilter *videoSource;
		DS::CAudioStreamFilter *audioSource;

		int framesPushed;
		LONG lastPushedVideoFrameTime;
		std::string fullFileName; // TODO: replace with boost::filesystem::path

        unsigned char* frameData;
        unsigned frameDataSize;
	};
}
