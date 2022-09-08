#pragma once

#include "V8DataModel/GlobalSettings.h"

namespace RBX
{
	extern const char *const sLuaSettings;
	class LuaSettings
		: public GlobalAdvancedSettingsItem<LuaSettings, sLuaSettings>
	{
	public:
		LuaSettings();
		int gcPause;
		int gcStepMul;
		double defaultWaitTime;
        double smallestWaitTime;

		int gcLimit;			//Ideal limit above which we trigger aggressive garbage collection, in average KB per gcFrequency
		int gcFrequency;		//How many heartbeats between maunal GC steps

		bool areScriptStartsReported;
		float waitingThreadsBudget;		// 0..1  A percentage
	};

}

