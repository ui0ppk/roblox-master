
#pragma once

#include "boost/type_traits.hpp"
#include "boost/any.hpp"
#include "rbx/boost.hpp"
#include "rbx/threadsafe.h"
#include "rbx/Debug.h"
#include <limits>
#include "rbx/Memory.h"
#include "rbx/intrusive_ptr_target.h"
#include "rbx/intrusive_weak_ptr.h"
#include "rbx/callable.h"

#ifdef _WIN32
	#ifdef max
		// Did you include a windows header file without defining NOMINMAX? 
		// If you can't do that, then #undef max instead
		#error
	#endif
#endif

using boost::shared_ptr;
using boost::weak_ptr;

LOGGROUP(ScopedConnection);

#ifdef _DEBUG
#define RBX_SIGNALS_DEBUGGING
#endif

#ifdef RBX_SIGNALS_DEBUGGING
#define RBX_SIGNALS_ASSERT RBX_CRASH_ASSERT
#pragma optimize( "", off )
#else
#define RBX_SIGNALS_ASSERT RBXASSERT
#endif

namespace rbx
{
	// The classes in this namespace mimic a small fraction of the features contained
	// in boost signals  
	namespace signals
	{
		/*
			This signal class is similar to boost::signal, but with a few
			important differences. First, it doesn't implement nearly as
			much functionality as boost's. It merely implements those 
			portions that Roblox uses.
			The big advantage to this implementation is its (limited) thread
			safety. Any thread is allowed to connect new slots to this
			signal and also disconnect them at any time. The firing of a signal 
			is not thread safe - only one thread is allowed to fire at a time. 
		*/

		// Set this to whatever you want to handle exceptions thrown by a slot
		extern boost::function<void(std::exception&)> slot_exception_handler;

		class connection
		{
		public:
			class islot 
				: boost::noncopyable
#if 0
				// No need to test maxStrong, since all strong references are internal
				// However, weak references are external via connection object. It is
				// expected that the total weak references to a slot are much less than 64000
				// NOTE: unsigned short is slightly slower than int on Win32. However, it
				//       saves 4 bytes.
				, public rbx::intrusive_ptr_target<islot, unsigned short, 0, 0>
#else
				, public rbx::intrusive_ptr_target<islot>
#endif
			{
			protected:
				islot()
				{}					
			public:
				virtual ~islot() {}
				virtual void disconnect() = 0;
				virtual bool connected() const = 0;
			};

			inline connection(islot* slot):weak_slot(slot) {}
			inline connection(const connection& con):weak_slot(con.weak_slot) {}
			inline connection() {}
			connection& operator= (const connection& con);

			void disconnect() const;
			bool connected() const;
			bool operator== (const connection& other) const;
			bool operator!= (const connection& other) const;

			void flogPrint()
			{
				boost::intrusive_ptr<islot> s(weak_slot.lock());
				FASTLOG2(FLog::Always, "Connection %p, slot %p", this, s.get());
			}

		private:
			// to make connections copyable, the data for a connection are shared
			rbx::intrusive_weak_ptr<islot> weak_slot;	// must be weak to avoid memory leaks
		};

		class scoped_connection : boost::noncopyable
		{
			// Has-a instead of Is-a. We do this because demoting scoped_connection reference
			// to a connection will alter the meaning of the = operator, leading to strange
			// bugs.
			connection con;
		
		public:
			inline scoped_connection() {}
			inline scoped_connection(const connection& con):con(con) {}

			inline scoped_connection& operator= (const connection& con)
			{
				if (this->con != con)
				{
					disconnect();
					this->con = con;
				}
				return *this;
			}
			inline ~scoped_connection() { disconnect(); }

			// Accessor to underlying connection, if you really want it
			inline connection& get() { return con; }

			// implementation of connection contract
			inline void disconnect() const { 
				con.disconnect(); }
			inline bool connected() const { return con.connected(); }
			inline bool operator== (const connection& other) const { return con == other; }
			inline bool operator!= (const connection& other) const { return con != other; }
		};

