#include "stdafx.h"

#include "V8Tree/Property.h"
#include "rbx/Debug.h"

#include "Util/BrickColor.h"
#include "Util/Region3.h"
#include "Util/Region3int16.h"
#include "util/CellID.h"
#include "util/PhysicalProperties.h"

using namespace G3D;

/*
// Stub function. Should never get called
template<>
bool XmlElement::getValue(Color3& value) const
{
	RBXASSERT(false);
	return false;
}

// Stub function. Should never get called
template<>
void XmlElement::setValue(Color3 value)
{
	RBXASSERT(false);
}
*/

namespace RBX {



template<>
std::string StringConverter<G3D::Vector2int16>::convertToString(const G3D::Vector2int16& value)
{
	std::string result = StringConverter<int>::convertToString(value[0]);
	result += ", ";
	result += StringConverter<int>::convertToString(value[1]);
	return result;
}
template<>
bool StringConverter<G3D::Vector2int16>::convertToValue(const std::string& text, G3D::Vector2int16& value)
{
	int pos = text.find_first_of(",;", 0);
	if (pos<0)
		return false;
	value[0] = atoi(text.substr(0, pos).c_str());

	int last = pos+1;
	pos = text.length();
	if (pos<last)
		return false;
	value[1] = atoi(text.substr(last, pos-last).c_str());

	return true;
}

template<>
std::string StringConverter<G3D::Vector3int16>::convertToString(const G3D::Vector3int16& value)
{
	std::string result = StringConverter<int>::convertToString(value[0]);
	result += ", ";
	result += StringConverter<int>::convertToString(value[1]);
	result += ", ";
	result += StringConverter<int>::convertToString(value[2]);
	return result;
}
template<>
bool StringConverter<G3D::Vector3int16>::convertToValue(const std::string& text, G3D::Vector3int16& value)
{
	int pos = text.find_first_of(",;", 0);
	if (pos<0)
		return false;
	value[0] = atoi(text.substr(0, pos).c_str());

	int last = pos+1;
	pos = text.find_first_of(",;", last);
	if (pos<last)
		return false;
	value[1] = atoi(text.substr(last, pos-last).c_str());

	last = pos+1;
	pos = text.length();
	if (pos<last)
		return false;
	value[2] = atoi(text.substr(last, pos-last).c_str());

	return true;
}

template<>
std::string StringConverter<G3D::Vector3>::convertToString(const G3D::Vector3& value)
{
	std::string result = StringConverter<float>::convertToString(value[0]);
	result += ", ";
	result += StringConverter<float>::convertToString(value[1]);
	result += ", ";
	result += StringConverter<float>::convertToString(value[2]);
	return result;
}


template<>
std::string StringConverter<RBX::Vector2>::convertToString(const RBX::Vector2& value)
{
	std::string result = StringConverter<float>::convertToString(value[0]);
	result += ", ";
	result += StringConverter<float>::convertToString(value[1]);
	return result;
}
    
template<>
std::string StringConverter<RBX::Rect2D>::convertToString(const RBX::Rect2D& value)
{
    std::string result = StringConverter<float>::convertToString(value.x0());
	result += ", ";
	result += StringConverter<float>::convertToString(value.y0());
	result += ", ";
	result += StringConverter<float>::convertToString(value.x1());
    result += ", ";
	result += StringConverter<float>::convertToString(value.y1());
	return result;
}

template<>
std::string StringConverter<PhysicalProperties>::convertToString(const PhysicalProperties& value)
{
	if (value.getCustomEnabled())
	{
		std::string result = StringConverter<float>::convertToString(value.getDensity());
		result +=", ";
		result += StringConverter<float>::convertToString(value.getFriction());
		result +=", ";
		result += StringConverter<float>::convertToString(value.getElasticity());
		result +=", ";
		result += StringConverter<float>::convertToString(value.getFrictionWeight());
		result +=", ";
		result += StringConverter<float>::convertToString(value.getElasticityWeight());

		return result;
	}
	
	return std::string("default");
}

template<>
bool StringConverter<PhysicalProperties>::convertToValue(const std::string& text, PhysicalProperties& value)
{
	return false;
}

template<>
std::string StringConverter<G3D::CoordinateFrame>::convertToString(const G3D::CoordinateFrame& value)
{
	std::string result = StringConverter<G3D::Vector3>::convertToString(value.translation);
	for (int i=0; i<3; ++i)
		for (int j=0; j<3; ++j)
		{
			result += ", ";
			result += StringConverter<float>::convertToString(value.rotation[i][j]);
		}
	return result;
}

template<>
bool StringConverter<RBX::CoordinateFrame>::convertToValue(const std::string& text, RBX::CoordinateFrame& value)
{
	return false;
}



template<>
std::string StringConverter<RBX::Region3>::convertToString(const RBX::Region3& value)
{
	std::string result = StringConverter<RBX::CoordinateFrame>::convertToString(value.getCFrame());
	result += "; ";
	result += StringConverter<RBX::Vector3>::convertToString(value.getSize());
	return result;
}
template<>
bool StringConverter<RBX::Region3>::convertToValue(const std::string& text, RBX::Region3& value)
{
	return false;
}

template<>
std::string StringConverter<RBX::Region3int16>::convertToString(const RBX::Region3int16& value)
{
	std::string result = StringConverter<G3D::Vector3int16>::convertToString(value.getMinPos());
	result += "; ";
	result += StringConverter<Vector3int16>::convertToString(value.getMaxPos());
	return result;
}
template<>
bool StringConverter<RBX::Region3int16>::convertToValue(const std::string& text, RBX::Region3int16& value)
{
	return false;
}


template<>
bool StringConverter<G3D::Vector3>::convertToValue(const std::string& text, G3D::Vector3& value)
{
	int pos = text.find_first_of(",;", 0);
	if (pos<0)
		return false;
	value[0] = (float)atof(text.substr(0, pos).c_str());

	int last = pos+1;
	pos = text.find_first_of(",;", last);
	if (pos<last)
		return false;
	value[1] = (float)atof(text.substr(last, pos-last).c_str());

	last = pos+1;
	pos = text.length();
	if (pos<last)
		return false;
	value[2] = (float)atof(text.substr(last, pos-last).c_str());

	return true;
}

template<>
bool StringConverter<RBX::Vector2>::convertToValue(const std::string& text, RBX::Vector2& value)
{
	int pos = text.find_first_of(",;", 0);
	if (pos<0)
		return false;
	value[0] = (float)atof(text.substr(0, pos).c_str());

	int last = pos+1;
	pos = text.length();
	if (pos<last)
		return false;
	value[1] = (float)atof(text.substr(last, pos-last).c_str());

	return true;
}

template<>
bool StringConverter<G3D::Rect2D>::convertToValue(const std::string& text, G3D::Rect2D& value)
{
    float x0,y0,x1,y1;
    int pos = text.find_first_of(",;", 0);
    if (pos<0)
        return false;
    x0 = (float)atof(text.substr(0, pos).c_str());
    
    int last = pos+1;
	pos = text.find_first_of(",;", last);
    if (pos<last)
        return false;
    y0 = (float)atof(text.substr(last, pos).c_str());
    
    last = pos+1;
	pos = text.find_first_of(",;", last);
    if (pos<last)
        return false;
    x1 = (float)atof(text.substr(last, pos).c_str());
    
    last = pos+1;
    pos = text.length();
    if (pos<last)
        return false;
    y1 = (float)atof(text.substr(last, pos-last).c_str());
    
    value = G3D::Rect2D::xyxy(x0, y0, x1, y1);
    
    return true;
}


template<>
std::string RBX::StringConverter<RBX::RbxRay>::convertToString(const RBX::RbxRay& value)
{
	std::string result = "{" + RBX::StringConverter<RBX::Vector3>::convertToString(value.origin()) + "}";
	result += ", ";
	result += "{" + RBX::StringConverter<RBX::Vector3>::convertToString(value.direction()) + "}";
	return result;
}

template<>
bool RBX::StringConverter<RBX::RbxRay>::convertToValue(const std::string& text, RBX::RbxRay& value)
{
	int openBracket = text.find_first_of("{", 0);
	if (openBracket < 0)
		return false;
	int closeBracket = text.find_first_of("}", openBracket);
	if(closeBracket < 0)
		return false;
	if(!RBX::StringConverter<RBX::Vector3>::convertToValue(text.substr(openBracket+1, closeBracket-openBracket-1), value.origin()))
		return false;

	int last = closeBracket+1;
	openBracket = text.find_first_of("{", last);
	if (openBracket < 0)
		return false;
	closeBracket = text.find_first_of("}", openBracket);
	if(closeBracket < 0)
		return false;
	if(!RBX::StringConverter<RBX::Vector3>::convertToValue(text.substr(openBracket+1, closeBracket-openBracket-1), value.direction()))
		return false;

	return true;
}

template<>
std::string StringConverter<RBX::BrickColor>::convertToString(const RBX::BrickColor& value)
{
	return value.name();
}

////////////////////////////////////////////////////////////////////////////////
//
// StringConverter<CellID>
//

template<>
std::string RBX::StringConverter<RBX::CellID>::convertToString(const RBX::CellID& value)
{
	std::string result = "{" + RBX::StringConverter<bool>::convertToString(value.getIsNil()) + "}";
	result += ", ";
	result += "{" + RBX::StringConverter<RBX::Vector3>::convertToString(value.getLocation()) + "}";
	return result;
}

template<>
bool RBX::StringConverter<RBX::CellID>::convertToValue(const std::string& text, RBX::CellID& value)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////
//
// StringConverter<G3D::Color3>
//

template<>
std::string StringConverter<G3D::Color3>::convertToString(const G3D::Color3& value)
{
	char szText[64];
#ifdef _WIN32
	_snprintf(szText, 64, "%g, %g, %g", value[0], value[1], value[2]);
#else	
	snprintf(szText, 64, "%g, %g, %g", value[0], value[1], value[2]);
#endif	
	return szText;
}

template<>
bool StringConverter<G3D::Color3>::convertToValue(const std::string& text, G3D::Color3& value)
{
	if (text[0]=='#')
	{
		// This is an HTML color: #rrggbb
		if (text.size()!=7)
			return false;
		value[0] = static_cast<float>(strtol(text.substr(1,2).c_str(), NULL, 16))/255.0f; 
		value[1] = static_cast<float>(strtol(text.substr(3,2).c_str(), NULL, 16))/255.0f; 
		value[2] = static_cast<float>(strtol(text.substr(5,2).c_str(), NULL, 16))/255.0f; 
		return true;
	}

	if (text.substr(0,2)=="0x")
	{
		// This is a color of the form 0xrrggbb
		if (text.size()!=8)
			return false;
		value[0] = static_cast<float>(strtol(text.substr(2,2).c_str(), NULL, 16))/255.0f; 
		value[1] = static_cast<float>(strtol(text.substr(4,2).c_str(), NULL, 16))/255.0f; 
		value[2] = static_cast<float>(strtol(text.substr(6,2).c_str(), NULL, 16))/255.0f; 
		return true;
	}

	int pos = text.find_first_of(",;", 0);
	if (pos<0)
		return false;
	value[0] = static_cast<float>(atof(text.substr(0, pos).c_str()));

	int last = pos+1;
	pos = text.find_first_of(",;", last);
	if (pos<last)
		return false;
	value[1] = static_cast<float>(atof(text.substr(last, pos-last).c_str()));

	last = pos+1;
	pos = text.length();
	if (pos<last)
		return false;
	value[2] = static_cast<float>(atof(text.substr(last, pos-last).c_str()));

	return true;
}


template<>
bool StringConverter<RBX::ContentId>::convertToValue(const std::string& text, RBX::ContentId& value)
{
	value = ContentId(text);
	return true;
}

template<>
bool StringConverter<RBX::BrickColor>::convertToValue(const std::string& text, RBX::BrickColor& value)
{
	// TODO: Is this the right thing to do?  Maybe we should just return false
	value = RBX::BrickColor::parse(text.c_str());
	return true;
}

template<>
std::string StringConverter< RBX::ContentId >::convertToString(const RBX::ContentId& value)
{
	return value.toString();
}




}	// namespace RBX