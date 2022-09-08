/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PlayerMouse.h"
#include "V8DataModel/ScriptMouseCommand.h"

namespace RBX {
	
const char *const sPlayerMouse = "PlayerMouse";

// So far, only differences between Mouse and PlayerMouse lie in Mouse.Icon behavior
PlayerMouse::PlayerMouse() : DescribedNonCreatable<PlayerMouse, Mouse, sPlayerMouse>()
{
}

PlayerMouse::~PlayerMouse()
{
}

TextureId PlayerMouse::getIcon() const
{
	return Super::getIcon();
}

void PlayerMouse::setIcon(const TextureId& value)
{
	Super::setIcon(value);
}
} // namespace