		class scoped_connection_logged : boost::noncopyable
		{
			// Has-a instead of Is-a. We do this because demoting scoped_connection reference
			// to a connection will alter the meaning of the = operator, leading to strange
			// bugs.
			connection con;
			bool logged;

		public:
			inline scoped_connection_logged() : logged(false) {}
			inline scoped_connection_logged(bool logged) : logged(logged) {}

			// Helper for using FastLog groups as trigger
			inline scoped_connection_logged(FLog::Channel channelId) : logged(channelId != 0) {}

			inline scoped_connection_logged(const connection& con):con(con) {}

			inline scoped_connection_logged& operator= (const connection& con)
			{
				if (this->con != con)
				{
					disconnect();
					this->con = con;
					if(logged)
					{
						FASTLOG2(FLog::Always, "Scoped connection %p assign: %p", this, &con);
					}
				}
				return *this;
			}
			inline ~scoped_connection_logged() { 
				if(logged)
					FASTLOG1(FLog::Always, "Scoped connection %p destructor", this);
				disconnect(); }

			// Accessor to underlying connection, if you really want it
			inline connection& get() { return con; }

			inline void setLogged(bool logged) { this->logged = logged; }

			// implementation of connection contract
			inline void disconnect() const { 
				if(logged)
					FASTLOG2(FLog::Always, "Scoped connection %p disconnect, previously connected: %u", this, con.connected());
				con.disconnect(); 
			}
			inline bool connected() const { return con.connected(); }
			inline bool operator== (const connection& other) const { return con == other; }
			inline bool operator!= (const connection& other) const { return con != other; }
		};
	
		template<typename Signature>
		class signal : boost::noncopyable
		{
		protected:
			friend class slot;
			class slot : 
				public connection::islot
				, public icallable<boost::function_traits<Signature>::arity, Signature>
			{
			public:
				boost::intrusive_ptr<slot> next;
				signal *sig;

				inline slot(signal *sig)
					:sig(sig)
				{
				}

				virtual bool connected() const
				{
					return sig != NULL;
				}
			public:
				SAFE_HEAP_STATIC(boost::mutex, mutex);

				virtual void disconnect()
				{
					if (!sig)
						return;

					boost::mutex::scoped_lock lock(mutex());

					if (sig)
					{
						signal *s = sig;
						sig = NULL;
						s->remove(this);
					}
				}
			};

			template<class Delegate>
			class callable_slot : public callable<slot, Delegate, boost::function_traits<Signature>::arity, Signature>
			{
			public:
				inline callable_slot(const Delegate& deleg, signal *sig)
					:callable<slot, Delegate, boost::function_traits<Signature>::arity, Signature>(deleg, sig)
				{
				}
			};

		private:
			// The slots are stored in a linked list, with "head" as a dummy slot used to anchor the list.
			boost::intrusive_ptr<slot> head;
			// TODO: Avoid contention by using one mutex per signal? Or an array of signals?
			// TODO: Is boost::mutex the best choice? Does it start up with a spin?
			// However, at least we have a separate mutex for each signature
			
			// SAFE_HEAP_STATIC is used instead of SAFE_STATIC to work around global variables using signals (like GameSettings)
			// If you use SAFE_STATIC, mutex can be destroyed before other global variables using signals, 
			// so signal destructor will fail on mutex access
			SAFE_HEAP_STATIC(boost::mutex, mutex)
			
			void remove(slot* item)
			{
				// Invariant: the value of item->next does not change

				RBXASSERT(!boost::intrusive_ptr_expired(item));

				if (item == head)
					head = item->next;
				else
				{
					// Find "prev". This is O(n)
					slot* prev = head.get();
					// TODO: Can we just assert that prev!=NULL?
					while (prev && prev->next != item)
						prev = prev->next.get();

					// In theory prev should never be NULL, because for it to be NULL
					// the slot would be destroyed, in which case remove() can't be
					// called. Let's play it safe and null-check anyway.
					RBX_SIGNALS_ASSERT(!prev || prev->next.get() == item);

					if (prev)
						prev->next = item->next;
				}

				RBXASSERT(!boost::intrusive_ptr_expired(item));
				// item is now deletable
			}

