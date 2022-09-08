/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"

namespace RBX {

	extern const char* const sGameTool;
	class GameTool : public Named<MouseCommand, sGameTool>
	{
	private:
		std::string cursor;

		bool draggablePart(const PartInstance* part, const Vector3& hitPoint) const;

		/////////////////////////////////////////////////////////
		// MouseCommand
		//
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const {return cursor;}

		/*override*/ bool drawConnectors() const {return true;}			// default mouse command no draw connectors

	public:
		GameTool(Workspace* workspace);
		~GameTool();

		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<GameTool>(workspace);}
	};

} // namespace RBX