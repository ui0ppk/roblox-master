#include "reflection/enumconverter.h"

#include "G3D/Color3uint8.h"
#include "util/exception.h"
#include "Util/BrickColor.h"
#include "Util/NormalId.h"
#include "Util/SystemAddress.h"
#include "Util/Region3.h"
#include "Util/Region3int16.h"
#include "Util/UDim.h"
#include "Util/Faces.h"
#include "Util/Axes.h"
#include "Util/BinaryString.h"
#include "Util/base64.hpp"
#include "Util/PhysicalProperties.h"
#include "v8tree/Instance.h"
#include "Script/ThreadRef.h"
#include "rbx/make_shared.h"
#include "rbx/signal.h"

using namespace RBX;
using namespace RBX::Reflection;

namespace RBX {
	float getElementValueOrDefault(const XmlElement* ele, int dflt)
	{
		float output;
		if (ele)
			ele->getValue(output);
		else
			output = dflt;

		return output;
	}

template<>
bool StringConverter<NormalId>::convertToValue(const std::string& text, NormalId& value)
{
	if(text.find("Top") != std::string::npos || text.find("top") != std::string::npos ){
		value = NORM_Y;
		return true;
	}
	if(text.find("Bottom") != std::string::npos || text.find("bottom") != std::string::npos ){
		value = NORM_Y_NEG;
		return true;
	}
	if(text.find("Back") != std::string::npos || text.find("back") != std::string::npos ){
		value = NORM_Z;
		return true;
	}
	if(text.find("Front") != std::string::npos || text.find("front") != std::string::npos ){
		value = NORM_Z_NEG;
		return true;
	}
	if(text.find("Right") != std::string::npos || text.find("right") != std::string::npos ){
		value = NORM_X;
		return true;
	}
	if(text.find("Left") != std::string::npos || text.find("left") != std::string::npos ){
		value = NORM_X_NEG;
		return true;
	}
	return false;
}

template<>
bool StringConverter<SystemAddress>::convertToValue(const std::string& text, SystemAddress& value)
{
	return false;
}

template<>
std::string StringConverter<BinaryString>::convertToString(const BinaryString& value)
{
    return value.value();
}

template<>
bool StringConverter<BinaryString>::convertToValue(const std::string& text, BinaryString& value)
{
    value = BinaryString(text);
    return true;
}

} //namespace RBX


