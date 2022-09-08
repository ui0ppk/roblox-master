#pragma once


#include "util/runstateowner.h"
#include "g3d/Array.h"
#include "boost/any.hpp"
#include "boost/shared_ptr.hpp"
#include "lua/luabridge.h"
#include "script/threadref.h"
#include <boost/thread/mutex.hpp>

#include <vector>

struct lua_State;
class ThreadInfo;



namespace RBX { 
	class Instance;
	class ScriptContext;

	namespace Lua {

	class YieldingThreads
	{
		ScriptContext* context;

		struct WaitingThread
		{
			boost::intrusive_ptr<WeakThreadRef> thread;
			RBX::Time waitTime;
			RBX::Time resumeTime;
			WaitingThread(lua_State *L, RBX::Time::Interval requestedDelay)
				:thread(new WeakThreadRef(L)),
				waitTime(RBX::Time::now<RBX::Time::Precise>())
			{
				resumeTime = waitTime + requestedDelay;
			}

			bool operator <(const WaitingThread& other) const
			{
				return this->resumeTime > other.resumeTime;
			}
		};
		typedef std::priority_queue< WaitingThread > WaitThreadRefs;

		// Lua refs to threads that are waiting on the event
		WaitThreadRefs waitingThreads;

	public:
		YieldingThreads(ScriptContext* context);

		// Hooking up consumers:
		void queueWaiter(lua_State *L);
		void queueWaiter(lua_State *L, LUA_NUMBER delay);

		void resume(double wallTime, Time expirationTime, bool& throttling);

		std::size_t waiterCount() const;

	private:
		friend class ScriptContext;
		void clearAllSinks();
	};

	// specialization
	template<>
	int Bridge<rbx::signals::connection>::on_tostring(const rbx::signals::connection& object, lua_State *L);

	template<>
	int Bridge<boost::intrusive_ptr<class WeakThreadRef::Node> >::on_tostring(const boost::intrusive_ptr<class WeakThreadRef::Node>& object, lua_State *L);

	template<>
	int Bridge< shared_ptr<GenericFunction> >::on_tostring(const shared_ptr<GenericFunction>& object, lua_State *L);

	template<>
	int Bridge< shared_ptr<GenericAsyncFunction> >::on_tostring(const shared_ptr<GenericAsyncFunction>& object, lua_State *L);

} }
