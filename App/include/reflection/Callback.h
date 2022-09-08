
#pragma once

#include "reflection/type.h"
#include "security/securitycontext.h"
#include "reflection/member.h"
#include "reflection/Type.h"
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>

namespace RBX
{
	namespace Reflection
	{
		class Callback;

		// Base class that describes a Callback
		class RBXBaseClass CallbackDescriptor : public MemberDescriptor
		{
		public:
			typedef Callback ConstMember;
			typedef Callback Member;

		protected:
			SignatureDescriptor signature;
            bool async;

			CallbackDescriptor(ClassDescriptor& classDescriptor, const char* name, Descriptor::Attributes attributes, Security::Permissions security, bool async);
		public:
			inline const SignatureDescriptor& getSignature() const { return signature; }
            bool isAsync() const { return async; }
		};

		class SyncCallbackDescriptor : public CallbackDescriptor
		{
		protected:
			SyncCallbackDescriptor(ClassDescriptor& classDescriptor, const char* name, Descriptor::Attributes attributes, Security::Permissions security);
		public:
			typedef boost::function<shared_ptr<Reflection::Tuple>(shared_ptr<const Reflection::Tuple> args)> GenericFunction;

			// the preferred way to set a generic function:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<GenericFunction> function) const = 0;
			virtual void clearCallback(DescribedBase* object) const = 0;

			// use this function if you don't have a shared_ptr:
			void setGenericCallbackHelper(DescribedBase* object, const GenericFunction& function) const;
		};

		class AsyncCallbackDescriptor : public CallbackDescriptor
		{
		public:
            typedef boost::function<void(shared_ptr<const Reflection::Tuple>)> ResumeFunction;
            typedef boost::function<void(std::string)> ErrorFunction;
            typedef boost::function<void(shared_ptr<const Reflection::Tuple> args, ResumeFunction resumeFunction, ErrorFunction errorFunction)> GenericFunction;

			// the preferred way to set a generic function:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<GenericFunction> function) const = 0;
			virtual void clearCallback(DescribedBase* object) const = 0;

			// use this function if you don't have a shared_ptr:
			void setGenericCallbackHelper(DescribedBase* object, const GenericFunction& function) const;

        protected:
			AsyncCallbackDescriptor(ClassDescriptor& classDescriptor, const char* name, Descriptor::Attributes attributes, Security::Permissions security);

			static void callGenericImpl(shared_ptr<AsyncCallbackDescriptor::GenericFunction> function, shared_ptr<Tuple> args,
                AsyncCallbackDescriptor::ResumeFunction resumeFunction, AsyncCallbackDescriptor::ErrorFunction errorFunction);

            template <typename Class, typename Function, typename Value>
            void setGenericCallbackImpl(DescribedBase* object, Function Class::*member, void (Class::*onChanged)(const Function&), const Value& value) const
            {
                Class* c = static_cast<Class*>(object);
				
				Function oldValue = c->*member;
                c->*member = value;
				if (onChanged)
					(c->*onChanged)(oldValue);
            }
		};


		// A light-weight convenience class that associates a CallbackDescriptor
		// with a described object to create a "Callback"
		class Callback
		{
			const CallbackDescriptor* descriptor;
			DescribedBase* instance;
		public:
			inline Callback(const CallbackDescriptor& descriptor, DescribedBase* instance)
				:descriptor(&descriptor),instance(instance)
			{}

			inline Callback(const Callback& other)
				:descriptor(other.descriptor),instance(other.instance)
			{}

			inline Callback& operator =(const Callback& other)
			{
				this->descriptor = other.descriptor;
				this->instance = other.instance;
				return *this;
			}

			inline const RBX::Name& getName() const { 
				return descriptor->name; 
			}

			inline DescribedBase* getInstance() const 
			{ 
				return instance; 
			}

			inline const CallbackDescriptor& getDescriptor() const 
			{ 
				return *descriptor; 
			}
		};


