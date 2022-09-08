#pragma once

#include "v8tree/Instance.h"
#include "util/BinaryString.h"

#include "Voxel/Grid.h"
#include "voxel2/Grid.h"

namespace RBX {

extern const char* const sTerrainRegion;

class TerrainRegion
	: public DescribedCreatable<TerrainRegion, Instance, sTerrainRegion>
{
public:
	TerrainRegion();
	TerrainRegion(const Voxel::Grid* otherGrid, const Region3int16& regionExtents);
	TerrainRegion(const Voxel2::Grid* otherGrid, const Region3int16& regionExtents);
	virtual ~TerrainRegion();

    void copyTo(Voxel::Grid& otherGrid, const Vector3int16& corner, bool copyEmptyCells);
    void copyTo(Voxel2::Grid& otherGrid, const Vector3int16& corner, bool copyEmptyCells);

	void convertToSmooth();

    Vector3 getSizeInCells() const;

    Vector3int16 getExtentsMin() const { return extentsMin; }
    void setExtentsMin(const Vector3int16& value);

    Vector3int16 getExtentsMax() const { return extentsMax; }
    void setExtentsMax(const Vector3int16& value);

	bool isSmooth() const { return !!smoothGrid; }

	void setPackagedGridV3(const BinaryString& data);
	BinaryString getPackagedGridV3() const;

	void setPackagedSmoothGrid(const BinaryString& clusterData);
	BinaryString getPackagedSmoothGrid() const;

    virtual bool askSetParent(const Instance* parent) const;

private:
    scoped_ptr<Voxel::Grid> voxelGrid;
    scoped_ptr<Voxel2::Grid> smoothGrid;

    Vector3int16 extentsMin;
    Vector3int16 extentsMax;

	void initializeGridMega();
	void initializeGridSmooth();
};

}
