#pragma once

#include "rbx/intrusive_ptr_target.h"
#include "boost/intrusive_ptr.hpp"
#include "rbx/boost.hpp"
#include "rbx/threadsafe.h"
#include "reflection/type.h"

struct lua_State;

using boost::shared_ptr;

LOGGROUP(WeakThreadRef)

namespace RBX { 

	namespace Lua {

		void dumpThreadRefCounts();

	// Used internally
		namespace detail {
			class LiveThreadRef 
				: public rbx::quick_intrusive_ptr_target<LiveThreadRef>
				, public Diagnostics::Countable<LiveThreadRef>
				, boost::noncopyable
			{
				lua_State* L;
				int threadId;
				friend class WeakThreadRef;
				friend class ThreadRef;
			public:
				// Do not create this. It is an internal class
				LiveThreadRef (lua_State* thread);
				~LiveThreadRef();
				bool empty() const {
					return L == NULL;
				}
				lua_State* thread() const {
					return L;
				}

			};
		}

	// You get this by calling lock() on WeakThreadRef
	class ThreadRef
		: public Diagnostics::Countable<ThreadRef>
	{
		boost::intrusive_ptr<detail::LiveThreadRef> liveThreadRef;
		friend class WeakThreadRef;
		ThreadRef (detail::LiveThreadRef* liveThreadRef):liveThreadRef(liveThreadRef) {}
	public:
		ThreadRef() {}
		ThreadRef(lua_State* thread)
			:liveThreadRef(new detail::LiveThreadRef(thread)) {}
		lua_State* get() const {
			return liveThreadRef ? liveThreadRef->thread() : NULL;
		}
		operator lua_State*() const
		{
			return get();
		}
		bool empty() const {
			return liveThreadRef && liveThreadRef->thread() != NULL;
		}
	};

	// Registers a weak reference to a thread, ensuring that it isn't collected (sometimes)
	class WeakThreadRef
		: public rbx::quick_intrusive_ptr_target<WeakThreadRef>
		, boost::noncopyable
		, public Diagnostics::Countable<WeakThreadRef>
	{
		// TODO: boost::mutex would be safer
		typedef rbx::spin_mutex Mutex;
		static Mutex sync;
	public:
		class Node
			: public rbx::quick_intrusive_ptr_target<Node>
			, boost::noncopyable
		{
			friend class WeakThreadRef;
			WeakThreadRef* first;
		public:
			Node():first(0) {}
			~Node();
			static boost::intrusive_ptr<Node> create(lua_State* thread);
			static Node* get(lua_State* thread);

			// Clear all refs to thread and its children
			void eraseAllRefs();

			template<class Func>
			void forEachRefs(Func func)
			{
				for (WeakThreadRef* ref = first; ref!=NULL; ref = ref->next)
				{
					func(ref->lock());
				}
			}
		};
		friend class Node;
	private:
		WeakThreadRef* previous;
		WeakThreadRef* next;
		boost::intrusive_ptr<detail::LiveThreadRef> liveThreadRef;
		void addRef(lua_State* L);
		void addToNode();
		void removeFromNode();
	protected:
		Node* node;
		virtual void removeRef();
		lua_State* thread() const {
			return threadDangerous();
		}
	public:
		WeakThreadRef():node(0), previous(0), next(0) {}
		WeakThreadRef(lua_State* thread);
		WeakThreadRef(const WeakThreadRef& other);
		WeakThreadRef& operator=(const WeakThreadRef& other);
		virtual ~WeakThreadRef();

		bool operator==(const WeakThreadRef& other) const;
		bool operator!=(const WeakThreadRef& other) const;
		void reset();
		bool empty() const {
			return liveThreadRef ? liveThreadRef->thread()==0 : true;
		}
		ThreadRef lock()
		{
			return ThreadRef(liveThreadRef.get());
		}
		lua_State* threadDangerous() const {
			return liveThreadRef ? liveThreadRef->thread() : NULL;
		}

	};

	// A function that takes any number of arguments and returns a tuple
	typedef boost::function<shared_ptr<const Reflection::Tuple>(shared_ptr<const Reflection::Tuple>)> GenericFunction;
	
	class IAsyncResult
	{
	public:
		// This may throw
		virtual boost::shared_ptr<const Reflection::Tuple> getValue() = 0;
		virtual ~IAsyncResult() {}
	};
	// A function that takes any number of arguments and returns the result through a callback
	typedef boost::function<void(shared_ptr<const Reflection::Tuple>, boost::function<void(IAsyncResult*)>)> GenericAsyncFunction;

	class WeakFunctionRef : public WeakThreadRef
	{
	private:
		int functionId;			
		typedef WeakThreadRef Super;
	public:
		WeakFunctionRef():functionId(0) {}
		
		WeakFunctionRef(lua_State* thread, int index);	// Constructs a FunctionRef from the Lua stack
		
		virtual ~WeakFunctionRef();
		
		// Copy:
		WeakFunctionRef(const WeakFunctionRef& other);
		WeakFunctionRef& operator=(const WeakFunctionRef& other);

		// Query:
		bool operator==(const WeakFunctionRef& other) const;
		bool operator!=(const WeakFunctionRef& other) const;

		friend WeakFunctionRef lua_tofunction(lua_State* L);
		friend void lua_pushfunction(lua_State* L, const WeakFunctionRef& function);
	protected:
		virtual void removeRef();
	};

	// Operations with Lua
	WeakFunctionRef lua_tofunction(lua_State* L, int index);
	void lua_pushfunction(lua_State* L, const WeakFunctionRef& function);
	void lua_pushfunction(lua_State* L, shared_ptr<GenericFunction> function);
	void lua_pushfunction(lua_State* L, shared_ptr<GenericAsyncFunction> function);

} }
