/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/SoundChannel.h"
#include "V8DataModel/ContentProvider.h"
#include "Util/SoundService.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"

#include "V8DataModel/GameSettings.h"
#include "V8DataModel/PlayerGui.h"
#include "Network/Players.h"

#include "FastLog.h"
#include "rbx/RbxDbgInfo.h"

#define FMOD_DEFAULT_CHANNEL_PRIORITY  128
#define FMOD_LONG_SOUND_CHANNEL_PRIORITY FMOD_DEFAULT_CHANNEL_PRIORITY - 16


LOGGROUP(FMOD)
LOGGROUP(Sound)
DYNAMIC_LOGGROUP(SoundTiming)
DYNAMIC_LOGGROUP(SoundTrace)

FASTINTVARIABLE(MinMsecBetweenTimePosEventReplication, 100)
FASTINTVARIABLE(MinSecondLengthForLongSoundChannel, 5)
FASTSTRINGVARIABLE(AssetTypeHeaderForSounds, "")
DYNAMIC_FASTFLAGVARIABLE(SoundFailedToLoadContext, false)
DYNAMIC_FASTFLAGVARIABLE(MinMaxDistanceEnabled, false)
DYNAMIC_FASTFLAGVARIABLE(RollOffModeEnabled, false)

namespace RBX
{

namespace Reflection
{
template<>
EnumDesc<RBX::Soundscape::RollOffMode>::EnumDesc()
	:EnumDescriptor("RollOffMode")
{
	addPair(RBX::Soundscape::Inverse, "Inverse");
	addPair(RBX::Soundscape::Linear, "Linear");
}
template<>
RBX::Soundscape::RollOffMode& Variant::convert<RBX::Soundscape::RollOffMode>(void)
{
	return genericConvert<RBX::Soundscape::RollOffMode>();
}
} //namespace Reflection
template<>
bool StringConverter<RBX::Soundscape::RollOffMode>::convertToValue(const std::string& text, RBX::Soundscape::RollOffMode& value)
{
	return Reflection::EnumDesc<RBX::Soundscape::RollOffMode>::singleton().convertToValue(text.c_str(),value);
}
}

