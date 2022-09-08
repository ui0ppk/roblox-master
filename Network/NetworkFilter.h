#pragma once

#include "reflection/property.h"
#include "reflection/event.h"
#include "network/api.h"
#include <boost/unordered_set.hpp>

namespace RBX
{
	class Instance;
	namespace Network
	{
		class Replicator;

		// Allow only white listed data
		class StrictNetworkFilter
		{
			Replicator* replicator;
			Instance* instanceBeingRemovedFromLocalPlayer;

			static boost::unordered_set<std::string> propertyWhiteList;
			static boost::unordered_set<std::string> eventWhiteList;

		public:
			StrictNetworkFilter(Replicator* replicator);
			~StrictNetworkFilter() {}

			FilterResult filterChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc);
			FilterResult filterParent(Instance* instance, Instance* newParent);
			FilterResult filterEvent(Instance* instance, const Reflection::EventDescriptor& desc);
			FilterResult filterNew(const Instance* instance, const Instance* parent);
			FilterResult filterDelete(const Instance* instance);
			FilterResult filterTerrainCellChange();

			void onChildRemoved(Instance* removed, const Instance* oldParent);
		};

		// Algorithms for filtering common incoming data.
		// Whitelists certain physics packets and certain humanoind packets.
		// Blacklists certain data.
		// Used as a first-pass filtering of known safe data.
		class NetworkFilter
		{
			Replicator* replicator;
		public:
			NetworkFilter(Replicator* replicator);
			~NetworkFilter(void);

			bool filterChangedProperty(Instance* instance, const Reflection::PropertyDescriptor& desc, FilterResult& result);
			bool filterParent(Instance* instance, Instance* parent, FilterResult& result);
			bool filterNew(Instance* instance, Instance* parent, FilterResult& result);
			bool filterDelete(Instance* instance, FilterResult& result);
			bool filterEvent(Instance* instance, const Reflection::EventDescriptor& desc, FilterResult& result);
		};
	}
}
