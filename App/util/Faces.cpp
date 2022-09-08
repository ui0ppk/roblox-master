#include "stdafx.h"

#include "Util/Faces.h"
#include "util/Utilities.h"

namespace RBX
{

Faces::Faces(int normalIdMask)
	:normalIdMask(normalIdMask)
{}

void Faces::setNormalId(NormalId normalId, bool value)
{
	if(value){
		normalIdMask |= normalIdToMask(normalId);
	}
	else{
		normalIdMask &= ~normalIdToMask(normalId);
	}
}
bool Faces::getNormalId(NormalId normalId) const
{
	return (normalIdMask & (normalIdToMask(normalId))) != 0;
}

template<>
std::string StringConverter<Faces>::convertToString(const Faces& value)
{
	std::string seperator = "";
	std::string result = "";
	for(NormalId normal = NORM_X;  normal != NORM_UNDEFINED; normal = (NormalId)(normal+1)){
		if(value.getNormalId(normal)){
			result += seperator;
			switch(normal){
				case NORM_Y:		result += "Top";	break;
				case NORM_Y_NEG:	result += "Bottom";	break;
				case NORM_Z:		result += "Back";	break;
				case NORM_Z_NEG:	result += "Front";	break;
				case NORM_X:		result += "Right";	break;
				case NORM_X_NEG:	result += "Left";	break;
                default: break;
			}
			seperator = ", ";
		}
	}
	return result;
}

template<>
bool StringConverter<Faces>::convertToValue(const std::string& text, Faces& value)
{
	value.clear();

	int oldpos = 0;
	int pos = 0;
	while((pos = text.find(",", pos)) != std::string::npos){
		NormalId normal;
		if(!StringConverter<NormalId>::convertToValue(text.substr(oldpos, (pos - oldpos)), normal))
			return false;
		value.setNormalId(normal, true);
		oldpos = pos;
	}
	
	//Handle the last one
	NormalId normal;
	if(!StringConverter<NormalId>::convertToValue(text.substr(oldpos, (text.size() - oldpos)), normal))
		return false;
	value.setNormalId(normal, true);

	return true;
}
}

