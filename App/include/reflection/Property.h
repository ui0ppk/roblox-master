
#pragma once

#include "reflection/member.h"
#include "reflection/enumconverter.h"
#include "reflection/type.h"
#include "v8xml/xmlelement.h"
#include "V8Xml/Reference.h"	// TODO: Reflection namespace should not know about V8Tree
#include "boost/cast.hpp"

namespace RBX
{
	namespace Reflection
	{
		class ConstProperty;
		class Property;

		typedef enum { 
			READONLY,
			READWRITE
		} Mutability;

		// Base that describes a Property
		class RBXBaseClass PropertyDescriptor : public MemberDescriptor
		{
		private:
			unsigned bIsPublic : 1;
            unsigned bIsEditable : 1;
			unsigned bCanReplicate : 1;
			unsigned bCanXmlRead : 1;
			unsigned bCanXmlWrite : 1;
			unsigned bIsScriptable : 1;
			unsigned bAlwaysClone : 1;

		public:
			typedef ConstProperty ConstMember;
			typedef Property Member;
			
		public:
			// Note:  isPublic == PropertyUI shown, and Can BaseScript against.  ToDO:  possibly split?

			enum Functionality { 
				STANDARD	 	   = 1 + 2 + 4 + 8 + 16,      // isPublic, canReplicate,  canXmlRead , canXmlWrite,  isScriptable
				NO_XML_WRITE       = 1 + 2 + 4 + 0 + 16,	  // isPublic, canReplicate,  canXmlRead ,			  ,  isScriptable
				UI				   = 1 + 0 +(4)+ 0 + 16,      // isPublic,			     (canXmlRead), 	   		     isScriptable	//Remove canXmlRead from UI
				SCRIPTING		   = 1 + 2 + 0 + 0 + 16,      // isPublic, canReplicate,						     isScriptable
				STREAMING		   = 0 + 2 + 4 + 8 +  0,      //           canReplicate,  canXmlRead , canXmlWrite, 
				CLUSTER 		   = 0 + 0 + 4 + 8 +  0,      //						  canXmlRead , canXmlWrite, 
				LEGACY			   = 0 + 0 + 4 + 0 +  0,      //						  canXmlRead ,
				REPLICATE_ONLY	   = 0 + 2 + 0 + 0 +  0,      //		   canReplicate						
				LEGACY_SCRIPTING   = 0 + 0 + 4 + 0 + 16,      //						  canXmlRead ,		         isScriptable
				HIDDEN_SCRIPTING   = 0 + 0 + 0 + 0 + 16,      //						 							 isScriptable
				PUBLIC_SERIALIZED  = 1 + 0 + 4 + 8 +  0,      // isPublic,				  canXmlRead , canXmlWrite,
				REPLICATE_CLONE	   = 0 + 2 + 0 + 0 +  0 + 32, //		   canReplicate					                           alwaysClone
				STANDARD_NO_REPLICATE = 1 + 0 + 4 + 8 + 16,	  // isPublic,                canXmlRead , canXmlWrite,   isScriptable
				STANDARD_NO_SCRIPTING = 1 + 2 + 4 + 8 +  0,   // isPublic, canReplicate,  canXmlRead , canXmlWrite
				PUBLIC_REPLICATE  =  1 + 2 + 0 + 0 +  0,      // isPublic, canReplicate
			};

			struct Attributes : public Descriptor::Attributes
			{
				Functionality flags;

				Attributes():flags(STANDARD) {}
				Attributes(Functionality flags):flags(flags) {}
				static Attributes deprecated(const MemberDescriptor& preferred, Functionality flags = UI);
				static Attributes deprecated(Functionality flags = UI);
			};

			const Type& type;
			const bool bIsEnum;

		protected:
			PropertyDescriptor(ClassDescriptor& classDescriptor, const Type& type, const char* name, const char* category, Attributes attributes, Security::Permissions security, bool isEnum = false);

			inline void checkFlags()
			{
				if (isWriteOnly())
				{
					bCanXmlWrite = 0;
					bCanReplicate = 0;
				}
				if (isReadOnly())
				{
					bCanXmlRead = 0;
					bCanReplicate = 0;
				}
			}
		public:
			inline bool isPublic() const { return bIsPublic != 0; }
			inline bool isScriptable() const { return bIsScriptable != 0; }

            void setEditable(bool editable) { bIsEditable = editable ? 1 : 0; }
            inline bool isEditable() const { return bIsEditable != 0; }

