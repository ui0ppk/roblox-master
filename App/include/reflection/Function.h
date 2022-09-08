
#pragma once

#include "reflection/type.h"
#include "security/securitycontext.h"
#include "reflection/member.h"
#include "util/G3DCore.h"
#include "util/Region3.h"
#include "util/Region3Int16.h"

struct lua_State;

namespace RBX
{
	namespace Reflection
	{
		class Function;
		class EnumDescriptor;

		// Base that describes a Function
		class RBXBaseClass FunctionDescriptor : public MemberDescriptor
		{
		public:
			enum Kind
			{
				Kind_Default,
				Kind_Custom
			};

			class RBXInterface Arguments
			{
			public:
				Variant returnValue;

				virtual size_t size() const = 0;
				// Place the value for the requested parameter in "value".
				// 
				// index:       1-based index into the argument list
				// returns:     true if the index contains a valid argument
				// value:       the value to set. the function returns false then value is unchanged
				virtual bool getVariant(int index, Variant& value) const = 0;
				virtual bool getBool(int index, bool& value) const = 0;
				virtual bool getLong(int index, long& value) const = 0;
				virtual bool getDouble(int index, double& value) const = 0;
				virtual bool getString(int index, std::string& value) const = 0;
				virtual bool getVector3int16(int index, Vector3int16& value) const = 0;
				virtual bool getRegion3int16(int index, Region3int16& value) const = 0;
				virtual bool getVector3(int index, Vector3& value) const = 0;
				virtual bool getRegion3(int index, Region3& value) const = 0;
				virtual bool getRect(int index, Rect2D& value) const = 0;
				virtual bool getObject(int index, shared_ptr<DescribedBase>& value) const = 0;
				virtual bool getEnum(int index, const EnumDescriptor& desc, int& value) const = 0;
			};
			typedef Function ConstMember;
			typedef Function Member;

		protected:
			SignatureDescriptor signature;
			Kind kind;
			FunctionDescriptor(ClassDescriptor& classDescriptor, const char* name, Security::Permissions security, Attributes attributes);

		public:
			const SignatureDescriptor& getSignature() const { return signature; }

			Kind getKind() const { return kind; }
			
			virtual int executeCustom(DescribedBase* instance, lua_State*) const { return 0; }

			virtual void execute(DescribedBase* instance, Arguments& arguments) const = 0;
		};


		// A light-weight convenience class that associates a FunctionDescriptor
		// with a described object to create a "Function"
		class Function
		{
		protected:
			const FunctionDescriptor* descriptor;
			DescribedBase* instance;
		public:
			inline Function(const FunctionDescriptor& descriptor, DescribedBase* instance)
				:descriptor(&descriptor),instance(instance)
			{}

			inline Function(const Function& other)
				:descriptor(other.descriptor),instance(other.instance)
			{}
			inline Function& operator =(const Function& other)
			{
				this->descriptor = other.descriptor;
				this->instance = other.instance;
				return *this;
			}

			inline const RBX::Name& getName() const { 
				return descriptor->name; 
			}

			inline const FunctionDescriptor* getDescriptor() const { 
				return descriptor; 
			} 

			void execute(FunctionDescriptor::Arguments& arguments) const {
				return descriptor->execute(const_cast<DescribedBase*>(instance), arguments);
			}
		};
	}
}
