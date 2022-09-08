#pragma once
#include "reflection/property.h"
#include "reflection/object.h"
#include "reflection/enumconverter.h"
#include "reflection/Type.h"
#include "rbx/make_shared.h"
#include <boost/static_assert.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>

#if defined RBX_PLATFORM_IOS
#define NULL_FUNCTION_PTR typeof(NULL)
#else
#define NULL_FUNCTION_PTR int
#endif
#define _PRISM_PYRAMID_

#ifdef RBX_RCC_SECURITY
// data_seg  = init rw
// const_seg = init ro
// bss_seg   =      rw
#define REFLECTION_BEGIN() __pragma(data_seg(".lua")) __pragma(const_seg(".lua")) __pragma(bss_seg(".lua"))
#define REFLECTION_END() __pragma(data_seg()) __pragma(const_seg()) __pragma(bss_seg())
#else
#define REFLECTION_BEGIN()
#define REFLECTION_END()
#endif

namespace RBX
{
	namespace Reflection
	{
		// This class is designed to prevent clients of the library
		// from forgetting to initialize their class descriptors
		template<class Class>
		class ClassRegistrar : boost::noncopyable
		{
			int x;
			ClassRegistrar(int i):x(i)
			{
				// This call registers the class descriptor 
				// in the reflection database
				Class::classDescriptor();
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
			static ClassRegistrar registrar;	
		};
		// A CRTP class for implementing reflection. Must be a descendant of DescribedBase
		template<
			class Class, 
				const char* const& sClassName, 
			class BaseClass = DescribedBase, 
				ClassDescriptor::Functionality functionality = Reflection::ClassDescriptor::PERSISTENT, 
				Security::Permissions security = Security::None
		>
		class RBXBaseClass Described : public BaseClass
		{
			void forceRegistration()
			{
				// Force linking of ClassRegistrar<Class>, which will force clients
				// of this library to define ClassRegistrar<Class>::registrar in
				// their startup code.
				ClassRegistrar<Class>::registrar.dummy();
			}
		public:

			static ClassDescriptor& classDescriptor()
			{
				static ClassDescriptor describedClassDescriptor(BaseClass::classDescriptor(), sClassName, functionality, security);
				return describedClassDescriptor;
			}
			inline Described() {
				this->descriptor = &classDescriptor();
				forceRegistration();
			}
			template<class Arg0>
			inline Described(Arg0 arg0):BaseClass(arg0) {
				this->descriptor = &classDescriptor();
				forceRegistration();
			}
			template<class Arg0, class Arg1>
			inline Described(Arg0 arg0, Arg1 arg1):BaseClass(arg0, arg1) {
				this->descriptor = &classDescriptor();
				forceRegistration();
			}
			template<class Arg0, class Arg1, class Arg2>
			inline Described(Arg0 arg0, Arg1 arg1, Arg2 arg2):BaseClass(arg0, arg1, arg2) {
				this->descriptor = &classDescriptor();
				forceRegistration();
			}
			template<class Arg0, class Arg1, class Arg2, class Arg3>
			inline Described(Arg0 arg0, Arg1 arg1, Arg2 arg2, Arg3 arg3):BaseClass(arg0, arg1, arg2, arg3) {
				this->descriptor = &classDescriptor();
				forceRegistration();
			}
		};



		// Handy macro for registering a class
#define RBX_REGISTER_CLASS(Class)		template<> RBX::Reflection::ClassRegistrar<Class> RBX::Reflection::ClassRegistrar<Class>::registrar(0)

		// This CRTP class puts a property declaration all together. 
		// It binds a PropertyDescriptor with getter/setter functions.
		template<class Class, typename V>
		class PropDescriptor : public TypedPropertyDescriptor<V>
		{
			template<typename Get, typename Set>
			class GetSetImpl : public Reflection::TypedPropertyDescriptor<V>::GetSet
			{
				Get get;
				Set set;
			public:
				GetSetImpl(Get get, Set set):get(get),set(set)
				{}

				/*implement*/ bool isReadOnly() const { return false; }
				/*implement*/ bool isWriteOnly() const { return false; }
				/*implement*/ V getValue(const DescribedBase* object) const
				{
					const Class* c = boost::polymorphic_downcast<const Class*>(object);
					return (c->*get)();
				}
				/*implement*/ void setValue(DescribedBase* object, const V& value) const
				{
					Class* c = boost::polymorphic_downcast<Class*>(object);
					(c->*set)(value);
				}
			};

			template<typename Get>
			class GetImpl : public Reflection::TypedPropertyDescriptor<V>::GetSet
			{
				Get get;
			public:
				GetImpl(Get get):get(get)
				{}

				/*implement*/ bool isReadOnly() const { return true; }
				/*implement*/ bool isWriteOnly() const { return false; }
				/*implement*/ V getValue(const DescribedBase* object) const
				{
					const Class* c = boost::polymorphic_downcast<const Class*>(object);
					return (c->*get)();
				}
				/*implement*/ void setValue(DescribedBase* object, const V& value) const
				{
					throw std::runtime_error("can't set value");
				}
			};

			template<typename Set>
			class SetImpl : public Reflection::TypedPropertyDescriptor<V>::GetSet
			{
				Set set;
			public:
				SetImpl(Set set):set(set)
				{}

				/*implement*/ bool isReadOnly() const { return false; }
				/*implement*/ bool isWriteOnly() const { return true; }
				/*implement*/ V getValue(const DescribedBase* object) const
				{
					throw std::runtime_error("can't get value");
				}
				/*implement*/ void setValue(DescribedBase* object, const V& value) const
				{
					Class* c = boost::polymorphic_downcast<Class*>(object);
					(c->*set)(value);
				}
			};

		public:
			template<typename Get, typename Set>
			PropDescriptor(const char* name, const char* category, Get get, Set set, PropertyDescriptor::Attributes flags = PropertyDescriptor::Attributes(), Security::Permissions security = Security::None)
				:TypedPropertyDescriptor<V>(Class::classDescriptor(), name, category, getset<Get, Set>(get, set), flags, security)
			{
			}

			template<typename Get, typename Set>
			static std::auto_ptr< typename Reflection::TypedPropertyDescriptor<V>::GetSet > getset(Get get, Set set)
			{
				return std::auto_ptr< typename Reflection::TypedPropertyDescriptor<V>::GetSet >(new GetSetImpl<Get, Set>(get, set));
			}
            
