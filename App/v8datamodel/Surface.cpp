/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Surface.h"
#include "V8DataModel/PartInstance.h"
#include "V8World/Primitive.h"
#include "Util/Hash.h"

DYNAMIC_FASTFLAGVARIABLE(UseRemoveTypeIDTricks,true)

namespace RBX {

////////////////////////////////////////////////////////////////
//
// Surface


Surface::Surface(
			PartInstance* partInstance,
			NormalId surfId
			)
: partInstance(partInstance)
, surfId(surfId)
{
}

Surface::Surface() : partInstance(NULL), surfId(NORM_X)
{

}

Surface::Surface(const Surface& lhs) : partInstance(lhs.partInstance), surfId(lhs.surfId)
{
}

SurfaceType Surface::getSurfaceType() const
{
	return partInstance->getSurfaceType(surfId);
}

void Surface::setSurfaceType(SurfaceType type) 
{
	partInstance->setSurfaceType(surfId, type);
}

LegacyController::InputType Surface::getInput() const
{
	return partInstance->getInput(surfId);
}


void Surface::setSurfaceInput(RBX::LegacyController::InputType value) 
{
	partInstance->setSurfaceInput(surfId, value);
}

float Surface::getParamA() const
{
	return partInstance->getParamA(surfId);
}


float Surface::getParamB() const
{
	return partInstance->getParamB(surfId);
}


void Surface::setParamA(float value) 
{
	partInstance->setParamA(surfId, value);
}

void Surface::setParamB(float value)
{
	partInstance->setParamB(surfId, value);
}

void Surface::toggleSurfaceType()
{
	if (this->getSurfaceType() == NO_SURFACE) {
		this->setSurfaceType(GLUE);
	}
	else {
		this->setSurfaceType(NO_SURFACE);
	}
}


void Surface::flat()
{
	setSurfaceType(NO_SURFACE);
	setSurfaceInput(SurfaceData::empty().inputType);
	setParamA(SurfaceData::empty().paramA);
	setParamB(SurfaceData::empty().paramB);
}

namespace Reflection {
	template<>
	const Type& Type::getSingleton<Surface>()
	{
		static TType<Surface> type("Surface", "Complex");
		return type;
	}
}//namespace Reflection

template<RBX::NormalId face, typename V, typename Get, typename Set>
class SurfaceGetSet : public Reflection::TypedPropertyDescriptor<V>::GetSet
{
	Get get;
	Set set;
public:
	SurfaceGetSet(Get get, Set set):get(get),set(set)
	{}

	virtual bool isReadOnly() const {
		return false;
	}

	virtual bool isWriteOnly() const {
		return false;
	}

	virtual V getValue(const Reflection::DescribedBase* instance) const
	{
		if (DFFlag::UseRemoveTypeIDTricks)
		{
			debugAssertM(dynamic_cast<const PartInstance*>(instance)!=NULL, instance->getClassName().toString());
		}
		else
		{
			debugAssertM(dynamic_cast<const PartInstance*>(instance)!=NULL, typeid(*instance).name());
		}
		//return (static_cast<const PartInstance*>(instance)->getSurfaces()[face].*get)();
		const PartInstance* part = static_cast<const PartInstance*>(instance);
		return ((*part).*get)(face);
	}
	virtual void setValue(Reflection::DescribedBase* instance, const V& value) const
	{
		if (DFFlag::UseRemoveTypeIDTricks)
		{
			debugAssertM(dynamic_cast<PartInstance*>(instance)!=NULL, instance->getClassName().toString());
		}
		else
		{
			debugAssertM(dynamic_cast<PartInstance*>(instance)!=NULL, typeid(*instance).name());
		}
		//(static_cast<PartInstance*>(instance)->getSurfaces()[face].*set)(value);
		PartInstance* part = static_cast<PartInstance*>(instance);
		((*part).*set)(face, value);
	}
};

// Specialized PropertyDescriptor use for referencing Surface Poperties of a PartInstance
// TODO: When we refactor Surface to be an Instance then this class can melt away (or be used for legacy reading only)
template<RBX::NormalId face, typename V >
class SurfacePropDescriptor : public Reflection::TypedPropertyDescriptor<V>
{
public:
	template<typename Get, typename Set>
	SurfacePropDescriptor(const char* name, const char* category, Get get, Set set, RBX::Reflection::PropertyDescriptor::Functionality flags = RBX::Reflection::PropertyDescriptor::STANDARD, Security::Permissions security = Security::None)
		:Reflection::TypedPropertyDescriptor<V>(
		PartInstance::classDescriptor(), 
		name, 
		category, 
		std::auto_ptr<typename Reflection::TypedPropertyDescriptor<V>::GetSet>(new SurfaceGetSet<face, V, Get, Set>(get, set)), 
		flags,
		security
		)
	{
	}

};

// TODO: This is almost identical to EnumPropDescriptor.  Perhaps we could use a Functor of some kind
// to make the GetFunc/SetFunc more generic. Then we could insert the "getSurfaces()[face]." bit into
// a common class.
template<RBX::NormalId face, typename V >
class SurfaceEnumPropDescriptor : public Reflection::EnumPropertyDescriptor
{
	std::auto_ptr<typename Reflection::TypedPropertyDescriptor<V>::GetSet> getset;
public:
	template<typename Get, typename Set>
	SurfaceEnumPropDescriptor(const char* name, const char* category, Get get, Set set, Functionality flags = STANDARD)
		:Reflection::EnumPropertyDescriptor(PartInstance::classDescriptor(), Reflection::EnumDesc<V>::singleton(), name, category, flags)
		,getset(new SurfaceGetSet<face, V, Get, Set>(get, set))
	{}

