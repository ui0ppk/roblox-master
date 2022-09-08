/* Copyright 2003-2008 ROBLOX Corporation, All Rights Reserved */

#include "BoostAppend.h"

#include "V8DataModel/PartInstance.h"
#include <boost/multi_index/hashed_index.hpp>

std::size_t boost::hash_value(const shared_ptr<RBX::PartInstance>& b)
{
	boost::hash<void*> hasher;
	return hasher(b.get());
}
