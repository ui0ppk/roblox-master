/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/SoundWorld.h"
#include "reflection/enumconverter.h"


namespace RBX 
{
	namespace Reflection
	{
		template<>
		Reflection::EnumDesc<SoundType>::EnumDesc()
			:Reflection::EnumDescriptor("SoundType")
		{
			addPair(NO_SOUND, "NoSound");
			addPair(BOING_SOUND, "Boing");
			addPair(BOMB_SOUND, "Bomb");
			addPair(BREAK_SOUND, "Break");
			addPair(CLICK_SOUND, "Click");
			addPair(CLOCK_SOUND, "Clock");
			addPair(RUBBERBAND_SOUND, "Slingshot");
			addPair(PAGE_SOUND, "Page");
			addPair(PING_SOUND, "Ping");
			addPair(SNAP_SOUND, "Snap");
			addPair(SPLAT_SOUND, "Splat");
			addPair(STEP_SOUND, "Step");
			addPair(STEP_ON_SOUND, "StepOn");
			addPair(SWOOSH_SOUND, "Swoosh");
			addPair(VICTORY_SOUND, "Victory");
		}

		template<>
		SoundType& Variant::convert<SoundType>(void)
		{
			return genericConvert<SoundType>();
		}
	} // namespace Reflection

	template<>
	bool StringConverter<SoundType>::convertToValue(const std::string& text, SoundType& value)
	{
		return Reflection::EnumDesc<SoundType>::singleton().convertToValue(text.c_str(),value);
	}

	//std::map<SoundType, FSOUND_SAMPLE*> FModSound::stockSounds;
	//std::map<std::string, FSOUND_SAMPLE*> FModSound::soundMap;

	//FSOUND_SAMPLE* FModSound::loadSound(std::string file)
	//{
	//	std::map<std::string, FSOUND_SAMPLE*>::iterator iter = soundMap.find(file);
	//	if (iter!=soundMap.end())
	//		return iter->second;
	//
	//	// fmod is set to handle cleanup of all resources
	//	FSOUND_SAMPLE* sound = FSOUND_Sample_Load(FSOUND_FREE, file.c_str(), 0, 0, 0);
	//	soundMap[file] = sound;
	//	return sound;
	//}



				//stockSounds[BOING_SOUND] = loadSound(contentDir + "Sounds\\bass.wav");					// FAILURE SOUND
				//stockSounds[BOMB_SOUND] = loadSound(contentDir + "Sounds\\collide.wav");
				//stockSounds[BREAK_SOUND] = loadSound(contentDir + "Sounds\\glassbreak.wav");
				//stockSounds[CLICK_SOUND] = loadSound(contentDir + "Sounds\\switch.wav");
				//stockSounds[CLOCK_SOUND] = loadSound(contentDir + "Sounds\\clickfast.wav");
				//stockSounds[DROP_SOUND] = loadSound(contentDir + "Sounds\\waterdrop.wav");
				//stockSounds[PAGE_SOUND] = loadSound(contentDir + "Sounds\\pageturn.wav");				// PAGE
				//stockSounds[PING_SOUND] = loadSound(contentDir + "Sounds\\electronicpingshort.wav");	// ACTION SOUND
				//stockSounds[SNAP_SOUND] = loadSound(contentDir + "Sounds\\snap.wav");
				//stockSounds[SPLAT_SOUND] = loadSound(contentDir + "Sounds\\splat.wav");
				//stockSounds[STEP_SOUND] = loadSound(contentDir + "Sounds\\SWITCH3.wav");
				//stockSounds[STEP_ON_SOUND] = loadSound(contentDir + "Sounds\\flashbulb.wav");
				//stockSounds[SWOOSH_SOUND] = loadSound(contentDir + "Sounds\\swoosh.wav");
				//stockSounds[VICTORY_SOUND] = loadSound(contentDir + "Sounds\\victory.wav");



} // namespace RBX