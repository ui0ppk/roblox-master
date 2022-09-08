/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8DataModel/MouseCommand.h"

namespace RBX {

	class ModelTool : public MouseCommand
	{
	private:	
		typedef MouseCommand Super;
	protected:
		shared_ptr<Instance> topInstance;

		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
		/*override*/ void render3dAdorn(Adorn* adorn);
	public:
		ModelTool(Workspace* workspace);
		~ModelTool();
	};

	extern const char* const sAnchorTool;
	class AnchorTool : public Named<ModelTool, sAnchorTool>
	{
	private:
		bool allAnchored;
	protected:
		/*override*/ void onMouseHover(const shared_ptr<InputObject>& inputObject);
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const;
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject) {return shared_from(this);} // sticky
	public:
		AnchorTool(Workspace* workspace) 
			: Named<ModelTool, sAnchorTool>(workspace)
			, allAnchored(false)
		{}
		static bool allChildrenAnchored(Instance* test);
	};

	extern const char* const sLockTool;
	class LockTool : public Named<ModelTool, sLockTool>
	{
	protected:
		/*override*/ shared_ptr<MouseCommand> onMouseDown(const shared_ptr<InputObject>& inputObject);
		/*override*/ const std::string getCursorName() const;
		/*override*/ shared_ptr<MouseCommand> onMouseUp(const shared_ptr<InputObject>& inputObject) {return shared_from(this);} // sticky
	public:
		LockTool(Workspace* workspace) : Named<ModelTool, sLockTool>(workspace) {}
	};

} // namespace RBX