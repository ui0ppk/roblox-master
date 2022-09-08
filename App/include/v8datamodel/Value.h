#pragma once

#include "v8tree/instance.h"
#include "V8DataModel/Configuration.h"
#include "Util/BinaryString.h"
#include "Util/BrickColor.h"
#include "Script/LuaVM.h"

#if defined(RBX_SECURE_DOUBLE)
#define LUA_VALUE_CONVERT(x) (x.binary[sizeof(ValueType)/sizeof(int)-1] ^= ::LuaSecureDouble::luaXorMask[3])
#else
#define LUA_VALUE_CONVERT(x) void(0)
#endif

namespace RBX
{
	void registerValueClasses();

	class IValue
	{

	};

	// Value Instances are lightweight objects that hold a single value property.
	// They are intended as simple bridges to scripting.  A script can listen to changes
	// in a Value, which effectively lets users send data to a script.
	// They could also be used as a "control panel" to a Model.
	template<class ValueType, const char* const& sClassName>
	class Value 
		: public DescribedCreatable<Value<ValueType, sClassName>, Instance, sClassName>
		, public IValue
	{
		typedef DescribedCreatable<Value<ValueType, sClassName>, Instance, sClassName> Super;
		const static ValueType defaultValue;
		ValueType value;
	public:
		Value():value(defaultValue) {
			Super::setName("Value");
		}

		static Reflection::BoundProp<ValueType> desc_Value;
		rbx::signal<void(ValueType)> valueChangedSignal;
		static Reflection::EventDesc< Value<ValueType, sClassName>, void(ValueType)> desc_ValueChanged;
		
		// Convenience functions:
		const ValueType& getValue() const { return this->value; }
		void setValue(const ValueType& _value) { desc_Value.setValue(this,_value); }

	protected:
		virtual bool askSetParent(const Instance* instance) const {
			// Values are willing to be placed anywhere
			return true;
		}
	private:
		void onValueChanged(const Reflection::PropertyDescriptor&) {
			valueChangedSignal(value);
		}
	};
    
    // NumericValue is an obscured version for int/double.  The name avoids RTTI issues.
    template<class ValueType, const char* const& sClassName>
    class NumericValue 
        : public DescribedCreatable<NumericValue<ValueType, sClassName>, Instance, sClassName>
        , public IValue
    {
        typedef DescribedCreatable<NumericValue<ValueType, sClassName>, Instance, sClassName> Super;
        static const ValueType defaultValue;
        ValueType value;
        union Storage
        {
            size_t binary[sizeof(ValueType)/sizeof(int)];
            ValueType native;
        };
    public:

        NumericValue() {
            Storage newValue;
            newValue.native = defaultValue;
            LUA_VALUE_CONVERT(newValue);
            value = newValue.native;
            Super::setName("Value");
        }
        
        REFLECTION_BEGIN();
        static Reflection::PropDescriptor<NumericValue<ValueType, sClassName>, ValueType> desc_Value;
        static Reflection::EventDesc< NumericValue<ValueType, sClassName>, void(ValueType)> desc_ValueChanged;
        REFLECTION_END();

        //static Reflection::BoundProp<ValueType> desc_Value;
        rbx::signal<void(ValueType)> valueChangedSignal;

        // Convenience functions:
        ValueType getValue() const 
        {
            Storage returnValue;
            returnValue.native = this->value;
            LUA_VALUE_CONVERT(returnValue);
            return returnValue.native;
        }

        void setValue(const ValueType& _value) 
        {
            if (getValue() != _value) {
                Storage newValue;
                newValue.native = _value;
                LUA_VALUE_CONVERT(newValue);
                value = newValue.native;
                this->raisePropertyChanged(desc_Value);
                valueChangedSignal(_value);
           }
        }

    protected:
        virtual bool askSetParent(const Instance* instance) const {
            // Values are willing to be placed anywhere
            return true;
        }
    private:
        void onValueChanged(const Reflection::PropertyDescriptor&) {
            valueChangedSignal(value);
        }
    };

	// TODO: implement min/max range for int and double
	extern const char* const sIntValue;
	typedef NumericValue<int, sIntValue> IntValue;

	extern const char* const sDoubleValue;
	typedef NumericValue<double, sDoubleValue> DoubleValue;

	extern const char* const sBoolValue;
	typedef Value<bool, sBoolValue> BoolValue;

	extern const char* const sVector3Value;
	typedef Value<G3D::Vector3, sVector3Value> Vector3Value;

	extern const char* const sRayValue;
	typedef Value<RBX::RbxRay, sRayValue> RayValue;

	extern const char* const sCFrameValue;
	typedef Value<G3D::CoordinateFrame, sCFrameValue> CFrameValue;

	extern const char* const sColor3Value;
	typedef Value<G3D::Color3, sColor3Value> Color3Value;

	extern const char* const sBrickColorValue;
	typedef Value<RBX::BrickColor, sBrickColorValue> BrickColorValue;

