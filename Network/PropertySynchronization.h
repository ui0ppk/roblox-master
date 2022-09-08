#pragma once

/*
This unit solves the "ships passing in the night" replication problem.
If two peers change a property simultaneously, then there is a risk that
they will swap values and never settle on a common shared value. For
example, if peer A sets a color to Red and peer B sets a color to Blue,
then after replication peer A will get Blue and peer B will get Red.

The solution involves appointing one peer as the master and the other peer 
as the slave. When the master sets a property, the slave must acknowledge
the change before it can successfully change the same property.

The method involves associating a version number with each property. 
When the master changes the property, it increments the version number. 
The slave must send an acknowledgment to the master that it has received 
the property update (and its current version number). Until then, the 
master will reject any property changes coming from the slave. Since the 
slave will eventually receive the master’s property value, we are 
guaranteed eventual consistency.

Note that a side-effect of this approach is that a slave might change a 
value temporarily. The master will never experience this temporary state.

The implementation is designed to avoid massive memory costs and it 
minimizes network bandwidth. Instead of storing a version number for every 
single property, we rely on the fact that network latency is finite. 
When the master changes a property, we associate a version number with the 
property for a "short" period of time - on the order of a network 
round-trip. Likewise, the slave keeps the association temporarily. If the 
slave doesn’t try to change a property during the lifetime of the version 
number, then it never needs to send an acknowledgement, so we save 
bandwidth. The end result is a modest memory footprint and nearly 
no networking overhead.
*/

namespace RBX { namespace Network { namespace PropSync {

namespace detail
{
	class PropertyKey
	{
		const Reflection::PropertyDescriptor* descriptor;
		Guid::Data guidData;

	public:
		PropertyKey() : descriptor(NULL) 
		{
			guidData.scope.setNull();
			guidData.index = 0;
		}

		PropertyKey(const Reflection::ConstProperty& prop) : descriptor(&prop.getDescriptor())
		{
			guidData.scope.setNull();
			guidData.index = 0;

			if (prop.getInstance())
			{
				if (const Instance* instance = prop.getInstance()->fastDynamicCast<const Instance>())
					instance->getGuid().extract(guidData);
			}
		}

		bool operator ==(const PropertyKey& other) const
		{
			return this->descriptor == other.descriptor && this->guidData.scope == other.guidData.scope && this->guidData.index == other.guidData.index;
		}

		friend std::size_t hash_value(const PropertyKey& key)
		{
			std::size_t seed = boost::hash<const Reflection::PropertyDescriptor*>()(key.descriptor);
			boost::hash_combine(seed, key.guidData.scope.getName());
			boost::hash_combine(seed, key.guidData.index);
			return seed;
		}
	};

	// This class contains common code between the master and the slave
	template<class Item>
	class Base
	{
	protected:
		RBX::Time::Interval expirationTime;
		typedef boost::unordered_map<PropertyKey, Item> Map;
		Map map;
		rbx::timestamped_safe_queue<PropertyKey> expirationQueue;

		Base(RBX::Time::Interval expirationTime):expirationTime(expirationTime) {}

	public:
		size_t itemCount() const { return map.size(); }
		void expireItems()
		{
			PropertyKey prop;
			while (expirationQueue.pop_if_waited(expirationTime, prop))
			{
				typename Map::iterator iter = map.find(prop);
				RBXASSERT(iter != map.end());
				if (RBX::Time::now<RBX::Time::Fast>() > iter->second.expiration)
					// expire the item
					map.erase(iter);
				else
					// Put it back in the queue for another time
					expirationQueue.push(prop);
			}
		}
	};

	class Item
	{
	public:
		int version;
		RBX::Time expiration;
		Item():version(0) {}
	};
	class MasterItem : public Item
	{
	public:
		bool isVersionSent;	// == the version number has been sent to the slave
		int slaveVersion;   // the version that the slave has acknowledged
		MasterItem():isVersionSent(false),slaveVersion(-1) {}
	};
	class SlaveItem : public Item
	{
	public:
		bool isAckSent;	// == an acknowledgement has been sent from slave to master
	};
}

class Master : protected detail::Base<detail::MasterItem>
{
	typedef detail::Base<detail::MasterItem> Super;
public:
	unsigned int propertyRejectionCount;
	Master():Super(RBX::Time::Interval(2)),propertyRejectionCount(0) {}

	// Call this periodically to cull the database
	void expireItems()
	{
		Super::expireItems();
	}

