/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/SoundService.h"

#include "Util/SoundChannel.h"
#include "V8DataModel/ContentProvider.h"

#include "V8DataModel/DataModel.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Camera.h"

#include "util/standardout.h"
#include "V8DataModel/GameSettings.h"
#include "V8DataModel/DebugSettings.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/GameBasicSettings.h"
#include "Network/Players.h"

#include "FastLog.h"
#include "rbx/RbxDbgInfo.h"

FASTINTVARIABLE(FMODSoundChannels, 100);

using namespace RBX;
using namespace RBX::Soundscape;

LOGVARIABLE(FMOD, 0)
LOGVARIABLE(Sound, 0)
DYNAMIC_LOGVARIABLE(SoundTiming, 0)
DYNAMIC_LOGVARIABLE(SoundTrace, 0)

FASTFLAGVARIABLE(DebugBreakOnFMODErrors, true)

namespace
{
void writeFMODLog(const char *error, const std::string &fmodOperation, const void *rbxFmodParent, const void *fmodObject)
{
    std::stringstream ss;
    ss << "FMOD object: " << fmodObject << ", parent: " << rbxFmodParent << ", fmodOperation: " << fmodOperation << ", error: " << error;
    FASTLOGS(FLog::FMOD, "%s", ss.str().c_str());
}

struct FMODSystemRelease
{
    void operator()(FMOD::System* system)
    {
        FASTLOG(FLog::Sound, "Releasing FMOD.");
        SoundService::checkResultNoThrow(system->release(), "release", NULL, system);
    }
};
} // namespace

namespace RBX
{
namespace Soundscape
{
const char* const sSoundService = "SoundService";
const char* const sSoundChannel = "Sound";

bool SoundService::soundDisabled = false;

} // namespace Soundscape

namespace Reflection
{
template<>
EnumDesc<ReverbType>::EnumDesc()
	:EnumDescriptor("ReverbType")
{
	addPair(NoReverb, "NoReverb");
	addPair(GenericReverb, "GenericReverb");
	addPair(PaddedCell, "PaddedCell");
	addPair(Room, "Room");
	addPair(Bathroom, "Bathroom");
	addPair(LivingRoom, "LivingRoom");
	addPair(StoneRoom, "StoneRoom");
	addPair(Auditorium, "Auditorium");
	addPair(ConcertHall, "ConcertHall");
	addPair(Cave, "Cave");
	addPair(Arena, "Arena");
	addPair(Hangar, "Hangar");
	addPair(CarpettedHallway, "CarpettedHallway");
	addPair(Hallway, "Hallway");
	addPair(StoneCorridor, "StoneCorridor");
	addPair(Alley, "Alley");
	addPair(Forest, "Forest");
	addPair(City, "City");
	addPair(Mountains, "Mountains");
	addPair(Quarry, "Quarry");
	addPair(Plain, "Plain");
	addPair(ParkingLot, "ParkingLot");
	addPair(SewerPipe, "SewerPipe");
	addPair(UnderWater, "UnderWater");
}
template<>
EnumDesc<ListenerType>::EnumDesc()
	:EnumDescriptor("ListenerType")
{
	addPair(CameraListener, "Camera");
	addPair(CFrame, "CFrame");
	addPair(ObjectPosition, "ObjectPosition");
	addPair(ObjectCFrame, "ObjectCFrame");
}
template<>
RBX::Soundscape::ListenerType& Variant::convert<RBX::Soundscape::ListenerType>(void)
{
	return genericConvert<RBX::Soundscape::ListenerType>();
}
} //namespace Reflection
template<>
bool StringConverter<RBX::Soundscape::ListenerType>::convertToValue(const std::string& text, RBX::Soundscape::ListenerType& value)
{
	return Reflection::EnumDesc<RBX::Soundscape::ListenerType>::singleton().convertToValue(text.c_str(),value);
}
} //namespace RBX

static const Time::Interval gcTime(5);		// GC the sounds every 5 seconds
static const float decay = 0.7;