			virtual bool isReadOnly() const = 0;
			virtual bool isWriteOnly() const = 0;
			inline bool canXmlRead() const 
			{ 
				RBXASSERT(bCanXmlRead == 0 || !isReadOnly()); 
				return bCanXmlRead != 0; 
			}
			inline bool canXmlWrite() const 
			{ 
				RBXASSERT(bCanXmlWrite == 0 || !isWriteOnly()); 
				return bCanXmlWrite != 0; 
			}
			inline bool canReplicate() const 
			{ 
				RBXASSERT(bCanReplicate == 0 || (!isReadOnly() && !isWriteOnly())); 
				return bCanReplicate != 0; 
			}
			inline bool alwaysClone() const
			{
				return bAlwaysClone != 0;
			}

			bool operator==(const PropertyDescriptor& other) const {
				return this == &other;
			}
			bool operator!=(const PropertyDescriptor& other) const {
				return this != &other;
			}

			virtual bool equalValues(const DescribedBase* a, const DescribedBase* b) const = 0;

			virtual void getVariant(const DescribedBase* instance, Variant& value) const = 0;
			virtual void setVariant(DescribedBase* instance, const Variant& value) const = 0;
			virtual void copyValue(const DescribedBase* source, DescribedBase* destination) const = 0;

            virtual int getDataSize(const DescribedBase* instance) const = 0;

			////////////////////////////////////////////////////////////////////////////////////////////////////
			// String conversion interface
		public:
			virtual bool hasStringValue() const = 0;
			virtual std::string getStringValue(const DescribedBase* instance) const  {
				if (hasStringValue())
					debugAssertM(false, "you must implement getStringValue");
				else
					debugAssertM(false, "don't call getStringValue when hasStringValue()==false");
				return "";
			}
			virtual bool setStringValue(DescribedBase* instance, const std::string& text) const {
				if (hasStringValue())
					debugAssertM(false, "you must implement setStringValue");
				else
					debugAssertM(false, "don't call setStringValue when hasStringValue()==false");
				return false;
			}
			//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/

			////////////////////////////////////////////////////////////////////////////////////////////////////
			// Xml streaming interface
		public:
			XmlElement* write(const DescribedBase* instance, bool ignoreWriteProtection = false) const;
			virtual void read(DescribedBase* instance, const XmlElement* element, RBX::IReferenceBinder& binder) const;
		private:
			virtual void writeValue(const DescribedBase* instance, XmlElement* element) const = 0;
			virtual void readValue(DescribedBase* instance, const XmlElement* element, RBX::IReferenceBinder& binder) const = 0;
			//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/
		
		};


		template <typename V>
		class TypedPropertyDescriptor : public PropertyDescriptor
		{
		private:			
			typedef PropertyDescriptor Super;

		public:
			class RBXInterface GetSet
			{
			public:
				virtual bool isReadOnly() const = 0;
				virtual bool isWriteOnly() const = 0;
				virtual V getValue(const DescribedBase* object) const = 0;
				virtual void setValue(DescribedBase* object, const V& value) const = 0;
			};
		protected:
			std::auto_ptr<GetSet> getset;
			TypedPropertyDescriptor(ClassDescriptor& classDescriptor, const Type& type, const char* name, const char* category, std::auto_ptr<GetSet> getset, Attributes flags, Security::Permissions security)
				:PropertyDescriptor(classDescriptor, type, name, category, flags, security),getset(getset) 
			{
				if (this->getset.get())
					this->checkFlags();
			}
			TypedPropertyDescriptor(ClassDescriptor& classDescriptor, const char* name, const char* category, std::auto_ptr<GetSet> getset, Attributes flags, Security::Permissions security)
				:PropertyDescriptor(classDescriptor, Type::singleton<V>(), name, category, flags, security),getset(getset) 
			{
				if (this->getset.get())
					this->checkFlags();
			}
		public:
			/*implement*/ V get(const DescribedBase* instance) const 
			{
				return getset->getValue(instance);
			}
			/*implement*/ void set(DescribedBase* instance, const V& value) const 
			{
				getset->setValue(instance, value);
			}

			/*implement*/ void getVariant(const DescribedBase* instance, Variant& value) const 
			{
				value = getset->getValue(instance);
			}
			/*implement*/ void setVariant(DescribedBase* instance, const Variant& value) const 
			{
				// TODO: This might be inefficient. How is the value stored in getset???
				getset->setValue(instance, value.get<V>());
			}
			/*implement*/ void copyValue(const DescribedBase* source, DescribedBase* destination) const
			{
				set(destination, get(source));
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////
			// Variant interface
		public:
			virtual bool isReadOnly() const {
				return getset->isReadOnly();
			}

			virtual bool isWriteOnly() const {
				return getset->isWriteOnly();
			}

			V getValue(const DescribedBase* object) const {
				return getset->getValue(object);
			}

			void setValue(DescribedBase* object, const V& value) const {
				getset->setValue(object, value);
			}
			
			/*implement*/ bool equalValues(const DescribedBase* a, const DescribedBase* b) const {
				return getValue(a) == getValue(b);
			}

            virtual int getDataSize(const DescribedBase* instance) const;

			//\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\/

			virtual bool hasStringValue() const;
			virtual std::string getStringValue(const DescribedBase* instance) const;
			virtual bool setStringValue(DescribedBase* instance, const std::string& text) const;
		private:
			virtual void readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const;
			virtual void writeValue(const DescribedBase* instance, XmlElement* element) const;
		};