			void insert(slot* item)
			{
				RBX_SIGNALS_ASSERT(item);

				boost::mutex::scoped_lock lock(mutex());

				if (!head)
				{
					head = item;
				}
				else
				{
					item->next = head;
					head = item;
				}
			}

		public:
			inline signal()
			{
				mutex();
			}

			inline ~signal()
			{
				disconnectAll();
			}

			void disconnectAll()
			{
				while (head)
				{
					boost::intrusive_ptr<slot> node;

					{
						boost::mutex::scoped_lock lock(mutex());

						// See DE131 for a justification of this "chunk" code
						const int chunkSize = 10;
						int count = chunkSize;
						for (node = head; node; node = node->next)
						{
							node->sig = NULL;
							if (count-- == 0)
							{
								// After 10 iterations we need to break out and collect
								// the slots. Otherwise we risk a stack crash
								break;
							}
						}
					}

					// the next line will cause nodes to be destroyed.
					// Notice that we want them to be destroyed
					// outside of the mutex lock because
					// destruction could have side-effects.
					head = node;
				}
			}

			inline bool empty() const
			{ 
				return !head; 
			}

			template<class Delegate>
			connection connect(const Delegate& function)
			{
				slot* item = new callable_slot<Delegate>(function, this);
				insert(item);
				return connection(item);
			}

			// For debugging:
			static size_t sizeof_slot()
			{
				return sizeof(slot);
			}

			void flogPrint()
			{
				FASTLOG1(FLog::Always, "Signal - %p", this);
				boost::intrusive_ptr<typename rbx::signals::signal<Signature>::slot> item;
				while (this->next(item))
					FASTLOG1(FLog::Always, "Signal slot = %p", item.get());
			}

		protected:
			void on_error(std::exception& e)
			{
				if (slot_exception_handler)
					slot_exception_handler(e);
			}
			bool next(boost::intrusive_ptr<slot>& item)
			{
				if (!item)
				{
					// Start iterating; this is safe to read from
					// If another thread is in the process of prepending, we can get old head or new head
					// If we do get the new head it should already have the new next so this is race-free
					item = this->head;
				}
				else
				{
					// Advance the iterator; we keep item alive so next is safe to read from
					// If another thread is in the process of removing the 'item->next' connection we may see
					// the next pointer either pointing to the element that's being removed or to the next one
					// Since replacing next is atomic and we can't observe any other values than these two this is
					// also race-free.
					item = item->next;
				}

				if (!item)
				{
					// Done iterating
					return false;
				}
				else
				{
					// Iteration succeeded
					return true;
				}
			}

		};
		

		template<int arity, typename Signature>
		class signal_with_args;

		template<typename Signature>
		class signal_with_args<0, Signature> : public signal<Signature>
		{
			static inline void fireItem(typename signal<Signature>::slot* item)
			{
				if (item->sig)	// Make sure this guy hasn't been disconnected
					item->call();
			}
		public:
			void operator()()
			{
				if (this->empty()) return;
				
                 typedef typename rbx::signals::signal<Signature>::slot slot;
				boost::intrusive_ptr<slot> item;
            begin:
				try
				{
					while (this->next(item))
						fireItem(item.get());
				}
				catch (RBX::base_exception& e)
				{
					rbx::signals::signal<Signature>::on_error(e);
					// Note: We put this handler on the outside of the for loop
					//       as an optimization. This is why we have a goto statement.
					goto begin;
				}
			}
		};