Reflection::BoundProp<float> SoundService::prop_dopplerscale("DopplerScale", category_Data, &SoundService::dopplerscale, &SoundService::on3DSettingChanged);
Reflection::BoundProp<float> SoundService::prop_distancefactor("DistanceFactor", category_Data, &SoundService::distancefactor, &SoundService::on3DSettingChanged);
Reflection::BoundProp<float> SoundService::prop_rolloffscale("RolloffScale", category_Data, &SoundService::rolloffscale, &SoundService::on3DSettingChanged);
Reflection::EnumPropDescriptor<SoundService, ReverbType> SoundService::prop_AmbientReverb("AmbientReverb", category_Data, &SoundService::getAmbientReverb, &SoundService::setAmbientReverb);
Reflection::BoundFuncDesc<SoundService, void(SoundType)> func_playSound(&SoundService::playSound, "PlayStockSound", "sound", Security::RobloxScript);
Reflection::BoundFuncDesc<SoundService, void(ListenerType, shared_ptr<const RBX::Reflection::Tuple>)> func_setListener(&SoundService::setListener, "SetListener", "listenerType", "listener", Security::None);
Reflection::BoundFuncDesc<SoundService, shared_ptr<const RBX::Reflection::Tuple>()> func_getListener(&SoundService::getListener, "GetListener", Security::None);

SoundService::SoundService()
	:dopplerscale(1.0f) 
	,distancefactor(10.0f)
	,rolloffscale(1.0f)
	,ambientReverb(NoReverb)
	,channelMaster(NULL)
	,currentListenerType(CameraListener)
    ,masterChannelFadeTimeMsec(0)
	,masterChannelFadeStatus(FADE_STATUS_NONE)
	,initialized(false)
    ,muted(false)
{
	setName(sSoundService);
}

unsigned int SoundService::getfmod_version() const
{
    unsigned int version;
    checkResult(system->getVersion(&version), "getVersion", this, system.get());
    return version;
}

void SoundService::openFmod()
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::openFmod(%p)", this);
	if (!soundDisabled && RBX::GameSettings::singleton().soundEnabled)
	{
		try
        {
            FASTLOG(FLog::Sound, "Opening FMOD...");

            FMOD::System *sys = NULL;
            checkResult(FMOD::System_Create(&sys), "System_Create", this, NULL);		// Create the main system object.
            system.reset(sys, FMODSystemRelease());

            unsigned int version;
            checkResult(system->getVersion(&version), "getVersion", this, system.get());
            if (version < FMOD_VERSION)
            {
                FASTLOG(FLog::Error, "Incorrect FMODEX DLL version");
                throw std::runtime_error("Incorrect FMODEX DLL version");
            }

            int driver = -1;
            int num_drivers = 0;
            checkResultNoThrow(system->getNumDrivers(&num_drivers), "getNumDrivers", this, system.get());

            if (num_drivers)
            {
                checkResultNoThrow(system->setDriver(0), "setDriver", this, system.get());
                checkResultNoThrow(system->getDriver(&driver), "getDriver", this, system.get());
                if (driver>=0)
                {
                    checkResultNoThrow(system->getDriverInfo(driver, RBX::RbxDbgInfo::s_instance.AudioDeviceName, DBG_STRING_MAX, NULL, NULL, NULL, NULL), "getDriverInfo", this, system.get());					
                }
            }
            else
            {
                FASTLOG(FLog::Warning, "No active sound driver detected");
                StandardOut::singleton()->print(MESSAGE_WARNING, "No active sound driver detected");
                throw std::runtime_error("No active sound driver detected");
            }



            FMOD_INITFLAGS flags = FMOD_INIT_NORMAL;
            if (DebugSettings::singleton().fmodProfiling)
                flags |= FMOD_INIT_PROFILE_ENABLE ;

            // lets force FMOD to work in stereo, so video recording won't be confused about more than 2 audio channels
            // we do capture only left and right channels of audio into video file if you have 5.1 sound some of the sound could be played
            // only on center channel, so video will miss them

			// In this special case we want to use stereo output and not worry about varying matrix sizes depending on user speaker mode.
            checkResult(system->setSoftwareFormat(48000, FMOD_SPEAKERMODE_DEFAULT, 0), "setSoftwareFormat", this, system.get()); 
            // WARNING 100 - is number of simultaneous channels played by fmod and has nothing todo with output channels of sound card
            checkResult(system->init(FInt::FMODSoundChannels, flags, 0), "init", this, system.get());

			checkResult(system->createChannelGroup(NULL, &channelMaster), "createChannelGroup", this, system.get());
			channelMaster->setVolume(RBX::GameBasicSettings::singleton().getMasterVolume());

			// listen for master volume changes
			gameSettingsChangedConnection = RBX::GameBasicSettings::singleton().propertyChangedSignal.connect(boost::bind(&SoundService::gameSettingsChanged,this,_1));

			update3DSettings();
			if (ambientReverb!=NoReverb)
				updateAmbientReverb();

			initialized = true;

			FASTLOG(FLog::Sound, "FMOD initialization complete!");
		}
		catch (const RBX::base_exception&)
		{
			initialized = false;

			// Failed to initialize sound system
			if (system)
			{
                system.reset();
			}
		}
	}
}

