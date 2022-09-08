#pragma once

#include <boost/functional/hash.hpp>
#include <vector>

#include "rbx/Debug.h"

namespace RBX
{
    // Internal implementation of DenseHashSet and DenseHashMap
    namespace detail
    {
        template <typename Key> struct DenseHashSetItem
        {
            Key key;

            DenseHashSetItem(const Key& key): key(key)
            {
            }
        };

        template <typename Key, typename Value> struct DenseHashMapItem
        {
            Key key;
            Value value;

            DenseHashMapItem(const Key& key): key(key), value()
            {
            }
        };

        template <typename Key, typename Item, typename Hash, typename Eq> class DenseHashTable
        {
        public:
            class const_iterator;

            DenseHashTable(const Key& empty_key, size_t buckets = 0): data(buckets, Item(empty_key)), count(0), empty_key(empty_key)
            {
                // buckets has to be power-of-two or zero
                RBXASSERT((buckets & (buckets - 1)) == 0);
            }

            void clear()
            {
                data.clear();
                count = 0;
            }

            Item* insert(const Key& key)
            {
                // It is invalid to insert empty_key into the table since it acts as a "entry does not exist" marker
                RBXASSERT(!eq(key, empty_key));

                if (count >= data.size() * 3 / 4)
                {
                    rehash();
                }

                size_t hashmod = data.size() - 1;
                size_t bucket = hasher(key) & hashmod;

                for (size_t probe = 0; probe <= hashmod; ++probe)
                {
                    Item& probe_item = data[bucket];

                    // Element does not exist, insert here
                    if (eq(probe_item.key, empty_key))
                    {
                        probe_item.key = key;
                        count++;
                        return &probe_item;
                    }

                    // Element already exists
                    if (eq(probe_item.key, key))
                    {
                        return &probe_item;
                    }

                    // Hash collision, quadratic probing
                    bucket = (bucket + probe + 1) & hashmod;
                }

                // Hash table is full - this should not happen
                RBXASSERT(false);
                return NULL;
            }

            const Item* find(const Key& key) const
            {
                if (data.empty()) return 0;
                if (eq(key, empty_key)) return 0;

                size_t hashmod = data.size() - 1;
                size_t bucket = hasher(key) & hashmod;

                for (size_t probe = 0; probe <= hashmod; ++probe)
                {
                    const Item& probe_item = data[bucket];

                    // Element exists
                    if (eq(probe_item.key, key))
                        return &probe_item;

                    // Element does not exist
                    if (eq(probe_item.key, empty_key))
                        return NULL;

                    // Hash collision, quadratic probing
                    bucket = (bucket + probe + 1) & hashmod;
                }

                // Hash table is full - this should not happen
                RBXASSERT(false);
                return NULL;
            }

            const_iterator begin() const
            {
                size_t start = 0;
                
                while (start < data.size() && eq(data[start].key, empty_key))
                    start++;
                    
                return const_iterator(this, start);
            }

            const_iterator end() const
            {
                return const_iterator(this, data.size());
            }

            size_t size() const
            {
                return count;
            }

            size_t bucket_count() const
            {
                return data.size();
            }

            class const_iterator
            {
            public:
                const_iterator(): set(0), index(0)
                {
                }

                const_iterator(const DenseHashTable<Key, Item, Hash, Eq>* set, size_t index): set(set), index(index)
                {
                }

                const Item& getItem() const
                {
                    return set->data[index];
                }
                
                const Key& operator*() const
                {
                    return set->data[index].key;
                }

                const Key* operator->() const
                {
                    return &set->data[index].key;
                }

                bool operator==(const const_iterator& other) const
                {
                    return set == other.set && index == other.index;
                }

                bool operator!=(const const_iterator& other) const
                {
                    return set != other.set || index != other.index;
                }

                const_iterator& operator++()
                {
                    size_t size = set->data.size();

                    do
                    {
                        index++;
                    }
                    while (index < size && set->eq(set->data[index].key, set->empty_key));
                    
                    return *this;
                }

                const_iterator operator++(int)
                {
                    const_iterator res = *this;
                    ++*this;
                    return res;
                }
            
            private:
                const DenseHashTable<Key, Item, Hash, Eq>* set;
                size_t index;
            };

        private:
            std::vector<Item> data;
            size_t count;
            Key empty_key;
            Hash hasher;
            Eq eq;

            void rehash()
            {
                size_t newsize = data.empty() ? 16 : data.size() * 2;
                DenseHashTable newtable(empty_key, newsize);

                for (size_t i = 0; i < data.size(); ++i)
                    if (!eq(data[i].key, empty_key))
                        *newtable.insert(data[i].key) = data[i];

                RBXASSERT(count == newtable.count);
                data.swap(newtable.data);
            }
        };
    }

    // This is a faster alternative of boost::unordered_set, but it does not implement the same interface (i.e. it does not support erasing and has contains() instead of find())
	template <typename Key, typename Hash = boost::hash<Key>, typename Eq = std::equal_to<Key> > class DenseHashSet
	{
        typedef detail::DenseHashTable<Key, detail::DenseHashSetItem<Key>, Hash, Eq> Impl;
        Impl impl;

	public:
        typedef typename Impl::const_iterator const_iterator;

		DenseHashSet(const Key& empty_key, size_t buckets = 0): impl(empty_key, buckets)
		{
		}

        void clear()
		{
            impl.clear();
		}

        void insert(const Key& key)
		{
            impl.insert(key);
		}

        bool contains(const Key& key) const
		{
            return impl.find(key) != 0;
		}

        size_t size() const
		{
            return impl.size();
		}

        bool empty() const
		{
            return impl.size() == 0;
		}

        size_t bucket_count() const
		{
            return impl.bucket_count();
		}

        const_iterator begin() const
		{
            return impl.begin();
		}

        const_iterator end() const
		{
            return impl.end();
		}
	};

    // This is a faster alternative of boost::unordered_map, but it does not implement the same interface (i.e. it does not support erasing and has contains() instead of find())
	template <typename Key, typename Value, typename Hash = boost::hash<Key>, typename Eq = std::equal_to<Key> > class DenseHashMap
	{
        typedef detail::DenseHashTable<Key, detail::DenseHashMapItem<Key, Value>, Hash, Eq> Impl;
		Impl impl;

	public:
        typedef typename Impl::const_iterator const_iterator;

		DenseHashMap(const Key& empty_key, size_t buckets = 0): impl(empty_key, buckets)
		{
		}

        void clear()
		{
            impl.clear();
		}

        // Note: this reference is invalidated by any insert operation (i.e. operator[])
		Value& operator[](const Key& key)
		{
			return impl.insert(key)->value;
		}

        // Note: this pointer is invalidated by any insert operation (i.e. operator[])
		const Value* find(const Key& key) const
		{
			const detail::DenseHashMapItem<Key, Value>* result = impl.find(key);

            return result ? &result->value : NULL;
		}

        // Note: this pointer is invalidated by any insert operation (i.e. operator[])
		Value* find(const Key& key)
		{
			const detail::DenseHashMapItem<Key, Value>* result = impl.find(key);

            return result ? const_cast<Value*>(&result->value) : NULL;
		}

        bool contains(const Key& key) const
		{
            return impl.find(key) != 0;
		}

        size_t size() const
		{
            return impl.size();
		}

        bool empty() const
		{
            return impl.size() == 0;
		}

        size_t bucket_count() const
		{
            return impl.bucket_count();
		}

        const_iterator begin() const
		{
            return impl.begin();
		}

        const_iterator end() const
		{
            return impl.end();
		}
	};
}