            // Partial specialization for read-only case
            template<typename Get, typename Set>
            static std::auto_ptr< typename Reflection::TypedPropertyDescriptor<V>::GetSet > getset(Get get, NULL_FUNCTION_PTR set)
            {
                return std::auto_ptr< typename Reflection::TypedPropertyDescriptor<V>::GetSet >(new GetImpl<Get>(get));
            }
            // Partial specialization for write-only case
            template<typename Get, typename Set>
            static std::auto_ptr< typename Reflection::TypedPropertyDescriptor<V>::GetSet > getset(NULL_FUNCTION_PTR get, Set set)
            {
                return std::auto_ptr< typename Reflection::TypedPropertyDescriptor<V>::GetSet >(new SetImpl<Set>(set));
            }
		};

		// This CRTP class puts an enum property declaration all together. 
		// It binds a PropertyDescriptor with getter/setter functions.
		// TODO: Refactor: This duplicates code it TypedPropertyDescriptor
		template<class Class, typename V>
		class EnumPropDescriptor : public EnumPropertyDescriptor
		{
			std::auto_ptr<typename TypedPropertyDescriptor<V>::GetSet> getset;
			const EnumDesc<V>& enumDesc;
		public:
			template<typename Get, typename Set>
			EnumPropDescriptor(const char* name, const char* category, Get get, Set set, PropertyDescriptor::Attributes flags = PropertyDescriptor::Attributes(), Security::Permissions security=Security::None)
				:EnumPropertyDescriptor(Class::classDescriptor(), EnumDesc<V>::singleton(), name, category, flags, security)
				,getset(PropDescriptor<Class, V>::template getset<Get, Set>(get, set))
				,enumDesc(EnumDesc<V>::singleton())
			{
				this->checkFlags();
			}

			virtual bool isReadOnly() const {
				return getset->isReadOnly();
			}

			virtual bool isWriteOnly() const {
				return getset->isWriteOnly();
			}

			/*implement*/ void getVariant(const DescribedBase* instance, Variant& value) const 
			{
				value = getEnumValue(instance);
			}
			/*implement*/ void setVariant(DescribedBase* instance, const Variant& value) const 
			{
				setEnumValue(instance, value.get<int>());
			}
			/*implement*/ void copyValue(const DescribedBase* source, DescribedBase* destination) const
			{
				setValue(destination, getValue(source));
			}

			V getValue(const DescribedBase* object) const {
				return getset->getValue(object);
			}

			void setValue(DescribedBase* object, V value) const {
				getset->setValue(object, value);
			}

			/*implement*/ bool equalValues(const DescribedBase* a, const DescribedBase* b) const {
				return getValue(a) == getValue(b);
			}

			/*implement*/ const EnumDescriptor::Item* getEnumItem(const DescribedBase* instance) const
			{
				const EnumDescriptor::Item* result = enumDesc.convertToItem(getValue(instance));
				return result;
			}

			/*implement*/ int getEnumValue(const DescribedBase* instance) const
			{
				return (int) getValue(instance);
			}
			/*implement*/ bool setEnumValue(DescribedBase* instance, int intValue) const
			{
				if (enumDesc.isValue(intValue))
				{
					setValue(instance, (V) intValue);
					return true;
				}
				else
					return false;
			}
			virtual size_t getIndexValue(const DescribedBase* instance) const
			{
				return enumDesc.convertToIndex(getValue(instance));
			}
			virtual bool setIndexValue(DescribedBase* instance, size_t index) const
			{
				V value;
				if (enumDesc.convertToValue(index, value))
				{
					setValue(instance, value);
					return true;
				}
				else
					return false;
			}

			bool setIntValue(DescribedBase* instance, int intValue) const
			{
				V v;
				if (!enumDesc.mapIntValue(intValue, v))
					return false;
				setValue(instance, v);
				return true;
			}

			// PropertyDescriptor implementation:
			virtual bool hasStringValue() const {
				return true;
			}
			virtual std::string getStringValue(const DescribedBase* instance) const
			{
				return enumDesc.convertToString(getValue(instance));
			}
			virtual bool setStringValue(DescribedBase* instance, const std::string& text) const
			{
				V value;
				if (enumDesc.convertToValue(text.c_str(), value))
				{
					setValue(instance, value);
					return true;
				}
				else
					return false;
			}
			// An alternate, more efficient version of setStringValue
			virtual bool setStringValue(DescribedBase* instance, const RBX::Name& name) const
			{
				V value;
				if (enumDesc.convertToValue(name, value))
				{
					setValue(instance, value);
					return true;
				}
				else
					return false;
			}

			virtual void readValue(DescribedBase* instance, const XmlElement* element, RBX::IReferenceBinder& binder) const
			{
				if (!element->isXsiNil()) {

					int value;
					if (element->getValue(value))
					{
						if (setIntValue(instance, value))
							return;
					}

					if (element->isValueType<std::string>())
					{
						// Legacy code to handle files older than 10/29/05
						// TODO: Opt: Remove this legacy code sometime? It slows text XML down a bit
						std::string sValue;
						if (element->getValue(sValue))
						{
							V e;
							if (enumDesc.convertToValue(sValue.c_str(), e))
							{
								setValue(instance, e);
								return;
							}
							if (sValue.size()==0)
							{
								if (setIndexValue(instance, 0))
									return;
							}
						}
					}

					// TODO: throw error?
					RBXASSERT(false);
				}
			}
			virtual void writeValue(const DescribedBase* instance, XmlElement* element) const
			{
				element->setValue(static_cast<int>(getValue(instance)));
			}
		};

		// Helper class
		template<typename T>
		class RefType : public Type
		{
		public:
			static const Type& singleton()
			{
				static RefType type("Object");
				return type;
			}

		private:
			RefType(const char* name)
				:Type(name, "Ref", (T*)NULL)
			{}
		};

