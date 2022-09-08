#pragma once

#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"

namespace RBX
{
	extern const char* const sScriptService;
	class ScriptService
		: public DescribedNonCreatable<ScriptService, Instance, sScriptService>
		, public Service
	{
	};
}
