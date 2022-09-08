#pragma once

#include <string>

#include "rbx/signal.h"
#include "boost/enable_shared_from_this.hpp"
#include <time.h>

#include "RbxFormat.h"

namespace RBX {

	typedef enum { 
		MESSAGE_OUTPUT,
		MESSAGE_INFO,
		MESSAGE_WARNING,
		MESSAGE_ERROR,
		MESSAGE_SENSITIVE,
        MESSAGE_TYPE_MAX
	} MessageType;

	struct StandardOutMessage {
	public:
		MessageType type;
		std::string message;
		time_t time;
		StandardOutMessage(MessageType type, const char* message) 
			:type(type),message(message)
		{
			::time(&time);
		}
		StandardOutMessage():type(MESSAGE_OUTPUT) {}
	};

	// A very basic singleton for distributing output to diagnostics - similar to stdout & stderr combined
	class StandardOut
		: public boost::enable_shared_from_this<StandardOut>
	{
		boost::mutex sync;
	private:
		// Purpose:
		// Prevent creation of the class outside of the singleton class.
		StandardOut(){	;  }

	public:
		static shared_ptr<StandardOut> singleton();

		
		// Value is true if warning should be allowed to print, false to ignore.
		static bool allowPrintWarnings;

		rbx::signal<void(const StandardOutMessage&)> messageOut;
		
		// Prints an exception if f() throws an exception (passes exception on)
		// TODO: Rewrite this to handle an kind of function????
		static void print_exception(const boost::function0<void>& f, MessageType type, bool rethrow);
		
		void print(MessageType type, const std::string& message);
		void print(MessageType type, const char* message);
		void printf(MessageType type, const char* format, ...) RBX_PRINTF_ATTR(3, 4);
		void print(MessageType type, const std::exception& exp);
	};

}
