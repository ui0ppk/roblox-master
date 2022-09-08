#pragma once

#include "Reflection/Function.h"
#include <boost/function.hpp>

namespace RBX
{
	namespace Reflection
	{
		class YieldFunction;

		// Base that describes a YieldFunction
		class RBXBaseClass YieldFunctionDescriptor : public MemberDescriptor
		{
		public:

			typedef YieldFunction ConstMember;
			typedef YieldFunction Member;

		protected:
			SignatureDescriptor signature;
			YieldFunctionDescriptor(ClassDescriptor& classDescriptor, const char* name, Security::Permissions security, Attributes attributes);

		public:
			const SignatureDescriptor& getSignature() const { return signature; }
			virtual void execute(DescribedBase* instance, FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const = 0;
		};


		// A light-weight convenience class that associates a FunctionDescriptor
		// with a described object to create a "Function"
		class YieldFunction
		{
		protected:
			const YieldFunctionDescriptor* descriptor;
			DescribedBase* instance;
		public:
			inline YieldFunction(const YieldFunctionDescriptor& descriptor, DescribedBase* instance)
				:descriptor(&descriptor),instance(instance)
			{}

			inline YieldFunction(const YieldFunction& other)
				:descriptor(other.descriptor),instance(other.instance)
			{}
			inline YieldFunction& operator =(const YieldFunction& other)
			{
				this->descriptor = other.descriptor;
				this->instance = other.instance;
				return *this;
			}

			inline const RBX::Name& getName() const { 
				return descriptor->name; 
			}

			inline const YieldFunctionDescriptor* getDescriptor() const { 
				return descriptor; 
			} 

			void execute(FunctionDescriptor::Arguments& arguments, boost::function<void(Variant)> resumeFunction, boost::function<void(std::string)> errorFunction) const {
				return descriptor->execute(const_cast<DescribedBase*>(instance), arguments, resumeFunction, errorFunction);
			}
		};
	}
}