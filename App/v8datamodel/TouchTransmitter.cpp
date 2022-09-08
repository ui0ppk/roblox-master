/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/TouchTransmitter.h"
#include "V8DataModel/Workspace.h"

namespace RBX {
	const char *const sTouchTransmitter = "TouchTransmitter";

	// This class encapsulates the de-bouncing of duplicate touch/untouch events.
	// Note that there is a minor potential for memory leak, but not much. If a Part
	// gets a bunch of touch events and then never any more, then it will keep the
	// vector filled with old data until it is destroyed.
	class TouchDebouncer
	{
		struct Item
		{
			weak_ptr<PartInstance> other;
			TouchPair::Type lastType;
			RBX::Time expireTime;
		};
		std::vector<Item> items;
		boost::mutex mutex;

	public:
		// This function in O(n), but that is probably the best compromise for
		// performance. 1) The list will be small for most parts. 2) Managing
		// expirations for a hash set is much more complicated. Here we just
		// test for expiration during the sweep. Another edge case could occur
		// if old objects pile up at the end of the vector. However, a new touch
		// object always causes a full sweep of the vector, so this edge case
		// can't lead to an indefinite leak: It will be small and finite
		bool checkTouch(const shared_ptr<PartInstance>& other, TouchPair::Type type)
		{
			const boost::mutex::scoped_lock lock(mutex);

			// Expire after 5 seconds - a reasonably long time for network lag
			static const RBX::Time::Interval expiration = RBX::Time::Interval(5);

			const RBX::Time now = RBX::Time::nowFast();

			for (size_t i = 0; i < items.size(); )
			{
				Item& item = items[i];
				if (item.other.lock() == other)
				{
					item.expireTime = now + expiration;
					if (item.lastType != type)
					{
						item.lastType = type;
						//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Touch accepted");
						return true;
					}
					else
					{
						//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Touch rejected");
						// Rejected!
						return false;
					}
				}
				else if (now > item.expireTime)
				{
					//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Touch expired");
					// fast-delete the entry
					if (i < items.size() - 1)
						item = items[items.size() - 1];
					items.resize(items.size() - 1);
					// don't increment i
				}
				else
					++i;
			}

			// No entry found. Add one:
			Item item = { other, type, now + expiration };
			items.push_back(item);
			return true;
		}
	};

	TouchTransmitter::TouchTransmitter()
		:touchDebouncer(new TouchDebouncer())
	{
		setName("TouchInterest");
	}
	TouchTransmitter::~TouchTransmitter()
	{
	}

	bool TouchTransmitter::checkTouch(const shared_ptr<PartInstance>& other)
	{
		return touchDebouncer->checkTouch(other, TouchPair::Touch);
	}
	bool TouchTransmitter::checkUntouch(const shared_ptr<PartInstance>& other)
	{
		return touchDebouncer->checkTouch(other, TouchPair::Untouch);
	}
}


// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag7 = 0;
    };
};