FMOD::DSP* SoundService::createDSP(FMOD_DSP_DESCRIPTION &dspdesc)
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::createDSP(%p)", this);
	FMOD::DSP* dsp = NULL;
	try
	{
		StandardOut::singleton()->printf(MESSAGE_INFO, "SoundService::createDSP: system value = %p, enabled value = %d", system.get(), (int)enabled());

		FMOD::ChannelGroup *masterChannel = 0;
		checkResult(system->getMasterChannelGroup(&masterChannel), "getMasterChannelGroup", this, system.get());
		checkResult(system->createDSP(&dspdesc, &dsp), "createDSP", this, system.get());
		checkResult(dsp->setBypass(false), "setBypass", this, dsp);
		checkResult(masterChannel->addDSP(0, dsp), "addDSP", this, system.get());
	}
	catch (const RBX::base_exception& ex)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "SoundService::createDSP: %s", ex.what());
		if (dsp)
		{
			dsp->release();
		}
		dsp = NULL;
	}
	return dsp;
}

int SoundService::getSampleRate()
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::getSampleRate(%p)", this);
	try
	{
		StandardOut::singleton()->printf(MESSAGE_INFO, "SoundService::getSampleRate: system value = %p, enabled value = %d", system.get(), (int)enabled());

		int samplerate;
		FMOD_SPEAKERMODE speakermode;
		int numrawspeakers;

		checkResult(system->getSoftwareFormat(&samplerate, &speakermode, &numrawspeakers), "getSoftwareFormat", this, system.get());

		StandardOut::singleton()->printf(MESSAGE_INFO, "SoundService::getSampleRate: result = %d", samplerate);
		return samplerate;
	}
	catch (const RBX::base_exception& ex)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "SoundService::getSampleRate: %s", ex.what());
		return -1;
	}
}

SoundService::~SoundService()
{
	RBXASSERT(stockSounds.size()==0);
	RBXASSERT(loadedSounds.size()==0);
	RBXASSERT(loaded3DSounds.size()==0);
	RBXASSERT(!system);
	RBXASSERT(!soundJob);
}

static void releaseSound(const std::pair<SoundId, boost::shared_ptr<Sound> >& p)
{
	FASTLOGS(DFLog::SoundTrace, "releaseSound(%s)", p.first.c_str());
	p.second->release();	// Let fmod clean things up
}

void SoundService::closeFmod()
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::closeFmod(%p)", this);
	FASTLOG(FLog::Sound, "Closing Fmod.");

	gameSettingsChangedConnection.disconnect();

	stockSounds.clear();
	this->removeAllChildren();

	std::for_each(loadedSounds.begin(), loadedSounds.end(), releaseSound);
	std::for_each(loaded3DSounds.begin(), loaded3DSounds.end(), releaseSound);

	loadedSounds.clear();
	loaded3DSounds.clear();

	if (system)
	{
        system.reset();
	}

	initialized = false;

	FASTLOG(FLog::Sound, "Fmod Closed.");
}

static FMOD_REVERB_PROPERTIES reverbs[UnderWater+1];

static void initReverbs()
{
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_OFF; reverbs[NoReverb] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_GENERIC; reverbs[GenericReverb] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_PADDEDCELL; reverbs[PaddedCell] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_PADDEDCELL; reverbs[Room] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_BATHROOM; reverbs[Bathroom] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_LIVINGROOM; reverbs[LivingRoom] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_STONEROOM; reverbs[StoneRoom] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_AUDITORIUM; reverbs[Auditorium] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_CONCERTHALL; reverbs[ConcertHall] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_CAVE; reverbs[Cave] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_ARENA; reverbs[Arena] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_HANGAR; reverbs[Hangar] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_CARPETTEDHALLWAY; reverbs[CarpettedHallway] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_HALLWAY; reverbs[Hallway] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_STONECORRIDOR; reverbs[StoneCorridor] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_ALLEY; reverbs[Alley] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_FOREST; reverbs[Forest] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_CITY; reverbs[City] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_MOUNTAINS; reverbs[Mountains] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_QUARRY; reverbs[Quarry] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_PLAIN; reverbs[Plain] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_PARKINGLOT; reverbs[ParkingLot] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_SEWERPIPE; reverbs[SewerPipe] = p; }
	{ FMOD_REVERB_PROPERTIES p = FMOD_PRESET_UNDERWATER; reverbs[UnderWater] = p; }
}


