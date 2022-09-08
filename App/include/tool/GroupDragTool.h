/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"
#include "Tool/DragUtilities.h"

namespace RBX {

	class MegaDragger;

	extern const char* const sGroupDragTool;
	class GroupDragTool : public Named<MouseCommand, sGroupDragTool>
	{
	protected:
		std::auto_ptr<MegaDragger> megaDragger;
		Vector2 downPoint;
		bool dragging;
		Vector3 lastHit;

		/*override*/ bool drawConnectors() const {return true;}			// default mouse command no draw connectors

	public:
		/////////////////////////////////////////////////////////
		// MouseCommand
		//
		/*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const {
			return dragging ? "GrabRotateCursor" : "DragCursor";
		}

		GroupDragTool(	PartInstance* mousePart,
						const Vector3& hitPointWorld,
						const PartArray& partArray,
						Workspace* workspace);

		~GroupDragTool();
	};

} // namespace RBX