namespace RBX {
    namespace Reflection
	{
//////////////////////////////////////////////////////////
template<>
Reflection::EnumDesc<NormalId>::EnumDesc()
	:Reflection::EnumDescriptor("NormalId")
{
/*
TODO:
Should be:  (Also in PartInstance)
> -Z eyes, nose, mouth
> +Z back
> -X left ear
> +X right ear
> -Y neck
> +Y top
*/

	addPair(NORM_Y, "Top");
	addPair(NORM_Y_NEG, "Bottom");
	addPair(NORM_Z, "Back");
	addPair(NORM_Z_NEG, "Front");
	addPair(NORM_X, "Right");
	addPair(NORM_X_NEG, "Left");

	//addPair(NORM_UNDEFINED, "Undefined");
}

template<>
RBX::NormalId& RBX::Reflection::Variant::convert<RBX::NormalId>(void)
{
	return genericConvert<RBX::NormalId>();
}


template<>
int TypedPropertyDescriptor<std::string>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(std::string) + getValue(instance).size();
}
template<>
bool RBX::Reflection::TypedPropertyDescriptor<std::string>::hasStringValue() const {
	return true;
}
template<>
std::string RBX::Reflection::TypedPropertyDescriptor<std::string>::getStringValue(const DescribedBase* instance) const{
	return getValue(instance);
}
template<>
bool RBX::Reflection::TypedPropertyDescriptor<std::string>::setStringValue(DescribedBase* instance, const std::string& text) const {
	setValue(instance, text);
	return true;
}


template<>
int TypedPropertyDescriptor<bool>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(bool);
}
template<>
bool TypedPropertyDescriptor<bool>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<bool>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<bool>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<bool>::setStringValue(DescribedBase* instance, const std::string& text) const {
	bool value;
	if (StringConverter<bool>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

template<>
int TypedPropertyDescriptor<float>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(float);
}
template<>
bool TypedPropertyDescriptor<float>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<float>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<float>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<float>::setStringValue(DescribedBase* instance, const std::string& text) const {
	float value;
	if (StringConverter<float>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

template<>
int TypedPropertyDescriptor<double>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(double);
}
template<>
bool TypedPropertyDescriptor<double>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<double>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<double>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<double>::setStringValue(DescribedBase* instance, const std::string& text) const {
	double value;
	if (StringConverter<double>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

template<>
int TypedPropertyDescriptor<int>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(int);
}
template<>
bool TypedPropertyDescriptor<int>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<int>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<int>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<int>::setStringValue(DescribedBase* instance, const std::string& text) const {
	int value;
	if (StringConverter<int>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

template<>
int TypedPropertyDescriptor<RBX::Region3>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(RBX::Region3);
}
template<>
bool TypedPropertyDescriptor<RBX::Region3>::hasStringValue() const {
	return false;
}
template<>
std::string TypedPropertyDescriptor<RBX::Region3>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<RBX::Region3>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<RBX::Region3>::setStringValue(DescribedBase* instance, const std::string& text) const {
	RBX::Region3 value;
	if (StringConverter<RBX::Region3>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

static bool ReadValue(Vector3int16& output, const XmlElement* element)
{
	if (!element->isXsiNil())
	{
		const XmlElement* xElement = element->findFirstChildByTag(tag_X);
		if (!xElement)
			return false;
		const XmlElement* yElement = element->findFirstChildByTag(tag_Y);
		const XmlElement* zElement = element->findFirstChildByTag(tag_Z);

		xElement->getValue((int&)output.x);
		yElement->getValue((int&)output.y);
		zElement->getValue((int&)output.z);

		return true;
	}
	return false;
}
static void WriteValue(XmlElement* element, const Vector3int16& v)
{
	XmlElement* xElement = element->addChild(tag_X);
	XmlElement* yElement = element->addChild(tag_Y);
	XmlElement* zElement = element->addChild(tag_Z);

	xElement->setValue((int&)v.x);
	yElement->setValue((int&)v.y);
	zElement->setValue((int&)v.z);

}

template<>
int TypedPropertyDescriptor<RBX::Region3int16>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(Region3int16);
}
template<>
bool TypedPropertyDescriptor<RBX::Region3int16>::hasStringValue() const {
	return false;
}
template<>
std::string TypedPropertyDescriptor<RBX::Region3int16>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<RBX::Region3int16>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<RBX::Region3int16>::setStringValue(DescribedBase* instance, const std::string& text) const {
	RBX::Region3int16 value;
	if (StringConverter<RBX::Region3int16>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}
template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::Region3int16>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		const XmlElement* minElement = element->findFirstChildByTag(tag_Min);
		const XmlElement* maxElement = element->findFirstChildByTag(tag_Max);

		Vector3int16 min, max;
		if(ReadValue(min, minElement) && ReadValue(max, maxElement))
		{
			Region3int16 v( min, max );
			setValue(instance, v);
		}
	}
}
template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::Region3int16>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	XmlElement* minElement	 = element->addChild(tag_Min);
	XmlElement* maxElement = element->addChild(tag_Max);

	Region3int16 v = getValue(instance);
	WriteValue(minElement, v.getMinPos());
	WriteValue(maxElement, v.getMaxPos());
}

template<>
int TypedPropertyDescriptor<G3D::Vector3>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(G3D::Vector3);
}
template<>
bool TypedPropertyDescriptor<G3D::Vector3>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<G3D::Vector3>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<G3D::Vector3>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<G3D::Vector3>::setStringValue(DescribedBase* instance, const std::string& text) const {
	G3D::Vector3 value;
	if (StringConverter<G3D::Vector3>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

template<>
int TypedPropertyDescriptor<G3D::Vector2>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(G3D::Vector2);
}
template<>
bool TypedPropertyDescriptor<G3D::Vector2>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<G3D::Vector2>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<G3D::Vector2>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<G3D::Vector2>::setStringValue(DescribedBase* instance, const std::string& text) const {
	G3D::Vector2 value;
	if (StringConverter<G3D::Vector2>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}



template<>
void RBX::Reflection::TypedPropertyDescriptor<std::string>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		std::string value;
		if (element->getValue(value))
			setValue(instance, value);
	}
}
template<>
void RBX::Reflection::TypedPropertyDescriptor<std::string>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	element->setValue(getValue(instance));
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<float>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		float value;
		if (element->getValue(value))
			setValue(instance, value);
	}
}
template<>
void RBX::Reflection::TypedPropertyDescriptor<float>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	element->setValue(getValue(instance));
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<bool>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		bool value;
		if (element->getValue(value))
			setValue(instance, value);
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<double>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	element->setValue(getValue(instance));
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<double>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		double value;
		if (element->getValue(value))
			setValue(instance, value);
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<bool>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	element->setValue(getValue(instance));
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<int>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		int value;
		if (element->getValue(value))
			setValue(instance, value);
	}
}
template<>
void RBX::Reflection::TypedPropertyDescriptor<int>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	element->setValue(getValue(instance));
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<class G3D::Vector2>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		const XmlElement* xElement = element->findFirstChildByTag(tag_X);
		if (!xElement)
			return;
		const XmlElement* yElement = element->findFirstChildByTag(tag_Y);

		G3D::Vector2 v;
		xElement->getValue(v.x);
		yElement->getValue(v.y);
		setValue(instance, v);
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<class G3D::Vector2>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	// Write the data out in accordance with ROBLOX Schema

	XmlElement* xElement = element->addChild(tag_X);
	XmlElement* yElement = element->addChild(tag_Y);

	G3D::Vector2 v = getValue(instance);
	xElement->setValue(v.x);
	yElement->setValue(v.y);
}

static bool ReadValue(Vector3& output, const XmlElement* element)
{
	if (!element->isXsiNil())
	{
		const XmlElement* xElement = element->findFirstChildByTag(tag_X);
		if (!xElement)
			return false;
		const XmlElement* yElement = element->findFirstChildByTag(tag_Y);
		const XmlElement* zElement = element->findFirstChildByTag(tag_Z);

		xElement->getValue(output.x);
		yElement->getValue(output.y);
		zElement->getValue(output.z);

		return true;
	}
	return false;
}
static void WriteValue(XmlElement* element, const Vector3& v)
{
	XmlElement* xElement = element->addChild(tag_X);
	XmlElement* yElement = element->addChild(tag_Y);
	XmlElement* zElement = element->addChild(tag_Z);

	xElement->setValue(v.x);
	yElement->setValue(v.y);
	zElement->setValue(v.z);

}

template<>
void RBX::Reflection::TypedPropertyDescriptor<class G3D::Vector3>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	G3D::Vector3 v;
	if(ReadValue(v, element)){
		setValue(instance, v);
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<class G3D::Vector3>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	G3D::Vector3 v = getValue(instance);
	WriteValue(element, v);
}

template<>
const Reflection::Type& Reflection::Type::getSingleton< shared_ptr<Reflection::DescribedBase> >()
{
	static TType< shared_ptr<Reflection::DescribedBase> > type("Object");
	return type;
}

template<>
const Type& Reflection::Type::getSingleton< shared_ptr<Instance> >()
{
	static TType< shared_ptr<Instance> > type("Instance");
	return type;
}

