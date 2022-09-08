#pragma once

#include "RBX/Debug.h"
#include "boost/noncopyable.hpp"

namespace RBX { namespace Intrusive {

	// A very efficient, constant time, unordered set
	// Features:
	//     Constant-time insert
	//     Constant-time remove
	//     Constant-time membership test
	//     items can remove themselves
	//     Items in the set auto-remove themselves upon destruction
	//     no memory is allocated for operations
	//     everything is nothrow
	//
	// TODO: Adding an item to a set silently removes it for membership in another set. Is this desirable? Should it be a runtime error?
	// TODO: Implement ConstIterator

	template<class Item, class Tag = Item>
	class Set : boost::noncopyable
	{
	private:
		class NextRef
		{
			friend class Set;
		protected:
			Item* next;
			inline NextRef() throw()
				:next(0)
			{
			}
			inline NextRef(const NextRef& other) throw()
				:next(0)
			{
				// Copies of objects aren't automatically added to containers
			}
			inline NextRef& operator=(const NextRef& other) throw()
			{
				// This object retains its membership to its container
			}		
		};
	public:
		class Hook : public NextRef
		{
			friend class Set;
			Set* _container;
			NextRef* prev;
		public:
			inline Hook() throw()
				:_container(0),prev(0)
			{
			}
			inline Hook(const Hook& other) throw()
				:_container(0),prev(0)
			{
				// Copies of objects aren't automatically added to containers
			}
			inline Hook& operator=(const Hook& other) throw()
			{
				// This object retains its membership to its container
			}
			inline ~Hook() throw()
			{
				remove();
			}
			inline void remove() throw()
			{
				if (is_linked())
				{
					RBXASSERT(prev!=0 || Set::NextRef::next!=0);

					if (prev)
						prev->Set::NextRef::next = NextRef::next;
					if (NextRef::next)
						NextRef::next->Set::Hook::prev = prev;

					_container->count--;

					NextRef::next = 0;
					prev = 0;
					_container = 0;
				}
			}
			inline bool is_linked() const throw()
			{
				RBXASSERT((_container != 0) == ((Set::NextRef::next != 0) || (prev != 0)));
				return _container != 0;
			}
			inline Set* container() throw()
			{
				return _container;
			}
		};

		class Iterator
		{
			friend class Set;
			Item* item;
			Iterator(Item* item) throw()
				:item(item) 
			{
				RBXASSERT(!item || item->Set::Hook::is_linked());
			}
		public:
			inline Iterator() throw()
				:item(0) {}

			inline bool operator==(const Iterator& other) const throw()
			{ 
				return item == other.item; 
			}

			inline bool operator!=(const Iterator& other) const throw()
			{ 
				return item != other.item; 
			}

			inline Item* operator->() throw()
			{ 
				RBXASSERT(item);
				RBXASSERT(!item || item->Set::Hook::is_linked());
				return item; 
			}

			inline Item& operator*()  throw()
			{ 
				RBXASSERT(item);
				RBXASSERT(!item || item->Set::Hook::is_linked());
				return *item; 
			}

			inline Iterator& operator++() throw()
			{
				RBXASSERT(item);

				item = item->Set::Hook::next;

				RBXASSERT(!item || item->Set::Hook::is_linked());

				return *this;
			}

			inline bool empty() const throw()
			{
				return item == 0;
			}

			// for std iterators:
			typedef std::forward_iterator_tag iterator_category;
			typedef Item& value_type;
			typedef void difference_type;
			typedef /*typename*/ Item* pointer;
			typedef /*typename*/ Item& reference;
		};
		
		inline Set() throw()
			:count(0)
		{}

		inline ~Set() throw()
		{
			for (Iterator iter = begin(); !iter.empty(); iter = erase(iter))
				;
		}

		inline size_t size() const throw() { return count; }
		inline bool empty() const throw() { return count==0; }

		Iterator erase(Iterator iter) throw()
		{
			Item& item(*iter);
			++iter;
			remove_element(item);
			return iter;
		}

		bool remove_element(Item& item) throw()
		{
			if (item.Set::Hook::_container == this)
			{
				item.Set::Hook::remove();
				return true;
			}
			else
				return false;
		}

		void insert(Item& item) throw()
		{
			if (item.Set::Hook::_container == this)
				return;

			// Items can be in only one list at a time
			item.Set::Hook::remove();

			RBXASSERT(!item.Set::Hook::next);
			RBXASSERT(!item.Set::Hook::prev);

			Item* head = head_ref.next;
			if (head)
			{
				RBXASSERT(head->Set::Hook::is_linked());
				RBXASSERT(head->Set::Hook::container() == this);
				item.Set::NextRef::next = head;
				head->Set::Hook::prev = &item;
			}
			head_ref.next = &item;
			item.Set::Hook::prev = &head_ref;

			item.Set::Hook::_container = this;
			RBXASSERT(item.Set::Hook::next || item.Set::Hook::prev);

			count++;
		}

		inline Iterator begin() throw()
		{
			return Iterator(head_ref.next);
		}

		inline Iterator end() throw()
		{
			return Iterator();
		}

		// For the std iterator pattern:
		typedef Iterator iterator;

		// For the boost::intrusive pattern:
		inline void push_front(Item& item) throw()
		{
			insert(item);
		}

		// For the boost::intrusive pattern:
		inline Iterator iterator_to(Item& item) throw()
		{
			return item.Set::Hook::_container == this ? Iterator(&item) : Iterator();
		}

	private:
		size_t count;
		NextRef head_ref;
	};


}}