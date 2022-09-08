/**
 * DSVideoCaptureEngine.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "DSVideoCaptureEngine.h"

// 3rd Party Headers
#undef INTSAFE_E_ARITHMETIC_OVERFLOW
#include "streams.h"

// SystemHeaders
#include <initguid.h>
#include <WMSysPrf.h>
#include <wmsdk.h>
#include <Dshowasf.h>
#include <control.h>

// 3rd Party Headers
#include "fmod.h"
#include "fmod.hpp"

// Roblox Headers
#include "rbx/Debug.h"
#include "util/FileSystem.h"
#include "util/Statistics.h"
#include "GfxCore/Device.h"
#include "GfxCore/Framebuffer.h"
#include "V8DataModel/GuiService.h" 

const char VidCapID[] = "VideoCapture";

#define ERROR_ON_FAIL(expr)\
	hr = expr;\
	if (FAILED(hr))\
	{\
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,  "Call failed 0x%x", hr);\
		goto Error;\
	}\

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

using namespace RBX;

LOGGROUP(VideoCapture)
LOGGROUP(DeviceLost)

namespace RBX {

	namespace DS {
		// {172325B6-8DA1-4DF0-A5B4-428E382E2968}
		DEFINE_GUID(CLSID_GameVideoStream, 
		0x172325b6, 0x8da1, 0x4df0, 0xa5, 0xb4, 0x42, 0x8e, 0x38, 0x2e, 0x29, 0x68);

		// {C75FC93F-0FD9-4fea-895D-5F5BFB59F621}
		DEFINE_GUID(CLSID_GameAudioStream, 
		0xc75fc93f, 0xfd9, 0x4fea, 0x89, 0x5d, 0x5f, 0x5b, 0xfb, 0x59, 0xf6, 0x21);

		const LPWSTR RbxVideoId = L"RBXVideo";
		const LPWSTR VideoFileExt = L"wmv";
		const LPWSTR ProcTitle = L"robloxapp";
		const LPWSTR RbxAudioId = L"RBXAudio";
		const int MaxWaitTime = 500; //2fps is very bad
		const int MaxFramesQueue = 5; // no more than 5 frames kept (will prevent out of memory on slow machines)
		const int DefSampleRate = 48000;

		class CVideoStream;

		class CVideoStreamFilter : public CSource
		{
		public:
			CVideoStreamFilter(LPUNKNOWN lpunk, HRESULT *phr, int cx, int cy);

			IPin *GetPin() const { return m_paStreams[0]; }
			void pushNextFrame(BYTE *data, int size, int cx, int cy, int bpp);
			int GetTargetFrameRate() const;
			int GetActualFrameRate() const;
			void SetAudioClock(IAudioTime *clock);
			void GetCaptureSize(int &width, int &height);
			int GetQueueSize();
		private:
		};

		//------------------------------------------------------------------------------
		// Class CVideoStream
		//
		// This class implements the stream which is used to output the images from game into encoding pipeline
		//------------------------------------------------------------------------------
		class CVideoStream : public CSourceStream
		{

		public:
			CVideoStream(HRESULT *phr, CVideoStreamFilter *pParent, LPCWSTR pPinName, int cx, int cy);
			~CVideoStream();

			HRESULT FillBuffer(IMediaSample *pms);
			HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties);
			HRESULT SetMediaType(const CMediaType *pMediaType);
			HRESULT CheckMediaType(const CMediaType *pMediaType);
			HRESULT GetMediaType(int iPosition, CMediaType *pmt);
			HRESULT OnThreadCreate(void);

			STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);

			void pushNextFrame(BYTE *data, int size, int cx, int cy, int bpp);
			int GetTargetFrameRate() const { return targetFrameRateTime; }
			int GetActualFrameRate() const { return frameRate; };
			void SetAudioClock(IAudioTime *clock);
			void GetCaptureSize(int &width, int &height);
			int GetQueueSize();
		private:
			CCritSec framesQueueLock;
			CAMEvent frameReadyEvent;

			std::queue<std::pair<BYTE*, int>> framesQueue;
			IAudioTime *audioTime;

			int height;
			int width;
			const int targetFrameRateTime;
			int frameRate;

			int pushedFrames;

			CCritSec m_cSharedState;            // Lock on m_rtSampleTime and m_Ball
			CRefTime m_rtSampleTime;            // The time stamp for each sample
		};


		class CAudioStream;

		class CAudioStreamFilter : public CSource
		{
		public:
			// It is only allowed to to create these objects with CreateInstance
			CAudioStreamFilter(LPUNKNOWN lpunk, HRESULT *phr, SoundState  *s, DS::IAudioTime *t);

			IPin *GetPin() const { return m_paStreams[0]; }
			int GetExpectedFramesCount(int frameDelay) const;
			IAudioTime *GetAudioTime();
			void StopSoundCapture();
		private:
		};

		//------------------------------------------------------------------------------
		// Class CAudioStream
		//
		//------------------------------------------------------------------------------
		class CAudioStream : public CSourceStream, public IAudioTime
		{

		public:
			CAudioStream(HRESULT *phr, CAudioStreamFilter *pParent, LPCWSTR pPinName, SoundState  *s, DS::IAudioTime *t);
			~CAudioStream();

			HRESULT FillBuffer(IMediaSample *pms);
			HRESULT DecideBufferSize(IMemAllocator *pIMemAlloc,	ALLOCATOR_PROPERTIES *pProperties);
			HRESULT SetMediaType(const CMediaType *pMediaType);
			HRESULT CheckMediaType(const CMediaType *pMediaType);
			HRESULT GetMediaType(int iPosition, CMediaType *pmt);
			HRESULT OnThreadCreate(void);
			STDMETHODIMP Notify(IBaseFilter * pSender, Quality q);
			int GetExpectedFramesCount(int frameDelay) const;
			virtual LONG GetTime() { return m_rtSampleTime.Millisecs(); }
			virtual LONG GetAbsoluteTime();
			void StopSoundCapture();
		private:
			static FMOD_RESULT F_CALLBACK audioGrabber(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels);

			CCritSec framesQueueLock;
			CAMEvent frameReadyEvent;

			std::queue<std::pair<short*,int>> framesQueue;

			CCritSec m_cSharedState;            // Lock on m_rtSampleTime and m_Ball
			CRefTime m_rtSampleTime;            // The time stamp for each sample
			SoundState *soundState;
			bool firstSampleDelivered;
			LONGLONG sampleMediaTimeStart;
			FMOD_DSP_DESCRIPTION  dspdesc;
			FMOD::DSP *dsp;
			DS::IAudioTime *time;
		};

		CVideoStreamFilter::CVideoStreamFilter(LPUNKNOWN lpunk, HRESULT *phr, int cx, int cy) :
			CSource(NAME("GameVideoStream"), lpunk, CLSID_GameVideoStream)
		{
			ASSERT(phr);
			CAutoLock cAutoLock(&m_cStateLock);

			m_paStreams = (CSourceStream **) new CVideoStream*[1];
			if(m_paStreams == NULL)
			{
				if(phr)
					*phr = E_OUTOFMEMORY;

				return;
			}

			m_paStreams[0] = new CVideoStream(phr, this, RbxVideoId, cx, cy);
			if(m_paStreams[0] == NULL)
			{
				if(phr)
					*phr = E_OUTOFMEMORY;

				return;
			}
		}

		void CVideoStreamFilter::pushNextFrame(BYTE *data, int size, int cx, int cy, int bpp)
		{
			(reinterpret_cast<CVideoStream*>(m_paStreams[0]))->pushNextFrame(data, size, cx, cy, bpp);
		}

		int CVideoStreamFilter::GetTargetFrameRate() const 
		{ 
			return (reinterpret_cast<const CVideoStream*>(m_paStreams[0]))->GetTargetFrameRate(); 
		}

		int CVideoStreamFilter::GetActualFrameRate() const
		{
			return (reinterpret_cast<const CVideoStream*>(m_paStreams[0]))->GetActualFrameRate(); 
		}

		void CVideoStreamFilter::SetAudioClock(IAudioTime *clock)
		{
			return (reinterpret_cast<CVideoStream*>(m_paStreams[0]))->SetAudioClock(clock); 
		}

		void CVideoStreamFilter::GetCaptureSize(int &width, int &height)
		{
			(reinterpret_cast<CVideoStream*>(m_paStreams[0]))->GetCaptureSize(width, height);
		}

		int CVideoStreamFilter::GetQueueSize()
		{
			return (reinterpret_cast<CVideoStream*>(m_paStreams[0]))->GetQueueSize();
		}

		CVideoStream::CVideoStream(HRESULT *phr, CVideoStreamFilter *pParent, LPCWSTR pPinName, int cx, int cy) :
			CSourceStream(NAME("GameVideoStream"),phr, pParent, pPinName),
			width(cx),
			height(cy),
			targetFrameRateTime(55),
			frameRate(0),
			pushedFrames(0)
		{
			ASSERT(phr);
			CAutoLock cAutoLock(&m_cSharedState);
		}

		CVideoStream::~CVideoStream() 
		{
			CAutoLock cAutoLock(&m_cSharedState);
			CAutoLock lock(&framesQueueLock);
			while (framesQueue.size())
			{
				std::pair<BYTE*,int> frameData = framesQueue.front();
				delete [] frameData.first;
				framesQueue.pop();
			}

			frameReadyEvent.Set();
		}

		void CVideoStream::SetAudioClock(IAudioTime *clock)
		{
			audioTime = clock;
		}

		void CVideoStream::pushNextFrame(BYTE *data, int size, int cx, int cy, int bpp)
		{
            CAutoLock lock(&framesQueueLock);

			framesQueue.push(std::pair<BYTE*,int>(data, size));
			frameReadyEvent.Set();
		}

		int CVideoStream::GetQueueSize()
		{
			CAutoLock lock(&framesQueueLock);

			return framesQueue.size();
		}

		void CVideoStream::GetCaptureSize(int &width, int &height)
		{
			width = this->width;
			height = this->height;
		}

		HRESULT CVideoStream::FillBuffer(IMediaSample *pms)
		{
			CheckPointer(pms,E_POINTER);

			BYTE *pData;
			long lDataLen;

			pms->GetPointer(&pData);
			lDataLen = pms->GetSize();
			BYTE *frame = NULL;
			{
				CAutoLock cAutoLockShared(&m_cSharedState);

				framesQueueLock.Lock();

				if (framesQueue.size() == 0) {
					framesQueueLock.Unlock();
					frameReadyEvent.Wait(MaxWaitTime);
					framesQueueLock.Lock();
				}
				frameReadyEvent.Reset();

				if (!framesQueue.empty())
				{
					std::pair<BYTE*, int> frameData = framesQueue.front();

					pushedFrames ++;
					frameRate = audioTime->GetAbsoluteTime() / pushedFrames;

					RBXASSERT(lDataLen == frameData.second);
					frame = frameData.first;

					RBXASSERT(frame);
					memcpy(pData, frame, lDataLen);

					delete [] frame;
					framesQueue.pop();
				}

				framesQueueLock.Unlock();

				// The current time is the sample's start
				CRefTime rtStart = m_rtSampleTime;

				// Increment to find the finish time
				m_rtSampleTime += (LONG)frameRate;

				pms->SetTime((REFERENCE_TIME *) &rtStart,(REFERENCE_TIME *) &m_rtSampleTime);
			}

			pms->SetSyncPoint(TRUE);
			return NOERROR;
		}

		//
		// Notify
		//
		// Alter the repeat rate according to quality management messages sent from
		// the downstream filter (often the renderer).  Wind it up or down according
		// to the flooding level - also skip forward if we are notified of Late-ness
		//
		STDMETHODIMP CVideoStream::Notify(IBaseFilter * pSender, Quality q)
		{
			return NOERROR;
		}

		HRESULT CVideoStream::GetMediaType(int iPosition, CMediaType *pmt)
		{
			CheckPointer(pmt,E_POINTER);

			CAutoLock cAutoLock(m_pFilter->pStateLock());
			if(iPosition < 0) {
				return E_INVALIDARG;
			}

			if(iPosition > 0) {
				return VFW_S_NO_MORE_ITEMS;
			}

			VIDEOINFO *pvi = (VIDEOINFO *) pmt->AllocFormatBuffer(sizeof(VIDEOINFO));
			if(NULL == pvi)
				return(E_OUTOFMEMORY);

			ZeroMemory(pvi, sizeof(VIDEOINFO));

			switch(iPosition)
			{
				case 0:
				{    
					pvi->bmiHeader.biCompression = BI_RGB;
					pvi->bmiHeader.biBitCount    = 32;
					break;
				}
			}

			// (Adjust the parameters common to all formats...)
			pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
			pvi->bmiHeader.biWidth      = width;
			pvi->bmiHeader.biHeight     = height;
			pvi->bmiHeader.biPlanes     = 1;
			pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
			pvi->bmiHeader.biClrImportant = 0;

			SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
			SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

			pmt->SetType(&MEDIATYPE_Video);
			pmt->SetFormatType(&FORMAT_VideoInfo);
			pmt->SetTemporalCompression(FALSE);

			// Work out the GUID for the subtype from the header info.
			const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
			pmt->SetSubtype(&SubTypeGUID);
			pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);

			return NOERROR;
		}


		HRESULT CVideoStream::CheckMediaType(const CMediaType *pMediaType)
		{
			CheckPointer(pMediaType,E_POINTER);

			if((*(pMediaType->Type()) != MEDIATYPE_Video) || !(pMediaType->IsFixedSize())) {                                                  
				return E_INVALIDARG;
			}

			// Check for the subtypes we support
			const GUID *SubType = pMediaType->Subtype();
			if (SubType == NULL)
				return E_INVALIDARG;

			if(*SubType != MEDIASUBTYPE_RGB32) {
				return E_INVALIDARG;
			}

			// Get the format area of the media type
			VIDEOINFO *pvi = (VIDEOINFO *) pMediaType->Format();

			if(pvi == NULL)
				return E_INVALIDARG;

			// Check the image size. As my default ball is 10 pixels big
			// look for at least a 20x20 image. This is an arbitary size constraint,
			// but it avoids balls that are bigger than the picture...

			if((pvi->bmiHeader.biWidth < 20) || ( abs(pvi->bmiHeader.biHeight) < 20)) {
				return E_INVALIDARG;
			}

			// Check if the image width & height have changed
			if(pvi->bmiHeader.biWidth != width || abs(pvi->bmiHeader.biHeight) != height) {
				// If the image width/height is changed, fail CheckMediaType() to force
				// the renderer to resize the image.
				return E_INVALIDARG;
			}

			return S_OK;  // This format is acceptable.
		}


		HRESULT CVideoStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
		{
			CheckPointer(pAlloc,E_POINTER);
			CheckPointer(pProperties,E_POINTER);

			CAutoLock cAutoLock(m_pFilter->pStateLock());
			HRESULT hr = NOERROR;

			VIDEOINFO *pvi = (VIDEOINFO *) m_mt.Format();
			pProperties->cBuffers = 1;
			pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

			ASSERT(pProperties->cbBuffer);

			// Ask the allocator to reserve us some sample memory, NOTE the function
			// can succeed (that is return NOERROR) but still not have allocated the
			// memory that we requested, so we must check we got whatever we wanted

			ALLOCATOR_PROPERTIES Actual;
			hr = pAlloc->SetProperties(pProperties, &Actual);
			if(FAILED(hr))
			{
				return hr;
			}

			if(Actual.cbBuffer < pProperties->cbBuffer)
			{
				return E_FAIL;
			}

			// Make sure that we have only 1 buffer (we erase the ball in the
			// old buffer to save having to zero a 200k+ buffer every time
			// we draw a frame)

			ASSERT(Actual.cBuffers == 1);
			return NOERROR;
		}

		HRESULT CVideoStream::SetMediaType(const CMediaType *pMediaType)
		{
			CAutoLock cAutoLock(m_pFilter->pStateLock());

			HRESULT hr = CSourceStream::SetMediaType(pMediaType);

			if(SUCCEEDED(hr)) {
				VIDEOINFO * pvi = (VIDEOINFO *) m_mt.Format();
				if (pvi == NULL)
					return E_UNEXPECTED;

				switch(pvi->bmiHeader.biBitCount) {
					case 32:
						break;

					default:
						// We should never agree any other pixel sizes
						RBXASSERT(0);
						break;
				}

				return NOERROR;
			} 
			return hr;
		}


		//
		// OnThreadCreate
		//
		// As we go active reset the stream time to zero
		//
		HRESULT CVideoStream::OnThreadCreate()
		{
			CAutoLock cAutoLockShared(&m_cSharedState);
			m_rtSampleTime = 0;

			return NOERROR;
		} // OnThreadCreate

		CAudioStreamFilter::CAudioStreamFilter(LPUNKNOWN lpunk, HRESULT *phr, SoundState  *s, DS::IAudioTime *t) :
			CSource(NAME("GameAudioStream"), lpunk, CLSID_GameAudioStream)
		{
			ASSERT(phr);
			CAutoLock cAutoLock(&m_cStateLock);

			m_paStreams = (CSourceStream **) new CAudioStream*[1];
			if(m_paStreams == NULL)
			{
				if(phr)
					*phr = E_OUTOFMEMORY;

				return;
			}

			m_paStreams[0] = new CAudioStream(phr, this, RbxAudioId, s, t);
			if(m_paStreams[0] == NULL)
			{
				if(phr)
					*phr = E_OUTOFMEMORY;

				return;
			}
		}

		int CAudioStreamFilter::GetExpectedFramesCount(int frameDelay) const 
		{
			return (reinterpret_cast<const CAudioStream*>(m_paStreams[0]))->GetExpectedFramesCount(frameDelay);
		};

		IAudioTime *CAudioStreamFilter::GetAudioTime()
		{
			return (reinterpret_cast<CAudioStream*>(m_paStreams[0]));
		}

		void CAudioStreamFilter::StopSoundCapture()
		{
			return (reinterpret_cast<CAudioStream*>(m_paStreams[0]))->StopSoundCapture();
		}

		CAudioStream::CAudioStream(HRESULT *phr, CAudioStreamFilter *pParent, LPCWSTR pPinName, SoundState *s, DS::IAudioTime *t) :
			CSourceStream(NAME("GameVideoStream"),phr, pParent, pPinName),
			soundState(s),
			time(t),
			firstSampleDelivered(false),
			sampleMediaTimeStart(0)
		{
			FMOD_DSP_DESCRIPTION  dspdesc; 
			memset(&dspdesc, 0, sizeof(FMOD_DSP_DESCRIPTION)); 

			strncpy_s(dspdesc.name, sizeof(dspdesc.name)/sizeof(dspdesc.name[0]), "SCRBX", sizeof(dspdesc.name)/sizeof(dspdesc.name[0])); 
			//dspdesc.channels     = 0;                   // 0 = whatever comes in, else specify, no longer a property in FMOD STUDIO
			dspdesc.read         = audioGrabber; 
			dspdesc.userdata     = (void *)this; 

			dsp = s->createDSPFunction(dspdesc);

			if (!dsp)
			{
				*phr = E_FAIL;
			}
		}

		CAudioStream::~CAudioStream()
		{
			if (dsp)
			{
				dsp->setBypass(true);
				dsp->release();
			}

			CAutoLock lock(&framesQueueLock);
			while (framesQueue.size())
			{
				std::pair<short*, int> frameInfo = framesQueue.front();
				delete [] frameInfo.first;
				framesQueue.pop();
			}

			frameReadyEvent.Set();
		}

		LONG CAudioStream::GetAbsoluteTime()
		{
			return time->GetAbsoluteTime();
		}

		int CAudioStream::GetExpectedFramesCount(int frameDelay) const
		{
			return (int)(const_cast<CRefTime &>(m_rtSampleTime).Millisecs() / frameDelay);
		}

		void CAudioStream::StopSoundCapture()
		{
			if (dsp)
			{
				dsp->setBypass(true);
				dsp->release();
				dsp = NULL;
			}
			frameReadyEvent.Set();
		}

		FMOD_RESULT F_CALLBACK CAudioStream::audioGrabber(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int *outchannels)
		{
			FMOD::DSP *dsp = (FMOD::DSP *)dsp_state->instance;
			void *userdata = NULL;
			dsp->getUserData((void **)&userdata);
			CAudioStream *stream = (CAudioStream*)userdata;
			const unsigned short videoChannelsCount = 2;
			short *soundData = new short[videoChannelsCount*length];
			RBXASSERT(inchannels == *outchannels);

			for (unsigned int count = 0; count < length; count++) 
			{ 
				for (int count2 = 0; count2 < *outchannels; count2++)
				{
					outbuffer[(count * (*outchannels)) + count2] = inbuffer[(count * inchannels) + count2];

					if (count2 < videoChannelsCount)
					{
						float inFloat = inbuffer[(count * inchannels) + count2];
						inFloat *= 32768;
						int outInt = inFloat < -32768 ? -32768 : inFloat > 32767 ? 32767 : (signed short)inFloat;

						short s = (short)(outInt);
						soundData[(count * videoChannelsCount) + count2] = s;
					}
				}
			}

			CAutoLock lock(&stream->framesQueueLock);

			stream->framesQueue.push(std::pair<short*, int>(soundData, videoChannelsCount*length));
			stream->frameReadyEvent.Set();

			return FMOD_OK;
		}


		HRESULT CAudioStream::FillBuffer(IMediaSample *pms) 
		{
			CheckPointer(pms, E_POINTER);

			BYTE *pData;

			HRESULT hr = pms->GetPointer(&pData);
			if (FAILED(hr)) 
			{
				return hr;
			}

			// This lock must be held because this function uses
			// m_dwTempPCMBufferSize, m_hPCMToMSADPCMConversionStream,
			// m_rtSampleTime, m_fFirstSampleDelivered and
			// m_llSampleMediaTimeStart.
			CAutoLock lShared(&m_cSharedState);

			WAVEFORMATEX* pwfexCurrent = (WAVEFORMATEX*)m_mt.Format();

			RBXASSERT(pwfexCurrent->wBitsPerSample == 16 && pwfexCurrent->nChannels == 2);

			short *sound = NULL;
			int actualLen = 0;
			framesQueueLock.Lock();

			if (framesQueue.size() == 0) {
				framesQueueLock.Unlock();
				frameReadyEvent.Wait(MaxWaitTime);
				framesQueueLock.Lock();
			}
			frameReadyEvent.Reset();

			if (!framesQueue.empty())
			{
				std::pair<short*, int> frameInfo = framesQueue.front();
				sound = frameInfo.first;
				actualLen = frameInfo.second*sizeof(short);
				framesQueue.pop();

				RBXASSERT(sound);
				memcpy(pData, sound, actualLen);
				delete [] sound;
			}

			framesQueueLock.Unlock();

			hr = pms->SetActualDataLength(actualLen);
			if (FAILED(hr))
				return hr;

			// Set the sample's time stamps.  
			CRefTime rtStart = m_rtSampleTime;

			m_rtSampleTime = rtStart + (REFERENCE_TIME)(UNITS * pms->GetActualDataLength()) / 
				(REFERENCE_TIME)pwfexCurrent->nAvgBytesPerSec;

			hr = pms->SetTime((REFERENCE_TIME*)&rtStart, (REFERENCE_TIME*)&m_rtSampleTime);

			if (FAILED(hr)) {
				return hr;
			}

			// Set the sample's properties.
			hr = pms->SetPreroll(FALSE);
			if (FAILED(hr)) {
				return hr;
			}

			hr = pms->SetMediaType(NULL);
			if (FAILED(hr)) {
				return hr;
			}

			hr = pms->SetDiscontinuity(!firstSampleDelivered);
			if (FAILED(hr)) {
				return hr;
			}

			hr = pms->SetSyncPoint(!firstSampleDelivered);
			if (FAILED(hr)) {
				return hr;
			}

			LONGLONG llMediaTimeStart = sampleMediaTimeStart;

			DWORD dwNumAudioSamplesInPacket = (pms->GetActualDataLength() * 8) / (pwfexCurrent->nChannels * pwfexCurrent->wBitsPerSample);

			LONGLONG llMediaTimeStop = sampleMediaTimeStart + dwNumAudioSamplesInPacket;

			hr = pms->SetMediaTime(&llMediaTimeStart, &llMediaTimeStop);
			if (FAILED(hr)) {
				return hr;
			}

			sampleMediaTimeStart = llMediaTimeStop;
			firstSampleDelivered = true;

			return NOERROR;
		}

		HRESULT CAudioStream::DecideBufferSize(IMemAllocator *pIMemAlloc, ALLOCATOR_PROPERTIES *pProperties) 
		{
			CheckPointer(pIMemAlloc,E_POINTER);
			CheckPointer(pProperties,E_POINTER);

			WAVEFORMATEX *pwfexCurrent = (WAVEFORMATEX*)m_mt.Format();

			RBXASSERT(WAVE_FORMAT_PCM == pwfexCurrent->wFormatTag);
			pProperties->cbBuffer = 16*1024;

			int nBitsPerSample = pwfexCurrent->wBitsPerSample;
			int nSamplesPerSec = pwfexCurrent->nSamplesPerSec;
			int nChannels = pwfexCurrent->nChannels;

			pProperties->cBuffers = (nChannels * nSamplesPerSec * nBitsPerSample) / (pProperties->cbBuffer * 8);

			// Get 1/2 second worth of buffers
			pProperties->cBuffers /= 2;
			if(pProperties->cBuffers < 1)
			{
				pProperties->cBuffers = 1;
			}

			// Ask the allocator to reserve us the memory

			ALLOCATOR_PROPERTIES Actual;
			HRESULT hr = pIMemAlloc->SetProperties(pProperties,&Actual);
			if(FAILED(hr))
			{
				return hr;
			}

			// Is this allocator unsuitable
			if(Actual.cbBuffer < pProperties->cbBuffer)
			{
				return E_FAIL;
			}

			return NOERROR;
		}

		HRESULT CAudioStream::SetMediaType(const CMediaType *pMediaType) 
		{
			CAutoLock cAutoLock(m_pFilter->pStateLock());

			HRESULT hr = CSourceStream::SetMediaType(pMediaType);

			return CheckMediaType(pMediaType);
		}

		HRESULT CAudioStream::CheckMediaType(const CMediaType *pMediaType)
		{
			CheckPointer(pMediaType,E_POINTER);
			CAutoLock lock(m_pFilter->pStateLock());

			CMediaType mt;
			GetMediaType(0, &mt);

			if(mt == *pMediaType)
			{
				return NOERROR;
			}

			return E_FAIL;
		}

		HRESULT CAudioStream::GetMediaType(int iPosition, CMediaType *pmt)
		{
			CheckPointer(pmt,E_POINTER);

			CAutoLock cAutoLock(m_pFilter->pStateLock());
			if(iPosition < 0) {
				return E_INVALIDARG;
			}

			if(iPosition > 0) {
				return VFW_S_NO_MORE_ITEMS;
			}

			WAVEFORMATEX *pwfex = (WAVEFORMATEX *)pmt->AllocFormatBuffer(sizeof(WAVEFORMATEX));
			if(NULL == pwfex)
				return(E_OUTOFMEMORY);

			ZeroMemory(pwfex, sizeof(WAVEFORMATEX));

			pwfex->wFormatTag = WAVE_FORMAT_PCM;
			pwfex->nChannels = 2;
			pwfex->nSamplesPerSec = DefSampleRate;
			pwfex->wBitsPerSample = 16;        
			pwfex->nBlockAlign = (WORD)((pwfex->wBitsPerSample * pwfex->nChannels) / 8);
			pwfex->nAvgBytesPerSec = pwfex->nBlockAlign * pwfex->nSamplesPerSec;
			pwfex->cbSize = 0;

			return CreateAudioMediaType(pwfex, pmt, FALSE);
		}

		HRESULT CAudioStream::OnThreadCreate(void) 
		{
			CAutoLock cAutoLockShared(&m_cSharedState);
			m_rtSampleTime = 0;
			return NOERROR;
		}
		
		STDMETHODIMP CAudioStream::Notify(IBaseFilter * pSender, Quality q)
		{
			HRESULT hr = S_OK;
			return hr;
		}
	}

	DSVideoCaptureEngine::DSVideoCaptureEngine() :
		defNx(16.0),
		defNy(9.0),
		MaxRecordTime(30*60*1000), //30 minutes in milliseconds
        frameData(NULL),
        frameDataSize(0)
	{
		HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
		switch (hr)
		{
		case S_OK:
		case S_FALSE:
			break;
		default:
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,  "CoInitializeEx result = 0x%x", hr);
		}
		graph = NULL;
		mediaControl = NULL;
		videoSource = NULL;
		audioSource = NULL;
		soundState = NULL;
		framesPushed = 0;
	}
	
	DSVideoCaptureEngine::~DSVideoCaptureEngine() {
		if (isRunning())
		{
			stop();
		}
		CoUninitialize();
	}
	
	HRESULT DSVideoCaptureEngine::BuildCaptureGraphNoThrow(int cx, int cy, bool forceNoAudio)
	{
		__try
		{
			return BuildCaptureGraph(cx, cy, false);
		}
		__except(true)
		{
			return E_FAIL;
		}
	}

	bool DSVideoCaptureEngine::start(int cx, int cy, SoundState  *s) {
		if (!isRunning())
		{
			soundState = s;
			framesPushed = 0;
			lastPushedVideoFrameTime = 0;
			int ncx, ncy;
			GetSquareSizes(cx, cy, ncx, ncy);

			HRESULT hr = S_OK;

			if (s->enabledFunction() && s->getSampleRateFunction() == DS::DefSampleRate)
			{
				hr = BuildCaptureGraphNoThrow(ncx, ncy, false);
			}
			else
			{
				hr = E_FAIL;
			}

			if (FAILED(hr))
			{
				ReportStatisticWithMessage(GetBaseURL(), VidCapID, RBX::format("Audio enabled = %d, Expected frame freq = 48000 actual freq = %d", (int)s->enabledFunction(), s->getSampleRateFunction()));

				DestroyCaptureGgaph();
				hr = BuildCaptureGraph(ncx, ncy, true);
			}

			RBXASSERT(graph);
			RBXASSERT(mediaControl);
			
			hr = mediaControl->Run();
			if (!SUCCEEDED(hr))
			{
				ReportStatisticWithMessage(GetBaseURL(), VidCapID, RBX::format("Video capture startup failed error = %d", hr));
				return false;
			}

            frameDataSize = cx*cy*4;
            frameData = new unsigned char[frameDataSize];
		}
		return true;
	}
	
	bool DSVideoCaptureEngine::stop() {

		bool running = isRunning();
		FASTLOG1(FLog::VideoCapture, "Stopping video capture. IsRunning: %b", running);
		HRESULT hr = S_OK;

        if (frameData)
        {
            delete[] frameData;
            frameData = NULL;
            frameDataSize = 0;
        }
        

		if (running)
		{
			RBXASSERT(mediaControl);
			if (audioSource)
			{
				audioSource->StopSoundCapture();
			}

			hr = mediaControl->Stop();
			if (FAILED(hr))
			{
				Sleep(1000);
				hr = mediaControl->Stop();
				RBXASSERT(SUCCEEDED(hr));
			}

			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,  "Video recording stopped");
		}
		DestroyCaptureGgaph();
		return SUCCEEDED(hr);
	}
	
	void DSVideoCaptureEngine::setVideoQuality(int vq) {
	}

	bool DSVideoCaptureEngine::isRunning()
	{
		if (!mediaControl)
		{
			return false;
		}
		
		OAFilterState state;
		HRESULT hr = mediaControl->GetState(INFINITE, &state);
		RBXASSERT(SUCCEEDED(hr));
		if (FAILED(hr))
		{
			FASTLOG(FLog::Error, "MediaControl GetState failed");
			return false;
		}

		return (state == State_Running);
	}

	void DSVideoCaptureEngine::GetSquareSizes(int cx, int cy, int &ncx, int &ncy)
	{
		ncx = cx;
		ncy = cy;
		double dcx = cx;
		double dcy = cy;
		if (dcx/dcy > defNx/defNy)
		{
			ncy = (defNy/defNx)*cx;
		}
		else
		{
			ncx = (defNx/defNy)*cy;
		}

		// it looks like we need this guy, otherwise graph will not start
		ncx += ncx % 4;
		ncy += ncy % 4;
	}

	static void doNothingCallbackContinuation() {}

	static void cancelRecording(DataModel* dm, Verb* cancelAction)
	{
		RBX::GuiService* guiService = RBX::ServiceProvider::find<GuiService>(dm);

		if (guiService && !guiService->notificationCallback.empty())
		{
			guiService->notificationCallback("Recording Stopped", "Because game window resolution changed", boost::bind(&doNothingCallbackContinuation), boost::bind(&doNothingCallbackContinuation));
		}

		// it supposed to be safe to pass NULL in this case
		cancelAction->doIt(NULL);
	}

    void DSVideoCaptureEngine::pushNextFrame(void* device, Verb *cancelAction)
    {
        FASTLOG(FLog::VideoCapture, "Pushing next frame.");
        RBXASSERT(isRunning());
        RBXASSERT(videoSource);
        RBXASSERT(cancelAction);
        HRESULT hr = S_OK;

        LONG absTime = GetAbsoluteTime();

        // lets limit video size by time
        if (absTime > MaxRecordTime)
        {
            FASTLOG(FLog::DeviceLost, "DSVideoCaptureEngine: Cancel by time");
            RBXASSERT(cancelAction);
            cancelAction->doIt(NULL);
            return;
        }

        RBX::Graphics::Device* graphicsDevice = static_cast<RBX::Graphics::Device*>(device);
        RBX::Graphics::Framebuffer* fb = graphicsDevice->getMainFramebuffer();
        // lets limit video size by time
        if (!fb)
        {
            FASTLOG(FLog::DeviceLost, "DSVideoCaptureEngine: Cancel by device lost");
            RBXASSERT(cancelAction);
            cancelAction->doIt(NULL);
            return;
        }

        int bpp = 4;
        int cx = fb->getWidth();
        int cy = fb->getHeight();

        int ncx, ncy;
        GetSquareSizes(cx, cy, ncx, ncy);

        int capcx, capcy;
        videoSource->GetCaptureSize(capcx, capcy);

        if (ncx != capcx || ncy != capcy)
        {
            FASTLOG(FLog::DeviceLost, "DSVideoCaptureEngine: Cancel by resize");

			if (RBX::DataModel* dm = dynamic_cast<RBX::DataModel*>(cancelAction->getContainer()))
			{
				// This is not safe if the verb dies before the DM does! But we can't take ownership of the verb here and generally verbs are tied to DM lifetime...
				dm->submitTask(boost::bind(cancelRecording, dm, cancelAction), DataModelJob::Write);
			}

            return;
        }

        if ((absTime - lastPushedVideoFrameTime) < videoSource->GetTargetFrameRate() || videoSource->GetQueueSize() >= RBX::DS::MaxFramesQueue)
        {
            return;
        }

        unsigned dataSize = bpp*cx*cy;
        if (dataSize != frameDataSize)
        {
            delete[] frameData;
            frameDataSize = dataSize;
            frameData = new unsigned char[frameDataSize];
        }
        
        fb->download(frameData, dataSize);

        int actualSize = bpp*ncx*ncy;
        unsigned char* videoData = new unsigned char[actualSize];
        memset(videoData, 0, actualSize);

        // this is address of first valid pixel (there might be padding)
        unsigned char* videoStart = videoData + ((ncy - cy)/2)*ncx*bpp + ((ncx - cx)/2)*bpp;

        for (int i = 0; i < cy; ++i)
        {
            unsigned originalIdx = i * cx * bpp;
            unsigned destIdx = (cy - 1 - i)*ncx*bpp;

            unsigned char* src = &frameData[originalIdx];
            for (int x = 0; x < cx; ++x)
                std::swap(src[x * bpp + 0], src[x * bpp + 2]);

            memcpy(&videoStart[destIdx], src, cx * bpp);
        }

        // videoData are now owned by videoSource and it will delete it after it doesn't need it
        videoSource->pushNextFrame(videoData, actualSize, ncx, ncy, (ncx-cx)*bpp);
    }

	LONG DSVideoCaptureEngine::GetTime()
	{
		return audioSource != NULL ? audioSource->GetAudioTime()->GetTime() : 0;
	}

	LONG DSVideoCaptureEngine::GetAbsoluteTime()
	{
		DWORD curTime = timeGetTime();
		return curTime - startTime;
	}

	const WCHAR profileDataV[] = L"<profile version=\"589824\" \r\n             storageformat=\"1\" \r\n             name=\"RBXVideo\" \r\n             description=\"Streams: 1 video\"> \r\n                   <streamconfig majortype=\"{73646976-0000-0010-8000-00AA00389B71}\" \r\n                   streamnumber=\"1\" \r\n                   streamname=\"Video1\" \r\n                   inputname=\"\" \r\n                   bitrate=\"1000000\" \r\n                   bufferwindow=\"3000\" \r\n                   reliabletransport=\"0\" \r\n                   decodercomplexity=\"\" \r\n                   rfc1766langid=\"en-us\" \r\n > \r\n                     <videomediaprops maxkeyframespacing=\"80000000\" \r\n                                     quality=\"100\"/> \r\n             <wmmediatype subtype=\"{32564D57-0000-0010-8000-00AA00389B71}\"  \r\n                   bfixedsizesamples=\"0\" \r\n                   btemporalcompression=\"1\" \r\n                   lsamplesize=\"0\"> \r\n       <videoinfoheader dwbitrate=\"1000000\" \r\n                        dwbiterrorrate=\"0\" \r\n                        avgtimeperframe=\"555555\"> \r\n        <rcsource left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n        <rctarget left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n            <bitmapinfoheader biwidth=\"854\" \r\n                              biheight=\"480\" \r\n                              biplanes=\"1\" \r\n                              bibitcount=\"24\" \r\n                              bicompression=\"WMV2\" \r\n                              bisizeimage=\"0\" \r\n                              bixpelspermeter=\"0\" \r\n                              biypelspermeter=\"0\" \r\n                              biclrused=\"0\" \r\n                              biclrimportant=\"0\"/> \r\n       </videoinfoheader>\r\n            </wmmediatype>\r\n            </streamconfig>\r\n                   </profile> \r\n ";

//	const WCHAR newProfileDataAV[] = L"<profile version=\"589824\" \r\n             storageformat=\"1\" \r\n             name=\"RBXVideo\" \r\n             description=\"Streams: 1 audio 1 video\"> \r\n                   <streamconfig majortype=\"{73646976-0000-0010-8000-00AA00389B71}\" \r\n                   streamnumber=\"1\" \r\n                   streamname=\"Video1\" \r\n                   inputname=\"\" \r\n                   bitrate=\"1000000\" \r\n                   bufferwindow=\"3000\" \r\n                   reliabletransport=\"0\" \r\n                   decodercomplexity=\"\" \r\n                   rfc1766langid=\"en-us\" \r\n > \r\n                     <videomediaprops maxkeyframespacing=\"80000000\" \r\n                                     quality=\"100\"/> \r\n             <wmmediatype subtype=\"{32564D57-0000-0010-8000-00AA00389B71}\"  \r\n                   bfixedsizesamples=\"0\" \r\n                   btemporalcompression=\"1\" \r\n                   lsamplesize=\"0\"> \r\n       <videoinfoheader dwbitrate=\"1000000\" \r\n                        dwbiterrorrate=\"0\" \r\n                        avgtimeperframe=\"555555\"> \r\n        <rcsource left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n        <rctarget left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n            <bitmapinfoheader biwidth=\"854\" \r\n                              biheight=\"480\" \r\n                              biplanes=\"1\" \r\n                              bibitcount=\"24\" \r\n                              bicompression=\"WMV2\" \r\n                              bisizeimage=\"0\" \r\n                              bixpelspermeter=\"0\" \r\n                              biypelspermeter=\"0\" \r\n                              biclrused=\"0\" \r\n                              biclrimportant=\"0\"/> \r\n       </videoinfoheader>\r\n            </wmmediatype>\r\n            </streamconfig>\r\n                   <streamconfig majortype=\"{73647561-0000-0010-8000-00AA00389B71}\" \r\n                   streamnumber=\"2\" \r\n                   streamname=\"Audio2\" \r\n                   inputname=\"\" \r\n                   bitrate=\"64024\" \r\n                   bufferwindow=\"3000\" \r\n                   reliabletransport=\"0\" \r\n                   decodercomplexity=\"\" \r\n                   rfc1766langid=\"en-us\" \r\n > \r\n             <wmmediatype subtype=\"{00000161-0000-0010-8000-00AA00389B71}\"  \r\n                   bfixedsizesamples=\"1\" \r\n                   btemporalcompression=\"0\" \r\n                   lsamplesize=\"0\"> \r\n           <waveformatex wFormatTag=\"353\" \r\n                         nChannels=\"2\" \r\n                         nSamplesPerSec=\"48000\" \r\n                         nAvgBytesPerSec=\"12000\" \r\n                         nBlockAlign=\"4096\" \r\n                         wBitsPerSample=\"16\" \r\n                         codecdata=\"008800000F0000000000\"/> \r\n            </wmmediatype>\r\n            </streamconfig>\r\n    </profile> \r\n";
	const WCHAR newProfileDataAV[] = L"<profile version=\"589824\" \r\n             storageformat=\"1\" \r\n             name=\"RBXVideo\" \r\n             description=\"Streams: 1 audio 1 video\"> \r\n                   <streamconfig majortype=\"{73646976-0000-0010-8000-00AA00389B71}\" \r\n                   streamnumber=\"1\" \r\n                   streamname=\"Video1\" \r\n                   inputname=\"\" \r\n                   bitrate=\"1000000\" \r\n                   bufferwindow=\"3000\" \r\n                   reliabletransport=\"0\" \r\n                   decodercomplexity=\"\" \r\n                   rfc1766langid=\"en-us\" \r\n > \r\n                     <videomediaprops maxkeyframespacing=\"80000000\" \r\n                                     quality=\"100\"/> \r\n             <wmmediatype subtype=\"{32564D57-0000-0010-8000-00AA00389B71}\"  \r\n                   bfixedsizesamples=\"0\" \r\n                   btemporalcompression=\"1\" \r\n                   lsamplesize=\"0\"> \r\n       <videoinfoheader dwbitrate=\"1000000\" \r\n                        dwbiterrorrate=\"0\" \r\n                        avgtimeperframe=\"555555\"> \r\n        <rcsource left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n        <rctarget left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n            <bitmapinfoheader biwidth=\"854\" \r\n                              biheight=\"480\" \r\n                              biplanes=\"1\" \r\n                              bibitcount=\"24\" \r\n                              bicompression=\"WMV2\" \r\n                              bisizeimage=\"0\" \r\n                              bixpelspermeter=\"0\" \r\n                              biypelspermeter=\"0\" \r\n                              biclrused=\"0\" \r\n                              biclrimportant=\"0\"/> \r\n       </videoinfoheader>\r\n            </wmmediatype>\r\n            </streamconfig>\r\n                   <streamconfig majortype=\"{73647561-0000-0010-8000-00AA00389B71}\" \r\n                   streamnumber=\"2\" \r\n                   streamname=\"Audio2\" \r\n                   inputname=\"\" \r\n                   bitrate=\"48000\" \r\n                   bufferwindow=\"3000\" \r\n                   reliabletransport=\"0\" \r\n                   decodercomplexity=\"\" \r\n                   rfc1766langid=\"en-us\" \r\n > \r\n             <wmmediatype subtype=\"{00000162-0000-0010-8000-00AA00389B71}\"  \r\n                   bfixedsizesamples=\"1\" \r\n                   btemporalcompression=\"0\" \r\n                   lsamplesize=\"0\"> \r\n           <waveformatex wFormatTag=\"354\" \r\n                         nChannels=\"2\" \r\n                         nSamplesPerSec=\"48000\" \r\n                         nAvgBytesPerSec=\"6000\" \r\n                         nBlockAlign=\"2048\" \r\n                         wBitsPerSample=\"16\" \r\n                         codecdata=\"1000030000000000000000000000600042C0\"/> \r\n            </wmmediatype>\r\n            </streamconfig>\r\n    </profile> \r\n";

	const WCHAR profWinXPAV[] = L"<profile version=\"589824\" \r\n             storageformat=\"1\" \r\n             name=\"RBXVidXP\" \r\n             description=\"Streams: 1 audio 1 video\"> \r\n                   <streamconfig majortype=\"{73646976-0000-0010-8000-00AA00389B71}\" \r\n                   streamnumber=\"1\" \r\n                   streamname=\"Video1\" \r\n                   inputname=\"\" \r\n                   bitrate=\"100000\" \r\n                   bufferwindow=\"3000\" \r\n                   reliabletransport=\"0\" \r\n                   decodercomplexity=\"\" \r\n                   rfc1766langid=\"en-us\" \r\n > \r\n                     <videomediaprops maxkeyframespacing=\"80000000\" \r\n                                     quality=\"100\"/> \r\n             <wmmediatype subtype=\"{33564D57-0000-0010-8000-00AA00389B71}\"  \r\n                   bfixedsizesamples=\"0\" \r\n                   btemporalcompression=\"1\" \r\n                   lsamplesize=\"0\"> \r\n       <videoinfoheader dwbitrate=\"100000\" \r\n                        dwbiterrorrate=\"0\" \r\n                        avgtimeperframe=\"555555\"> \r\n        <rcsource left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n        <rctarget left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n            <bitmapinfoheader biwidth=\"854\" \r\n                              biheight=\"480\" \r\n                              biplanes=\"1\" \r\n                              bibitcount=\"24\" \r\n                              bicompression=\"WMV3\" \r\n                              bisizeimage=\"0\" \r\n                              bixpelspermeter=\"0\" \r\n                              biypelspermeter=\"0\" \r\n                              biclrused=\"0\" \r\n                              biclrimportant=\"0\"/> \r\n       </videoinfoheader>\r\n            </wmmediatype>\r\n            </streamconfig>\r\n                   <streamconfig majortype=\"{73647561-0000-0010-8000-00AA00389B71}\" \r\n                   streamnumber=\"2\" \r\n                   streamname=\"Audio2\" \r\n                   inputname=\"\" \r\n                   bitrate=\"64024\" \r\n                   bufferwindow=\"3000\" \r\n                   reliabletransport=\"0\" \r\n                   decodercomplexity=\"\" \r\n                   rfc1766langid=\"en-us\" \r\n> \r\n             <wmmediatype subtype=\"{00000161-0000-0010-8000-00AA00389B71}\"  \r\n                   bfixedsizesamples=\"1\" \r\n                   btemporalcompression=\"0\" \r\n                   lsamplesize=\"0\"> \r\n           <waveformatex wFormatTag=\"353\" \r\n                         nChannels=\"2\" \r\n                         nSamplesPerSec=\"48000\" \r\n                         nAvgBytesPerSec=\"12000\" \r\n                         nBlockAlign=\"4096\" \r\n                         wBitsPerSample=\"16\" \r\n                         codecdata=\"008800000F0000000000\"/> \r\n            </wmmediatype>\r\n            </streamconfig>\r\n    </profile> \r\n";

	const WCHAR profWinXPV[] = L"<profile version=\"589824\" \r\n             storageformat=\"1\" \r\n             name=\"RBXVidXP\" \r\n             description=\"Streams: 1 video\"> \r\n                   <streamconfig majortype=\"{73646976-0000-0010-8000-00AA00389B71}\" \r\n                   streamnumber=\"1\" \r\n                   streamname=\"Video1\" \r\n                   inputname=\"\" \r\n                   bitrate=\"100000\" \r\n                   bufferwindow=\"3000\" \r\n                   reliabletransport=\"0\" \r\n                   decodercomplexity=\"\" \r\n                   rfc1766langid=\"en-us\" \r\n > \r\n                     <videomediaprops maxkeyframespacing=\"80000000\" \r\n                                     quality=\"100\"/> \r\n             <wmmediatype subtype=\"{33564D57-0000-0010-8000-00AA00389B71}\"  \r\n                   bfixedsizesamples=\"0\" \r\n                   btemporalcompression=\"1\" \r\n                   lsamplesize=\"0\"> \r\n       <videoinfoheader dwbitrate=\"100000\" \r\n                        dwbiterrorrate=\"0\" \r\n                        avgtimeperframe=\"555555\"> \r\n        <rcsource left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n        <rctarget left=\"0\" \r\n                  top=\"0\" \r\n                  right=\"854\" \r\n                  bottom=\"480\"/> \r\n            <bitmapinfoheader biwidth=\"854\" \r\n                              biheight=\"480\" \r\n                              biplanes=\"1\" \r\n                              bibitcount=\"24\" \r\n                              bicompression=\"WMV3\" \r\n                              bisizeimage=\"0\" \r\n                              bixpelspermeter=\"0\" \r\n                              biypelspermeter=\"0\" \r\n                              biclrused=\"0\" \r\n                              biclrimportant=\"0\"/> \r\n       </videoinfoheader>\r\n            </wmmediatype>\r\n            </streamconfig>\r\n            </profile> \r\n";

	HRESULT DSVideoCaptureEngine::BuildCaptureGraph(int cx, int cy, bool forceNoAudio)
	{
		IBaseFilter *asfWriterFilter = NULL;
		IFileSinkFilter2 *fileSink = NULL;
		IWMProfileManager *profileMgr = NULL;
		IWMProfile *profile = NULL;
		IConfigAsfWriter *asfConfig = NULL;

		OSVERSIONINFO osvi = {0};
		osvi.dwOSVersionInfoSize=sizeof(osvi);
		GetVersionEx(&osvi);

		HRESULT hr = S_OK;

		ERROR_ON_FAIL(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER, IID_IGraphBuilder, (void**)&graph));

		ERROR_ON_FAIL(graph->QueryInterface(IID_IMediaControl, (void**)&mediaControl));
		
		videoSource = new DS::CVideoStreamFilter(NULL, &hr, cx, cy);
		if (FAILED(hr)) {
			goto Error;
		}
		ERROR_ON_FAIL(graph->AddFilter(videoSource, L"GameVideoSource"));
		
		ERROR_ON_FAIL(CoCreateInstance(CLSID_WMAsfWriter, NULL, CLSCTX_INPROC_SERVER, IID_IBaseFilter, (void**)&asfWriterFilter));

		ERROR_ON_FAIL(asfWriterFilter->QueryInterface(IID_IFileSinkFilter2, (void**)&fileSink));

		WCHAR path[MAX_PATH];
		GenerateFileName(path);
		ERROR_ON_FAIL(fileSink->SetFileName(path, NULL));

		ERROR_ON_FAIL(graph->AddFilter(asfWriterFilter, L"asfWrite"));

		ERROR_ON_FAIL(WMCreateProfileManager(&profileMgr));

		// sound should be enables we should not force you to no sound and we have sound only on Vista & Win7 for now
		if (soundState->enabledFunction() && !forceNoAudio && osvi.dwMajorVersion > 5)
		{
			if (osvi.dwMajorVersion > 5)
			{
				ERROR_ON_FAIL(profileMgr->LoadProfileByData(newProfileDataAV, &profile));
			}
			else
			{
				ERROR_ON_FAIL(profileMgr->LoadProfileByData(profWinXPAV, &profile));
			}
		}
		else
		{
			if (osvi.dwMajorVersion > 5)
			{
				ERROR_ON_FAIL(profileMgr->LoadProfileByData(profileDataV, &profile));
			}
			else
			{
				ERROR_ON_FAIL(profileMgr->LoadProfileByData(profWinXPV, &profile));
			}
		}

		ERROR_ON_FAIL(asfWriterFilter->QueryInterface(IID_IConfigAsfWriter, (void**)&asfConfig));

		ERROR_ON_FAIL(asfConfig->ConfigureFilterUsingProfile(profile));

		ERROR_ON_FAIL(hr = graph->Render(videoSource->GetPin()));

		if (soundState->enabledFunction() && !forceNoAudio)
		{
			audioSource = new DS::CAudioStreamFilter(NULL, &hr, soundState, this);
			if (FAILED(hr)) {
				goto Error;
			}
			ERROR_ON_FAIL(graph->AddFilter(audioSource, L"AudioStream"));

			ERROR_ON_FAIL(graph->Render(audioSource->GetPin()));
		}

		startTime = timeGetTime();

		videoSource->SetAudioClock(this);
Error:
		SAFE_RELEASE(asfWriterFilter);
		SAFE_RELEASE(fileSink);

		SAFE_RELEASE(profileMgr);
		SAFE_RELEASE(profile);
		SAFE_RELEASE(asfConfig);

		return hr;
	}

	void DSVideoCaptureEngine::DestroyCaptureGgaph()
	{
	    SAFE_RELEASE(graph);
	    SAFE_RELEASE(mediaControl);

		graph = NULL;
		mediaControl = NULL;
		videoSource = NULL;
	}

	int DSVideoCaptureEngine::GenerateFileName(LPWSTR fileName)
	{
		SYSTEMTIME time;
		::GetLocalTime(&time);

        boost::filesystem::path folder = RBX::FileSystem::getUserDirectory(true, RBX::DirVideo);

        wchar_t name[MAX_PATH];
		int iLen = _snwprintf_s(name, _MAX_PATH-1, _MAX_PATH-1, L"%s-%d%02d%02d-%02d%02d%02d%01d%s.%s", 
			DS::ProcTitle, 
			time.wYear, time.wMonth, time.wDay,
			time.wHour, time.wMinute, time.wSecond,
			time.wMilliseconds/100,	// uniqueness to tenths of a sec. 
			L"",
			DS::VideoFileExt);

        boost::filesystem::path path = folder / name;
        wcsncpy_s( fileName, MAX_PATH, path.native().c_str(), _TRUNCATE ); // world sucks...
		
		// TODO: Localization: Unicode????
        fullFileName.clear();
		for (wchar_t* p = fileName; *p; ++p )
		{
            fullFileName.push_back(*p);
		}
		return iLen;
	}
}