void SoundService::updateAmbientReverb()
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::updateAmbientReverb(%p)", this);
	if (!system)
		return;

	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&initReverbs, flag);

	checkResultNoThrow(system->setReverbProperties(0, &reverbs[ambientReverb]), "setReverbProperties", this, system.get());
}

void SoundService::update3DSettings()
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::update3DSettings(%p)", this);
	if (system)
		checkResultNoThrow(system->set3DSettings(dopplerscale, distancefactor, rolloffscale), "set3DSettings", this, system.get());
}

void SoundService::setAmbientReverb(const ReverbType& value)
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::setAmbientReverb(%p)", this);
	if (value!=ambientReverb)
	{
		ambientReverb = value;
		raisePropertyChanged(prop_AmbientReverb);
		updateAmbientReverb();
	}
}

void SoundService::setListener(ListenerType listenerType, shared_ptr<const RBX::Reflection::Tuple> value)
{
	if (!((value && value->values.size() > 0) || listenerType == CameraListener))
	{
		throw RBX::runtime_error("SoundService:SetListener called with an incorrect argument.");
		return;
	}

	if (listenerType == CFrame)
	{
		Reflection::ValueArray valueArray = value->values;
		Reflection::ValueArray::iterator tupleIter = valueArray.begin();
		if (tupleIter->isType<CoordinateFrame>())
		{
			currentListenerValues.listenCFrame = tupleIter->cast<CoordinateFrame>();
		}
		else
		{
			throw RBX::runtime_error("SoundService:SetListener value given is not a valid CFrame when given Enum.ListenerType.CFrame");
			return;
		}
	}
	else if (listenerType == ObjectPosition || listenerType == ObjectCFrame)
	{
		Reflection::ValueArray valueArray = value->values;
		Reflection::ValueArray::iterator tupleIter = valueArray.begin();
		if (tupleIter->isType<shared_ptr<Instance> >())
		{
			shared_ptr<Instance> instance = tupleIter->cast<shared_ptr<Instance> >();
			if (shared_ptr<IHasLocation> location = shared_dynamic_cast<IHasLocation>(instance))
			{
				currentListenerValues.listenObject = location;
			}
			else
			{
				throw RBX::runtime_error("SoundService:SetListener value given does not have a location");
				return;
			}
		}
		else
		{
			throw RBX::runtime_error("SoundService:SetListener value given is not a valid instance");
			return;
		}
	}
	currentListenerType = listenerType;
}

shared_ptr<const RBX::Reflection::Tuple> SoundService::getListener()
{
	shared_ptr<RBX::Reflection::Tuple> result(new RBX::Reflection::Tuple(2));
	result->values[0] = currentListenerType;
	if (currentListenerType == CFrame)
	{
		result->values[1] = currentListenerValues.listenCFrame;
	}
	else if (currentListenerType == ObjectPosition || currentListenerType == ObjectCFrame)
	{
		result->values[1] = shared_dynamic_cast<Instance>(currentListenerValues.listenObject);
	}
	return result;
}

void SoundService::playSound(SoundType soundType)
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::playSound(%p)", this);
	StockSounds::iterator iter = stockSounds.find(soundType);
	if (iter!=stockSounds.end())
	{
		if (DFLog::SoundTrace)
		{
			std::stringstream ss;
			ss << reinterpret_cast<void*>(iter->second.get()) << ") with soundId = " << iter->second->getSoundId().toString() << ", isLoaded = " << iter->second->isSoundLoaded();
			FASTLOGS(DFLog::SoundTrace, "SoundService::playSound(%s", ss.str().c_str());
		}
		iter->second->play();
	}
}

