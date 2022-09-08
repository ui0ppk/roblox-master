/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"
#include "Tool/LuaDragger.h"
#include <boost/shared_ptr.hpp>

namespace RBX {

	class PartInstance;
	
	extern const char* const sLuaDragTool;
	class LuaDragTool : public Named<MouseCommand, sLuaDragTool>
	{
	private:
		boost::shared_ptr<LuaDragger> luaDragger;
		boost::weak_ptr<Instance> selectIfNoDrag;

		/////////////////////////////////////////////////////////
		// MouseCommand
		//
		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseMove(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const;
		/*override*/ shared_ptr<MouseCommand> onKeyDown(const shared_ptr<InputObject>& inputObject);

	public:
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);

		LuaDragTool(	PartInstance* mousePart,
						const Vector3& hitPointWorld,
						const std::vector<weak_ptr<PartInstance> >& partArray,
						Workspace* workspace,
						shared_ptr<Instance> selectIfNoDrag);

		~LuaDragTool();
	};

} // namespace RBX