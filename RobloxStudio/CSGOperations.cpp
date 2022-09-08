/* Copyright 2014 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "CSGOperations.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "v8datamodel/PartOperation.h"
#include "V8DataModel/CSGDictionaryService.h"
#include "V8Xml/SerializerBinary.h"
#include "../Rendering/GfxRender/GeometryGenerator.h"
#include "V8Datamodel/SpecialMesh.h"
#include "v8DataModel/Workspace.h"
#include "v8DataModel/DataModel.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"
#include "V8DataModel/ChangeHistory.h"
#include "../CSG/CSGKernel.h"

DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)

FASTFLAGVARIABLE(CSGMeshColorToolsEnabled, false)
FASTFLAGVARIABLE(StudioCSGChildDataNoSizeLimit, false)
FASTFLAGVARIABLE(CSGNewTriangulateFallBack, true)
FASTFLAGVARIABLE(CSGDelayParentingOperationToEnd, false)

RBX::CSGOperations::CSGOperations( DataModel* pDatamodel, MessageCallback failedFun )
  : mDatamodel(pDatamodel)
  , operationFailed(failedFun)
{
}

bool RBX::CSGOperations::doUnion(
    std::vector<boost::shared_ptr<RBX::Instance> >::const_iterator start,
    std::vector<boost::shared_ptr<RBX::Instance> >::const_iterator end,
    boost::shared_ptr<PartOperation>& partOperation )
{
    bool allNeg = true;
    for (Instances::const_iterator iter = start; iter != end; ++iter)
    {
        if (!RBX::Instance::fastSharedDynamicCast<NegateOperation>(*iter))
        {
            allNeg = false;
            break;
        }
    }
    //shared_ptr<RBX::PartOperation> partOperation;
    try
    {
        if (allNeg)
            partOperation = Creatable<Instance>::create<NegateOperation>();
        else
            partOperation = Creatable<Instance>::create<UnionOperation>();
		if (!FFlag::CSGDelayParentingOperationToEnd)
			partOperation->setParent(mDatamodel->getWorkspace());
    }
    catch (std::exception &)
    {
        return false;
    }

	Instance* newOperationParent = NULL;
	if (FFlag::CSGDelayParentingOperationToEnd)
	{
		newOperationParent = getCommonParentOrWorkspace(start, end);
	}

    //Convert to mesh
    if (!setUnionMesh(partOperation, start, end, false))
    {
        partOperation->setParent(NULL);
        partOperation = boost::shared_ptr<RBX::PartOperation>();
        return false;
    }

    CSGDictionaryService* dictionaryService = ServiceProvider::create< CSGDictionaryService >(mDatamodel);
    NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::create<NonReplicatedCSGDictionaryService>(mDatamodel);

	if (FFlag::CSGDelayParentingOperationToEnd)
	{
		Instances selectionCopy(start, end);
		for (auto iter : selectionCopy)
		{
			iter->setParent(partOperation.get());

			if (shared_ptr<PartOperation> partOp = RBX::Instance::fastSharedDynamicCast<PartOperation>(iter))
			{
				dictionaryService->retrieveData(*partOp);
				nrDictionaryService->retrieveData(*partOp);

				partOp->setMeshData(RBX::BinaryString(""));
				partOp->setPhysicsData(RBX::BinaryString(""));
			}
		}
	}
	else
	{
		for (RBX::Instances::const_iterator iter = start; iter != end; ++iter)
		{
			(*iter)->setParent(partOperation.get());

			if (shared_ptr<PartOperation> partOp = RBX::Instance::fastSharedDynamicCast<PartOperation>(*iter))
			{
				dictionaryService->retrieveData(*partOp);
				nrDictionaryService->retrieveData(*partOp);

				partOp->setMeshData(RBX::BinaryString(""));
				partOp->setPhysicsData(RBX::BinaryString(""));
			}
		}
	}

    //Serialize children
    std::stringstream ss;
    SerializerBinary::serialize(ss, partOperation.get());

    BinaryString childDataBinaryStr(ss.str());

    if (!FFlag::StudioCSGChildDataNoSizeLimit && childDataBinaryStr.value().size() > PartOperation::getMaximumMeshStreamSize())
    {
        char buf[128];
        sprintf( buf, "This union is too complex.  There is a %d triangle limit for unions.  Please undo the last change.", (int)RBX::PartOperation::getMaximumTriangleCount());
        partOperation->setParent(NULL);
        partOperation = boost::shared_ptr<RBX::PartOperation>();
        operationFailed("Union Rejected", buf);
        return true;
    }

    partOperation->setChildData(childDataBinaryStr);

	if (!FFlag::CSGDelayParentingOperationToEnd)
	{
		dictionaryService->storeData(*partOperation);
		nrDictionaryService->storeData(*partOperation);
	}

    dictionaryService->insertMesh(*partOperation);

    //Remove Children
    while(partOperation->numChildren())
        partOperation->getChild(0)->setParent(NULL);

	if (FFlag::CSGDelayParentingOperationToEnd)
	{
		RBXASSERT(partOperation->getParent() == NULL);
		RBXASSERT(newOperationParent != NULL);
		partOperation->setParent(newOperationParent);
	}

    return true;
}

bool RBX::CSGOperations::doNegate(
    std::vector<boost::shared_ptr<RBX::Instance> >::const_iterator start,
    std::vector<boost::shared_ptr<RBX::Instance> >::const_iterator end,
    std::vector<shared_ptr<RBX::Instance> >& toSelect )
{
    std::vector<shared_ptr<RBX::Instance> > toRemove;

    bool success = negateSelection(toRemove, toSelect, start, end);

    if (!success)
    {
        for (std::vector<shared_ptr<RBX::Instance> >::iterator iter = toSelect.begin(); iter != toSelect.end(); ++iter)
            (*iter)->setParent(NULL);
        return true;
    }

    for (std::vector<shared_ptr<RBX::Instance> >::iterator iter = toRemove.begin(); iter != toRemove.end(); ++iter)
        (*iter)->setParent(NULL);

    return true;
}

bool RBX::CSGOperations::doSeparate(
    std::vector<boost::shared_ptr<RBX::Instance> >::const_iterator start,
    std::vector<boost::shared_ptr<RBX::Instance> >::const_iterator end,
    std::vector<shared_ptr<RBX::Instance> >& ungroupedItems )
{
    // First make a copy of the selection list
    Instances itemsToUngroup;
    for (std::vector<boost::shared_ptr<RBX::Instance> >::const_iterator iter = start; iter != end; ++iter)
    {
        if (shared_ptr<PartOperation> o = RBX::Instance::fastSharedDynamicCast<PartOperation>(*iter))
            itemsToUngroup.push_back(o);
    }

    // Now iterate over the objects to be ungrouped and ungroup them
    Instances itemsToDelete;
    bool didSomething = false;
    for ( Instances::const_iterator iter = itemsToUngroup.begin(); iter !=  itemsToUngroup.end(); iter++ )
        didSomething |= separate( (*iter), ungroupedItems, itemsToDelete );

    if (didSomething)
    {
        for (Instances::const_iterator iter = itemsToDelete.begin(); iter != itemsToDelete.end(); ++iter)
            (*iter)->setParent(NULL);
    }
    else
        debugAssertM(0, "Calling seperate command without checking is-enabled");
    return true;
}

bool RBX::CSGOperations::setOperationMesh(shared_ptr<PartOperation> partOperation, Instances instances)
{
    bool negativeChildren = true;

    for (Instances::const_iterator iter = instances.begin(); iter != instances.end(); ++iter)
    {
        if (!Instance::fastSharedDynamicCast<NegateOperation>(*iter))
        {
            negativeChildren = false;
            break;
        }
    }

    if (Instance::fastSharedDynamicCast<UnionOperation>(partOperation) || negativeChildren)
        return setUnionMesh(partOperation, instances.begin(), instances.end(), true);
    else if (shared_ptr<NegateOperation> negateOperation = Instance::fastSharedDynamicCast<NegateOperation>(partOperation))
        return setNegateMesh(negateOperation, *instances.begin());

    return false;
}

bool RBX::CSGOperations::recalculateMesh(shared_ptr<PartOperation> partOperation)
{
    std::stringstream ss(partOperation->peekChildData(mDatamodel).value());
    Instances instances;
    SerializerBinary::deserialize(ss, instances);

    for (std::vector<boost::shared_ptr<Instance> >::const_iterator iter = instances.begin(); iter != instances.end(); ++iter)
    {
        if (shared_ptr<PartOperation> childOperation = Instance::fastSharedDynamicCast<PartOperation>(*iter))
        {
            bool opSuccess = recalculateMesh(childOperation);

            if (!opSuccess)
                return false;
        }
    }

    Color3 partColor = partOperation->getColor3();
    CoordinateFrame cFrame = partOperation->getCoordinateFrame();
    Vector3 operationSize = partOperation->getPartSizeUi();
    PartMaterial partMaterial = partOperation->getRenderMaterial();
    float transparency = partOperation->getTransparencyXml();

    bool opSuccess = setOperationMesh(partOperation, instances);

    if (!opSuccess)
        return false;

    partOperation->setCoordinateFrame(cFrame);

    partOperation->setPartSizeXml(operationSize);
    partOperation->setColor3(partColor);
    partOperation->setRenderMaterial(partMaterial);
    partOperation->setTransparency(transparency);

    return true;
}

bool RBX::CSGOperations::createPartMesh(PartInstance* part, CSGMesh* modelData)
{    
    std::vector<Graphics::GeometryGenerator::Vertex> vertices;
    std::vector<unsigned short> indices;

    std::vector<CSGVertex> meshVertices;
    std::vector<unsigned int> meshIndices;

    {
        Graphics::GeometryGenerator geomGen;
        
        geomGen.addCSGPrimitive(part);

        vertices.resize(geomGen.getVertexCount());
        indices.resize(geomGen.getIndexCount());
        meshVertices.resize(geomGen.getVertexCount());
        meshIndices.resize(geomGen.getIndexCount());
    }

    if (vertices.size() == 0 || indices.size() < 3)
        return false;

    {
        Graphics::GeometryGenerator geomGen(&vertices[0], &indices[0]);
        geomGen.addCSGPrimitive(part);
    }
    
    for (size_t i = 0; i < vertices.size(); i++)
    {
        meshVertices[i].pos = vertices[i].pos;
        meshVertices[i].uv = vertices[i].uv;
        meshVertices[i].color = vertices[i].color;
        meshVertices[i].uvStuds = vertices[i].uvStuds;
        meshVertices[i].normal = vertices[i].normal;
        meshVertices[i].tangent = vertices[i].tangent;
    }

    for (size_t i = 0; i < indices.size(); i++)
    {
        meshIndices[i] = (unsigned int)indices[i];
    }

    modelData->set(meshVertices, meshIndices);
    modelData->weldMesh(true);
    modelData->buildBRep();

    return true;
}

void RBX::CSGOperations::resetInstanceCFrame(
    std::vector<boost::shared_ptr<Instance> >::const_iterator start,
    std::vector<boost::shared_ptr<Instance> >::const_iterator end,
    boost::unordered_map<boost::shared_ptr<Instance>, CoordinateFrame> originalCFrames)
{
    for (std::vector<boost::shared_ptr<Instance> >::const_iterator iter = start; iter != end; ++iter)
        if (shared_ptr<PartInstance> partInstance = Instance::fastSharedDynamicCast<PartInstance>(*iter))
            if (originalCFrames.count(*iter))
                partInstance->setCoordinateFrame(originalCFrames.at(*iter));
}

static void warnIfChildInstance(shared_ptr<RBX::Instance> instance,
                                shared_ptr<RBX::PartInstance> partInstance,
                                const std::string& operationType)
{
    if (instance->fastDynamicCast<RBX::DataModelMesh>())
        return;

    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Warning:  Child object \"%s\" of parent \"%s\" is not supported by %s.", instance->getFullName().c_str(), partInstance->getFullName().c_str(), operationType.c_str());
}

void RBX::CSGOperations::warnIfChildInstances(shared_ptr<PartInstance> partInstance, const std::string& operationType)
{
    partInstance->visitDescendants(boost::bind(&warnIfChildInstance, _1, partInstance, operationType));
}

const char* RBX::CSGOperations::unionFailedMsg = "This union is not currently solvable.  Try offseting the parts from each other or try unioning each part individually.";

bool RBX::CSGOperations::setUnionMesh(shared_ptr<PartOperation> finalPartOperation,
                                      std::vector<boost::shared_ptr<Instance> >::const_iterator start,
                                      std::vector<boost::shared_ptr<Instance> >::const_iterator end,
                                      bool isChildRecalculation)
{
    boost::scoped_ptr<CSGMesh> positiveMesh;
    boost::scoped_ptr<CSGMesh> negativeMesh;

    DataModel* dataModel = FFlag::CSGDelayParentingOperationToEnd ? mDatamodel : DataModel::get(finalPartOperation.get());

    CSGDictionaryService* dictionaryService = ServiceProvider::create< CSGDictionaryService >(dataModel);

    Instance* parentToSet = FFlag::CSGDelayParentingOperationToEnd ? NULL : (*start)->getParent();

    CoordinateFrame primaryDelta;
    bool primaryDeltaSet = false;

    boost::unordered_map<boost::shared_ptr<Instance>, CoordinateFrame> originalCFrames;

    for (std::vector<boost::shared_ptr<Instance> >::const_iterator iter = start; iter != end; ++iter)
    {
		if (!FFlag::CSGDelayParentingOperationToEnd)
		{
			if (parentToSet != (*iter)->getParent())
				parentToSet = NULL;
		}

        if (shared_ptr<PartInstance> partInstance = Instance::fastSharedDynamicCast<PartInstance>(*iter))
        {
            partInstance->destroyJoints();

            if (!isChildRecalculation)
            {
                if (!primaryDeltaSet)
                {
                    primaryDeltaSet = true;
                    primaryDelta = CoordinateFrame(partInstance->getCoordinateFrame().rotation.inverse(), -partInstance->getCoordinateFrame().translation);
                }
                originalCFrames.emplace(*iter, partInstance->getCoordinateFrame());
                partInstance->setCoordinateFrame(CoordinateFrame(primaryDelta.rotation) * (partInstance->getCoordinateFrame() + primaryDelta.translation));
            }
        }

        boost::scoped_ptr<CSGMesh> meshData;
        if (shared_ptr<PartOperation> partOperation = Instance::fastSharedDynamicCast<PartOperation>(*iter))
        {
            if (!partOperation->getMesh())
                continue;

            if (!partOperation->getMesh()->isValid())
            {
                partOperation->refreshMesh();
                if (!partOperation->getMesh()->isValid())
                {
                    if (!recalculateMesh(partOperation))
                    {
                        if (!isChildRecalculation)
                            resetInstanceCFrame(start, end, originalCFrames);
                        return false;
                    }
                }
            }

            meshData.reset(partOperation->getMesh()->clone());

              meshData->applyScale(partOperation->getSizeDifference());

            if (FFlag::CSGMeshColorToolsEnabled && partOperation->getUsePartColor())
                meshData->applyColor(Vector3(partOperation->getColor3().r, partOperation->getColor3().g, partOperation->getColor3().b));

            meshData->applyCoordinateFrame(partOperation->getCoordinateFrame());
        }

        if (shared_ptr<UnionOperation> unionOperation = Instance::fastSharedDynamicCast<UnionOperation>(*iter))
        {
            warnIfChildInstances(unionOperation, "union");

            if (!positiveMesh)
            {
                finalPartOperation->setColor(unionOperation->getColor());
                finalPartOperation->setRenderMaterial(unionOperation->getRenderMaterial());
                finalPartOperation->setReflectance(unionOperation->getReflectance());
                finalPartOperation->setTransparency(unionOperation->getTransparencyUi());
                finalPartOperation->setAnchored(unionOperation->getAnchored());
                finalPartOperation->setCanCollide(unionOperation->getCanCollide());
                finalPartOperation->setElasticity(unionOperation->getElasticity());
                finalPartOperation->setFriction(unionOperation->getFriction());
				if (DFFlag::MaterialPropertiesEnabled)
				{
					finalPartOperation->setPhysicalProperties(unionOperation->getPhysicalProperties());
				}
                positiveMesh.swap(meshData);
            }
            else
            {
                if (!positiveMesh->unionMesh(positiveMesh.get(), meshData.get()))
                {
                    if (!isChildRecalculation)
                        resetInstanceCFrame(start, end, originalCFrames);
                    finalPartOperation->setParent(NULL);
                    operationFailed("Union Unsolvable -1", unionFailedMsg);
                    return false;
                }
            }
        }
        else if (shared_ptr<NegateOperation> negateOperation = Instance::fastSharedDynamicCast<NegateOperation>(*iter))
        {
            warnIfChildInstances(negateOperation, "union");

            if (!negativeMesh)
            {
                negativeMesh.swap(meshData);
            }
            else
            {
                if (!negativeMesh->unionMesh(negativeMesh.get(), meshData.get()))
                {
                    if (!isChildRecalculation)
                        resetInstanceCFrame(start, end, originalCFrames);
                    finalPartOperation->setParent(NULL);
                    operationFailed("Union Unsolvable -2", unionFailedMsg);
                    return false;
                }
            }
        }
        else if (shared_ptr<PartInstance> partInstance = Instance::fastSharedDynamicCast<PartInstance>(*iter))
        {
            warnIfChildInstances(partInstance, "union");
            if (!positiveMesh)
            {
                finalPartOperation->setColor(partInstance->getColor());
                finalPartOperation->setRenderMaterial(partInstance->getRenderMaterial());
                finalPartOperation->setReflectance(partInstance->getReflectance());
                finalPartOperation->setTransparency(partInstance->getTransparencyUi());
                finalPartOperation->setAnchored(partInstance->getAnchored());
                finalPartOperation->setCanCollide(partInstance->getCanCollide());
                finalPartOperation->setElasticity(partInstance->getElasticity());
                finalPartOperation->setFriction(partInstance->getFriction());
				if (DFFlag::MaterialPropertiesEnabled)
				{
					finalPartOperation->setPhysicalProperties(partInstance->getPhysicalProperties());
				}
                
                positiveMesh.reset(CSGMeshFactory::singleton()->createMesh());

                bool partMeshCreated = createPartMesh(partInstance.get(), positiveMesh.get());

                if (partMeshCreated)
                {
                    positiveMesh->applyCoordinateFrame(partInstance->getCoordinateFrame());
                }
                else
                {
                    if (!isChildRecalculation)
                        resetInstanceCFrame(start, end, originalCFrames);
                    finalPartOperation->setParent(NULL);
                    operationFailed("Warning", "This part or mesh type is not supported.");
                    return false;
                }
            }
            else
            {
                boost::scoped_ptr<CSGMesh> model(CSGMeshFactory::singleton()->createMesh());
                bool partMeshCreated = createPartMesh(partInstance.get(), model.get());
                if (partMeshCreated)
                {
                    model->applyCoordinateFrame(partInstance->getCoordinateFrame());
                    if (!positiveMesh->unionMesh(positiveMesh.get(), model.get()))
                    {
                        if (!isChildRecalculation)
                            resetInstanceCFrame(start, end, originalCFrames);
                        finalPartOperation->setParent(NULL);
                        operationFailed("Union Unsolvable -3", unionFailedMsg);
                        return false;
                    }
                }
                else
                {
                    if (!isChildRecalculation)
                        resetInstanceCFrame(start, end, originalCFrames);
                    finalPartOperation->setParent(NULL);
                    operationFailed("Warning", "This part or mesh type is not supported.");
                    return false;
                }
            }
        }
        else
        {
            if (!isChildRecalculation)
                resetInstanceCFrame(start, end, originalCFrames);
            finalPartOperation->setParent(NULL);
            operationFailed("Warning", "Only parts, union parts, and negative parts are supported by union.");
            return false;
        }
    }

    if (!positiveMesh && !negativeMesh)
    {
        if (!isChildRecalculation)
            resetInstanceCFrame(start, end, originalCFrames);
        finalPartOperation->setParent(NULL);
        operationFailed("Union Unsolvable -6", unionFailedMsg);
        return false;
    }

    if (negativeMesh)
    {
        if (!positiveMesh)
        {
            positiveMesh.swap(negativeMesh);
            negativeMesh.reset();
        }
        else if (!positiveMesh->subractMesh(positiveMesh.get(), negativeMesh.get()))
        {
            if (!isChildRecalculation)
                resetInstanceCFrame(start, end, originalCFrames);
            finalPartOperation->setParent(NULL);
            operationFailed("Union Unsolvable -7", unionFailedMsg);
            return false;
        }
    }

    if (!positiveMesh->isValid())
    {
        if (!isChildRecalculation)
            resetInstanceCFrame(start, end, originalCFrames);
        finalPartOperation->setParent(NULL);
        operationFailed("Union Unsolvable -8", unionFailedMsg);
        return false;
    }
    if ( !positiveMesh->newTriangulate() )
    {
        if (FFlag::CSGNewTriangulateFallBack)
            positiveMesh->triangulate();
        else
        {
            if (!isChildRecalculation)
                resetInstanceCFrame(start, end, originalCFrames);
            operationFailed("Union Unsolvable -9", unionFailedMsg);
            return false;
        }
    }

    size_t numTriangles = positiveMesh->getIndices().size() / 3;

    if (numTriangles > PartOperation::getMaximumTriangleCount())
    {
        char buf[128];
        sprintf( buf, "This union would be %d triangles.  There is a %d triangle limit for unions.", (int)numTriangles, (int)PartOperation::getMaximumTriangleCount());
        if (!isChildRecalculation)
            resetInstanceCFrame(start, end, originalCFrames);
        finalPartOperation->setParent(NULL);
        operationFailed("Union Rejected", buf);
        return false;
    }
    
    if (positiveMesh->toBinaryString().size() > PartOperation::getMaximumMeshStreamSize())
    {
        char buf[128];
        sprintf( buf, "This union is too complex.  There is a %d triangle limit for unions.", (int)PartOperation::getMaximumTriangleCount());
        if (!isChildRecalculation)
            resetInstanceCFrame(start, end, originalCFrames);
        finalPartOperation->setParent(NULL);
        operationFailed("Union Rejected", buf);
        return false;
    }
    
    G3D::Vector3 trans = positiveMesh->extentsCenter();
    finalPartOperation->setCoordinateFrame(trans);

    for (std::vector<boost::shared_ptr<Instance> >::const_iterator iter = start; iter != end; ++iter)
    {
        if (boost::shared_ptr<PartInstance> partInstance = Instance::fastSharedDynamicCast<PartInstance>(*iter))
        {
            CoordinateFrame tmpFrame = partInstance->getCoordinateFrame();
            tmpFrame.translation = tmpFrame.translation - trans;
            partInstance->setCoordinateFrame(tmpFrame);
        }
    }
	if (!FFlag::CSGDelayParentingOperationToEnd && dataModel)
        finalPartOperation->setParent(parentToSet != NULL ? parentToSet : dataModel->getWorkspace());
    finalPartOperation->setPartSizeXml(positiveMesh->extentsSize());
    finalPartOperation->setInitialSize(finalPartOperation->getPartSizeUi());

    G3D::Vector3 negTrans = -trans;
    positiveMesh->applyTranslation(negTrans);
    positiveMesh->translate(negTrans);

    dictionaryService->retrieveMeshData(*finalPartOperation);
    finalPartOperation->setMesh(positiveMesh->clone());

    if (!finalPartOperation->checkDecompExists() && !finalPartOperation->createPhysicsData(positiveMesh.get()))
    {
        if (!isChildRecalculation)
            resetInstanceCFrame(start, end, originalCFrames);
        finalPartOperation->setParent(NULL);
        operationFailed("Union Rejected", "This union's Physics Data is too complex, simplify the model and try again");
        return false;
    }

    if (!isChildRecalculation)
    {
        //Move back to original position
        CoordinateFrame identityFrameDelta = CoordinateFrame(-finalPartOperation->getCoordinateFrame().translation);
        identityFrameDelta = CoordinateFrame(primaryDelta.rotation).inverse() * identityFrameDelta;

        finalPartOperation->setCoordinateFrame(CoordinateFrame(primaryDelta.rotation.inverse(), G3D::Vector3::zero()));
        finalPartOperation->setCoordinateFrame(finalPartOperation->getCoordinateFrame() - identityFrameDelta.translation - primaryDelta.translation);
    }


    return true;
}

bool RBX::CSGOperations::setNegateMesh(shared_ptr<NegateOperation> negateOperation, shared_ptr<Instance> instance)
{
    boost::scoped_ptr<CSGMesh> model;

	CSGDictionaryService* dictionaryService = FFlag::CSGDelayParentingOperationToEnd ?
		mDatamodel->create<CSGDictionaryService>() :
		ServiceProvider::create< CSGDictionaryService >(DataModel::get(negateOperation.get()));
	NonReplicatedCSGDictionaryService* nrDictionaryService = FFlag::CSGDelayParentingOperationToEnd ?
		mDatamodel->create<NonReplicatedCSGDictionaryService>() : ServiceProvider::create<NonReplicatedCSGDictionaryService>(DataModel::get(negateOperation.get()));

    if (shared_ptr<PartInstance> partInstance = Instance::fastSharedDynamicCast<PartInstance>(instance))
        partInstance->destroyJoints();

    if (shared_ptr<UnionOperation> unionOperation = Instance::fastSharedDynamicCast<UnionOperation>(instance))
    {
        warnIfChildInstances(unionOperation, "negation");

        dictionaryService->retrieveData(*unionOperation);
        nrDictionaryService->retrieveData(*unionOperation);

        if (!unionOperation->getMesh() || !unionOperation->getMesh()->isValid())
        {
            if (!recalculateMesh(unionOperation))
                return false;
        }

        model.reset(unionOperation->getMesh()->clone());

        model->applyScale(unionOperation->getSizeDifference());
        negateOperation->setInitialSize(unionOperation->getPartSizeUi());

        negateOperation->setCoordinateFrame(unionOperation->getCoordinateFrame());
        negateOperation->setPartSizeXml(unionOperation->getPartSizeUi());
    }
    else if (shared_ptr<PartInstance> partInstance = Instance::fastSharedDynamicCast<PartInstance>(instance))
    {
        warnIfChildInstances(partInstance, "negation");

        negateOperation->setCoordinateFrame(partInstance->getCoordinateFrame());
        negateOperation->setPartSizeXml(partInstance->getPartSizeUi());
        negateOperation->setInitialSize(negateOperation->getPartSizeUi());

        partInstance->getColor3();

        model.reset(CSGMeshFactory::singleton()->createMesh());

        bool partMeshCreated = createPartMesh(partInstance.get(), model.get());

        if (!partMeshCreated)
        {
            operationFailed("Warning", "This part or mesh type is not supported.");
            return false;
        }
    }
    else
    {
        operationFailed("Warning", "Only parts, union parts, and negative parts are supported for Negation.");
        return false;
    }

    if (!model || !model->isValid())
    {
        operationFailed("Warning", "Could not create model.");
        return false;
    }

    if (boost::shared_ptr<PartInstance> partInstance = Instance::fastSharedDynamicCast<PartInstance>(instance))
    {
        CoordinateFrame tmpFrame = partInstance->getCoordinateFrame();
        tmpFrame.translation = tmpFrame.translation - negateOperation->getCoordinateFrame().translation;
        partInstance->setCoordinateFrame(tmpFrame);
    }

    
    if ( !model->newTriangulate() )
    {
        if (FFlag::CSGNewTriangulateFallBack)
            model->triangulate();
        else
        {
            operationFailed("Union Unsolvable -10", unionFailedMsg);
            return false;
        }
    }
    negateOperation->setMesh(model->clone());
    // Temporary, Remove and fix after Merging with CO
    CSGMeshSgCore meshData = *static_cast<CSGMeshSgCore*>(negateOperation->getMesh().get());
    if (!negateOperation->checkDecompExists())
        negateOperation->createPhysicsData(&meshData);
    else
        negateOperation->createPlaceholderPhysicsData();
    return true;
}

bool RBX::CSGOperations::negateSelection(Instances& toRemove,
                                         Instances& toSelect,
                                         std::vector<boost::shared_ptr<Instance> >::const_iterator start, 
                                         std::vector<boost::shared_ptr<Instance> >::const_iterator end)
{
    CSGDictionaryService* dictionaryService = ServiceProvider::create< CSGDictionaryService >(mDatamodel);
    NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::create<NonReplicatedCSGDictionaryService>(mDatamodel);

    for (std::vector<boost::shared_ptr<Instance> >::const_iterator iter = start; iter != end; ++iter)
    {
        if (!(Instance::fastSharedDynamicCast<PartOperation>(*iter) ||
            Instance::fastSharedDynamicCast<NegateOperation>(*iter) ||
            Instance::fastSharedDynamicCast<PartInstance>(*iter)))

        {
            operationFailed("Warning", "Only parts, union parts, and negative parts are supported by negation.");
            return false;
        }
    }

    for (std::vector<boost::shared_ptr<Instance> >::const_iterator iter = start; iter != end; ++iter)
    {
        if (shared_ptr<NegateOperation> negateOperation = Instance::fastSharedDynamicCast<NegateOperation>(*iter))
        {
            dictionaryService->retrieveData(*negateOperation);
            nrDictionaryService->retrieveData(*negateOperation);
            Instances instances;
            std::stringstream ss(negateOperation->getChildDataBlocking().value());
            SerializerBinary::deserialize(ss, instances);

            Instances separatedItems;
            Instances itemsToDelete;
            bool didSomething = separate( negateOperation, separatedItems, itemsToDelete );

            if (didSomething)
            {
                toSelect.insert(toSelect.end(), separatedItems.begin(), separatedItems.end());
                toRemove.insert(toRemove.end(), itemsToDelete.begin(), itemsToDelete.end());
            }

            toRemove.push_back(negateOperation);
        }
        else if (shared_ptr<PartInstance> pInst = Instance::fastSharedDynamicCast<PartInstance>(*iter))
        {
            shared_ptr<NegateOperation> negateOperation;
            try
            {
                negateOperation = NegateOperation::createInstance();
				if (!FFlag::CSGDelayParentingOperationToEnd)
					negateOperation->setParent(pInst->getParent());
                toSelect.push_back(negateOperation);
            }
            catch (std::exception &)
            {
                operationFailed("Error", "Could not create part instance.");
                return false;
            }

            if (shared_ptr<PartOperation> pOperation = Instance::fastSharedDynamicCast<PartOperation>(pInst))
            {
                dictionaryService->retrieveData(*pOperation);
                nrDictionaryService->retrieveData(*pOperation);

                pOperation->setMeshData(BinaryString(""));
                pOperation->setPhysicsData(BinaryString(""));

                negateOperation->setUsePartColor(pOperation->getUsePartColor());
                negateOperation->setColor(pOperation->getColor());
            }

            bool validNegation = setNegateMesh(negateOperation, pInst);

            if (!validNegation)
                return false;

            shared_ptr<ModelInstance> groupInstance = ModelInstance::createInstance();
            shared_ptr<Instance> dupe = pInst->clone(SerializationCreator);
            dupe->setParent(groupInstance.get());
            
            std::stringstream ss;
            SerializerBinary::serialize(ss, groupInstance.get());
            negateOperation->setChildData(BinaryString(ss.str()));
            groupInstance->setParent(NULL);

			if (FFlag::CSGDelayParentingOperationToEnd)
			{
				negateOperation->setParent(pInst->getParent());
			}
			else
			{
	            dictionaryService->storeData(*negateOperation);
		        nrDictionaryService->storeData(*negateOperation);
			}

            toRemove.push_back(pInst);
        }
    }

    return true;
}

bool RBX::CSGOperations::separate(const shared_ptr<Instance>& wInstance,
                                  Instances &separatedItems,
                                  Instances &itemsToDelete)
{
    CSGDictionaryService* dictionaryService = ServiceProvider::create<CSGDictionaryService>(DataModel::get(wInstance.get()));
    NonReplicatedCSGDictionaryService* nrDictionaryService = ServiceProvider::create<NonReplicatedCSGDictionaryService>(DataModel::get(wInstance.get()));

    bool didSomethingHere = false;
    if (!dictionaryService || !nrDictionaryService)
        return didSomethingHere;

    if (shared_ptr<PartOperation> partOperation = Instance::fastSharedDynamicCast<PartOperation>(wInstance))
    {
        bool isNegate = false;

        if (Instance::fastSharedDynamicCast<NegateOperation>(partOperation))
            isNegate = true;

        Instances instances;
        dictionaryService->retrieveData(*partOperation);
        nrDictionaryService->retrieveData(*partOperation);
        std::stringstream ss(partOperation->getChildDataBlocking().value());
        SerializerBinary::deserialize(ss, instances);

        for (std::vector<boost::shared_ptr<Instance> >::const_iterator iter = instances.begin(); iter != instances.end(); ++iter)
        {
            if (shared_ptr<PartOperation> partOperationChild = Instance::fastSharedDynamicCast<PartOperation>(*iter))
                if (!recalculateMesh(partOperationChild))
                    return didSomethingHere;

            bool childNegate = false;

            if (Instance::fastSharedDynamicCast<NegateOperation>(*iter))
                childNegate = true;

            if (shared_ptr<PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(*iter))
            {
                CoordinateFrame newFrame = part->getCoordinateFrame();

                if (!isNegate || childNegate)
                    newFrame.rotation = partOperation->getCoordinateFrame().rotation * newFrame.rotation;
                else
                    newFrame.rotation = partOperation->getCoordinateFrame().rotation;

                newFrame.translation *= partOperation->getSizeDifference();
                newFrame.translation = (partOperation->getCoordinateFrame().rotation * newFrame.translation) + partOperation->getCoordinateFrame().translation;

                part->setCoordinateFrame(newFrame);
                part->setPartSizeXml(part->getPartSizeUi() * partOperation->getSizeDifference());
                separatedItems.push_back(part);
            }

            (*iter)->setParent(wInstance->getParent());
        }

        itemsToDelete.push_back(partOperation);

        didSomethingHere = true;
    }
    return didSomethingHere;
}

RBX::Instance* RBX::CSGOperations::getCommonParentOrWorkspace(Instances::const_iterator start,
	Instances::const_iterator end)
{
	Instance* parentMatch = (*start)->getParent();
	for (Instances::const_iterator itr = start; itr != end; ++itr)
	{
		if (parentMatch != (*itr)->getParent())
		{
			parentMatch = mDatamodel->getWorkspace();
			break;
		}
	}
	return parentMatch;
}
