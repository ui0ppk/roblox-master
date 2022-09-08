
#pragma once

#include "reflection/Property.h"
#include "reflection/Function.h"
#include "reflection/YieldFunction.h"
#include "Reflection/Event.h"
#include "reflection/Callback.h"

#include <vector>
#include "boost/crc.hpp"

namespace RBX
{
    namespace Reflection
	{
		enum ReplicationLevel{
			NEVER_REPLICATE   = 0, //Never replicate this object
			STANDARD_REPLICATE= 1, //Replicate to/from server according to standard rules
			PLAYER_REPLICATE  = 2, //Replicate changes to/from the "player" who owns this object
		} ;

		class ClassDescriptor 
			: public Descriptor
			, public MemberDescriptorContainer<PropertyDescriptor>
			, public MemberDescriptorContainer<EventDescriptor>
			, public MemberDescriptorContainer<FunctionDescriptor>
			, public MemberDescriptorContainer<YieldFunctionDescriptor>
			, public MemberDescriptorContainer<CallbackDescriptor>
		{
		public:
			typedef std::vector<ClassDescriptor*> ClassDescriptors;

			enum Functionality{
				PERSISTENT					= 0x1 + 0x2 + 0x8 + 0x10,   // isPublic, Replicate,			canXmlWrite, isScriptable
				PERSISTENT_PLAYER			= 0x1 + 0x4 + 0x8 + 0x10,   // isPublic, ReplicatePlayer,	canXmlWrite, isScriptable
				PERSISTENT_LOCAL			= 0x1 + 0x0 + 0x8 + 0x10,   // isPublic,					canXmlWrite, isScriptable
				RUNTIME						= 0x1 + 0x2 + 0x0 + 0x10,	// isPublic, Replicate,						 isScriptable
				RUNTIME_PLAYER				= 0x1 + 0x4 + 0x0 + 0x10,	// isPublic, ReplicatePlayer,				 isScriptable
				RUNTIME_LOCAL				= 0x1 + 0x0 + 0x0 + 0x10,	// isPublic,								 isScriptable
				INTERNAL					= 0x1 + 0x2 + 0x0 + 0x0,    // isPublic, Replicate
				INTERNAL_PLAYER				= 0x1 + 0x4 + 0x0 + 0x0,	// isPublic, ReplicatePlayer,
				INTERNAL_LOCAL				= 0x1 + 0x0 + 0x0 + 0x0,    // isPublic
				PERSISTENT_HIDDEN			= 0x1 + 0x2 + 0x8 + 0x0,    // isPublic, Replicate,			canXmlWrite
				PERSISTENT_LOCAL_INTERNAL	= 0x1 + 0x0 + 0x8 + 0x0,    // isPublic,  					canXmlWrite
			};

			struct Attributes : public Descriptor::Attributes
			{
				Functionality flags;

				Attributes(Functionality flags):flags(flags) {}
				static Attributes deprecated(Functionality flags, const ClassDescriptor* preferred);
			};

			const Security::Permissions security;

		private:
			ClassDescriptor();
			static ClassDescriptors& allClasses();

			ClassDescriptors derivedClasses;
			ClassDescriptor* const base;
			const unsigned bReplicateType : 2;
			const unsigned bCanXmlWrite : 1;
			const unsigned bIsScriptable : 1;

			static int count;

        public:
			ClassDescriptor(ClassDescriptor& base, const char* name, Attributes attributes, Security::Permissions security);
			~ClassDescriptor() { count--; }

			const ClassDescriptor* getBase() const { return base; }

			bool isBaseOf(const ClassDescriptor& child) const;
			bool isA(const ClassDescriptor& test) const;

			bool isBaseOf(const char* childName) const;
			bool isA(const char* testName) const;

			inline ReplicationLevel getReplicationLevel() const { return (ReplicationLevel)bReplicateType; }
			inline bool isScriptCreatable() const { return bIsScriptable != 0; }
			inline bool isSerializable() const { return bCanXmlWrite != 0; }

			// The root ClassDescriptor of all other Descriptors
			static ClassDescriptor& rootDescriptor()
			{
				static ClassDescriptor root;
				return root;
			}

			/////////////////////////////////////////////////////////////////////////////////
			// Class enumeration
			static ClassDescriptors::const_iterator all_begin() { 
				return allClasses().begin(); 
			}
			static ClassDescriptors::const_iterator all_end() { 
				return allClasses().end(); 
			}
            static size_t all_size()
            {
                return allClasses().size();
            }
            static unsigned int checksum();
            static unsigned int checksum(const PropertyDescriptor* t, boost::crc_32_type& result);
            static unsigned int checksum(const EventDescriptor* t, boost::crc_32_type& result);
            static unsigned int checksum(const ClassDescriptor* t, boost::crc_32_type& result);
            static unsigned int checksum(const Type* t, boost::crc_32_type& result);

			/////////////////////////////////////////////////////////////////////////////////
			// Derived Class enumeration
			ClassDescriptors::const_iterator derivedClasses_begin() const { 
				return derivedClasses.begin(); 
			}
			ClassDescriptors::const_iterator derivedClasses_end() const { 
				return derivedClasses.end(); 
			}

			/////////////////////////////////////////////////////////////////////////////////
			// Convenience functions for enumerating and querying members of a Class
			PropertyDescriptor* findPropertyDescriptor(const char* name) const
			{
				return MemberDescriptorContainer<PropertyDescriptor>::findDescriptor(name);
			}
			FunctionDescriptor* findFunctionDescriptor(const char* name) const
			{
				return MemberDescriptorContainer<FunctionDescriptor>::findDescriptor(name);
			}
			YieldFunctionDescriptor* findYieldFunctionDescriptor(const char* name) const
			{
				return MemberDescriptorContainer<YieldFunctionDescriptor>::findDescriptor(name);
			}
			EventDescriptor* findEventDescriptor(const char* name) const
			{
				return MemberDescriptorContainer<EventDescriptor>::findDescriptor(name);
			}
			CallbackDescriptor* findCallbackDescriptor(const char* name) const
			{
				return MemberDescriptorContainer<CallbackDescriptor>::findDescriptor(name);
			}

			template<class T>
			typename MemberDescriptorContainer<T>::Collection::const_iterator begin() const {
				return MemberDescriptorContainer<T>::descriptors_begin();
			}
			template<class T>
			typename MemberDescriptorContainer<T>::Collection::const_iterator end() const {
				return MemberDescriptorContainer<T>::descriptors_end();
			}

			bool operator==(const ClassDescriptor& other) const;
			bool operator!=(const ClassDescriptor& other) const;
		};

