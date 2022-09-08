#include "stdafx.h"

#include "v8datamodel/value.h"

namespace RBX
{
	int valueCount = 0;
    REFLECTION_BEGIN();
	template<>
	const int IntValue::defaultValue(0);
	const char* const sIntValue = "IntValue";
	template<>
	Reflection::PropDescriptor<IntValue, int> IntValue::desc_Value("Value", category_Data, &IntValue::getValue, &IntValue::setValue);
	template<>
	Reflection::EventDesc< IntValue, void(int)> IntValue::desc_ValueChanged(&IntValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< IntValue, void(int)> dep_IntValueChanged(&IntValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(IntValue::desc_ValueChanged));

	const char* const sBoolValue = "BoolValue";
	template<>
	const bool BoolValue::defaultValue(false);
	template<>
	Reflection::BoundProp<bool> BoolValue::desc_Value("Value", category_Data, &BoolValue::value, &BoolValue::onValueChanged);
	template<>
	Reflection::EventDesc< BoolValue, void(bool)> BoolValue::desc_ValueChanged(&BoolValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< BoolValue, void(bool)> dep_BoolValueChanged(&BoolValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(BoolValue::desc_ValueChanged));

	const char* const sDoubleValue = "NumberValue";
	template<>
	const double DoubleValue::defaultValue(0.0);
	template<>
	Reflection::PropDescriptor<DoubleValue, double> DoubleValue::desc_Value("Value", category_Data, &DoubleValue::getValue, &DoubleValue::setValue);
	template<>
	Reflection::EventDesc< DoubleValue, void(double)> DoubleValue::desc_ValueChanged(&DoubleValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< DoubleValue, void(double)> dep_DoubleValueChanged(&DoubleValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(DoubleValue::desc_ValueChanged));

	const char* const sStringValue = "StringValue";
	const std::string StringValue::defaultValue("");
	Reflection::BoundProp<std::string> StringValue::desc_Value("Value", category_Data, &StringValue::value, &StringValue::onValueChanged);
	Reflection::EventDesc< StringValue, void(std::string)> StringValue::desc_ValueChanged(&StringValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< StringValue, void(std::string)> dep_StringValueChanged(&StringValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(StringValue::desc_ValueChanged));

   	const char* const sBinaryStringValue = "BinaryStringValue";
	template<>
	const BinaryString BinaryStringValue::defaultValue("");
	template<>
	Reflection::BoundProp<BinaryString> BinaryStringValue::desc_Value("Value", category_Data, &BinaryStringValue::value, &BinaryStringValue::onValueChanged, Reflection::PropertyDescriptor::STREAMING);

	const char* const sVector3Value = "Vector3Value";
	template<>
	const G3D::Vector3 Vector3Value::defaultValue(0,0,0);
	template<>
	Reflection::BoundProp<G3D::Vector3> Vector3Value::desc_Value("Value", category_Data, &Vector3Value::value, &Vector3Value::onValueChanged);
	template<>
	Reflection::EventDesc< Vector3Value, void(G3D::Vector3)> Vector3Value::desc_ValueChanged(&Vector3Value::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< Vector3Value, void(G3D::Vector3)> dep_Vector3ValueChanged(&Vector3Value::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(Vector3Value::desc_ValueChanged));
	
	const char* const sRayValue = "RayValue";
	template<>
	const RBX::RbxRay RayValue::defaultValue(G3D::Vector3(0,0,0),G3D::Vector3(0,0,0));
	template<>
	Reflection::BoundProp<RBX::RbxRay> RayValue::desc_Value("Value", category_Data, &RayValue::value, &RayValue::onValueChanged);
	template<>
	Reflection::EventDesc< RayValue, void(RBX::RbxRay)> RayValue::desc_ValueChanged(&RayValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< RayValue, void(RBX::RbxRay)> dep_RayValueChanged(&RayValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(RayValue::desc_ValueChanged));

	const char* const sCFrameValue = "CFrameValue";
	template<>
	const G3D::CoordinateFrame CFrameValue::defaultValue(G3D::Vector3(0,0,0));
	template<>
	Reflection::BoundProp<G3D::CoordinateFrame> CFrameValue::desc_Value("Value", category_Data, &CFrameValue::value, &CFrameValue::onValueChanged);
	template<>
	Reflection::EventDesc< CFrameValue, void(G3D::CoordinateFrame)> CFrameValue::desc_ValueChanged(&CFrameValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< CFrameValue, void(G3D::CoordinateFrame)> dep_CFrameValueChanged(&CFrameValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(CFrameValue::desc_ValueChanged));

	const char* const sColor3Value = "Color3Value";
	template<>
	const G3D::Color3 Color3Value::defaultValue(0,0,0);
	template<>
	Reflection::BoundProp<G3D::Color3> Color3Value::desc_Value("Value", category_Data, &Color3Value::value, &Color3Value::onValueChanged);
	template<>
	Reflection::EventDesc< Color3Value, void(G3D::Color3)> Color3Value::desc_ValueChanged(&Color3Value::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< Color3Value, void(G3D::Color3)> dep_Color3ValueChanged(&Color3Value::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(Color3Value::desc_ValueChanged));

    const char* const sBrickColorValue = "BrickColorValue";
	template<>
	const BrickColor BrickColorValue::defaultValue(BrickColor::brick_194);
	template<>
	Reflection::BoundProp<BrickColor> BrickColorValue::desc_Value("Value", category_Data, &BrickColorValue::value, &BrickColorValue::onValueChanged);
	template<>
	Reflection::EventDesc< BrickColorValue, void(BrickColor)> BrickColorValue::desc_ValueChanged(&BrickColorValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< BrickColorValue, void(BrickColor)> dep_BrickColorValueChanged(&BrickColorValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(BrickColorValue::desc_ValueChanged));


	const char* const sIntConstrainedValue = "IntConstrainedValue";
	template<>
	const int IntConstrainedValue::defaultMinValue(0);
	template<>
	const int IntConstrainedValue::defaultMaxValue(10);
	template<>
	const int IntConstrainedValue::defaultValue(0);
	template<>
	Reflection::BoundProp<int> IntConstrainedValue::desc_MinValue("MinValue", category_Data, &IntConstrainedValue::minValue);
	template<>
	Reflection::BoundProp<int> IntConstrainedValue::desc_MaxValue("MaxValue", category_Data, &IntConstrainedValue::maxValue);
	template<>
	const Reflection::PropDescriptor<IntConstrainedValue, int> IntConstrainedValue::desc_ValueUi("Value", category_Data, &IntConstrainedValue::getValue, &IntConstrainedValue::setValue, Reflection::PropertyDescriptor::UI);
	template<>
	const Reflection::PropDescriptor<IntConstrainedValue, int> IntConstrainedValue::desc_ValueUiDeprecated("ConstrainedValue", category_Data, &IntConstrainedValue::getValue, &IntConstrainedValue::setValue, Reflection::PropertyDescriptor::LEGACY_SCRIPTING);
	template<>
	const Reflection::PropDescriptor<IntConstrainedValue, int> IntConstrainedValue::desc_ValueRaw("value", category_Data, &IntConstrainedValue::getValue, &IntConstrainedValue::setValueRaw, Reflection::PropertyDescriptor::STREAMING);
	template<>
	Reflection::EventDesc< IntConstrainedValue, void(int)> IntConstrainedValue::desc_ValueChanged(&IntConstrainedValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< IntConstrainedValue, void(int)> dep_IntConstrainedValueChanged(&IntConstrainedValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(IntConstrainedValue::desc_ValueChanged));

	const char* const sDoubleConstrainedValue = "DoubleConstrainedValue";
	template<>
	const double DoubleConstrainedValue::defaultMinValue(0.0);
	template<>
	const double DoubleConstrainedValue::defaultMaxValue(1.0);
	template<>
	const double DoubleConstrainedValue::defaultValue(0);
	template<>
	Reflection::BoundProp<double> DoubleConstrainedValue::desc_MinValue("MinValue", category_Data, &DoubleConstrainedValue::minValue);
	template<>
	Reflection::BoundProp<double> DoubleConstrainedValue::desc_MaxValue("MaxValue", category_Data, &DoubleConstrainedValue::maxValue);
	template<>
	const Reflection::PropDescriptor<DoubleConstrainedValue, double> DoubleConstrainedValue::desc_ValueUi("Value", category_Data, &DoubleConstrainedValue::getValue, &DoubleConstrainedValue::setValue, Reflection::PropertyDescriptor::UI);
	template<>
	const Reflection::PropDescriptor<DoubleConstrainedValue, double> DoubleConstrainedValue::desc_ValueUiDeprecated("ConstrainedValue", category_Data, &DoubleConstrainedValue::getValue, &DoubleConstrainedValue::setValue, Reflection::PropertyDescriptor::LEGACY_SCRIPTING);
	template<>
	const Reflection::PropDescriptor<DoubleConstrainedValue, double> DoubleConstrainedValue::desc_ValueRaw("value", category_Data, &DoubleConstrainedValue::getValue, &DoubleConstrainedValue::setValueRaw, Reflection::PropertyDescriptor::STREAMING);
	template<>
	Reflection::EventDesc< DoubleConstrainedValue, void(double)> DoubleConstrainedValue::desc_ValueChanged(&DoubleConstrainedValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< DoubleConstrainedValue, void(double)> dep_DoubleConstrainedValueChanged(&DoubleConstrainedValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(DoubleConstrainedValue::desc_ValueChanged));

	const char* const sObjectValue = "ObjectValue";
	Reflection::RefPropDescriptor<ObjectValue, Instance> ObjectValue::desc_Value("Value", category_Data, &ObjectValue::getValue, &ObjectValue::setValue);
	Reflection::EventDesc< ObjectValue, void(shared_ptr<Instance>)> desc_ValueChanged(&ObjectValue::valueChangedSignal, "Changed", "value");
	Reflection::EventDesc< ObjectValue, void(shared_ptr<Instance>)> dep_ValueChanged(&ObjectValue::valueChangedSignal, "changed", "value", Reflection::Descriptor::Attributes::deprecated(desc_ValueChanged));
    REFLECTION_END();
	Instance* ObjectValue::getValue() const
	{
		shared_ptr<Instance> v = value.lock();
		// TODO: This is not good practice. In theory, the bald
		// pointer could be collected. In practice that is unlikely.
		// Solution: RefPropDescriptor should use shared_ptr
		return v.get();
	}

	void ObjectValue::setValue(Instance* value)
	{
		shared_ptr<Instance> v = this->value.lock();
		if (v.get() != value)
		{
			this->value = shared_from(value);
			this->raisePropertyChanged(desc_Value);
			valueChangedSignal(this->value.lock());
		}
	}

	void registerValueClasses()
	{
		Creatable<Instance>::create<IntValue>();
		Creatable<Instance>::create<DoubleValue>();
		Creatable<Instance>::create<BoolValue>();
		Creatable<Instance>::create<StringValue>();
		Creatable<Instance>::create<Vector3Value>();
		Creatable<Instance>::create<CFrameValue>();
		Creatable<Instance>::create<Color3Value>();
		Creatable<Instance>::create<BrickColorValue>();
		Creatable<Instance>::create<ObjectValue>();
		Creatable<Instance>::create<RayValue>();
		Creatable<Instance>::create<IntConstrainedValue>();
		Creatable<Instance>::create<DoubleConstrainedValue>();
	}

}