		template<typename Signature>
		class signal_with_args<1, Signature> : public signal<Signature>
		{
			static inline void fireItem( typename signal<Signature>::slot* item, typename boost::function_traits<Signature>::arg1_type arg1)
			{
				if (item->sig)	// Make sure this guy hasn't been disconnected
					item->call(arg1);
			}
		public:
			void operator()(typename boost::function_traits<Signature>::arg1_type arg1)
			{
				if (this->empty()) return;

				boost::intrusive_ptr<typename rbx::signals::signal<Signature>::slot> item;
begin:
				try
				{
					while (this->next(item))
						fireItem(item.get(), arg1);
				}
				catch (RBX::base_exception& e)
				{
					rbx::signals::signal<Signature>::on_error(e);
					// Note: We put this handler on the outside of the for loop
					//       as an optimization. This is why we have a goto statement.
					goto begin;
				}
			}
		};

		template<typename Signature>
		class signal_with_args<2, Signature> : public signal<Signature>
		{
			static inline void fireItem( typename signal<Signature>::slot* item, typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2)
			{
				if (item->sig)	// Make sure this guy hasn't been disconnected
					item->call(arg1, arg2);
			}
		public:
			void operator ()(typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2)
			{
				if (this->empty()) return;

				boost::intrusive_ptr<typename rbx::signals::signal<Signature>::slot> item;
begin:
				try
				{
					while (this->next(item))
						fireItem(item.get(), arg1, arg2);
				}
				catch (RBX::base_exception& e)
				{
					rbx::signals::signal<Signature>::on_error(e);
					// Note: We put this handler on the outside of the for loop
					//       as an optimization. This is why we have a goto statement.
					goto begin;
				}
			}
		};

		template<typename Signature>
		class signal_with_args<3, Signature> : public signal<Signature>
		{
			static inline void fireItem( typename signal<Signature>::slot* item, typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3)
			{
				if (item->sig)	// Make sure this guy hasn't been disconnected
					item->call(arg1, arg2, arg3);
			}
		public:
			void operator ()(typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3)
			{
				if (this->empty()) return;

				boost::intrusive_ptr<typename rbx::signals::signal<Signature>::slot> item;

begin:
				try
				{
					while (this->next(item))
						fireItem(item.get(), arg1, arg2, arg3);
				}
				catch (RBX::base_exception& e)
				{
					rbx::signals::signal<Signature>::on_error(e);
					// Note: We put this handler on the outside of the for loop
					//       as an optimization. This is why we have a goto statement.
					goto begin;
				}
			}
		};

		template<typename Signature>
		class signal_with_args<4, Signature> : public signal<Signature>
		{
			static inline void fireItem( typename signal<Signature>::slot* item, typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3, typename boost::function_traits<Signature>::arg4_type arg4)
			{
				if (item->sig)	// Make sure this guy hasn't been disconnected
					item->call(arg1, arg2, arg3, arg4);
			}
		public:
			void operator ()(typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3, typename boost::function_traits<Signature>::arg4_type arg4)
			{
				if (this->empty()) return;

				boost::intrusive_ptr<typename rbx::signals::signal<Signature>::slot> item;
begin:
				try
				{
					while (this->next(item))
						fireItem(item.get(), arg1, arg2, arg3, arg4);
				}
				catch (RBX::base_exception& e)
				{
					rbx::signals::signal<Signature>::on_error(e);
					// Note: We put this handler on the outside of the for loop
					//       as an optimization. This is why we have a goto statement.
					goto begin;
				}
			}
		};
        
        template<typename Signature>
		class signal_with_args<5, Signature> : public signal<Signature>
		{
			static inline void fireItem( typename signal<Signature>::slot* item, typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3, typename boost::function_traits<Signature>::arg4_type arg4, typename boost::function_traits<Signature>::arg5_type arg5)
			{
				if (item->sig)	// Make sure this guy hasn't been disconnected
					item->call(arg1, arg2, arg3, arg4, arg5);
			}
		public:
			void operator ()(typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3, typename boost::function_traits<Signature>::arg4_type arg4, typename boost::function_traits<Signature>::arg5_type arg5)
			{
				if (this->empty()) return;
                
				boost::intrusive_ptr<typename rbx::signals::signal<Signature>::slot> item;
            begin:
				try
				{
					while (this->next(item))
						fireItem(item.get(), arg1, arg2, arg3, arg4, arg5);
				}
				catch (RBX::base_exception& e)
				{
					rbx::signals::signal<Signature>::on_error(e);
					// Note: We put this handler on the outside of the for loop
					//       as an optimization. This is why we have a goto statement.
					goto begin;
				}
			}
		};