namespace RBX
{

void registerSound()
{
    Soundscape::SoundChannel::classDescriptor();
}

namespace Soundscape
{
REFLECTION_BEGIN();
static Reflection::PropDescriptor<SoundChannel, SoundId> sound_desc_SoundId("SoundId", category_Data, &SoundChannel::getSoundId, &SoundChannel::setSoundId);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_Volume("Volume", category_Data, &SoundChannel::getVolume, &SoundChannel::setVolume);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_Pitch("Pitch", category_Data, &SoundChannel::getPitch, &SoundChannel::setPitch);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_MinDistance("MinDistance", category_Data, &SoundChannel::getMinDistance, &SoundChannel::setMinDistance);
static Reflection::PropDescriptor<SoundChannel, float> sound_desc_MaxDistance("MaxDistance", category_Data, &SoundChannel::getMaxDistance, &SoundChannel::setMaxDistance);
// DFFLag::RollOffModeEnabled 
// static Reflection::EnumPropDescriptor<SoundChannel, RollOffMode> sound_desc_RollOffMode("RollOffMode", category_Data, &SoundChannel::getRollOffMode, &SoundChannel::setRollOffMode);

static Reflection::PropDescriptor<SoundChannel, double> sound_desc_SoundLength("TimeLength", category_Data, &SoundChannel::getSoundLength, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<SoundChannel, double> sound_desc_SoundPosition("TimePosition", category_Data, &SoundChannel::getSoundPosition, &SoundChannel::setSoundPositionLua, Reflection::PropertyDescriptor::UI);

static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_Looped("Looped", category_Behavior, &SoundChannel::getLooped, &SoundChannel::setLooped);
Reflection::BoundProp< bool> SoundChannel::sound_desc_playOnRemove("PlayOnRemove", category_Behavior, &SoundChannel::playOnRemove);

static Reflection::BoundFuncDesc<SoundChannel, void()> sound_playFunction(&SoundChannel::play, "Play", Security::None);
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_resumeFunction(&SoundChannel::resume, "Resume", Security::None);
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_pauseFunction(&SoundChannel::pause, "Pause", Security::None);
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_stopFunction(&SoundChannel::stop, "Stop", Security::None);

static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_IsPlaying("IsPlaying", category_Data, &SoundChannel::isPlaying, NULL, Reflection::PropertyDescriptor::UI);
static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_isPaused("IsPaused", category_Data, &SoundChannel::isPaused, NULL, Reflection::PropertyDescriptor::UI);

static Reflection::EventDesc<SoundChannel, void(std::string, int)> sound_loopedSignal(&SoundChannel::soundLoopedSignal, "DidLoop", "soundId", "numOfTimesLooped");
static Reflection::EventDesc<SoundChannel, void(std::string)> sound_pausedSignal(&SoundChannel::soundPausedSignal, "Paused", "soundId");
static Reflection::EventDesc<SoundChannel, void(std::string)> sound_playedSignal(&SoundChannel::soundPlayedSignal, "Played", "soundId");
static Reflection::EventDesc<SoundChannel, void(std::string)> sound_stoppedSignal(&SoundChannel::soundStoppedSignal, "Stopped", "soundId");
static Reflection::EventDesc<SoundChannel, void(std::string)> sound_endedSignal(&SoundChannel::soundEndedSignal, "Ended", "soundId");


//////////////////////////////////////////////////////
// Backend Events/Properties
/////////////////////////////////////////////////
static Reflection::PropDescriptor<SoundChannel, int> sound_desc_PlayCount("PlayCount", category_Data, &SoundChannel::getPlayCount, &SoundChannel::setPlayCount, Reflection::PropertyDescriptor::REPLICATE_ONLY);
static Reflection::RemoteEventDesc<SoundChannel, void(int)> event_timePositionUpdatedFromServer(&SoundChannel::timePositionUpdatedFromServerSignal, "TimePositionUpdated", "newPositionSeconds", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<SoundChannel, void(int)> event_timePositionUpdatedFromServerScript(&SoundChannel::timePositionUpdatedFromServerScriptSignal, "TimePositionUpdatedFromScript", "newPositionSeconds", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);
static Reflection::RemoteEventDesc<SoundChannel, void(int)> event_soundResumedFromServer(&SoundChannel::soundResumedFromServerSignal, "SoundResumedFromServer", "currentTimePosition", Security::Roblox, Reflection::RemoteEventCommon::REPLICATE_ONLY, Reflection::RemoteEventCommon::BROADCAST);


//////////////////////////////////////////////////////
// DEPRECATED LUA FUNCTIONS/PROPS
/////////////////////////////////////////////////
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_stopFunctionDep(&SoundChannel::stop, "stop", Security::None, Reflection::Descriptor::Attributes::deprecated(sound_stopFunction));
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_playFunctionDeprecated(&SoundChannel::play, "play", Security::None, Reflection::Descriptor::Attributes::deprecated(sound_playFunction));
static Reflection::BoundFuncDesc<SoundChannel, void()> sound_dep_pauseFunction(&SoundChannel::pause, "pause", Security::None, Reflection::Descriptor::Attributes::deprecated(sound_pauseFunction));
static Reflection::PropDescriptor<SoundChannel, bool> sound_desc_dep_IsPlaying("isPlaying", category_Data, &SoundChannel::isPlaying, NULL, Reflection::PropertyDescriptor::Attributes::deprecated(sound_desc_IsPlaying));
REFLECTION_END();

SoundChannel::SoundChannel()
	:looped(false)
	,is3D(false)
	,playOnRemove(false)
	,fmod_channel(NULL)
	,part(NULL)
	,volume(0.5)
	,pitch(1)
	,minDistance(10)
	,maxDistance(100000)
	,rollOff(Inverse)
	,soundDisabled(false)
	,playCount(-1)
	,reqPlayCount(-1)
	,soundPositionSeconds(0)
	,invalidChannel(true)
	,numOfTimesLooped(0)
	,lastSoundPositionMsec(0)
{
	this->setName("Sound");
}

SoundChannel::~SoundChannel()
{
	FASTLOG3(DFLog::SoundTrace, "SoundChannel::~SoundChannel(%p, %p, %p)", this, fmod_channel, sound.get());
    releaseChannel();
	RBXASSERT(fmod_channel==NULL);
	RBXASSERT(!sound);
}


void SoundChannel::releaseChannel()
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::releaseChannel(%p, %p)", this, fmod_channel);
	if (fmod_channel)
	{
#ifdef _DEBUG
		SoundChannel* oldSound;
		SoundService::checkResultNoThrow(fmod_channel->getUserData(reinterpret_cast<void**>(&oldSound)), "getUserData", this, fmod_channel);
		RBXASSERT(oldSound==0 || oldSound==this);
#endif
            SoundService::checkResultNoThrow(fmod_channel->setUserData(reinterpret_cast<void*>(NULL)), "setUserData release", this, fmod_channel);
		fmod_channel = NULL;
	}
	if (sound)
	{
        sound.reset();
	}
}

bool SoundChannel::askSetParent(const Instance* instance) const
{
	// Sounds can be a child of anything
	return true;
}


void SoundChannel::updateListenState(const Time::Interval& timeSinceLastStep)
{
	if (fmod_channel)
	{
		update3D(fmod_channel);
	}

	if (controlledByAndIsServer() && (timeSinceLastStep.msec() > 0))
	{
		// we aren't actually playing sounds, but should keep the scrub in a good location
		// for client replication (say we start playing a sound in workspace when it is actually half over)

		double position = soundPositionSeconds;
		if (isPlaying() && !isPaused())
		{
			position += (timeSinceLastStep.seconds() * getPitch());
			double soundLength = getSoundLength();
			
			if (!getLooped() && (position > soundLength) && sound)
			{
				position = 0;
				DataModel::scoped_write_request request(RBX::DataModel::get(this));
				stop();
			}
			else
			{
				if ( getLooped() && isPlaying() && (position > soundLength) )
				{
					numOfTimesLooped++;

					DataModel::scoped_write_request request(RBX::DataModel::get(this));
					soundLoopedSignal(getSoundId().toString(), numOfTimesLooped);
				}

				position = fmod(position, (soundLength ? soundLength : 1));
				DataModel::scoped_write_request request(RBX::DataModel::get(this));
				setSoundPosition(position);
			}
		}
	}
	else if (!controlledByAndIsServer() && fmod_channel && getLooped() && isPlaying())
	{
		unsigned currentPosMs = 0;
		fmod_channel->getPosition(&currentPosMs,FMOD_TIMEUNIT_MS);

		if (currentPosMs < lastSoundPositionMsec)
		{
			numOfTimesLooped++;

			DataModel::scoped_write_request request(RBX::DataModel::get(this));
			soundLoopedSignal(getSoundId().toString(), numOfTimesLooped);
		}

		lastSoundPositionMsec = currentPosMs;
	}
}

void SoundChannel::onAncestorChanged(const AncestorChanged& event)
{
	FASTLOG3(DFLog::SoundTrace, "SoundChannel::onAncestorChanged(%p, %p), event.child = %p", this, event.newParent, event.child);
	Super::onAncestorChanged(event);

	if (event.child==this)
	{
		part = Instance::fastDynamicCast<PartInstance>(event.newParent);
	}

	if (playOnRemove && !getLooped())
	{
		// If this Sound or one of its parents is being removed from the Workspace, then play the sound!
		const Instance* oldWorkspace = ServiceProvider::find<Workspace>(event.oldParent);
		const bool wasInWorkspace = oldWorkspace && (event.oldParent==oldWorkspace || event.oldParent->isDescendantOf(oldWorkspace));

		if (wasInWorkspace)
		{
			const Instance* newWorkspace = ServiceProvider::find<Workspace>(event.newParent);
			const bool isInWorkspace = newWorkspace && (event.newParent==newWorkspace || event.newParent->isDescendantOf(newWorkspace));
			if (!isInWorkspace)
			{
				FASTLOG1(DFLog::SoundTrace, "Play on remove with SoundChannel %p", this);
				RBXASSERT(oldWorkspace);
                loadSound(oldWorkspace, true);
			}
		}
	}
}

void SoundChannel::serverUpdatedTimePositionFromScript(unsigned int timePosition)
{
	// a server script updated time position, make sure we update too
	setSoundPosition(timePosition);
}

void SoundChannel::serverUpdatedTimePosition(unsigned int timePosition)
{
	// only get initial position from server, then don't worry about this type of update
	serverUpdatedTimeConnection.disconnect();
	setSoundPosition(timePosition);
}

void SoundChannel::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	FASTLOG5(DFLog::SoundTrace, "SoundChannel::onServiceProvider(%p, %p, %p, %d, %d)", this, oldProvider, newProvider, playCount, reqPlayCount);
	if (oldProvider)
	{
		// Looped sounds should be turned off when we leave the world (like a rocket engine)
		// Non-looped sounds can finish playing (like an explosion)
		if (getLooped())
		{
			FASTLOG1(DFLog::SoundTrace, "SoundChannel::onServiceProvider(%p) calling stop on looped sound.", this);
			stop();
		}
		
		releaseChannel();
        SoundService* soundService = ServiceProvider::create<SoundService>(oldProvider);
        if (soundService)
        {
            soundService->unregisterSoundChannel(this);
        }

		serverUpdatedTimeConnection.disconnect();
		serverScriptUpdatedTimeConnection.disconnect();
		serverResumedSoundConnection.disconnect();
	}

	Super::onServiceProvider(oldProvider,newProvider);

	if (newProvider)
	{
		SoundService* soundService = ServiceProvider::create<SoundService>(newProvider);
        soundService->registerSoundChannel(this);
        part = Instance::fastDynamicCast<PartInstance>(this->getParent());
        FASTLOG2(DFLog::SoundTrace, "SoundChannel::onServiceProvider(%p) setting part to %p", this, part);
		if (soundService && soundService->enabled())
		{
			soundDisabled = false;	// Worth trying again to see if we have a working sound manager
			FASTLOG2(DFLog::SoundTrace, "SoundChannel::onServiceProvider(%p), playCount < reqPlayCount = %d", this, playCount < reqPlayCount);
            loadSound(this, playCount < reqPlayCount);
            playCount = reqPlayCount;
		}

		lastTimePosReplication.reset();

		if (RBX::Network::Players::clientIsPresent(newProvider))
		{
			// only get initial position update from server if sound is replicated to each client
			if (isHeardGlobally())
			{
				serverUpdatedTimeConnection = timePositionUpdatedFromServerSignal.connect(boost::bind(&SoundChannel::serverUpdatedTimePosition, this, _1));
			}

			serverScriptUpdatedTimeConnection = timePositionUpdatedFromServerScriptSignal.connect(boost::bind(&SoundChannel::serverUpdatedTimePositionFromScript, this, _1));
			serverResumedSoundConnection = soundResumedFromServerSignal.connect(boost::bind(&SoundChannel::resume, this));
		}
	}
}

namespace {
	void onSoundLoadedForSoundChannel(shared_ptr<SoundChannel> soundChannel, const SoundId &soundId, const Instance *context, bool shouldPlayOnLoad, AsyncHttpQueue::RequestResult result, shared_ptr<const std::string> item)
	{
		switch (result)
		{
		case AsyncHttpQueue::Failed:
			FASTLOGS(FLog::Sound, "onSoundLoaded Failed to load %s", soundId.c_str());
			if (DFFlag::SoundFailedToLoadContext && context && !soundId.isNull())
			{
				RBX::StandardOut::singleton()->printf(MESSAGE_ERROR, "Sound failed to load %s : %s", context->getFullName().c_str(), soundId.c_str());
			}
			break;
		case AsyncHttpQueue::Waiting:
			FASTLOGS(FLog::Sound, "onSoundLoaded Waiting for %s", soundId.c_str());
			break;
		case AsyncHttpQueue::Succeeded:
			FASTLOGS(FLog::Sound, "onSoundLoaded Succeeded in loading %s", soundId.c_str());

			// Make sure we only trigger an onSoundLoaded event if our callback contained the data we want
			// and we don't currently have the sound loaded.
			if (soundChannel->getSoundId() == soundId)
			{
				soundChannel->onSoundLoaded(context, shouldPlayOnLoad);
			}
			break;
		}
	}
} // namespace

void SoundChannel::onSoundLoaded(const Instance *context, bool shouldPlayOnLoad)
{
	FASTLOG3(DFLog::SoundTrace, "SoundChannel::onSoundLoaded(%p, %p, %d)", this, context, shouldPlayOnLoad);

	is3D = part != NULL;

	SoundService* soundService = ServiceProvider::create<SoundService>(context);
	if (soundService)
	{
		sound = soundService->loadSound(getSoundId(), is3D);

		raisePropertyChanged(sound_desc_SoundLength);

		// Only play our sound at the appropriate time.
		if (shouldPlayOnLoad)
		{
			playLocal(context);
		}
	}
}

bool SoundChannel::isPaused() const
{
    FASTLOG2(DFLog::SoundTrace, "SoundChannel::isPaused(%p, %p)", this, fmod_channel);

	if (!fmod_channel)
	{
        return playCount <= 0;
	}

	bool paused = true;
	if (!invalidChannel)
	{
		FMOD_RESULT fmodResult = fmod_channel->getPaused(&paused);
		if (FMOD_OK != fmodResult)
		{
			invalidChannel = true;
			SoundService::checkResultNoThrow(fmodResult, "getPaused", this, fmod_channel);
			paused = true;
		}
        FASTLOG2(DFLog::SoundTrace, "Returning SoundChannel::isPaused(%p) = %d", this, paused);
	}

	return paused;
}

bool SoundChannel::isPlaying() const
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::isPlaying(%p, %p)", this, fmod_channel);
	if (!fmod_channel)
	{
		if (RBX::Network::Players::clientIsPresent(this, false))
		{
			return false;
		}

		return playCount > 0;
	}

