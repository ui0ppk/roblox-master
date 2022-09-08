/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#pragma once
#include "rbx/boost.hpp"
#include <boost/unordered_map.hpp>
#include <vector>
#include "util/Extents.h"
#include "v8tree/Instance.h"

namespace RBX
{
class DataModel;
class PartInstance;
class PartOperation;
class NegateOperation;
class CSGMesh;

typedef void (*MessageCallback)( const char* title, const char* msg );

class CSGOperations
{
public:
    CSGOperations( DataModel* pDatamodel, MessageCallback failedFun );
    bool doUnion(
        Instances::const_iterator start,
        Instances::const_iterator end,
        boost::shared_ptr<PartOperation>& partOperation );
    bool doNegate(
        Instances::const_iterator start,
        Instances::const_iterator end,
        Instances& toSelect );
    bool doSeparate(
        Instances::const_iterator start,
        Instances::const_iterator end,
        Instances& ungroupedItems );

private:
    DataModel* mDatamodel;
    MessageCallback operationFailed;

    bool setUnionMesh(boost::shared_ptr<PartOperation> finalPartOperation,
                      Instances::const_iterator start,
                      Instances::const_iterator end,
                      bool isChildRecalculation);
    bool setNegateMesh(shared_ptr<NegateOperation> negateOperation, shared_ptr<Instance> instance );
    bool recalculateMesh(shared_ptr<PartOperation> partOperation);
    bool negateSelection(Instances& toRemove,
                        Instances& toSelect,
                        Instances::const_iterator start, 
                        Instances::const_iterator end);
    bool separate(const shared_ptr<Instance>& wInstance,
                  Instances& separatedItems,
                  Instances& itemsToDelete);
    bool setOperationMesh(shared_ptr<PartOperation> partOperation, Instances instances);
    bool createPartMesh(PartInstance* part, CSGMesh* modelData);
    void resetInstanceCFrame(Instances::const_iterator start,
                             Instances::const_iterator end,
                             boost::unordered_map<boost::shared_ptr<Instance>, CoordinateFrame> originalCFrames);
    void warnIfChildInstances(shared_ptr<PartInstance> partInstance, const std::string& operationType);
	Instance* getCommonParentOrWorkspace(Instances::const_iterator start, Instances::const_iterator end);
    static const char* unionFailedMsg;
};
}
