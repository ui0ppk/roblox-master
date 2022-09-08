/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/MegaCluster.h"

#include "G3D/CollisionDetection.h"
#include "Reflection/Reflection.h"
#include "util/Region3int16.h"
#include "util/RunStateOwner.h"
#include "util/stringbuffer.h"
#include "V8DataModel/Workspace.h"
#include "V8World/Primitive.h"
#include "V8World/MegaClusterPoly.h"
#include "V8World/SmoothClusterGeometry.h"
#include "V8World/TerrainPartition.h"
#include "V8World/World.h"
#include "V8World/ContactManager.h"
#include "Voxel/CellChangeListener.h"
#include "V8DataModel/TerrainRegion.h"
#include "Network/Players.h"
#include "v8datamodel/ChangeHistory.h"

#include "Voxel2/Grid.h"
#include "Voxel2/MaterialTable.h"
#include "Voxel2/Mesher.h"
#include "Voxel2/Conversion.h"

#include "script/LuaArguments.h"

LOGVARIABLE(TerrainCellListener, 0)

FASTINTVARIABLE(SmoothTerrainMaxLuaRegion, 4*1024*1024)
FASTINTVARIABLE(SmoothTerrainMaxCppRegion, 64*1024*1024)

namespace RBX
{
using namespace Voxel;

const Vector3int16 kLegacyChunkCount(16, 4, 16);
const Vector3int16 kLegacyChunkOffset(8, 0, 8);

static const ContentId kSmoothTerrainMaterials("rbxasset://terrain/materials.json");

namespace Reflection
{
	template<> Reflection::EnumDesc<CellMaterial>::EnumDesc()
		:RBX::Reflection::EnumDescriptor("CellMaterial")
	{
		addPair(CELL_MATERIAL_Deprecated_Empty, "Empty");
		addPair(CELL_MATERIAL_Grass, "Grass");
		addPair(CELL_MATERIAL_Sand, "Sand");
		addPair(CELL_MATERIAL_Brick, "Brick");
		addPair(CELL_MATERIAL_Granite, "Granite");
		addPair(CELL_MATERIAL_Asphalt, "Asphalt");
		addPair(CELL_MATERIAL_Iron, "Iron");
		addPair(CELL_MATERIAL_Aluminum, "Aluminum");
		addPair(CELL_MATERIAL_Gold, "Gold");
		addPair(CELL_MATERIAL_Wood_Plank, "WoodPlank");
		addPair(CELL_MATERIAL_Wood_Log, "WoodLog");
		addPair(CELL_MATERIAL_Gravel, "Gravel");
		addPair(CELL_MATERIAL_Cinder_Block, "CinderBlock");
		addPair(CELL_MATERIAL_Stone_Block, "MossyStone");
		addPair(CELL_MATERIAL_Cement, "Cement");
		addPair(CELL_MATERIAL_Red_Plastic, "RedPlastic");
		addPair(CELL_MATERIAL_Blue_Plastic, "BluePlastic");
		addPair(CELL_MATERIAL_Water, "Water");
	}
	template<>
	CellMaterial& Variant::convert<CellMaterial>(void)
	{
		return genericConvert<CellMaterial>();
	}

	template<> Reflection::EnumDesc<CellBlock>::EnumDesc()
		:RBX::Reflection::EnumDescriptor("CellBlock")
	{
		addPair(CELL_BLOCK_Solid, "Solid" );
		addPair(CELL_BLOCK_VerticalWedge, "VerticalWedge" );
		addPair(CELL_BLOCK_CornerWedge, "CornerWedge" );
		addPair(CELL_BLOCK_InverseCornerWedge, "InverseCornerWedge" );
		addPair(CELL_BLOCK_HorizontalWedge, "HorizontalWedge" );
	}
	template<>
	CellBlock& Variant::convert<CellBlock>(void)
	{
		return genericConvert<CellBlock>();
	}

	template<> Reflection::EnumDesc<CellOrientation>::EnumDesc()
		:RBX::Reflection::EnumDescriptor("CellOrientation")
	{
		addPair(CELL_ORIENTATION_NegZ, "NegZ");
		addPair(CELL_ORIENTATION_X, "X");
		addPair(CELL_ORIENTATION_Z, "Z");
		addPair(CELL_ORIENTATION_NegX, "NegX");
	}
	template<>
	CellOrientation& Variant::convert<CellOrientation>(void)
	{
		return genericConvert<CellOrientation>();
	}

	template<> Reflection::EnumDesc<WaterCellForce>::EnumDesc()
		:RBX::Reflection::EnumDescriptor("WaterForce")
	{
		addPair(WATER_CELL_FORCE_None, "None");
		addPair(WATER_CELL_FORCE_Small, "Small");
		addPair(WATER_CELL_FORCE_Medium, "Medium");
		addPair(WATER_CELL_FORCE_Strong, "Strong");
		addPair(WATER_CELL_FORCE_MaxForce, "Max");
	}
	template<>
	WaterCellForce& Variant::convert<WaterCellForce>(void)
	{
		return genericConvert<WaterCellForce>();
	}

