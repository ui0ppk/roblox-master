
#pragma once

#include <boost/noncopyable.hpp>
#include <boost/pool/object_pool.hpp>

namespace rbx
{
	/// A fast trie, but a little expensive memory-wise
	/// Only supports ascii strings in the range 32-127
	/// operator[] is not thread safe
	template<class V, unsigned int maxDepth>
	class trie : public boost::noncopyable
	{
	public:
		class depth_exceeded_exception : public std::exception
		{
		public:
			virtual const char* what() const throw()
			{
				return "trie depth exceeded";
			};
		};
		class bad_key : public std::exception
		{
		public:
			const char key;
			bad_key(char key):key(key) {}
			virtual const char* what() const throw()
			{
				return "key out of range";
			};
		};

	private:
		class Node;
		typedef boost::object_pool<Node> Pool;
		typedef boost::object_pool<V> ValuePool;
		class Node : public boost::noncopyable
		{
			friend class trie;

			std::string leaf;		// used when there are no branches. Saves a lot of memory

			static const size_t array_size = 128 - 32;
			Node* array[array_size];

			V* value;
			const int depth;
			inline void check_key(char key)
			{
				if (key < 32)
		            throw bad_key(key);
			}
		public:
			static inline unsigned char to_index(char c)
			{
				return (unsigned char)(c - 32);
			}
			static inline bool is_legal_char(char c)
			{
				return c >= 32;
			}
			Node(int depth)
				:value(0)
				,depth(depth)
			{
				if (depth > maxDepth)
					throw depth_exceeded_exception();
				memset(array, 0, sizeof(array));
			}
			void destroy(Pool& pool, ValuePool& valuePool)
			{
				for (size_t i = 0; i<array_size; ++i)
					if (array[i])
						array[i]->destroy(pool, valuePool);

				if (value)
					valuePool.destroy(value);

				pool.destroy(this);
			}
			template<class F>
			void each_value(const F& f) const
			{
				for (size_t i = 0; i<array_size; ++i)
					each_value(f);
				if (value)
					f(*value);
			}

			bool empty_array() const
			{
				for (size_t i = 0; i<array_size; ++i)
					if (array[i])
						return false;
				return true;
			}

			void removeLeaf(Pool& pool, ValuePool& valuePool)
			{
				// We can't use the leaf shortcut. Need to use the array for branches

				// Construct the path for the existing leaf
				const size_t index = to_index(leaf[0]);
				Node* next = array[index] = pool.construct(depth + 1);
				V& v = next->array_subscript(leaf.c_str() + 1, pool, valuePool);

				// Move the value over to its new home
				v = *value;
				valuePool.destroy(value);
				value = NULL;

				leaf = "";
			}

			V& array_subscript(const char* key, Pool& pool, ValuePool& valuePool)
			{
				if (*key == 0)
				{
					if (!leaf.empty())
						removeLeaf(pool, valuePool);
					if (!value)
						value = valuePool.construct();
					return *value;
				}

				check_key(*key);

				// If the array is empty, then set the leaf
				if (empty_array()) {
					if (leaf.empty() && !value) {
						// This is the first entry, so we can use the leaf shortcut
						leaf = key;
						value = valuePool.construct();
						return *value;
					} else if (leaf == key) {
						return *value;
                    } else if (!leaf.empty()) {
						removeLeaf(pool, valuePool);
                    }
                }

				size_t index = to_index(*key);
				Node* next = array[index];
				if (!next)
					array[index] = next = pool.construct(depth + 1);

				return next->array_subscript(key + 1, pool, valuePool);
			}

			size_t compute_size() const
			{
				size_t size = 1;
				for (size_t i = 0; i<array_size; ++i)
					if (array[i])
						size += array[i]->compute_size();
				return size;
			}

		};
		Pool pool;
		ValuePool valuePool;
		Node* root;
	public:
		trie():pool(),root(pool.construct(1)) {}
		~trie()
		{
			root->destroy(pool, valuePool);
		}

		static inline bool equal(const char* s, const char* k)
		{
			// For some reason this is MUCH faster than strcmp
			for (; *s == *k; ++s, ++k)
				if (*s == 0)
					return true;
			return false;
		}

		inline bool lookup(const char* key, V& value) const
		{
			const Node* node = root;
			while (true) 
			{
				// If this node has a value, then see if it matches the key
				if (node->value)
				{
					// Look for a match between the key and our leaf.
					// Note that key and leaf might both be "", which would be a match
					if (equal(node->leaf.c_str(), key))
					{
						value = *node->value;
						return true;
					}
				}

				if (!Node::is_legal_char(*key))	// *key could be 0, meaning the end of the key
					return false;

				// Advance to the next node
				node = node->array[Node::to_index(*key)];
				if (!node)
					return false;
				++key;
			}
		}
		V& operator[](const char* key)
		{
			return root->array_subscript(key, pool, valuePool);
		}
		size_t compute_size()
		{
			return root->compute_size();
		}
		size_t compute_memory_usage()
		{
			return root->compute_size() * sizeof(Node);
		}
		template<class F>
		void each_value(const F& f) const
		{
			root->each_value(f);
		}
	};

}
