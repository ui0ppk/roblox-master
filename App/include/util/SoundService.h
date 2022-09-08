/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Service.h"
#include "V8datamodel/DataModel.h"
#include "V8DataModel/Stats.h"
#include "Util/SoundWorld.h"
#include "Util/SoundChannel.h"
#include "Reflection/Event.h"
#include "Util/IHasLocation.h"

#include "fmod.h"
#include "fmod.hpp"
#include "fmod_errors.h"



#if FMOD_VERSION != 0x00010702
#	error Wrong version of fmod.
#endif


namespace RBX 
{
	typedef enum 
	{
		FADE_STATUS_NONE = 0,
		FADE_STATUS_IN,
		FADE_STATUS_OUT
	} FadeStatus;

	namespace Soundscape
	{
		class SoundJob;
		class SoundChannel;
		class Sound;
		class SoundId;

		enum ReverbType
		{
			NoReverb = 0,
			GenericReverb,
			PaddedCell,
			Room,
			Bathroom,
			LivingRoom,
			StoneRoom,
			Auditorium,
			ConcertHall,
			Cave,
			Arena,
			Hangar,
			CarpettedHallway,
			Hallway,
			StoneCorridor,
			Alley,
			Forest,
			City,
			Mountains,
			Quarry,
			Plain,
			ParkingLot,
			SewerPipe,
			UnderWater
		};

		enum ListenerType
		{
			CameraListener = 0,
			CFrame,
			ObjectPosition,
			ObjectCFrame
		};

		struct listenerValues
		{
			CoordinateFrame listenCFrame;
			shared_ptr<IHasLocation> listenObject;
		};

		extern const char* const sSoundService;
	
		class SoundService 
			: public DescribedCreatable<SoundService, Instance, sSoundService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
			, public Service
		{
		private:
			typedef DescribedCreatable<SoundService, Instance, sSoundService, Reflection::ClassDescriptor::PERSISTENT_HIDDEN> Super;
			typedef boost::unordered_set<SoundChannel*> SoundChannels;
			friend class SoundChannel;
			shared_ptr<FMOD::System> system;
			typedef boost::unordered_map<SoundType, shared_ptr<SoundChannel> > StockSounds;
			StockSounds stockSounds;
			float  dopplerscale;
			float  distancefactor;
			float  rolloffscale;
			ListenerType currentListenerType;
			listenerValues currentListenerValues;
			shared_ptr<Instance> statsItem;
			ReverbType ambientReverb;
			SoundChannels soundChannels;
			FMOD::ChannelGroup *channelMaster;

			shared_ptr<SoundJob> soundJob;

			rbx::signals::scoped_connection gameSettingsChangedConnection;

			Time nextGarbageCollectTime;
			typedef boost::unordered_map<SoundId, shared_ptr<Sound> > LoadedSounds;
			LoadedSounds loadedSounds;
			LoadedSounds loaded3DSounds;

			float masterChannelFadeTimeMsec;
			FadeStatus masterChannelFadeStatus;

			bool initialized;
			bool muted;

			void openFmod();
			void closeFmod();
			void garbageCollectSounds();
			static void gcSounds(LoadedSounds& sounds);
			static void getSoundStats(const LoadedSounds& sounds, unsigned int& numSounds, unsigned int& numUnusedSounds);
			void updateSoundChannels(const Time::Interval& timeSinceLastStep);
			void updateMasterChannelGroup(const Time::Interval& timeSinceLastStep);

			void update3DSettings();
			void on3DSettingChanged(const Reflection::PropertyDescriptor&) { update3DSettings(); }
			void updateAmbientReverb();

		protected:
			///////////////////////////////////////////////////////////////////////////////////////////////
			// Instance Overrides
			//////////////////////////////////////////////////////////////////////////////
			/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

		public:
			static bool soundDisabled;		// Used to suppress all fmod calls by builds that don't play sound (like web service)

			shared_ptr<Sound> loadSound(SoundId id, bool is3D);

			bool enabled() const { return initialized; }
			SoundService();
			~SoundService();
			void playSound(SoundType sound);

			FMOD::DSP* createDSP(FMOD_DSP_DESCRIPTION &dspdesc);
			int getSampleRate();

			unsigned int getfmod_version() const;
			void getSoundStats(unsigned int& numSounds, unsigned int& numUnusedSounds) const;
			void getChannelsPlaying(int& value) const;
			void muteAllChannels(bool mute);
			bool isMuted();

			void setListener(ListenerType listenerType, shared_ptr<const RBX::Reflection::Tuple> value);
			shared_ptr<const RBX::Reflection::Tuple> getListener();
			CoordinateFrame getListenCFrame(Camera* camera);
			void setMasterVolume(float value);
			float getMasterVolume();

			void setMasterVolumeFadeOut(float timeToFadeMsec);
			void setMasterVolumeFadeIn(float timeToFadeMsec);

			void gameSettingsChanged(const Reflection::PropertyDescriptor* propertyDescriptor);

			FMOD::ChannelGroup* getMasterChannel() { return channelMaster; }

			struct CpuStats
			{
				float total;
				float dsp;
				float stream;
				float geometry;
				float update;
			};
			void getCpuStats(CpuStats& stats) const;

			ReverbType getAmbientReverb() const { return ambientReverb; }
			void setAmbientReverb(const ReverbType& value);

			static Reflection::BoundProp<float> prop_dopplerscale;
			static Reflection::BoundProp<float> prop_distancefactor;
			static Reflection::BoundProp<float> prop_rolloffscale;
			static Reflection::EnumPropDescriptor<SoundService, ReverbType> prop_AmbientReverb;

			void registerSoundChannel(SoundChannel *soundChannel);
			void unregisterSoundChannel(SoundChannel *soundChannel);

            void step(const Time::Interval& timeSinceLastStep);

            static void checkResultNoThrow(FMOD_RESULT result, const char* fmodOperation, const void *rbxFmodParent, const void *fmodObject);
            static void checkResult(FMOD_RESULT result, const char* fmodOperation, const void *rbxFmodParent, const void *fmodObject);

            static bool convert(const G3D::Vector3& src, FMOD_VECTOR& dst);
		};


