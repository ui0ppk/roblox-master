/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"
#include "Util/BrickColor.h"
#include "GuiBase.h"

namespace RBX {
	extern const char* const sGuiBase3d;

	//A base class for "adornment" instances (3D objects that adorn Instances)
	class GuiBase3d 
		: public DescribedNonCreatable<GuiBase3d, GuiBase, sGuiBase3d>
	{
	public:
		GuiBase3d(const char* name);

		void		setBrickColor(BrickColor value);
		BrickColor	getBrickColor() const { return BrickColor::closest(color); }

		void		setColor(Color3 value);
		Color3		getColor() const { return color; }

		void		setTransparency(float value);
		float		getTransparency() const { return transparency; }

		void		setVisible(bool value);
		bool		getVisible() const { return visible;}

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// IAdornable
		/*override*/ bool shouldRender3dAdorn() const {return getVisible();}

		////////////////////////////////////////////////////////////////////////////////////
		// 
		// GuiBase
		/*implement*/ virtual bool canProcessMeAndDescendants() const { return false; };
		/*implement*/ virtual int getZIndex() const { return -1; }
		/*implement*/ virtual GuiQueue getGuiQueue() const { return GUIQUEUE_GENERAL; }

	protected:
		Color3					color;
		float					transparency;
		bool					visible;

	private:
		typedef DescribedNonCreatable<GuiBase3d, GuiBase, sGuiBase3d> Super;

	};
}