		template<typename Signature>
		class signal_with_args<6, Signature> : public signal<Signature>
		{
			static inline void fireItem( typename signal<Signature>::slot* item, typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3, typename boost::function_traits<Signature>::arg4_type arg4, typename boost::function_traits<Signature>::arg5_type arg5, typename boost::function_traits<Signature>::arg6_type arg6)
			{
				if (item->sig)	// Make sure this guy hasn't been disconnected
					item->call(arg1, arg2, arg3, arg4, arg5, arg6);
			}
		public:
			void operator ()(typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3, typename boost::function_traits<Signature>::arg4_type arg4, typename boost::function_traits<Signature>::arg5_type arg5, typename boost::function_traits<Signature>::arg6_type arg6)
			{
				if (this->empty()) return;

				boost::intrusive_ptr<typename rbx::signals::signal<Signature>::slot> item;
begin:
				try
				{
					while (this->next(item))
						fireItem(item.get(), arg1, arg2, arg3, arg4, arg5, arg6);
				}
				catch (RBX::base_exception& e)
				{
					rbx::signals::signal<Signature>::on_error(e);
					// Note: We put this handler on the outside of the for loop
					//       as an optimization. This is why we have a goto statement.
					goto begin;
				}
			}
		};

		template<typename Signature>
		class signal_with_args<7, Signature> : public signal<Signature>
		{
			static inline void fireItem( typename signal<Signature>::slot* item, typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3, typename boost::function_traits<Signature>::arg4_type arg4, typename boost::function_traits<Signature>::arg5_type arg5, typename boost::function_traits<Signature>::arg6_type arg6, typename boost::function_traits<Signature>::arg7_type arg7)
			{
				if (item->sig)	// Make sure this guy hasn't been disconnected
					item->call(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
			}
		public:
			void operator ()(typename boost::function_traits<Signature>::arg1_type arg1, typename boost::function_traits<Signature>::arg2_type arg2, typename boost::function_traits<Signature>::arg3_type arg3, typename boost::function_traits<Signature>::arg4_type arg4, typename boost::function_traits<Signature>::arg5_type arg5, typename boost::function_traits<Signature>::arg6_type arg6, typename boost::function_traits<Signature>::arg7_type arg7)
			{
				if (this->empty()) return;

				boost::intrusive_ptr<typename rbx::signals::signal<Signature>::slot> item;
begin:
				try
				{
					while (this->next(item))
						fireItem(item.get(), arg1, arg2, arg3, arg4, arg5, arg6, arg7);
				}
				catch (RBX::base_exception& e)
				{
					rbx::signals::signal<Signature>::on_error(e);
					// Note: We put this handler on the outside of the for loop
					//       as an optimization. This is why we have a goto statement.
					goto begin;
				}
			}
		};
	}

	template<typename Signature>
	class signal : public signals::signal_with_args<boost::function_traits<Signature>::arity, Signature>
	{
	};

	//Note that remote signal is *not* virtualized against signal.  This only works because the Event class is templatized, and not using polymorphism.  
	// If Event becomes polymorphics, THIS CODE WILL FAIL
	template<typename Signature>
	class remote_signal : public signal<Signature>
	{
private:
		typedef signal<Signature> Super;

public:
		signal<void()> connectionSignal;
		
		remote_signal()
		{}

		template<typename F>
		signals::connection connect(const F& function)
		{
			connectionSignal();
			return Super::connect(function);
		}

	};
	
}

#ifdef RBX_SIGNALS_DEBUGGING
#pragma optimize( "", on )
#endif