template<>
const Type& Type::getSingleton<shared_ptr<const Instances> >()
{
	static TType<shared_ptr<const Instances> > type("Objects");
	return type;
}

template<>
const Type& Type::getSingleton<int>()
{
	static TType<int> type("int");
	return type;
}

template<>
const Type& Type::getSingleton<long>()
{
	static TType<long> type("long");
	return type;
}


template<>
int& RBX::Reflection::Variant::convert<int>(void)
{
	// Convert double to int
	if (_type->isType<double>())
	{
		value = G3D::iRound(cast<double>());
		_type = &Type::singleton<int>();
	}

	// Convert float to int
	if (_type->isType<float>())
	{
		value = G3D::iRound(cast<float>());
		_type = &Type::singleton<int>();
	}

	// Convert bool to int
	if (_type->isType<bool>())
	{
		value = (cast<bool>() ? 1 : 0);
		_type = &Type::singleton<int>();
	}

	return genericConvert<int>();
}

template<>
const Type& Type::getSingleton<bool>()
{
	static TType<bool> type("bool");
	return type;
}

template<>
bool& RBX::Reflection::Variant::convert<bool>(void)
{
	// Convert int to bool
	if (_type->isType<int>())
	{
		value = (bool) (cast<int>()!=0);
		_type = &Type::singleton<bool>();
	}

	// Convert float to bool
	else if (_type->isType<float>())
	{
		value = (bool) (cast<float>()!=0.0f);
		_type = &Type::singleton<bool>();
	}

	// Convert double to bool
	else if (_type->isType<double>())
	{
		value = (bool) (cast<double>()!=0.0);
		_type = &Type::singleton<bool>();
	}

	return genericConvert<bool>();
}

template<>
const Type& Type::getSingleton<float>()
{
	static TType<float> type("float");
	return type;
}

template<>
float& RBX::Reflection::Variant::convert<float>(void)
{
	// Convert double to float
	if (_type->isType<double>())
	{
		value = (float) cast<double>();
		_type = &Type::singleton<float>();
	}

	// Convert int to float
	else if (_type->isType<int>())
	{
		value = (float) cast<int>();
		_type = &Type::singleton<float>();
	}

	// Convert bool to float
	else if (_type->isType<bool>())
	{
		value = (float) (cast<bool>() ? 1 : 0);
		_type = &Type::singleton<float>();
	}

	return genericConvert<float>();
}

template<>
const Type& Type::getSingleton<double>()
{
	static TType<double> type("double");
	return type;
}




template<>
double& RBX::Reflection::Variant::convert<double>(void)
{
	// Convert int to double
	if (_type->isType<int>())
	{
		value = (double) cast<int>();
		_type = &Type::singleton<double>();
	}

	// Convert bool to double
	else if (_type->isType<bool>())
	{
		value = (double) (cast<bool>() ? 1 : 0);
		_type = &Type::singleton<double>();
	}

	// Convert float to double
	else if (_type->isType<float>())
	{
		value = (double) cast<float>();
		_type = &Type::singleton<double>();
	}

	return genericConvert<double>();
}


template<>
RBX::Region3& RBX::Reflection::Variant::convert<RBX::Region3>(void)
{
	return genericConvert<RBX::Region3>();
}

template<>
RBX::Region3int16& RBX::Reflection::Variant::convert<RBX::Region3int16>(void)
{
	return genericConvert<RBX::Region3int16>();
}

template<>
G3D::Vector3& RBX::Reflection::Variant::convert<G3D::Vector3>(void)
{
	return genericConvert<G3D::Vector3>();
}

template<>
RBX::Vector2& RBX::Reflection::Variant::convert<RBX::Vector2>(void)
{
	return genericConvert<RBX::Vector2>();
}

template<>
G3D::CoordinateFrame& RBX::Reflection::Variant::convert<G3D::CoordinateFrame>(void)
{
	return genericConvert<G3D::CoordinateFrame>();
}

template<>
const Type& Type::getSingleton<RBX::ContentId>()
{
	static TType<RBX::ContentId> type("Content");
	return type;
}

template<>
ContentId& RBX::Reflection::Variant::convert<ContentId>(void)
{
	if (_type->isType<std::string>())
	{
		value = ContentId(cast<std::string>());
		_type = &Type::singleton<ContentId>();
	}
	return genericConvert<ContentId>();
}


template<>
const Type& Type::getSingleton<std::string>()
{
	static TType<std::string> type("string");
	return type;
}

template<>
const Type& Type::getSingleton<G3D::Vector3>()
{
	static TType<G3D::Vector3> type("Vector3");
	return type;
}

template<>
const Type& Type::getSingleton<RBX::Vector2>()
{
	static TType<RBX::Vector2> type("Vector2");
	return type;
}

template<>
const Type& Type::getSingleton<rbx::signals::connection>()
{
	static TType<rbx::signals::connection> type("Connection");
	return type;
}


template<>
const Type& Type::getSingleton<shared_ptr<const Tuple> >()
{
	static TType<shared_ptr<const Tuple> > type("Tuple");
	return type;
}