		// Base class of typed CallbackDescriptors
		template<typename Signature>
		class SyncCallbackDesc : public SyncCallbackDescriptor
		{
		protected:
			typedef typename boost::function<Signature> Function;
			typedef typename boost::function_traits<Signature>::result_type result_type;

			template<typename Result>
			static typename boost::enable_if<boost::is_void<Result>, void>::type 
				callGeneric(shared_ptr<SyncCallbackDescriptor::GenericFunction> function, shared_ptr<Tuple> args)
			{
				(*function)(args);
			}

			template<typename Result>
			static typename boost::disable_if<boost::is_same<shared_ptr<const Tuple>, Result>, Result>::type 
				convertResult(shared_ptr<Reflection::Tuple> result)
			{
				// Extract the first value in the Tuple and return it as the result. Ignore other vales
				if (result->values.size() == 0)
					throw std::runtime_error("Callback did not return a value");
				return result->values[0].convert<Result>();
			}

			template<typename Result>
			static typename boost::enable_if<boost::is_same<shared_ptr<const Tuple>, Result>, Result>::type 
				convertResult(shared_ptr<Reflection::Tuple> result)
			{
				return result;
			}

			template<typename Result>
			static typename boost::disable_if<boost::is_void<Result>, Result>::type 
				callGeneric(shared_ptr<SyncCallbackDescriptor::GenericFunction> function, shared_ptr<Tuple> args)
			{
				shared_ptr<Reflection::Tuple> result = (*function)(args);
				return convertResult<Result>(result);
			}

			class RBXInterface ISetter
			{
			public:
                virtual ~ISetter() {}
				virtual void setCallback(DescribedBase* object, const Function& value) const = 0;
			};
			boost::scoped_ptr<ISetter> setter;

			SyncCallbackDesc(ClassDescriptor& classDescriptor, const char* name, Descriptor::Attributes attributes, Security::Permissions security):
			SyncCallbackDescriptor(classDescriptor, name, attributes, security)
			{}
		public:
			void setCallback(DescribedBase* object, const Function& value) const
			{
				setter->setCallback(object, value);
			}
			void clearCallback(DescribedBase* object) const
			{
				setter->setCallback(object, Function());
			}
		};

		// Specialized class that implements generic bindings and sets the signature
		template <typename Signature, int arity>
		class SyncCallbackDescImpl;

		template<typename Signature>
		class SyncCallbackDescImpl<Signature, 0> : public SyncCallbackDesc<Signature>
		{
			typedef typename SyncCallbackDesc<Signature>::result_type result_type;
			static result_type callGeneric(shared_ptr<SyncCallbackDescriptor::GenericFunction> function)
			{
				shared_ptr<Reflection::Tuple> args(new Tuple());
				return SyncCallbackDesc<Signature>::template callGeneric<result_type>(function, args);
			}

		protected:
			SyncCallbackDescImpl(ClassDescriptor& classDescriptor, const char* name, Descriptor::Attributes attributes, Security::Permissions security)
				:SyncCallbackDesc<Signature>(classDescriptor, name, attributes, security)
			{
				BOOST_STATIC_ASSERT((boost::function_traits<Signature>::arity == 0));
				this->signature.resultType = &Type::singleton<result_type>();
			}
		public:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<SyncCallbackDescriptor::GenericFunction> function) const
			{
				this->setCallback(object, boost::bind(callGeneric, function));
			}
		};

		template<typename Signature>
		class SyncCallbackDescImpl<Signature, 1> : public SyncCallbackDesc<Signature>
		{
			typedef typename SyncCallbackDesc<Signature>::result_type result_type;
			static result_type callGeneric(shared_ptr<SyncCallbackDescriptor::GenericFunction> function,
				// TODO: Use const ref for args and bind with boost::cref?
				typename boost::function_traits<Signature>::arg1_type arg1)
			{
				shared_ptr<Reflection::Tuple> args(new Tuple());
				args->values.push_back(arg1);
				return SyncCallbackDesc<Signature>::template callGeneric<result_type>(function, args);
			}