	// Call this function when the master changes a property:
	void onPropertyChanged(Reflection::ConstProperty prop)
	{
		// get-or-create the Item
		// TODO: use emplace when it becomes available
		std::pair<Map::iterator, bool> pair = map.insert(Map::value_type(detail::PropertyKey(prop), detail::MasterItem()));
		detail::MasterItem& item(pair.first->second);
		bool createdItem = pair.second;

		if (createdItem)
		{
			RBXASSERT(item.version==0);
			RBXASSERT(!item.isVersionSent);
			// Set the expiration, queue for expiration
			item.expiration = RBX::Time::now<RBX::Time::Fast>() + expirationTime;
			expirationQueue.push(detail::PropertyKey(prop));
		}
		else if (item.isVersionSent)
		{
			item.version++;
			item.isVersionSent = false;
			// touch the expiration time
			item.expiration = RBX::Time::now<RBX::Time::Fast>() + expirationTime;
		}
	}

	// Call this function when the master is about to send a property.
	// The returned value specifies if a version reset message should
	// be sent to the slave
	typedef enum { SendVersionReset, DontSendVersionReset } PropertySendResult;
	PropertySendResult onPropertySend(Reflection::ConstProperty prop)
	{
		// get-or-create the Item
		// TODO: use emplace when it becomes available
		std::pair<Map::iterator, bool> pair = map.insert(Map::value_type(detail::PropertyKey(prop), detail::MasterItem()));
		detail::MasterItem& item(pair.first->second);
		bool createdItem = pair.second;

		// we assume that the caller will send the Item now
		item.isVersionSent = true;	

		// Find out if this is a brand new Item
		if (createdItem)
		{
			// Set the expiration, queue for expiration and request a version reset
			item.expiration = RBX::Time::now<RBX::Time::Fast>() + expirationTime;
			expirationQueue.push(detail::PropertyKey(prop));
			RBXASSERT(item.version == 0);
			return SendVersionReset;
		}

		return item.version == 0 ? SendVersionReset : DontSendVersionReset;

	}

	// Call this function when an acknowledgement is received from the slave
	void onReceivedAcknowledgement(Reflection::ConstProperty prop, int version)
	{
		Map::iterator iter = map.find(detail::PropertyKey(prop));
		if (iter == map.end())
			return;

		iter->second.slaveVersion = version;
	}

	// Call this function when a property is received from the slave
	FilterResult onReceivedPropertyChanged(Reflection::ConstProperty prop)
	{
		Map::iterator iter = map.find(detail::PropertyKey(prop));
		if (iter == map.end())
			return Accept;

		detail::MasterItem& item(iter->second);
		if (item.slaveVersion == item.version)
			return Accept;

		propertyRejectionCount++;
		return Reject;
	}

	size_t itemCount() const
	{
		return Super::itemCount();
	}

	void setExpiration(RBX::Time::Interval expirationTime)
	{
		this->expirationTime = expirationTime;
	}
};

class Slave : protected detail::Base<detail::SlaveItem>
{
	typedef detail::Base<detail::SlaveItem> Super;
public:
	unsigned int ackCount;
	Slave():Super(RBX::Time::Interval(4)),ackCount(0) {}

	// Call this periodically to cull the database
	void expireItems()
	{
		Super::expireItems();
	}

	// Call this function when the slave receives a property from the master
	void onReceivedPropertyChanged(Reflection::ConstProperty prop, bool versionReset)
	{
		// get-or-create the Item
		// TODO: use emplace when it becomes available
		std::pair<Map::iterator, bool> pair = map.insert(Map::value_type(detail::PropertyKey(prop), detail::SlaveItem()));
		detail::SlaveItem& item(pair.first->second);
		bool createdItem = pair.second;

		if (createdItem)
		{
            // Disabling this for now.
			// RBXASSERT(versionReset);

			// Set the expiration, queue for expiration and request a version reset
			item.expiration = RBX::Time::now<RBX::Time::Fast>() + expirationTime;
			expirationQueue.push(detail::PropertyKey(prop));
		}
		else
		{
			// touch the expiration time
			item.expiration = RBX::Time::now<RBX::Time::Fast>() + expirationTime;
		}

		if (versionReset)
			item.version = 0;
		else
			item.version++;

		item.isAckSent = false;
	}

	// Call this function when the slave is about to send a property.
	// If it returns SendAcknowledgement, then first send a message with the
	// specified version number
	typedef enum { SendAcknowledgement, DontSendAcknowledgement } PropertySendResult;
	PropertySendResult onPropertySend(Reflection::ConstProperty prop, int& version)
	{
		Map::iterator iter = map.find(detail::PropertyKey(prop));
		if (iter == map.end())
			return DontSendAcknowledgement;

		if (iter->second.isAckSent)
			return DontSendAcknowledgement;

		iter->second.isAckSent = true;
		version = iter->second.version;
		ackCount++;
		return SendAcknowledgement;
	}

	size_t itemCount() const
	{
		return Super::itemCount();
	}

	void setExpiration(RBX::Time::Interval expirationTime)
	{
		this->expirationTime = expirationTime;
	}
};

}}}