template<>
std::string& RBX::Reflection::Variant::convert<std::string>(void)
{
	// TODO: This should be made compact somehow!!!
	if (isType<bool>())
	{
		value = StringConverter<bool>::convertToString(cast<bool>());
		_type = &Type::singleton<std::string>();
	}
	else if (isType<int>())
	{
		value = StringConverter<int>::convertToString(cast<int>());
		_type = &Type::singleton<std::string>();
	}
	else if (isType<long>())
	{
		value = StringConverter<long>::convertToString(cast<long>());
		_type = &Type::singleton<std::string>();
	}
	else if (isType<float>())
	{
		value = StringConverter<float>::convertToString(cast<float>());
		_type = &Type::singleton<std::string>();
	}
	else if (isType<double>())
	{
		value = StringConverter<double>::convertToString(cast<double>());
		_type = &Type::singleton<std::string>();
	}
	else if (isType<G3D::Vector3>())
	{
		value = StringConverter<G3D::Vector3>::convertToString(cast<G3D::Vector3>());
		_type = &Type::singleton<std::string>();
	}
	else if (isType<RBX::CoordinateFrame>())
	{
		if (cast<RBX::CoordinateFrame>() == RBX::CoordinateFrame())
			value = std::string("Identity");
		else
			value = std::string("?");
		_type = &Type::singleton<std::string>();
	}
	else if (isType<Lua::WeakFunctionRef>())
	{
		if(cast<Lua::WeakFunctionRef>().empty())
			value = std::string("nil");
		else
			value = std::string("function");
		_type = &Type::singleton<std::string>();			
	}
	else if (isType<shared_ptr<Instance> >())
	{
		const shared_ptr<Instance>& v = cast<shared_ptr<Instance> >();
		if (v)
			value = std::string(Type::singleton< shared_ptr<Instance> >().name.c_str());
		else
			value = std::string("nil");
		_type = &Type::singleton<std::string>();
	}
	else if(isType<shared_ptr<const Reflection::ValueArray> >()) 
	{
		value = std::string("{}"); 
		_type = &Type::singleton<std::string>(); 
	}
	else if (const Reflection::EnumDescriptor* desc = Reflection::EnumDescriptor::lookupDescriptor(type()))
	{
		const Reflection::EnumDescriptor::Item* item = desc->lookup(*this);
		if (item!=NULL)
		{
			std::string stringValue;
			if(item->convertToString(stringValue)){
				value = stringValue;
				_type = &Type::singleton<std::string>();
			}
		}
	}

	std::string* id = tryCast<std::string>();
	if (id==NULL)
		throw std::runtime_error("Unable to cast value to std::string");
	return *id;
}


template<>
shared_ptr<Instance>& Reflection::Variant::convert<shared_ptr<Instance> >(void)
{
	// Interpret "void" (Lua nil) as a null Instance
	if (isType<void>())
	{
		value = shared_ptr<Instance>();
		_type = &Type::singleton<shared_ptr<Instance> >();
	}
	else if (isType<shared_ptr<Reflection::DescribedBase> >())
	{
		const shared_ptr<Reflection::DescribedBase>& v = cast<shared_ptr<Reflection::DescribedBase> >();

		value = shared_dynamic_cast<Instance>(v);
		_type = &Type::singleton<shared_ptr<Instance> >();
	}

	shared_ptr<Instance>* v = tryCast<shared_ptr<Instance> >();
	if (v==NULL)
		throw std::runtime_error("Unable to cast value to Object");
	return *v;
}
}}

static void CastInstance(Variant value, shared_ptr<Instances> instancesCollection)
{
	instancesCollection->push_back(value.convert<shared_ptr<Instance> >());
}

namespace RBX {
namespace Reflection {

template<>
shared_ptr<const Instances>& Reflection::Variant::convert<shared_ptr<const Instances> >(void)
{
	{
		shared_ptr<const Instances>* v = tryCast<shared_ptr<const Instances> >();
		if (v!=NULL){
			return *v;
		}
	}

	{
		shared_ptr<const ValueArray>* v = tryCast<shared_ptr<const ValueArray> >();
		if(v != NULL){
			shared_ptr<Instances> newInstances(new Instances());
			std::for_each((*v)->begin(), (*v)->end(), boost::bind(&CastInstance, _1, newInstances));
			
			value = shared_ptr<const Instances>(newInstances);
			_type = &Type::singleton<shared_ptr<const Instances> >();

			return cast<shared_ptr<const Instances> >();
		}
	}  
	throw std::runtime_error("Unable to cast value to Objects");
}

template<>
shared_ptr<Reflection::DescribedBase>& Reflection::Variant::convert<shared_ptr<Reflection::DescribedBase> >(void)
{
	// Interpret "void" (Lua nil) as a null Instance
	if (isType<void>())
	{
		value = shared_ptr<Reflection::DescribedBase>();
		_type = &Type::singleton<shared_ptr<Reflection::DescribedBase> >();
	}

	else if (isType<shared_ptr<Instance> >())
	{
		const shared_ptr<Instance>& v = cast<shared_ptr<Instance> >();
		value = shared_static_cast<Reflection::DescribedBase>(v);
		_type = &Type::singleton<shared_ptr<Reflection::DescribedBase> >();
	}

	shared_ptr<Reflection::DescribedBase>* v = tryCast<shared_ptr<Reflection::DescribedBase> >();
	if (v==NULL)
		throw std::runtime_error("Unable to cast value to Object");
	return *v;
}

template<>
shared_ptr<const Tuple>& Variant::convert<shared_ptr<const Tuple> >(void)
{
	shared_ptr<const Tuple>* v = tryCast<shared_ptr<const Tuple> >();
	if (v != NULL)
		return *v;

	if (isVoid())
	{
		value = shared_ptr<const Tuple>(); // null is equivalent to 0 items (optimization)
		_type = &Type::singleton< shared_ptr<const Tuple> >();
		return cast< shared_ptr<const Tuple> >();
	}

	// Any value can be converted to a tuple of 1
	shared_ptr<Tuple> tuple = rbx::make_shared<Tuple>(1);
	tuple->values[0] = *this;
	value = shared_ptr<const Tuple>(tuple);
	_type = &Type::singleton< shared_ptr<const Tuple> >();
	return cast< shared_ptr<const Tuple> >();
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// Vector2int16
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
G3D::Vector2int16& RBX::Reflection::Variant::convert<G3D::Vector2int16>(void)
{
	return genericConvert<G3D::Vector2int16>();
}

template<>
const Type& Type::getSingleton<G3D::Vector2int16>()
{
	static TType<G3D::Vector2int16> type("Vector2int16");
	return type;
}

template<>
int TypedPropertyDescriptor<G3D::Vector2int16>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(G3D::Vector2int16);
}

template<>
bool TypedPropertyDescriptor<G3D::Vector2int16>::hasStringValue() const {
	return true;
}
		
template<>
std::string TypedPropertyDescriptor<G3D::Vector2int16>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<G3D::Vector2int16>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<G3D::Vector2int16>::setStringValue(DescribedBase* instance, const std::string& text) const {
	G3D::Vector2int16 value;
	if (StringConverter<G3D::Vector2int16>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<G3D::Vector2int16>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		const XmlElement* xElement = element->findFirstChildByTag(tag_X);
		const XmlElement* yElement = element->findFirstChildByTag(tag_Y);

		int x, y;
		xElement->getValue(x);
		yElement->getValue(y);
		setValue(instance, G3D::Vector2int16(x,y));

	}
}
template<>
void RBX::Reflection::TypedPropertyDescriptor<G3D::Vector2int16>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	// Write the data out in accordance with ROBLOX Schema

