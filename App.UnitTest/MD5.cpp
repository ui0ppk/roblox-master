/*
 *  MD5.cpp - boost unit test for RBX::MD5
 *
 *  Copyright 2010 ROBLOX, INC. All rights reserved.
 *
 */

#include <boost/test/unit_test.hpp>
#include <boost/scoped_ptr.hpp>

#include "util/MD5hasher.h"

static const char *ZERO_LENGTH_HASH = "d41d8cd98f00b204e9800998ecf8427e";
static const char *QBF_HASH         = "bfb85e401a205cde01d17164bd3de689";

BOOST_AUTO_TEST_SUITE(MD5Suite)

BOOST_AUTO_TEST_CASE(MD5_CreateThenDestroy)
{
	{
		boost::scoped_ptr<RBX::MD5Hasher> hasher(RBX::MD5Hasher::create());
		
		const char *result = hasher->c_str();
		
		BOOST_CHECK_EQUAL(result, ZERO_LENGTH_HASH);
	}
	
}

BOOST_AUTO_TEST_CASE(MD5_HashSimpleString)
{
	{
		boost::scoped_ptr<RBX::MD5Hasher> hasher(RBX::MD5Hasher::create());
		
		hasher->addData("The quick brown fox jumps over the lazy dog. 1234567890");
		
		const char *result = hasher->c_str();
		
		// do it twice, first time should create
		BOOST_CHECK_EQUAL(result, QBF_HASH);
		
		result = hasher->c_str();
		// second time uses cached value
		BOOST_CHECK_EQUAL(result, QBF_HASH);
	}
}

BOOST_AUTO_TEST_CASE(MD5_HashMultipleSimpleString)
{
	{
		boost::scoped_ptr<RBX::MD5Hasher> hasher(RBX::MD5Hasher::create());
		
		hasher->addData("The quick brown fox");
		hasher->addData(" jumps over the lazy dog.");
		hasher->addData(" 1234567890");
		
		const char *result = hasher->c_str();
		
		// do it twice, first time should create
		BOOST_CHECK_EQUAL(result, QBF_HASH);
		
		result = hasher->c_str();
		// second time uses cached value
		BOOST_CHECK_EQUAL(result, QBF_HASH);
	}
}

BOOST_AUTO_TEST_CASE(MD5_IStream)
{
	{
		boost::scoped_ptr<RBX::MD5Hasher> hasher(RBX::MD5Hasher::create());
		
		std::stringstream ss;
		
		ss << "The quick brown fox";
		ss << " jumps over the lazy dog.";
		ss << " 1234567890";
		hasher->addData(ss);
		
		const char *result = hasher->c_str();
		
		// do it twice, first time should create
		BOOST_CHECK_EQUAL(result, QBF_HASH);
		
		result = hasher->c_str();
		// second time uses cached value
		BOOST_CHECK_EQUAL(result, QBF_HASH);
	}
}

BOOST_AUTO_TEST_SUITE_END()
