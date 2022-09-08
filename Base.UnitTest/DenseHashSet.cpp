#include "rbx/DenseHash.h"

#include <boost/test/unit_test.hpp>

using namespace boost;

BOOST_AUTO_TEST_SUITE(DenseHashSet)

template <typename Set> void verifyContentsRange(const Set& set, int begin, int end)
{
	BOOST_CHECK_EQUAL(set.size(), end - begin);

	std::vector<int> data;
	
	for (typename Set::const_iterator it = set.begin(); it != set.end(); ++it)
		data.push_back(*it);
		
	BOOST_CHECK_EQUAL(data.size(), end - begin);

	std::sort(data.begin(), data.end());
	
	for (size_t i = 0; i < data.size(); ++i)
		BOOST_CHECK_EQUAL(data[i], begin + i);
}

BOOST_AUTO_TEST_CASE(SimpleForward)
{
	RBX::DenseHashSet<int> set(-1);
	
	for (int i = 0; i < 5; ++i)
		set.insert(i);
		
	verifyContentsRange(set, 0, 5);
}

BOOST_AUTO_TEST_CASE(SimpleBackward)
{
	RBX::DenseHashSet<int> set(-1);
	
	for (int i = 4; i >= 0; --i)
		set.insert(i);
		
	verifyContentsRange(set, 0, 5);
}

BOOST_AUTO_TEST_CASE(SimpleDuplicate)
{
	RBX::DenseHashSet<int> set(-1);
	
	for (int i = 0; i < 5; ++i)
		set.insert(i);

	for (int i = 4; i >= 0; --i)
		set.insert(i);
		
	verifyContentsRange(set, 0, 5);
}

struct HasherOne
{
	int operator()(int value) const
	{
		return 1;
	}
};

BOOST_AUTO_TEST_CASE(LargeBadHashFunction)
{
	RBX::DenseHashSet<int, HasherOne> set(-1);
	
	for (int i = 0; i < 512; ++i)
		set.insert(i);
		
	verifyContentsRange(set, 0, 512);
}

BOOST_AUTO_TEST_CASE(ContainsSimple)
{
	RBX::DenseHashSet<int> set(0);
	
	BOOST_CHECK_EQUAL(set.contains(1), false);	
	BOOST_CHECK_EQUAL(set.contains(2), false);
	
	set.insert(1);
	
	BOOST_CHECK_EQUAL(set.contains(1), true);
	BOOST_CHECK_EQUAL(set.contains(2), false);
}

BOOST_AUTO_TEST_CASE(ContainsSieve)
{
	RBX::DenseHashSet<int> sieve(0);
	
	for (int i = 2; i < 100; ++i)
	{
		if (!sieve.contains(i))
		{
			for (int k = 2; k * i < 100; ++k)
				sieve.insert(k * i);
		}
	}
	
	BOOST_CHECK_EQUAL(sieve.contains(81), true);
	BOOST_CHECK_EQUAL(sieve.contains(97), false);
	BOOST_CHECK_EQUAL(sieve.contains(98), true);
	BOOST_CHECK_EQUAL(sieve.contains(99), true);
}

BOOST_AUTO_TEST_CASE(SizeEmpty)
{
	RBX::DenseHashSet<int> set(0);
	
	BOOST_CHECK_EQUAL(set.size(), 0);
	BOOST_CHECK_EQUAL(set.empty(), true);
	
	set.insert(5);
	
	BOOST_CHECK_EQUAL(set.size(), 1);
	BOOST_CHECK_EQUAL(set.empty(), false);

	BOOST_CHECK_EQUAL(*set.begin(), 5);
}

BOOST_AUTO_TEST_CASE(SizeBuckets)
{
	RBX::DenseHashSet<int> set(-1);
	
	BOOST_CHECK_EQUAL(set.size(), 0);
	BOOST_CHECK_EQUAL(set.bucket_count(), 0);

	for (int i = 0; i < 100; ++i)
		set.insert(i*i);
	
	for (int i = 99; i >= 0; --i)
		set.insert(i*i);
		
	BOOST_CHECK_EQUAL(set.size(), 100);
	BOOST_CHECK_EQUAL(set.bucket_count(), 256);
}

BOOST_AUTO_TEST_CASE(BucketsStartingAt1)
{
	RBX::DenseHashSet<int> set(-1, 1);
	
	BOOST_CHECK_EQUAL(set.size(), 0);
	BOOST_CHECK_EQUAL(set.bucket_count(), 1);
	
	set.insert(1);
	
	BOOST_CHECK_EQUAL(set.size(), 1);
	BOOST_CHECK_EQUAL(set.bucket_count(), 2);

	set.insert(4);
	
	BOOST_CHECK_EQUAL(set.size(), 2);
	BOOST_CHECK_EQUAL(set.bucket_count(), 4);

	set.insert(9);
	
	BOOST_CHECK_EQUAL(set.size(), 3);
	BOOST_CHECK_EQUAL(set.bucket_count(), 4);

	set.insert(16);
	
	BOOST_CHECK_EQUAL(set.size(), 4);
	BOOST_CHECK_EQUAL(set.bucket_count(), 8);

	for (int i = 0; i < 100; ++i)
		set.insert(i * i);
	
	BOOST_CHECK_EQUAL(set.size(), 100);
	BOOST_CHECK_EQUAL(set.bucket_count(), 256);
}

BOOST_AUTO_TEST_CASE(IteratorOps)
{
	RBX::DenseHashSet<int> set(-1);
	
	set.insert(1);
	set.insert(2);
	
	RBX::DenseHashSet<int>::const_iterator it1 = set.begin();
	
	RBX::DenseHashSet<int>::const_iterator it2 = it1;
	RBX::DenseHashSet<int>::const_iterator it3 = ++it2;
	
	RBX::DenseHashSet<int>::const_iterator it4 = it1;
	RBX::DenseHashSet<int>::const_iterator it5 = it4++;
	
	BOOST_CHECK_EQUAL(it1 == set.begin(), true);
	BOOST_CHECK_EQUAL(it1 != set.begin(), false);
	BOOST_CHECK_EQUAL(it1 == set.end(), false);
	BOOST_CHECK_EQUAL(it1 != set.end(), true);
	
	BOOST_CHECK(it2 == it4);
	BOOST_CHECK(it2 != it1);
	BOOST_CHECK(it2 != set.end());
	
	BOOST_CHECK(it3 == it2);
	BOOST_CHECK(it5 == it1);
	
	BOOST_CHECK(it1 == it5);
	
	BOOST_CHECK((*it1 == 1 && *it2 == 2) || (*it1 == 2 && *it1 == 1));
}

BOOST_AUTO_TEST_SUITE_END()
