/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "v8tree/Instance.h"
#include "Reflection/Event.h"

#define FMOD_RESOURCES_FREED_STRING "FMOD System already closed.  Resources previously freed."

namespace FMOD
{
	class System;
	class Sound;
	class Channel;
	class ChannelGroup;
	class DSP;
}

namespace RBX 
{
	namespace Soundscape
	{
		// A wrapper of contentId we expose to lua as a type
		class SoundId : public ContentId
		{
		public:
			SoundId(const ContentId& id):ContentId(id) {}
			SoundId(const char* id):ContentId(id) {}
			SoundId(const std::string& id):ContentId(id) {}
			SoundId() {}
		};

		// essentially a wrapper for FMOD::Sound with some extra info
		class Sound : boost::noncopyable
		{
			FMOD::Sound* fmod_sound;
			shared_ptr<FMOD::System> const system;
			int refCount;  // Sadly, we can't use shared_ptr logic to manage the lifetime of fmod_sound
			bool isStreaming;

		public:
			SoundId const id;
			bool const is3D;
			Sound(shared_ptr<FMOD::System>& system, SoundId id, bool is3D):fmod_sound(0),system(system),id(id),is3D(is3D),refCount(0),isStreaming(false) {}
			~Sound() { release(); }
			FMOD::Sound* get() {return fmod_sound;}
			FMOD::Sound* tryLoad(const RBX::Instance* context);
			void detatch() { fmod_sound = 0; }
			void release();

			bool isReferenced() const { return refCount > 0; }
			void acquire() { ++refCount; }
			void unacquire() { refCount = std::max(0, refCount - 1); }

			bool getIsStreaming() const { return isStreaming; }
		};
	} // namespace Soundscape
} // namespace RBX