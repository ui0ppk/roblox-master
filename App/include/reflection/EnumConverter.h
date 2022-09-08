#pragma once

#include "reflection/Type.h"
#include "util/utilities.h"
#include "util/math.h"
#include <boost/bind.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/once.hpp>

namespace RBX {


	namespace Reflection {

		class EnumDescriptor : public Type
		{
		public:
			static std::vector< const EnumDescriptor* >::const_iterator enumsBegin();
			static std::vector< const EnumDescriptor* >::const_iterator enumsEnd();
            static size_t allEnumSize() {return allEnums().size();}

			class Item : public Descriptor
			{
			public:
				const EnumDescriptor& owner;
				const int value;			// value of the enum
				const size_t index;	// place in ordered enum values  (0<=index<enumCount)
				Item(const char* name, Descriptor::Attributes attributes, int value, size_t index, const EnumDescriptor& owner)
					:Descriptor(name, attributes)
					,value(value)
					,index(index)
					,owner(owner)
				{
				}
				bool convertToValue(Variant& value) const
				{
					return owner.convertToValue(index, value);
				}
				bool convertToString(std::string& value) const
				{
					return owner.convertToString(index, value);
				}
			};

#if 0
			typedef boost::unordered_map<const RBX::Name*, const EnumDescriptor*> EnumNameTable;
#else
			typedef std::map<const RBX::Name*, const EnumDescriptor*> EnumNameTable;
#endif

		private:
			static EnumNameTable& allEnumsNameLookup();
			static std::vector<const EnumDescriptor*>& allEnums();

			static bool equalValue(const Item* item, int intValue)
			{
				return item->value == intValue;
			}

			static int count;
		protected:
			std::vector< const Item* > allItems;
			size_t enumCount;
			size_t enumCountMSB;
			EnumDescriptor(const char* typeName);
			~EnumDescriptor();
		public:
			size_t getEnumCount() const { return enumCount; }
			size_t getEnumCountMSB() const { return enumCountMSB; }
			std::vector< const Item* >::const_iterator begin() const {
				return allItems.begin();
			}		
			std::vector< const Item* >::const_iterator end() const {
				return allItems.end();
			}		
			static const EnumDescriptor* lookupDescriptor(const RBX::Name& name) {
				EnumNameTable::const_iterator iter = allEnumsNameLookup().find(&name);
				if (iter!=allEnumsNameLookup().end())
					return iter->second;
				else
					return NULL;
			}
			static const EnumDescriptor* lookupDescriptor(const Type& type) {
				if (type.isEnum)
					return static_cast<const EnumDescriptor*>(&type);
				else
					return NULL;
			}

			bool isValue(int intValue) const {
				return std::find_if(allItems.begin(), allItems.end(), boost::bind(&equalValue, _1, intValue)) != allItems.end();
			}
			virtual const Item* lookup(const char* text) const = 0;
			virtual const Item* lookup(const Variant& value) const = 0;
			virtual bool convertToValue(size_t index, Variant& value) const = 0;
			virtual bool convertToString(size_t index, std::string& value) const = 0;
		};

		// A thread-safe singleton!
		template<typename T>
		class Singleton : boost::noncopyable
		{
			static T& doGetSingleton()
			{
				static T s;
				return s;
			}
			static void initSingleton()
			{
				doGetSingleton();
			}
		public:
			static T& singleton()
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(&initSingleton, flag);
				return doGetSingleton();
			};
		};

template<typename Enum> class EnumRegistrar;

		template<typename Enum>
		class EnumDesc : public EnumDescriptor
		{
		public:
			friend class Singleton<const EnumDesc<Enum> >;
			static const EnumDesc& singleton()
			{
				return Singleton<const EnumDesc<Enum> >::singleton();
			}
			
		private:
			// You must implement the following constructor for each EnumDesc that you define
			EnumDesc();
			~EnumDesc()
			{
				// Force linking of EnumRegistrar<Enum>, which will force clients
				// of this library to define EnumRegistrar<Enum>::registrar in
				// their startup code.
				
				
				Reflection::EnumRegistrar<Enum>::registrar.dummy();

				std::for_each(allItems.begin(), allItems.end(), &del_fun<const Item>);
			}

			std::map<const RBX::Name*, Enum> nameToEnum;
			std::map<const RBX::Name*, Enum> nameToEnumLegacy;
			std::vector< const RBX::Name* > enumToName;		// maps enum to Name (there may be gaps)

			std::vector< std::string > enumToString;		// maps enum to String (there may be gaps)
			std::vector< const Item* > enumToItem;		// maps enum to Item (there may be gaps)

