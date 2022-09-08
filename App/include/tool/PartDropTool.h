/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Tool/PartDragTool.h"
#include "Tool/ICancelableTool.h"
#include <boost/shared_ptr.hpp>

namespace RBX {

	extern const char* const sPartDropTool;
	class PartDropTool 
		: public Named<PartDragTool, sPartDropTool>
		, public ICancelableTool
	{
	private:
		typedef Named<PartDragTool, sPartDropTool> Super;
	public:
		/////////////////////////////////////////////////////////
		// MouseCommand
		//
		/*override*/ void onMouseDelta(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		///*override*/ MouseCommand* onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);
		/*override*/const std::string getCursorName() const	{return MouseCommand::isAdvArrowToolEnabled() ? "advClosed-hand" : "DropCursor";}

		/////////////////////////////////////////////////////////
		// ICancelableTool
		//
		/*override*/ shared_ptr<MouseCommand>  onCancelOperation();
	
		PartDropTool(	PartInstance* mousePart,
						const Vector3& hitPointWorld,
						Workspace* workspace,
						shared_ptr<Instance> selectIfNoDrag);

		~PartDropTool();

	private:
		Vector3 hitLocal;
	};

} // namespace RBX