		// A light-weight convenience class that associates a PropertyDescriptor
		// with a described object to create a "Property"
		class ConstProperty
		{
		protected:
			const PropertyDescriptor* descriptor;
			const DescribedBase* instance;
		public:
			inline ConstProperty():descriptor(0),instance(0) {}
			inline ConstProperty(const PropertyDescriptor& descriptor, const DescribedBase* instance)
				:descriptor(&descriptor),instance(instance)
			{
				RBXASSERT(!instance || descriptor.isMemberOf(instance));
			}

			inline ConstProperty(const ConstProperty& other)
				:descriptor(other.descriptor),instance(other.instance)
			{}

			inline const DescribedBase* getInstance() const { return instance; }
			inline const PropertyDescriptor& getDescriptor() const { return *descriptor; }

			inline ConstProperty& operator =(const ConstProperty& other)
			{
				this->descriptor = other.descriptor;
				this->instance = other.instance;
				return *this;
			}

			inline bool operator ==(const ConstProperty& other) const
			{
				return (this->descriptor == other.descriptor) && (this->instance == other.instance);
			}

			inline const RBX::Name& getName() const { 
				return descriptor->name; 
			}

			template<typename V>
			inline bool isValueType() const
			{
				return descriptor->type==Type::singleton<V>();
			}
			template<typename V>
			inline V getValue() const
			{
				RBXASSERT(isValueType<V>());
				return static_cast<const TypedPropertyDescriptor<V>*>(descriptor)->getValue(instance);
			}

			inline bool hasStringValue() const
			{
				return descriptor->hasStringValue();
			}
			inline std::string getStringValue() const
			{
				return descriptor->getStringValue(instance);
			}

			inline XmlElement* write() const
			{
				return descriptor->write(instance);
			}
		};


		// A light-weight convenience class that associates a PropertyDescriptor
		// with a described object to create a "Property"
		class Property : public ConstProperty
		{
		public:
			inline Property(const PropertyDescriptor& descriptor, DescribedBase* instance)
				:ConstProperty(descriptor, instance)
			{}
			inline Property(const Property& other)
				:ConstProperty(*other.descriptor, other.instance)
			{}
			inline Property& operator =(const Property& other)
			{
				this->descriptor = other.descriptor;
				this->instance = other.instance;
				return *this;
			}
			inline bool operator ==(const Property& other) const
			{
				return this->descriptor==other.descriptor && this->instance==other.instance;
			}

			inline bool operator !=(const Property& other) const
			{
				return this->descriptor!=other.descriptor || this->instance!=other.instance;
			}

			DescribedBase* getInstance() const { return const_cast<DescribedBase*>(instance); }

			template<typename V>
			inline void setValue(const V& value)
			{
				RBXASSERT(isValueType<V>());
				static_cast<const TypedPropertyDescriptor<V>*>(descriptor)->setValue(const_cast<DescribedBase*>(instance), value);
			}

			inline bool setStringValue(const std::string& text)
			{
				return descriptor->setStringValue(const_cast<DescribedBase*>(instance), text);
			}

			inline void read(const XmlElement* element, RBX::IReferenceBinder& binder)
			{
				descriptor->read(const_cast<DescribedBase*>(instance), element, binder);
			}

		};

		std::size_t hash_value(const ConstProperty& prop);

		// Interface
		// maps enums to an index (Used by UIs like a property grid)
		class RBXInterface EnumPropertyDescriptor : public PropertyDescriptor {
		public:
			const EnumDescriptor& enumDescriptor;
			virtual size_t getIndexValue(const DescribedBase* instance) const = 0;
			virtual bool setIndexValue(DescribedBase* instance, size_t value) const = 0;		// throws an exception if value is illegal
			virtual int getEnumValue(const DescribedBase* instance) const = 0;
			virtual bool setEnumValue(DescribedBase* instance, int index) const = 0;
			virtual const EnumDescriptor::Item* getEnumItem(const DescribedBase* instance) const = 0;
			bool setEnumItem(DescribedBase* instance, const EnumDescriptor::Item& item) const { 
				if (item.owner!=enumDescriptor)
					return false;
				return setEnumValue(instance, item.value); 
			}
            virtual int getDataSize(const DescribedBase* instance) const
            { return sizeof(int); }
		protected:
			EnumPropertyDescriptor(ClassDescriptor& classDescriptor, const EnumDescriptor& enumDescriptor, const char* name, const char* category, Attributes flags = STANDARD, Security::Permissions security=Security::None)
				:PropertyDescriptor(classDescriptor, enumDescriptor, name, category, flags, security, true)
				,enumDescriptor(enumDescriptor)
			{}
		};

