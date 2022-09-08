#include "stdafx.h"

#include "Voxel/Serializer.h"

namespace RBX { namespace Voxel {

const unsigned char SerializerConstants::kNewCellMarker = 0x01;
const unsigned char SerializerConstants::kRepeatCellMarker = 0x02;
const unsigned char SerializerConstants::kEndSequenceMarker = 0x03;
const unsigned int SerializerConstants::kRecentlyEncodedReferenceBits = 3;

} }
