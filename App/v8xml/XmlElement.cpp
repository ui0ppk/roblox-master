#include "stdafx.h"

#include "RbxAssert.h"
#include "V8Xml/XmlElement.h"
#include "V8Xml/XmlSerializer.h"
#include "Util/Guid.h"
#include "Util/Utilities.h"
#include "rbx/Debug.h"
#include <stdlib.h>
#include <stdio.h>


using std::string;
using std::vector;
using namespace G3D;

using namespace RBX;

const RBX::Name& value_IDREF_null = Name::declare("null");
const RBX::Name& value_IDREF_nil = Name::declare("nil");

const XmlTag& name_xsinil = Name::declare("xsi:nil");
const XmlTag& name_xsitype = Name::declare("xsi:type");
const XmlTag& tag_xmlnsxsi = Name::declare("xmlns:xsi");

// TODO: Put these in a file that knows about the Roblox schema
const XmlTag& name_root = Name::declare("root");
const XmlTag& name_referent = Name::declare("referent");
const XmlTag& tag_roblox = Name::declare("roblox");
const XmlTag& tag_version = Name::declare("version");
const XmlTag& tag_assettype = Name::declare("assettype");
const XmlTag& tag_External = Name::declare("External");
const XmlTag& name_Ref = Name::declare("Ref");
const XmlTag& name_token = Name::declare("token");
const XmlTag& name_name = Name::declare("name");
const XmlTag& tag_Refs = Name::declare("Refs");
const XmlTag& tag_X = Name::declare("X");
const XmlTag& tag_Y = Name::declare("Y");
const XmlTag& tag_Z = Name::declare("Z");

const XmlTag& tag_R00 = Name::declare("R00");
const XmlTag& tag_R01 = Name::declare("R01");
const XmlTag& tag_R02 = Name::declare("R02");
const XmlTag& tag_R10 = Name::declare("R10");
const XmlTag& tag_R11 = Name::declare("R11");
const XmlTag& tag_R12 = Name::declare("R12");
const XmlTag& tag_R20 = Name::declare("R20");
const XmlTag& tag_R21 = Name::declare("R21");
const XmlTag& tag_R22 = Name::declare("R22");

const XmlTag& tag_R = Name::declare("R");
const XmlTag& tag_G = Name::declare("G");
const XmlTag& tag_B = Name::declare("B");
const XmlTag& tag_class = Name::declare("class");
const XmlTag& tag_Item = Name::declare("Item");
const XmlTag& tag_Properties = Name::declare("Properties");
const XmlTag& tag_Feature = Name::declare("Feature");

const XmlTag& tag_hash = Name::declare("hash");
const XmlTag& tag_null = Name::lookup("null");		// already declared elsewhere
const XmlTag& tag_mimeType = Name::declare("mimeType");

const XmlTag& tag_S = Name::declare("S");
const XmlTag& tag_O = Name::declare("O");

const XmlTag& tag_XS = Name::declare("XS");
const XmlTag& tag_XO = Name::declare("XO");
const XmlTag& tag_YS = Name::declare("YS");
const XmlTag& tag_YO = Name::declare("YO");

const XmlTag& tag_faces = Name::declare("faces");
const XmlTag& tag_axes = Name::declare("axes");

const XmlTag& tag_Origin = Name::declare("origin");
const XmlTag& tag_Direction = Name::declare("direction");

const XmlTag& tag_Min = Name::declare("min");
const XmlTag& tag_Max = Name::declare("max");

const XmlTag& tag_WebTable = Name::declare("Table");
const XmlTag& tag_WebList = Name::declare("List");
const XmlTag& tag_WebEntry = Name::declare("Entry");
const XmlTag& tag_WebKey = Name::declare("Key");
const XmlTag& tag_WebValue = Name::declare("Value");
const XmlTag& tag_WebType = Name::declare("Type");

const XmlTag& tag_customPhysProp            = Name::declare("CustomPhysics");
const XmlTag& tag_customDensity				= Name::declare("Density");
const XmlTag& tag_customFriction			= Name::declare("Friction");
const XmlTag& tag_customElasticity			= Name::declare("Elasticity");
const XmlTag& tag_customFrictionWeight		= Name::declare("FrictionWeight");
const XmlTag& tag_customElasticityWeight    = Name::declare("ElasticityWeight");

