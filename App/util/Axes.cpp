#include "stdafx.h"
#include "Util/Axes.h"
#include "util/Utilities.h"
#include "Reflection/Type.h"
#include "Reflection/EnumConverter.h"

namespace RBX
{

Axes::Axes(int axisMask)
	:axisMask(axisMask)
{}

Vector3::Axis Axes::normalIdToAxis(NormalId normalId)
{
	switch(normalId){
		case NORM_X:
		case NORM_X_NEG:
			return Vector3::X_AXIS;
		case NORM_Y:
		case NORM_Y_NEG:
			return Vector3::Y_AXIS;
		case NORM_Z:
		case NORM_Z_NEG:
			return Vector3::Z_AXIS;
		default:
			return Vector3::DETECT_AXIS;
	}
}

NormalId Axes::axisToNormalId(Vector3::Axis axis)
{
	switch(axis){
		case Vector3::X_AXIS:
			return NORM_X;
		case Vector3::Y_AXIS:
			return NORM_Y;
		case Vector3::Z_AXIS:
			return NORM_Z;
		default:
			return NORM_UNDEFINED;
	}
}

int Axes::axisToMask(Vector3::Axis axis)
{
	switch(axis)
	{
	case Vector3::X_AXIS:
	case Vector3::Y_AXIS:
	case Vector3::Z_AXIS:
		return (1<< (int)axis);
	default:
		return 0;
	}
}
void Axes::setAxisByNormalId(NormalId normalId, bool value)
{
	if(value){
		axisMask |= axisToMask(normalIdToAxis(normalId));
	}
	else{
		axisMask &= ~axisToMask(normalIdToAxis(normalId));
	}
}
bool Axes::getAxisByNormalId(NormalId normalId) const
{
	return (axisMask & axisToMask(normalIdToAxis(normalId))) != 0;
}

void Axes::setAxis(Vector3::Axis axis, bool value)
{
	if(value){
		axisMask |= axisToMask(axis);
	}
	else{
		axisMask &= ~axisToMask(axis);
	}
}
bool Axes::getAxis(Vector3::Axis axis) const
{
	return (axisMask & axisToMask(axis)) != 0;
}

template<>
std::string StringConverter<Axes>::convertToString(const Axes& value)
{
	std::string seperator = "";
	std::string result = "";
	for(int axis = 0; axis < 3; axis++){
		if(value.getAxis((RBX::Vector3::Axis)axis)){
			result += seperator;
			switch((RBX::Vector3::Axis)axis){
				case RBX::Vector3::X_AXIS:		result += "X";	break;
				case RBX::Vector3::Y_AXIS:		result += "Y";	break;
				case RBX::Vector3::Z_AXIS:		result += "Z";	break;
                default: break;
			}
			seperator = ", ";
		}
	}
	return result;
}

namespace Reflection
{
	template<>
	EnumDesc<RBX::Vector3::Axis>::EnumDesc()
	:EnumDescriptor("Axis")
	{
		addPair(RBX::Vector3::X_AXIS, "X");
		addPair(RBX::Vector3::Y_AXIS, "Y");
		addPair(RBX::Vector3::Z_AXIS, "Z");
	}

	template<>
	RBX::Vector3::Axis& Variant::convert<RBX::Vector3::Axis>(void)
	{
		return genericConvert<RBX::Vector3::Axis>();
	}
}

template<>
bool RBX::StringConverter<G3D::Vector3::Axis>::convertToValue(const std::string& text, RBX::Vector3::Axis& value)
{
	if(text.find("Y") != std::string::npos || text.find("Top") != std::string::npos || text.find("Bottom") != std::string::npos){
		value = RBX::Vector3::Y_AXIS;
		return true;
	}
	if(text.find("Z") != std::string::npos || text.find("Back") != std::string::npos || text.find("Front") != std::string::npos ){
		value = RBX::Vector3::Z_AXIS;
		return true;
	}
	if(text.find("X") != std::string::npos || text.find("Right") != std::string::npos || text.find("Left") != std::string::npos ){
		value = RBX::Vector3::X_AXIS;
		return true;
	}
	return false;
}
template<>
bool StringConverter<RBX::Axes>::convertToValue(const std::string& text, Axes& value)
{
	value.clear();

	size_t oldpos = 0;
	size_t pos = 0;
	while((pos = text.find(",", oldpos)) != std::string::npos){
		RBX::Vector3::Axis axis;
		if(!StringConverter<G3D::Vector3::Axis>::convertToValue(text.substr(oldpos, (pos - oldpos)), axis))
			return false;
		value.setAxis(axis, true);
		oldpos = pos+1;
	}
	
	//Handle the last one
	RBX::Vector3::Axis axis;
	if(!StringConverter<G3D::Vector3::Axis>::convertToValue(text.substr(oldpos, (text.size() - oldpos)), axis))
		return false;
	value.setAxis(axis, true);

	return true;
}

}

