
#pragma once

#include "reflection/Descriptor.h"
#include <boost/any.hpp>
#include <boost/static_assert.hpp>
#include <util/utilities.h>

#include <boost/unordered_map.hpp>
#include <list>


namespace RBX
{
	namespace Reflection
	{
		template<typename T> class TypeRegistrar;
	
		// Types supported by the Reflection framework
		class Type : public Descriptor
		{
			template<class T>
			friend class TypeRegistrar;

			template<class T>
			static const Type& getSingleton();		// Must be implemented for each type used
			void addToAllTypes();

		public:
			const Name& tag;
			const bool isFloat;
			const bool isNumber;
			const bool isEnum;

			static const std::vector<const Type*>& getAllTypes();

			template<class T>
			static inline const Type& singleton()
			{
				return getSingleton<T>();
			}

			bool operator==(const Type& right) const {
				return this==&right;
			}
			bool operator!=(const Type& right) const {
				return this!=&right;
			}

			template<class T>
			bool isType() const {
				return this == &getSingleton<T>();
			}

		protected:
			template<class T>
			Type(const char* name, T* dummy)
				:Descriptor(name, Descriptor::Attributes())
				,tag(Name::lookup(name))
				,isNumber(boost::is_arithmetic<T>::value)
				,isFloat(boost::is_float<T>::value)
				,isEnum(false)
			{
                *isOutdated = false;
                *isReplicable = true;
				RBXASSERT(!this->tag.empty());
				addToAllTypes();
			}
			template<class T>
			Type(const char* name, const char* tag, T* dummy)
				:Descriptor(name, Descriptor::Attributes())
				,tag(Name::declare(tag))
				,isNumber(boost::is_arithmetic<T>::value)
				,isFloat(boost::is_float<T>::value)
				,isEnum(false)
			{
				RBXASSERT(!this->tag.empty());
				addToAllTypes();
			}

			Type(const char* name, const char* tag, bool isNumber, bool isFloat, bool isEnum)
				:Descriptor(name, Descriptor::Attributes())
				,tag(Name::declare(tag))
				,isNumber(isNumber)
				,isFloat(isFloat)
				,isEnum(isEnum)
			{
				RBXASSERT(!this->tag.empty());
				addToAllTypes();
			}
		};

		std::ostream& operator<<(std::ostream& os, const RBX::Reflection::Type& type);

		// Handy macro for registering a type
#define RBX_REGISTER_TYPE(mType)		template<> RBX::Reflection::TypeRegistrar<mType> RBX::Reflection::TypeRegistrar<mType>::registrar(0)

		// This class is designed to prevent clients of the library
		// from forgetting to initialize their class descriptors
		template<class T>
		class TypeRegistrar : boost::noncopyable
		{
			int x;

			//// GCC does not generate the registrar variable defination & fails at Link Time. Force Construct by passing in an dummy arg to ctor. That works. WEIRD huh? 
			TypeRegistrar(int i):x(i)
			{
				// This assertion is added to catch a nasty implicit use of boost::any with Variant objects.
				// If you get a tricky link error, add your own assertion here
				BOOST_STATIC_ASSERT((!boost::is_same<T, boost::any>::value)); 
				// This call registers the Type descriptor 
				// in the reflection database
				Type::getSingleton<T>();
			}

		public:
			// The instantiation of this static member must be in a unit 
			// that is initialized in the main thread before any objects
			// are created. Otherwise the reflection database
			// can change at runtime, which would be a disaster
			static TypeRegistrar registrar;	
		};

		// Helper class
		template<typename T>
		class TType : public Type
		{
			friend class Type;
		protected:
			TType(const char* name)
				:Type(name, (T*)NULL)
			{
			}
			TType(const char* name, const char* tag)
				:Type(name, tag, (T*)NULL)
			{
			}
		};

		class Variant
		{
            struct Storage
            {
                char data[96];
            };

			const Type* _type;
			rbx::placement_any<Storage> value;

		public:
			inline Variant()
				: _type(&Type::singleton<void>())
				, value()
			{}

			inline Variant(const Variant& other)
				: _type(other._type)
				, value(other.value)
			{}

