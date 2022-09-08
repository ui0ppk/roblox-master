/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Tool/GroupDragTool.h"
#include "Tool/ICancelableTool.h"
#include "Tool/DragUtilities.h"

namespace RBX {

	class MegaDragger;

	extern const char* const sGroupDropTool;
	class GroupDropTool 
		: public Named<GroupDragTool, sGroupDropTool>
		, public ICancelableTool
	{
	private:
		typedef Named<GroupDragTool, sGroupDropTool> Super;
	public:
		/////////////////////////////////////////////////////////
		// MouseCommand
		//
		///*override*/ void onMouseDelta(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/const std::string getCursorName() const	{return MouseCommand::isAdvArrowToolEnabled() ? "advClosed-hand" : "DropCursor";}
		
		/////////////////////////////////////////////////////////
		// ICancelableTool
		//
		/*override*/ shared_ptr<MouseCommand> onCancelOperation();

		GroupDropTool(	PartInstance* mousePart,
						const PartArray& partArray,
						Workspace* workspace,
						bool suppressPartsAlign = false);

		~GroupDropTool();
	};

} // namespace RBX