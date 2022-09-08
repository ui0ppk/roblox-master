/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Tool/ToolsArrow.h"
#include "V8DataModel/MouseCommand.h"
#include "Tool/AdvLuaDragger.h"
#include "Util/Math.h"
#include <boost/shared_ptr.hpp>

namespace RBX {

	class PartInstance;
	
	extern const char* const sAdvLuaDragTool;

	class AdvLuaDragTool : public Named<AdvArrowToolBase, sAdvLuaDragTool>
	{
	private:
		typedef Named<AdvArrowToolBase, sAdvLuaDragTool> Super;
		boost::shared_ptr<AdvLuaDragger> advLuaDragger;
		boost::weak_ptr<Instance> selectIfNoDrag;
		std::string cursor;
        bool dragging;
        G3D::Vector2 downPoint2d;

        bool canDrag(const shared_ptr<InputObject>& inputObject) const;

		/////////////////////////////////////////////////////////
		// MouseCommand
		//
		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const { return cursor; }
		/*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void setCursor(std::string newCursor) { cursor = newCursor; }

	public:
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);

		AdvLuaDragTool(	PartInstance* mousePart,
						const Vector3& hitPointWorld,
						const std::vector<weak_ptr<PartInstance> >& partArray,
						Workspace* workspace,
						shared_ptr<Instance> selectIfNoDrag);

		~AdvLuaDragTool();
	};

} // namespace RBX