	bool playing = false;
	bool paused = false;
	if (!invalidChannel)
	{
		FMOD_RESULT fmodResult = fmod_channel->isPlaying(&playing);

		if (FMOD_OK != fmodResult)
		{
			invalidChannel = true;
			SoundService::checkResultNoThrow(fmodResult, "isPlaying", this, fmod_channel);
			playing = false;
		}

		fmodResult = fmod_channel->getPaused(&paused);
		if (FMOD_OK != fmodResult)
		{
			invalidChannel = true;
			SoundService::checkResultNoThrow(fmodResult, "isPlaying", this, fmod_channel);
			playing = false;
		}

		FASTLOG2(DFLog::SoundTrace, "Returning SoundChannel::isPlaying(%p) = %d", this, playing && !paused);
	}

	return playing && !paused;
}

bool SoundChannel::isSoundLoaded() const
{
	return sound != NULL && sound->id == getSoundId();
}

void SoundChannel::loadSound(const Instance *context, bool shouldPlayOnLoad)
{
	FASTLOG3(DFLog::SoundTrace, "SoundChannel::loadSound(%p, %p, %d)", this, context, shouldPlayOnLoad);
	if (!SoundService::soundDisabled && !soundDisabled && RBX::GameSettings::singleton().soundEnabled)
	{
		ContentProvider* contentProvider = ServiceProvider::create<ContentProvider>(context);
		FASTLOG2(DFLog::SoundTrace, "SoundChannel::loadSound(%p), contentProvider = %p", this, contentProvider);
		if (contentProvider)
		{
			if (!isSoundLoaded())
			{
				if (contentProvider->hasContent(getSoundId()))
				{
					FASTLOGS(FLog::Sound, "Loading cached soundId = %s", getSoundId().c_str());
					onSoundLoaded(context, shouldPlayOnLoad);
				}
				else
				{
					FASTLOGS(FLog::Sound, "Fetching soundId = %s", getSoundId().c_str());
					contentProvider->preloadContentWithCallback(soundId, ContentProvider::PRIORITY_SOUND, 
						boost::bind(&onSoundLoadedForSoundChannel, shared_from(this), soundId, context, shouldPlayOnLoad, _1, shared_ptr<const std::string>()),
						AsyncHttpQueue::AsyncWrite,
						FString::AssetTypeHeaderForSounds
					);
				}
			}
			else
			{
				FASTLOGS(FLog::Sound, "Already loaded soundId = %s", getSoundId().c_str());
				onSoundLoaded(context, shouldPlayOnLoad);
			}
		}
	}
}

void SoundChannel::setSoundId(SoundId value)
{
	if (DFLog::SoundTrace)
	{
		std::stringstream ss;
		ss << reinterpret_cast<void*>(this) << ", " << value.toString() << ")";
		FASTLOGS(DFLog::SoundTrace, "SoundChannel::setSoundId(%s", ss.str().c_str());
	}

	if (value != soundId)
	{
		soundId = value;
        releaseChannel();
        loadSound(this, false);
		raiseChanged(sound_desc_SoundId);
	}
}

const SoundId &SoundChannel::getSoundId() const
{
	return soundId;
}

float SoundChannel::getVolume() const
{
	return volume;
}

double SoundChannel::getSoundLength() const
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::getSoundLength(%p, %p)", this, fmod_channel);

