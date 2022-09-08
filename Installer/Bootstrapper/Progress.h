#pragma once

#include "BootstrapperSite.h"

struct Progress
{
	int value;
	int maxValue;
	bool done;
	Progress():value(0),maxValue(0),done(false) {}
	void update(IInstallerSite *site, int value, int maxValue) { site->CheckCancel(); this->value = value; this->maxValue = maxValue; }
};
