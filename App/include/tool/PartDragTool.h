/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"

namespace RBX {

	class RunDragger;
	class MegaDragger;

	extern const char* const sPartDragTool;
	class PartDragTool : public Named<MouseCommand, sPartDragTool>
	{
	private:
		typedef Named<MouseCommand, sPartDragTool> Super;
	protected:
		std::auto_ptr<RunDragger> runDragger;		// does snapping
		std::auto_ptr<MegaDragger> megaDragger;		// does join / unJoin
		shared_ptr<Instance> selectIfNoDrag;
		Vector2 downPoint;
		bool dragging;
		Vector3 hitWorld;

		/*override*/ bool drawConnectors() const {return true;}			// default mouse command no draw connectors

	public:
		/////////////////////////////////////////////////////////
		// MouseCommand
		//
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseDelta(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void render3dAdorn(Adorn* adorn);
		/*override*/ const std::string getCursorName() const {
			return dragging ? "GrabRotateCursor" : "DragCursor";
		}

		PartDragTool(	PartInstance* mousePart,
						const Vector3& hitPointWorld,
						Workspace* workspace,
						shared_ptr<Instance> selectIfNoDrag);

		~PartDragTool();
	};

} // namespace RBX