    extern const char* const sBinaryStringValue;
    typedef Value<RBX::BinaryString, sBinaryStringValue> BinaryStringValue;

	extern const char* const sStringValue;
	class StringValue 
		: public DescribedCreatable<StringValue, Instance, sStringValue>
		, public IValue
	{
		typedef DescribedCreatable<StringValue, Instance, sStringValue> Super;
		const static std::string defaultValue;
		std::string value;
	public:
		StringValue():value(defaultValue) {
			Super::setName("Value");
		}
		static Reflection::BoundProp<std::string> desc_Value;
		rbx::signal<void(std::string)> valueChangedSignal;
		static Reflection::EventDesc< StringValue, void(std::string)> desc_ValueChanged;
		
		// Convenience functions:
		const std::string& getValue() const { return this->value; }
		void setValue(const std::string& _value) { desc_Value.setValue(this,_value); }

		/*override*/ int getPersistentDataCost() const 
		{
			return Super::getPersistentDataCost() + computeStringCost(getValue());
		}

	protected:
		virtual bool askSetParent(const Instance* instance) const {
			// Values are willing to be placed anywhere
			return true;
		}
	private:
		void onValueChanged(const Reflection::PropertyDescriptor&) {
			valueChangedSignal(value);
		}
	};

	// Value Instances are lightweight objects that hold a single value property.
	// They are intended as simple bridges to scripting.  A script can listen to changes
	// in a Value, which effectively lets users send data to a script.
	// They could also be used as a "control panel" to a Model.
	template<class ValueType, const char* const& sClassName>
	class ConstrainedValue 
		: public DescribedCreatable<ConstrainedValue<ValueType, sClassName>, Instance, sClassName>
		, public IValue
	{
		typedef DescribedCreatable<ConstrainedValue<ValueType, sClassName>, Instance, sClassName> Super;

		const static ValueType defaultValue;
		const static ValueType defaultMinValue;
		const static ValueType defaultMaxValue;
		ValueType value;
		ValueType minValue;
		ValueType maxValue;
	public:
		ConstrainedValue()
			:value(defaultValue)
			,minValue(defaultMinValue)
			,maxValue(defaultMaxValue) 
		{
			Super::setName("Value");
		}
		static const Reflection::PropDescriptor<ConstrainedValue<ValueType, sClassName>, ValueType> desc_ValueUi;
		static const Reflection::PropDescriptor<ConstrainedValue<ValueType, sClassName>, ValueType> desc_ValueUiDeprecated;
		static const Reflection::PropDescriptor<ConstrainedValue<ValueType, sClassName>, ValueType> desc_ValueRaw;
		static Reflection::BoundProp<ValueType> desc_MinValue;
		static Reflection::BoundProp<ValueType> desc_MaxValue;
		rbx::signal<void(ValueType)> valueChangedSignal;
		static Reflection::EventDesc< ConstrainedValue<ValueType, sClassName>, void(ValueType)> desc_ValueChanged;
		
		// Convenience functions:
		ValueType getValue() const { return this->value; }
		void setValue(ValueType _value) 
		{
			if(_value < minValue) 
				_value = minValue;

			if(_value > maxValue) 
				_value = maxValue;

			setValueRaw(_value);
		}

		void setValueRaw(ValueType _value) 
		{
			if(value != _value){
				value = _value;
				this->raisePropertyChanged(desc_ValueUi);
				this->raisePropertyChanged(desc_ValueRaw);
				valueChangedSignal(value);
			}
		}

	protected:
		virtual bool askSetParent(const Instance* instance) const {
			// ConstrainedValues only make sense in Configuration objects
			return (Instance::fastDynamicCast<Configuration>(instance) != NULL);
		}
	};
	extern const char* const sIntConstrainedValue;
	typedef ConstrainedValue<int, sIntConstrainedValue> IntConstrainedValue;

	extern const char* const sDoubleConstrainedValue;
	typedef ConstrainedValue<double, sDoubleConstrainedValue> DoubleConstrainedValue;


	extern const char* const sObjectValue;
	class ObjectValue 
		: public DescribedCreatable<ObjectValue, Instance, sObjectValue>
		, public IValue
	{
		weak_ptr<Instance> value;
	public:
		ObjectValue() 
		{
			setName("Value");
		}
		ObjectValue(const char* name) 
		{
			setName(name);
		}
		Instance* getValue() const;
		void setValue(Instance* value);
		static RBX::Reflection::RefPropDescriptor<ObjectValue, Instance> desc_Value;
		rbx::signal<void(shared_ptr<Instance>)> valueChangedSignal;
	protected:
		virtual bool askSetParent(const Instance* instance) const {
			// Values are willing to be placed anywhere
			return true;
		}
	private:
		void onValueChanged(const Reflection::PropertyDescriptor&) {
			valueChangedSignal(value.lock());
		}
	};

}
