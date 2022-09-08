/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Service.h"
#include "Util/RunStateOwner.h"
#include "Reflection/Event.h"


namespace RBX {

	class PartInstance;

	class MeshId : public ContentId
	{
	public:
		MeshId(const ContentId& id):ContentId(id) {}
		MeshId(const char* id):ContentId(id) {}
		MeshId(const std::string& id):ContentId(id) {}
		MeshId() {}
	};
} // namespace RBX
