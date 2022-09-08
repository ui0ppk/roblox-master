/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Tool/ToolsArrow.h"
#include "AppDraw/HandleType.h"
#include "Util/NormalId.h"
#include "Tool/DragTypes.h"


namespace RBX {

	extern const char* const sMoveResizeJoinTool;
	class MoveResizeJoinTool : public Named<AdvArrowTool, sMoveResizeJoinTool>
	{
	private:
		typedef Named<AdvArrowTool, sMoveResizeJoinTool> Super;
		void findTargetPV(const shared_ptr<InputObject>& inputObject);
		void capturedDrag(float axisDelta);
		float moveIncrement(void);
		int smallGridBoxesPerStud(void);

		/*override*/ bool drawConnectors() const {return true;}			// default mouse command no draw connectors

		static weak_ptr<PVInstance> scalingPart;

	protected:

        bool resizeFloat(shared_ptr<PartInstance> part, NormalId localNormalId, float amount, bool checkIntersection);
        bool advResizeImpl(shared_ptr<PartInstance> part, NormalId localNormalId, float amount, bool checkIntersection);

		// IAdornable
		/*override*/ void render3dAdorn(Adorn* adorn);
		/*override*/ void render2d(Adorn* adorn);

		// Tool
		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const {return cursor;}
		/*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void setCursor(std::string newCursor) {cursor = newCursor;}

		weak_ptr<PVInstance> targetPV;

		bool				overHandle;
		NormalId			localNormalId;
		Vector3				hitPointGrid;
		Vector2int16		down;
		int					movePerp;
		bool				dragging;
        CoordinateFrame		origPartPosition;
		Vector3				origPartSize;
		std::string		    cursor;
		float               origPartTransparency;

	public:
		MoveResizeJoinTool(Workspace* workspace) :
		  Named<AdvArrowTool, sMoveResizeJoinTool>(workspace),
		  overHandle(false),
		  movePerp(0),
		  dragging(false),
		  origPartSize(0.0f, 0.0f, 0.0f),
		  cursor("advCursor-default"),
		  origPartTransparency(0.0f)
		  {}
		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<MoveResizeJoinTool>(workspace);}

		static void setSelection(shared_ptr<RBX::Instance> oldSelection, shared_ptr<RBX::Instance> newSelection);
	};

} // namespace RBX