const XmlTag& tag_xsinoNamespaceSchemaLocation = Name::declare("xsi:noNamespaceSchemaLocation");





///////////////////////////////////////////////////////////////////////////

bool XmlElement::isXsiNil() const {
	const XmlAttribute* xnil = findAttribute(name_xsinil);
	bool isNil;
	return xnil!=NULL && xnil->getValue(isNil) && isNil;
}

const XmlElement* XmlElement::findFirstChildByTag(const XmlTag& _tag) const {
	for (const XmlElement* child = firstChild(); child!=NULL; child = child->nextSibling())
		if (child->getTag()==_tag)
			return child;
	return NULL;
}
const XmlElement* XmlElement::findNextChildWithSameTag(const XmlElement* node) const {
	for (const XmlElement* child = node->nextSibling(); child!=NULL; child = child->nextSibling())
		if (child->getTag()==node->getTag())
			return child;
	return NULL;
}

const XmlAttribute* XmlElement::findAttribute(const XmlTag& _tag) const {
	for (const XmlAttribute* attribute = getFirstAttribute(); attribute!=NULL; attribute = getNextAttribute(attribute))
		if (attribute->getTag()==_tag)
			return attribute;
	return NULL;
}

XmlAttribute* XmlElement::findAttribute(const XmlTag& _tag) {
	for (XmlAttribute* attribute = getFirstAttribute(); attribute!=NULL; attribute = getNextAttribute(attribute))
		if (attribute->getTag()==_tag)
			return attribute;
	return NULL;
}

void XmlNameValuePair::clearValue() const {
	switch (valueType) {
	case STRING:
		delete stringValue;
		break;
	case CONTENTID:
		delete contentIdValue;
		break;
	case HANDLE:
		delete handleValue;
		break;
    default:
        break;
	}
	valueType = NONE;
}

bool XmlNameValuePair::isValueEqual(const RBX::Name* value) const {
	switch (valueType) {
	case STRING:
		return *value==*stringValue;
	case NAME:
		return *value==*nameValue;
	default:
		return false;
	}
}

bool XmlNameValuePair::getValue(const RBX::Name*& value) const
{
	if (valueType==NAME) {
		value = nameValue;
		return true;
	}
	if (valueType==STRING) {
		value = &RBX::Name::declare(stringValue->c_str());
		clearValue();
		nameValue = value;
		valueType = NAME;
		return true;
	}
	return false;
}

template<>
bool XmlNameValuePair::isValueType<ContentId>() const
{
	return valueType==CONTENTID;
}

template<>
bool XmlNameValuePair::isValueType<std::string>() const
{
	return valueType==STRING;
}

template<>
bool XmlNameValuePair::isValueType<int>() const
{
	return valueType==INT;
}

template<>
bool XmlNameValuePair::isValueType<unsigned int>() const
{
	return valueType==UINT;
}

template<>
bool XmlNameValuePair::isValueType<bool>() const
{
	return valueType==BOOL;
}

template<>
bool XmlNameValuePair::isValueType<float>() const
{
	return valueType==FLOAT;
}

template<>
bool XmlNameValuePair::isValueType<double>() const
{
	return valueType==DOUBLE;
}

template<>
bool XmlNameValuePair::isValueType<const RBX::Name*>() const
{
	return valueType==NAME;
}

template<>
bool XmlNameValuePair::isValueType<RBX::InstanceHandle>() const
{
	return valueType==HANDLE;
}



bool XmlNameValuePair::getValue(RBX::ContentId& value) const
{
	if (valueType==CONTENTID) {
		value = *contentIdValue;
		return true;
	}

	if (valueType==STRING) {
		value = ContentId(*stringValue);
		clearValue();
		contentIdValue = new ContentId(value);
		valueType = CONTENTID;
		return true;
	}

	return false;
}

bool XmlNameValuePair::getValue(std::string& value) const
{
	if (valueType==STRING) {
		value = *stringValue;
		return true;
	}
	if (valueType==NAME) {
		value = nameValue ? nameValue->c_str() : "";
		return true;
	}
	return false;
}

bool XmlNameValuePair::getValue(int& value) const
{
	if (valueType==INT) {
		value = intValue;
		return true;
	}

	if (valueType==STRING) {
		if (StringConverter<int>::convertToValue(*stringValue, value)) {
			clearValue();
			intValue = value;
			valueType = INT;
			return true;
		}
	}

	return false;
}