	virtual bool isReadOnly() const {
		return getset->isReadOnly();
	}

	virtual bool isWriteOnly() const {
		return getset->isWriteOnly();
	}

	/*implement*/ void get(const Reflection::DescribedBase* instance, Reflection::Variant& value) const 
	{
		value = getset->getValue(instance);
	}
	/*implement*/ void set(Reflection::DescribedBase* instance, const Reflection::Variant& value) const 
	{
		// TODO: This might be inefficient. How is the value stored in getset???
		getset->setValue(instance, value.get<V>());
	}

	V getValue(const Reflection::DescribedBase* object) const {
		return getset->getValue(object);
	}

	/*implement*/ void getVariant(const Reflection::DescribedBase* instance, Reflection::Variant& value) const 
	{
		value = getset->getValue(instance);
	}

	/*implement*/ void setVariant(Reflection::DescribedBase* instance, const Reflection::Variant& value) const 
	{
		// TODO: This might be inefficient. How is the value stored in getset???
		getset->setValue(instance, value.get<V>());
	}

	void setValue(Reflection::DescribedBase* object, const V& value) const {
		getset->setValue(object, value);
	}

	/*implement*/ void copyValue(const Reflection::DescribedBase* source, Reflection::DescribedBase* destination) const
	{
		setValue(destination, getValue(source));
	}


	/*implement*/ const Reflection::EnumDescriptor::Item* getEnumItem(const Reflection::DescribedBase* instance) const
	{
		const Reflection::EnumDescriptor::Item* result = Reflection::EnumDesc<V>::singleton().convertToItem(getValue(instance));
		return result;
	}

	/*implement*/ bool equalValues(const Reflection::DescribedBase* a, const Reflection::DescribedBase* b) const {
		return getValue(a) == getValue(b);
	}
	/*implement*/ int getEnumValue(const Reflection::DescribedBase* instance) const
	{
		return (int) getValue(instance);
	}
	/*implement*/ bool setEnumValue(Reflection::DescribedBase* instance, int intValue) const
	{
		if (Reflection::EnumDesc<V>::singleton().isValue(intValue))
		{
			setValue(instance, (V) intValue);
			return true;
		}
		else
			return false;
	}
	virtual size_t getIndexValue(const Reflection::DescribedBase* instance) const
	{
		return Reflection::EnumDesc<V>::singleton().convertToIndex(getValue(instance));
	}
	virtual bool setIndexValue(Reflection::DescribedBase* instance, size_t index) const
	{
		V value;
		if (Reflection::EnumDesc<V>::singleton().convertToValue(index, value))
		{
			setValue(instance, value);
			return true;
		}
		else
			return false;
	}

	// PropertyDescriptor implementation:
	virtual bool hasStringValue() const {
		return true;
	}
	virtual std::string getStringValue(const Reflection::DescribedBase* instance) const
	{
		return Reflection::EnumDesc<V>::singleton().convertToString(getValue(instance));
	}
	virtual bool setStringValue(Reflection::DescribedBase* instance, const std::string& text) const
	{
		V value;
		if (Reflection::EnumDesc<V>::singleton().convertToValue(text.c_str(), value))
		{
			setValue(instance, value);
			return true;
		}
		else
			return false;
	}
	// An alternate, more efficient version of setStringValue
	virtual bool setStringValue(Reflection::DescribedBase* instance, const RBX::Name& name) const
	{
		V value;
		if (Reflection::EnumDesc<V>::singleton().convertToValue(name, value))
		{
			setValue(instance, value);
			return true;
		}
		else
			return false;
	}

