#pragma once

#include "Util/G3DCore.h"
#include "Util/ExtentsInt32.h"
#include "Util/Extents.h"
#include "rbx/Debug.h"

//#define _RBX_DEBUGGING_SPATIAL_HASH 

#ifdef _RBX_DEBUGGING_SPATIAL_HASH
	#define RBXASSERT_SPATIAL_HASH(expr) RBXASSERT(expr)
	const bool assertingSpatialHash = true;
#else
	#define RBXASSERT_SPATIAL_HASH(expr) ((void)0)
	const bool assertingSpatialHash = false;
#endif

namespace RBX {

	/** use the SpatialHash with classes that contain these members:
	 * basic Primitive must implement:
	 */ 
	class BasicSpatialHashPrimitive
	{
	private:
		ExtentsInt32			oldSpatialExtents;
		int						spatialNodeLevel;

#ifdef _RBX_DEBUGGING_SPATIAL_HASH
		void*					spatialNodes;		
		int                     spatialNodeCount;
#endif

	public:
		BasicSpatialHashPrimitive()
			: spatialNodeLevel(-1)
#ifdef _RBX_DEBUGGING_SPATIAL_HASH
			, spatialNodes(0)
			, spatialNodeCount(0)
#endif
		{};

		~BasicSpatialHashPrimitive()
		{
			RBXASSERT(spatialNodeLevel == -1);	//
			RBXASSERT_SPATIAL_HASH(spatialNodes == NULL);
			RBXASSERT_SPATIAL_HASH(spatialNodeCount == 0);
			spatialNodeLevel = -2;
		}

		bool IsInSpatialHash() { return spatialNodeLevel > -1;}

		// The remaining functions are used by the SpatialHash<> implementation
        int getSpatialNodeLevel() const {
			RBXASSERT(spatialNodeLevel >= -1);
			return spatialNodeLevel;
		}
		void setSpatialNodeLevel(int value)						{spatialNodeLevel = value;}

		const ExtentsInt32&	getOldSpatialExtents() const			{return oldSpatialExtents;}
		void setOldSpatialExtents(const ExtentsInt32& value)		{oldSpatialExtents = value;}	

		const Vector3int32& getOldSpatialMin()	{return oldSpatialExtents.low;}
		const Vector3int32& getOldSpatialMax()	{return oldSpatialExtents.high;}

	};

}	// namespace
