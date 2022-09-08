/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/Adornment.h"
#include "GfxBase/IAdornable.h"

namespace RBX
{
	extern const char* const sSelectionBox;

	class SelectionBox
		: public DescribedCreatable<SelectionBox, PVAdornment, sSelectionBox>
	{
	public:
		SelectionBox();

		void setSurfaceColor(Color3 value);
		Color3 getSurfaceColor() const { return surfaceColor; }

		void setSurfaceBrickColor(BrickColor value);
		BrickColor getSurfaceBrickColor() const { return BrickColor::closest(surfaceColor); }

		void setSurfaceTransparency(float value);
		float getSurfaceTransparency() const { return surfaceTransparency; }

		void setLineThickness(float value);
		float getLineThickness() const { return lineThickness; }

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ void render3dAdorn(Adorn* adorn);

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// Instance
		/*override*/ bool askSetParent(const Instance* instance) const { return true; }

	private:
		Color3 surfaceColor;
		float surfaceTransparency;
		float lineThickness;
	};

}


