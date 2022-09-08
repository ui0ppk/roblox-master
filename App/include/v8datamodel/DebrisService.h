#pragma once

#include "V8Tree/Service.h"
#include <queue>

namespace RBX {

class TimerService;

extern const char* const sDebrisService;
class DebrisService 
	: public DescribedNonCreatable<DebrisService, Instance, sDebrisService>
	, public Service
{	
private:
	typedef DescribedNonCreatable<DebrisService, Instance, sDebrisService> Super;
	std::queue<weak_ptr<Instance> > queue;
	int maxItems;
	bool legacyMaxItems;
	shared_ptr<TimerService> timer;
public:
	DebrisService();
	void addItem(shared_ptr<Instance> item, double lifetime);
	void setMaxItems(int value);
	int getMaxItems() const { return maxItems; }

	void setLegacyMaxItems(bool);
protected:
	virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
private:
	void cleanup();
};

}