	XmlElement* xElement = element->addChild(tag_X);
	XmlElement* yElement = element->addChild(tag_Y);

	G3D::Vector2int16 v = getValue(instance);
	xElement->setValue((int)v.x);
	yElement->setValue((int)v.y);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// Vector3int16
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
G3D::Vector3int16& RBX::Reflection::Variant::convert<G3D::Vector3int16>(void)
{
	return genericConvert<G3D::Vector3int16>();
}

template<>
const Type& Type::getSingleton<G3D::Vector3int16>()
{
	static TType<G3D::Vector3int16> type("Vector3int16");
	return type;
}

template<>
int TypedPropertyDescriptor<G3D::Vector3int16>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(G3D::Vector3int16);
}

template<>
bool TypedPropertyDescriptor<G3D::Vector3int16>::hasStringValue() const
{
	return true;
}
		
template<>
std::string TypedPropertyDescriptor<G3D::Vector3int16>::getStringValue(const DescribedBase* instance) const
{
	return StringConverter<G3D::Vector3int16>::convertToString(getValue(instance));
}

template<>
bool TypedPropertyDescriptor<G3D::Vector3int16>::setStringValue(DescribedBase* instance, const std::string& text) const
{
	G3D::Vector3int16 value;
	if (StringConverter<G3D::Vector3int16>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3int16>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		const XmlElement* xElement = element->findFirstChildByTag(tag_X);
		const XmlElement* yElement = element->findFirstChildByTag(tag_Y);
		const XmlElement* zElement = element->findFirstChildByTag(tag_Z);

		int x, y, z;
		xElement->getValue(x);
		yElement->getValue(y);
		zElement->getValue(z);
		setValue(instance, G3D::Vector3int16(x,y,z));
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<G3D::Vector3int16>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	XmlElement* xElement = element->addChild(tag_X);
	XmlElement* yElement = element->addChild(tag_Y);
	XmlElement* zElement = element->addChild(tag_Z);

	G3D::Vector3int16 v = getValue(instance);
	xElement->setValue((int)v.x);
	yElement->setValue((int)v.y);
	zElement->setValue((int)v.z);
}
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// PhysicalProperties
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue
template<>
PhysicalProperties& RBX::Reflection::Variant::convert<PhysicalProperties>(void)
{
	return genericConvert<PhysicalProperties>();
}

template<>
const Type& Type::getSingleton<PhysicalProperties>()
{
	static TType<PhysicalProperties> type("PhysicalProperties");
	return type;
}

template<> 
int TypedPropertyDescriptor<PhysicalProperties>::getDataSize(const DescribedBase* instance) const
{
	return sizeof(PhysicalProperties);
}

template<> 
bool TypedPropertyDescriptor<PhysicalProperties>::hasStringValue() const {
	return true;
}

template<>
std::string TypedPropertyDescriptor<PhysicalProperties>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<PhysicalProperties>::convertToString(getValue(instance));
}

template<>
bool TypedPropertyDescriptor<PhysicalProperties>::setStringValue(DescribedBase* instance, const std::string& text) const {
	return false;
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<PhysicalProperties>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		bool customPhysEnabled = false;
		const XmlElement* customPhysPropElement	= element->findFirstChildByTag(tag_customPhysProp);
		if (customPhysPropElement)
		{
			customPhysPropElement->getValue(customPhysEnabled);
		}

		if (customPhysEnabled)
		{
			const XmlElement* densityElement			= element->findFirstChildByTag(tag_customDensity);
			const XmlElement* frictionElement			= element->findFirstChildByTag(tag_customFriction);
			const XmlElement* elasticityElement			= element->findFirstChildByTag(tag_customElasticity);
			const XmlElement* frictionWeightElement		= element->findFirstChildByTag(tag_customFrictionWeight);
			const XmlElement* elasticityWeightElement	= element->findFirstChildByTag(tag_customElasticityWeight);
			float density			= getElementValueOrDefault(densityElement, 1.0f);
			float friction			= getElementValueOrDefault(frictionElement, 0.0f);
			float elasticity		= getElementValueOrDefault(elasticityElement, 0.0f);
			float frictionWeight	= getElementValueOrDefault(frictionWeightElement, 1.0f);
			float elasticityWeight	= getElementValueOrDefault(elasticityWeightElement, 1.0f);

			setValue(instance, PhysicalProperties(density, friction, elasticity, frictionWeight, elasticityWeight));
		}
		else
		{
			setValue(instance, PhysicalProperties());
		}
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<PhysicalProperties>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	
	//Only worth saving if CustomPhysicalProperties is enabled
	PhysicalProperties currentProperties = getValue(instance);
	bool customEnabled = currentProperties.getCustomEnabled();

	XmlElement* customPhysPropElement	= element->addChild(tag_customPhysProp);
	customPhysPropElement->setValue(customEnabled);
	if (customEnabled)
	{
		XmlElement* densityElement			= element->addChild(tag_customDensity);
		XmlElement* frictionElement			= element->addChild(tag_customFriction);
		XmlElement* elasticityElement		= element->addChild(tag_customElasticity);
		XmlElement* frictionWeightElement   = element->addChild(tag_customFrictionWeight);
		XmlElement* elasticityWeightElement = element->addChild(tag_customElasticityWeight);

		densityElement->setValue(currentProperties.getDensity());
		frictionElement->setValue(currentProperties.getFriction());
		elasticityElement->setValue(currentProperties.getElasticity());
		frictionWeightElement->setValue(currentProperties.getFrictionWeight());
		elasticityWeightElement->setValue(currentProperties.getElasticityWeight());
	}
}
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// Rect2D
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
G3D::Rect2D& RBX::Reflection::Variant::convert<G3D::Rect2D>(void)
{
    return genericConvert<G3D::Rect2D>();
}

template<>
const Type& Type::getSingleton<G3D::Rect2D>()
{
    static TType<G3D::Rect2D> type("Rect2D");
    return type;
}

template<>
int TypedPropertyDescriptor<G3D::Rect2D>::getDataSize(const DescribedBase* instance) const
{
    return sizeof(G3D::Rect2D);
}
template<>
bool TypedPropertyDescriptor<G3D::Rect2D>::hasStringValue() const {
    return true;
}
template<>
std::string TypedPropertyDescriptor<G3D::Rect2D>::getStringValue(const DescribedBase* instance) const{
    return StringConverter<G3D::Rect2D>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<G3D::Rect2D>::setStringValue(DescribedBase* instance, const std::string& text) const {
    return false;
}
template<>
void RBX::Reflection::TypedPropertyDescriptor<G3D::Rect2D>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
    if (!element->isXsiNil())
    {
        const XmlElement* minElement = element->findFirstChildByTag(tag_Min);
        const XmlElement* maxElement = element->findFirstChildByTag(tag_Max);
        
        Vector2 min;
        Vector2 max;
        
        // TODO: re-use vector2 code?
        {
            const XmlElement* xElement = minElement->findFirstChildByTag(tag_X);
            const XmlElement* yElement = minElement->findFirstChildByTag(tag_Y);
            
            xElement->getValue(min.x);
            yElement->getValue(min.y);
        }
        {
            const XmlElement* xElement = maxElement->findFirstChildByTag(tag_X);
            const XmlElement* yElement = maxElement->findFirstChildByTag(tag_Y);
            
            xElement->getValue(max.x);
            yElement->getValue(max.y);
        }
        
        setValue(instance, G3D::Rect2D(min,max));
    }
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<G3D::Rect2D>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
    // Write the data out in accordance with ROBLOX Schema
    
    XmlElement* minElement = element->addChild(tag_Min);
    XmlElement* maxElement = element->addChild(tag_Max);
    
    // TODO: re-use vector2 code?
    G3D::Rect2D v = getValue(instance);
    {
        XmlElement* xElement = minElement->addChild(tag_X);
        XmlElement* yElement = minElement->addChild(tag_Y);
        
        xElement->setValue(v.x0());
        yElement->setValue(v.y0());
    }
    {
        XmlElement* xElement = maxElement->addChild(tag_X);
        XmlElement* yElement = maxElement->addChild(tag_Y);
        
        xElement->setValue(v.x1());
        yElement->setValue(v.y1());
    }
}
    
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// UDim
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
RBX::UDim& RBX::Reflection::Variant::convert<RBX::UDim>(void)
{
	return genericConvert<RBX::UDim>();
}

template<>
const Type& Type::getSingleton<RBX::UDim>()
{
	static TType<RBX::UDim> type("UDim");
	return type;
}

template<>
int TypedPropertyDescriptor<RBX::UDim>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(RBX::UDim);
}
template<>
bool TypedPropertyDescriptor<RBX::UDim>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<RBX::UDim>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<RBX::UDim>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<RBX::UDim>::setStringValue(DescribedBase* instance, const std::string& text) const {
	return false;
}
template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::UDim>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		const XmlElement* sElement = element->findFirstChildByTag(tag_S);
		const XmlElement* oElement = element->findFirstChildByTag(tag_O);

		float s;
		int o;
		sElement->getValue(s);
		oElement->getValue(o);
		setValue(instance, RBX::UDim(s,o));
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::UDim>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	// Write the data out in accordance with ROBLOX Schema

	XmlElement* sElement = element->addChild(tag_S);
	XmlElement* oElement = element->addChild(tag_O);

	RBX::UDim v = getValue(instance);
	sElement->setValue(v.scale);
	oElement->setValue((int)v.offset);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// UDim2
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
RBX::UDim2& RBX::Reflection::Variant::convert<RBX::UDim2>(void)
{
	return genericConvert<RBX::UDim2>();
}

