/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8World/CellContact.h"
#include "V8Kernel/PolyConnectors.h"
#include "V8Kernel/Kernel.h"
#include "V8World/Primitive.h"
#include "Voxel/Grid.h"
#include "V8DataModel/MegaCluster.h"

namespace RBX {

CellMeshContact::~CellMeshContact()
{
	deleteConnectors(polyConnectors);

	RBXASSERT(polyConnectors.size() == 0);
}

ContactConnector* CellMeshContact::getConnector(int i)
{
	return polyConnectors[i];
}

void CellMeshContact::deleteAllConnectors()
{
	deleteConnectors(polyConnectors);
}


void CellMeshContact::deleteConnectors(ConnectorArray& deleteConnectors)
{
	removeAllConnectorsFromKernel();

	for (size_t i = 0; i < deleteConnectors.size(); ++i) {
		RBXASSERT(!deleteConnectors[i]->isInKernel());
		delete deleteConnectors[i];
	}

	deleteConnectors.fastClear();
}


void CellMeshContact::removeAllConnectorsFromKernel()
{
	Kernel* kernel = NULL;
	for (size_t i = 0; i < polyConnectors.size(); ++i) {
		if (polyConnectors[i]->isInKernel()) {
			kernel = kernel ? kernel : getKernel();		// small optimization - getKernel walks the IPipelines
			kernel->removeConnector(polyConnectors[i]);
		}
	}
}

void CellMeshContact::putAllConnectorsInKernel()
{
	Kernel* kernel = NULL;
	for (size_t i = 0; i < polyConnectors.size(); ++i) {
		if (!polyConnectors[i]->isInKernel()  &&
			 polyConnectors[i]->getContactPoint().length < -ContactConnector::overlapGoal()) {
			kernel = kernel ? kernel : getKernel();		// small optimization - getKernel walks the IPipelines
			kernel->insertConnector(polyConnectors[i]);
		}
	}
}

bool CellMeshContact::stepContact()
{
	if (computeIsColliding(0.0f)) {
		if (inKernel()) {
			putAllConnectorsInKernel();
		}
		return true;
	}
	else {
		removeAllConnectorsFromKernel();
		return false;
	}
}

bool CellMeshContact::computeIsColliding(float overlapIgnored)
{
    updateClosestFeatures();
    if (polyConnectors.size() > 0) {
        float overlap = worstFeatureOverlap();
        if (overlap > overlapIgnored) {
            return true;
        }
    }
	return false;
}

void CellMeshContact::updateClosestFeatures()
{
	ConnectorArray newConnectors;

	findClosestFeatures(newConnectors);

	matchClosestFeatures(newConnectors);	// new Connectors is now the deal!

	deleteConnectors(polyConnectors);		// any remaining not matched

	polyConnectors = newConnectors;			// transfer over the pointers

	updateContactPoints();
}

float CellMeshContact::worstFeatureOverlap()
{
	float worstOverlap = -FLT_MAX;		// i.e. not overlapping
	RBXASSERT(polyConnectors.size() > 0);
	for (size_t i = 0; i < polyConnectors.size(); ++i) {				// may not have any overlapping features!
		float overlap = polyConnectors[i]->computeOverlap();				// computeLength returns negative
		worstOverlap = std::max(worstOverlap, overlap);
	}
	return worstOverlap;
}

// TODO - turn optimizer back on here after fixed
void CellMeshContact::matchClosestFeatures(ConnectorArray& newConnectors)
{
	for (size_t i = 0; i < newConnectors.size(); ++i) {
		if (PolyConnector* match = matchClosestFeature(newConnectors[i])) {
			delete newConnectors[i];
			newConnectors.replace(i, match);
		}
	}
}

PolyConnector* CellMeshContact::matchClosestFeature(PolyConnector* newConnector)
{
	for (size_t i = 0; i < polyConnectors.size(); ++i) {
		PolyConnector* answer = polyConnectors[i];
		if (PolyConnector::match(answer, newConnector)) {
			polyConnectors.fastRemove(i);
			return answer;
		}
	}
	return NULL;
}

void CellMeshContact::updateContactPoints()
{
	for (size_t i = 0; i < polyConnectors.size(); ++i)
		polyConnectors[i]->updateContactPoint();
}

bool CellMeshContact::cellFaceIsInterior(const Vector3int16& mainCellLoc, RBX::Voxel::FaceDirection faceDir)
{
    using namespace Voxel;
    
	Grid* vs = rbx_static_cast<MegaClusterInstance*>(getPrimitive(0)->getOwner())->getVoxelGrid();

    Grid::Region mainRegion = vs->getRegion(mainCellLoc, mainCellLoc);
    Voxel::Cell mainVoxel = mainRegion.voxelAt(mainCellLoc);

    const Vector3int16 neighborLoc = mainCellLoc + kFaceDirectionToLocationOffset[faceDir];
    Grid::Region neighborRegion = vs->getRegion(neighborLoc, neighborLoc);
    Voxel::Cell neighborVoxel = neighborRegion.voxelAt(neighborLoc);

    const Voxel::BlockAxisFace& mainFace = GetOrientedFace(mainVoxel, faceDir);
    const Voxel::BlockAxisFace& neighborFace = GetOrientedFace(neighborVoxel, oppositeSideOffset(faceDir));
    if (faceDir == Voxel::PlusY || faceDir == Voxel::MinusY)
        return mainFace.skippedCorner == Voxel::BlockAxisFace::YAxisMirror(neighborFace.skippedCorner);
    else
        return mainFace.skippedCorner == Voxel::BlockAxisFace::XZAxisMirror(neighborFace.skippedCorner);
}

} // namespace
