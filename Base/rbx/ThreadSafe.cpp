#include "rbx/boost.hpp"
#include "rbx/ThreadSafe.h"
#include "rbx/Debug.h"
#include "rbx/atomic.h"



RBX::concurrency_catcher::scoped_lock::scoped_lock(concurrency_catcher& m):m(m)
{
	if (m.value.swap(concurrency_catcher::locked) != concurrency_catcher::unlocked)
	{
		RBXCRASH();
	}
}

RBX::concurrency_catcher::scoped_lock::~scoped_lock()
{
	m.value.swap(concurrency_catcher::unlocked);
}

const unsigned long RBX::reentrant_concurrency_catcher::noThreadId = 4493024;

RBX::reentrant_concurrency_catcher::scoped_lock::scoped_lock(RBX::reentrant_concurrency_catcher& m)
	:m(m)
{
	const long threadId(GetCurrentThreadId());
	isChild = threadId == m.threadId;
	if (isChild)
		return;	// Ah ha! I'm a recursive lock request. Just go ahead :)

	if (m.value.swap(reentrant_concurrency_catcher::locked) != reentrant_concurrency_catcher::unlocked)
	{
		RBXCRASH();
	}

	// We own the lock, so assign the current thread ID
	RBXASSERT(m.threadId == reentrant_concurrency_catcher::noThreadId);
	m.threadId = threadId;
}


RBX::reentrant_concurrency_catcher::scoped_lock::~scoped_lock()
{
	if (!isChild)
	{
		m.threadId = reentrant_concurrency_catcher::noThreadId;
		m.value.swap(reentrant_concurrency_catcher::unlocked);
	}
}


RBX::readwrite_concurrency_catcher::scoped_write_request::scoped_write_request(readwrite_concurrency_catcher& m):m(m)
{
	if (m.write_requested.swap(readwrite_concurrency_catcher::locked) != readwrite_concurrency_catcher::unlocked)
		RBXASSERT(false); // should be a RBXCRASH();
	if (m.read_requested > 0)
		RBXASSERT(false); // should be a RBXCRASH();
}
RBX::readwrite_concurrency_catcher::scoped_write_request::~scoped_write_request()
{
	if (m.write_requested.swap(readwrite_concurrency_catcher::unlocked) != readwrite_concurrency_catcher::locked)
		RBXASSERT(false); // should be a RBXCRASH();
}

// Place this code around tasks that write to a DataModel
RBX::readwrite_concurrency_catcher::scoped_read_request::scoped_read_request(readwrite_concurrency_catcher& m):m(m)
{
	if (m.write_requested != readwrite_concurrency_catcher::unlocked)
		RBXASSERT(false); // should be a RBXCRASH();
	++m.read_requested;
}
RBX::readwrite_concurrency_catcher::scoped_read_request::~scoped_read_request()
{
	if (--m.read_requested < 0)
		RBXASSERT(false); // should be a RBXCRASH();
}
