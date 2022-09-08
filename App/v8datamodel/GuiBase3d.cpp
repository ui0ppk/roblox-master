/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/GuiBase3d.h"

namespace RBX {

const char* const sGuiBase3d = "GuiBase3d";

const Reflection::PropDescriptor<GuiBase3d, Color3>				prop_color("Color3", category_Appearance, &GuiBase3d::getColor, &GuiBase3d::setColor, Reflection::PropertyDescriptor::STANDARD);
const Reflection::PropDescriptor<GuiBase3d, BrickColor>			prop_depBrickColor("Color", category_Appearance, &GuiBase3d::getBrickColor, &GuiBase3d::setBrickColor, 
																				   Reflection::PropertyDescriptor::Attributes::deprecated(prop_color, Reflection::PropertyDescriptor::LEGACY_SCRIPTING));

const Reflection::PropDescriptor<GuiBase3d, float>				prop_transparency("Transparency", category_Appearance, &GuiBase3d::getTransparency, &GuiBase3d::setTransparency);
const Reflection::PropDescriptor<GuiBase3d, bool>				prop_Visible("Visible", category_Data, &GuiBase3d::getVisible, &GuiBase3d::setVisible);

GuiBase3d::GuiBase3d(const char* name)
	:Super(name)
	,color(BrickColor::brickBlue().color3())
	,transparency(0.0)
	,visible(true)
{}

void GuiBase3d::setBrickColor(BrickColor value)
{
	setColor(value.color3());
}

void GuiBase3d::setColor(Color3 value)
{
	if (color != value)
	{
		color = value;
		raisePropertyChanged(prop_color);
	}
}

void GuiBase3d::setTransparency(float value)
{
	if (transparency != value)
	{
		transparency = value;
		raisePropertyChanged(prop_transparency);
	}
}

void GuiBase3d::setVisible(bool value)
{
	if (visible != value)
	{
		visible = value;
		raisePropertyChanged(prop_Visible);
		shouldRenderSetDirty();
	}
}

}

