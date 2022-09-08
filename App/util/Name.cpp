#include "stdafx.h"

#include "util/name.h"

#include "rbx/threadsafe.h"

#include "rbx/Debug.h"

#include <boost/thread/once.hpp>
#include <boost/unordered_map.hpp>

using namespace RBX;


namespace { 

struct StrHash : std::unary_function<const char*, std::size_t>
{
	size_t operator()(const char*s) const
	{
		size_t h = boost::hash_range(s, s + strlen(s) );
		return h;
	}
};

struct StrEqualTo : public std::binary_function<const char*, const char*, bool>
{
	bool operator()(const char* a, const char* b) const
	{
		return strcmp(a, b) == 0;
	}
};

} // namespace

class Name::NameMap : public boost::unordered_map<const char *, Name*, StrHash, StrEqualTo>
{
public:
	~NameMap();
};

Name::Name(const char* const& name)
	:str(name)
{
	setOrderIndex();
}

Name::NameMap::~NameMap()
{
	NameMap::iterator iter = begin();
	while (iter!=end())
	{
		delete iter->second;
		++iter;
	}
}

//////////////////////////////////////////
// Cumbersome code to ensure that the mutex is initalized 
// in a thread-safe manner
static RBX::mutex& moo2()
{
	static RBX::mutex mutex2;
	return mutex2;
}
static void initMoo()
{
	moo2();
}
static boost::once_flag mooFlag = BOOST_ONCE_INIT;
RBX::mutex& Name::mutex()
{
	boost::call_once(&initMoo, mooFlag);
	return moo2();
}
//
////////////////////////////////////////////

Name::NameMap& Name::map()
{
	static NameMap n;
	return n;
}

size_t Name::approximateMemoryUsage()
{
	// is this reasonable?
	return 64 * map().size();
}
size_t Name::size()
{
	return map().size();
}

static const Name* nullName;

static void declareNullName()
{
	nullName = &Name::declare("");
}

const Name& Name::getNullName() {
	static boost::once_flag flag2 = BOOST_ONCE_INIT;
	boost::call_once(&declareNullName, flag2);
	return *nullName;
}

void Name::setOrderIndex()
{
	static std::vector<Name*> ordered;

	// Go from highest to lowest to keep sort order index comparisons invariant
	size_t i = ordered.size();
	for (; i > 0; --i)
	{
		RBXASSERT(ordered[i-1]->str != str);
		if (str > ordered[i-1]->str)
			break;

		ordered[i-1]->sortIndex++;
		RBXASSERT(ordered[i-1]->sortIndex == i);
	}
	this->sortIndex = i;
	ordered.insert(ordered.begin() + i, this);

#ifdef _DEBUG
	for (size_t j = 0; j < ordered.size(); ++j)
		RBXASSERT(ordered[j]->sortIndex == j);
#endif
}

const Name& Name::declare(const char* const& sName) {

	if (sName==NULL)
	{
		return getNullName();
	}

	Name* result;
	{
		// This scope needs to be closed before calling getNullName or the 
		// mutex will be recursively locked.

		RBX::mutex::scoped_lock lock(mutex());
        const Name& (*thisFunction)(const char* const &) = &RBX::Name::declare; // required due to overload.
        checkRbxCaller<kCallCheckCallersCode, callCheckSetApiFlag<kNameApiOffset> >(reinterpret_cast<void*>(thisFunction));

		NameMap::iterator iter = map().find(sName);
		if (iter!=map().end())
			return *iter->second;

		Name* name = new Name(sName);
		map()[name->c_str()] = name;

		result = name;
	}

	RBXASSERT(sName[0]=='\0' || (!result->str.empty() && lookup(sName) != Name::getNullName()) );
	return *result;
}

const Name& Name::lookup(const char* const& sName) {
	if (sName==NULL)  
		return getNullName();

	const Name* result = NULL;
	{
		RBX::mutex::scoped_lock lock(mutex());
        const Name& (*thisFunction)(const char* const &) = &RBX::Name::lookup; // required due to overload.
        checkRbxCaller<kCallCheckCallersCode, callCheckSetApiFlag<kNameApiOffset> >(
            reinterpret_cast<void*>(thisFunction));

		NameMap::iterator iter = map().find(sName);
		if (iter!=map().end())
			result = iter->second;
	}

	// Avoid recursively locking mutex().
	return result != NULL ? *result : Name::getNullName();
}

std::ostream& RBX::operator<<( std::ostream& os, const Name& name )
{
	return os << name.c_str();
}
