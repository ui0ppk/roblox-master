#include "stdafx.h"

#include "V8DataModel/GameSettings.h"
#include "V8DataModel/GameBasicSettings.h"

namespace RBX {
	namespace Reflection {
		template<>
		EnumDesc<GameSettings::VideoQuality>::EnumDesc()
		:EnumDescriptor("VideoQualitySettings")
		{
			addPair(GameSettings::LOW_RES, "LowResolution");
			addPair(GameSettings::MEDIUM_RES, "MediumResolution");
			addPair(GameSettings::HIGH_RES, "HighResolution");
			addLegacyName("Low Resolution", GameSettings::LOW_RES);
			addLegacyName("Medium Resolution", GameSettings::MEDIUM_RES);
			addLegacyName("High Resolution", GameSettings::HIGH_RES);
		}
		
		template<>
		EnumDesc<GameSettings::UploadSetting>::EnumDesc()
		:EnumDescriptor("UploadSetting")
		{
			addPair(GameSettings::NEVER, "Never");
			addPair(GameSettings::ASK, "Ask");
			addLegacyName("Ask me first", GameSettings::ASK);
			addPair(GameSettings::ALWAYS, "Always");
		}
	}//namespace Reflection
}//namespace RBX

using namespace RBX;
const char *const RBX::sGameSettings = "GameSettings";

REFLECTION_BEGIN();
Reflection::BoundProp<int> prop_ChatHistory("ChatHistory", "Online", &GameSettings::chatHistory);
Reflection::BoundProp<int> prop_ReportAbuseChatHistory("ReportAbuseChatHistory", "Online", &GameSettings::reportAbuseChatHistory);
Reflection::BoundProp<int> prop_ChatScrollLength("ChatScrollLength", "Online", &GameSettings::chatScrollLength);
Reflection::BoundProp<bool> prop_SoundEnabled("SoundEnabled", "Sound", &GameSettings::soundEnabled);
Reflection::BoundProp<bool> prop_SoftwareSound("SoftwareSound", "Sound", &GameSettings::softwareSound);
Reflection::BoundProp<bool> prop_CollisionSoundEnabled("CollisionSoundEnabled", "Sound", &GameSettings::collisionSoundEnabled, Reflection::PropertyDescriptor::Attributes::deprecated());
Reflection::BoundProp<float> prop_CollisionSoundVolume("CollisionSoundVolume", "Sound", &GameSettings::collisionSoundVolume, Reflection::PropertyDescriptor::Attributes::deprecated());
Reflection::BoundProp<int> prop_MaxCollisionSounds("MaxCollisionSounds", "Sound", &GameSettings::maxCollisionSounds, Reflection::PropertyDescriptor::Attributes::deprecated());
Reflection::BoundProp<int> prop_bubbleChatMaxBubbles("BubbleChatMaxBubbles", "Online", &GameSettings::bubbleChatMaxBubbles);
Reflection::BoundProp<float> prop_bubbleChatLifetime("BubbleChatLifetime", "Online", &GameSettings::bubbleChatLifetime);

#if defined(RBX_PLATFORM_DURANGO)
Reflection::BoundProp<float> prop_overscanPX("OverscanPX", category_Video, &GameSettings::overscanPX);
Reflection::BoundProp<float> prop_overscanPY("OverscanPY", category_Video, &GameSettings::overscanPY);
#endif

Reflection::BoundProp<bool> prop_hardwareMouse("HardwareMouse", "Input", &GameSettings::hardwareMouse);

static const Reflection::EnumPropDescriptor<GameSettings, GameSettings::VideoQuality> prop_videoSettings("VideoQuality", category_Video, &GameSettings::getVideoQualitySetting, &GameSettings::setVideoQualitySetting);
static Reflection::EventDesc<GameSettings, void(bool)> event_videoRecordingRequest(&GameSettings::videoRecordingSignal, "VideoRecordingChangeRequest","recording",Security::RobloxScript);
Reflection::BoundProp<bool> prop_videoCaptureEnabled("VideoCaptureEnabled", category_Video, &GameSettings::videoCaptureEnabled);
REFLECTION_END();

GameSettings::GameSettings(void)
	:chatHistory(100)
	,reportAbuseChatHistory(50)
	,chatScrollLength(5)
	,soundEnabled(true)
	,collisionSoundEnabled(true)
	,collisionSoundVolume(10)
	,maxCollisionSounds(-1)
	,softwareSound(false)
	,bubbleChatMaxBubbles(3)
	,bubbleChatLifetime(30.0f)
	,videoQuality(MEDIUM_RES)
	,videoCaptureEnabled(true)
	,hardwareMouse(false)
    ,overscanPX(-1)
    ,overscanPY(-1)
{
	setName("Game Options");
}

GameSettings::UploadSetting GameSettings::getPostImageSetting() const
{
	return GameBasicSettings::singleton().getPostImageSetting();
}

void GameSettings::setPostImageSetting(GameSettings::UploadSetting setting) 
{
	GameBasicSettings::singleton().setPostImageSetting(setting);
}

void GameSettings::setVideoQualitySetting(VideoQuality value)
{
	if(videoQuality != value){
		videoQuality = value;

		raisePropertyChanged(prop_videoSettings);
	}
}

