/* 
  Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved 
*/

#pragma once

namespace RBX 
{
	// Base class to control all selection functionality
	class RBXBaseClass Selectable
	{
	public:
		virtual bool isSelectable3d() { return true; }
	};
} // namespace
