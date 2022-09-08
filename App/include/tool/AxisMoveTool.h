/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Tool/ToolsArrow.h"
#include "Tool/MegaDragger.h"
#include "AppDraw/HandleType.h"
#include "Util/NormalId.h"

namespace RBX {

	class MegaDragger;
	class Extents;

	class AxisToolBase	: public ArrowToolBase
	{
	private:
		typedef ArrowToolBase Super;
		std::auto_ptr<MegaDragger> megaDragger;
		std::string		cursor;
		bool			dragging;
		Vector2int16	downPoint2d;
		RbxRay			dragRay;
		int				dragAxis;

		// dynamic - last point on the Ray we dragged to
		Vector3			lastPoint3d;

		bool getExtents(Extents& extents) const;
		bool getOverHandle(const shared_ptr<InputObject>& inputObject) const;
		bool getOverHandle(const shared_ptr<InputObject>& inputObject, Vector3& hitPointWorld, NormalId& normalId) const;

		/*override*/ bool drawConnectors() const {return true;}			// default mouse command no draw connectors

	protected:
		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);

		/*override*/ void render2d(Adorn* adorn);
		/*override*/ void render3dAdorn(Adorn* adorn);
		/*override*/ const std::string getCursorName() const {return cursor;}

		/*implement*/ virtual Color3 getHandleColor() const = 0;
		/*implement*/ virtual HandleType getDragType() const = 0;

	public:
		AxisToolBase(Workspace* workspace);
	};



} // namespace RBX