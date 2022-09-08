/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"

/*
	// End user tools

	GameTool  (select top parts for dragging, close to character)
		-> PartDragTool (called for dragging)

	GrabTool (drag parts and models in game)
		-> DragTool (called for dragging)

	ArrowTool (i.e. - powerpoint - select, box select, shift select, drag)
		-> DragTool	(called for dragging)

	// Auxillary (private)
	PartDragTool	(drag a single part)

	GroupDragTool	(drag a group of parts, or a model, or a selection of parts/models)

	DragTool
		-> PartDragTool (part dragging)
		-> GroupDragTool (>1 part, or model dragging)

	
*/

namespace RBX {

// TEMP COMMENT: verify merging...
	class Workspace;
	class PartInstance;
	class PVInstance;

	class AdvDragTool
	{
	public:
		static shared_ptr<MouseCommand> onMouseDown(PartInstance* hitPart,
											const Vector3& hitWorld,
											const std::vector<Instance*>& dragInstances,
											const shared_ptr<InputObject>& inputObject, 
											Workspace* workspace,
											shared_ptr<Instance> selectIfNoDrag);
// TEMP COMMENT: Verifying merge...
	};

} // namespace RBX