	unsigned length = 0;
	FMOD::Sound* fmod_sound = sound ? sound->tryLoad(this) : NULL;
	if (fmod_sound)
	{
		FMOD_RESULT fmodResult = fmod_sound->getLength(&length, FMOD_TIMEUNIT_MS);
		if (FMOD_OK != fmodResult)
		{
			SoundService::checkResultNoThrow(fmodResult, "getLength", this, fmod_sound);
			length = 0;
		}
	}
	return static_cast<double>(length/1000.0);
}

double SoundChannel::getSoundPosition() const
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::getSoundPosition(%p, %p)", this, fmod_channel);
	double position = 0.0;

	// server emulates sound position, has no actual fmod channels
	if (controlledByAndIsServer())
	{
		position = soundPositionSeconds;
	}
	else
	{
		unsigned positionMs = 0;
		FMOD_RESULT fmodResult = fmod_channel->getPosition(&positionMs, FMOD_TIMEUNIT_MS);
		if (FMOD_OK != fmodResult)
		{
			invalidChannel = true;
			SoundService::checkResultNoThrow(fmodResult, "getPosition", this, fmod_channel);
			position = 0;
		}
		else
		{
			position = ((double)positionMs)/1000.0;
		}
	}

	double soundLength = getSoundLength();
	position = fmod(position, (soundLength ? soundLength : 1));

	if (DFLog::SoundTiming)
	{
		std::stringstream ss;
		ss << reinterpret_cast<const void*>(this) << ", " << getSoundId().toString() << ") = " << position;
		FASTLOGS(DFLog::SoundTiming, "Returning SoundChannel::getSoundPosition(%s", ss.str().c_str());
	}

	return position;
}


