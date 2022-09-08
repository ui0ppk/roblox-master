#include "stdafx.h"

#include "Util/Guid.h"
#include "rbx/atomic.h"

#ifdef _WIN32
#include "objbase.h"
#elif defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#include <boost/algorithm/string/case_conv.hpp>
#else
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
static boost::uuids::basic_random_generator<boost::mt19937> gen;
#endif

namespace RBX {
	template<>
	bool StringConverter<RBX::Guid::Data>::convertToValue(const std::string& text, RBX::Guid::Data& value)
	{
		RBXASSERT(false); // not fully implemented
		return false;
	}
	namespace Reflection
	{
		template<>
		RBX::Guid::Data& Variant::convert<RBX::Guid::Data>(void)
		{
			return genericConvert<RBX::Guid::Data>();
		}

		template<>
		const Type& Type::getSingleton<Guid::Data>()
		{
			static TType<Guid::Data> type("GuidData");
			return type;
		}
	}
}

static rbx::atomic<int> nextIndex = 0;
static RBX::Guid::Scope* localScope;
RBX::Guid::Scope RBX::Guid::Scope::nullScope;

const RBX::Guid::Scope& RBX::Guid::Scope::null()
{
	return nullScope;
}

static void initLocalScope()
{
    RBX::Guid::Scope scope;
	RBX::Guid::generateRBXGUID(scope);

    // Note: localScope has to be a pointer to avoid initialization order fiasco between localScope and getLocalScope() callers
    // We never free this memory because we don't really need to and that avoids a symmetrical problem during deinitialization
    // (see SAFE_HEAP_STATIC in rbx/threadsafe.h)
    localScope = new RBX::Guid::Scope(scope);
}

const RBX::Guid::Scope& RBX::Guid::getLocalScope()
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(&initLocalScope, flag);
	return *localScope;
}

RBX::Guid::Guid()
{
	data.scope = getLocalScope();
	data.index = ++nextIndex;
}

void RBX::Guid::generateStandardGUID(std::string& result)
{
	// Creates a string like this: {c200e360-38c5-11ce-ae62-08002b2b79ef}
#ifdef _WIN32
	::GUID guid;
	CoCreateGuid(&guid);

	// This string will hold a string-ized GUID.  It needs
	// to be 38 characters, plus the terminator
	// character.  But just to be safe we'll make
	// it a little bigger.
	WCHAR wszGUID[ 64 ] = { L"" };
	StringFromGUID2( guid, wszGUID, 64 );

	char ansiClsid[64];
	WideCharToMultiByte(CP_ACP, 0, wszGUID, 64, ansiClsid, 64, 0, 0);
	
	result = ansiClsid;
#elif defined(__APPLE__)
    CFUUIDRef uuid = CFUUIDCreate(0);
    CFStringRef uuidStrRef = CFUUIDCreateString(0, uuid);

	char szGUID[64];
	CFStringGetCString(uuidStrRef, &szGUID[0], sizeof(szGUID), kCFStringEncodingASCII);

    CFRelease(uuidStrRef);
    CFRelease(uuid);
    result = "{"; result.append(szGUID); result.append("}");
	
	boost::to_lower(result);
#else

	boost::uuids::uuid u = gen();

	result.reserve(38);
    result += '{';

    std::size_t i=0;
    for (boost::uuids::uuid::const_iterator it_data = u.begin(); it_data!=u.end(); ++it_data, ++i) {
        const size_t hi = ((*it_data) >> 4) & 0x0F;
        result += boost::uuids::detail::to_char(hi);

        const size_t lo = (*it_data) & 0x0F;
        result += boost::uuids::detail::to_char(lo);

        if (i == 3 || i == 5 || i == 7 || i == 9) {
            result += '-';
        }
    }

    result += '}';
#endif
}

void RBX::Guid::generateRBXGUID(RBX::Guid::Scope& result)
{
	std::string tmp;
	generateRBXGUID(tmp);

	result.set( tmp );
}

void RBX::Guid::generateRBXGUID(std::string& result)
{
	// Start with a text character (so it conforms to xs:ID requirement)
	generateStandardGUID(result);
	result = "RBX" + result;

	// result looks like this: RBX{c200e360-38c5-11ce-ae62-08002b2b79ef} 

	// strip the {}- characters
	result.erase(40, 1);
	result.erase(27, 1);
	result.erase(22, 1);
	result.erase(17, 1);
	result.erase(12, 1);
	result.erase(3, 1);

	// result looks like this: RBXc200e36038c511ceae6208002b2b79ef 

	// TODO: This string could be more compact if we included g-z
}

void RBX::Guid::assign(Data data)
{
	this->data = data;
}


bool RBX::Guid::Data::operator ==(const RBX::Guid::Data& other) const
{
	return index == other.index && scope == other.scope;
}

bool RBX::Guid::Data::operator <(const RBX::Guid::Data& other) const
{
	const int compare = scope.compare(other.scope);
	
	if (compare < 0)
		return true;
	if (compare > 0)
		return false;

	return index < other.index;
}

int RBX::Guid::compare(const Guid* a, const Guid* b)
{
	const Scope& sa = a ? a->data.scope : Scope::null();
	const Scope& sb = b ? b->data.scope : Scope::null();

	const int compare = sa.compare(sb);
	if (compare)
		return compare;
	int ia = a ? a->data.index : 0;
	int ib = b ? b->data.index : 0;
	return ia - ib;
}

int RBX::Guid::compare(const Guid* a0, const Guid* a1, const Guid* b0, const Guid* b1)
{
	int aComp = compare(a0, a1);
	int bComp = compare(b0, b1);

	const Guid* aMax = (aComp > 0) ? a0 : a1;
	const Guid* bMax = (bComp > 0) ? b0 : b1;

	const int compare = Guid::compare(aMax, bMax);
	if (compare)
		return compare;

	const Guid* aMin = (aComp > 0) ? a1 : a0;
	const Guid* bMin = (bComp > 0) ? b1 : b0;

	return Guid::compare(aMin, bMin);
}

#pragma warning(push)
#pragma warning(disable: 4996) //  warning C4996: 'sprintf': This function or variable may be unsafe. Consider using sprintf_s instead. 

std::string RBX::Guid::Data::readableString(int scopeLength) const
{

	char result[64];
	if (scopeLength>0)
	{
		std::string s = scope.getName()->toString();
		s = s.substr(std::min((size_t)3,s.size()), std::min(scopeLength,32));
		sprintf(result, "%s_%d", s.c_str(), index);
	}
	else
		sprintf(result, "%d", index);
	return result;

}

#pragma warning(pop)
