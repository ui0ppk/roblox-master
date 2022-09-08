//
//  StreamingUtil.h
//  Network
//
//  Created by Martin Robaszewski on 5/2/13.
//  Copyright (c) 2013 Roblox Inc. All rights reserved.
//

#ifndef Network_StreamingUtil_h
#define Network_StreamingUtil_h

#include "bitstream.h"

namespace RBX
{
	class BrickColor;
	class UDim;
	class UDim2;
	class Faces;
	class Axes;
	class SystemAddress;
    class BinaryString;
    class NumberSequence;
    class ColorSequence;
    class NumberRange;
    class NumberSequenceKeypoint;
    class ColorSequenceKeypoint;
	class PhysicalProperties;

	namespace StreamRegion {
		class Id;
	}
    
    
	template<class T>
	RakNet::BitStream& operator >> (RakNet::BitStream& stream, T& value);
    
    RakNet::BitStream& operator << (RakNet::BitStream& stream, char value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, signed char value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, unsigned char value);

    RakNet::BitStream& operator << (RakNet::BitStream& stream, short value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, unsigned short value);
    
    RakNet::BitStream& operator << (RakNet::BitStream& stream, int value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, unsigned int value);

    RakNet::BitStream& operator << (RakNet::BitStream& stream, unsigned long long value);

    RakNet::BitStream& operator << (RakNet::BitStream& stream, bool value);

	RakNet::BitStream& operator << (RakNet::BitStream& stream, float value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, double value);

    RakNet::BitStream& operator << (RakNet::BitStream& stream, const RBX::Guid::Scope& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const BinaryString& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const std::string& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const ContentId& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const StreamRegion::Id& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Vector3& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Vector2& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Vector3int16& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Vector2int16& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::Color3& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const G3D::CoordinateFrame& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const RBX::Velocity& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, RBX::SystemAddress value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const BrickColor& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const UDim& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const UDim2& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const RBX::RbxRay& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const Faces& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const Axes& value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, const NumberSequence& value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, const ColorSequence& value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, const NumberRange& value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, const NumberSequenceKeypoint& value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, const ColorSequenceKeypoint& value);
	RakNet::BitStream& operator << (RakNet::BitStream& stream, const Rect2D& value);
    RakNet::BitStream& operator << (RakNet::BitStream& stream, const PhysicalProperties& value);
        
}

#endif
