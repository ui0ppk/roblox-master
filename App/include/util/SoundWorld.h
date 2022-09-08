/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

namespace RBX
{
	typedef enum SoundType {	NO_SOUND = 0,
								BOING_SOUND,
								BOMB_SOUND,
								BREAK_SOUND,
								CLICK_SOUND, 
								CLOCK_SOUND,
								RUBBERBAND_SOUND, 
								PAGE_SOUND, 
								PING_SOUND, 
								SNAP_SOUND, 
								SPLAT_SOUND, 
								STEP_SOUND, 
								STEP_ON_SOUND,
								SWOOSH_SOUND,
								VICTORY_SOUND
								} SoundType;


	class SoundWorld {
	public:

		static SoundType ActionSound() {return PING_SOUND;}
		static SoundType TrashSound() {return PAGE_SOUND;}
		static SoundType ClickSound() {return CLICK_SOUND;}
		static SoundType SplatSound() {return SPLAT_SOUND;}
		static SoundType StepSound() {return STEP_SOUND;}
		static SoundType StepOnSound() {return STEP_ON_SOUND;}
		static SoundType SwooshSound() {return SWOOSH_SOUND;}
		static SoundType LimitSound() {return BOING_SOUND;}
		static SoundType WinSound() {return VICTORY_SOUND;}
		static SoundType LoseSound() {return BOING_SOUND;}
	};
}
