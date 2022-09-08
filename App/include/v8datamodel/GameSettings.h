#pragma once

#include "V8DataModel/GlobalSettings.h"

namespace RBX
{
	extern const char *const sGameSettings;
	class GameSettings
		: public GlobalAdvancedSettingsItem<GameSettings, sGameSettings>
	{
	public:
		enum ChatMode { CHAT_AUTO = 0, CHAT_CLASSIC= 1, CHAT_BUBBLE= 2, CHAT_BOTH= 3 } ;

		GameSettings();
		int chatHistory;
		int reportAbuseChatHistory;
		int chatScrollLength;
		bool soundEnabled;
		bool softwareSound;

		bool collisionSoundEnabled;
		float collisionSoundVolume;
		int maxCollisionSounds;

		int bubbleChatMaxBubbles;
		float bubbleChatLifetime;

        float overscanPX;
        float overscanPY;

		rbx::signal<void(bool)> videoRecordingSignal;

		typedef enum { LOW_RES = 0, MEDIUM_RES, HIGH_RES } VideoQuality;
		typedef enum { NEVER = 0, ASK, ALWAYS } UploadSetting;

		VideoQuality getVideoQualitySetting() const { return videoQuality; }
		void setVideoQualitySetting(VideoQuality value);

		GameSettings::UploadSetting getPostImageSetting() const;
		void setPostImageSetting(GameSettings::UploadSetting setting);

		bool hardwareMouse;
		
		bool videoCaptureEnabled;

	private:		
		VideoQuality videoQuality;
	};
}

