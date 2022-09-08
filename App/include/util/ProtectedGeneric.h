#pragma once
#include <memory>
#include <string>
#include <boost/functional/hash/hash.hpp>

namespace RBX {

	template<class Type>
	class ProtectedGeneric
	{
	private:
		Type value;
		std::size_t hash;
	public:
		const Type& peekValue() const 
		{ 
			return value; 
		}
		bool getValue(Type& _value) const
		{
			_value = this->value;

			boost::hash<Type> hasher;
			std::size_t newHash = hasher(_value);
			return (hash == newHash);
		}
		void setValue(Type _value)
		{
			this->value = _value;

			boost::hash<Type> hasher;
			hash = hasher(_value);
		}

		ProtectedGeneric(Type _value)
		{
			setValue(_value);
		}
	private:
		ProtectedGeneric(const ProtectedGeneric& other)
		{}
	};
}
