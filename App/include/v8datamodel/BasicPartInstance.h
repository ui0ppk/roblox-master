/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/PartInstance.h"

namespace RBX {

extern const char* const sFormFactorPart;


class FormFactorPart 
	: public DescribedNonCreatable<FormFactorPart, PartInstance, sFormFactorPart>
{
	typedef DescribedNonCreatable<FormFactorPart, PartInstance, sFormFactorPart> Super;
public:
	/*override*/ virtual FormFactor getFormFactor() const { return formFactor; }

	void setFormFactorUi(FormFactor value);
	void setFormFactorXml(FormFactor value);

	FormFactorPart();
	virtual ~FormFactorPart();

	/* override */ void readProperty(const XmlElement* propertyElement, IReferenceBinder& binder);

protected:
	virtual void validateFormFactor(FormFactor& value) {}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const char* const sBasicPart;

class BasicPartInstance
	: public DescribedCreatable<BasicPartInstance, FormFactorPart, sBasicPart>
{
private:
	/*override*/ void validateFormFactor(FormFactor& value);

public:
	static const Reflection::EnumPropDescriptor<BasicPartInstance, LegacyPartType> prop_shapeXml;

	BasicPartInstance();
	virtual ~BasicPartInstance();

	/*override*/ virtual bool hasThreeDimensionalSize();

	void setLegacyPartTypeUi(LegacyPartType _type);	
	void setLegacyPartTypeXml(LegacyPartType _type);

	/*override*/ virtual PartType getPartType() const;
		
	LegacyPartType getLegacyPartType() const {return legacyPartType; }
};



} // namespace