		template<class Class, typename RefClass>
		class RefPropDescriptor
			: public RefPropertyDescriptor
			, public IIDREF
		{
			std::auto_ptr<typename TypedPropertyDescriptor<RefClass*>::GetSet> getset;
		public:
			template<typename Get, typename Set>
			RefPropDescriptor(const char* name, const char* category, Get get, Set set, PropertyDescriptor::Attributes attributes = PropertyDescriptor::Attributes(), Security::Permissions security=Security::None)
				:RefPropertyDescriptor(
				Class::classDescriptor(), 
				RefType<RefClass*>::singleton(), 
				name, category, 
				attributes, security
				)
				,getset(PropDescriptor<Class, RefClass*>::template getset<Get, Set>(get, set))
			{
				this->checkFlags();
			}

			virtual bool isReadOnly() const {
				return getset->isReadOnly();
			}

			virtual bool isWriteOnly() const {
				return getset->isWriteOnly();
			}

			/*implement*/ void getVariant(const DescribedBase* instance, Variant& value) const 
			{
				shared_ptr<Reflection::DescribedBase> ref = shared_from(getValue(instance));
				value = ref;
			}
			/*implement*/ void setVariant(DescribedBase* instance, const Variant& value) const 
			{
				shared_ptr<Reflection::DescribedBase> ref = value.get<shared_ptr<Reflection::DescribedBase> >();
				setRefValue(instance, ref.get());
			}
			/*implement*/ void copyValue(const DescribedBase* source, DescribedBase* destination) const
			{
				setValue(destination, getValue(source));
			}

			RefClass* getValue(const DescribedBase* object) const {
				return getset->getValue(object);
			}

			void setValue(DescribedBase* object, RefClass* value) const {
				getset->setValue(object, value);
			}

			/*implement*/ bool equalValues(const DescribedBase* a, const DescribedBase* b) const {
				return getValue(a) == getValue(b);
			}

			/*implement*/ DescribedBase* getRefValue(const DescribedBase* instance) const {
				return getValue(instance);
			}
			/*implement*/ void setRefValue(DescribedBase* instance, DescribedBase* value) const {
				// if value!=NULL then ensure it is the proper type. If value==NULL then it is OK
				RefClass* v = value!=NULL ? boost::polymorphic_cast<RefClass*>(value) : NULL;
				setValue(instance, v);
			}
			/*implement*/ void setRefValueUnsafe(DescribedBase* instance, DescribedBase* value) const {
				setValue(instance, boost::polymorphic_downcast<RefClass*>(value));
			}

			void readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
			{
				binder.announceIDREF(element, instance, this);
			}
			void writeValue(const DescribedBase* instance, XmlElement* element) const
			{
				element->setValue(InstanceHandle(getValue(instance)));
			}

			/*implement*/ void assignIDREF(DescribedBase* propertyOwner, const InstanceHandle& handle) const
			{
				// We know (assume) that this is the right type. If not, then the file is corrupt!
				shared_ptr<DescribedBase> t = handle.getTarget();
				RefClass* r = boost::polymorphic_downcast<RefClass*>(t.get());
				setValue(propertyOwner, r);
			}
		};

		template <class Class>
		class FuncDesc : public FunctionDescriptor
		{
		protected:
			FuncDesc(const char* name, Security::Permissions security, Attributes attributes)
				:FunctionDescriptor(Class::classDescriptor(), name, security, attributes)
			{
			}
		};

		class ArgHelper
		{
			template <int index, class T>
			static bool try_integral(FunctionDescriptor::Arguments& arguments, bool& arg)
			{
				return arguments.getBool(index, arg);
			}

			template <int index, class T>
			static bool try_integral(FunctionDescriptor::Arguments& arguments, int& arg)
			{
				long a;
				if (!arguments.getLong(index, a)) 
					return false;
				arg = (int) a;
				return true;
			}

			template <int index, class T>
			static bool try_integral(FunctionDescriptor::Arguments& arguments, long& arg)
			{
				return arguments.getLong(index, arg);
			}

			template <int index, class T>
			static inline bool try_integral(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_integral<T> >::type* dummy = 0)
			{
				return false;
			}

			template <int index, class T>
			static bool try_floating_point(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::enable_if<boost::is_floating_point<T> >::type* dummy = 0)
			{
				double a;
				if (!arguments.getDouble(index, a)) 
					return false;
				arg = a;
				return true;
			}

			template <int index, class T>
			static inline bool try_floating_point(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_floating_point<T> >::type* dummy = 0)
			{
				return false;
			}

			template <int index, class T>
			static bool try_string(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::enable_if<boost::is_same<T, std::string> >::type* dummy = 0)
			{
				return arguments.getString(index, arg);
			}

			template <int index, class T>
			static inline bool try_string(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_same<T, std::string> >::type* dummy = 0)
			{
				return false;
			}

			template <int index, class T>
			static bool try_Vector3int16(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::enable_if<boost::is_same<T, Vector3int16> >::type* dummy = 0)
			{
				return arguments.getVector3int16(index, arg);
			}

			template <int index, class T>
			static inline bool try_Vector3int16(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_same<T, Vector3int16> >::type* dummy = 0)
			{
				return false;
			}

			template <int index, class T>
			static bool try_Region3int16(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::enable_if<boost::is_same<T, Region3int16> >::type* dummy = 0)
			{
				return arguments.getRegion3int16(index, arg);
			}

			template <int index, class T>
			static inline bool try_Region3int16(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_same<T, Region3int16> >::type* dummy = 0)
			{
				return false;
			}

			template <int index, class T>
			static bool try_Vector3(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::enable_if<boost::is_same<T, Vector3> >::type* dummy = 0)
			{
				return arguments.getVector3(index, arg);
			}

			template <int index, class T>
			static inline bool try_Vector3(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_same<T, Vector3> >::type* dummy = 0)
			{
				return false;
			}

			template <int index, class T>
			static bool try_Region3(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::enable_if<boost::is_same<T, Region3> >::type* dummy = 0)
			{
				return arguments.getRegion3(index, arg);
			}

			template <int index, class T>
			static inline bool try_Region3(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_same<T, Region3> >::type* dummy = 0)
			{
				return false;
			}

			template <int index, class T>
			static bool try_Rect(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::enable_if<boost::is_same<T, Rect2D> >::type* dummy = 0)
			{
				return arguments.getRect(index, arg);
			}

			template <int index, class T>
			static inline bool try_Rect(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_same<T, Rect2D> >::type* dummy = 0)
			{
				return false;
			}

			template <int index, class T>
			static bool try_object(FunctionDescriptor::Arguments& arguments, shared_ptr<T>& arg, typename boost::enable_if< boost::is_base_of<DescribedBase, T> >::type* dummy = 0)
			{
				shared_ptr<DescribedBase> instance;
				if (!arguments.getObject(index, instance))
					return false;
				arg = shared_static_cast<T>(instance);
				return true;
			}

			template <int index, class T>
			static inline bool try_object(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_convertible<T, shared_ptr<DescribedBase> > >::type* dummy = 0)
			{
				return false;
			}

			template <int index, class T>
			static bool try_enum(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::enable_if<boost::is_enum<T> >::type* dummy = 0)
			{
				int a;
				if (!arguments.getEnum(index, EnumDesc<T>::singleton(), a)) 
					return false;
				arg = (T) a;
				return true;
			}

			template <int index, class T>
			static inline bool try_enum(FunctionDescriptor::Arguments& arguments, T& arg, typename boost::disable_if<boost::is_enum<T> >::type* dummy = 0)
			{
				return false;
			}