void SoundService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	FASTLOG3(DFLog::SoundTrace, "SoundService::onServiceProvider(%p, %p, %p)", this, oldProvider, newProvider);
	TaskScheduler::singleton().removeBlocking(soundJob);
	soundJob.reset();

	if (statsItem) {
		statsItem->setParent(NULL);
		statsItem.reset();
	}

	if (oldProvider)
		closeFmod();

	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		if (!soundDisabled)
		{
			openFmod();

			soundJob = shared_ptr<SoundJob>(new SoundJob(this));
			TaskScheduler::singleton().add(soundJob);
		}
	}

	Stats::StatsService* stats = ServiceProvider::create<Stats::StatsService>(newProvider);
	if (stats) {
		statsItem = SoundServiceStatsItem::create(this);
		statsItem->setParent(stats);
	}
}


bool SoundService::convert(const G3D::Vector3& src, FMOD_VECTOR& dst)
{
	G3D::Vector3 filter = src;

	if (Math::isDenormal(filter.x))
		filter.x = 0;
	if (Math::isDenormal(filter.y))
		filter.y = 0;
	if (Math::isDenormal(filter.z))
		filter.z = 0;

	if (Math::isNanInfDenormVector3(filter))
		return false;

	filter = filter.clamp(-100000, 100000);
	dst.x = filter.x;
	dst.y = filter.y;
	dst.z = filter.z;

	return true;
}

CoordinateFrame SoundService::getListenCFrame(Camera* camera)
{
	CoordinateFrame cf = camera->getCameraCoordinateFrame();
	if (currentListenerType == CFrame)
	{
		cf = currentListenerValues.listenCFrame;
	}
	else if (currentListenerType == ObjectPosition)
	{
		cf.translation = currentListenerValues.listenObject->getLocation().translation;
	}
	else if (currentListenerType == ObjectCFrame)
	{
		cf = currentListenerValues.listenObject->getLocation();
	}
	return cf;
}

void SoundService::step(const Time::Interval& timeSinceLastStep)
{
	if (enabled())
	{
		Workspace* workspace = ServiceProvider::find<Workspace>(this);
		if (workspace!=NULL)
		{
			Camera* camera = workspace->getCamera();
			if (camera!=NULL)
			{
				CoordinateFrame cf = getListenCFrame(camera);
			
				FMOD_VECTOR listener_pos;
				if (convert(cf.translation, listener_pos))
				{
					FMOD_VECTOR listener_forward;
					if (convert(-cf.lookVector(), listener_forward))
					{
						FMOD_VECTOR listener_up;
						if (convert(cf.upVector(), listener_up))
						{
							FMOD_VECTOR listener_vel;
							listener_vel.x = listener_vel.y = listener_vel.z = 0;
							RBX::PVInstance* pv = dynamic_cast<PVInstance*>(camera->getCameraSubject());
							RBX::PartInstance* part = pv ? pv->getPrimaryPart() : NULL;
							if (part)
								convert(part->getVelocity().linear, listener_vel);

							// TODO: Get camera velocity
							FMOD_RESULT fmodResult = system->set3DListenerAttributes(0, &listener_pos, &listener_vel, &listener_forward, &listener_up);
							if (FMOD_OK != fmodResult) // don't overlog during traces
							{
								checkResultNoThrow(fmodResult, "set3DListenerAttributes", this, system.get());     // update 'ears'
							}
						}
					}
				}
			}
		}

		garbageCollectSounds();
	}

	updateMasterChannelGroup(timeSinceLastStep);
	updateSoundChannels(timeSinceLastStep);

	if (enabled())
	{
		FMOD_RESULT fmodResult = system->update();
		if (FMOD_OK != fmodResult) // don't overlog during traces
		{
			checkResultNoThrow(fmodResult, "update", this, system.get());
		}
	}
}

void SoundService::updateMasterChannelGroup(const Time::Interval& timeSinceLastStep)
{
	if (!channelMaster || masterChannelFadeStatus == FADE_STATUS_NONE)
	{
		return;
	}

	const bool muting = (masterChannelFadeStatus == FADE_STATUS_OUT);
	const float storedMasterVolume = muting ? getMasterVolume() : RBX::GameBasicSettings::singleton().getMasterVolume();

	float volumeStep = 0.0f;
	if (masterChannelFadeTimeMsec <= 0.0f)
	{
		volumeStep = getMasterVolume() * (muting ? -1.0f : 1.0f);
	}
	else
	{
		volumeStep = ( (masterChannelFadeTimeMsec/timeSinceLastStep.msec() * storedMasterVolume)/masterChannelFadeTimeMsec ) * (muting ? -1.0f : 1.0f);
	}

	if (!muting)
	{
		muteAllChannels(false);
	}

	if ( muting ? (getMasterVolume() > 0.0f) : (getMasterVolume() < storedMasterVolume))
	{
		float newMasterVolume = getMasterVolume() + volumeStep;
		if (fabs(newMasterVolume) <= 0.01f)
			newMasterVolume = 0.0f;
		else if(newMasterVolume >= 0.99f)
			newMasterVolume = 1.0f;

		setMasterVolume(newMasterVolume);
	}

	if (getMasterVolume() <= 0.0f && muting)
	{
		muteAllChannels(true);
		masterChannelFadeStatus = FADE_STATUS_NONE;
	}
	else if (!muting && getMasterVolume() >= storedMasterVolume)
	{
		setMasterVolume(storedMasterVolume);
		masterChannelFadeStatus = FADE_STATUS_NONE;
	}
}