		protected:
			SyncCallbackDescImpl(ClassDescriptor& classDescriptor, const char* name, const char* arg1name, Descriptor::Attributes attributes, Security::Permissions security)
				:SyncCallbackDesc<Signature>(classDescriptor, name, attributes, security)
			{
				BOOST_STATIC_ASSERT((boost::function_traits<Signature>::arity == 1));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1name), Type::singleton<typename boost::function_traits<Signature>::arg1_type>());
			}
		public:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<SyncCallbackDescriptor::GenericFunction> function) const
			{
				this->setCallback(object, boost::bind(callGeneric, function, _1));
			}
		};

		template<typename Signature>
		class SyncCallbackDescImpl<Signature, 2> : public SyncCallbackDesc<Signature>
		{
			typedef typename SyncCallbackDesc<Signature>::result_type result_type;
			static result_type callGeneric(shared_ptr<SyncCallbackDescriptor::GenericFunction> function,
				typename boost::function_traits<Signature>::arg1_type arg1,
				typename boost::function_traits<Signature>::arg2_type arg2)
			{
				shared_ptr<Reflection::Tuple> args(new Tuple());
				args->values.push_back(arg1);
				args->values.push_back(arg2);
				return SyncCallbackDesc<Signature>::template callGeneric<result_type>(function, args);
			}

		protected:
			SyncCallbackDescImpl(ClassDescriptor& classDescriptor, const char* name, const char* arg1name, const char* arg2name, Descriptor::Attributes attributes, Security::Permissions security)
				:SyncCallbackDesc<Signature>(classDescriptor, name, attributes, security)
			{
				BOOST_STATIC_ASSERT((boost::function_traits<Signature>::arity == 2));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1name), Type::singleton<typename boost::function_traits<Signature>::arg1_type>());
				this->signature.addArgument(RBX::Name::declare(arg2name), Type::singleton<typename boost::function_traits<Signature>::arg2_type>());
			}
		public:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<SyncCallbackDescriptor::GenericFunction> function) const
			{
				this->setCallback(object, boost::bind(callGeneric, function, _1, _2));
			}
		};

		template<typename Signature>
		class SyncCallbackDescImpl<Signature, 3> : public SyncCallbackDesc<Signature>
		{
			typedef typename SyncCallbackDesc<Signature>::result_type result_type;
			static result_type callGeneric(shared_ptr<SyncCallbackDescriptor::GenericFunction> function,
				typename boost::function_traits<Signature>::arg1_type arg1,
				typename boost::function_traits<Signature>::arg2_type arg2,
				typename boost::function_traits<Signature>::arg3_type arg3)
			{
				shared_ptr<Reflection::Tuple> args(new Tuple());
				args->values.push_back(arg1);
				args->values.push_back(arg2);
				args->values.push_back(arg3);
				return SyncCallbackDesc<Signature>::template callGeneric<result_type>(function, args);
			}

		protected:
			SyncCallbackDescImpl(ClassDescriptor& classDescriptor, const char* name, const char* arg1name, const char* arg2name, const char* arg3name, Descriptor::Attributes attributes, Security::Permissions security)
				:SyncCallbackDesc<Signature>(classDescriptor, name, attributes, security)
			{
				BOOST_STATIC_ASSERT((boost::function_traits<Signature>::arity == 3));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1name), Type::singleton<typename boost::function_traits<Signature>::arg1_type>());
				this->signature.addArgument(RBX::Name::declare(arg2name), Type::singleton<typename boost::function_traits<Signature>::arg2_type>());
				this->signature.addArgument(RBX::Name::declare(arg3name), Type::singleton<typename boost::function_traits<Signature>::arg3_type>());
			}
		public:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<SyncCallbackDescriptor::GenericFunction> function) const
			{
				this->setCallback(object, boost::bind(callGeneric, function, _1, _2, _3));
			}
		};

		template<typename Signature>
		class SyncCallbackDescImpl<Signature, 4> : public SyncCallbackDesc<Signature>
		{
			typedef typename SyncCallbackDesc<Signature>::result_type result_type;
			static result_type callGeneric(shared_ptr<SyncCallbackDescriptor::GenericFunction> function,
				typename boost::function_traits<Signature>::arg1_type arg1,
				typename boost::function_traits<Signature>::arg2_type arg2,
				typename boost::function_traits<Signature>::arg3_type arg3,
				typename boost::function_traits<Signature>::arg4_type arg4)
			{
				shared_ptr<Reflection::Tuple> args(new Tuple());
				args->values.push_back(arg1);
				args->values.push_back(arg2);
				args->values.push_back(arg3);
				args->values.push_back(arg4);
				return SyncCallbackDesc<Signature>::template callGeneric<result_type>(function, args);
			}

		protected:
			SyncCallbackDescImpl(ClassDescriptor& classDescriptor, const char* name, const char* arg1name, const char* arg2name, const char* arg3name, const char* arg4name, Descriptor::Attributes attributes, Security::Permissions security)
				:SyncCallbackDesc<Signature>(classDescriptor, name, attributes, security)
			{
				BOOST_STATIC_ASSERT((boost::function_traits<Signature>::arity == 4));
				this->signature.resultType = &Type::singleton<result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1name), Type::singleton<typename boost::function_traits<Signature>::arg1_type>());
				this->signature.addArgument(RBX::Name::declare(arg2name), Type::singleton<typename boost::function_traits<Signature>::arg2_type>());
				this->signature.addArgument(RBX::Name::declare(arg3name), Type::singleton<typename boost::function_traits<Signature>::arg3_type>());
				this->signature.addArgument(RBX::Name::declare(arg4name), Type::singleton<typename boost::function_traits<Signature>::arg4_type>());
			}
		public:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<SyncCallbackDescriptor::GenericFunction> function) const
			{
				this->setCallback(object, boost::bind(callGeneric, function, _1, _2, _3, _4));
			}
		};

		// The fully functional descriptor that binds to class members
		template<typename Signature>
		class BoundCallbackDesc : public SyncCallbackDescImpl<Signature, boost::function_traits<Signature>::arity>
		{
			typedef typename boost::function<Signature> Function;

			template<class Class>
			class Setter : public SyncCallbackDesc<Signature>::ISetter
			{
				typedef void (Class::*OnChanged)();
				Function Class::*member;
				OnChanged onChanged;
			public:
				Setter(Function Class::*member, OnChanged onChanged = NULL):member(member),onChanged(onChanged) {}

				virtual void setCallback(DescribedBase* object, const Function& value) const
				{
					Class* c = static_cast<Class*>(object);
					c->*member = value;
					if (onChanged)
						(c->*onChanged)();
				}
			};
		public:
			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 0>(Class::classDescriptor(), name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member));
			}

			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, void (Class::*onChanged)(), Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 0>(Class::classDescriptor(), name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member, onChanged));
			}

			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, const char* arg1name, Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 1>(Class::classDescriptor(), name, arg1name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member));
			}

			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, const char* arg1name, void (Class::*onChanged)(), Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 1>(Class::classDescriptor(), name, arg1name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member, onChanged));
			}

			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, const char* arg1name, const char* arg2name, Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 2>(Class::classDescriptor(), name, arg1name, arg2name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member));
			}

			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, const char* arg1name, const char* arg2name, void (Class::*onChanged)(), Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 2>(Class::classDescriptor(), name, arg1name, arg2name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member, onChanged));
			}

			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, const char* arg1name, const char* arg2name, const char* arg3name, Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 3>(Class::classDescriptor(), name, arg1name, arg2name, arg3name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member));
			}

			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, const char* arg1name, const char* arg2name, const char* arg3name, void (Class::*onChanged)(), Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 3>(Class::classDescriptor(), name, arg1name, arg2name, arg3name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member, onChanged));
			}

			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, const char* arg1name, const char* arg2name, const char* arg3name, const char* arg4name, Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 4>(Class::classDescriptor(), name, arg1name, arg2name, arg3name, arg4name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member));
			}		

			template<class Class>
			BoundCallbackDesc(const char* name, Function Class::*member, const char* arg1name, const char* arg2name, const char* arg3name, const char* arg4name, void (Class::*onChanged)(), Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				:SyncCallbackDescImpl<Signature, 4>(Class::classDescriptor(), name, arg1name, arg2name, arg3name, arg4name, attributes, security)
			{
				this->setter.reset(new Setter<Class>(member, onChanged));
			}			
		};

		template <class Class, typename Signature, int arity = boost::function_traits<Signature>::arity>
		class BoundAsyncCallbackDesc;

		template <class Class, typename Signature>
		class BoundAsyncCallbackDesc<Class, Signature, 0> : public AsyncCallbackDescriptor
		{
            typedef boost::function<void(AsyncCallbackDescriptor::ResumeFunction resumeFunction, AsyncCallbackDescriptor::ErrorFunction errorFunction)> Function;

			static void callGeneric(shared_ptr<AsyncCallbackDescriptor::GenericFunction> function,
                AsyncCallbackDescriptor::ResumeFunction resumeFunction, AsyncCallbackDescriptor::ErrorFunction errorFunction)
			{
				shared_ptr<Reflection::Tuple> args(new Tuple());
                callGenericImpl(function, args, resumeFunction, errorFunction);
			}

            void declareSignature()
            {
				BOOST_STATIC_ASSERT((boost::function_traits<Signature>::arity == 0));
				this->signature.resultType = &Type::singleton<typename boost::function_traits<Signature>::result_type>();
            }

            Function Class::*member;
            void (Class::*onChanged)(const Function&);

		public:
			BoundAsyncCallbackDesc(const char* name, Function Class::*member, Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				: AsyncCallbackDescriptor(Class::classDescriptor(), name, attributes, security)
                , member(member)
                , onChanged(NULL)
			{
				declareSignature();
			}

			BoundAsyncCallbackDesc(const char* name, Function Class::*member, void (Class::*onChanged)(const Function&), Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				: AsyncCallbackDescriptor(Class::classDescriptor(), name, attributes, security)
                , member(member)
                , onChanged(onChanged)
			{
				declareSignature();
			}

		public:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<AsyncCallbackDescriptor::GenericFunction> function) const
			{
                setGenericCallbackImpl(object, member, onChanged, boost::bind(callGeneric, function, _1, _2));
			}

			virtual void clearCallback(DescribedBase* object) const
			{
                setGenericCallbackImpl(object, member, onChanged, Function());
			}
		};

		template <class Class, typename Signature>
		class BoundAsyncCallbackDesc<Class, Signature, 1> : public AsyncCallbackDescriptor
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
            typedef boost::function<void(Arg1, AsyncCallbackDescriptor::ResumeFunction resumeFunction, AsyncCallbackDescriptor::ErrorFunction errorFunction)> Function;

			static void callGeneric(shared_ptr<AsyncCallbackDescriptor::GenericFunction> function,
                Arg1 arg1,
                AsyncCallbackDescriptor::ResumeFunction resumeFunction, AsyncCallbackDescriptor::ErrorFunction errorFunction)
			{
				shared_ptr<Reflection::Tuple> args(new Tuple());
                args->values.push_back(arg1);
                callGenericImpl(function, args, resumeFunction, errorFunction);
			}

            void declareSignature(const char* arg1name)
            {
				BOOST_STATIC_ASSERT((boost::function_traits<Signature>::arity == 1));
				this->signature.resultType = &Type::singleton<typename boost::function_traits<Signature>::result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1name), Type::singleton<typename boost::function_traits<Signature>::arg1_type>());
            }

            Function Class::*member;
            void (Class::*onChanged)(const Function&);

		public:
			BoundAsyncCallbackDesc(const char* name, Function Class::*member, const char* arg1name, Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				: AsyncCallbackDescriptor(Class::classDescriptor(), name, attributes, security)
                , member(member)
                , onChanged(NULL)
			{
				declareSignature(arg1name);
			}

			BoundAsyncCallbackDesc(const char* name, Function Class::*member, const char* arg1name, void (Class::*onChanged)(const Function&), Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				: AsyncCallbackDescriptor(Class::classDescriptor(), name, attributes, security)
                , member(member)
                , onChanged(onChanged)
			{
				declareSignature(arg1name);
			}

		public:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<AsyncCallbackDescriptor::GenericFunction> function) const
			{
                setGenericCallbackImpl(object, member, onChanged, boost::bind(callGeneric, function, _1, _2, _3));
			}

			virtual void clearCallback(DescribedBase* object) const
			{
                setGenericCallbackImpl(object, member, onChanged, Function());
			}
		};

		template <class Class, typename Signature>
		class BoundAsyncCallbackDesc<Class, Signature, 2> : public AsyncCallbackDescriptor
		{
			typedef typename boost::function_traits<Signature>::arg1_type Arg1;
			typedef typename boost::function_traits<Signature>::arg2_type Arg2;
            typedef boost::function<void(Arg1, Arg2, AsyncCallbackDescriptor::ResumeFunction resumeFunction, AsyncCallbackDescriptor::ErrorFunction errorFunction)> Function;

			static void callGeneric(shared_ptr<AsyncCallbackDescriptor::GenericFunction> function,
                Arg1 arg1,
                Arg2 arg2,
                AsyncCallbackDescriptor::ResumeFunction resumeFunction, AsyncCallbackDescriptor::ErrorFunction errorFunction)
			{
				shared_ptr<Reflection::Tuple> args(new Tuple());
                args->values.push_back(arg1);
                args->values.push_back(arg2);
                callGenericImpl(function, args, resumeFunction, errorFunction);
			}

            void declareSignature(const char* arg1name, const char* arg2name)
            {
				BOOST_STATIC_ASSERT((boost::function_traits<Signature>::arity == 2));
				this->signature.resultType = &Type::singleton<typename boost::function_traits<Signature>::result_type>();
				this->signature.addArgument(RBX::Name::declare(arg1name), Type::singleton<typename boost::function_traits<Signature>::arg1_type>());
				this->signature.addArgument(RBX::Name::declare(arg2name), Type::singleton<typename boost::function_traits<Signature>::arg2_type>());
            }

            Function Class::*member;
            void (Class::*onChanged)(const Function&);

		public:
			BoundAsyncCallbackDesc(const char* name, Function Class::*member, const char* arg1name, const char* arg2name, Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				: AsyncCallbackDescriptor(Class::classDescriptor(), name, attributes, security)
                , member(member)
                , onChanged(NULL)
			{
				declareSignature(arg1name, arg2name);
			}

			BoundAsyncCallbackDesc(const char* name, Function Class::*member, const char* arg1name, const char* arg2name, void (Class::*onChanged)(const Function&), Security::Permissions security = Security::None, Descriptor::Attributes attributes = Descriptor::Attributes())
				: AsyncCallbackDescriptor(Class::classDescriptor(), name, attributes, security)
                , member(member)
                , onChanged(onChanged)
			{
				declareSignature(arg1name, arg2name);
			}

		public:
			virtual void setGenericCallback(DescribedBase* object, shared_ptr<AsyncCallbackDescriptor::GenericFunction> function) const
			{
                setGenericCallbackImpl(object, member, onChanged, boost::bind(callGeneric, function, _1, _2, _3, _4));
			}

			virtual void clearCallback(DescribedBase* object) const
			{
                setGenericCallbackImpl(object, member, onChanged, Function());
			}
		};
	}
}