	template<> Reflection::EnumDesc<WaterCellDirection>::EnumDesc()
		:RBX::Reflection::EnumDescriptor("WaterDirection")
	{
		addPair(WATER_CELL_DIRECTION_NegX, "NegX");
		addPair(WATER_CELL_DIRECTION_X, "X");
		addPair(WATER_CELL_DIRECTION_NegY, "NegY");
		addPair(WATER_CELL_DIRECTION_Y, "Y");
		addPair(WATER_CELL_DIRECTION_NegZ, "NegZ");
		addPair(WATER_CELL_DIRECTION_Z, "Z");
	}
	template<>
	WaterCellDirection& Variant::convert<WaterCellDirection>(void)
	{
		return genericConvert<WaterCellDirection>();
	}
}

template<>
bool RBX::StringConverter<CellMaterial>::convertToValue(const std::string& text, CellMaterial& value)
{
	return Reflection::EnumDesc<CellMaterial>::singleton().convertToValue(text.c_str(),value);
}
template<>
bool RBX::StringConverter<CellBlock>::convertToValue(const std::string& text, CellBlock& value)
{
	return Reflection::EnumDesc<CellBlock>::singleton().convertToValue(text.c_str(),value);
}
template<>
bool RBX::StringConverter<CellOrientation>::convertToValue(const std::string& text, CellOrientation& value)
{
	return Reflection::EnumDesc<CellOrientation>::singleton().convertToValue(text.c_str(),value);
}
template<>
bool RBX::StringConverter<WaterCellForce>::convertToValue(const std::string& text, WaterCellForce& value)
{
	return Reflection::EnumDesc<WaterCellForce>::singleton().convertToValue(text.c_str(),value);
}
template<>
bool RBX::StringConverter<WaterCellDirection>::convertToValue(const std::string& text, WaterCellDirection& value)
{
	return Reflection::EnumDesc<WaterCellDirection>::singleton().convertToValue(text.c_str(),value);
}


// these defaults should all correspond with char cell = 0
const CellMaterial MegaClusterInstance::kDefaultMaterial = CELL_MATERIAL_Deprecated_Empty;
const CellBlock MegaClusterInstance::kDefaultBlock = CELL_BLOCK_Solid;
const CellOrientation MegaClusterInstance::kDefaultOrientation = CELL_ORIENTATION_NegZ;

const char* const sMegaCluster = "Terrain";

using namespace Reflection;

REFLECTION_BEGIN();
// LUA property reflections
static Reflection::PropDescriptor<MegaClusterInstance, std::string> desc_ClusterGridV1("ClusterGrid", category_Data, NULL, &MegaClusterInstance::setPackagedClusterGridV1, Reflection::PropertyDescriptor::LEGACY, Security::None);
static Reflection::PropDescriptor<MegaClusterInstance, std::string> desc_ClusterGridV2("ClusterGridV2", category_Data, NULL, &MegaClusterInstance::setPackagedClusterGridV2, Reflection::PropertyDescriptor::LEGACY, Security::None);
static Reflection::PropDescriptor<MegaClusterInstance, BinaryString> desc_ClusterGridV3("ClusterGridV3", category_Data, &MegaClusterInstance::getPackagedClusteredGridV3, &MegaClusterInstance::setPackagedClusterGridV3, Reflection::PropertyDescriptor::CLUSTER, Security::None);

static Reflection::PropDescriptor<MegaClusterInstance, BinaryString> desc_SmoothGrid("SmoothGrid", category_Data, &MegaClusterInstance::getPackagedSmoothGrid, &MegaClusterInstance::setPackagedSmoothGrid, Reflection::PropertyDescriptor::CLUSTER, Security::None);

static Reflection::PropDescriptor<MegaClusterInstance, int> desc_SmoothReplicate("SmoothReplicate", category_Data, &MegaClusterInstance::getSmoothReplicate, &MegaClusterInstance::setSmoothReplicate, Reflection::PropertyDescriptor::REPLICATE_ONLY, Security::None);

static Reflection::PropDescriptor<MegaClusterInstance, Region3int16> prop_MaxExtents("MaxExtents", category_Data, &MegaClusterInstance::getMaxExtents, NULL,  PropertyDescriptor::UI );

// LUA function reflections
static Reflection::BoundFuncDesc<MegaClusterInstance, shared_ptr<const Reflection::Tuple>(int,int,int)> func_getCell(&MegaClusterInstance::getCellScript, 
																					 "GetCell", "x", "y", "z", Security::None);
static Reflection::BoundFuncDesc<MegaClusterInstance, void(int,int,int,CellMaterial,CellBlock,CellOrientation)> 
	func_setCell(&MegaClusterInstance::setCellScript, "SetCell", "x", "y", "z", "material", "block", "orientation", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, void(Region3int16,CellMaterial,CellBlock,CellOrientation)> 
	func_setCells(&MegaClusterInstance::setCellsScript, "SetCells", "region", "material", "block", "orientation", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, shared_ptr<const Reflection::Tuple>(int,int,int)>
	func_getWaterCell(&MegaClusterInstance::getWaterCellScript,"GetWaterCell", "x", "y", "z", Security::None);
static Reflection::BoundFuncDesc<MegaClusterInstance, void(int,int,int,WaterCellForce,WaterCellDirection)> 
	func_setWaterCell(&MegaClusterInstance::setWaterCellScript, "SetWaterCell", "x", "y", "z", "force", "direction", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, bool(int, int, int)>
	func_autoWedgeCell(&MegaClusterInstance::autoWedgeCellScript, "AutowedgeCell", "x", "y", "z", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, void(Region3int16)>
	func_autoWedgeCells(&MegaClusterInstance::autoWedgeCellsScript, "AutowedgeCells", "region", Security::None );


static Reflection::BoundFuncDesc<MegaClusterInstance, Vector3(int, int, int)> 
	func_cellCornerToWorld(&MegaClusterInstance::cellCornerToWorldScript, "CellCornerToWorld", "x", "y", "z", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, Vector3(int, int, int)> 
	func_cellCenterToWorld(&MegaClusterInstance::cellCenterToWorldScript, "CellCenterToWorld", "x", "y", "z", Security::None );

static Reflection::BoundFuncDesc<MegaClusterInstance, Vector3(Vector3)>
	func_worldToCellPreferSolid(&MegaClusterInstance::worldToCellPreferSolidScript, "WorldToCellPreferSolid", "position", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, Vector3(Vector3)>
	func_worldToCellPreferEmpty(&MegaClusterInstance::worldToCellPreferEmptyScript, "WorldToCellPreferEmpty", "position", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, Vector3(Vector3)>
	func_worldToCell(&MegaClusterInstance::worldToCellScript, "WorldToCell", "position", Security::None );
	
static Reflection::BoundFuncDesc<MegaClusterInstance, void(void)> 
	func_clear(&MegaClusterInstance::clear, "Clear", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, int(void)> 
	func_countCells(&MegaClusterInstance::countCellsScript, "CountCells", Security::None );

static Reflection::BoundFuncDesc<MegaClusterInstance, shared_ptr<Instance> (Region3int16)>
	func_copyRegion(&MegaClusterInstance::copyRegion, "CopyRegion", "region", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, void (shared_ptr<Instance>, Vector3int16, bool)>
	func_pasteRegion(&MegaClusterInstance::pasteRegion, "PasteRegion", "region", "corner", "pasteEmptyCells", Security::None );

static Reflection::PropDescriptor<MegaClusterInstance, bool>
	prop_IsSmooth("IsSmooth", category_Data, &MegaClusterInstance::isSmooth, NULL, Reflection::PropertyDescriptor::UI, Security::None);
static Reflection::BoundFuncDesc<MegaClusterInstance, void(void)> 
	func_convertToSmooth(&MegaClusterInstance::convertToSmooth, "ConvertToSmooth", Security::Plugin);

static Reflection::CustomBoundFuncDesc<MegaClusterInstance, shared_ptr<const Reflection::Tuple>(Region3, float)>
	func_readVoxels(&MegaClusterInstance::readVoxels, "ReadVoxels", "region", "resolution", Security::None );
static Reflection::CustomBoundFuncDesc<MegaClusterInstance, void(Region3, float, shared_ptr<const Reflection::ValueArray>, shared_ptr<const Reflection::ValueArray>)>
	func_writeVoxels(&MegaClusterInstance::writeVoxels, "WriteVoxels", "region", "resolution", "materials", "occupancy", Security::None );

static Reflection::BoundFuncDesc<MegaClusterInstance, void(Region3, float, PartMaterial)>
	func_fillRegion(&MegaClusterInstance::fillRegion, "FillRegion", "region", "resolution", "material", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, void(CoordinateFrame, Vector3, PartMaterial)>
	func_fillBlock(&MegaClusterInstance::fillBlock, "FillBlock", "cframe", "size", "material", Security::None );
static Reflection::BoundFuncDesc<MegaClusterInstance, void(Vector3, float, PartMaterial)>
	func_fillBall(&MegaClusterInstance::fillBall, "FillBall", "center", "radius", "material", Security::None );
REFLECTION_END();

static Reflection::PropDescriptor<MegaClusterInstance, Color3> desc_WaterColor("WaterColor", category_Appearance, &MegaClusterInstance::getWaterColor, &MegaClusterInstance::setWaterColor);
static Reflection::PropDescriptor<MegaClusterInstance, float> desc_WaterTransparency("WaterTransparency", category_Appearance, &MegaClusterInstance::getWaterTransparency, &MegaClusterInstance::setWaterTransparency);
static Reflection::PropDescriptor<MegaClusterInstance, float> desc_WaterWaveSize("WaterWaveSize", category_Appearance, &MegaClusterInstance::getWaterWaveSize, &MegaClusterInstance::setWaterWaveSize);
static Reflection::PropDescriptor<MegaClusterInstance, float> desc_WaterWaveSpeed("WaterWaveSpeed", category_Appearance, &MegaClusterInstance::getWaterWaveSpeed, &MegaClusterInstance::setWaterWaveSpeed);

// Properties to make read-only with constant return values
const bool MegaClusterInstance::kConstArchivable = true;
const BrickColor MegaClusterInstance::kConstBrickColor = BrickColor();
const bool MegaClusterInstance::kConstCanCollide = true;
const float MegaClusterInstance::kConstElasticity(0.3f);
const float MegaClusterInstance::kConstFriction(0.5f);
const bool MegaClusterInstance::kConstLocked = true;
const PartMaterial MegaClusterInstance::kConstMaterial(PLASTIC_MATERIAL);
const PhysicalProperties MegaClusterInstance::kConstCustomPhysicalProperties = PhysicalProperties();

const std::string MegaClusterInstance::kConstName("Terrain");
const float MegaClusterInstance::kConstReflectance = 0;
const Vector3 MegaClusterInstance::kConstRotVelocity(0, 0, 0);
const float MegaClusterInstance::kConstTransparency = 0;
const Vector3 MegaClusterInstance::kConstVelocity(0, 0, 0);

// Member methods
void MegaClusterInstance::destroyJoints() {
	// Cannot throw as long as this extends PartInstance because other
	// property updates trigger destroyJoints().
}

shared_ptr<Instance> MegaClusterInstance::luaClone() {
	throw std::runtime_error("Cannot Clone() Terrain");
}

void MegaClusterInstance::destroy() {
	throw std::runtime_error("Cannot Destroy() Terrain");
}

void MegaClusterInstance::join() {
	// Cannot throw as long as this extends PartInstance because other
	// property updates trigger join().
}

bool MegaClusterInstance::resize(NormalId normalId, int deltaAmount) {
	throw std::runtime_error("Cannot Resize() Terrain");
}

shared_ptr<const Instances> MegaClusterInstance::getTouchingParts()
{
	throw RBX::runtime_error("GetTouchingParts is not a valid member of Terrain");
}

MegaClusterInstance::MegaClusterInstance()
    : smoothReplicate(-1)
    , waterColor(0.05f, 0.33f, 0.36f)
    , waterTransparency(0.3f)
    , waterWaveSize(0.15f)
    , waterWaveSpeed(10.f)
{
	FASTLOG1(FLog::MegaClusterInit, "MegaCluster created - %p", this);

	Vector3 terrainSize = Vector3(511*4,63*4,511*4);
	setName( "Terrain" );
	PartInstance::setPartSizeXml(terrainSize);
	setAnchored(true);
	setPartLocked(true);

	setElasticity(kConstElasticity);
	setFriction(kConstFriction);

	// Initialize tables for legacy terrain
    initBlockOrientationFaceMap();

	// Initialize tables for smooth terrain
	materialTable.reset(new Voxel2::MaterialTable(ContentProvider::findAsset(kSmoothTerrainMaterials), Voxel2::Cell::Material_Max + 1));
	
	Voxel2::Mesher::prepareTables();
}

MegaClusterInstance::~MegaClusterInstance()
{
	FASTLOG1(FLog::MegaClusterInit, "MegaCluster destroyed - %p", this);

	Workspace* parentWorkspace = dynamic_cast<Workspace*>( this->getParent() );
	if( parentWorkspace ) parentWorkspace->setTerrain( NULL );

    // This makes sure we delete physics geometry before voxel grid
    getPartPrimitive()->setGeometryType(Geometry::GEOMETRY_BLOCK);
}

void MegaClusterInstance::setAnchored(bool value)
{
	// Mega Clusters are ALWAYS anchored
	Super::setAnchored( true );
}

void MegaClusterInstance::setPartSizeXml(const Vector3& rbxSize)  
{
	// do nothing. this property should be essentially read only
}

void MegaClusterInstance::setPartSizeUi(const Vector3& rbxSize)
{
	// do nothing. this property should be essentially read only
}

void MegaClusterInstance::setTranslationUi(const Vector3& set)
{
	// do nothing. this property should be essentially read only
}

void MegaClusterInstance::setCoordinateFrame(const CoordinateFrame& value)
{
	// Disregard any input coordinate frame. Instead, send a fixed cluster
	// coordinate frame
	PartInstance::setCoordinateFrame(clusterCoordinateFrame);
}

void MegaClusterInstance::verifySetParent(const Instance* instance) const
{
	Super::verifySetParent(instance);

	const Workspace* workspace = instance != NULL ?
		instance->fastDynamicCast<Workspace>() : NULL;
	if( instance != NULL && ((workspace == NULL) || (workspace->getTerrain() != NULL)) ){
		throw RBX::runtime_error("Unable to change Terrain's parent. Workspace already has Terrain");
	}

	if (instance)
		const_cast<MegaClusterInstance*>(this)->initialize();
}

void MegaClusterInstance::initialize()
{
	if (!voxelGrid && !smoothGrid)
	{
		if (smoothReplicate == 1)
            initializeGridSmooth(); // terrain replicated from server, smooth
		else if (smoothReplicate == 0)
            initializeGridMega(); // terrain replicated from server, mega
		else
            initializeGridMega(); // default terrain, mega (for now!)
	}
}

PartType MegaClusterInstance::getPartType() const
{
	return MEGACLUSTER_PART;
	//return BLOCK_PART;
}

bool MegaClusterInstance::isAllocated() const
{
    if (smoothGrid)
		return smoothGrid->isAllocated();

    if (voxelGrid)
		return voxelGrid->isAllocated();

    return false;
}

void MegaClusterInstance::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider, newProvider);
	FASTLOG2(FLog::MegaClusterInit, "onServiceProvider, oldProvider: %p, new parent: %p", oldProvider, newProvider);

	if(newProvider == NULL && oldProvider != NULL)
	{
		Workspace* workspace = Workspace::findWorkspace(oldProvider);
		if(workspace->getTerrain() == this)
		{
			FASTLOG(FLog::MegaClusterInit, "onServiceProvider: Clearing terrain instance");
			workspace->setTerrain(NULL);
		}
	}
}

void MegaClusterInstance::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);

	FASTLOG1(FLog::MegaClusterInit, "Ancestor change, new parent: %p", event.newParent);
	if(event.newParent == NULL)
	{
		return;
	}

	Workspace* workspace = Workspace::findWorkspace(event.newParent);
	
	if(!workspace->getTerrain())
	{
		FASTLOG(FLog::MegaClusterInit, "No terrain yet - it's going go be me!");
		workspace->setTerrain(this);		
	}
	else if( workspace->getTerrain() != this )
	{
		FASTLOG(FLog::MegaClusterInit, "Terrain already exists - replace the existing one with this one");
		workspace->setTerrain(this);		
	}
}

shared_ptr<const Reflection::Tuple> MegaClusterInstance::getCellScript(int x, int y, int z)
{
	validateApi();

	if (smoothGrid)
	{
		using namespace Voxel2::Conversion;

		Voxel2::Cell cell = smoothGrid->getCell(x, y, z);

		shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(3));

		if (cell.getMaterial() == Voxel2::Cell::Material_Water)
		{
			result->values[0] = CELL_MATERIAL_Water;
			result->values[1] = kDefaultBlock;
			result->values[2] = kDefaultOrientation;
		}
		else
		{
			CellBlock block = getCellBlockFromCell(cell);

			if (block == CELL_BLOCK_Empty)
			{
                result->values[0] = kDefaultMaterial;
				result->values[1] = kDefaultBlock;
				result->values[2] = kDefaultOrientation;
			}
			else
			{
				result->values[0] = getCellMaterialFromMaterial(getMaterialFromVoxelMaterial(cell.getMaterial()));
				result->values[1] = block;
				result->values[2] = kDefaultOrientation;
			}
		}

		return result;
	}
	else
	{
		Vector3int16 internalPos = Vector3int16(x,y,z);
		Cell cellValue = voxelGrid->getCell(internalPos);
		Cell waterCell = voxelGrid->getWaterCell(internalPos);
		CellMaterial cellMaterial = (CellMaterial) voxelGrid->getCellMaterial(internalPos);
		shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(3));

		if ( cellValue.solid.getBlock() == CELL_BLOCK_Empty )
		{
			if (!waterCell.isEmpty()) {
				result->values[0] = CELL_MATERIAL_Water;
			} else {
				result->values[0] = kDefaultMaterial;
			}
			result->values[1] = kDefaultBlock;
			result->values[2] = kDefaultOrientation;
		} else {
			result->values[0] = cellMaterial;
			result->values[1] = cellValue.solid.getBlock();
			result->values[2] = cellValue.solid.getOrientation();
		}
		
		return result;
	}
}

void MegaClusterInstance::setCellsScript(Region3int16 region, CellMaterial material, CellBlock block, CellOrientation orientation )
{
	validateApi();

	if (smoothGrid)
	{
		Vector3int16 minPos = region.getMinPos();
		Vector3int16 maxPos = region.getMaxPos();

		if (minPos.x <= maxPos.x && minPos.y <= maxPos.y && minPos.z <= maxPos.z)
		{
            Voxel2::Region rr(Vector3int32(minPos), Vector3int32(maxPos) + Vector3int32::one());
            Vector3int32 rsize = rr.size();

            Voxel2::Box box(rsize.x, rsize.y, rsize.z);

			using namespace Voxel2::Conversion;

			Voxel2::Cell cell =
				(material == CELL_MATERIAL_Water)
				? Voxel2::Cell(Voxel2::Cell::Material_Water, Voxel2::Cell::Occupancy_Max)
				:
					(material == CELL_MATERIAL_Deprecated_Empty || block == CELL_BLOCK_Empty)
					? Voxel2::Cell()
					: Voxel2::Cell(getVoxelMaterialFromMaterial(getMaterialFromCellMaterial(material)), getOccupancyFromSolidBlock(block));

            if (cell.getMaterial() != Voxel2::Cell::Material_Air)
            {
    			for (int y = 0; y < rsize.y; ++y)
    				for (int z = 0; z < rsize.z; ++z)
                    {
                        Voxel2::Cell* row = box.writeRow(0, y, z);

    					for (int x = 0; x < rsize.x; ++x)
                            row[x] = cell;
                    }
            }

            smoothGrid->write(rr, box);
		}
	}
	else
	{
		Vector3int16 minPos = region.getMinPos();
		Vector3int16 maxPos = region.getMaxPos();

		// Data is stored in (Y > Z > X) format [so Y is most significant index and X is least in pointer arithmetic], so traverse in this order for significant speed-up!
		for( int j = minPos.y; j <= maxPos.y; ++j )
		{
			for( int k = minPos.z; k <= maxPos.z; ++k )
			{
				for( int i = minPos.x; i <= maxPos.x; ++i )
				{
					setCellScript(i, j, k, material, block, orientation);
				}
			}
		}
	}
}

bool MegaClusterInstance::autoWedgeCellScript(int scriptX, int scriptY, int scriptZ)
{
	validateApi();

	if (smoothGrid)
		return false;

	Vector3int16 internalCellPos = Vector3int16(scriptX, scriptY, scriptZ);
	Cell currentCell = voxelGrid->getCell(internalCellPos);
	CellMaterial cellMat = voxelGrid->getCellMaterial(internalCellPos);
	if (currentCell.solid.getBlock() != CELL_BLOCK_Empty) {

		Cell cell;

		bool surroundings[3][2][3];
		bool sides[4];
		bool corners[4];
		bool sidesAbove[4];
		bool cornersAbove[4];
		int adjacentSides = 0;
		int adjacentSidesAbove = 0;
		int adjacentCorners = 0;
		int adjacentCornersAbove = 0;
		bool blockBelow = (voxelGrid->getCell(Vector3int16(internalCellPos.x, internalCellPos.y - 1, internalCellPos.z)).solid.getBlock() != CELL_BLOCK_Empty);

		// build out the surrounding geometry
		for ( int j = 0; j <= 1; ++j ){
			for ( int k = 0; k <= 2; ++k ){
				for ( int i = 0; i <= 2; ++i ){
					surroundings[i][j][k] = (voxelGrid->getCell(Vector3int16(internalCellPos.x - 1 + i, internalCellPos.y + j, internalCellPos.z - 1 + k)).solid.getBlock() != CELL_BLOCK_Empty);
				}
			}
		}
		
		for ( int step = 0; step <= 2; step += 2 ){
			sides[step] = surroundings[step][0][1];
			sides[step + 1] = surroundings[1][0][2 - step];

			sidesAbove[step] = surroundings[step][1][1];
			sidesAbove[step + 1] = surroundings[1][1][2 - step];

			corners[step] = surroundings[step][0][step];
			corners[step + 1] = surroundings[step][0][2 - step];

			cornersAbove[step] = surroundings[step][1][step];
			cornersAbove[step + 1] = surroundings[step][1][2 - step];
		}
		
		for ( int i = 0; i < 4; ++i ){
			if (sides[i]) adjacentSides++;
			if (corners[i]) adjacentCorners++;
			if (sidesAbove[i]) adjacentSidesAbove++;
			if (cornersAbove[i]) adjacentCornersAbove++;
		}
		
		// Figure out which cell to autowedge to:
		CellBlock wedge;
		CellOrientation rotation;
		// type 1: 45 degree ramp [must not have a block on top and be surrounded by 1 side; or 3 sides and the 2 corners between them]
		if (!surroundings[1][1][1])
		{
			if (adjacentSides == 1 && blockBelow) // if only surrounded by 1 side, then MUST have a block beneath it
			{
				for (int n = 0; n < 4; ++n )
				{
					if (sides[n])
					{
						wedge = (CellBlock)1;
						rotation = (CellOrientation)((n + 1) % 4);

						cell.solid.setBlock(wedge);
						cell.solid.setOrientation(rotation);
						voxelGrid->setCell(internalCellPos, cell, cellMat);
						return true;
					}	
				}
			}
			else if (adjacentSides == 3)
			{
				for (int n = 0; n < 4; ++n )
				{
					if (sides[n] && corners[(n + 1) % 4] && sides[(n + 1) % 4] && corners[(n + 2) % 4] && sides[(n + 2) % 4])
					{
						wedge = (CellBlock)1;
						rotation = (CellOrientation)((n + 2) % 4);

						cell.solid.setBlock(wedge);
						cell.solid.setOrientation(rotation);
						voxelGrid->setCell(internalCellPos, cell, cellMat);
						return true;
					}
				}
			}

			//type 2: 45 degree corner [must not have a block on top, and be surrounded by 2 sides and the 1 corner between them; or 3 sides and 1 corner between 2 of them (facing towards that corner)]
			for (int n = 0; n < 4; ++n )
			{
				if (sides[n] && corners[(n + 1) % 4] && sides[(n + 1) % 4] && (adjacentSides == 2 || (adjacentSides == 3 && (corners[(n + 3) % 4] || (sides[(n + 2) % 4] && corners[(n + 2) % 4]) || (sides[(n + 3) % 4] && corners[n])))))
				{
					wedge = (CellBlock)2;
					rotation = (CellOrientation)((n + 2) % 4);
					cell.solid.setBlock(wedge);
					cell.solid.setOrientation(rotation);
					voxelGrid->setCell(internalCellPos, cell, cellMat);
					return true;
				}
			}
		}

		//type 3: 45 degree inverse corner [surrounded by three sides or 4 sides and 3 corners, with nothing above or else a block on top surrounded on 2 sides and the corner between them]
		if (adjacentSides == 3 && surroundings[1][1][1])
		{
			if (adjacentCorners > 1)
			{
				for ( int n = 0; n < 4; ++n )
				{
					if ((!corners[n] || !cornersAbove[n]) && (!sides[(n - 1) % 4] || !sides[n]) && (!sidesAbove[n] && sidesAbove[(n + 1) % 4] && sidesAbove[(n + 2) % 4] && !sidesAbove[(n + 3) % 4]))
					{
						wedge = (CellBlock)3;
						rotation = (CellOrientation)((n + 3) % 4);
						cell.solid.setBlock(wedge);
						cell.solid.setOrientation(rotation);
						voxelGrid->setCell(internalCellPos, cell, cellMat);
						return true;
					}
				}
			}
		}
		else if ( adjacentSides == 4 && adjacentCorners == 3 )
		{
			for ( int n = 0; n < 4; ++n )
			{
				if (!corners[n] && (!surroundings[1][1][1] || (!sidesAbove[n] && sidesAbove[(n + 1) % 4] && cornersAbove[(n + 2) % 4] && sidesAbove[(n + 2) % 4] && !sidesAbove[(n + 3) % 4])))
				{
					wedge = (CellBlock)3;
					rotation = (CellOrientation)((n + 3) % 4);
					cell.solid.setBlock(wedge);
					cell.solid.setOrientation(rotation);
					voxelGrid->setCell(internalCellPos, cell, cellMat);
					return true;
				}
			}
		}
		
		//type 4: half a cube, as if it were cut diagonally from front to back [surrounded by 2 sides]
		if (adjacentSides == 2 && adjacentCorners < 4)
		{
			for ( int n = 0; n < 4; ++n )
			{
				if ( !sides[n] && !sides[(n + 1) % 4] && (!surroundings[1][1][1] || (!sidesAbove[n] && !sidesAbove[(n + 1) % 4] && sidesAbove[(n + 2) % 4] && sidesAbove[(n + 3) % 4])))
				{
					wedge = (CellBlock)4;
					rotation = (CellOrientation)n;
					cell.solid.setBlock(wedge);
					cell.solid.setOrientation(rotation);
					voxelGrid->setCell(internalCellPos, cell, cellMat);
					return true;
				}
			}
		}

		// should be a regular block
		wedge = (CellBlock)0;
		rotation = (CellOrientation)0;
		cell.solid.setBlock(wedge);
		cell.solid.setOrientation(rotation);
		voxelGrid->setCell(internalCellPos, cell, cellMat);
		return false;
	}
	
	// was empty
	return false;
}

void MegaClusterInstance::autoWedgeCellsScript(Region3int16 region)
{
	validateApi();

	if (smoothGrid)
		return;

    std::vector<SpatialRegion::Id> chunks = voxelGrid->getNonEmptyChunksInRegion(region);

    for (size_t i = 0; i < chunks.size(); ++i)
    {
        Region3int16 regionExtents = SpatialRegion::inclusiveVoxelExtentsOfRegion(chunks[i]);
        Vector3int16 minPos = regionExtents.getMinPos().max(region.getMinPos());
        Vector3int16 maxPos = regionExtents.getMaxPos().min(region.getMaxPos());

        for (int y = minPos.y; y <= maxPos.y; ++y)
            for (int z = minPos.z; z <= maxPos.z; ++z)
                for (int x = minPos.x; x <= maxPos.x; ++x)
                    autoWedgeCellScript(x, y, z);
    }
}

void MegaClusterInstance::autoWedgeCellsInternal(Region3int16 region)
{
	Vector3int16 minPos = region.getMinPos();
	Vector3int16 maxPos = region.getMaxPos();
	autoWedgeCellsScript(Region3int16(minPos, maxPos));
}

Surface MegaClusterInstance::getSurface(const RbxRay& gridRay, int& surfaceId)
{
    Geometry* geometry = getPrimitive(this)->getGeometry();

	Vector3 notUsed1;
	CoordinateFrame notUsed2;
	if (geometry->hitTestTerrain(gridRay, notUsed1, surfaceId, notUsed2))
		return Surface(this, NormalId(surfaceId));
	else
		return Surface();
}

template<class Stream> void writeCountValue(Stream& s, unsigned count)
{
	RBXASSERT(count <= 0xffff);

	if(count < 255)
	{
		s << ((unsigned char)count);
	}
	else
	{
		s << (unsigned char)255;
		s << (unsigned char)(count >> 8);
		s << (unsigned char)(count & 0xff);
	}
}

template<class Stream> unsigned readCountValue(Stream& s)
{
	unsigned char count;
	s >> count;

	if(count < 255)
	{
		return count;
	}
	else
	{
		unsigned char hi, lo;
		s >> hi;
		s >> lo;
		return (hi << 8) | lo;
	}
}

template <class Stream> void writeInt16(Stream& s, short value)
{
    unsigned char v0 = value;
    unsigned char v1 = value >> 8;

    s << v0;
    s << v1;
}

template <class Stream> short readInt16(Stream& s)
{
    unsigned char v0, v1;
    s >> v0;
    s >> v1;

    return v0 | (v1 << 8);
}

template <class Stream> void MegaClusterInstance::decodeChunkDataFromStreamV1_Deprecated(const Vector3int16& chunkPos, Stream& encodedData)
{
	Vector3int16 cellInChunkIndex = Vector3int16( 0, 0, 0 );
	while( cellInChunkIndex.y < Voxel::kY_CHUNK_SIZE )
	{
		unsigned char value;
		unsigned count;

		encodedData >> value;

		count = readCountValue(encodedData);

		RBXASSERT(count > 0);

		for( unsigned i = 0; i < count; ++i )
		{
			Vector3int16 cellPos = 
				Vector3int16( chunkPos.x * Voxel::kXZ_CHUNK_SIZE, 
				chunkPos.y * Voxel::kY_CHUNK_SIZE, 
				chunkPos.z * Voxel::kXZ_CHUNK_SIZE );
			cellPos += cellInChunkIndex;

			setCellInternalV1_Deprecated( cellPos, value );
			incrementCellInChunkIndex( cellInChunkIndex );

			// validate we're not trying to process into other chunks
			if (cellInChunkIndex.y >= Voxel::kY_CHUNK_SIZE)
				return;
		}
	}
}

template <class Stream> void MegaClusterInstance::decodeChunkDataFromStream(Voxel::Grid& grid, const Vector3int16& chunkPos, Stream& encodedData)
{
	FASTLOG3(FLog::MegaClusterDecodeStream,
		"Decode chunk pos (%d,%d,%d)", chunkPos.x, chunkPos.y, chunkPos.z);
		
	std::vector<unsigned char> cells(Voxel::kXZ_CHUNK_SIZE * Voxel::kXZ_CHUNK_SIZE * Voxel::kY_CHUNK_SIZE);
	std::vector<unsigned char> materials(Voxel::kXZ_CHUNK_SIZE * Voxel::kXZ_CHUNK_SIZE * Voxel::kY_CHUNK_SIZE);
	
	// Read cell data
	unsigned int totalCells = 0;
	
	while (totalCells < cells.size())
	{
		unsigned char value;
		unsigned count;

		encodedData >> value;

		count = std::min(readCountValue(encodedData), static_cast<unsigned>(cells.size() - totalCells));
		RBXASSERT(count > 0);
		
		memset(&cells[totalCells], value, count);
		
		totalCells += count;
	}

	// Read material data
	unsigned int totalMaterials = 0;
	
	while (totalMaterials < materials.size())
	{
		unsigned char value;
		unsigned count;

		encodedData >> value;

		count = std::min(readCountValue(encodedData), static_cast<unsigned>(materials.size() - totalMaterials));
		RBXASSERT(count > 0);
		
		memset(&materials[totalMaterials], value, count);
		
		totalMaterials += count;
	}
	
	// Set all cells
	Region3int16 region = SpatialRegion::inclusiveVoxelExtentsOfRegion(SpatialRegion::Id(chunkPos));
	Vector3int16 startLocation = region.getMinPos();
	unsigned int offset = 0;
	
	bool chunkWasEmpty =
		grid.getRegion(region.getMinPos(), region.getMaxPos()).isGuaranteedAllEmpty();
	
	for (int y = 0; y < Voxel::kY_CHUNK_SIZE; ++y)
	{
		for (int z = 0; z < Voxel::kXZ_CHUNK_SIZE; ++z)
		{
			for (int x = 0; x < Voxel::kXZ_CHUNK_SIZE; ++x)
			{
				Cell cell = Voxel::Cell::readUnsignedCharFromFile(cells[offset]);
				
				// We only need to set non-empty cells if the chunk was empty before we started
				if (!(chunkWasEmpty && cell.isEmpty()))
					grid.setCell(startLocation + Vector3int16(x, y, z), cell, static_cast<Voxel::CellMaterial>(materials[offset]));
				
				offset++;
			}
		}
	}
}

template<class Stream> void MegaClusterInstance::encodeChunkDataIntoStream( const Voxel::Grid::Region& chunk, Stream& s )
{
	if (chunk.isGuaranteedAllEmpty())
	{
		s << Voxel::Cell::convertToUnsignedCharForFile(Voxel::Constants::kUniqueEmptyCellRepresentation);
		writeCountValue(s, Voxel::kXZ_CHUNK_SIZE * Voxel::kY_CHUNK_SIZE * Voxel::kXZ_CHUNK_SIZE);
		
		s << static_cast<unsigned char>(Voxel::CELL_MATERIAL_Water);
		writeCountValue(s, Voxel::kXZ_CHUNK_SIZE * Voxel::kY_CHUNK_SIZE * Voxel::kXZ_CHUNK_SIZE);
		
		return;
	}
	
	unsigned char value = 0;
	int count = 0;

	for (Voxel::Grid::Region::iterator it = chunk.begin(); it != chunk.end(); ++it)
	{
		unsigned char temp = Voxel::Cell::convertToUnsignedCharForFile(it.getCellAtCurrentLocation());

		if( value != temp && count != 0 )
		{
			s << value;
			writeCountValue(s, count);
			count = 0;
		}

		value = temp;
		count++;
	}

	// encode the final value/count pair
	RBXASSERT( count > 0 );
	
	s << value;
	writeCountValue(s, count);

	// Next try to encode new material byte
	value = 0;
	count = 0;

	for (Voxel::Grid::Region::iterator it = chunk.begin(); it != chunk.end(); ++it)
	{
		Voxel::CellMaterial temp = it.getMaterialAtCurrentLocation();

		// 0xffff should fit entire chunk if necessary
		if( value != temp && count != 0 )
		{
			s << value;
			writeCountValue(s, count);
			count = 0;
		}

		value = temp;
		count++;
	}

	// encode the final value/count pair
	RBXASSERT(count > 0);

	s << value;
	writeCountValue(s, count);
}

void MegaClusterInstance::incrementCellInChunkIndex( Vector3int16& index ) const
{
	index.x++;
	if( index.x >= kXZ_CHUNK_SIZE ) 
	{
		index.x = 0;
		index.z++;
	}

	if( index.z >= kXZ_CHUNK_SIZE )
	{
		index.z = 0;
		index.y++;
	}
}

void MegaClusterInstance::setPackagedClusterGridV1(const std::string& clusterData)
{
	if (clusterData.empty())
        return;

	initializeGridMega();

	// iterate over all chunks and decode them
	Vector3int16 numChunks = kLegacyChunkCount;
    Vector3int16 chunkPosOffset = kLegacyChunkOffset;

	StringReadBuffer stream(clusterData);

	for (int i = 0; i < numChunks.x; ++i)
		for (int j = 0; j < numChunks.y; ++j)
			for (int k = 0; k < numChunks.z; ++k)
			{
				decodeChunkDataFromStreamV1_Deprecated(Vector3int16(i, j, k) - chunkPosOffset, stream);
				if (stream.eof())
					return;
			}
}

void MegaClusterInstance::setPackagedClusterGridV2(const std::string& clusterData)
{
	if (clusterData.empty())
        return;

	initializeGridMega();

	// iterate over all chunks and decode them
	Vector3int16 numChunks = kLegacyChunkCount;
    Vector3int16 chunkPosOffset = kLegacyChunkOffset;

	StringReadBuffer stream(clusterData);

	for (int i = 0; i < numChunks.x; ++i)
		for (int j = 0; j < numChunks.y; ++j)
			for (int k = 0; k < numChunks.z; ++k)
			{
				decodeChunkDataFromStream(*voxelGrid, Vector3int16(i, j, k) - chunkPosOffset, stream);
					
                if (stream.eof())
                    return;
			}
}

void MegaClusterInstance::setPackagedClusterGridV3(const BinaryString& clusterData)
{
	if (clusterData.value().empty())
        return;

	initializeGridMega();

    deserializeGridV3(*voxelGrid, clusterData);
}

BinaryString MegaClusterInstance::getPackagedClusteredGridV3() const
{
    if (voxelGrid)
		return serializeGridV3(*voxelGrid);
    else
        return BinaryString();
}

void MegaClusterInstance::setPackagedSmoothGrid(const BinaryString& clusterData)
{
	if (clusterData.value().empty())
        return;

	initializeGridSmooth();

	smoothGrid->deserialize(clusterData.value());
}

BinaryString MegaClusterInstance::getPackagedSmoothGrid() const
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

BinaryString MegaClusterInstance::serializeGridV3(const Voxel::Grid& grid)
{
    if (grid.getNonEmptyCellCount() == 0)
        return BinaryString();
    
	StringWriteBuffer encodedData;

	// iterate over chunks
    std::vector<SpatialRegion::Id> chunks = grid.getNonEmptyChunks();

    for (size_t i = 0; i < chunks.size(); ++i)
    {
        SpatialRegion::Id id = chunks[i];

        Region3int16 region = SpatialRegion::inclusiveVoxelExtentsOfRegion(id);
        Voxel::Grid::Region chunk = grid.getRegion(region.getMinPos(), region.getMaxPos());

        writeInt16(encodedData, id.value().x);
        writeInt16(encodedData, id.value().y);
        writeInt16(encodedData, id.value().z);

        encodeChunkDataIntoStream(chunk, encodedData);
	}
	
	return BinaryString(encodedData.str());
}

void MegaClusterInstance::deserializeGridV3(Voxel::Grid& grid, const BinaryString& data)
{
	if (data.value().empty())
        return;

	StringReadBuffer stream(data.value());

    do
    {
        short chunkX = readInt16(stream);
        short chunkY = readInt16(stream);
        short chunkZ = readInt16(stream);

        decodeChunkDataFromStream(grid, Vector3int16(chunkX, chunkY, chunkZ), stream);
    }
    while (!stream.eof());
}

void MegaClusterInstance::setCellInternalV1_Deprecated(
		const Vector3int16& pos, const unsigned char& inputCell) {
	Cell cell;
	CellMaterial material = getCellMaterial_Deprecated(inputCell);
	if (material == CELL_MATERIAL_Deprecated_Empty) {
		cell = Constants::kUniqueEmptyCellRepresentation;
		material = CELL_MATERIAL_Water;
	} else {
		Cell tmp = Cell::readUnsignedCharFromFile(inputCell);
		cell.solid.setBlock(tmp.solid.getBlock());
		cell.solid.setOrientation(tmp.solid.getOrientation());
	}

	voxelGrid->setCell(pos, cell, material);
}

shared_ptr<const Reflection::Tuple> MegaClusterInstance::getWaterCellScript(int x, int y, int z)
{
	validateApi();

	if (smoothGrid)
	{
		Voxel2::Cell cell = smoothGrid->getCell(x, y, z);

		shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(3));

        result->values[0] = (cell.getMaterial() == Voxel2::Cell::Material_Water);
		result->values[1] = WATER_CELL_FORCE_None;
		result->values[2] = WATER_CELL_DIRECTION_NegX;

        return result;
	}
	else
	{
		Cell waterCell = voxelGrid->getWaterCell(Vector3int16(x,y,z));
		shared_ptr<Reflection::Tuple> result(new Reflection::Tuple(3));

		if ( waterCell.isEmpty() ) {
			result->values[0] = false;
			result->values[1] = (WaterCellForce)0;
			result->values[2] = (WaterCellDirection)0;
		} else {
			result->values[0] = true;
			result->values[1] = waterCell.water.getForce();
			result->values[2] = waterCell.water.getDirection();
		}

		return result;
	}
}

void MegaClusterInstance::setWaterCellScript( int x, int y, int z, WaterCellForce force, WaterCellDirection direction )
{
	validateApi();

	if (smoothGrid)
	{
		setCellsScript(Region3int16(Vector3int16(x, y, z), Vector3int16(x, y, z)), CELL_MATERIAL_Water, CELL_BLOCK_Solid, CELL_ORIENTATION_NegZ);
	}
	else
	{
		Vector3int16 pos = Vector3int16(x,y,z);

		Cell cell;
		cell.solid.setBlock(CELL_BLOCK_Empty);
		cell.water.setForceAndDirection(force, direction);
		voxelGrid->setCell(pos, cell, CELL_MATERIAL_Water);
	}
}

void MegaClusterInstance::setCellScript(int x, int y, int z, CellMaterial material, CellBlock block, CellOrientation orientation )
{
	validateApi();

	if (smoothGrid)
	{
		setCellsScript(Region3int16(Vector3int16(x, y, z), Vector3int16(x, y, z)), material, block, orientation);
	}
	else
	{
		Vector3int16 pos = Vector3int16(x,y,z);

		if (material == CELL_MATERIAL_Water) {
			setWaterCellScript(x,y,z, (WaterCellForce)0, (WaterCellDirection)0);
			return;
		}

		Cell cell;

		if (material == CELL_MATERIAL_Deprecated_Empty || block == CELL_BLOCK_Empty) {
			cell = Constants::kUniqueEmptyCellRepresentation;
			material = CELL_MATERIAL_Water;
		} else {
			cell.solid.setBlock(block);
			cell.solid.setOrientation(orientation);
		}

		voxelGrid->setCell(pos, cell, material);
	}
}

bool MovePosition(Vector3int16& pos, FaceDirection dir)
{
	switch(dir)
	{
	case PlusX:
		pos.x += 1;
		return true;
	case PlusZ:
		pos.z += 1;
		return true;
	case MinusX:
		if(pos.x == 0)
			return false;
		pos.x -= 1;
		return true;
	case MinusZ:
		if(pos.z == 0)
			return false;
		pos.z -= 1;
		return true;
	case PlusY:
		pos.y += 1;
		return true;
	case MinusY:
		if(pos.y == 0)
			return false;
		pos.y -= 1;		
		return true;
    default:
        break;
	}

	return false;
}

Vector3 MegaClusterInstance::worldToCellPreferSolidScript(Vector3 worldPos )
{
	Vector3int16 result = worldToCellWithPreference( worldPos, true );
	return Vector3( result.x, result.y, result.z );
}

Vector3 MegaClusterInstance::worldToCellPreferEmptyScript(Vector3 worldPos )
{
	Vector3int16 result = worldToCellWithPreference( worldPos, false );
	return Vector3( result.x, result.y, result.z );
}

Vector3 MegaClusterInstance::worldToCellScript(Vector3 worldPos )
{
    Vector3int16 result = Voxel::worldToCell_floor(worldPos);
	return Vector3( result.x, result.y, result.z );
}

bool MegaClusterInstance::isCellIdealForPreference(
		const Vector3int16& location, const Cell cell,
		bool preferSolid) const {
	Cell tmpCell = voxelGrid->getCell(location);
	return
		((preferSolid && cell.solid.getBlock() != CELL_BLOCK_Empty) ||
		(!preferSolid && tmpCell.isEmpty()));
}

bool MegaClusterInstance::isCellAcceptablePreferenceWaterAlternative(
		const Vector3int16& location, const Cell cell,
		bool preferSolid) const {
	Cell waterCell = voxelGrid->getWaterCell(location);
	bool hasWater = !waterCell.isEmpty();
	return
		((preferSolid && hasWater) ||
		(!preferSolid && hasWater && cell.solid.getBlock() == CELL_BLOCK_Empty));
}

struct WorldToCellCandidate
{
	Vector3int16 pos;
	float distance;

	WorldToCellCandidate()
	{
		distance = 0;
	}

	WorldToCellCandidate(const Vector3& worldPos, int ox, int oy, int oz)
	{
		Vector3int16 center = Voxel::worldToCell_floor(worldPos);

		pos = center + Vector3int16(ox, oy, oz);
		distance = (worldPos - Voxel::cellToWorld_center(pos)).squaredLength();
	}

	bool operator<(const WorldToCellCandidate& other) const
	{
		return distance < other.distance;
	}

	static bool isValid(const Voxel2::Cell& cell, bool preferSolid)
	{
		return (cell.getMaterial() != Voxel2::Cell::Material_Air) == preferSolid;
	}
};

Vector3int16 MegaClusterInstance::worldToCellWithPreference(const Vector3& worldPos, bool preferSolid)
{
	validateApi();

	if (smoothGrid)
	{
		Vector3int16 center = Voxel::worldToCell_floor(worldPos);

		if (WorldToCellCandidate::isValid(smoothGrid->getCell(center.x, center.y, center.z), preferSolid))
			return center;

		std::vector<WorldToCellCandidate> candidates;

		candidates.reserve(27);

		for (int x = -1; x <= 1; ++x)
			for (int y = -1; y <= 1; ++y)
				for (int z = -1; z <= 1; ++z)
				{
					candidates.push_back(WorldToCellCandidate(worldPos, x, y, z));
				}

		std::sort(candidates.begin(), candidates.end());

		for (auto& c: candidates)
			if (WorldToCellCandidate::isValid(smoothGrid->getCell(c.pos.x, c.pos.y, c.pos.z), preferSolid))
				return c.pos;

		return center;
	}
	else
	{
		Vector3int16 firstApproximation = Voxel::worldToCell_floor(worldPos);
		Cell tmpCell = voxelGrid->getCell(firstApproximation);

		if (isCellIdealForPreference(firstApproximation, tmpCell, preferSolid)) {
			return firstApproximation;
		}

		bool foundAcceptableWaterCell = false;
		Vector3int16 acceptableWaterCell;

		const float searchEpsilon = 0.00048828125f; // 1 / 2048
		for (int y = -1; y < 2; y += 2) {
			for (int z = -1; z < 2; z += 2) {
				for (int x = -1; x < 2; x += 2) {
					Vector3int16 test = Voxel::worldToCell_floor(worldPos +
						Vector3(searchEpsilon * x,
							searchEpsilon * y,
							searchEpsilon * z));
					tmpCell = voxelGrid->getCell(test);
					if (isCellIdealForPreference(test, tmpCell, preferSolid)) {
						return test;
					}
					if (isCellAcceptablePreferenceWaterAlternative(test, tmpCell, preferSolid)) {
						foundAcceptableWaterCell = true;
						acceptableWaterCell = test;
					}
				}
			}
		}

		if (foundAcceptableWaterCell) {
			return acceptableWaterCell;
		}

		Vector3int16 bestCell = firstApproximation;
		bool foundTotalMatch = false;
		float bestDistance = 10;
		foundAcceptableWaterCell = false;
		float bestWaterDistance = 10;

		// Ok, now we need to search for empty around it
		for(unsigned i = 0; i < Invalid; i++)
		{
			Vector3int16 npos = firstApproximation;
			if(!MovePosition(npos, (FaceDirection)i))
				continue;

			float distance = (cellToWorldExtents(npos).center() - worldPos).squaredLength();
			tmpCell = voxelGrid->getCell(npos);
			if (isCellIdealForPreference(npos, tmpCell, preferSolid) &&
					distance < bestDistance) {
				foundTotalMatch = true;
				bestCell = npos;
				bestDistance = distance;
			}
			if (isCellAcceptablePreferenceWaterAlternative(npos, tmpCell,
					preferSolid) &&
					distance < bestWaterDistance) {
				foundAcceptableWaterCell = true;
				acceptableWaterCell = npos;
				bestWaterDistance = distance;
			}
		}

		if (!foundTotalMatch && foundAcceptableWaterCell) {
			return acceptableWaterCell;
		} else {
			return bestCell;
		}
	}
}

inline Vector3int16 floorToInt(const Vector3& vec)
{
	return Vector3int16((G3D::int16)floor(vec.x), (G3D::int16)floor(vec.y), (G3D::int16)floor(vec.z));
}

Vector3 MegaClusterInstance::cellCornerToWorld( const Vector3int16& v ) const
{
    return Voxel::cellToWorld_smallestCorner(v);
}

Vector3 MegaClusterInstance::cellCenterToWorldScript( int x, int y, int z )
{
	return cellCornerToWorldScript( x, y, z ) +  Vector3(2,2,2);
}

void MegaClusterInstance::clear()
{
	validateApi();

	if (smoothGrid)
	{
		std::vector<Voxel2::Region> regions = smoothGrid->getNonEmptyRegions();

		for (size_t i = 0; i < regions.size(); ++i)
		{
			Vector3int32 size = regions[i].size();

			Voxel2::Box box(size.x, size.y, size.z);

			smoothGrid->write(regions[i], box);
		}
	}
	else
	{
		std::vector<SpatialRegion::Id> chunks = voxelGrid->getNonEmptyChunks();

		for (size_t i = 0; i < chunks.size(); ++i)
		{
			SpatialRegion::Id id = chunks[i];
			Region3int16 region = SpatialRegion::inclusiveVoxelExtentsOfRegion(id);

			Vector3int16 min = region.getMinPos();
			Vector3int16 max = region.getMaxPos();

			for (int y = min.y; y <= max.y; ++y)
				for (int z = min.z; z <= max.z; ++z)
					for (int x = min.x; x <= max.x; ++x)
						voxelGrid->setCell(Vector3int16(x,y,z), Constants::kUniqueEmptyCellRepresentation, CELL_MATERIAL_Water);
		}
	}
}

int MegaClusterInstance::countCellsScript()
{
	return getNonEmptyCellCount();
}

const Region3int16 MegaClusterInstance::getMaxExtents() const
{
    return Voxel::getTerrainExtentsInCells();
}

void MegaClusterInstance::render3dSelect(Adorn* adorn, SelectState selectState)
{
	// do nothing. we don't want to render the selection box
}

Extents MegaClusterInstance::cellToWorldExtents( const Vector3int16& cellpos )
{
    return Extents::fromCenterRadius(Voxel::cellToWorld_center(cellpos), kHALF_CELL);
}

Extents MegaClusterInstance::computeExtentsWorld() const
{
    Extents result;

	if (smoothGrid)
	{
		std::vector<Voxel2::Region> regions = smoothGrid->getNonEmptyRegions();

        for (size_t i = 0; i < regions.size(); ++i)
        {
			result.expandToContain(Voxel::cellSpaceToWorldSpace(regions[i].begin().toVector3()));
			result.expandToContain(Voxel::cellSpaceToWorldSpace(regions[i].end().toVector3()));
        }
	}
	else
	{
        std::vector<SpatialRegion::Id> chunks = voxelGrid->getNonEmptyChunks();

        for (size_t i = 0; i < chunks.size(); ++i)
        {
            result.expandToContain(SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(chunks[i]).toVector3());
            result.expandToContain(SpatialRegion::largestCornerOfRegionInGlobalCoordStuds(chunks[i]).toVector3());
        }
	}

    return result;
}

shared_ptr<Instance> MegaClusterInstance::copyRegion(Region3int16 region)
{
	validateApi();

	if (smoothGrid)
		return Creatable<Instance>::create<TerrainRegion>(smoothGrid.get(), region);
	else
		return Creatable<Instance>::create<TerrainRegion>(voxelGrid.get(), region);
}

void MegaClusterInstance::pasteRegion(shared_ptr<Instance> region, Vector3int16 corner, bool pasteEmptyCells)
{
	validateApi();

    if (TerrainRegion* terrainRegion = fastDynamicCast<TerrainRegion>(region.get()))
    {
		if (smoothGrid)
			terrainRegion->copyTo(*smoothGrid, corner, pasteEmptyCells);
		else
			terrainRegion->copyTo(*voxelGrid, corner, pasteEmptyCells);
    }
    else
        throw std::runtime_error("region has to be a TerrainRegion");
}

void MegaClusterInstance::reloadMaterialTable()
{
	if (!smoothGrid)
        return;

	materialTable.reset(new Voxel2::MaterialTable(ContentProvider::findAsset(kSmoothTerrainMaterials), Voxel2::Cell::Material_Max + 1));

	Primitive* myPrim = getPartPrimitive();

	DenseHashSet<Primitive*> primitives(NULL);
	DenseHashSet<Contact*> contacts(NULL);

	for (int i = 0; i < myPrim->getNumContacts(); ++i)
	{
		primitives.insert(myPrim->getContactOther(i));
        contacts.insert(myPrim->getContact(i));
	}

	for (DenseHashSet<Contact*>::const_iterator it = contacts.begin(); it != contacts.end(); ++it)
		myPrim->getWorld()->destroyContact(*it);

	static_cast<SmoothClusterGeometry*>(myPrim->getGeometry())->updateAllChunks();

	for (DenseHashSet<Primitive*>::const_iterator it = primitives.begin(); it != primitives.end(); ++it)
		myPrim->getWorld()->getContactManager()->checkTerrainContact(*it);
}

Voxel::Grid* MegaClusterInstance::getVoxelGrid()
{
    RBXASSERT(voxelGrid);

	return voxelGrid.get();
}

Voxel2::Grid* MegaClusterInstance::getSmoothGrid()
{
    RBXASSERT(smoothGrid);

	return smoothGrid.get();
}

bool MegaClusterInstance::isSmooth() const
{
    return !!smoothGrid;
}

bool MegaClusterInstance::isInitialized() const
{
    return voxelGrid || smoothGrid;
}

void MegaClusterInstance::setSmoothReplicate(int value)
{
    if (smoothReplicate != value)
	{
		smoothReplicate = value;
		raisePropertyChanged(desc_SmoothReplicate);
	}
}

unsigned int MegaClusterInstance::getNonEmptyCellCount() const
{
	if (smoothGrid)
		return smoothGrid->getNonEmptyCellCountApprox();

    if (voxelGrid)
        return voxelGrid->getNonEmptyCellCount();

    return 0;
}

void MegaClusterInstance::convertToSmooth()
{
	validateApi();

    if (smoothGrid)
		throw std::runtime_error("Terrain is already smooth!");

    if (Network::Players::getGameMode(this) != Network::EDIT ||
		Network::Players::isCloudEdit(this))
	{
		throw std::runtime_error("Terrain can only be converted in edit mode");
	}

    // Unparent to get ourselves disconnected from CHS/etc
    Instance* parent = getParent();

    setLockedParent(NULL);

	// Stash old grid somewhere and reinitialize as smooth grid
	scoped_ptr<Voxel::Grid> oldGrid;
	voxelGrid.swap(oldGrid);

	initializeGridSmooth();

    // Transfer data from voxel grid into smooth grid
	Voxel2::Conversion::convertToSmooth(*oldGrid, *smoothGrid);

	// Reparent ourselves back
	setLockedParent(parent);

	// Set a waypoint so that further editing operations are undone up until this point
    ChangeHistoryService* chs = ServiceProvider::find<ChangeHistoryService>(parent);

	if (chs)
		chs->resetBaseWaypoint();

	raisePropertyChanged(prop_IsSmooth);
}

void MegaClusterInstance::initializeGridMega()
{
    RBXASSERT(!getParent());
    RBXASSERT(!voxelGrid && !smoothGrid);

	voxelGrid.reset(new Voxel::Grid());
	getPartPrimitive()->setGeometryType(Geometry::GEOMETRY_MEGACLUSTER);

	smoothReplicate = 0;
}

void MegaClusterInstance::initializeGridSmooth()
{
    RBXASSERT(!getParent());
    RBXASSERT(!voxelGrid && !smoothGrid);

	smoothGrid.reset(new Voxel2::Grid());
	getPartPrimitive()->setGeometryType(Geometry::GEOMETRY_SMOOTHCLUSTER);

	smoothReplicate = 1;
}

static Voxel2::Region getRegionAtResolution(const Region3& region, float resolution)
{
	if (resolution != 4)
		throw std::runtime_error("Resolution has to be 4");

	Vector3 min = region.minPos() / resolution;
	Vector3 max = region.maxPos() / resolution;

	Vector3int32 imin(min.x, min.y, min.z);
	Vector3int32 imax(max.x, max.y, max.z);

	if (imin.toVector3() != min || imax.toVector3() != max)
		throw std::runtime_error("Region has to be aligned to the grid (use Region3:ExpandToGrid)");

	if (imin.x >= imax.x || imin.y >= imax.y || imin.z >= imax.z)
		throw std::runtime_error("Region cannot be empty");

	Voxel2::Region result(imin, imax);

	// One cell is represented as two Lua values, resulting in ~32b/cell overhead
	// 4M voxels would take 128Mb
    if (result.getChunkCount(0) > FInt::SmoothTerrainMaxLuaRegion)
		throw std::runtime_error("Region is too large");

	return result;
}

static Voxel2::Region getRegionFromExtents(const Vector3& min, const Vector3& max)
{
	Voxel2::Region result = Voxel2::Region::fromExtents(min, max);

	if (result.empty())
		throw std::runtime_error("Extents cannot be empty");

	// 64M voxels would take 128Mb
    if (result.getChunkCount(0) > FInt::SmoothTerrainMaxCppRegion)
		throw std::runtime_error("Extents are too large");

	return result;
}

template <typename K, typename V, size_t N> struct FixedSizeCache
{
	K keys[N];
	V values[N];
	size_t next;

	FixedSizeCache(K invalid): next(0)
	{
		for (size_t i = 0; i < N; ++i)
			keys[i] = invalid;
	}

	std::pair<bool, size_t> insert(K key)
	{
		for (size_t i = 0; i < N; ++i)
			if (keys[i] == key)
				return std::make_pair(false, i);

		size_t index = next;

        keys[index] = key;

		next = (next + 1) % N;

		return std::make_pair(true, index);
	}

	size_t size() const
	{
		return N;
	}
};

int MegaClusterInstance::readVoxels(lua_State* L)
{
	validateApiSmooth();

	Lua::LuaArguments args(L, 1);

	Region3 region;
	if (!args.getRegion3(1, region))
        throw std::runtime_error("Argument 1 missing or nil");

	double resolution;
	if (!args.getDouble(2, resolution))
        throw std::runtime_error("Argument 2 missing or nil");

	Voxel2::Region rr = getRegionAtResolution(region, resolution);
	const Voxel2::Box box = smoothGrid->read(rr);

    const Reflection::EnumDesc<PartMaterial>& matDesc = Reflection::EnumDesc<PartMaterial>::singleton();

    FixedSizeCache<unsigned char, int, 8> materialCache(Voxel2::Cell::Material_Max + 1);

	int materialCacheStack = lua_gettop(L);

	for (size_t i = 0; i < materialCache.size(); ++i)
		lua_pushnil(L);

	lua_createtable(L, box.getSizeX(), 0); // mat
	lua_createtable(L, box.getSizeX(), 0); // occ

	for (int x = 0; x < box.getSizeX(); ++x)
	{
		lua_createtable(L, box.getSizeY(), 0); // matX
		lua_createtable(L, box.getSizeY(), 0); // occX

		for (int y = 0; y < box.getSizeY(); ++y)
		{
			lua_createtable(L, box.getSizeZ(), 0); // matY
			lua_createtable(L, box.getSizeZ(), 0); // occY

			for (int z = 0; z < box.getSizeZ(); ++z)
			{
				const Voxel2::Cell& cell = box.get(x, y, z);

				std::pair<bool, size_t> materialCacheIndex = materialCache.insert(cell.getMaterial());

                // make sure material cache is up to date
				if (materialCacheIndex.first)
				{
					PartMaterial material = Voxel2::Conversion::getMaterialFromVoxelMaterial(cell.getMaterial());

					Lua::EnumItem::push(L, matDesc.convertToItem(material));
					lua_replace(L, materialCacheStack + materialCacheIndex.second);
				}

                // push material
				lua_pushvalue(L, materialCacheStack + materialCacheIndex.second);

                // push occupancy
				if (cell.getMaterial() == Voxel2::Cell::Material_Air)
				{
					lua_pushnumber(L, 0);
				}
				else
				{
					float occScale = 1.f / (Voxel2::Cell::Occupancy_Max + 1);
					lua_pushnumber(L, (cell.getOccupancy() + 1) * occScale);
				}

				lua_rawseti(L, -3, z + 1);
				lua_rawseti(L, -3, z + 1);
			}

			lua_rawseti(L, -3, y + 1);
			lua_rawseti(L, -3, y + 1);
		}

		lua_rawseti(L, -3, x + 1);
		lua_rawseti(L, -3, x + 1);
	}

	lua_pushstring(L, "Size");
	Lua::Vector3Bridge::pushVector3(L, box.getSize().toVector3());

	lua_rawset(L, -4);

	lua_pushstring(L, "Size");
	Lua::Vector3Bridge::pushVector3(L, box.getSize().toVector3());

	lua_rawset(L, -3);

	return 2;
}

static bool isArray(lua_State* L, int index, int size)
{
	return lua_istable(L, index) && lua_objlen(L, index) == size;
}

int MegaClusterInstance::writeVoxels(lua_State* L)
{
	validateApiSmooth();

	Lua::LuaArguments args(L, 1);

	Region3 region;
	if (!args.getRegion3(1, region))
        throw std::runtime_error("Argument 1 missing or nil");

	double resolution;
	if (!args.getDouble(2, resolution))
        throw std::runtime_error("Argument 2 missing or nil");

	if (4 > lua_gettop(L))
        throw std::runtime_error("Argument 3 missing or nil");

	if (5 > lua_gettop(L))
        throw std::runtime_error("Argument 4 missing or nil");

	Voxel2::Region rr = getRegionAtResolution(region, resolution);
	Voxel2::Box box(rr.size().x, rr.size().y, rr.size().z);

    const Reflection::EnumDesc<PartMaterial>& matDesc = Reflection::EnumDesc<PartMaterial>::singleton();

	FixedSizeCache<void*, unsigned char, 8> materialCache(NULL);

	if (!isArray(L, 4, box.getSizeX()))
		throw RBX::runtime_error("Bad argument materials to 'WriteVoxels' (%dx%dx%d array expected)", box.getSizeX(), box.getSizeY(), box.getSizeZ());

	if (!isArray(L, 5, box.getSizeX()))
		throw RBX::runtime_error("Bad argument occupancy to 'WriteVoxels' (%dx%dx%d array expected)", box.getSizeX(), box.getSizeY(), box.getSizeZ());

	for (int x = 0; x < box.getSizeX(); ++x)
	{
		lua_rawgeti(L, 4, x + 1); // matX
		lua_rawgeti(L, 5, x + 1); // occX

		if (!isArray(L, -2, box.getSizeY()))
			throw RBX::runtime_error("Bad argument materials[%d] to 'WriteVoxels' (%dx%d array expected)", x + 1, box.getSizeY(), box.getSizeZ());

		if (!isArray(L, -1, box.getSizeY()))
			throw RBX::runtime_error("Bad argument occupancy[%d] to 'WriteVoxels' (%dx%d array expected)", x + 1, box.getSizeY(), box.getSizeZ());

		for (int y = 0; y < box.getSizeY(); ++y)
		{
			lua_rawgeti(L, -2, y + 1); // matY
			lua_rawgeti(L, -2, y + 1); // occY

			if (!isArray(L, -2, box.getSizeZ()))
				throw RBX::runtime_error("Bad argument materials[%d][%d] to 'WriteVoxels' (%d-element array expected)", x + 1, y + 1, box.getSizeZ());

			if (!isArray(L, -1, box.getSizeZ()))
				throw RBX::runtime_error("Bad argument occupancy[%d][%d] to 'WriteVoxels' (%d-element array expected)", x + 1, y + 1, box.getSizeZ());

			for (int z = 0; z < box.getSizeZ(); ++z)
			{
				lua_rawgeti(L, -2, z + 1); // matZ
				lua_rawgeti(L, -2, z + 1); // occZ

				void* mat = lua_touserdata(L, -2);
				if (!mat)
					luaL_error(L, "Bad argument materials[%d][%d][%d] to 'WriteVoxels' (Enum.Material expected, got %s)", x + 1, y + 1, z + 1, luaL_typename(L, -2));

				std::pair<bool, size_t> materialCacheIndex = materialCache.insert(mat);

				if (materialCacheIndex.first)
				{
					const Reflection::EnumDescriptor::Item* matItemDesc = NULL;

					if (!Lua::EnumItem::getItem(L, -2, matItemDesc))
						luaL_error(L, "Bad argument materials[%d][%d][%d] to 'WriteVoxels' (Enum.Material expected, got %s)", x + 1, y + 1, z + 1, luaL_typename(L, -2));

					if (&matItemDesc->owner != &matDesc)
						luaL_error(L, "Bad argument materials[%d][%d][%d] to 'WriteVoxels' (Enum.Material expected, got %s)", x + 1, y + 1, z + 1, matItemDesc->owner.name.c_str());

					unsigned char material = Voxel2::Conversion::getVoxelMaterialFromMaterial(static_cast<PartMaterial>(matItemDesc->value));

					materialCache.values[materialCacheIndex.second] = material;
				}

				unsigned char material = materialCache.values[materialCacheIndex.second];

				if (!lua_isnumber(L, -1))
					luaL_error(L, "Bad argument occupancy[%d][%d][%d] to 'WriteVoxels' (number expected, got %s)", x + 1, y + 1, z + 1, luaL_typename(L, -1));

				float occ = lua_tonumber(L, -1);
                
				if (material == Voxel2::Cell::Material_Air || Math::isNanInf(occ) || occ <= 0)
				{
					box.set(x, y, z, Voxel2::Cell());
				}
				else
				{
					// We round occupancy to nearest integer value, keeping in mind that 1/256.f means 0 in Cell
					float occInt = occ * (Voxel2::Cell::Occupancy_Max + 1) - 0.5f;

					box.set(x, y, z, Voxel2::Cell(material, std::min(std::max(static_cast<int>(occInt), 0), static_cast<int>(Voxel2::Cell::Occupancy_Max))));
				}

				lua_pop(L, 2);
			}

			lua_pop(L, 2);
		}

		lua_pop(L, 2);
	}

	smoothGrid->write(rr, box);

	return 0;
}

void MegaClusterInstance::fillRegion(Region3 region, float resolution, PartMaterial material)
{
	validateApiSmooth();

	Voxel2::Region rr = getRegionAtResolution(region, resolution);
	Vector3int32 rsize = rr.size();

	Voxel2::Box box(rsize.x, rsize.y, rsize.z);

	unsigned char voxelMaterial = Voxel2::Conversion::getVoxelMaterialFromMaterial(material);

	if (voxelMaterial != Voxel2::Cell::Material_Air)
	{
		Voxel2::Cell cell(voxelMaterial, Voxel2::Cell::Occupancy_Max);

		for (int y = 0; y < rsize.y; ++y)
			for (int z = 0; z < rsize.z; ++z)
            {
                Voxel2::Cell* row = box.writeRow(0, y, z);

				for (int x = 0; x < rsize.x; ++x)
					row[x] = cell;
            }
	}

	smoothGrid->write(rr, box);
}

static void fillCell(Voxel2::Cell& cell, unsigned char material, int occ)
{
	if (material == Voxel2::Cell::Material_Air)
	{
		if (cell.getOccupancy() <= occ)
			cell = Voxel2::Cell();
		else
		{
			int aocc = Voxel2::Cell::Occupancy_Max - occ;

			if (cell.getOccupancy() > aocc)
				cell = Voxel2::Cell(cell.getMaterial(), aocc);
		}
	}
	else
	{
		if (cell.getOccupancy() <= occ && occ != 0)
			cell = Voxel2::Cell(material, occ);
	}
}

void MegaClusterInstance::fillBlock(CoordinateFrame cframe, Vector3 size, PartMaterial material)
{
	validateApiSmooth();

	AABox aabb = cframe.AABBtoWorldSpace(AABox(-size / 2, size / 2));

	Voxel2::Region rr = getRegionFromExtents(aabb.low(), aabb.high());
	Vector3int32 rsize = rr.size();

	Voxel2::Box box = smoothGrid->read(rr);

	unsigned char voxelMaterial = Voxel2::Conversion::getVoxelMaterialFromMaterial(material);

	if (voxelMaterial == Voxel2::Cell::Material_Air && box.isEmpty())
		return;

	// Since we only care about the size if it's less than one cell, we clamp this to make the calculations below faster.
	Vector3 sizeCellsClamped = (size / Voxel::kCELL_SIZE).min(Vector3(1.0f, 1.0f, 1.0f));
	Vector3 sizeCellsHalfOffset = size * (0.5f / Voxel::kCELL_SIZE) + Vector3(0.5f, 0.5f, 0.5f);

	Vector3 localCorner = cframe.pointToObjectSpace(Vector3(rr.begin().x + 0.5f, rr.begin().y + 0.5f, rr.begin().z + 0.5f) * Voxel::kCELL_SIZE) / Voxel::kCELL_SIZE;

	Vector3 localAxisX = Vector3(cframe.rotation[0][0], cframe.rotation[0][1], cframe.rotation[0][2]);
	Vector3 localAxisY = Vector3(cframe.rotation[1][0], cframe.rotation[1][1], cframe.rotation[1][2]);
	Vector3 localAxisZ = Vector3(cframe.rotation[2][0], cframe.rotation[2][1], cframe.rotation[2][2]);

	Vector3 localY = localCorner;

	for (int y = 0; y < rsize.y; ++y)
	{
		Vector3 localZ = localY;

		for (int z = 0; z < rsize.z; ++z)
		{
			Vector3 localX = localZ;

            Voxel2::Cell* row = box.writeRow(0, y, z);

			for (int x = 0; x < rsize.x; ++x)
			{
				float distX = sizeCellsHalfOffset.x - fabsf(localX.x);
				float distY = sizeCellsHalfOffset.y - fabsf(localX.y);
				float distZ = sizeCellsHalfOffset.z - fabsf(localX.z);

				// if distance <-0.5, voxel is completely inside the part
				// if distance >+0.5, voxel is completely outside the part
				// note that we include 0.5 offset in the distXYZ calculation
				float factorX = std::max(0.f, std::min(distX, sizeCellsClamped.x));
				float factorY = std::max(0.f, std::min(distY, sizeCellsClamped.y));
				float factorZ = std::max(0.f, std::min(distZ, sizeCellsClamped.z));

                // occupancy is a linear metric, not volumetric
                // it is more correct to use powf(fx*fy*fz, 1/3.f), but it is slower
				float factor = std::min(factorX, std::min(factorY, factorZ));

				int occ = static_cast<int>(factor * Voxel2::Cell::Occupancy_Max + 0.5f);

				fillCell(row[x], voxelMaterial, occ);

				localX += localAxisX;
			}

			localZ += localAxisZ;
		}

		localY += localAxisY;
	}

	smoothGrid->write(rr, box);
}

void MegaClusterInstance::fillBall(Vector3 center, float radius, PartMaterial material)
{
	fillBallInternal(center, radius, material, /* skipWater= */ false);
}

void MegaClusterInstance::fillBallInternal(Vector3 center, float radius, PartMaterial material, bool skipWater)
{
	validateApiSmooth();

    Voxel2::Region rr = getRegionFromExtents(center - Vector3(radius), center + Vector3(radius));
	Vector3int32 rsize = rr.size();

	Voxel2::Box box = smoothGrid->read(rr);

	unsigned char voxelMaterial = Voxel2::Conversion::getVoxelMaterialFromMaterial(material);

	if (voxelMaterial == Voxel2::Cell::Material_Air && box.isEmpty())
		return;

	unsigned char skipMaterial = skipWater ? Voxel2::Cell::Material_Water : Voxel2::Cell::Material_Max + 1;

    Vector3 centerv = center / Voxel::kCELL_SIZE;
	float vradius = radius / Voxel::kCELL_SIZE;

	for (int y = 0; y < rsize.y; ++y)
		for (int z = 0; z < rsize.z; ++z)
		{
			Vector3 diff = Vector3(rr.begin().x, y + rr.begin().y, z + rr.begin().z) + Vector3(0.5) - centerv;

            Voxel2::Cell* row = box.writeRow(0, y, z);

			for (int x = 0; x < rsize.x; ++x, diff.x += 1)
			{
				float d = diff.length();

				int occ = static_cast<int>(G3D::clamp(vradius - d + 0.5f, 0.f, 1.f) * Voxel2::Cell::Occupancy_Max + 0.5f);

				if (row[x].getMaterial() != skipMaterial)
					fillCell(row[x], voxelMaterial, occ);
			}
		}
	
	smoothGrid->write(rr, box);
}
void MegaClusterInstance::validateApi()
{
	if (!voxelGrid && !smoothGrid)
        throw std::runtime_error("Terrain API is not available");
}

void MegaClusterInstance::validateApiSmooth()
{
	if (!smoothGrid)
        throw std::runtime_error("Smooth terrain API is not available");
}

void MegaClusterInstance::setWaterColor(const Color3& value)
{
    if (waterColor != value)
    {
        waterColor = value;
        raisePropertyChanged(desc_WaterColor);
    }
}

void MegaClusterInstance::setWaterTransparency(float value)
{
    value = G3D::clamp(value, 0.f, 1.f);

    if (waterTransparency != value)
    {
        waterTransparency = value;
        raisePropertyChanged(desc_WaterTransparency);
    }
}

void MegaClusterInstance::setWaterWaveSize(float value)
{
    value = G3D::clamp(value, -FLT_MIN, FLT_MAX);

    if (waterWaveSize != value)
    {
        waterWaveSize = value;
        raisePropertyChanged(desc_WaterWaveSize);
    }
}

void MegaClusterInstance::setWaterWaveSpeed(float value)
{
	value = G3D::clamp(value, -FLT_MIN, FLT_MAX);

    if (waterWaveSpeed != value)
    {
        waterWaveSpeed = value;
        raisePropertyChanged(desc_WaterWaveSpeed);
    }
}

} //namespace