template<>
const Type& Type::getSingleton<RBX::UDim2>()
{
	static TType<RBX::UDim2> type("UDim2");
	return type;
}

template<>
int TypedPropertyDescriptor<RBX::UDim2>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(RBX::UDim2);
}
template<>
bool TypedPropertyDescriptor<RBX::UDim2>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<RBX::UDim2>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<RBX::UDim2>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<RBX::UDim2>::setStringValue(DescribedBase* instance, const std::string& text) const {
	return false;
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		const XmlElement* xsElement = element->findFirstChildByTag(tag_XS);
		const XmlElement* xoElement = element->findFirstChildByTag(tag_XO);
		const XmlElement* ysElement = element->findFirstChildByTag(tag_YS);
		const XmlElement* yoElement = element->findFirstChildByTag(tag_YO);

		float xs,ys;
		int xo,yo;
		xsElement->getValue(xs);
		xoElement->getValue(xo);
		ysElement->getValue(ys);
		yoElement->getValue(yo);
		setValue(instance, RBX::UDim2(xs,xo, ys,yo));
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::UDim2>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	// Write the data out in accordance with ROBLOX Schema

	XmlElement* xsElement = element->addChild(tag_XS);
	XmlElement* xoElement = element->addChild(tag_XO);
	XmlElement* ysElement = element->addChild(tag_YS);
	XmlElement* yoElement = element->addChild(tag_YO);

	RBX::UDim2 v = getValue(instance);
	xsElement->setValue(v.x.scale);
	xoElement->setValue((int)v.x.offset);
	ysElement->setValue(v.y.scale);
	yoElement->setValue((int)v.y.offset);
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// Faces
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
RBX::Faces& RBX::Reflection::Variant::convert<RBX::Faces>(void)
{
	return genericConvert<RBX::Faces>();
}

template<>
const Type& Type::getSingleton<RBX::Faces>()
{
	static TType<RBX::Faces> type("Faces");
	return type;
}

template<>
int TypedPropertyDescriptor<RBX::Faces>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(RBX::Faces);
}
template<>
bool TypedPropertyDescriptor<RBX::Faces>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<RBX::Faces>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<RBX::Faces>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<RBX::Faces>::setStringValue(DescribedBase* instance, const std::string& text) const {
	return false;
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::Faces>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		const XmlElement* facesElement = element->findFirstChildByTag(tag_faces);

		int faces;
		facesElement->getValue(faces);
		setValue(instance, RBX::Faces(faces));
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::Faces>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	// Write the data out in accordance with ROBLOX Schema

	XmlElement* facesElement = element->addChild(tag_faces);

	RBX::Faces v = getValue(instance);
	facesElement->setValue(v.normalIdMask);
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// Axes
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
RBX::Axes& RBX::Reflection::Variant::convert<RBX::Axes>(void)
{
	return genericConvert<RBX::Axes>();
}

template<>
const Type& Type::getSingleton<RBX::Axes>()
{
	static TType<RBX::Axes> type("Axes");
	return type;
}

template<>
int TypedPropertyDescriptor<RBX::Axes>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(RBX::Axes);
}
template<>
bool TypedPropertyDescriptor<RBX::Axes>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<RBX::Axes>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<RBX::Axes>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<RBX::Axes>::setStringValue(DescribedBase* instance, const std::string& text) const {
	return false;
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::Axes>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		const XmlElement* axesElement = element->findFirstChildByTag(tag_axes);

		int axes;
		axesElement->getValue(axes);
		setValue(instance, RBX::Axes(axes));
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::Axes>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	// Write the data out in accordance with ROBLOX Schema

	XmlElement* axesElement = element->addChild(tag_axes);

	RBX::Axes v = getValue(instance);
	axesElement->setValue(v.axisMask);
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// Color3
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
G3D::Color3& RBX::Reflection::Variant::convert<G3D::Color3>(void)
{
	return genericConvert<G3D::Color3>();
}

template<>
const Type& Type::getSingleton<G3D::Color3>()
{
	static TType<G3D::Color3> type("Color3");
	return type;
}

template<>
int TypedPropertyDescriptor<G3D::Color3>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(G3D::Color3);
}
template<>
bool TypedPropertyDescriptor<G3D::Color3>::hasStringValue() const {
	return true;
}
template<>
std::string TypedPropertyDescriptor<G3D::Color3>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<G3D::Color3>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<G3D::Color3>::setStringValue(DescribedBase* instance, const std::string& text) const {
	G3D::Color3 value;
	if (StringConverter<G3D::Color3>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		unsigned int argb;
		if (element->getValue(argb))
		{
			setValue(instance, G3D::Color3uint8::fromARGB(argb));
		}
		else
		{
			// Old-style
			const XmlElement* xElement = element->findFirstChildByTag(tag_R);
			const XmlElement* yElement = element->findFirstChildByTag(tag_G);
			const XmlElement* zElement = element->findFirstChildByTag(tag_B);

			if (xElement && yElement && zElement)
			{
				G3D::Color3 v;
				xElement->getValue(v.r);
				yElement->getValue(v.g);
				zElement->getValue(v.b);
				setValue(instance, v);
			}
		}
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<G3D::Color3>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	// Write the data out in accordance with ROBLOX Schema

	G3D::Color3uint8 v = G3D::Color3uint8(getValue(instance));
	element->setValue(v.asUInt32());
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// Ray
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
RBX::RbxRay& RBX::Reflection::Variant::convert<RBX::RbxRay>(void)
{
	return genericConvert<RBX::RbxRay>();
}

template<>
const Type& Type::getSingleton<RBX::RbxRay>()
{
	static TType<RBX::RbxRay> type("Ray");
	return type;
}

template<>
int TypedPropertyDescriptor<RBX::RbxRay>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(RBX::RbxRay);
}

template<>
bool TypedPropertyDescriptor<RBX::RbxRay>::hasStringValue() const {
	return true;
}

template<>
std::string TypedPropertyDescriptor<RBX::RbxRay>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<RBX::RbxRay>::convertToString(getValue(instance));
}
template<>
bool TypedPropertyDescriptor<RBX::RbxRay>::setStringValue(DescribedBase* instance, const std::string& text) const {
	RBX::RbxRay value;
	if (StringConverter<RBX::RbxRay>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}


template<>
void RBX::Reflection::TypedPropertyDescriptor<class RBX::RbxRay>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		const XmlElement* originElement = element->findFirstChildByTag(tag_Origin);
		const XmlElement* directionElement = element->findFirstChildByTag(tag_Direction);

		RbxRay v;
		if(ReadValue(v.origin(), originElement) && ReadValue(v.direction(), directionElement)){
			setValue(instance, v);
		}
	}
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<class RBX::RbxRay>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	XmlElement* originElement	 = element->addChild(tag_Origin);
	XmlElement* directionElement = element->addChild(tag_Direction);

	RBX::RbxRay v = getValue(instance);
	WriteValue(originElement, v.origin());
	WriteValue(directionElement, v.direction());
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// Brick Color
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue

