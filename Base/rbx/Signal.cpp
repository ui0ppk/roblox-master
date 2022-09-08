#include "rbx/signal.h"

LOGVARIABLE(ScopedConnection, 0);

struct Init
{
	static void initStaticData()
	{
	}
	Init()
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(&initStaticData, flag);
	}
};
static Init init;

using namespace rbx;
using namespace signals;

boost::function<void(std::exception&)> rbx::signals::slot_exception_handler(0);

void connection::disconnect() const 
{ 
	boost::intrusive_ptr<islot> s(weak_slot.lock());
	if (s) 
		s->disconnect();
}

bool connection::connected() const 
{ 
	boost::intrusive_ptr<islot> s(weak_slot.lock());
	return s && s->connected(); 
}

bool connection::operator== (const connection& other) const 
{ 
	return weak_slot.lock() == other.weak_slot.lock(); 
}

bool connection::operator!= (const connection& other) const 
{ 
	return weak_slot.lock() != other.weak_slot.lock(); 
}

connection& connection::operator= (const connection& con)
{
	weak_slot = con.weak_slot;
	return *this;
}

