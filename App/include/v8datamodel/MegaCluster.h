/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "FastLog.h"
#include "V8DataModel/PartInstance.h"
#include "rbx/signal.h"
#include "util/PhysicalProperties.h"
#include "util/Region3int16.h"
#include "Util/BinaryString.h"
#include "Voxel/Cell.h"
#include "Voxel/Grid.h"

LOGGROUP(MegaClusterInit)
LOGGROUP(MegaClusterDirty)
LOGGROUP(MegaClusterDecodeStream)

namespace RBX { namespace Voxel2 { class MaterialTable; class Grid; } }

namespace RBX {

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern const char* const sMegaCluster;

class TerrainPartition;
class Heartbeat;

class MegaClusterInstance
	: public DescribedCreatable<MegaClusterInstance, PartInstance, sMegaCluster, Reflection::ClassDescriptor::PERSISTENT_HIDDEN>
{
public:
	typedef DescribedCreatable<MegaClusterInstance, PartInstance, sMegaCluster, Reflection::ClassDescriptor::PERSISTENT_HIDDEN> Super;

	MegaClusterInstance();  
	virtual ~MegaClusterInstance();

	/*override*/ virtual PartType getPartType() const;

	/*override*/ virtual void setPartSizeXml(const Vector3& rbxSize);
	/*override*/ virtual void setPartSizeUi(const Vector3& rbxSize);
	/*override*/ virtual void onAncestorChanged(const AncestorChanged& event);
	/*override*/ virtual void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);
	/*override*/ virtual void setTranslationUi(const Vector3& set);
	/*override*/ virtual void setAnchored(bool value);
	/*override*/ virtual bool getDragUtilitiesSupport() const { return false; }
	/*override*/ virtual Faces getResizeHandleMask() const		{return Faces(NORM_NONE_MASK); }
	/*override*/ virtual void setCoordinateFrame(const CoordinateFrame& value);
	/*override*/ virtual void destroyJoints();
	/*override*/ virtual shared_ptr<Instance> luaClone();
	/*override*/ virtual void destroy();
	/*override*/ virtual void join();
	/*override*/ virtual bool resize(NormalId localNormalId, int amount);
	/*override*/ virtual Extents computeExtentsWorld() const;
	/*override*/ virtual shared_ptr<const Instances> getTouchingParts();

	/*override*/ void verifySetParent(const Instance* instance) const;

	void initialize();

	// LUA functions
	shared_ptr<const Reflection::Tuple> getCellScript(int x, int y, int z);
	void setCellScript( int x, int y, int z, Voxel::CellMaterial material, Voxel::CellBlock block = Voxel::CELL_BLOCK_Solid, Voxel::CellOrientation orientation = Voxel::CELL_ORIENTATION_NegX );
	void setCellsScript( Region3int16 region, Voxel::CellMaterial material, Voxel::CellBlock block = Voxel::CELL_BLOCK_Solid, Voxel::CellOrientation orientation = Voxel::CELL_ORIENTATION_NegX );
	shared_ptr<const Reflection::Tuple> getWaterCellScript(int x, int y, int z);
	void setWaterCellScript( int x, int y, int z, Voxel::WaterCellForce force, Voxel::WaterCellDirection direction );

	bool autoWedgeCellScript( int x, int y, int z );
	void autoWedgeCellsScript( Region3int16 region );
	void autoWedgeCellsInternal( Region3int16 region );
	
	Vector3 cellCornerToWorld( const Vector3int16& v ) const;
	Extents cellToWorldExtents( const Vector3int16& v );

	Vector3 cellCornerToWorldScript( int x, int y, int z ) { return cellCornerToWorld(Vector3int16(x,y,z)); }
	Vector3 cellCenterToWorldScript( int x, int y, int z );

	Vector3 worldToCellPreferSolidScript( Vector3 position );
	Vector3 worldToCellPreferEmptyScript( Vector3 position );
	Vector3 worldToCellScript( Vector3 position );

	void convertToSmooth();

	int readVoxels(lua_State* L);
	int writeVoxels(lua_State* L);

	void fillRegion(Region3 region, float resolution, PartMaterial material);
	void fillBlock(CoordinateFrame cframe, Vector3 size, PartMaterial material);
	void fillBall(Vector3 center, float radius, PartMaterial material);

	void fillBallInternal(Vector3 center, float radius, PartMaterial material, bool skipWater);

    shared_ptr<Instance> copyRegion(Region3int16 region);
    void pasteRegion(shared_ptr<Instance> region, Vector3int16 corner, bool pasteEmptyCells);

	void clear();
	unsigned int getNonEmptyCellCount() const;
	int countCellsScript();

	const Region3int16 getMaxExtents() const;
	//

	Vector3int16 worldToCellWithPreference(const Vector3& worldpos, bool preferSolid);	

	/*override*/ virtual void render3dSelect(Adorn* adorn, SelectState selectState);
    /*override*/ Surface getSurface(const RbxRay& gridRay, int& surfaceId);

	void setPackagedClusterGridV1( const std::string& clusterData );

	void setPackagedClusterGridV2( const std::string& clusterData );
	std::string getPackagedClusteredGridV2() const;