template<>
RBX::BrickColor& RBX::Reflection::Variant::convert<RBX::BrickColor>(void)
{
	return genericConvert<RBX::BrickColor>();
}

template<>
const Type& Type::getSingleton<BrickColor>()
{
	static TType<BrickColor> type("BrickColor", "int");
	return type;
}

template<>
int TypedPropertyDescriptor<BrickColor>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(BrickColor);
}

template<>
bool TypedPropertyDescriptor<BrickColor>::hasStringValue() const {
	return false;
}

template<>
std::string TypedPropertyDescriptor<BrickColor>::getStringValue(const DescribedBase* instance) const {
	return Super::getStringValue(instance);
}
template<>
bool TypedPropertyDescriptor<BrickColor>::setStringValue(DescribedBase* instance, const std::string& text) const {
	return Super::setStringValue(instance, text);
}


template<>
void TypedPropertyDescriptor<BrickColor>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		int number;
		if (element->getValue(number))
		{
			setValue(instance, BrickColor(number));
		}
	}
}


template<>
void TypedPropertyDescriptor<BrickColor>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	// Write the data out in accordance with ROBLOX Schema
	element->setValue(getValue(instance).asInt());
}



/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// SystemAddress
// convert, getSingleton, hasStringValue, getStringValue, setStringValue, readValue, writeValue, convertToValue

