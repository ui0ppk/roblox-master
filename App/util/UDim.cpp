#include "stdafx.h"

#include "Util/UDim.h"
#include "util/Utilities.h"

#include <sstream>

using namespace RBX;

namespace RBX
{
	template<>
	std::string StringConverter<UDim>::convertToString(const UDim& value)
	{
		std::string result = StringConverter<float>::convertToString(value.scale);
		result += ", ";
		result += StringConverter<int>::convertToString(value.offset);
		return result;
	}


	template<>
	bool StringConverter<UDim>::convertToValue(const std::string& text, UDim& value)
	{
		int pos = text.find_first_of(",;", 0);
		if (pos<0)
			return false;
		value.scale = (float)atof(text.substr(0, pos).c_str());

		int last = pos+1;
		pos = text.length();
		if (pos<last)
			return false;
		value.offset = (int)atoi(text.substr(last, pos-last).c_str());

		return true;
	}


	template<>
	std::string StringConverter<UDim2>::convertToString(const UDim2& value)
	{
		std::string result = "{" + StringConverter<UDim>::convertToString(value.x) + "}";
		result += ", ";
		result += "{" + StringConverter<UDim>::convertToString(value.y) + "}";
		return result;
	}


	template<>
	bool StringConverter<UDim2>::convertToValue(const std::string& text, UDim2& value)
	{
		int openBracket = text.find_first_of("{", 0);
		if (openBracket < 0)
			return false;
		int closeBracket = text.find_first_of("}", openBracket);
		if(closeBracket < 0)
			return false;
		if(!StringConverter<UDim>::convertToValue(text.substr(openBracket+1, closeBracket-openBracket-1), value[0]))
			return false;

		int last = closeBracket+1;
		openBracket = text.find_first_of("{", last);
		if (openBracket < 0)
			return false;
		closeBracket = text.find_first_of("}", openBracket);
		if(closeBracket < 0)
			return false;
		if(!StringConverter<UDim>::convertToValue(text.substr(openBracket+1, closeBracket-openBracket-1), value[1]))
			return false;

		return true;
	}
}//namespace RBX

G3D::int16 UDim::transform(G3D::int16 rhs) const
{
	return G3D::int16(this->scale*rhs) + this->offset;
}

UDim UDim::operator*(const G3D::int16 rhs) const
{
	return UDim(this->scale*rhs, this->offset*rhs);
}

float UDim::transform(const float value) const
{
	return this->scale*value + this->offset;
}

UDim UDim::operator*(float rhs) const
{
	return UDim(this->scale*rhs, this->offset*rhs);
}
UDim UDim::operator+ (const UDim& v) const
{
	return UDim(scale+v.scale, offset+v.offset);
}
UDim UDim::operator- (const UDim& v) const
{
	return UDim(scale-v.scale, offset-v.offset);
}
UDim UDim::operator- () const
{
	return UDim(-scale, -offset);
}


G3D::Vector2int16 UDim2::operator*(const G3D::Vector2int16 rhs) const
{
	return G3D::Vector2int16(this->x.transform(rhs.x), this->y.transform(rhs.y));
}

G3D::Vector2 UDim2::operator*(const G3D::Vector2 rhs) const
{
	return G3D::Vector2(this->x.transform(rhs.x), this->y.transform(rhs.y));
}
UDim2 UDim2::operator* (float v) const
{
	return UDim2(x*v, y*v);
}
	

UDim2 UDim2::operator+ (const UDim2& v) const
{
	return UDim2(x+v.x, y+v.y);
}
UDim2 UDim2::operator- (const UDim2& v) const
{
	return UDim2(x-v.x, y-v.y);
}
UDim2 UDim2::operator- () const
{
	return UDim2(-x, -y);
}

