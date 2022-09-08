#include "stdafx.h"

#include "SuperCluster.h"
#include "SceneUpdater.h"
#include "VisualEngine.h"


FASTINTVARIABLE(SuperClusterFastClusterSize, 1000);


namespace RBX
{
namespace Graphics
{


SuperCluster::SuperCluster(VisualEngine* ve, SpatialGrid<SuperCluster>* grid, const SpatialGridIndex& idx, bool fw)
    : visualEngine(ve)
    , spatialGrid(grid)
    , spatialIndex(idx)
    , fw(fw)
    , lastCluster(0)
{
}

SuperCluster::~SuperCluster()
{
    clear();
}

void SuperCluster::clear()
{
    for(int j=0, e = clusters.size(); j<e; ++j)
    {
        delete clusters[j];
    }
    clusters.clear();
    lastCluster = 0;
}

void SuperCluster::destroyFastCluster(FastCluster* fc)
{
    clusters.erase(std::remove(clusters.begin(), clusters.end(), fc), clusters.end());

    if(lastCluster == fc)
        lastCluster = 0;

    delete fc;

    if(clusters.empty())
    {
        visualEngine->getSceneUpdater()->destroySuperCluster(this);
        return;
    }
}

FastCluster* SuperCluster::findBestCluster()
{
    if(lastCluster && lastCluster->getPartCount() < (unsigned)FInt::SuperClusterFastClusterSize)
        return lastCluster;

    for(int j=0, e=clusters.size(); j<e; ++j)
    {
        if(clusters[j]->getPartCount() < (unsigned)FInt::SuperClusterFastClusterSize)
        {
            lastCluster = clusters[j];
            return lastCluster;
        }
    }

    clusters.push_back(new FastCluster(visualEngine, NULL, this, fw));
    lastCluster = clusters.back();
    return lastCluster;
}

void SuperCluster::invalidateAllFastClusters()
{
    for(int j=0, e=clusters.size(); j<e; ++j)
    {
        clusters[j]->invalidateEntity();
    }
}

void SuperCluster::checkCluster(FastCluster* fc)
{
    static std::vector< shared_ptr<PartInstance> > tmpparts;
    unsigned curCount = fc->getPartCount();
    if(!curCount || curCount > (unsigned)FInt::SuperClusterFastClusterSize/2)
        return;

    SceneUpdater* scu = visualEngine->getSceneUpdater();

    // check if we can re-distribute the parts to other fastclusters within the same supercluster
    int partsToGo = curCount;
    for (int j=0, e=clusters.size(); partsToGo>0 && j<e; ++j)
    {
        if (clusters[j] != fc)
        {
            int freeSlots = FInt::SuperClusterFastClusterSize - (int)clusters[j]->getPartCount();
            partsToGo -= freeSlots;
        }
    }

    if (partsToGo <= 0)
    { // yes, they all fit
        fc->getAllParts(tmpparts);
        destroyFastCluster(fc);

        for (unsigned j = 0; j<curCount; ++j)
        {
            scu->queuePartToCreate(tmpparts[j]);
        }
        tmpparts.clear();
    }
}

FastCluster* SuperCluster::addPart(const boost::shared_ptr<PartInstance>& part)
{
    FastCluster* fc = findBestCluster();
    fc->addPart(part);
    return fc;
}

}}