		public:
			template <typename T, int index>
			static T getArg(FunctionDescriptor::Arguments& arguments, const scoped_ptr<T>& defaultArg, typename boost::disable_if<boost::is_same<T, shared_ptr<const Tuple> > >::type* dummy = 0)
			{
				// Use default if no argument exists
				if (arguments.size() >= index)
				{
					// Try using direct argument access
					// Good compilers will nicely no-opt all
					// but one of these calls:
					T arg;
					if (try_integral<index, T>(arguments, arg))
						return arg;
					if (try_floating_point<index, T>(arguments, arg))
						return arg;
					if (try_string<index>(arguments, arg))
						return arg;
					if (try_enum<index, T>(arguments, arg))
						return arg;
					if (try_object<index>(arguments, arg))
						return arg;
					if (try_Vector3int16<index>(arguments, arg))
						return arg;
					if (try_Region3int16<index>(arguments, arg))
						return arg;
					if (try_Vector3<index>(arguments, arg))
						return arg;
					if (try_Region3<index>(arguments, arg))
						return arg;
					if (try_Rect<index>(arguments, arg))
						return arg;

					// Fall back to Variant
					Variant v;
					// nil args return false, so we use the default
					// See http://lua-users.org/wiki/TrailingNilParameters
					if (arguments.getVariant(index, v))	
						return v.convert<T>();
				}

				if (defaultArg)
					return *defaultArg;
				else
					throw RBX::runtime_error("Argument %d missing or nil", index);
			}

			// Specialization for Tuple, which takes all arguments >= index
			template <typename T, int index>
			static T getArg(FunctionDescriptor::Arguments& arguments, const scoped_ptr<T>& defaultArg, typename boost::enable_if<boost::is_same<T, shared_ptr<const Tuple> > >::type* dummy = 0)
			{
				if (arguments.size() < index)
					return shared_ptr<const Tuple>();	// no arguments. Return an null Tuple, which is shorthand for empty

				// Create a tuple that is the same size as the number of arguments in the stack
				shared_ptr<Tuple> tuple = rbx::make_shared<Tuple>(arguments.size() - index + 1);
				for (size_t i = 0; i < tuple->values.size(); ++i)
				{
					// Ignore the result. It might return false for nil args, but that is OK
					arguments.getVariant(index + i, tuple->values[i]);
				}
				// According to http://lua-users.org/wiki/TrailingNilParameters we might
				// want to strip trailing nils.
				return tuple;
			}
		};

		template <class Class, typename Signature, int arity = boost::function_traits<Signature>::arity >
		class BoundFuncDesc;

		///////////////////
		// 0 argument	
		template<class Class1, class FunctionPtr1, typename ResultType1>
		class Call0Helper
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue)
			{
				returnValue = (o->*function)();
			}
		};

