#pragma once

namespace RBX { namespace Voxel2 {

    class Region;
    
    class GridListener
	{
	public:
		virtual ~GridListener() {}

        virtual void onTerrainRegionChanged(const Region& region) = 0;
	};
    
} }