void SoundChannel::setSoundPositionLua(double value)
{
	if (isHeardGlobally() && !RBX::Network::Players::serverIsPresent(this))
	{
		if (RBX::Network::Players::clientIsPresent(this))
		{	
			throw std::runtime_error("Sound.TimePosition was set from local script while either in Workspace or SoundService. Only use a server script to set TimePosition when a sound is in these locations.");
			return;
		}
	}

	setSoundPosition(value, true);
}

// Set our sound position, but if the value passed in is the same as soundPositionMillisecs, then try to set FMOD's
// position without replicating the value back to everyone else.  See SoundChannel::playSound for usage of this
// latter feature.
void SoundChannel::setSoundPosition(double value, bool setFromLua)
{
	FASTLOG3(DFLog::SoundTrace, "SoundChannel::setSoundPosition(%p, %p, %d)", this, fmod_channel, (int)value);
	if (fmod_channel)
	{
		double oldPosition = getSoundPosition();
		double soundLength = getSoundLength();
		double newPosition = fmod(value, (soundLength ? soundLength : 1.0));
		if (oldPosition != newPosition)
		{
			FASTLOG1F(FLog::Sound, "Setting sound position on channel to %f", newPosition);
			SoundService::checkResultNoThrow(fmod_channel->setPosition((newPosition * 1000.0), FMOD_TIMEUNIT_MS), "setPosition", this, fmod_channel);
		}
	}

	
	// Only send a property update if our property really changed.  This is a bit of trickery to allow
	// clients to set their positions at "join time" without re-replicating the value
	// back out to others.
	if (value != soundPositionSeconds)
	{
		soundPositionSeconds = value;
		raiseChanged(sound_desc_SoundPosition);

		if (controlledByAndIsServer() && (getSoundLength() >= FInt::MinSecondLengthForLongSoundChannel))
		{
			if (setFromLua)
			{
				event_timePositionUpdatedFromServerScript.fireAndReplicateEvent(this, soundPositionSeconds);
			}
			else if ( lastTimePosReplication.delta().msec() > FInt::MinMsecBetweenTimePosEventReplication )
			{
				// todo: just always fire this event, and only replicated it to clients that are actually listening
				lastTimePosReplication.reset();
				event_timePositionUpdatedFromServer.fireAndReplicateEvent(this, soundPositionSeconds);
			}
		}
	}
}

void SoundChannel::setVolume(float value)
{
	value = G3D::clamp(value, 0.0f, 1.0f);
	if (DFLog::SoundTrace)
	{
		std::stringstream ss;
		ss << reinterpret_cast<void*>(this) << ", " << value << ")";
		FASTLOGS(DFLog::SoundTrace, "SoundChannel::setVolume(%s", ss.str().c_str());
	}

	if (volume != value)
	{
		volume = value;

		if (fmod_channel)
		{
			SoundService::checkResultNoThrow(fmod_channel->setVolume(value), "setVolume", this, fmod_channel);
		}

		raiseChanged(sound_desc_Volume);
	}
}

float SoundChannel::getPitch() const
{
	return pitch;
}

void SoundChannel::setPitch(float value)
{
	if (value < 0.0f)
	{
		value = 0.0f;
	}

	if (pitch != value)
	{
		pitch = value;

		if (fmod_channel)
		{
			SoundService::checkResultNoThrow(fmod_channel->setFrequency(pitch * defaultFrequency), "setFrequency", this, fmod_channel);
		}

		raiseChanged(sound_desc_Pitch);
	}
}

float SoundChannel::getMinDistance() const
{
	if (!DFFlag::MinMaxDistanceEnabled)
	{
		return 0;
	}
	return minDistance;
}