		// Specialization for void return types:
		template<class Class1, class FunctionPtr1>
		class Call0Helper<Class1,FunctionPtr1,void>
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue)
			{
				(o->*function)();
			}
		};


		// A simple version of FunctionDescriptor for member functions that take no arguments
		template <class Class, typename Signature>
		class BoundFuncDesc<Class, Signature, 0> : public FuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::result_type result_type;
			typedef result_type (Class::*FunctionPtr)();
			FunctionPtr function;

			void declareSignature()
			{
				this->signature.resultType = &Type::singleton<result_type>();
			}


		public:
			BoundFuncDesc(FunctionPtr function, const char* name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
			{
				declareSignature();
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments) const
			{
				Call0Helper<Class,FunctionPtr,result_type>::call(boost::polymorphic_downcast<Class*>(instance),function, arguments.returnValue);
			}
		};


		///////////////////
		// 1 argument		
		template<class Class1, class FunctionPtr1, typename Arg1, typename ResultType1>
		class Call1Helper
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1)
			{
				returnValue = (o->*function)(arg1);
			}
		};

		// Specialization for void return types:
		template<class Class1, class FunctionPtr1, typename Arg1>
		class Call1Helper<Class1,FunctionPtr1, Arg1, void>
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1)
			{
				(o->*function)(arg1);
			}
		};

		// A simple version of FunctionDescriptor for member functions that take 1 arguments
		template <class Class, typename Signature>
		class BoundFuncDesc<Class, Signature, 1> : public FuncDesc<Class>
		{

			typedef typename boost::function_traits<Signature>::result_type result_type;
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef result_type (Class::*FunctionPtr)(Arg1);
			FunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;

			void declareSignature(const char* arg1Name, Variant arg1Default)
			{
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
			}

		public:
			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default1()
			{
				declareSignature(arg1Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments) const
			{
				Call1Helper<Class,FunctionPtr,Arg1,result_type>::call(boost::polymorphic_downcast<Class*>(instance), function, 
					arguments.returnValue,
					ArgHelper::getArg<Arg1, 1>(arguments, default1)
					);
			}
		};

		///////////////////
		// 2 arguments

		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename ResultType1>
		class Call2Helper
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2)
			{
				returnValue = (o->*function)(arg1, arg2 );
			}
		};

		// Specialization for void return types:
		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2>
		class Call2Helper<Class1,FunctionPtr1, Arg1, Arg2, void>
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2)
			{
				(o->*function)(arg1, arg2);
			}
		};


		// A simple version of FunctionDescriptor for member functions that take 3 arguments
		template <class Class, typename Signature>
		class BoundFuncDesc<Class, Signature, 2> : public FuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::result_type result_type;
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef result_type (Class::*FunctionPtr)(Arg1, Arg2);
			FunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
			}

		public:
			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments) const
			{
				Call2Helper<Class,FunctionPtr,Arg1, Arg2,result_type>::call(boost::polymorphic_downcast<Class*>(instance), 
					function,
					arguments.returnValue, 
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2));
			}
		};


		///////////////////
		// 3 arguments

		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3, typename ResultType1>
		class Call3Helper
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
			{
				returnValue = (o->*function)(arg1, arg2, arg3 );
			}
		};

		// Specialization for void return types:
		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3>
		class Call3Helper<Class1,FunctionPtr1, Arg1, Arg2, Arg3, void>
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3)
			{
				(o->*function)(arg1, arg2, arg3);
			}
		};



		// A simple version of FunctionDescriptor for member functions that take 3 arguments
		template <class Class, typename Signature>
		class BoundFuncDesc<Class, Signature, 3> : public FuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::result_type result_type;
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef result_type (Class::*FunctionPtr)(Arg1, Arg2, Arg3);
			FunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
			}

		public:
			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments) const
			{
				Call3Helper<Class,FunctionPtr,Arg1,Arg2,Arg3,result_type>::call(boost::polymorphic_downcast<Class*>(instance),
					function,
					arguments.returnValue, 
					ArgHelper::getArg<Arg1, 1>(arguments, default1),
					ArgHelper::getArg<Arg2, 2>(arguments, default2),
					ArgHelper::getArg<Arg3, 3>(arguments, default3));
			}
		};

		///////////////////
		// 4 arguments

		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename ResultType1>
		class Call4Helper
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
			{
				returnValue = (o->*function)(arg1, arg2, arg3, arg4 );
			}
		};

		// Specialization for void return types:
		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
		class Call4Helper<Class1,FunctionPtr1, Arg1, Arg2, Arg3, Arg4, void>
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4)
			{
				(o->*function)(arg1, arg2, arg3, arg4);
			}
		};

		// A simple version of FunctionDescriptor for member functions that take 4 arguments
		template <class Class, typename Signature>
		class BoundFuncDesc<Class, Signature, 4> : public FuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::result_type result_type;
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef typename boost::function_traits<Signature>::arg4_type Arg4;
			typedef result_type (Class::*FunctionPtr)(Arg1, Arg2, Arg3, Arg4);
			FunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;
			const boost::scoped_ptr<Arg4> default4;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default, const char* arg4Name, Variant arg4Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg3, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
				this->signature.addArgument(RBX::Name::declare(arg4Name), Type::singleton<Arg4>(), arg4Default);
			}

		public:
			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3, arg4Name, default4);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3, arg4Name, default4);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3, arg4Name, default4);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, default4);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments) const
			{
				Call4Helper<Class,FunctionPtr,Arg1,Arg2,Arg3,Arg4,result_type>::call(boost::polymorphic_downcast<Class*>(instance), 
					function,
					arguments.returnValue,
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					ArgHelper::getArg<Arg4, 4>(arguments, default4));

			}
		};

		///////////////////
		// 5 arguments

		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename ResultType1>
		class Call5Helper
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
			{
				returnValue = (o->*function)(arg1, arg2, arg3, arg4, arg5 );
			}
		};

		// Specialization for void return types:
		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
		class Call5Helper<Class1,FunctionPtr1, Arg1, Arg2, Arg3, Arg4, Arg5, void>
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5)
			{
				(o->*function)(arg1, arg2, arg3, arg4, arg5);
			}
		};

		// A simple version of FunctionDescriptor for member functions that take 5 arguments
		template <class Class, typename Signature>
		class BoundFuncDesc<Class, Signature, 5> : public FuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::result_type result_type;
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef typename boost::function_traits<Signature>::arg4_type Arg4;
			typedef typename boost::function_traits<Signature>::arg5_type Arg5;
			typedef result_type (Class::*FunctionPtr)(Arg1, Arg2, Arg3, Arg4, Arg5);
			FunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;
			const boost::scoped_ptr<Arg4> default4;
			const boost::scoped_ptr<Arg5> default5;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default, const char* arg4Name, Variant arg4Default, const char* arg5Name, Variant arg5Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg3, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg4, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
				this->signature.addArgument(RBX::Name::declare(arg4Name), Type::singleton<Arg4>(), arg4Default);
				this->signature.addArgument(RBX::Name::declare(arg5Name), Type::singleton<Arg5>(), arg5Default);
			}

		public:
			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3, arg4Name, default4, arg5Name, default5);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, default4, arg5Name, default5);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, Arg5 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, default5);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5()
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments) const
			{
				Call5Helper<Class,FunctionPtr,Arg1,Arg2,Arg3,Arg4,Arg5,result_type>::call(boost::polymorphic_downcast<Class*>(instance), 
					function, 
					arguments.returnValue,
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					ArgHelper::getArg<Arg4, 4>(arguments, default4), 
					ArgHelper::getArg<Arg5, 5>(arguments, default5));

			}
		};

		///////////////////
		// 6 arguments

		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename ResultType1>
		class Call6Helper
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5, const Arg6& arg6)
			{
				returnValue = (o->*function)(arg1, arg2, arg3, arg4, arg5, arg6);
			}
		};

		// Specialization for void return types:
		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
		class Call6Helper<Class1,FunctionPtr1, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, void>
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5, const Arg6& arg6)
			{
				(o->*function)(arg1, arg2, arg3, arg4, arg5, arg6);
			}
		};

		// A simple version of FunctionDescriptor for member functions that take 5 arguments
		template <class Class, typename Signature>
		class BoundFuncDesc<Class, Signature, 6> : public FuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::result_type result_type;
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef typename boost::function_traits<Signature>::arg4_type Arg4;
			typedef typename boost::function_traits<Signature>::arg5_type Arg5;
			typedef typename boost::function_traits<Signature>::arg6_type Arg6;
			typedef result_type (Class::*FunctionPtr)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6);
			FunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;
			const boost::scoped_ptr<Arg4> default4;
			const boost::scoped_ptr<Arg5> default5;
			const boost::scoped_ptr<Arg6> default6;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default, const char* arg4Name, Variant arg4Default, const char* arg5Name, Variant arg5Default, const char* arg6Name, Variant arg6Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg3, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg4, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg5, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
				this->signature.addArgument(RBX::Name::declare(arg4Name), Type::singleton<Arg4>(), arg4Default);
				this->signature.addArgument(RBX::Name::declare(arg5Name), Type::singleton<Arg5>(), arg5Default);
				this->signature.addArgument(RBX::Name::declare(arg6Name), Type::singleton<Arg6>(), arg6Default);
			}

		public:
			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5, arg6Name, default6);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5, arg6Name, default6);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3, arg4Name, default4, arg5Name, default5, arg6Name, default6);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, default4, arg5Name, default5, arg6Name, default6);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, default5, arg6Name, default6);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, const char* arg6Name, Arg6 default6, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default6(new Arg6(default6))
				,default5()
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, Variant(), arg6Name, default6);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, const char* arg6Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default6()
				,default5()
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, Variant(), arg6Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments) const
			{
				Call6Helper<Class,FunctionPtr,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,result_type>::call(boost::polymorphic_downcast<Class*>(instance), 
					function, 
					arguments.returnValue,
					ArgHelper::getArg<Arg1, 1>(arguments, default1),
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					ArgHelper::getArg<Arg4, 4>(arguments, default4), 
					ArgHelper::getArg<Arg5, 5>(arguments, default5), 
					ArgHelper::getArg<Arg6, 6>(arguments, default6));

			}
		};

		///////////////////
		// 7 arguments

		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename ResultType1>
		class Call7Helper
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7)
			{
				returnValue = (o->*function)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
			}
		};

		// Specialization for void return types:
		template<class Class1, class FunctionPtr1, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
		class Call7Helper<Class1,FunctionPtr1, Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7, void>
		{
		public:
			static void call(Class1* o, FunctionPtr1 function, Variant& returnValue, const Arg1& arg1, const Arg2& arg2, const Arg3& arg3, const Arg4& arg4, const Arg5& arg5, const Arg6& arg6, const Arg7& arg7)
			{
				(o->*function)(arg1, arg2, arg3, arg4, arg5, arg6, arg7);
			}
		};

		// A simple version of FunctionDescriptor for member functions that take 5 arguments
		template <class Class, typename Signature>
		class BoundFuncDesc<Class, Signature, 7> : public FuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::result_type result_type;
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef typename boost::function_traits<Signature>::arg4_type Arg4;
			typedef typename boost::function_traits<Signature>::arg5_type Arg5;
			typedef typename boost::function_traits<Signature>::arg6_type Arg6;
			typedef typename boost::function_traits<Signature>::arg7_type Arg7;
			typedef result_type (Class::*FunctionPtr)(Arg1, Arg2, Arg3, Arg4, Arg5, Arg6, Arg7);
			FunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;
			const boost::scoped_ptr<Arg4> default4;
			const boost::scoped_ptr<Arg5> default5;
			const boost::scoped_ptr<Arg6> default6;
			const boost::scoped_ptr<Arg7> default7;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default, const char* arg4Name, Variant arg4Default, const char* arg5Name, Variant arg5Default, const char* arg6Name, Variant arg6Default, const char* arg7Name, Variant arg7Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg3, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg4, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg5, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg6, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
				this->signature.addArgument(RBX::Name::declare(arg4Name), Type::singleton<Arg4>(), arg4Default);
				this->signature.addArgument(RBX::Name::declare(arg5Name), Type::singleton<Arg5>(), arg5Default);
				this->signature.addArgument(RBX::Name::declare(arg6Name), Type::singleton<Arg6>(), arg6Default);
				this->signature.addArgument(RBX::Name::declare(arg7Name), Type::singleton<Arg7>(), arg7Default);
			}

		public:
			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, const char* arg7Name, Arg7 default7, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default7(new Arg7(default7))
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5, arg6Name, default6, arg7Name, default7);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, const char* arg7Name, Arg7 default7, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default7(new Arg7(default7))
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5, arg6Name, default6, arg7Name, default7);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, const char* arg7Name, Arg7 default7, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default7(new Arg7(default7))
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3, arg4Name, default4, arg5Name, default5, arg6Name, default6, arg7Name, default7);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, const char* arg7Name, Arg7 default7, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default7(new Arg7(default7))
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, default4, arg5Name, default5, arg6Name, default6, arg7Name, default7);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, Arg5 default5, const char* arg6Name, Arg6 default6, const char* arg7Name, Arg7 default7, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default7(new Arg7(default7))
				,default6(new Arg6(default6))
				,default5(new Arg5(default5))
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, default5, arg6Name, default6, arg7Name, default7);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, const char* arg6Name, Arg6 default6, const char* arg7Name, Arg7 default7, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default7(new Arg7(default7))
				,default6(new Arg6(default6))
				,default5()
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, Variant(), arg6Name, default6, arg7Name, default7);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, const char* arg6Name, const char* arg7Name, Arg7 default7, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,default7(new Arg7(default7))
				,default6()
				,default5()
				,default4()
				,default3()
				,default2()
				,default1()
				,function(function)
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, Variant(), arg6Name, Variant(), arg7Name, default7);
			}

			BoundFuncDesc(FunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, const char* arg6Name, const char* arg7Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:FuncDesc<Class>(name, security, attributes)
				,function(function)
				,default7()
				,default6()
				,default5()
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, Variant(), arg6Name, Variant(), arg7Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments) const
			{
				Call7Helper<Class,FunctionPtr,Arg1,Arg2,Arg3,Arg4,Arg5,Arg6,Arg7,result_type>::call(boost::polymorphic_downcast<Class*>(instance), 
					function, 
					arguments.returnValue,
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					ArgHelper::getArg<Arg4, 4>(arguments, default4), 
					ArgHelper::getArg<Arg5, 5>(arguments, default5), 
					ArgHelper::getArg<Arg6, 6>(arguments, default6), 
					ArgHelper::getArg<Arg7, 7>(arguments, default7));

			}
		};


		template <class Class>
		class YieldFuncDesc : public YieldFunctionDescriptor
		{
		protected:
			YieldFuncDesc(const char* name, Security::Permissions security, Attributes attributes)
				:YieldFunctionDescriptor(Class::classDescriptor(), name, security, attributes)
			{
			}
		};

		template<typename ReturnType>
		static void resume_adapter(boost::function<void(Variant)> resumeFunction, ReturnType returnValue)
		{
			Variant value = returnValue;
			resumeFunction(value);
		}


		template <class Class, typename Signature, typename ReturnType = typename boost::function_traits<Signature>::result_type, int arity = boost::function_traits<Signature>::arity >
		class BoundYieldFuncDesc;

		// ReturnType() specialization
		template <class Class, typename Signature, typename ReturnType>
		class BoundYieldFuncDesc<Class, Signature, ReturnType, 0> : public YieldFuncDesc<Class>
		{
			typedef void (Class::*YieldFunctionPtr)(boost::function<void(ReturnType)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			void declareSignature()
			{
				this->signature.resultType = &Type::singleton<ReturnType>();
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
			{
				declareSignature();
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					boost::bind(&resume_adapter<ReturnType>, resumeFunction, _1), errorFunction);
			}
		};



		// void() specialization
		template <class Class, typename Signature>
		class BoundYieldFuncDesc<Class, Signature, void, 0> : public YieldFuncDesc<Class>
		{
			typedef void (Class::*YieldFunctionPtr)(boost::function<void()> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			void declareSignature()
			{
				this->signature.resultType = &Type::singleton<void>();
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
			{
				declareSignature();
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					boost::bind(resumeFunction, Variant()), errorFunction);
			}
		};



		// A simple version of FunctionDescriptor for member functions that take 1 arguments
		template <class Class, typename Signature, typename ReturnType>
		class BoundYieldFuncDesc<Class, Signature, ReturnType, 1> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef void (Class::*YieldFunctionPtr)(Arg1, boost::function<void(ReturnType)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;

			void declareSignature(const char* arg1Name, Variant arg1Default)
			{
				this->signature.resultType = &Type::singleton<ReturnType>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default1()
			{
				declareSignature(arg1Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					boost::bind(&resume_adapter<ReturnType>, resumeFunction, _1), errorFunction);
			}
		};


		// void(Arg1) specialization
		template <class Class, typename Signature>
		class BoundYieldFuncDesc<Class, Signature, void, 1> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef void (Class::*YieldFunctionPtr)(Arg1, boost::function<void(void)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;

			void declareSignature(const char* arg1Name, Variant arg1Default)
			{
				this->signature.resultType = &Type::singleton<void>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default1()
			{
				declareSignature(arg1Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					boost::bind(resumeFunction, Variant()), errorFunction);
			}
		};


		// A simple version of FunctionDescriptor for member functions that take 2 arguments
		template <class Class, typename Signature, typename ReturnType>
		class BoundYieldFuncDesc<Class, Signature, ReturnType, 2> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef void (Class::*YieldFunctionPtr)(Arg1, Arg2, boost::function<void(ReturnType)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<ReturnType>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					boost::bind(&resume_adapter<ReturnType>, resumeFunction, _1), errorFunction);
			}
		};


		// void(Arg1, Arg2) specialization
		template <class Class, typename Signature>
		class BoundYieldFuncDesc<Class, Signature, void, 2> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef void (Class::*YieldFunctionPtr)(Arg1, Arg2, boost::function<void(void)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<void>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					boost::bind(resumeFunction, Variant()), errorFunction);
			}
		};


		// A simple version of FunctionDescriptor for member functions that take 3 arguments
		template <class Class, typename Signature, typename ReturnType>
		class BoundYieldFuncDesc<Class, Signature, ReturnType, 3> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef void (Class::*YieldFunctionPtr)(Arg1, Arg2, Arg3, boost::function<void(ReturnType)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<ReturnType>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					boost::bind(&resume_adapter<ReturnType>, resumeFunction, _1), errorFunction);
			}
		};


		// void(Arg1, Arg2, Arg3) specialization
		template <class Class, typename Signature>
		class BoundYieldFuncDesc<Class, Signature, void, 3> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef void (Class::*YieldFunctionPtr)(Arg1, Arg2, Arg3, boost::function<void(void)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<void>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					boost::bind(resumeFunction, Variant()), errorFunction);
			}
		};

		// A simple version of FunctionDescriptor for member functions that take 4 arguments
		template <class Class, typename Signature, typename ReturnType>
		class BoundYieldFuncDesc<Class, Signature, ReturnType, 4> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef typename boost::function_traits<Signature>::arg4_type Arg4;
			typedef void (Class::*YieldFunctionPtr)(Arg1, Arg2, Arg3, Arg4, boost::function<void(ReturnType)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;
			const boost::scoped_ptr<Arg4> default4;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default, const char* arg4Name, Variant arg4Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<ReturnType>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
				this->signature.addArgument(RBX::Name::declare(arg4Name), Type::singleton<Arg4>(), arg4Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3, arg4Name, default4);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3, arg4Name, default4);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3, arg4Name, default4);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, default4);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					ArgHelper::getArg<Arg4, 4>(arguments, default4), 
					boost::bind(&resume_adapter<ReturnType>, resumeFunction, _1), errorFunction);
			}
		};


		// void(Arg1, Arg2, Arg3, Arg4) specialization
		template <class Class, typename Signature>
		class BoundYieldFuncDesc<Class, Signature, void, 4> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef typename boost::function_traits<Signature>::arg4_type Arg4;
			typedef void (Class::*YieldFunctionPtr)(Arg1, Arg2, Arg3, Arg4, boost::function<void(void)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;
			const boost::scoped_ptr<Arg4> default4;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default, const char* arg4Name, Variant arg4Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<void>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
				this->signature.addArgument(RBX::Name::declare(arg4Name), Type::singleton<Arg4>(), arg4Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3, arg4Name, default4);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3, arg4Name, default4);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3, arg4Name, default4);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Arg4 default4, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4(new Arg4(default4))
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, default4);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant());
			}
			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					ArgHelper::getArg<Arg4, 4>(arguments, default4), 
					boost::bind(resumeFunction, Variant()), errorFunction);
			}
		};

		// A simple version of FunctionDescriptor for member functions that take 4 arguments
		template <class Class, typename Signature, typename ReturnType>
		class BoundYieldFuncDesc<Class, Signature, ReturnType, 5> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef typename boost::function_traits<Signature>::arg4_type Arg4;
			typedef typename boost::function_traits<Signature>::arg5_type Arg5;
			typedef void (Class::*YieldFunctionPtr)(Arg1, Arg2, Arg3, Arg4, Arg5, boost::function<void(ReturnType)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;
			const boost::scoped_ptr<Arg4> default4;
			const boost::scoped_ptr<Arg5> default5;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default, const char* arg4Name, Variant arg4Default, const char* arg5Name, Variant arg5Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<ReturnType>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
				this->signature.addArgument(RBX::Name::declare(arg4Name), Type::singleton<Arg4>(), arg4Default);
				this->signature.addArgument(RBX::Name::declare(arg5Name), Type::singleton<Arg5>(), arg5Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg4 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg4 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3, arg4Name, default4, arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg4 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, default4, arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, Arg4 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5()
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					ArgHelper::getArg<Arg4, 4>(arguments, default4),
					ArgHelper::getArg<Arg5, 5>(arguments, default5), 
					boost::bind(&resume_adapter<ReturnType>, resumeFunction, _1), errorFunction);
			}
		};


		// void(Arg1, Arg2, Arg3, Arg4, Arg5) specialization
		template <class Class, typename Signature>
		class BoundYieldFuncDesc<Class, Signature, void, 5> : public YieldFuncDesc<Class>
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
			typedef typename boost::function_traits<Signature>::arg3_type Arg3;
			typedef typename boost::function_traits<Signature>::arg4_type Arg4;
			typedef typename boost::function_traits<Signature>::arg5_type Arg5;
			typedef void (Class::*YieldFunctionPtr)(Arg1, Arg2, Arg3, Arg4, Arg5, boost::function<void(void)> resumeFunction, boost::function<void(std::string)> errorFunction);
			YieldFunctionPtr function;

			const boost::scoped_ptr<Arg1> default1;
			const boost::scoped_ptr<Arg2> default2;
			const boost::scoped_ptr<Arg3> default3;
			const boost::scoped_ptr<Arg4> default4;
			const boost::scoped_ptr<Arg5> default5;

			void declareSignature(const char* arg1Name, Variant arg1Default, const char* arg2Name, Variant arg2Default, const char* arg3Name, Variant arg3Default, const char* arg4Name, Variant arg4Default, const char* arg5Name, Variant arg5Default)
			{
				BOOST_STATIC_ASSERT((!boost::is_same<Arg1, shared_ptr<const Tuple> >::value));
				BOOST_STATIC_ASSERT((!boost::is_same<Arg2, shared_ptr<const Tuple> >::value));
				this->signature.resultType = &Type::singleton<void>();
				this->signature.addArgument(RBX::Name::declare(arg1Name), Type::singleton<Arg1>(), arg1Default);
				this->signature.addArgument(RBX::Name::declare(arg2Name), Type::singleton<Arg2>(), arg2Default);
				this->signature.addArgument(RBX::Name::declare(arg3Name), Type::singleton<Arg3>(), arg3Default);
				this->signature.addArgument(RBX::Name::declare(arg4Name), Type::singleton<Arg4>(), arg4Default);
				this->signature.addArgument(RBX::Name::declare(arg5Name), Type::singleton<Arg5>(), arg5Default);
			}

		public:
			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, Arg1 default1, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg4 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1(new Arg1(default1))
			{
				declareSignature(arg1Name, default1, arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Arg2 default2, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg4 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2(new Arg2(default2))
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, default2, arg3Name, default3, arg4Name, default4, arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Arg3 default3, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg5 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3(new Arg3(default3))
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, default3, arg4Name, default4, arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Arg4 default4, const char* arg5Name, Arg4 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4(new Arg4(default4))
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, default4, arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, Arg4 default5, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5(new Arg5(default5))
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, default5);
			}

			BoundYieldFuncDesc(YieldFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, const char* arg5Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				:YieldFuncDesc<Class>(name, security, attributes)
				,function(function)
				,default5()
				,default4()
				,default3()
				,default2()
				,default1()
			{
				declareSignature(arg1Name, Variant(), arg2Name, Variant(), arg3Name, Variant(), arg4Name, Variant(), arg5Name, Variant());
			}

			/*implement*/ void execute(Reflection::DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const
			{
				(boost::polymorphic_downcast<Class*>(instance)->*function)(
					ArgHelper::getArg<Arg1, 1>(arguments, default1), 
					ArgHelper::getArg<Arg2, 2>(arguments, default2), 
					ArgHelper::getArg<Arg3, 3>(arguments, default3), 
					ArgHelper::getArg<Arg4, 4>(arguments, default4),
					ArgHelper::getArg<Arg5, 5>(arguments, default5), 
					boost::bind(resumeFunction, Variant()), errorFunction);
			}
		};

		template <class Class, typename Signature, int arity = boost::function_traits<Signature>::arity >
		class CustomBoundFuncDesc;

		template <class Class, typename Signature>
		class CustomBoundFuncDesc<Class, Signature, 0> : public BoundFuncDesc<Class, Signature, 0>
		{
			typedef int (Class::*CustomFunctionPtr)(lua_State*);
			CustomFunctionPtr customFunction;

		public:
			CustomBoundFuncDesc(CustomFunctionPtr function, const char* name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				: BoundFuncDesc<Class, Signature, 0>(NULL, name, security, attributes)
				, customFunction(function)
			{
				this->kind = FunctionDescriptor::Kind_Custom;
			}

			/*implement*/ int executeCustom(DescribedBase* instance, lua_State* state) const
			{
				return (boost::polymorphic_downcast<Class*>(instance)->*customFunction)(state);
			}
		};

		template <class Class, typename Signature>
		class CustomBoundFuncDesc<Class, Signature, 1> : public BoundFuncDesc<Class, Signature, 1>
		{
			typedef int (Class::*CustomFunctionPtr)(lua_State*);
			CustomFunctionPtr customFunction;

		public:
			CustomBoundFuncDesc(CustomFunctionPtr function, const char* name, const char* arg1Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				: BoundFuncDesc<Class, Signature, 1>(NULL, name, arg1Name, security, attributes)
				, customFunction(function)
			{
				this->kind = FunctionDescriptor::Kind_Custom;
			}

			/*implement*/ int executeCustom(DescribedBase* instance, lua_State* state) const
			{
				return (boost::polymorphic_downcast<Class*>(instance)->*customFunction)(state);
			}
		};

		template <class Class, typename Signature>
		class CustomBoundFuncDesc<Class, Signature, 2> : public BoundFuncDesc<Class, Signature, 2>
		{
			typedef int (Class::*CustomFunctionPtr)(lua_State*);
			CustomFunctionPtr customFunction;

		public:
			CustomBoundFuncDesc(CustomFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				: BoundFuncDesc<Class, Signature, 2>(NULL, name, arg1Name, arg2Name, security, attributes)
				, customFunction(function)
			{
				this->kind = FunctionDescriptor::Kind_Custom;
			}

			/*implement*/ int executeCustom(DescribedBase* instance, lua_State* state) const
			{
				return (boost::polymorphic_downcast<Class*>(instance)->*customFunction)(state);
			}
		};

		template <class Class, typename Signature>
		class CustomBoundFuncDesc<Class, Signature, 3> : public BoundFuncDesc<Class, Signature, 3>
		{
			typedef int (Class::*CustomFunctionPtr)(lua_State*);
			CustomFunctionPtr customFunction;

		public:
			CustomBoundFuncDesc(CustomFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				: BoundFuncDesc<Class, Signature, 3>(NULL, name, arg1Name, arg2Name, arg3Name, security, attributes)
				, customFunction(function)
			{
				this->kind = FunctionDescriptor::Kind_Custom;
			}

			/*implement*/ int executeCustom(DescribedBase* instance, lua_State* state) const
			{
				return (boost::polymorphic_downcast<Class*>(instance)->*customFunction)(state);
			}
		};

		template <class Class, typename Signature>
		class CustomBoundFuncDesc<Class, Signature, 4> : public BoundFuncDesc<Class, Signature, 4>
		{
			typedef int (Class::*CustomFunctionPtr)(lua_State*);
			CustomFunctionPtr customFunction;

		public:
			CustomBoundFuncDesc(CustomFunctionPtr function, const char* name, const char* arg1Name, const char* arg2Name, const char* arg3Name, const char* arg4Name, Security::Permissions security, Descriptor::Attributes attributes = Descriptor::Attributes())
				: BoundFuncDesc<Class, Signature, 4>(NULL, name, arg1Name, arg2Name, arg3Name, arg4Name, security, attributes)
				, customFunction(function)
			{
				this->kind = FunctionDescriptor::Kind_Custom;
			}

			/*implement*/ int executeCustom(DescribedBase* instance, lua_State* state) const
			{
				return (boost::polymorphic_downcast<Class*>(instance)->*customFunction)(state);
			}
		};

	}
}

