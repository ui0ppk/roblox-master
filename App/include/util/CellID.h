/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

#include "G3D/Vector3.h"
#include "V8Tree/Instance.h"

namespace RBX 
{
	class CellID
	{
	private:
		bool isNil;
		RBX::Vector3 location;
		shared_ptr<Instance> terrainPart;
	public:
		CellID();
		CellID( bool isNil, const RBX::Vector3& location, shared_ptr<Instance> terrainPart )
			:isNil(isNil)
			,location(location)
			,terrainPart(terrainPart)
		{
		}
		CellID( bool newIsNil, float newLocation[3], shared_ptr<Instance> newTerrainPart );
		~CellID();

		bool operator ==(const CellID& other) const
		{
			if (isNil != other.isNil)
				return false;
			if (location != other.location)
				return false;
			if (terrainPart != other.terrainPart)
				return false;
			return true;
		}

		bool getIsNil() const { return isNil; }
		void setIsNil( bool newIsNil ) { isNil = newIsNil; }
		
		G3D::Vector3 getLocation() const { return location; }
		void setLocation( RBX::Vector3 newLocation ) { location = newLocation; }

		shared_ptr<Instance> getTerrainPart() const { return terrainPart; }
		void setTerrainPart( shared_ptr<Instance> newTerrainPart ) { terrainPart = newTerrainPart; }

		static CellID fromParameters( bool newIsNil, float newLocation[3], shared_ptr<Instance> newTerrainPart ) { return CellID( newIsNil, newLocation, newTerrainPart ); }
	};
}