template<>
RBX::SystemAddress& RBX::Reflection::Variant::convert<RBX::SystemAddress>(void)
{
	return genericConvert<RBX::SystemAddress>();
}

template<>
const Type& Type::getSingleton<SystemAddress>()
{
	static TType<SystemAddress> type("SystemAddress");
	return type;
}

template<>
int TypedPropertyDescriptor<SystemAddress>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(SystemAddress);
}

template<>
bool TypedPropertyDescriptor<SystemAddress>::hasStringValue() const {
	return false;
}

template<>
std::string TypedPropertyDescriptor<SystemAddress>::getStringValue(const DescribedBase* instance) const {
	return Super::getStringValue(instance);
}
template<>
bool TypedPropertyDescriptor<SystemAddress>::setStringValue(DescribedBase* instance, const std::string& text) const {
	return Super::setStringValue(instance, text);
}


template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::SystemAddress>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	RBXASSERT(0);	// should never be streamed...  REPLICATE_ONLY
}

template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::SystemAddress>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	RBXASSERT(0);	// should never be streamed...  REPLICATE_ONLY
}


/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////




template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::ContentId>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		ContentId value;
		if (element->getValue(value))
			setValue(instance, value);
	}
}
template<>
void RBX::Reflection::TypedPropertyDescriptor<RBX::ContentId>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	element->setValue(ContentId(getValue(instance)));
}

template<>
int TypedPropertyDescriptor<RBX::ContentId>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(RBX::ContentId) + getValue(instance).toString().size();
}

template<>
bool RBX::Reflection::TypedPropertyDescriptor<ContentId>::hasStringValue() const {
	return true;
}
template<>
std::string RBX::Reflection::TypedPropertyDescriptor<ContentId>::getStringValue(const DescribedBase* instance) const{
	return StringConverter<ContentId>::convertToString(getValue(instance));
}
template<>
bool RBX::Reflection::TypedPropertyDescriptor<ContentId>::setStringValue(DescribedBase* instance, const std::string& text) const {
	ContentId value;
	if (StringConverter<ContentId>::convertToValue(text, value))
	{
		setValue(instance, value);
		return true;
	}
	else
		return false;
}
		
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//
// BinaryString
template<>
const Type& Type::getSingleton<BinaryString>()
{
    static TType<RBX::BinaryString> type("BinaryString");
    return type;
}

template<>
void TypedPropertyDescriptor<BinaryString>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
    if (!element->isXsiNil())
    {
        std::string text;
        if (element->getValue(text))
        {
            std::ostringstream result;
            base64<char> decoder;
            int state = 0;
            std::ostreambuf_iterator<char> out(result);
            decoder.get(text.begin(), text.end(), out, state);

            BinaryString value(result.str());
            setValue(instance, value);
        }
    }
}

template<>
void TypedPropertyDescriptor<BinaryString>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
    BinaryString value = getValue(instance);

    std::string result;
    base64<char>::encode(value.value().c_str(), value.value().length(), result, base64<>::lf());

    element->setValue(result);
}

template<>
BinaryString& Variant::convert<BinaryString>(void)
{
    return genericConvert<BinaryString>();
}

template<>
int TypedPropertyDescriptor<BinaryString>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(BinaryString) + getValue(instance).value().length();
}

template<>
bool TypedPropertyDescriptor<BinaryString>::hasStringValue() const {
    return true;
}

template<>
std::string TypedPropertyDescriptor<BinaryString>::getStringValue(const DescribedBase* instance) const{
    return getValue(instance).value();
}

template<>
bool TypedPropertyDescriptor<BinaryString>::setStringValue(DescribedBase* instance, const std::string& text) const {
    setValue(instance, BinaryString(text));
    return true;
}

}} // namespaces