			inline Variant& operator=(const Variant& rhs)
			{
				_type = rhs._type;
				value = rhs.value;
				return *this;
			}

			template<typename ValueType>
			inline Variant(const ValueType& value)
				: _type(&Type::singleton<ValueType>())
				, value(value)
			{
			}

			template<typename ValueType>
			inline Variant& operator=(const ValueType& rhs)
			{
				_type = &Type::singleton<ValueType>();
				value = rhs;
				return *this;
			}

			inline const Type& type() const {
				return *_type;
			}

			inline bool isVoid() const
			{
				return *_type==Type::singleton<void>();
			}
			inline bool isFloat() const { return type().isFloat; }
			inline bool isNumber() const { return type().isNumber; }
			inline bool isString() const { return isType<std::string>();}

			template<class ValueType>
			inline bool isType() const {
				return _type->isType<ValueType>();
			}

			// throws an exception if unable to convert
			template<typename ValueType>
			ValueType& convert();

			// throws an exception if unable to convert
			template<typename ValueType>
			inline ValueType get() const
			{
				if (isType<ValueType>())
					return cast<ValueType>();
				else
				{
					// Create a non-const copy to extract the value from
					Variant v(*this);
					return v.convert<ValueType>();
				}
			}

			template<typename T>
			inline const T& cast() const {
				if (!isType<T>())
					throw std::runtime_error("Variant cast failed");
				return *reinterpret_cast<const T*>(value.getData());
			}

			template<typename T>
			inline T& cast() {
				if (!isType<T>())
					throw std::runtime_error("Variant cast failed");
				return *reinterpret_cast<T*>(value.getData());
			}

			template<typename T>
			inline const T* tryCast() const {
				if (!isType<T>())
					return NULL;
				return reinterpret_cast<const T*>(value.getData());
			}

			template<typename T>
			inline T* tryCast() {
				if (!isType<T>())
					return NULL;
				return reinterpret_cast<T*>(value.getData());
			}

		private:
			template<class ValueType>
			ValueType& genericConvert();

		};

		// Equivalent to an array in Lua
		typedef std::vector<Variant> ValueArray;

		// A limited table in Lua (keys must be strings for now)
		typedef boost::unordered_map<std::string, Variant> ValueTable;

		struct Tuple
		{
			ValueArray values;
			Tuple() {}
			Tuple(size_t count):values(count) {}
			Tuple(const Tuple& other):values(other.values) {}
			//Tuple(const ValueArray& values):values(values) {}
			Variant& at(size_t i) { return values[i]; }
			const Variant& at(size_t i) const { return values[i]; }
		};

		// The same as a ValueTable for now, but will always have a string key.
		// TODO: Use boost::unordered_map<> or vector<> instead?
		typedef std::map<std::string, Variant> ValueMap;

		// Describes a function's signature
		class SignatureDescriptor
		{
		public:
			struct Item {
				friend class SignatureDescriptor;
			public:
				Item(const RBX::Name* name, const Type* type, const Variant& defaultValue);
				Item(const RBX::Name* name, const Type* type);
				const RBX::Name* name;
				const Type* type;
				const Variant defaultValue;
				bool hasDefaultValue() const
				{
					return defaultValue.type() == *type;
				}
			};
			// TODO: Would vector be more efficient?
			typedef std::list<Item> Arguments;

			const Type* resultType;
			Arguments arguments;

			void addArgument(const RBX::Name& name, const Type& type);
			void addArgument(const RBX::Name& name, const Type& type, const Variant& defaultValue);

			SignatureDescriptor();
		};

		template<class ValueType>
		ValueType& RBX::Reflection::Variant::genericConvert()
		{
			ValueType* id = tryCast<ValueType>();
			if (id!=NULL)
				return *id;

			if (_type->isType<std::string>())
			{
				ValueType v;
				if (StringConverter<ValueType>::convertToValue(cast<std::string>(), v))
				{
					value = v;
					_type = &Type::singleton<ValueType>();
					return cast<ValueType>();
				}
			}

			throw RBX::runtime_error("Unable to cast %s to %s", _type->tag.c_str(), Type::singleton<ValueType>().tag.c_str() );
		}	
	}
}
