#include "stdafx.h"

#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/ExtrudedPartInstance.h"
#include "V8DataModel/PrismInstance.h"
#include "V8DataModel/PyramidInstance.h"
#include "V8DataModel/Handles.h"
#include "V8DataModel/GuiObject.h"
#include "Reflection/EnumConverter.h"

namespace RBX {
namespace Reflection {
template<>
EnumDesc<RBX::BasicPartInstance::LegacyPartType>::EnumDesc()
:EnumDescriptor("PartType")
{
	addPair(RBX::BasicPartInstance::BALL_LEGACY_PART, "Ball");
	addPair(RBX::BasicPartInstance::BLOCK_LEGACY_PART, "Block");
	addPair(RBX::BasicPartInstance::CYLINDER_LEGACY_PART, "Cylinder");
}

template<>
EnumDesc<RBX::ExtrudedPartInstance::VisualTrussStyle>::EnumDesc()
:EnumDescriptor("Style")
{
	addPair(RBX::ExtrudedPartInstance::FULL_ALTERNATING_CROSS_BEAM, "AlternatingSupports");
	addPair(RBX::ExtrudedPartInstance::BRIDGE_STYLE_CROSS_BEAM, "BridgeStyleSupports");
	addPair(RBX::ExtrudedPartInstance::NO_CROSS_BEAM, "NoSupports");
	addLegacyName("Alternating Supports", RBX::ExtrudedPartInstance::FULL_ALTERNATING_CROSS_BEAM);
	addLegacyName("Bridge Style Supports", RBX::ExtrudedPartInstance::BRIDGE_STYLE_CROSS_BEAM);
	addLegacyName("No Supports", RBX::ExtrudedPartInstance::NO_CROSS_BEAM);
}

#ifdef _PRISM_PYRAMID_
template<>
EnumDesc<RBX::PrismInstance::NumSidesEnum>::EnumDesc()
:EnumDescriptor("PrismSides")
{
	addPair(RBX::PrismInstance::sides3, "3");
	// Don't allow a 4 sided prism - should use block.
	addPair(RBX::PrismInstance::sides5, "5");
	addPair(RBX::PrismInstance::sides6, "6");
	addPair(RBX::PrismInstance::sides8, "8");
	addPair(RBX::PrismInstance::sides10, "10");
	addPair(RBX::PrismInstance::sides20, "20");
}

template<>
EnumDesc<RBX::PyramidInstance::NumSidesEnum>::EnumDesc()
:EnumDescriptor("PyramidSides")
{
	addPair(RBX::PyramidInstance::sides3, "3");
	addPair(RBX::PyramidInstance::sides4, "4");
	addPair(RBX::PyramidInstance::sides5, "5");
	addPair(RBX::PyramidInstance::sides6, "6");
	addPair(RBX::PyramidInstance::sides8, "8");
	addPair(RBX::PyramidInstance::sides10, "10");
	addPair(RBX::PyramidInstance::sides20, "20");
}
#endif //_PRISM_PYRAMID_

template<>
EnumDesc<RBX::Handles::VisualStyle>::EnumDesc()
:EnumDescriptor("HandlesStyle")
{
	addPair(RBX::Handles::RESIZE_HANDLES, "Resize");
	addPair(RBX::Handles::MOVEMENT_HANDLES, "Movement");
}

template<>
EnumDesc<RBX::GuiObject::SizeConstraint>::EnumDesc()
:EnumDescriptor("SizeConstraint")
{
	addPair(RBX::GuiObject::RELATIVE_XY, "RelativeXY");
	addPair(RBX::GuiObject::RELATIVE_XX, "RelativeXX");
	addPair(RBX::GuiObject::RELATIVE_YY, "RelativeYY");
}

template<>
EnumDesc<RBX::GuiObject::ImageScale>::EnumDesc()
:EnumDescriptor("ScaleType")
{
	addPair(RBX::GuiObject::SCALE_STRETCH, "Stretch");
	addPair(RBX::GuiObject::SCALE_SLICED, "Slice");
}
}//namespace Reflection
}//namespace RBX