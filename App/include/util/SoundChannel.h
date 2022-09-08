/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "v8tree/Instance.h"
#include "Reflection/Event.h"
#include "Util/Sound.h"
#include "Util/RunStateOwner.h"

namespace FMOD
{
	class Channel;
}

struct FMOD_CHANNEL;

namespace RBX {

	class PartInstance;

	extern void registerSound();

	namespace Soundscape
	{
		enum RollOffMode
		{
			Inverse = 0,
			Linear,
		};
		// A simple sound object
		extern const char* const sSoundChannel;
		class SoundChannel
			: public DescribedCreatable<SoundChannel, Instance, sSoundChannel>
			, public Diagnostics::Countable<SoundChannel>
		{
		private:
			typedef DescribedCreatable<SoundChannel, Instance, sSoundChannel> Super;
			shared_ptr<Sound> sound;
			FMOD::Channel* fmod_channel;		// the latest channel

			RBX::Timer<RBX::Time::Fast> lastTimePosReplication; // to regulate how often we replicate the time
			
			SoundId soundId;
			float volume;
			float pitch;
			float minDistance;
			float maxDistance;
			RollOffMode rollOff;
			float defaultFrequency;

			double soundPositionSeconds;

			int numOfTimesLooped;
			unsigned lastSoundPositionMsec;

			bool playOnRemove;
			bool is3D : 1;
			bool looped : 1;
			bool soundDisabled : 1;	// a cached value coming from SoundService
			int playCount;		// actual number of how many times this sound has played
			int reqPlayCount;	// requested number of play calls.      Hack to get play() and pause() to replicate  (-1 stopped, 0 paused, 1+ play)
			mutable bool invalidChannel : 1;

			PartInstance* part;	// The Part (if any) that this sound is attached to

			rbx::signals::scoped_connection serverUpdatedTimeConnection;
			rbx::signals::scoped_connection serverScriptUpdatedTimeConnection;
			rbx::signals::scoped_connection serverResumedSoundConnection;

			/** 
			 * Some sounds are "played" everywhere, but should only be heard by certain peers.
			 * For example, sounds in Player GUIs should only be heard by the Player that sees the GUI.
			 **/
			bool isHeardLocally(const Instance* context) const;
			// see if everyone in the world can hear this sound
			bool isHeardGlobally() const;

			void updateLooped();
			void update3D(FMOD::Channel* channel);
			void playLocal(const Instance *context);
			void playSound(bool isResuming = false);
			void playSound(const Instance* context, bool isResuming = false);
			void releaseChannel();
			void loadSound(const Instance *context, bool shouldPlayOnLoad);

			void serverUpdatedTimePositionFromScript(unsigned int timePosition);
			void serverUpdatedTimePosition(unsigned int timePosition);

			bool controlledByAndIsServer() const;

		protected:
			/*override*/ bool askSetParent(const Instance* instance) const;
			/*override*/ void onAncestorChanged(const AncestorChanged& event);
			/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		public:
			rbx::signal<void(std::string soundId, int)> soundLoopedSignal;
			rbx::signal<void(std::string soundId)> soundPausedSignal;
			rbx::signal<void(std::string soundId)> soundStoppedSignal;
			rbx::signal<void(std::string soundId)> soundPlayedSignal;
			rbx::signal<void(std::string soundId)> soundEndedSignal;

			rbx::remote_signal<void(int)> timePositionUpdatedFromServerSignal;
			rbx::remote_signal<void(int)> timePositionUpdatedFromServerScriptSignal;

			rbx::remote_signal<void(int)> soundResumedFromServerSignal;

			SoundChannel();
			~SoundChannel();

			static Reflection::BoundProp<bool> sound_desc_playOnRemove;

			bool doFmodChannelAddressesMatch(const FMOD::Channel *channel) const;
            
            FMOD::Channel* getFMODChannel() { return fmod_channel; }

			void setSoundId(SoundId value);
			const SoundId &getSoundId() const;

			float getVolume() const;
			void setVolume(float value);

			float getPitch() const;
			void setPitch(float value);

			float getMinDistance() const;
			void setMinDistance(float value);
			float getMaxDistance() const;
			void setMaxDistance(float value);

			RollOffMode getRollOffMode() const { return rollOff; }
			void setRollOffMode(Soundscape::RollOffMode value);

			bool getLooped() const;
			void setLooped(bool value);

			int getPlayCount() const { return playCount; }
			void setPlayCount(int value);

			void resume();
			void play();
			void pause();
			void stop();
			
			// does not replicate play count
			void playLocal(); 
			void pauseLocal();
			////

			bool isPlaying() const;
			bool isPaused() const;
			bool isSoundLoaded() const;

			double getSoundLength() const;

			void setSoundPosition(double position, bool setFromLua = false);
			void setSoundPositionLua(double position);
			double getSoundPosition() const;

			bool getHasPlayed() const;
			void setHasPlayed(bool value);

			void updateListenState(const Time::Interval& timeSinceLastStep);

			static void soundEnded(weak_ptr<SoundChannel> channelWeak, std::string soundId);
			void onChannelEnd(const FMOD_CHANNEL *channel);
			void onSoundLoaded(const Instance *context, bool shouldPlayOnLoad);
		};
        
	} // namespace Soundscape
} // namespace RBX