	void setPackagedClusterGridV3( const BinaryString& clusterData );
	BinaryString getPackagedClusteredGridV3() const;

	void setPackagedSmoothGrid(const BinaryString& clusterData);
	BinaryString getPackagedSmoothGrid() const;

	int getSmoothReplicate() const { return smoothReplicate; }
    void setSmoothReplicate(int value);

    static BinaryString serializeGridV3(const Voxel::Grid& grid);
    static void deserializeGridV3(Voxel::Grid& grid, const BinaryString& data);

	// from class Selectable inherited through PartInstance
	virtual bool isSelectable3d() { return false; }
	bool isAllocated() const;

	Voxel::Grid* getVoxelGrid();
	Voxel2::Grid* getSmoothGrid();
	bool isSmooth() const;
	bool isInitialized() const;

    Color3 getWaterColor() const { return waterColor; }
    void setWaterColor(const Color3& value);

    float getWaterTransparency() const { return waterTransparency; }
    void setWaterTransparency(float value);
    
    float getWaterWaveSize() const { return waterWaveSize; }
    void setWaterWaveSize(float value);
    
    float getWaterWaveSpeed() const { return waterWaveSpeed; }
    void setWaterWaveSpeed(float value);

	Voxel2::MaterialTable* getMaterialTable() const { return materialTable.get(); }

    void reloadMaterialTable();

#define CLUSTER_CONST_PROP_OVERRIDE(ConstType, ArgType, FieldName, Ancestor, SetMethod) \
	static const ConstType kConst ## FieldName; \
	virtual void SetMethod(ArgType arg) { \
		Ancestor :: SetMethod(kConst ## FieldName); \
	}
	
	// Properties to make read only, and to only return a constant value
	CLUSTER_CONST_PROP_OVERRIDE(
		bool, bool, Archivable, Instance, setIsArchivable);
	CLUSTER_CONST_PROP_OVERRIDE(
		BrickColor, BrickColor, BrickColor, PartInstance, setColor);
	CLUSTER_CONST_PROP_OVERRIDE(
		bool, bool, CanCollide, PartInstance, setCanCollide);
	CLUSTER_CONST_PROP_OVERRIDE(
		float, float, Elasticity, PartInstance, setElasticity);
	CLUSTER_CONST_PROP_OVERRIDE(
		float, float, Friction, PartInstance, setFriction);
	CLUSTER_CONST_PROP_OVERRIDE(
		bool, bool, Locked, PartInstance, setPartLocked);
	CLUSTER_CONST_PROP_OVERRIDE(
		PartMaterial, PartMaterial, Material, PartInstance, setRenderMaterial);
	CLUSTER_CONST_PROP_OVERRIDE(
		std::string, const std::string&, Name, Instance, setName);
	CLUSTER_CONST_PROP_OVERRIDE(
		float, float, Reflectance, PartInstance, setReflectance);
	CLUSTER_CONST_PROP_OVERRIDE(
		Vector3, const Vector3&, RotVelocity, PartInstance, setRotationalVelocity);
	CLUSTER_CONST_PROP_OVERRIDE(
		float, float, Transparency, PartInstance, setTransparency);
	CLUSTER_CONST_PROP_OVERRIDE(
		Vector3, const Vector3&, Velocity, PartInstance, setLinearVelocity);
	CLUSTER_CONST_PROP_OVERRIDE(
		PhysicalProperties, PhysicalProperties, CustomPhysicalProperties, PartInstance, setPhysicalProperties);

private:
	// these defaults should all correspond with char cell = 0
	static const Voxel::CellMaterial kDefaultMaterial;
	static const Voxel::CellBlock kDefaultBlock;
	static const Voxel::CellOrientation kDefaultOrientation;

	CoordinateFrame clusterCoordinateFrame;

    int smoothReplicate;

	scoped_ptr<Voxel::Grid> voxelGrid;
	scoped_ptr<Voxel2::Grid> smoothGrid;

	scoped_ptr<Voxel2::MaterialTable> materialTable;

    Color3 waterColor;
    float waterTransparency;
    float waterWaveSpeed;
    float waterWaveSize;

	// Helper function for stepping through chunks when decoding
	void incrementCellInChunkIndex( Vector3int16& index ) const;

	// Helpers for worldToCellWithPreference
	bool isCellIdealForPreference(
		const Vector3int16& location, const Voxel::Cell cell,
		bool preferSolid) const;

	bool isCellAcceptablePreferenceWaterAlternative(
		const Vector3int16& location, const Voxel::Cell cell, bool preferSolid) const;

	template <class Stream> static void encodeChunkDataIntoStream(const Voxel::Grid::Region& chunk, Stream& s);
	template <class Stream> static void decodeChunkDataFromStream(Voxel::Grid& grid, const Vector3int16& chunkPos, Stream& encodedData);

	template <class Stream> void decodeChunkDataFromStreamV1_Deprecated(const Vector3int16& chunkPos, Stream& encodedData);

	void setCellInternalV1_Deprecated(const Vector3int16& pos, const unsigned char& cell);

    void initializeGridMega();
    void initializeGridSmooth();

	void validateApi();
	void validateApiSmooth();
};

} // namespace
