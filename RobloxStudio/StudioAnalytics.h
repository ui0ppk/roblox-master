#pragma once

#include <QSettings>
#include <vector>

class StudioAnalytics;

namespace RBX {
    class DataModel;
}

class PersistentVariable
{
	const char* name;
	int accumulatedValue;
	StudioAnalytics* owner;

public:
	PersistentVariable(const char *name, StudioAnalytics* owner);
	void increment(int value = 1) { accumulatedValue += value; }


	const char* getName() { return name; }
	int getAccumulatedValue() { return accumulatedValue; }
	void clearAccumulatedValue() { accumulatedValue = 0; }
};

class StudioAnalytics
{
public:
	StudioAnalytics();

	void savePersistentVariables();
	void reportPersistentVariables();

	void reportPublishStats(RBX::DataModel* dm);

	void registerPersistentVariable(PersistentVariable* variable);

private:
	void reportEvent(const char* name, int value = 0, const char* label = "none", const char* category = "UsageFeatures");
	std::vector<PersistentVariable*> persistentVariables;

	QSettings storage;
};