void SoundChannel::setMinDistance(float value)
{
	if (value < 0.0f)
	{
		value = 0.0f;
	}

	if (minDistance != value)
	{
		minDistance = value;
		raiseChanged(sound_desc_MinDistance);
	}
}

float SoundChannel::getMaxDistance() const
{
	if (!DFFlag::MinMaxDistanceEnabled)
	{
		return 0;
	}
	return maxDistance;
}

void SoundChannel::setMaxDistance(float value)
{
	if (value < 0.0f)
	{
		value = 0.0f;
	}

	if (maxDistance != value)
	{
		maxDistance = value;
		raiseChanged(sound_desc_MaxDistance);
	}
}

void SoundChannel::setRollOffMode(Soundscape::RollOffMode value)
{
	if (!DFFlag::RollOffModeEnabled) {
		return;
	}

	if (rollOff != value) {
		rollOff = value;
// DFFLag::RollOffModeEnabled 
//		raiseChanged(sound_desc_RollOffMode);
	}
}

bool SoundChannel::getLooped() const
{
	return looped;
}

void SoundChannel::updateLooped()
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::updateLooped(%p, %p)", this, fmod_channel);
	if (fmod_channel)
	{
		FMOD_MODE mode;
		SoundService::checkResultNoThrow(fmod_channel->getMode(&mode), "getMode", this, fmod_channel);
		mode |= (getLooped() ? FMOD_LOOP_NORMAL : FMOD_LOOP_OFF);
		mode &= (getLooped() ? ~FMOD_LOOP_OFF : ~FMOD_LOOP_NORMAL);
		SoundService::checkResultNoThrow(fmod_channel->setMode(mode), "setMode", this, fmod_channel);
		SoundService::checkResultNoThrow(fmod_channel->setLoopCount(getLooped() ? -1 : 0), "setLoopCount", this, fmod_channel);

		if (FInt::MinSecondLengthForLongSoundChannel > 0)
		{
			FASTLOGS(FLog::Sound, "Updating channel priority on soundId = %s", getSoundId().c_str());

			if (FInt::MinSecondLengthForLongSoundChannel <= getSoundLength())
			{
				SoundService::checkResult(fmod_channel->setPriority(FMOD_LONG_SOUND_CHANNEL_PRIORITY), "setPriority", this, fmod_channel);
			}
		}
	}
}

void SoundChannel::setLooped(bool value)
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::setLooped(%p, %d)", this, value);
	if (looped!=value)
	{
		looped = value;
		updateLooped();

		raiseChanged(sound_desc_Looped);
	}
}

void SoundChannel::setPlayCount(int value)
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::setPlayCount(%p, %d)", this, value);
	// TODO: This isn't very satisfying.  This won't replicate if you call "play" multiple times
	switch (value)
	{
	case -1:
		if (isHeardGlobally() && RBX::Network::Players::clientIsPresent(this))
		{
			soundPositionSeconds = 0;
		}
		stop();
		break;
	case 0:
		if (isHeardGlobally() && RBX::Network::Players::clientIsPresent(this))
		{
			soundPositionSeconds = getSoundPosition();
		}
		pause();
		break;
	default:
		if (value > reqPlayCount)
		{
			reqPlayCount = playCount = value;
			this->raiseChanged(sound_desc_PlayCount);

			SoundService *soundService = ServiceProvider::create<SoundService>(this);
			if (!soundService)
			{
				--playCount; // In onServiceProvider, we will have another chance to play.
            }
            else
            {
                FASTLOG3(DFLog::SoundTrace, "SoundChannel(%p) soundService = %p, enabled = %d", this, soundService, soundService->enabled());
            }

			loadSound(this, true);
		}

		break;
	}
}


void SoundChannel::update3D(FMOD::Channel* channel)
{
	if (is3D && part)
	{
		if (DFFlag::MinMaxDistanceEnabled)
		{
			channel->set3DMinMaxDistance(minDistance, maxDistance);
		}
		if (DFFlag::RollOffModeEnabled) 
		{
			if (rollOff == Inverse)
			{
				channel->setMode(FMOD_3D_INVERSEROLLOFF);
			} 
			else if (rollOff == Linear) 
			{
				channel->setMode(FMOD_3D_LINEARROLLOFF);
			}
		}
		FMOD_VECTOR pos;
		if (SoundService::convert(part->getCoordinateFrame().translation, pos))
		{
			FMOD_VECTOR vel;
			
			if (SoundService::convert(part->getLinearVelocity(), vel))
			{
				FMOD_RESULT fmodResult = channel->set3DAttributes(&pos, &vel);
				if (FMOD_OK != fmodResult) // don't overlog during traces
				{
					SoundService::checkResultNoThrow(fmodResult, "set3DAttributes", this, channel);
				}
			}
		}
	}
}

void SoundChannel::soundEnded(weak_ptr<SoundChannel> channelWeak, std::string soundId)
{
	if (shared_ptr<SoundChannel> channel = channelWeak.lock())
	{
		channel->soundEndedSignal(soundId);
	}
}

