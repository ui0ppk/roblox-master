/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"

namespace RBX {

	class PartInstance;

	extern const char* const sCloneTool;
	class CloneTool : public Named<MouseCommand, sCloneTool>
	{
	private:
		shared_ptr<PartInstance> clonePart;

		/*override*/ void onMouseIdle(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const;
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);

	public:
		CloneTool(Workspace* workspace);
		~CloneTool();

		/*override*/ shared_ptr<MouseCommand> isSticky() const {return Creatable<MouseCommand>::create<CloneTool>(workspace);}
		/*override*/ bool drawConnectors() const {return true;}			// default mouse command no draw connectors
	};




} // namespace RBX