#pragma once

#include "V8Tree/Verb.h"

namespace RBX
{
	class StudioTool;
	class Workspace;

	class StudioToolVerb
		:public Verb
	{
	protected:
		Workspace* workspace;
		StudioTool* studioTool;
		bool toggle;

	public:
		StudioToolVerb(const char* name, StudioTool* studioTool, Workspace* workspace, bool toggle = true);
		/*override*/ bool isEnabled() const;
		/*override*/ bool isChecked() const;

		/*override*/ void doIt(RBX::IDataState* dataState);
	};
};