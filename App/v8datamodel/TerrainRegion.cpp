#include "stdafx.h"

#include "V8DataModel/TerrainRegion.h"

#include "v8datamodel/MegaCluster.h"

#include "Voxel2/Conversion.h"

namespace RBX {

const char* const sTerrainRegion = "TerrainRegion";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<TerrainRegion, Vector3> prop_Extents("SizeInCells", category_Data, &TerrainRegion::getSizeInCells, NULL, Reflection::PropertyDescriptor::UI);

static Reflection::PropDescriptor<TerrainRegion, Vector3int16> prop_ExtentsMin("ExtentsMin", category_Data, &TerrainRegion::getExtentsMin, &TerrainRegion::setExtentsMin, Reflection::PropertyDescriptor::STREAMING);
static Reflection::PropDescriptor<TerrainRegion, Vector3int16> prop_ExtentsMax("ExtentsMax", category_Data, &TerrainRegion::getExtentsMax, &TerrainRegion::setExtentsMax, Reflection::PropertyDescriptor::STREAMING);

static Reflection::PropDescriptor<TerrainRegion, BinaryString> prop_GridV3("GridV3", category_Data, &TerrainRegion::getPackagedGridV3, &TerrainRegion::setPackagedGridV3, Reflection::PropertyDescriptor::CLUSTER, Security::None);

static Reflection::PropDescriptor<TerrainRegion, BinaryString> desc_SmoothGrid("SmoothGrid", category_Data, &TerrainRegion::getPackagedSmoothGrid, &TerrainRegion::setPackagedSmoothGrid, Reflection::PropertyDescriptor::CLUSTER, Security::None);

static Reflection::PropDescriptor<TerrainRegion, bool> prop_IsSmooth("IsSmooth", category_Data, &TerrainRegion::isSmooth, NULL, Reflection::PropertyDescriptor::UI, Security::None);

static Reflection::BoundFuncDesc<TerrainRegion, void(void)> func_convertToSmooth(&TerrainRegion::convertToSmooth, "ConvertToSmooth", Security::Plugin);
REFLECTION_END();

struct RegionHasher
{
	size_t operator()(const Voxel2::Region& value) const
	{
		size_t result = 0;
		boost::hash_combine(result, value.begin());
		boost::hash_combine(result, value.end());
		return result;
	}
};

static void copyEmptyVoxels(Voxel::Grid& target, const Region3int16& sourceExtents, const Vector3int32& offset, const std::vector<SpatialRegion::Id>& sourceRegions)
{
    boost::unordered_set<SpatialRegion::Id, SpatialRegion::Id::boost_compatible_hash_value> sourceRegionsSet(sourceRegions.begin(), sourceRegions.end());

    Region3int16 targetExtents(
        (Vector3int32(sourceExtents.getMinPos()) + offset).toVector3int16(),
        (Vector3int32(sourceExtents.getMaxPos()) + offset).toVector3int16());

    std::vector<SpatialRegion::Id> targetRegions = target.getNonEmptyChunksInRegion(targetExtents);

    for (size_t i = 0; i < targetRegions.size(); ++i)
    {
        SpatialRegion::Id regionId = targetRegions[i];
        Region3int16 regionExtents = SpatialRegion::inclusiveVoxelExtentsOfRegion(regionId);

        Vector3int16 minOffset = regionExtents.getMinPos().max(targetExtents.getMinPos());
        Vector3int16 maxOffset = regionExtents.getMaxPos().min(targetExtents.getMaxPos());

        if (offset == Vector3int32())
        {
            if (sourceRegionsSet.count(regionId) == 0)
            {
                for (int y = minOffset.y; y <= maxOffset.y; ++y)
                    for (int z = minOffset.z; z <= maxOffset.z; ++z)
                        for (int x = minOffset.x; x <= maxOffset.x; ++x)
                        {
                            Vector3int16 location(x, y, z);

                            target.setCell(location, Voxel::Constants::kUniqueEmptyCellRepresentation, Voxel::CELL_MATERIAL_Water);
                        }
            }
        }
        else
        {
            for (int y = minOffset.y; y <= maxOffset.y; ++y)
                for (int z = minOffset.z; z <= maxOffset.z; ++z)
                    for (int x = minOffset.x; x <= maxOffset.x; ++x)
                        if (sourceRegionsSet.count(SpatialRegion::regionContainingVoxel(Vector3int16(x - offset.x, y - offset.y, z - offset.z))) == 0)
                        {
                            Vector3int16 location(x, y, z);

                            target.setCell(location, Voxel::Constants::kUniqueEmptyCellRepresentation, Voxel::CELL_MATERIAL_Water);
                        }
        }
    }
}

static void copyVoxels(Voxel::Grid& target, const Voxel::Grid& source, const Region3int16& sourceExtents, const Vector3int32& offset, bool copyEmptyCells)
{
    std::vector<SpatialRegion::Id> sourceRegions = source.getNonEmptyChunksInRegion(sourceExtents);

    if (copyEmptyCells)
    {
        // Let's do a fast "copy" of empty voxels -- i.e. let's clear all filled voxels for the target grid that won't get set by the code below
        copyEmptyVoxels(target, sourceExtents, offset, sourceRegions);
    }

    for (size_t i = 0; i < sourceRegions.size(); ++i)
    {
        SpatialRegion::Id regionId = sourceRegions[i];
        Region3int16 regionExtents = SpatialRegion::inclusiveVoxelExtentsOfRegion(regionId);
        Voxel::Grid::Region region = source.getRegion(regionExtents.getMinPos(), regionExtents.getMaxPos());

        Vector3int16 minOffset = regionExtents.getMinPos().max(sourceExtents.getMinPos());
        Vector3int16 maxOffset = regionExtents.getMaxPos().min(sourceExtents.getMaxPos());

        for (int y = minOffset.y; y <= maxOffset.y; ++y)
            for (int z = minOffset.z; z <= maxOffset.z; ++z)
                for (int x = minOffset.x; x <= maxOffset.x; ++x)
                {
                    Vector3int16 location(x, y, z);

                    Voxel::Cell cell = region.voxelAt(location);

                    if (copyEmptyCells || !cell.isEmpty())
                    {
                        target.setCell(Vector3int16(x + offset.x, y + offset.y, z + offset.z), cell, region.materialAt(location));
                    }
                }
    }
}

static void copyVoxels(Voxel2::Grid& target, const Voxel2::Grid& source, const Region3int16& sourceExtents, const Vector3int32& offset, bool copyEmptyCells)
{
	Voxel2::Region sourceRegion(Vector3int32(sourceExtents.getMinPos()), Vector3int32(sourceExtents.getMaxPos()) + Vector3int32(1, 1, 1));

	std::vector<Voxel2::Region> sourceRegions = source.getNonEmptyRegionsInside(sourceRegion);

	if (copyEmptyCells && target.isAllocated())
	{
        // Let's do a fast "copy" of empty voxels -- i.e. let's clear all filled voxels for the target grid that won't get set by the code below
		boost::unordered_set<Voxel2::Region, RegionHasher> sourceRegionsSet(sourceRegions.begin(), sourceRegions.end());

		std::vector<Voxel2::Region> targetRegions = target.getNonEmptyRegionsInside(sourceRegion.offset(offset));

		for (auto& tr: targetRegions)
		{
			Voxel2::Region sr = tr.offset(-offset);

			if (sourceRegionsSet.find(sr) == sourceRegionsSet.end())
			{
				Voxel2::Box box(tr.size().x, tr.size().y, tr.size().z);

				target.write(tr, box);
			}
		}
	}

	for (auto& sr: sourceRegions)
	{
		Voxel2::Region tr = sr.offset(offset);

		Voxel2::Box sb = source.read(sr);

		if (!copyEmptyCells && target.isAllocated())
		{
			Voxel2::Box tb = target.read(tr);

			if (!tb.isEmpty())
			{
				Vector3int32 size = tb.getSize();

				for (int y = 0; y < size.y; ++y)
					for (int z = 0; z < size.z; ++z)
                    {
                        Voxel2::Cell* srow = sb.writeRow(0, y, z);
                        const Voxel2::Cell* trow = tb.readRow(0, y, z);

						for (int x = 0; x < size.x; ++x)
						{
                            Voxel2::Cell& c = srow[x];

							if (c.getMaterial() == Voxel2::Cell::Material_Air)
                                c = trow[x];
						}
                    }
			}

			target.write(tr, sb);
		}
		else
		{
			target.write(tr, sb);
		}
	}
}

TerrainRegion::TerrainRegion() 
    : DescribedCreatable<TerrainRegion, Instance, sTerrainRegion>("TerrainRegion")
{
}

TerrainRegion::TerrainRegion(const Voxel::Grid* otherGrid, const Region3int16& regionExtents)
    : DescribedCreatable<TerrainRegion, Instance, sTerrainRegion>("TerrainRegion")
    , extentsMin(regionExtents.getMinPos())
    , extentsMax(regionExtents.getMaxPos())
{
	initializeGridMega();

    copyVoxels(*voxelGrid, *otherGrid, regionExtents, Vector3int32(), /* copyEmptyCells= */ false);
}

TerrainRegion::TerrainRegion(const Voxel2::Grid* otherGrid, const Region3int16& regionExtents)
    : DescribedCreatable<TerrainRegion, Instance, sTerrainRegion>("TerrainRegion")
    , extentsMin(regionExtents.getMinPos())
    , extentsMax(regionExtents.getMaxPos())
{
	initializeGridSmooth();

	copyVoxels(*smoothGrid, *otherGrid, regionExtents, Vector3int32(), /* copyEmptyCells= */ false);
}

TerrainRegion::~TerrainRegion()
{
}

void TerrainRegion::copyTo(Voxel::Grid& otherGrid, const Vector3int16& corner, bool copyEmptyCells)
{
	if (!voxelGrid)
		throw std::runtime_error("TerrainRegion is smooth");

    Vector3int32 offset = Vector3int32(corner) - Vector3int32(extentsMin);

    // Since 16-bit integer computations can result in overflow, we need to adjust extents to only copy the relevant portion of the grid
    Region3int16 extentsLimit = Voxel::getTerrainExtentsInCells();

    Vector3int32 extentsMinLimit = Vector3int32(extentsLimit.getMinPos()) - offset;
    Vector3int32 extentsMaxLimit = Vector3int32(extentsLimit.getMaxPos()) - offset;

    Vector3int16 extentsMinClamped = extentsMinLimit.max(Vector3int32(extentsMin)).toVector3int16();
    Vector3int16 extentsMaxClamped = extentsMaxLimit.min(Vector3int32(extentsMax)).toVector3int16();

    copyVoxels(otherGrid, *voxelGrid, Region3int16(extentsMinClamped, extentsMaxClamped), offset, copyEmptyCells);
}

void TerrainRegion::copyTo(Voxel2::Grid& otherGrid, const Vector3int16& corner, bool copyEmptyCells)
{
	if (!smoothGrid)
		throw std::runtime_error("TerrainRegion is not smooth");

    Vector3int32 offset = Vector3int32(corner) - Vector3int32(extentsMin);

	copyVoxels(otherGrid, *smoothGrid, Region3int16(extentsMin, extentsMax), offset, copyEmptyCells);
}

void TerrainRegion::convertToSmooth()
{
    if (smoothGrid)
		throw std::runtime_error("TerrainRegion is already smooth!");

	if (!voxelGrid)
        throw std::runtime_error("Terrain API is not available");

	// Stash old grid somewhere and reinitialize as smooth grid
	scoped_ptr<Voxel::Grid> oldGrid;
	voxelGrid.swap(oldGrid);

	initializeGridSmooth();

    // Transfer data from voxel grid into smooth grid
	Voxel2::Conversion::convertToSmooth(*oldGrid, *smoothGrid);

	raisePropertyChanged(prop_IsSmooth);
}

void TerrainRegion::setPackagedGridV3(const BinaryString& data)
{
	if (data.value().empty())
        return;

	initializeGridMega();

    MegaClusterInstance::deserializeGridV3(*voxelGrid, data);
}

BinaryString TerrainRegion::getPackagedGridV3() const
{
	if (voxelGrid)
		return MegaClusterInstance::serializeGridV3(*voxelGrid);
	else
		return BinaryString();
}

void TerrainRegion::setPackagedSmoothGrid(const BinaryString& clusterData)
{
	if (clusterData.value().empty())
        return;

	initializeGridSmooth();

	smoothGrid->deserialize(clusterData.value());
}

BinaryString TerrainRegion::getPackagedSmoothGrid() const
{
    if (smoothGrid)
	{
        std::string result;
		smoothGrid->serialize(result);
        return BinaryString(result);
	}
	else
        return BinaryString();
}

Vector3 TerrainRegion::getSizeInCells() const
{
    return (Vector3int32(extentsMax) - Vector3int32(extentsMin) + Vector3int32::one()).toVector3();
}

void TerrainRegion::setExtentsMin(const Vector3int16& value)
{
    if (extentsMin != value)
    {
        extentsMin = value;
        raisePropertyChanged(prop_ExtentsMin);
    }
}

void TerrainRegion::setExtentsMax(const Vector3int16& value)
{
    if (extentsMax != value)
    {
        extentsMax = value;
        raisePropertyChanged(prop_ExtentsMax);
    }
}

bool TerrainRegion::askSetParent(const Instance* parent) const
{
	return true;
}

void TerrainRegion::initializeGridMega()
{
    RBXASSERT(!voxelGrid && !smoothGrid);

	voxelGrid.reset(new Voxel::Grid());
}

void TerrainRegion::initializeGridSmooth()
{
    RBXASSERT(!voxelGrid && !smoothGrid);

	smoothGrid.reset(new Voxel2::Grid());
}

}
