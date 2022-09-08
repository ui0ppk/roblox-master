/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Visit.h"
#include "util/StandardOut.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "Util/Http.h"

#include <boost/thread/xtime.hpp>

namespace RBX {
	const char* const sVisit = "Visit";
}

using namespace RBX;
using namespace RBX::Network;

REFLECTION_BEGIN();
// Still used
static Reflection::BoundFuncDesc<Visit, void(std::string)> UploadUrlFunction(&Visit::setUploadUrl, "SetUploadUrl", "url", Security::Roblox);
static Reflection::BoundFuncDesc<Visit, std::string()> getUploadUrl(&Visit::getUploadUrl, "GetUploadUrl", Security::Roblox);
static Reflection::BoundFuncDesc<Visit, void(std::string, int)> desc_setPing( &Visit::setPing, "SetPing", "pingUrl", "interval", Security::Roblox);
REFLECTION_END();


Visit::Visit() 
	: uploadUrl("")
{
	setName("Visit");
}

Visit::~Visit()
{
}

void Visit::setUploadUrl(std::string value)
{
	uploadUrl = value;
}

worker_thread::work_result Visit::ping(std::string url, int interval)
{
	try
	{
		//StandardOut::singleton()->printf(MESSAGE_INFO, "Visit::ping %s", url.c_str());
		std::string response;
		RBX::Http(url).get(response);
	}
	catch (RBX::base_exception& /*exp*/)
	{
		//TODO: This is the kind of message that should only go to the output view for Roblox employees. We should hide it from general users
		//std::string message = RBX::format("ping %s failed: %s", url.c_str(), exp.what());
		//StandardOut::singleton()->printf(MESSAGE_WARNING, message.c_str());
	}

	// TODO: use a timed condition so that interruption can be instantaneous
	boost::xtime xt;
	boost::xtime_get(&xt, boost::TIME_UTC_);
	xt.sec += interval;
	boost::thread::sleep(xt);

	return RBX::worker_thread::more;
}

void Visit::setPing(std::string url, int interval)
{
	if (url!="")
	{
		// Create a new thread
		pingThread.reset(new worker_thread(
			boost::bind(&Visit::ping, url, interval),
			"rbx_visit"));
	}
	else
		// Just stop the current ping thread:
		pingThread.reset();
}