			std::vector< Enum > intToEnum;  // maps legacy values to proper enum
			std::vector< Enum > indexToEnum;
			std::vector< size_t > enumToIndex;		// maps enum to Index (there may be gaps)

			
			// Used in constructor
			void addPair(Enum value, const char* name, Descriptor::Attributes attributes = Descriptor::Attributes())
			{
				RBXASSERT_VERY_FAST(value >= 0);
				// No spaces in enums:
				RBXASSERT_VERY_FAST(std::string(name).find(' ') == std::string::npos);
				// No no CamelCase in enums:
				RBXASSERT_VERY_FAST(!isCamel(name));

				const Item* item = new Item(name, attributes, value, enumCount, *this);

				allItems.push_back(item);

				if (intToEnum.size()<=(size_t)value)
					intToEnum.resize(value+1, (Enum)-1);
				intToEnum[value] = value;

				RBXASSERT(value>=0);

				if (enumToIndex.size()<=(size_t)value)
					enumToIndex.resize(value+1, -1);
				enumToIndex[value] = enumCount;
				indexToEnum.push_back(value);

				if (enumToName.size()<=(size_t)value)
					enumToName.resize(value+1, &RBX::Name::getNullName());
				enumToName[value] = &item->name;

				if (enumToString.size()<=(size_t)value)
					enumToString.resize(value+1);
				enumToString[value] = name;

				if (enumToItem.size()<=(size_t)value)
					enumToItem.resize(value+1);
				enumToItem[value] = item;

				nameToEnum[&item->name] = value;

				enumCount++;
				enumCountMSB = Math::computeMSB(enumCount);
			}
			void addLegacy(int oldValue, const char* name, Enum value)
			{
				RBXASSERT_VERY_FAST(value >= 0);

				if (intToEnum.size()<=(size_t)oldValue)
					intToEnum.resize(oldValue+1, (Enum)-1);
				intToEnum[oldValue] = value;
				nameToEnumLegacy[&RBX::Name::declare(name)] = value;
			}
			void addLegacyName(const char* name, Enum value)
			{
				nameToEnumLegacy[&RBX::Name::declare(name)] = value;
			}
		public:
			const RBX::Name& convertToName(const Enum& value) const
			{
				RBXASSERT(value>=0);
				RBXASSERT(value<enumToItem.size());
				if (value<0)
					return RBX::Name::getNullName();
				if ((size_t)value>=enumToName.size())
					return RBX::Name::getNullName();

				return *enumToName[value];
			}
			std::string convertToString(const Enum& value) const
			{
				RBXASSERT(value>=0);
				RBXASSERT((size_t)value<enumToItem.size());
				if (value<0)
					return "";
				if ((size_t)value>=enumToString.size())
					return "";

				return enumToString[value];
			}
			const Item* convertToItem(const Enum& value) const
			{
				RBXASSERT(value>=0);
				RBXASSERT((size_t)value<enumToItem.size());
				if (value<0)
					return NULL;
				if ((size_t)value>=enumToItem.size())
					return NULL;

				return enumToItem[value];
			}

			bool mapIntValue(int intValue, Enum& value) const
			{
				if (intValue < 0)
					return false;

				if ((size_t)intValue >= intToEnum.size())
					return false;

				value = intToEnum[intValue];
                if ((int)value == -1)
					return false;
				return true;
			}

			bool convertToValue(const RBX::Name& name, Enum& value) const
			{
				typename std::map<const RBX::Name*, Enum>::const_iterator iter = nameToEnum.find(&name);
				if (iter!=nameToEnum.end()) {
					value = iter->second;
					return true;
				}

				iter = nameToEnumLegacy.find(&name);
				if (iter!=nameToEnumLegacy.end()) {
					value = iter->second;
					return true;
				}

				return false;
			}
			/*implement*/ const Item* lookup(const char* text) const
			{
				Enum e;
				if (convertToValue(RBX::Name::lookup(text), e))
					return convertToItem(e);
				else
					return NULL;
			}
			/*implement*/ const Item* lookup(const Variant& value) const 
			{
				return convertToItem(value.cast<Enum>());
			}

			/*implement*/ bool convertToValue(size_t index, Variant& value) const
			{
				Enum enumValue;
				bool result = convertToValue(index, enumValue);
				value = enumValue;
				return result;
			}
			/*implement*/ bool convertToString(size_t index, std::string& stringValue) const
			{
				Enum enumValue;
				if(convertToValue(index, enumValue)){
					stringValue = convertToString(enumValue);
					return true;
				}
				return false;
			}


			bool convertToValue(const char* text, Enum& value) const
			{
				return convertToValue(RBX::Name::lookup(text), value);
			}
			size_t convertToIndex(Enum value) const
			{
				RBXASSERT(value>=0);
				if ((size_t)value<enumToIndex.size())
					return enumToIndex[value];
				else
					return -1;
			}
			bool convertToValue(size_t index, Enum& value) const
			{
				if (index<enumCount) {
					value = indexToEnum[index];
					return true;
				}
				else {
					return false;
                }
			}
		};

// Helper macro
// GCC does not generate the registrar variable defination & fails at Link Time. Force Construct by passing in an dummy arg to ctor. That works. WEIRD huh? 
#define RBX_REGISTER_ENUM(Enum)		namespace RBX { namespace Reflection {								\
									template<>															\
									const Type& Type::getSingleton<Enum>()			  					\
									{																	\
										return EnumDesc<Enum>::singleton();							  	\
									}																	\
									template<> EnumRegistrar<Enum> EnumRegistrar<Enum>::registrar(0);	\
									template<> TypeRegistrar<Enum> TypeRegistrar<Enum>::registrar(0);	\
									}}
		
		// This class is intended to prevent clients of the library
		// from forgetting to initialize the enum descriptor
		template<typename Enum>
		class EnumRegistrar : boost::noncopyable
		{
			int x;

			//// GCC does not generate the registrar variable defination & fails at Link Time. Force Construct by passing in an dummy arg to ctor. That works. WEIRD huh? 
			EnumRegistrar(int i):x(i)
			{
				// This call registers the enum descriptor 
				// in the reflection database
				EnumDesc<Enum>::singleton();
			}
		public:
			void dummy()
			{
				x++;
			}

			// The instantiation of this static member must be in a unit 
			// that is initialized in the main thread before any objects
			// are created. Otherwise the reflection database
			// can change at runtime, which would be a disaster
			static EnumRegistrar registrar;	
		};


	}
}
