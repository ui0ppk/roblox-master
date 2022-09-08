/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/G3DCore.h"
#include "rbx/Declarations.h"
#include "rbx/boost.hpp"

// Common base class for Tool, Accoutrement
//
//	IEquipable
//		Tool
//		Accoutrement


namespace RBX {

	class Weld;
	class PartInstance;
	class Workspace;

	class RBXInterface IEquipable
	{
	protected:

		// "Backend" properties - only tracked on the server side where all connection/destroy occurs
		shared_ptr<Weld> weld;					// Weld (I create/destroy) - no weld == dropped, UNEQUIPPED

		void buildWeld(
				PartInstance* humanoidPart, 
				PartInstance* gadgetPart,
				const CoordinateFrame& humanoidCoord,
				const CoordinateFrame& gadgetCoord,
				const std::string& name);

		IEquipable();

		virtual ~IEquipable();
	};

} // namespace