void SoundChannel::onChannelEnd(const FMOD_CHANNEL *channel) 
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::onChannelEnd(%p, %p)", this, fmod_channel);
	if (fmod_channel && reinterpret_cast<const void*>(channel) == reinterpret_cast<const void*>(fmod_channel))
	{
		FASTLOG2(FLog::Sound, "Sound ended for channel = %p, fmod_channel = %p", this, fmod_channel);
		FASTLOGS(FLog::Sound, "Sound ended for soundId = %s", getSoundId().c_str());

		SoundService::checkResultNoThrow(fmod_channel->setUserData(reinterpret_cast<void*>(NULL)), "setUserData onChannelEnd", this, fmod_channel);
		fmod_channel = NULL;

		if (sound)
		{
			sound->unacquire();
				if (DataModel* dm = RBX::DataModel::get(this)) 
				{
					dm->submitTask(boost::bind(&SoundChannel::soundEnded, shared_from(this), getSoundId().toString()), RBX::DataModelJob::Write);
				}
			}
		}
	}

FMOD_RESULT F_CALLBACK callbackChannelEnd(FMOD_CHANNELCONTROL *channelControl, FMOD_CHANNELCONTROL_TYPE type, FMOD_CHANNELCONTROL_CALLBACK_TYPE callbacktype, void *commanddata1, void *commanddata2)
{
	FMOD_RESULT fmodResult = FMOD_OK;

	if(type == FMOD_CHANNELCONTROL_CHANNEL)
	{
		//Channel specific functions here
		FMOD_CHANNEL* channel = (FMOD_CHANNEL *)channelControl;
		if (callbacktype==FMOD_CHANNELCONTROL_CALLBACK_END)
	{
		// TODO: For some reason not all channels call this function... I have no idea why.  The result is that we have some sound "leaks" reported by getSoundStats()
		RBX::Soundscape::SoundChannel* sound;
		FMOD_RESULT fmodResult = FMOD_Channel_GetUserData(channel, reinterpret_cast<void**>(&sound));
		SoundService::checkResultNoThrow(fmodResult, "FMOD_Channel_GetUserData", NULL, channel);
		if (FMOD_OK == fmodResult && sound)
        {
                sound->onChannelEnd(channel);
            }
	}
	}
	else
	{
		// ChannelGroup specific functions here
		// FMOD::ChannelGroup *group = (FMOD::ChannelGroup *)channelControl;
		// do we need to handle channel groups?
	
	}
	
	
	return fmodResult;
}

void SoundChannel::playSound(bool isResuming)
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::playSound(%p)", this);
	playSound(this, isResuming);
}

bool SoundChannel::isHeardGlobally() const
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::isHeardGlobally(%p)", this);

	if (Workspace* workspace = RBX::ServiceProvider::find<Workspace>(this))
	{
		return isDescendantOf(workspace);
	}

	return false;
}

bool SoundChannel::isHeardLocally(const Instance* context) const
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::isHeardLocally(%p, %p)", this, context);
	const Network::Player* player = Network::Players::findConstLocalPlayer(context);

	if (context == this)
	{
		context = context->getParent();
	}

	const Instance* parent = context;
	while (parent)
	{
		// Local stock sounds play
		if (Instance::fastDynamicCast<const SoundService>(parent))
			return true;

		// Sounds inside the Workspace may always play
		if (Instance::fastDynamicCast<const Workspace>(parent))
			return true;
		
		// Sounds inside the CoreGui may always play
		if (Instance::fastDynamicCast<const CoreGuiService>(parent))
			return true;
		
		// Sounds inside the local Player may play
		if (parent == player)
			return true;

		parent = parent->getParent();
	}

	
	return false;
}

