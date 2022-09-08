#pragma once

#include "util/standardout.h"

namespace RBX { namespace Test {

class CaptureErrorLogs
{
	rbx::signals::scoped_connection connection;
	static void onMessage(const RBX::StandardOutMessage& message)
	{
		BOOST_CHECK_MESSAGE(message.type != RBX::MESSAGE_ERROR, message.message);
		// ??? BOOST_WARN_MESSAGE(message.type != RBX::MESSAGE_WARNING, message.message);
	}
public:
	CaptureErrorLogs()
	{
		connection = RBX::StandardOut::singleton()->messageOut.connect(&CaptureErrorLogs::onMessage);
	}
};

}}