	virtual void readValue(Reflection::DescribedBase* instance, const XmlElement* element, RBX::IReferenceBinder& binder) const
	{
		if (!element->isXsiNil()) {

			if (element->isValueType<std::string>())
			{
				// Legacy code to handle files older than 10/29/05
				// TODO: Opt: Remove this legacy code sometime? It slows text XML down a bit
				std::string sValue;
				if (element->getValue(sValue))
				{
					V e;
					if (Reflection::EnumDesc<V>::singleton().convertToValue(sValue.c_str(), e))
					{
						setValue(instance, e);
						return;
					}
				}
			}

			int value;
			if (element->getValue(value))
			{
				setValue(instance, static_cast<V>(value));
				return;
			}

			RBXASSERT(false);
		}
	}
	virtual void writeValue(const Reflection::DescribedBase* instance, XmlElement* element) const
	{
		element->setValue(static_cast<int>(getValue(instance)));
	}
};

namespace Reflection {
	template<>
	SurfaceType& Variant::convert<SurfaceType>(void)
	{
		return genericConvert<SurfaceType>();
	}

	template<>
	LegacyController::InputType& Variant::convert<LegacyController::InputType>(void)
	{
		return genericConvert<LegacyController::InputType>();
	}
}//namespace Reflection

template<>
bool StringConverter<SurfaceType>::convertToValue(const std::string& text, SurfaceType& value)
{
	return false;
}

template<>
bool StringConverter<LegacyController::InputType>::convertToValue(const std::string& text, LegacyController::InputType& value)
{
	return false;
}


static SurfaceEnumPropDescriptor<NORM_Y, SurfaceType> desc_TopType("TopSurface", "Surface", &PartInstance::getSurfaceType, &PartInstance::setSurfaceType);
static SurfaceEnumPropDescriptor<NORM_Y, LegacyController::InputType> desc_TopSurfaceInput("TopSurfaceInput", "Surface Inputs", &PartInstance::getInput, &PartInstance::setSurfaceInput);
static SurfacePropDescriptor<NORM_Y, float> desc_TopParamA("TopParamA", "Surface Inputs", &PartInstance::getParamA, &PartInstance::setParamA);
static SurfacePropDescriptor<NORM_Y, float> desc_TopParamB("TopParamB", "Surface Inputs", &PartInstance::getParamB, &PartInstance::setParamB);

static SurfaceEnumPropDescriptor<NORM_Y_NEG, SurfaceType> desc_BottomType("BottomSurface", "Surface", &PartInstance::getSurfaceType, &PartInstance::setSurfaceType);
static SurfaceEnumPropDescriptor<NORM_Y_NEG, LegacyController::InputType> desc_BottomSurfaceInput("BottomSurfaceInput", "Surface Inputs", &PartInstance::getInput, &PartInstance::setSurfaceInput);
static SurfacePropDescriptor<NORM_Y_NEG, float> desc_BottomParamA("BottomParamA", "Surface Inputs", &PartInstance::getParamA, &PartInstance::setParamA);
static SurfacePropDescriptor<NORM_Y_NEG, float> desc_BottomParamB("BottomParamB", "Surface Inputs", &PartInstance::getParamB, &PartInstance::setParamB);

static SurfaceEnumPropDescriptor<NORM_X_NEG, SurfaceType> desc_LeftType("LeftSurface", "Surface", &PartInstance::getSurfaceType, &PartInstance::setSurfaceType);
static SurfaceEnumPropDescriptor<NORM_X_NEG, LegacyController::InputType> desc_LeftSurfaceInput("LeftSurfaceInput", "Surface Inputs", &PartInstance::getInput, &PartInstance::setSurfaceInput);
static SurfacePropDescriptor<NORM_X_NEG, float> desc_LeftParamA("LeftParamA", "Surface Inputs", &PartInstance::getParamA, &PartInstance::setParamA);
static SurfacePropDescriptor<NORM_X_NEG, float> desc_LeftParamB("LeftParamB", "Surface Inputs", &PartInstance::getParamB, &PartInstance::setParamB);

static SurfaceEnumPropDescriptor<NORM_X, SurfaceType> desc_RightType("RightSurface", "Surface", &PartInstance::getSurfaceType, &PartInstance::setSurfaceType);
static SurfaceEnumPropDescriptor<NORM_X, LegacyController::InputType> desc_RightSurfaceInput("RightSurfaceInput", "Surface Inputs", &PartInstance::getInput, &PartInstance::setSurfaceInput);
static SurfacePropDescriptor<NORM_X, float> desc_RightParamA("RightParamA", "Surface Inputs", &PartInstance::getParamA, &PartInstance::setParamA);
static SurfacePropDescriptor<NORM_X, float> desc_RightParamB("RightParamB", "Surface Inputs", &PartInstance::getParamB, &PartInstance::setParamB);

static SurfaceEnumPropDescriptor<NORM_Z_NEG, SurfaceType> desc_FrontType("FrontSurface", "Surface", &PartInstance::getSurfaceType, &PartInstance::setSurfaceType);
static SurfaceEnumPropDescriptor<NORM_Z_NEG, LegacyController::InputType> desc_FrontSurfaceInput("FrontSurfaceInput", "Surface Inputs", &PartInstance::getInput, &PartInstance::setSurfaceInput);
static SurfacePropDescriptor<NORM_Z_NEG, float> desc_FrontParamA("FrontParamA", "Surface Inputs", &PartInstance::getParamA, &PartInstance::setParamA);
static SurfacePropDescriptor<NORM_Z_NEG, float> desc_FrontParamB("FrontParamB", "Surface Inputs", &PartInstance::getParamB, &PartInstance::setParamB);

static SurfaceEnumPropDescriptor<NORM_Z, SurfaceType> desc_BackType("BackSurface", "Surface", &PartInstance::getSurfaceType, &PartInstance::setSurfaceType);
static SurfaceEnumPropDescriptor<NORM_Z, LegacyController::InputType> desc_BackSurfaceInput("BackSurfaceInput", "Surface Inputs", &PartInstance::getInput, &PartInstance::setSurfaceInput);
static SurfacePropDescriptor<NORM_Z, float> desc_BackParamA("BackParamA", "Surface Inputs", &PartInstance::getParamA, &PartInstance::setParamA);
static SurfacePropDescriptor<NORM_Z, float> desc_BackParamB("BackParamB", "Surface Inputs", &PartInstance::getParamB, &PartInstance::setParamB);

RBX_REGISTER_TYPE(RBX::Surface);

// This is already getting registered as an Enum in Factory Registration.cpp
// Enum also registers is as Type, so following is not required, complains as duplicate symbols on gcc
//RBX_REGISTER_TYPE(RBX::SurfaceType);

void Surface::registerSurfaceDescriptors()
{
	// Simply being here causes the descriptors to load
}
bool Surface::isSurfaceDescriptor(const Reflection::PropertyDescriptor& desc)
{
	if (desc==desc_TopType)
		return true;
	if (desc==desc_BottomType)
		return true;
	if (desc==desc_LeftType)
		return true;
	if (desc==desc_RightType)
		return true;
	if (desc==desc_FrontType)
		return true;
	if (desc==desc_BackType)
		return true;
	return false;
}

const Reflection::PropertyDescriptor& Surface::getSurfaceTypeStatic(NormalId face)
{
	switch (face)
	{
	default:
		RBXASSERT(false);
	case NORM_Y:
		return desc_TopType;
	case NORM_Y_NEG:
		return desc_BottomType;
	case NORM_Z:
		return desc_BackType;
	case NORM_Z_NEG:
		return desc_FrontType;
	case NORM_X:
		return desc_RightType;
	case NORM_X_NEG:
		return desc_LeftType;
	}
}

const Reflection::PropertyDescriptor& Surface::getSurfaceInputStatic(NormalId face)
{
	switch (face)
	{
	default:
		RBXASSERT(false);
	case NORM_Y:
		return desc_TopSurfaceInput;
	case NORM_Y_NEG:
		return desc_BottomSurfaceInput;
	case NORM_Z:
		return desc_BackSurfaceInput;
	case NORM_Z_NEG:
		return desc_FrontSurfaceInput;
	case NORM_X:
		return desc_RightSurfaceInput;
	case NORM_X_NEG:
		return desc_LeftSurfaceInput;
	}
}

const Reflection::PropertyDescriptor& Surface::getParamAStatic(NormalId face)
{
	switch (face)
	{
	default:
		RBXASSERT(false);
	case NORM_Y:
		return desc_TopParamA;
	case NORM_Y_NEG:
		return desc_BottomParamA;
	case NORM_Z:
		return desc_BackParamA;
	case NORM_Z_NEG:
		return desc_FrontParamA;
	case NORM_X:
		return desc_RightParamA;
	case NORM_X_NEG:
		return desc_LeftParamA;
	}
}

const Reflection::PropertyDescriptor& Surface::getParamBStatic(NormalId face)
{
	switch (face)
	{
	default:
		RBXASSERT(false);
	case NORM_Y:
		return desc_TopParamB;
	case NORM_Y_NEG:
		return desc_BottomParamB;
	case NORM_Z:
		return desc_BackParamB;
	case NORM_Z_NEG:
		return desc_FrontParamB;
	case NORM_X:
		return desc_RightParamB;
	case NORM_X_NEG:
		return desc_LeftParamB;
	}
}

} // namespace