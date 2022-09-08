#pragma once

// Roblox Headers
#include "v8tree/Instance.h"
#include "reflection/reflection.h"
#include <boost/filesystem.hpp>

namespace RBX
{
	namespace Reflection
	{
		// A collection of classes that contain metadata for class documentation
		// TODO: Support Enum matadata
		namespace Metadata
		{
			void writeEverything(std::ostream& stream);

			class Classes;
			class Class;
			class Member;
			class Enums;
			class Enum;
			class EnumItem;

			extern const char* const sReflection;
			class Reflection : public DescribedCreatable<Reflection, Instance, sReflection>
			{
				Classes* classes;
				Enums* enums;
				static shared_ptr<Reflection> safe_static_do_get_singleton();
				static void safe_static_init_singleton() { safe_static_do_get_singleton(); }
			public:
				Reflection();
				static void registerClasses();

				static shared_ptr<Reflection> singleton()
				{
					static boost::once_flag once_init_singleton = BOOST_ONCE_INIT;
					boost::call_once(safe_static_init_singleton, once_init_singleton);
					return safe_static_do_get_singleton();
				}

				void save(const boost::filesystem::path& filePath);
				void load(const boost::filesystem::path& file);
				// findBestMatch means find a base Class object if none exists for descriptor
				Class* get(const ClassDescriptor& descriptor, bool findBestMatch) const;
				Member* get(const PropertyDescriptor& descriptor) const;
				Member* get(const FunctionDescriptor& descriptor) const;
				Member* get(const YieldFunctionDescriptor& descriptor) const;
				Member* get(const EventDescriptor& descriptor) const;
				Member* get(const CallbackDescriptor& descriptor) const;

				Enum* get(const EnumDescriptor& descriptor) const;
				EnumItem* get(const EnumDescriptor::Item& item) const;
			};

			extern const char* const sClasses;
			class Classes : public DescribedCreatable<Classes, Instance, sClasses>
			{
			public:
				Classes() {;}
				Class* get(const ClassDescriptor& descriptor, bool findBestMatch);
			};

			extern const char* const sItem;
			class Item : public DescribedNonCreatable<Item, Instance, sItem>
			{
				friend class Classes;
				bool browsable;
				bool deprecated;
				bool backend;		// This item can only be used in game servers and solo games
			public:
				static bool isDeprecated(const Item* item, const Descriptor& descriptor)	// TODO: || with owner, if any
				{
					if (item && item->deprecated)
						return true;
					return descriptor.attributes.isDeprecated;
				}
				// Browsable means it is visible in docs for end-users
				bool isBrowsable() const { return browsable; }	    // TODO: && with owner, if any
				static bool isBackend(const Item* item, const Descriptor& descriptor)	// TODO: || with owner, if any
				{
					if (item && item->backend)
						return true;
					return false;
				}
				static bool isBrowsable(const Item* item, const Descriptor& descriptor)	// TODO: || with owner, if any
				{
					if (item && item->browsable)
						return true;
					return false;
				}
				std::string description;
				double minimum;
				double maximum;

				Item()
					: browsable(true)
					, deprecated(false)
					, backend(false)
					, minimum(0)
					, maximum(0)
				{}

				static BoundProp<bool> prop_deprecated;
				static BoundProp<bool> prop_browsable;
			private:
				static BoundProp<bool> prop_backend;
				static BoundProp<std::string> prop_description;
				static BoundProp<double> prop_minimum;
				static BoundProp<double> prop_maximum;
			};

			extern const char* const sClass;
			class Class : public DescribedCreatable<Class, Item, sClass>
			{
				int explorerOrder;		// -1 hide,  0 unspecified,  1+ order to show
				int explorerImageIndex;	// -1 unspecified,  0 generic image
				std::string preferredParent; // class name for the service you want this object to default under (Players, Lighting, StarterGui, etc.)
                bool insertable; // class will show up in the list of insertable objects
			public:
				static BoundProp<int> prop_ExplorerOrder;
				static BoundProp<int> prop_ExplorerImageIndex;
				static BoundProp<std::string> prop_PreferredParent;
				static BoundProp<bool> prop_Insertable;
				
				const Class* getBase() const { return dynamic_cast<const Class*>(getParent()); };

				int getExplorerOrder() const { 
					return explorerOrder==0 
						? (getBase() && getBase()->getExplorerOrder())
						: explorerOrder;
				}
				bool isExplorerItem() const { 
					return getExplorerOrder()>=0;
				}
				int getExplorerImageIndex() const { 
					return explorerImageIndex==-1
						? (getBase() && getBase()->getExplorerImageIndex())
						: std::max(explorerImageIndex, 0);		// don't ever return -1 
				}

				std::string getPreferredParent() const {
					return preferredParent;
				}

                bool getInsertable() const { return insertable; }
                
				Class():explorerOrder(-1),explorerImageIndex(0), preferredParent(""),insertable(true) {}
			};

			class Members : public Instance
			{
			public:
				Members() { }
			};

			extern const char* const sProperties;
			class Properties : public DescribedCreatable<Properties, Members, sProperties>
			{
			public:
				Properties() { }
			};

			extern const char* const sFunctions;
			class Functions : public DescribedCreatable<Functions, Members, sFunctions>
			{
			public:
				Functions() { }
			};

			extern const char* const sYieldFunctions;
			class YieldFunctions : public DescribedCreatable<YieldFunctions, Members, sYieldFunctions>
			{
			public:
				YieldFunctions() { }
			};

			extern const char* const sEvents;
			class Events : public DescribedCreatable<Events, Members, sEvents>
			{
			public:
				Events() { }
			};

			extern const char* const sCallbacks;
			class Callbacks : public DescribedCreatable<Callbacks, Members, sCallbacks>
			{
			public:
				Callbacks() { }
			};

			extern const char* const sMember;
			class Member : public DescribedCreatable<Member, Item, sMember>
			{
			public:
				Member() { }
			};

			extern const char* const sEnums;
			class Enums : public DescribedCreatable<Enums, Instance, sEnums>
			{
			public:
				Enums() {}
			};

			extern const char* const sEnum;
			class Enum : public DescribedCreatable<Enum, Item, sEnum>
			{
			public:
				Enum() { }
			};

			extern const char* const sEnumItem;
			class EnumItem : public DescribedCreatable<EnumItem, Item, sEnumItem>
			{
			public:
				EnumItem() { }
			};
		}
	}
}