void SoundService::updateSoundChannels(const Time::Interval& timeSinceLastStep)
{
	FASTLOG1(DFLog::SoundTiming, "Number of channels = %d", soundChannels.size());
	for (SoundChannels::iterator it = soundChannels.begin(); soundChannels.end() != it; ++it)
	{
		SoundChannel *soundChannel = *it;
		soundChannel->updateListenState(timeSinceLastStep);
	}
}


void SoundService::getCpuStats(CpuStats& stats) const
{
	if (!system)
	{
		stats.dsp = 0;
		stats.stream = 0;
		stats.geometry = 0;
		stats.update = 0;
		stats.total = 0;
		return;
	}

	CpuStats stats100;
	system->getCPUUsage(&stats100.dsp, &stats100.stream, &stats100.geometry, &stats100.update, &stats100.total);
	stats.dsp = 0.01 * stats100.dsp;
	stats.geometry = 0.01 * stats100.geometry;
	stats.stream = 0.01 * stats100.stream;
	stats.total = 0.01 * stats100.total;
	stats.update = 0.01 * stats100.update;
}

void SoundService::getSoundStats(const LoadedSounds& sounds, unsigned int& numSounds, unsigned int& numUnusedSounds)
{
	for (LoadedSounds::const_iterator iter = sounds.begin(); iter!=sounds.end(); ++iter)
	{
		++numSounds;
		if (!iter->second->isReferenced())
			++numUnusedSounds;
	}
}

void SoundService::getSoundStats(unsigned int& numSounds, unsigned int& numUnusedSounds) const
{
	numSounds = numUnusedSounds = 0;
	getSoundStats(loadedSounds, numSounds, numUnusedSounds);
	getSoundStats(loaded3DSounds, numSounds, numUnusedSounds);
}

void SoundService::getChannelsPlaying(int& value) const
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::getChannelsPlaying(%p)", this);
	value = 0;
	if (system)
		checkResultNoThrow(system->getChannelsPlaying(&value), "getChannelIsPlaying", this, system.get());
}

bool SoundService::isMuted()
{
	if (channelMaster)
	{
		bool isMuted = false;
		
		FMOD::System* systemCheck = NULL;
		FMOD_RESULT result = channelMaster->getSystemObject(&systemCheck);

		if (result == FMOD_OK && systemCheck)
		{
			channelMaster->getMute(&isMuted);
		}

		return isMuted;
	}
	else
	{
		return false;
	}
}

void SoundService::muteAllChannels(bool mute)
{
	FASTLOG1(DFLog::SoundTrace, "SoundService::muteAllChannels(%p)", this);

	if (channelMaster)
	{
		FMOD::System* systemCheck = NULL;
		FMOD_RESULT result = channelMaster->getSystemObject(&systemCheck);

		if (result == FMOD_OK && systemCheck)
		{
			checkResult(channelMaster->setMute(mute), "setMute", this, channelMaster);
		}
	}
}

void SoundService::gameSettingsChanged(const Reflection::PropertyDescriptor* propertyDescriptor)
{
	if (propertyDescriptor == &RBX::GameBasicSettings::singleton().prop_masterVolume)
	{
		setMasterVolume(RBX::GameBasicSettings::singleton().getMasterVolume());
	}
}

void SoundService::setMasterVolume(float value)
{
	if (channelMaster)
	{
		bool muted = false;
		channelMaster->getMute(&muted);

		if (!muted)
		{
			channelMaster->setVolume(value);
		}
	}
}

float SoundService::getMasterVolume()
{
	float volume = -1;
	if (channelMaster)
	{
		channelMaster->getVolume(&volume);
	}
	return volume;
}

