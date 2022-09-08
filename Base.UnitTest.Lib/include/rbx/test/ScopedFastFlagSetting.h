#pragma once

// FOR TESTS ONLY.
// DO NOT USE THIS IN ANY NON-TEST CODE
// THIS HAS NO THREADING GUARANTEES, AND IS UNSUITABLE FOR PRODUCTION

// Most tests will not need this. Instead, the command line option 
// --fflags can be used to run tests with different flags enabled / disabled.

// Because FastFlags are static (process scoped), it can be difficult to
// know exactly when in the execution a FastFlag is read. For this reason,
// be extra careful to examine the uses of a FastFlag before setting it
// in a unit test. Beware, some flags are only read at startup time, and
// the effects of the flag are not changed if the flag value is subsequently
// changed.

#include "FastLog.h"
#include "rbx/Debug.h"

struct ScopedFastFlagSetting {
private:
	bool success;
	std::string oldValue;
	const char* flagName;
public:
	ScopedFastFlagSetting(const char* flagName, bool value) : flagName(flagName) {
		success = false;
		success = FLog::GetValue(flagName, oldValue)
			&& FLog::SetValue(flagName, value ? "True" : "False", FASTVARTYPE_ANY, false);
		RBXASSERT(success);
	}
	~ScopedFastFlagSetting() {
		if (success) {
			FLog::SetValue(flagName, oldValue, FASTVARTYPE_ANY, false);
		}
	}
};

struct ScopedFastIntSetting {
private:
	bool success;
	std::string oldValue;
	const char* flagName;
public:
	ScopedFastIntSetting(const char* flagName, int value) : flagName(flagName) {
		success = false;
		std::string newValueString = convertToString(value);
		success = FLog::GetValue(flagName, oldValue)
			&& FLog::SetValue(flagName, newValueString, FASTVARTYPE_ANY, false);
		RBXASSERT(success);
	}
	~ScopedFastIntSetting() {
		if (success) {
			FLog::SetValue(flagName, oldValue, FASTVARTYPE_ANY, false);
		}
	}

	std::string convertToString(int a)
	{
#pragma warning(push)
#pragma warning(disable: 4996)
		char temp[200];
		snprintf(temp, 200-1, "%d", a);
		return temp;
#pragma warning(pop)
	}
};

