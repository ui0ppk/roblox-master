/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "Util/CellID.h"

namespace RBX 
{
	namespace Reflection
	{
		template<>
		const Type& Type::getSingleton<CellID>()
		{
		static TType<CellID> type("CellId");
		return type;
		}
	}

	CellID::CellID( bool newIsNil, float newLocation[3], shared_ptr<Instance> newTerrainPart )
	{
		isNil = newIsNil; 
		location = G3D::Vector3( newLocation[0], newLocation[1], newLocation[2] );
		terrainPart = newTerrainPart;
	}

	CellID::CellID()
	{
		isNil = true;
		location = G3D::Vector3( 0.0, 0.0, 0.0 );
	}

	CellID::~CellID()
	{

	}

} // namespace