void SoundService::setMasterVolumeFadeIn(float timeToFadeMsec)
{
	masterChannelFadeTimeMsec = G3D::clamp(timeToFadeMsec, 0.0f, 100000.0f);
	masterChannelFadeStatus = FADE_STATUS_IN;
	}
void SoundService::setMasterVolumeFadeOut(float timeToFadeMsec)
{
	masterChannelFadeTimeMsec = G3D::clamp(timeToFadeMsec, 0.0f, 100000.0f);
	masterChannelFadeStatus = FADE_STATUS_OUT;
}

void SoundService::gcSounds(LoadedSounds& sounds)
{
	for (LoadedSounds::iterator iter = sounds.begin(); iter!=sounds.end();)
	{
		if (!iter->second->isReferenced() && G3D::uniformRandom()>=decay)
		{
			FASTLOGS(DFLog::SoundTrace, "Garbage collecting soundId = %s", iter->second->id.c_str());
			iter->second->release();
			sounds.erase(iter++);
		}
		else
		{
			++iter;
		}
	}
}

void SoundService::garbageCollectSounds()
{
	if (nextGarbageCollectTime > Time::now<Time::Fast>())
		return;

	// Now flush the loadedSounds database of empty items
	gcSounds(loadedSounds);
	gcSounds(loaded3DSounds);

	nextGarbageCollectTime = Time::now<Time::Fast>() + gcTime;
}

void SoundService::registerSoundChannel(SoundChannel *soundChannel)
{
	if (FLog::Sound)
	{
		std::stringstream ss;
		ss << "soundChannel = " << reinterpret_cast<void*>(soundChannel) << ", soundId = " << soundChannel->getSoundId().c_str();
		FASTLOGS(FLog::Sound, "Registering with SoundService: %s", ss.str().c_str());
	}

	soundChannels.insert(soundChannel);
}

void SoundService::unregisterSoundChannel(SoundChannel *soundChannel)
{
	SoundChannels::iterator iter = soundChannels.find(soundChannel);
	if (soundChannels.end() != iter)
	{
		if (FLog::Sound)
		{
			SoundChannel *soundChannel = *iter;
			std::stringstream ss;
			ss << "soundChannel = " << reinterpret_cast<void*>(soundChannel) << ", soundId = " << soundChannel->getSoundId().c_str();
			FASTLOGS(FLog::Sound, "Unregistering from SoundService: %s", ss.str().c_str());
		}
		soundChannels.erase(iter);
	}
}

shared_ptr<Sound> SoundService::loadSound(SoundId id, bool is3D)
{
	LoadedSounds& database(is3D ? loaded3DSounds : loadedSounds);

	LoadedSounds::iterator iter = database.find(id);
	// don't return this sound object if we are streaming, each stream needs a new object
	if (iter!=database.end() && !iter->second->getIsStreaming())
	{
		return iter->second;
	}
	shared_ptr<Sound> result(new Sound(system, id, is3D));
	database[id] = result;
	return result;
}

void SoundService::checkResultNoThrow(FMOD_RESULT result, const char* fmodOperation, const void *rbxFmodParent, const void *fmodObject)
{
    if (FMOD_OK != result)
    {
        if (DebugSettings::singleton().soundWarnings || FLog::FMOD)
        {
            char err[512];
            strncpy(err, FMOD_ErrorString(result), 512);
            if (DebugSettings::singleton().soundWarnings)
            {
                StandardOut::singleton()->printf(MESSAGE_WARNING, "FMOD %d: %s during %s", result, err, fmodOperation);
            }
            writeFMODLog(err, fmodOperation, rbxFmodParent, fmodObject);
            RBXASSERT(!FFlag::DebugBreakOnFMODErrors || FMOD_OK == result);
        }
    }
}

void SoundService::checkResult(FMOD_RESULT result, const char* fmodOperation, const void *rbxFmodParent, const void *fmodObject)
{
    if (FMOD_OK != result)
    {
        std::string message = RBX::format("FMOD %d: %s during %s", result, FMOD_ErrorString(result), fmodOperation);
        StandardOut::singleton()->print(MESSAGE_WARNING, message.c_str());
        writeFMODLog(FMOD_ErrorString(result), fmodOperation, rbxFmodParent, fmodObject);
        if (FMOD_OK != result)
        {
            throw std::runtime_error(message);
        }
    }
}
