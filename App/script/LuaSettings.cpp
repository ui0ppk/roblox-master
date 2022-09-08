#include "stdafx.h"

#include "Script/LuaSettings.h"
#include "LuaConf.h"

const char *const RBX::sLuaSettings = "LuaSettings";

REFLECTION_BEGIN();
static RBX::Reflection::BoundProp<int> prop_1("GcPause", "Garbage Collection", &RBX::LuaSettings::gcPause);
static RBX::Reflection::BoundProp<int> prop_2("GcStepMul", "Garbage Collection", &RBX::LuaSettings::gcStepMul);
static RBX::Reflection::BoundProp<double> prop_3("DefaultWaitTime", "Settings", &RBX::LuaSettings::defaultWaitTime);
static RBX::Reflection::BoundProp<int> prop_4("GcLimit", "Garbage Collection", &RBX::LuaSettings::gcLimit);
static RBX::Reflection::BoundProp<int> prop_5("GcFrequency", "Garbage Collection", &RBX::LuaSettings::gcFrequency);
static RBX::Reflection::BoundProp<bool> prop_areScriptStartsReported("AreScriptStartsReported", "Diagnostics", &RBX::LuaSettings::areScriptStartsReported);
static RBX::Reflection::BoundProp<float> prop_AreWaitingThreadsBudgeted("WaitingThreadsBudget", "Settings", &RBX::LuaSettings::waitingThreadsBudget);
REFLECTION_END();

RBX::LuaSettings::LuaSettings(void)
	:gcPause(LUAI_GCPAUSE)
	,gcStepMul(LUAI_GCMUL)
	,defaultWaitTime(0.03)
    ,smallestWaitTime(0.016667)
	,gcLimit(2)
	,gcFrequency(0)
	,areScriptStartsReported(false)
	,waitingThreadsBudget(0.1)
{
	setName("Lua");
}