		class RBXBaseClass RefPropertyDescriptor : public PropertyDescriptor {

		private:
			typedef PropertyDescriptor Super;

		public:
			virtual DescribedBase* getRefValue(const DescribedBase* instance) const = 0;
			virtual void setRefValue(DescribedBase* instance, DescribedBase* value) const = 0;
			virtual void setRefValueUnsafe(DescribedBase* instance, DescribedBase* value) const = 0;
			
			RefPropertyDescriptor(ClassDescriptor& classDescriptor, const Type& type, const char* name, const char* category, Attributes flags = STANDARD, Security::Permissions security=Security::None)
				:PropertyDescriptor(classDescriptor, type, name, category, flags, security)
			{}

            virtual int getDataSize(const DescribedBase* instance) const
            { return 0; }

			bool hasStringValue() const {
				return false;
			}
			std::string getStringValue(const DescribedBase* instance) const{
				return Super::getStringValue(instance);
			}
			bool setStringValue(DescribedBase* instance, const std::string& text) const {
				return Super::setStringValue(instance, text);
			}


            static bool isRefPropertyDescriptor(const Reflection::Type& type)
            {
                static const RBX::Name& name = RBX::Name::lookup("Object");
                return (type.name == name);
            }

			static bool isRefPropertyDescriptor(const PropertyDescriptor& descriptor)
			{
				// See RefType in reflection.h
				bool result = isRefPropertyDescriptor(descriptor.type);
				RBXASSERT(result == (0 != dynamic_cast<const Reflection::RefPropertyDescriptor*>(&descriptor)));
				return result;
			}
		};
		
		// A very useful class for binding Instance members to PropertyDescriptors
		template<typename V, Mutability mutability = READWRITE>
		class BoundProp : public Reflection::TypedPropertyDescriptor<V>
		{
			template<class Class>
			class BoundPropGetSet : public TypedPropertyDescriptor<V>::GetSet
			{
				BoundProp& desc;
				V Class::*member;
				typedef void (Class::*ChangedMember)(const Reflection::PropertyDescriptor&);
				ChangedMember changed;
			public:
				BoundPropGetSet(BoundProp& desc, V Class::*member, ChangedMember changed):desc(desc),member(member),changed(changed) {}
				virtual bool isReadOnly() const {
					return mutability == READONLY;
				}
				virtual bool isWriteOnly() const {
					return false;
				}
				virtual V getValue(const Reflection::DescribedBase* object) const {
					const Class* c = static_cast<const Class*>(object);
					return c->*member;
				}
				virtual void setValue(Reflection::DescribedBase* object, const V& value) const {
					if (mutability == READONLY)
						throw std::runtime_error("can't set value");

					Class* c = static_cast<Class*>(object);
					if (c->*member != value)
					{
						c->*member = value;
						if (changed)
							(c->*changed)(desc);
						c->raisePropertyChanged(desc);
					}
				}
			};
		public:
			template<class Class>
			BoundProp(const char* name, const char* category, V Class::*member, void (Class::*changed)(const Reflection::PropertyDescriptor&), typename PropertyDescriptor::Attributes flags = PropertyDescriptor::STANDARD, Security::Permissions security = Security::None)
				:Reflection::TypedPropertyDescriptor<V>(Class::classDescriptor(), name, category, std::auto_ptr<typename TypedPropertyDescriptor<V>::GetSet>(), flags, security)
			{
				this->getset.reset(new BoundPropGetSet<Class>(*this, member, changed));
				this->checkFlags();
			}
			template<class Class>
			BoundProp(const char* name, const char* category, V Class::*member, typename PropertyDescriptor::Attributes flags = PropertyDescriptor::STANDARD, Security::Permissions security = Security::None)
				:Reflection::TypedPropertyDescriptor<V>(Class::classDescriptor(), name, category, std::auto_ptr<typename TypedPropertyDescriptor<V>::GetSet>(), flags, security)
			{
				this->getset.reset(new BoundPropGetSet<Class>(*this, member, NULL));
				this->checkFlags();
			}
		};
	}
}
