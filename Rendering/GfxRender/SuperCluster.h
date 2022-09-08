#pragma once

#include "FastCluster.h"

namespace RBX
{
namespace Graphics
{

class FastCluster;
class VisualEngine;

// TODO: remove debug stuff!
class SuperCluster
{
public:
    SuperCluster(VisualEngine* ve, SpatialGrid<SuperCluster>* grid, const SpatialGridIndex& idx, bool fw);
    ~SuperCluster();

    SpatialGrid<SuperCluster>*  getSpatialGrid() const { return spatialGrid; }
    const SpatialGridIndex&     getSpatialIndex() const { return spatialIndex; }

    FastCluster* addPart(const boost::shared_ptr<PartInstance>& part);
    void checkCluster(FastCluster* fc);
    void destroyFastCluster(FastCluster* fc);
    void invalidateAllFastClusters();

private:
    FastCluster*                findBestCluster();
    void clear();

    VisualEngine*               visualEngine;
    SpatialGrid<SuperCluster>*  spatialGrid;
    SpatialGridIndex            spatialIndex;
    std::vector<FastCluster*>   clusters;
    FastCluster*                lastCluster;
    bool                        fw;
};

//*/


}}
