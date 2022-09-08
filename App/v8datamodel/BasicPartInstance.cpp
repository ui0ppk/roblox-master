/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/BasicPartInstance.h"
#include "V8World/Primitive.h"

DYNAMIC_FASTFLAGVARIABLE(SpheresAllowedCustom, false)
DYNAMIC_FASTFLAG(FormFactorDeprecated)

namespace RBX
{

const char* const sFormFactorPart = "FormFactorPart";
const char* const sBasicPart = "Part";

using namespace Reflection;

const char* category_BasicPart = "Part ";

/*
	Unfortunately, we have 3 FormFactor properties. The reason for this
	is legacy content. For a while "FormFactor" was the STREAMING version
	and "formFactor" was the scriptable version. To support legacy files
	we override readProperty so that old files with FormFactor use the raw
	XML version of set.
*/
static const EnumPropDescriptor<FormFactorPart, FormFactorPart::FormFactor> prop_formFactorUi("FormFactor", category_BasicPart, &FormFactorPart::getFormFactor, &FormFactorPart::setFormFactorUi, PropertyDescriptor::LEGACY_SCRIPTING);
static const EnumPropDescriptor<FormFactorPart, FormFactorPart::FormFactor> prop_FormFactorLegacy("formFactor", category_BasicPart, &FormFactorPart::getFormFactor, &FormFactorPart::setFormFactorUi, PropertyDescriptor::LEGACY_SCRIPTING);
static const EnumPropDescriptor<FormFactorPart, FormFactorPart::FormFactor> prop_FormFactorData("formFactorRaw", category_BasicPart, &FormFactorPart::getFormFactor, &FormFactorPart::setFormFactorXml, PropertyDescriptor::STREAMING);

FormFactorPart::FormFactorPart()
{
}

FormFactorPart::~FormFactorPart()
{}

void FormFactorPart::readProperty(const XmlElement* propertyElement, IReferenceBinder& binder)
{
	std::string name;
	if (propertyElement->findAttributeValue(name_name, name))
	{
		// Redirect legacy FormFactor to formFactorRaw
		if (name == "FormFactor")
		{
			prop_FormFactorData.readValue(this, propertyElement, binder);
			return;
		}
	}
	Super::readProperty(propertyElement, binder);
}


void FormFactorPart::setFormFactorXml(FormFactor value)
{
	if (value != formFactor) {
		formFactor = value;
		raisePropertyChanged(prop_FormFactorData);
		raisePropertyChanged(prop_formFactorUi);
	}
}

void FormFactorPart::setFormFactorUi(FormFactor value)
{
	validateFormFactor(value);
	// Deprication of FormFactor
	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING,"FormFactor is deprecated. You should no longer use this property.");

	if (value != formFactor) {
		destroyJoints();
			setFormFactorXml(value);
			if(value != CUSTOM)
			{
				// Snap to a valid size 
				Vector3 uiSize = getPartSizeUi();
				uiSize.y = std::max(1.0f, (float)Math::iRound(uiSize.y));	// trunc
				setPartSizeUnjoined(uiSize);
			}
		join();
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



const EnumPropDescriptor<BasicPartInstance, BasicPartInstance::LegacyPartType> prop_shapXml("shap", category_BasicPart, NULL, &BasicPartInstance::setLegacyPartTypeXml, PropertyDescriptor::LEGACY); // Used to prepare for TA14264
const EnumPropDescriptor<BasicPartInstance, BasicPartInstance::LegacyPartType> BasicPartInstance::prop_shapeXml("shape", category_BasicPart, &BasicPartInstance::getLegacyPartType, &BasicPartInstance::setLegacyPartTypeXml, PropertyDescriptor::STREAMING);
static const EnumPropDescriptor<BasicPartInstance, BasicPartInstance::LegacyPartType> prop_shapeUi("Shape", category_BasicPart , &BasicPartInstance::getLegacyPartType, &BasicPartInstance::setLegacyPartTypeUi, PropertyDescriptor::UI);


BasicPartInstance::BasicPartInstance()
{
}



BasicPartInstance::~BasicPartInstance()
{}


void BasicPartInstance::validateFormFactor(FormFactor& formFactor)
{
	if (DFFlag::SpheresAllowedCustom)
	{
		if (!hasThreeDimensionalSize() && formFactor != PartInstance::CUSTOM)
		{
			formFactor = PartInstance::SYMETRIC;
		}
	}
	else if (!hasThreeDimensionalSize())
	{
		formFactor = PartInstance::SYMETRIC;
	}
}


PartType BasicPartInstance::getPartType() const
{
	switch(legacyPartType){
		case BLOCK_LEGACY_PART: return BLOCK_PART;
		case CYLINDER_LEGACY_PART: return CYLINDER_PART;
		case BALL_LEGACY_PART: return BALL_PART;
		default:
			RBXASSERT(0);
			return BLOCK_PART;
	}
}

void BasicPartInstance::setLegacyPartTypeXml(LegacyPartType _type)
{
	if (legacyPartType != _type) 
	{
		legacyPartType = _type;

		getPartPrimitive()->setGeometryType(legacyPartType == BLOCK_LEGACY_PART 
											? Geometry::GEOMETRY_BLOCK
                                            : legacyPartType == CYLINDER_LEGACY_PART
                                            ? Geometry::GEOMETRY_CYLINDER
											: Geometry::GEOMETRY_BALL);

		if (!DFFlag::FormFactorDeprecated) {
			if (DFFlag::SpheresAllowedCustom)
			{
				if (!hasThreeDimensionalSize() && getFormFactor() != PartInstance::CUSTOM)
				{
					setFormFactorXml(PartInstance::SYMETRIC);
				}
			} 
			else if (!hasThreeDimensionalSize())
			{
				setFormFactorXml(PartInstance::SYMETRIC);
			}
		}

		raisePropertyChanged(prop_shapeXml);
		raisePropertyChanged(prop_shapeUi);

		shouldRenderSetDirty();
	}
}

void BasicPartInstance::setLegacyPartTypeUi(LegacyPartType _type)
{
    if (!hasThreeDimensionalSize()) {
		RBXASSERT(formFactor == PartInstance::SYMETRIC);
	}

	if (legacyPartType != _type) {
		destroyJoints();

			// 1.  Set the new type / Make form factor symetric for balls, cylinders (form factor may change)
			setLegacyPartTypeXml(_type);

			// 2.  Make the dimensions uniform for balls, cyclinders (size may change)
			if (!hasThreeDimensionalSize()) {
				setPartSizeUnjoined(getPartSizeUi());
			}
			safeMove();

		join();
	}
}


bool BasicPartInstance::hasThreeDimensionalSize()
{
    return legacyPartType != BALL_LEGACY_PART;
}

} //namespace