		// Convenience typedefs:
		typedef MemberDescriptorContainer<PropertyDescriptor>::ConstIterator		ConstPropertyIterator;
		typedef MemberDescriptorContainer<PropertyDescriptor>::Iterator				PropertyIterator;
		typedef MemberDescriptorContainer<FunctionDescriptor>::ConstIterator		FunctionIterator;
		typedef MemberDescriptorContainer<YieldFunctionDescriptor>::ConstIterator   YieldFunctionIterator;
		typedef MemberDescriptorContainer<EventDescriptor>::ConstIterator			ConstSignalIterator;
		typedef MemberDescriptorContainer<EventDescriptor>::Iterator				SignalIterator;
		typedef MemberDescriptorContainer<CallbackDescriptor>::Iterator             CallbackIterator;

		// The base class of any class that supports Reflection
		class RBXBaseClass DescribedBase
			: public EventSource
			, public boost::enable_shared_from_this<DescribedBase>
		{
		protected:
			// Each instance has a reference to it's most-specific ClassDescriptor:
			const ClassDescriptor* descriptor;
			boost::scoped_ptr<std::string> xmlId;

		public:
			// The ClassDescriptor for this base class
			static ClassDescriptor& classDescriptor()
			{
				return ClassDescriptor::rootDescriptor();
			}
			
			DescribedBase()
			{
				Descriptor::lockedDown = true;	// See Descriptor::checkLockedDown() for an explanation

				// By default, each DescribedBase has a null ClassDescriptor
				this->descriptor = &classDescriptor();
			}
            
            virtual ~DescribedBase()
            {
            }

			inline const ClassDescriptor& getDescriptor() const { return *descriptor; };

			template<class T>
			inline bool isA() const
			{
				return getDescriptor().isA(T::classDescriptor());
			}

			template<class T>
			static inline bool isA(const DescribedBase* instance)
			{
				return instance ? instance->getDescriptor().isA(T::classDescriptor()) : false;
			}

			// This function is slower than the others
			bool isA(std::string className)
			{
				return getDescriptor().isA(className.c_str());
			}

			// Regular dynamic_casts are very slow. These faster versions uses our reflection framework to determine type then uses static_cast for speed.
			// Use these functions to replace dynamic_casts for classes that derives from DescribedCreatable or DescribedNonCreatable.
			template<class T>
			inline T* fastDynamicCast()
			{
				return (getDescriptor().isA(T::classDescriptor())) ? static_cast<T*>(this) : NULL;
			}

			template<class T>
			inline const T* fastDynamicCast() const
			{
				return (getDescriptor().isA(T::classDescriptor())) ? static_cast<const T*>(this) : NULL;
			}

			template<class T>
			static inline T* fastDynamicCast(DescribedBase* instance)
			{
				return (instance && instance->getDescriptor().isA(T::classDescriptor())) ? static_cast<T*>(instance) : NULL;
			}

			template<class T>
			static inline const T* fastDynamicCast(const DescribedBase* instance)
			{
				return (instance && instance->getDescriptor().isA(T::classDescriptor())) ? static_cast<const T*>(instance) : NULL;
			}

			// This function replaces shared_dynamic_cast for classes that derives from DescribedCreatable or DescribedNonCreatable.
			template<class T, class U>
			static inline shared_ptr<T> fastSharedDynamicCast(const shared_ptr<U>& instance)
			{
				return isA<T>(instance.get()) ? shared_static_cast<T>(instance) : shared_ptr<T>();
			}


			/////////////////////////////////////////////////////////////////////////////////
			// Convenience functions for getting members of a described object
			PropertyDescriptor* findPropertyDescriptor(const char* name)
			{
				return getDescriptor().MemberDescriptorContainer<PropertyDescriptor>::findDescriptor(name);
			}
			ConstPropertyIterator properties_begin() const {
				return getDescriptor().MemberDescriptorContainer<PropertyDescriptor>::members_begin(this);
			}
			ConstPropertyIterator properties_end() const {
				return getDescriptor().MemberDescriptorContainer<PropertyDescriptor>::members_end(this);
			}
			PropertyIterator properties_begin() {
				return getDescriptor().MemberDescriptorContainer<PropertyDescriptor>::members_begin(this);
			}
			PropertyIterator properties_end() {
				return getDescriptor().MemberDescriptorContainer<PropertyDescriptor>::members_end(this);
			}

			FunctionDescriptor* findFunctionDescriptor(const char* name)
			{
				return getDescriptor().MemberDescriptorContainer<FunctionDescriptor>::findDescriptor(name);
			}
			FunctionIterator functions_begin() const {
				return getDescriptor().MemberDescriptorContainer<FunctionDescriptor>::members_begin(this);
			}
			FunctionIterator functions_end() const {
				return getDescriptor().MemberDescriptorContainer<FunctionDescriptor>::members_end(this);
			}

			YieldFunctionDescriptor* findYieldFunctionDescriptor(const char* name) const
			{
				return getDescriptor().MemberDescriptorContainer<YieldFunctionDescriptor>::findDescriptor(name);
			}

			YieldFunctionIterator yield_functions_begin() const {
				return getDescriptor().MemberDescriptorContainer<YieldFunctionDescriptor>::members_begin(this);
			}
			YieldFunctionIterator yield_functions_end() const {
				return getDescriptor().MemberDescriptorContainer<YieldFunctionDescriptor>::members_end(this);
			}

			CallbackDescriptor* findCallbackDescriptor(const char* name)
			{
				return getDescriptor().MemberDescriptorContainer<CallbackDescriptor>::findDescriptor(name);
			}
			CallbackIterator callbacks_begin() {
				return getDescriptor().MemberDescriptorContainer<CallbackDescriptor>::members_begin(this);
			}
			CallbackIterator callbacks_end() {
				return getDescriptor().MemberDescriptorContainer<CallbackDescriptor>::members_end(this);
			}

			EventDescriptor* findSignalDescriptor(const char* name) const
			{
				return getDescriptor().MemberDescriptorContainer<EventDescriptor>::findDescriptor(name);
			}

			ConstSignalIterator signals_begin() const {
				return getDescriptor().MemberDescriptorContainer<EventDescriptor>::members_begin(this);
			}
			ConstSignalIterator signals_end() const {
				return getDescriptor().MemberDescriptorContainer<EventDescriptor>::members_end(this);
			}
			SignalIterator signals_begin() {
				return getDescriptor().MemberDescriptorContainer<EventDescriptor>::members_begin(this);
			}
			SignalIterator signals_end() {
				return getDescriptor().MemberDescriptorContainer<EventDescriptor>::members_end(this);
			}

			const std::string* getXmlId() const {
				return xmlId.get();
			}

			void setXmlId(const std::string& newId) {
				if (!xmlId)
				{
					xmlId.reset(new std::string(newId));
				}
				else
				{
					*xmlId = newId;
				}
			}
            
            virtual const RBX::Name& getClassName() const = 0;
		};
	}
}
