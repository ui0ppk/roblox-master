/* Copyright 2003-2006 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"
#include "rbx/Declarations.h"

namespace RBX {

// 
// http://www.parashift.com/c++-faq-lite/multiple-inheritance.html#faq-25.10

	// This is a virtual base class - see note above.  Any object that descends from it
	// should use the "virtual" keyword, so only one is included.
	//
	class RBXInterface IHasLocation
	{
	public:
		virtual const CoordinateFrame getLocation() = 0;
        
		virtual ~IHasLocation() {}
	};

} // namespace
