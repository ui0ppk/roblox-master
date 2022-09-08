/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/CharacterAppearance.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/ModelInstance.h"
#include "Humanoid/Humanoid.h"
#include "Network/Players.h"

DYNAMIC_FASTFLAG(UseR15Character)

namespace RBX {

const char* const sCharacterAppearance	= "CharacterAppearance";

const char *const sShirt = "Shirt";
const char *const sPants = "Pants";
const char *const sShirtGraphic = "ShirtGraphic";
const char *const sClothing = "Clothing";


Reflection::BoundProp<TextureId> ShirtGraphic::prop_Graphic("Graphic", category_Appearance, &ShirtGraphic::graphic, &ShirtGraphic::dataChanged);

Reflection::BoundProp<TextureId> Clothing::prop_outfit1("Outfit1", category_Appearance, &Clothing::outfit1, &Clothing::dataChanged, Reflection::PropertyDescriptor::LEGACY);
Reflection::BoundProp<TextureId> Clothing::prop_outfit2("Outfit2", category_Appearance, &Clothing::outfit2, &Clothing::dataChanged, Reflection::PropertyDescriptor::LEGACY);

Reflection::PropDescriptor<Shirt, TextureId> Shirt::prop_ShirtTemplate("ShirtTemplate", category_Appearance, &Shirt::getTemplate, &Shirt::setTemplate);
Reflection::PropDescriptor<Pants, TextureId> Pants::prop_PantsTemplate("PantsTemplate", category_Appearance, &Pants::getTemplate, &Pants::setTemplate);

ShirtGraphic::ShirtGraphic()
{
	setName("Shirt Graphic");
}

Clothing::Clothing()
{
	setName("Clothing");
}

Shirt::Shirt()
{
	// Don't remove this! Gcc needs it so that Creatable works
}
Pants::Pants()
{
	// Don't remove this! Gcc needs it so that Creatable works
}

void ShirtGraphic::applyByMyself(Humanoid* humanoid)
{
	if (PartInstance* torso = humanoid->getVisibleTorsoSlow())
		if (Decal* decal = torso->findFirstChildOfType<Decal>())
			decal->setTexture(graphic);
}

void Clothing::applyByMyself(Humanoid* humanoid)
{
	if (PartInstance* leftLeg = humanoid->getLeftLegSlow()) {
		leftLeg->fireOutfitChanged();
	}
	if (PartInstance* rightLeg = humanoid->getRightLegSlow()) {
		rightLeg->fireOutfitChanged();
	}
	if (PartInstance* torso = humanoid->getVisibleTorsoSlow()) {
		torso->fireOutfitChanged();
	}
	if (PartInstance* leftSleeve = humanoid->getLeftArmSlow()) {
		leftSleeve->fireOutfitChanged();
	}
	if (PartInstance* rightSleeve = humanoid->getRightArmSlow()) {
		rightSleeve->fireOutfitChanged();
	}
}

void Shirt::setTemplate(TextureId value)
{
	if (value != Clothing::outfit2)
	{
		prop_outfit2.setValue(this, value);
		raisePropertyChanged(prop_ShirtTemplate);
	}
}

void Pants::setTemplate(TextureId value)
{
	if (value != Clothing::outfit1)
	{
		prop_outfit1.setValue(this, value);
		raisePropertyChanged(prop_PantsTemplate);
	}
}

const char *const sSkin = "Skin";

Reflection::BoundProp<BrickColor> Skin::prop_skinColor("SkinColor", category_Appearance, &Skin::skinColor, &Skin::dataChanged);

Skin::Skin()
:skinColor(BrickColor::brick_226)
{
	setName("Skin");
}

void Skin::applyByMyself(Humanoid* humanoid)
{
	if (PartInstance* head = humanoid->getHeadSlow())
		head->setColor(skinColor);
	if (PartInstance* leftLeg = humanoid->getLeftLegSlow())
		leftLeg->setColor(skinColor);
	if (PartInstance* rightLeg = humanoid->getRightLegSlow())
		rightLeg->setColor(skinColor);
	if (PartInstance* torso = humanoid->getVisibleTorsoSlow())
		torso->setColor(skinColor);
	if (PartInstance* leftSleeve = humanoid->getLeftArmSlow())
		leftSleeve->setColor(skinColor);
	if (PartInstance* rightSleeve = humanoid->getRightArmSlow())
		rightSleeve->setColor(skinColor);
}

const char *const sBodyColors = "BodyColors";

Reflection::BoundProp<BrickColor> BodyColors::prop_HeadColor("HeadColor", category_Appearance, &BodyColors::headColor, &BodyColors::dataChanged);
Reflection::BoundProp<BrickColor> BodyColors::prop_LeftArmColor("LeftArmColor", category_Appearance, &BodyColors::leftArmColor, &BodyColors::dataChanged);
Reflection::BoundProp<BrickColor> BodyColors::prop_RightArmColor("RightArmColor", category_Appearance, &BodyColors::rightArmColor, &BodyColors::dataChanged);
Reflection::BoundProp<BrickColor> BodyColors::prop_TorsoColor("TorsoColor", category_Appearance, &BodyColors::torsoColor, &BodyColors::dataChanged);
Reflection::BoundProp<BrickColor> BodyColors::prop_LeftLegColor("LeftLegColor", category_Appearance, &BodyColors::leftLegColor, &BodyColors::dataChanged);
Reflection::BoundProp<BrickColor> BodyColors::prop_RightLegColor("RightLegColor", category_Appearance, &BodyColors::rightLegColor, &BodyColors::dataChanged);

BodyColors::BodyColors()
:headColor(BrickColor::brick_226)
,leftArmColor(BrickColor::brick_226)
,rightArmColor(BrickColor::brick_226)
,torsoColor(BrickColor::brick_28)
,leftLegColor(BrickColor::brick_23)
,rightLegColor(BrickColor::brick_23)
{
	setName("Body Colors");
}

void BodyColors::applyByMyself(Humanoid* humanoid)
{
	bool hasSkin = ModelInstance::findFirstModifierOfType<Skin>(getParent())!=0;
	if (hasSkin)
		return;

    if (humanoid->getUseR15())
	{
		Instance *pParent = getParent();
		if (pParent)
		{
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LowerTorso")))
				answer->setColor(torsoColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("UpperTorso")))
				answer->setColor(torsoColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("Head")))
				answer->setColor(headColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightUpperArm")))
				answer->setColor(rightArmColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightLowerArm")))
				answer->setColor(rightArmColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightHand")))
				answer->setColor(rightArmColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftUpperArm")))
				answer->setColor(leftArmColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftLowerArm")))
				answer->setColor(leftArmColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftHand")))
				answer->setColor(leftArmColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightUpperLeg")))
				answer->setColor(rightLegColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightLowerLeg")))
				answer->setColor(rightLegColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("RightFoot")))
				answer->setColor(rightLegColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftUpperLeg")))
				answer->setColor(leftLegColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftLowerLeg")))
				answer->setColor(leftLegColor);
			if (PartInstance* answer = Instance::fastDynamicCast<PartInstance>(pParent->findFirstChildByName("LeftFoot")))
				answer->setColor(leftLegColor);

		}
	} else {
		if (PartInstance* head = humanoid->getHeadSlow())
			head->setColor(headColor);
		if (PartInstance* leftLeg = humanoid->getLeftLegSlow())
			leftLeg->setColor(leftLegColor);
		if (PartInstance* rightLeg = humanoid->getRightLegSlow())
			rightLeg->setColor(rightLegColor);
		if (PartInstance* torso = humanoid->getVisibleTorsoSlow())
			torso->setColor(torsoColor);
		if (PartInstance* leftSleeve = humanoid->getLeftArmSlow())
			leftSleeve->setColor(leftArmColor);
		if (PartInstance* rightSleeve = humanoid->getRightArmSlow())
			rightSleeve->setColor(rightArmColor);
	}
}

void LegacyCharacterAppearance::apply()
{
	if (Network::Players::backendProcessing(this, false))
		Super::apply();
}
void CharacterAppearance::apply()
{
	if (Humanoid* humanoid = Humanoid::modelIsCharacter(getParent()))
		applyByMyself(humanoid);
}

void CharacterAppearance::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);

	// Search for a Humanoid sibling:
	if (event.child==this) {

		if(event.newParent)
		{
			RBX::Humanoid* humanoid = Humanoid::modelIsCharacter(event.newParent);
			if(humanoid)
			{
				applyByMyself(humanoid);
			}
		}
		if(event.oldParent)
		{
			RBX::Humanoid* humanoid = Humanoid::modelIsCharacter(event.oldParent);
			if(humanoid)
			{
				applyByMyself(humanoid);
			}
		}
	}
}

bool CharacterAppearance::askSetParent(const Instance* instance) const
{
	return Instance::fastDynamicCast<ModelInstance>(instance)!=NULL;
}

}	// namespace
