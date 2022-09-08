#include "stdafx.h"

#include "V8DataModel/CharacterMesh.h"
#include "Humanoid/Humanoid.h"

namespace RBX
{

const char* const sCharacterMesh = "CharacterMesh";

REFLECTION_BEGIN();
static const Reflection::EnumPropDescriptor<CharacterMesh, CharacterMesh::BodyPart> prop_bodyPart("BodyPart", category_Data, &CharacterMesh::getBodyPart, &CharacterMesh::setBodyPart);
Reflection::BoundProp<int> CharacterMesh::prop_baseTextureAssetId("BaseTextureId", "Data", &CharacterMesh::baseTextureAssetId);
Reflection::BoundProp<int> CharacterMesh::prop_overlayTextureAssetId("OverlayTextureId", "Data", &CharacterMesh::overlayTextureAssetId);
Reflection::BoundProp<int> CharacterMesh::prop_meshAssetId("MeshId", "Data", &CharacterMesh::meshAssetId);
REFLECTION_END();

namespace Reflection {
template<>
EnumDesc<CharacterMesh::BodyPart>::EnumDesc()
	:EnumDescriptor("BodyPart")

{
	addPair(CharacterMesh::HEAD, "Head");
	addPair(CharacterMesh::TORSO, "Torso");
	addPair(CharacterMesh::LEFTARM, "LeftArm");
	addPair(CharacterMesh::RIGHTARM, "RightArm");
	addPair(CharacterMesh::LEFTLEG, "LeftLeg");
	addPair(CharacterMesh::RIGHTLEG, "RightLeg");
}
template<>
CharacterMesh::BodyPart& Variant::convert<CharacterMesh::BodyPart>(void)
{
	return genericConvert<CharacterMesh::BodyPart>();
}
}//namespace Reflection

template<>
bool StringConverter<CharacterMesh::BodyPart>::convertToValue(const std::string& text, CharacterMesh::BodyPart& value)
{
	return Reflection::EnumDesc<CharacterMesh::BodyPart>::singleton().convertToValue(text.c_str(),value);
}

CharacterMesh::CharacterMesh()
	:DescribedCreatable<CharacterMesh,CharacterAppearance,sCharacterMesh>()
	, bodyPart(HEAD)
	, baseTextureAssetId(0)
	, overlayTextureAssetId(0)
	, meshAssetId(0)
{
	setName(sCharacterMesh);
}

void CharacterMesh::onPropertyChanged(const Reflection::PropertyDescriptor& descriptor)
{
	CharacterAppearance::apply();
}

void CharacterMesh::setBodyPart(BodyPart value)
{
	if(bodyPart != value){
		bodyPart = value;

		raisePropertyChanged(prop_bodyPart);
	}
}

void CharacterMesh::applyByMyself(Humanoid* humanoid)
{
	// re-using fireOutfitChanged. consider: own event.
	// right now, both end up doing the exact same thing, so reuse for now.
	if (PartInstance* leftLeg = humanoid->getLeftLegSlow()) {
		leftLeg->fireOutfitChanged();
	}
	if (PartInstance* rightLeg = humanoid->getRightLegSlow()) {
		rightLeg->fireOutfitChanged();
	}
	if (PartInstance* torso = humanoid->getTorsoSlow()) {
		torso->fireOutfitChanged();
	}
	if (PartInstance* leftSleeve = humanoid->getLeftArmSlow()) {
		leftSleeve->fireOutfitChanged();
	}
	if (PartInstance* rightSleeve = humanoid->getRightArmSlow()) {
		rightSleeve->fireOutfitChanged();
	}
}

TextureId CharacterMesh::getBaseTextureId() const
{
	if(baseTextureAssetId != 0)
	{
		return TextureId(RBX::format("rbxassetid://%d", baseTextureAssetId));
	}
	else
	{
		return TextureId::nullTexture();
	}
}
TextureId CharacterMesh::getOverlayTextureId() const
{
	if(overlayTextureAssetId != 0)
	{
		return TextureId(RBX::format("rbxassetid://%d", overlayTextureAssetId));
	}
	else
	{
		return TextureId::nullTexture();
	}
}
MeshId CharacterMesh::getMeshId() const
{
	if(meshAssetId != 0)
	{
		return MeshId(RBX::format("rbxassetid://%d", meshAssetId));
	}
	else
	{
		return MeshId();
	}
}

}