bool XmlNameValuePair::getValue(unsigned int& value) const
{
	if (valueType==UINT) {
		value = uintValue;
		return true;
	}

	if (valueType==STRING) {
		if (StringConverter<unsigned int>::convertToValue(*stringValue, value)) {
			clearValue();
			uintValue = value;
			valueType = UINT;
			return true;
		}
	}

	return false;
}

bool XmlNameValuePair::getValue(bool& value) const
{
	if (valueType==BOOL) {
		value = boolValue;
		return true;
	}

	if (valueType==STRING) {
		if (StringConverter<bool>::convertToValue(*stringValue, value)) {
			clearValue();
			boolValue = value;
			valueType = BOOL;
			return true;
		}
	}

	return false;
}

bool XmlNameValuePair::getValue(float& value) const
{
	if (valueType==FLOAT) {
		value = floatValue;
		return true;
	}

	if (valueType==STRING) {
		if (RBX::StringConverter<float>::convertToValue(*stringValue, value)) {
			clearValue();
			floatValue = value;
			valueType = FLOAT;
			return true;
		}
	}

	RBXASSERT(valueType!=DOUBLE);	// No provision (yet) for converting from double back to float

	return false;
}

bool XmlNameValuePair::getValue(double& value) const
{
	if (valueType==DOUBLE) {
		value = doubleValue;
		return true;
	}

	if (valueType==FLOAT) {
		value = (double)floatValue;
		clearValue();
		doubleValue = value;
		valueType = DOUBLE;
		return true;
	}

	if (valueType==STRING) {
		if (RBX::StringConverter<double>::convertToValue(*stringValue, value)) {
			clearValue();
			doubleValue = value;
			valueType = DOUBLE;
			return true;
		}
	}

	return false;
}

bool XmlNameValuePair::getValue(RBX::InstanceHandle &value) const {
	
	if (valueType==NAME && *nameValue==value_IDREF_null) {
		clearValue();
		handleValue = new RBX::InstanceHandle(NULL);
		valueType = HANDLE;
	} else if (valueType==STRING && value_IDREF_null==*stringValue) {
		clearValue();
		handleValue = new RBX::InstanceHandle(NULL);
		valueType = HANDLE;
	} else if (valueType==STRING && *stringValue=="") {
		// legacy files didn't use the "null" keyword
		clearValue();
		handleValue = new RBX::InstanceHandle(NULL);
		valueType = HANDLE;
	}

	if (valueType==HANDLE) {
		value = *handleValue;
		return true;
	} 
	else
		return false;
}


std::string XmlNameValuePair::toString(XmlWriter* writer) const
{
	switch (valueType)
	{
	case BOOL:
		{
			 return RBX::StringConverter<bool>::convertToString(boolValue);
		};

	case INT:
		{
			 return RBX::StringConverter<int>::convertToString(intValue);
		};

	case UINT:
		{
			 return RBX::StringConverter<unsigned int>::convertToString(uintValue);
		};

	case FLOAT:
		{
			 return RBX::StringConverter<float>::convertToString(floatValue);
		};

	case DOUBLE:
		{
			 return RBX::StringConverter<double>::convertToString(doubleValue);
		};

	case NAME:
		return nameValue->toString();

	case HANDLE:
		if (handleValue->getTarget()==NULL)
			return value_IDREF_null.toString();
		else {
			shared_ptr<Reflection::DescribedBase> base = handleValue->getTarget();
			const std::string* lastId = base->getXmlId();
				
			if (lastId == NULL || !writer->isValidId(*lastId, *handleValue))
			{
				std::string newId;
				Guid::generateRBXGUID(newId);
				RBXASSERT(writer->isValidId(newId, *handleValue));

				// set the id back into the object in case we serialize again before loading
				base->setXmlId(newId);
				lastId = base->getXmlId();
				RBXASSERT(lastId != NULL);
			}
				
			writer->recordId(*lastId, *handleValue);
			return *lastId;
		}

	case STRING:
		return *stringValue;

	case CONTENTID:
		return contentIdValue->toString();

	case NONE:
		return "";

	default:
		RBXASSERT(false);
		return "";
	}
}