		// Responsible for updating all sound logic
		class SoundJob : public DataModelJob
		{
		private:
			SoundService* const soundService;
			const double fps;
		public:
			SoundJob(SoundService* soundService)
				:DataModelJob("Sound", DataModelJob::Write, false,
				shared_from_dynamic_cast<DataModel>(DataModel::get(soundService)), Time::Interval(0.003))
				,fps(30)
				,soundService(soundService)
			{
				cyclicExecutive = true;
			}

			Time::Interval sleepTime(const Stats& stats)
			{
				return computeStandardSleepTime(stats, fps);
			}

			virtual Job::Error error(const Stats& stats)
			{
				return computeStandardErrorCyclicExecutiveSleeping(stats, fps);
			}

			TaskScheduler::StepResult stepDataModelJob(const Stats& stats)
			{
				soundService->step(stats.timespanSinceLastStep);

				return TaskScheduler::Stepped;
			}
		};

		class SoundServiceStatsItem : public Stats::Item
		{
			const SoundService* service;
			size_t currentalloced;
			size_t maxalloced;
			unsigned int numSounds;
			unsigned int numUnusedSounds;
			int channelsPlaying;
			SoundService::CpuStats cpuStats;
		public:
			SoundServiceStatsItem(const SoundService* service)
				:service(service),currentalloced(0),maxalloced(0)
			{
				setName("Sound");
			}

			static shared_ptr<SoundServiceStatsItem> create(const SoundService* service)
			{
				shared_ptr<SoundServiceStatsItem> result = Creatable<Instance>::create<SoundServiceStatsItem>(service);
				Stats::Item* cpu = result->createBoundPercentChildItem("CPU", result->cpuStats.total);
				cpu->createBoundPercentChildItem("Dsp", result->cpuStats.dsp);
				cpu->createBoundPercentChildItem("Stream", result->cpuStats.stream);
				cpu->createBoundPercentChildItem("Geometry", result->cpuStats.geometry);
				cpu->createBoundPercentChildItem("Update", result->cpuStats.update);
				result->createBoundChildItem("ChannelsPlaying", result->channelsPlaying);
				result->createBoundMemChildItem("Current", result->currentalloced);
				result->createBoundMemChildItem("Max", result->maxalloced);
				result->createBoundChildItem("# Sounds", result->numSounds);
				result->createBoundChildItem("# Unused", result->numUnusedSounds);
				return result;
			}
            
			/*override*/ void update()
            {
				if (service->enabled())
				{
					this->formatValue(service->getfmod_version(), "fmod %08x", service->getfmod_version());
					int a, b;
					if (FMOD::Memory_GetStats(&a, &b)==FMOD_OK)
					{
						currentalloced = (size_t) a;
						maxalloced = (size_t) b;
					}
					service->getSoundStats(numSounds, numUnusedSounds);
					service->getChannelsPlaying(channelsPlaying);
					service->getCpuStats(cpuStats);
				}
				else
				{
					this->setValue(0, "-disabled-");
				}
			}
		};


	} // namespace Soundscape
} // namespace RBX