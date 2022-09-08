#include <boost/test/unit_test.hpp>

#include <map>
#include <string>
#include <boost/unordered_map.hpp>

#include "rbx/trie.h"

BOOST_AUTO_TEST_SUITE(Trie)

BOOST_AUTO_TEST_CASE(NameConflict)
{
	int v;

	rbx::trie<int, 10> trie;
	trie["ab"] = 1;

	BOOST_CHECK(trie.lookup("ab", v));					  
	BOOST_CHECK_EQUAL(v, 1);

	trie["a"] = 2;

	BOOST_CHECK(trie.lookup("ab", v));					  
	BOOST_CHECK_EQUAL(v, 1);
	BOOST_CHECK(trie.lookup("a", v));					  
	BOOST_CHECK_EQUAL(v, 2);

	trie["ac"] = 3;

	BOOST_CHECK(trie.lookup("ab", v));					  
	BOOST_CHECK_EQUAL(v, 1);
	BOOST_CHECK(trie.lookup("a", v));					  
	BOOST_CHECK_EQUAL(v, 2);
	BOOST_CHECK(trie.lookup("ac", v));					  
	BOOST_CHECK_EQUAL(v, 3);

	trie["acart"] = 4;

	BOOST_CHECK(!trie.lookup("aca", v));					  
	BOOST_CHECK(!trie.lookup("acar", v));					  
	BOOST_CHECK(trie.lookup("acart", v));					  
	BOOST_CHECK_EQUAL(v, 4);

	BOOST_CHECK(trie.lookup("ab", v));					  
	BOOST_CHECK_EQUAL(v, 1);
	BOOST_CHECK(trie.lookup("a", v));					  
	BOOST_CHECK_EQUAL(v, 2);
	BOOST_CHECK(trie.lookup("ac", v));					  
	BOOST_CHECK_EQUAL(v, 3);
}


static std::string newKey()
{
	std::string key;
	for (int k = 0; k < 10; ++k)
	{
		int r = 32 + rand() % 96;
		key += (char) r;
	}
	return key;
}

typedef rbx::trie<int, 30> Trie;

static std::string init(Trie& trie, std::map<std::string, int>& map, boost::unordered_map<std::string, int>& hashmap)
{
	std::string key;
	for (int i = 0; i < 100; ++i)
	{
		key = newKey();
		static int j = 0;
		int value = ++j;

		trie[key.c_str()] = value;
		map[key] = value;
		hashmap[key] = value;
		int v;
		BOOST_REQUIRE(trie.lookup(key.c_str(), v));					  
		BOOST_REQUIRE_EQUAL(v, value);
		BOOST_REQUIRE_EQUAL(map[key], value);
	}
	return key;
}

BOOST_AUTO_TEST_CASE(SimpleTest)
{
	Trie trie;
	std::map<std::string, int> map;
	boost::unordered_map<std::string, int> hashmap;
	std::string key = init(trie, map, hashmap);
}

#ifndef _DEBUG
BOOST_AUTO_TEST_CASE(PerfTrie)
{
	Trie trie;
	std::map<std::string, int> map;
	boost::unordered_map<std::string, int> hashmap;
	std::string key = init(trie, map, hashmap);

	int v;
	for (int i = 0; i < 1000000; ++i)
		trie.lookup(key.c_str(), v);
}
BOOST_AUTO_TEST_CASE(PerfMap)
{
	Trie trie;
	std::map<std::string, int> map;
	boost::unordered_map<std::string, int> hashmap;
	std::string key = init(trie, map, hashmap);

	for (int i = 0; i < 1000000; ++i)
		map.find(key);
}
BOOST_AUTO_TEST_CASE(PerfHashMap)
{
	Trie trie;
	std::map<std::string, int> map;
	boost::unordered_map<std::string, int> hashmap;
	std::string key = init(trie, map, hashmap);

	for (int i = 0; i < 1000000; ++i)
		hashmap.find(key);
}
#endif

BOOST_AUTO_TEST_SUITE_END()

