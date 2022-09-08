#include "stdafx.h"

#include "reflection/type.h"
#include "reflection/member.h"

#include <boost/functional/hash.hpp>


namespace RBX { namespace Reflection {

	size_t StringHashPredicate::operator ()(const char* s) const
	{
		return boost::hash_range(s, s + strlen(s));
	}


std::ostream& operator<<( std::ostream& os, const Type& type )
{
	return os << "Type::" << type.name;
}

template<>
const Type& Type::getSingleton<shared_ptr<const ValueArray> >()
{
	static TType<shared_ptr<const ValueArray> > type("Array");
	return type;
}

template<>
const Type& Type::getSingleton<Variant>()
{
	static TType<shared_ptr<const ValueArray> > type("Variant");
	return type;
}

template<>
Variant& Variant::convert<Variant>(void)
{
	return *this;
}

template<>
shared_ptr<const ValueArray>& Variant::convert<shared_ptr<const ValueArray> >(void)
{
	shared_ptr<const ValueArray>* v = tryCast<shared_ptr<const ValueArray> >();
	if (v)
		return *v;
	shared_ptr<const ValueTable>* t = tryCast<shared_ptr<const ValueTable> >();
	if (t)
	{
		// empty table is the same as an empty array. (This makes Lua easier, since an empty Lua table could be either)
		if ((*t)->size()==0)
		{
			value = shared_ptr<const ValueArray>(new ValueArray());
			_type = &Type::singleton<shared_ptr<const ValueArray> >();
			return cast<shared_ptr<const ValueArray> >();
		}
	}
	throw std::runtime_error("Unable to cast to Array");
}

template<>
const Type& Type::getSingleton<shared_ptr<const ValueTable> >()
{
	static TType<shared_ptr<const ValueTable> > type("Dictionary");
	return type;
}

template<>
shared_ptr<const ValueTable>& Variant::convert<shared_ptr<const ValueTable> >(void)
{
	shared_ptr<const ValueTable>* v = tryCast<shared_ptr<const ValueTable> >();
	if (v)
		return *v;
	shared_ptr<const ValueArray>* a = tryCast<shared_ptr<const ValueArray> >();
	if (a)
	{
		// empty array is the same as an empty table. (This makes Lua easier, since an empty Lua table could be either)
		if ((*a)->size()==0)
		{
			value = shared_ptr<const ValueTable>(new ValueTable());
			_type = &Type::singleton<shared_ptr<const ValueTable> >();
			return cast<shared_ptr<const ValueTable> >();
		}
	}

	throw std::runtime_error("Unable to cast to Dictionary");
}

template<>
const Type& Type::getSingleton<shared_ptr<const ValueMap> >()
{
	static TType<shared_ptr<const ValueMap> > type("Map");
	return type;
}

template<>
shared_ptr<const ValueMap>& Variant::convert<shared_ptr<const ValueMap> >(void)
{
	shared_ptr<const ValueMap>* v = tryCast<shared_ptr<const ValueMap> >();
	if (v==NULL)
		throw std::runtime_error("Unable to cast to Map");
	return *v;
}

// Declare the "void" type now
template<>
const Type& Type::getSingleton<void>()
{
	static TType<void> type("void");
	return type;
}

static std::vector<const Type*>* allTypes;

void Type::addToAllTypes()
{
	static std::vector<const Type*> v;
	allTypes = &v;
	v.push_back(this);
}

const std::vector<const Type*>& Type::getAllTypes()
{
	return *allTypes;
}

SignatureDescriptor::SignatureDescriptor()
	:resultType(&Type::singleton<void>())
{
}

SignatureDescriptor::Item::Item(const RBX::Name* name, const Type* type, const Variant& defaultValue)
	:name(name)
	,type(type)
	,defaultValue(defaultValue)
{
	// Arguments must be camelCase
	RBXASSERT(name->c_str()[0] == tolower(name->c_str()[0]));
}

SignatureDescriptor::Item::Item(const RBX::Name* name, const Type* type)
	:name(name)
	,type(type)
	,defaultValue()
{
	// Arguments must be camelCase
	RBXASSERT(name->c_str()[0] == tolower(name->c_str()[0]));
	RBXASSERT(defaultValue.isVoid());
}

void SignatureDescriptor::addArgument(const RBX::Name& name, const Type& type)
{
	Item i(&name, &type);
	arguments.push_back(i);
}

void SignatureDescriptor::addArgument(const RBX::Name& name, const Type& type, const Variant& defaultValue)
{
	RBXASSERT(defaultValue.isVoid() || defaultValue.type()==type);
	Item i(&name, &type, defaultValue);
	arguments.push_back(i);
}

}} // namespace