void SoundChannel::playSound(const Instance* context, bool isResuming)
{
	FASTLOG4(DFLog::SoundTrace, "SoundChannel::playSound(%p, %p, %p, %d)", this, context, fmod_channel, soundDisabled);
	if (!soundDisabled)
	{
		try
		{
			SoundService* soundService = ServiceProvider::create<SoundService>(context);

			if (!isHeardLocally(context))
			{
				FASTLOG2(DFLog::SoundTrace, "Context %p is not heard locally for %p.", context, this);
				return;
			}

			if (fmod_channel && getLooped() && isPlaying())
			{
				update3D(fmod_channel);

				FASTLOG1(FLog::Sound, "Playing a looped sound on %p.", this);
				SoundService::checkResult(fmod_channel->setPaused(false), "setPaused play existing", this, fmod_channel);
			}
			else
			{
				// If the service is not reachable yet, then put this off until later
				if (!soundService)
				{
					FASTLOG1(DFLog::SoundTrace, "SoundChannel::playSound(%p) soundService not found", this);
                    return;
				}
				else
				{
					if (!soundService->enabled())
					{
						FASTLOG(FLog::Sound, "Sound service is not enabled.");
						soundDisabled = true;
                        return;
					}
					else
					{
						RBXASSERT(sound);
						if (!sound) // just in case we missed some case during testing
						{
							return;
						}

						if (fmod_channel)
						{
							update3D(fmod_channel);

							if (isResuming)
							{
								if ( FMOD_OK == fmod_channel->setPaused(false) )
								{
									return;
								}
							}
							else
							{
								if (FMOD_OK == fmod_channel->setPosition(0, FMOD_TIMEUNIT_MS))
								{
									setSoundPosition(0);
									if ( FMOD_OK == fmod_channel->setPaused(false) )
									{
										return;
									}
								}
							}
							
						}
							
						sound = soundService->loadSound(getSoundId(), is3D);
						sound->acquire();
						FMOD::Sound* fmod_sound = sound->tryLoad(context);
						RBXASSERT(fmod_sound);
						if (!fmod_sound)
						{
                            FASTLOGS(FLog::Sound, "SoundId %s return no data.", getSoundId().c_str());
							return;
						}

						if (FLog::Sound)
						{
							std::stringstream ss;
							ss << reinterpret_cast<void*>(this) << " playing soundId " << getSoundId().toString() << ", is3D = " << is3D;
							FASTLOGS(FLog::Sound, "SoundChannel %s", ss.str().c_str());
						}

						FMOD_RESULT fmodResult = soundService->system->playSound(fmod_sound, NULL, true, &fmod_channel);
						if (FMOD_OK != fmodResult)
						{
							FASTLOGS(FLog::Sound, "Failed to play soundId = %s", getSoundId().c_str());
							SoundService::checkResult(fmodResult, "playSound", this, soundService->system.get());
							return;
						}

						FASTLOG2(DFLog::SoundTrace, "SoundChannel %p has new fmod_channel %p", this, fmod_channel);
						invalidChannel = false;

						updateLooped();

						SoundService::checkResult(fmod_sound->getDefaults(&defaultFrequency, NULL), "getDefaults play", this, fmod_sound);
						if (pitch != 1)
						{
							SoundService::checkResult(fmod_channel->setFrequency(pitch * defaultFrequency), "setFrequency play", this, fmod_channel);
						}

						// Wire up callbacks
						SoundService::checkResult(fmod_channel->setUserData(reinterpret_cast<void*>(this)), "setUserData play", this, fmod_channel);
						SoundService::checkResult(fmod_channel->setCallback(&callbackChannelEnd), "setCallback play", this, fmod_channel);

						// add to master group (for master volume control)
						fmod_channel->setChannelGroup(soundService->getMasterChannel());

						FASTLOG1F(DFLog::SoundTrace, "Volume is %f", volume);
						SoundService::checkResult(fmod_channel->setVolume(volume), "setVolume play", this, fmod_channel);

						// make sure our channel is in sync with our sound position data
						setSoundPosition(soundPositionSeconds);

						// make sure sound is in right position for volume
						update3D(fmod_channel);

						// Now we are ready to play!
						SoundService::checkResult(fmod_channel->setPaused(false), "setPaused play", this, fmod_channel);
					}
				}
			}
		}
		catch (RBX::base_exception& e)
		{
			// What should we do in case of an error?  Throwing this exception further might be a problem
			StandardOut::singleton()->print(MESSAGE_ERROR, e);
		}
	}
	else
	{
		FASTLOG(DFLog::SoundTrace, "Sound service is currently disabled.");
	}
}

void SoundChannel::playLocal(const Instance *context)
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::playLocal(%p, %p)", this, context);
	playSound(context);
}

void SoundChannel::playLocal()
{
	playLocal(this);
}

void SoundChannel::resume()
{
	if (isPaused())
	{
		playSound(true);
		playCount = reqPlayCount = std::max(reqPlayCount + 1, 1);
		if (controlledByAndIsServer())
		{
			event_soundResumedFromServer.fireAndReplicateEvent(this, getSoundPosition());
		}
	}
}

void SoundChannel::play()
{
	FASTLOG1(DFLog::SoundTrace, "SoundChannel::play(%p)", this);
    loadSound(this, true);
	playCount = reqPlayCount = std::max(reqPlayCount + 1, 1);
	this->raiseChanged(sound_desc_PlayCount);
	soundPlayedSignal(getSoundId().toString());
}

void SoundChannel::pauseLocal()
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::pauseLocal(%p, %p)", this, fmod_channel);
	if (fmod_channel)
	{
		bool currentlyPaused = false;
		fmod_channel->getPaused(&currentlyPaused);
		if (!currentlyPaused)
		{
			soundPausedSignal(getSoundId().toString());
		}

		SoundService::checkResultNoThrow(fmod_channel->setPaused(true), "setPaused", this, fmod_channel);
	}
}

void SoundChannel::pause()
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::pause(%p, %p)", this, fmod_channel);
	pauseLocal();

	if (0 != reqPlayCount)
	{
		playCount = reqPlayCount = 0;
		this->raiseChanged(sound_desc_PlayCount);
	}
}

bool SoundChannel::controlledByAndIsServer() const
{
	return SoundService::soundDisabled && RBX::Network::Players::serverIsPresent(this) && isHeardGlobally();
}

void SoundChannel::stop()
{
	FASTLOG2(DFLog::SoundTrace, "SoundChannel::stop(%p, %p)", this, fmod_channel);
	if (fmod_channel)
	{
		bool currentlyPaused = false;
		fmod_channel->getPaused(&currentlyPaused);
		if (!currentlyPaused)
		{
			soundStoppedSignal(getSoundId().toString());
		}
		
		SoundService::checkResultNoThrow(fmod_channel->setUserData(reinterpret_cast<void*>(NULL)), "setUserData", this, fmod_channel);
		SoundService::checkResultNoThrow(fmod_channel->stop(), "stop", this, fmod_channel);
		fmod_channel = NULL;

		numOfTimesLooped = 0;
	}
	
	// make sure sound position gets set to the right spot
	setSoundPosition(0);

	if (-1 != reqPlayCount)
	{
		playCount = reqPlayCount = -1;

		this->raiseChanged(sound_desc_PlayCount);
	}
}

} // namespace Soundscape
} // namespace RBX


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag2 = 0;
    };
};
