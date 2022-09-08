#include "stdafx.h"
#include "MegaCluster.h"

#include "v8datamodel/MegaCluster.h"
#include "Voxel/AreaCopy.h"
#include "Voxel/Grid.h"

#include "v8world/Primitive.h"
#include "SceneUpdater.h"

#include "Util.h"
#include "FastLog.h"

#include "Water.h"

#include "G3D/Vector4int8.h"

#include "Material.h"
#include "ShaderManager.h"
#include "TextureManager.h"
#include "LightGrid.h"
#include "SceneManager.h"
#include "VisualEngine.h"

#include "GfxBase/RenderCaps.h"

LOGGROUP(RbxMegaClustersUpdate)
LOGGROUP(MegaClusterDirty)
LOGGROUP(TerrainCellListener)

using namespace RBX::Voxel;

namespace RBX
{
namespace Graphics
{

static Color4uint8 packNormal(float x, float y, float z)
{
    Vector3 normal = normalize(Vector3(x, y, z));

    return Color4uint8(normal.x * 127 + 127, normal.y * 127 + 127, normal.z * 127 + 127, 0);
}

struct Quad
{
    unsigned char p0, p1, p2, p3;
};

struct Wedge
{
    Quad geom;

    Color4uint8 normals[4];
    Color4uint8 tangents[4];
};

const Wedge kWedges[6] =
{
    // Block
    {
        {
            7, 4, 1, 2
        },

        {
            packNormal(0,1,1),
            packNormal(1,1,0),
            packNormal(0,1,-1),
            packNormal(-1,1,0),
        }
    },

    // Vertical Wedge
    {
        {
            7, 4, 1, 2
        },
        
        {
            packNormal(0,1,1),
            packNormal(1,1,0),
            packNormal(0,1,-1),
            packNormal(-1,1,0),
        },

        {
            packNormal(1,0,0),
            packNormal(0,0,-1),
            packNormal(-1,0,0),
            packNormal(0,0,1),
        }
    },

    // Corner Wedge
    {
        {
            7, 7, 0, 2
        },

        {
            packNormal(-1,1,1),
            packNormal(1,1,1),
            packNormal(1,1,-1),
            packNormal(-1,1,-1),
        },

        {
            packNormal(1,0,1),
            packNormal(1,0,-1),
            packNormal(-1,0,-1),
            packNormal(-1,0,1),
        }
    },

    // Inverse corner Wedge
    {
        {
            6, 4, 1, 1, 
        },

        {
            packNormal(-1,1,1),
            packNormal(1,1,1),
            packNormal(1,1,-1),
            packNormal(-1,1,-1),
        },

        {
            packNormal(1,0,1),
            packNormal(1,0,-1),
            packNormal(-1,0,-1),
            packNormal(-1,0,1),
        }
    },

    // Horizontal Wedge
    {
        {
            6, 4, 0, 2
        },

        {
            packNormal(-1,0,1),
            packNormal(1,0,1),
            packNormal(1,0,-1),
            packNormal(-1,0,-1),
        },

        {
            packNormal(1,0,1),
            packNormal(1,0,-1),
            packNormal(-1,0,-1),
            packNormal(-1,0,1),
        }
    },

    // Empty Wedge
    {
        {
            7, 4, 1, 2
        },

        {
            packNormal(0,1,1),
            packNormal(1,1,0),
            packNormal(0,1,-1),
            packNormal(-1,1,0),
        },
        
        {
            packNormal(1,0,0),
            packNormal(0,0,1),
            packNormal(-1,0,0),
            packNormal(0,0,-1),
        }
    }
};

enum SideStatus
{
    SideEmpty = 0,
    SideTriangle = 1,
    SideFull = 2
};

static const SideStatus kFaceToStatusMap[] = { SideTriangle, SideTriangle, SideTriangle, SideTriangle, SideEmpty, SideFull };

static unsigned char rotateCornerIndex(unsigned char index, CellOrientation orient)
{
    unsigned char hipart = index & 0x4;

    unsigned lowpart = index & 0x3;
    lowpart = (lowpart + orient) % 4; 

    return hipart | lowpart;	
}

static SideStatus getStatusFromFace(const BlockAxisFace& face)
{
    return kFaceToStatusMap[ face.skippedCorner ];
}

static const Vector3int16 centerOrig = Vector3int16(kCELL_SIZE /2 , kCELL_SIZE /2, kCELL_SIZE /2);

struct OFFSETINFOV2
{
    Vector3int16 position[4];
    Color4uint8 normal;
    Color4uint8 tangent;
};

static const int HSIZE = kCELL_SIZE/2;

static const OFFSETINFOV2 kAxisSideLookup[6] =
{
    {	// PlusX
        Vector3int16( HSIZE,  HSIZE, -HSIZE),
        Vector3int16( HSIZE,  HSIZE,  HSIZE),
        Vector3int16( HSIZE, -HSIZE,  HSIZE),
        Vector3int16( HSIZE, -HSIZE, -HSIZE),
        packNormal(1, 0, 0),				// normal
        packNormal(0, 0, -1),               // tangent
    },
    {	// PlusZ
        Vector3int16( HSIZE,  HSIZE,  HSIZE),
        Vector3int16(-HSIZE,  HSIZE,  HSIZE),
        Vector3int16(-HSIZE, -HSIZE,  HSIZE),
        Vector3int16( HSIZE, -HSIZE,  HSIZE),
        packNormal(0, 0, 1),				// normal
        packNormal(1, 0, 0),                // tangent
    },
    {	// MinusX
        Vector3int16(-HSIZE,  HSIZE,  HSIZE),
        Vector3int16(-HSIZE,  HSIZE, -HSIZE),
        Vector3int16(-HSIZE, -HSIZE, -HSIZE),
        Vector3int16(-HSIZE, -HSIZE,  HSIZE),
        packNormal(-1, 0, 0),				// normal
        packNormal(0, 0, 1),                // tangent
    },
    {	// MinusZ
        Vector3int16(-HSIZE,  HSIZE, -HSIZE),
        Vector3int16( HSIZE,  HSIZE, -HSIZE),
        Vector3int16( HSIZE, -HSIZE, -HSIZE),
        Vector3int16(-HSIZE, -HSIZE, -HSIZE),
        packNormal(0, 0, -1),				// normal
        packNormal(-1, 0, 0),               // tangent
    },
    {	// PlusY
        Vector3int16( HSIZE,  HSIZE, -HSIZE),
        Vector3int16(-HSIZE,  HSIZE, -HSIZE),
        Vector3int16(-HSIZE,  HSIZE,  HSIZE),
        Vector3int16( HSIZE,  HSIZE,  HSIZE),
        packNormal(0, 1, 0),				// normal
        packNormal(1, 0, 0),                // tangent
    },
    {	// MinusY
        Vector3int16( HSIZE, -HSIZE,  HSIZE),
        Vector3int16(-HSIZE, -HSIZE,  HSIZE),
        Vector3int16(-HSIZE, -HSIZE, -HSIZE),
        Vector3int16( HSIZE, -HSIZE, -HSIZE),
        packNormal(0, -1, 0),				// normal
        packNormal(1, 0, 0),                // tangent
    },
};


struct CORNEROFFSET
{
    Vector3int16 posoffset;
};

static CORNEROFFSET CornerLookup[8] =
{
    { Vector3int16(-kCELL_SIZE/2, -kCELL_SIZE/2, -kCELL_SIZE/2) },
    { Vector3int16(-kCELL_SIZE/2, -kCELL_SIZE/2, kCELL_SIZE/2) },
    { Vector3int16(kCELL_SIZE/2, -kCELL_SIZE/2, kCELL_SIZE/2) },
    { Vector3int16(kCELL_SIZE/2, -kCELL_SIZE/2, -kCELL_SIZE/2) },
    
    { Vector3int16(-kCELL_SIZE/2, kCELL_SIZE/2, -kCELL_SIZE/2) },
    { Vector3int16(-kCELL_SIZE/2, kCELL_SIZE/2, kCELL_SIZE/2) },
    { Vector3int16(kCELL_SIZE/2, kCELL_SIZE/2, kCELL_SIZE/2) },
    { Vector3int16(kCELL_SIZE/2, kCELL_SIZE/2, -kCELL_SIZE/2) },
};

struct MaterialTextureCoordinates
{
    const Vector2int16 startPixel;
    const Vector2int16 oneCellPixels;
    const unsigned int xMask;
    const unsigned int yMask;

    MaterialTextureCoordinates(const Vector2int16& startPixel, const Vector2int16& endPixel, const Vector2int16& logicalCellSize)
        : startPixel(startPixel)
        , xMask(logicalCellSize.x - 1)
        , yMask(logicalCellSize.y - 1)
        , oneCellPixels((endPixel - startPixel) / logicalCellSize)
    {
        // assert that x and y dimensions are powers of 2
        RBXASSERT((logicalCellSize.x & (logicalCellSize.x - 1)) == 0);
        RBXASSERT((logicalCellSize.y & (logicalCellSize.y - 1)) == 0);
    }

    void calculateTextureCoordinates(const Vector2int16& cell, const Vector2int16& atlasOffset, Vector2int16* uvs) const
    {
        Vector2int16 cellMod = Vector2int16(cell.x & xMask, cell.y & yMask) * oneCellPixels;
        
        Vector2int16 uvMin = startPixel + cellMod + atlasOffset;
        Vector2int16 uvMax = uvMin + oneCellPixels;

        uvs[0] = Vector2int16(uvMax.x, uvMin.y);
        uvs[1] = Vector2int16(uvMin.x, uvMin.y);
        uvs[2] = Vector2int16(uvMin.x, uvMax.y);
        uvs[3] = Vector2int16(uvMax.x, uvMax.y);
    }
};

struct TriangleMaterialTextureCoordinates
{
    struct Triangle
    {
        Vector2int16 a, b, c, d;
    };
    
    boost::scoped_array<Triangle> table;
    int mask;
    
    TriangleMaterialTextureCoordinates(const Vector2int16& startPixel, const Vector2int16& endPixel, int cellCount, int tiling, bool inverse)
    {
        table.reset(new Triangle[cellCount * cellCount * tiling * tiling]);
        mask = cellCount * tiling - 1;
        
        Vector2 startPixelUv = Vector2(endPixel.x, startPixel.y);
        Vector2 areaSizeUv = Vector2(endPixel - startPixel);
        
        // X axis has triangle base, repeated cellCount + 0.5 times
        float triangleBase = areaSizeUv.x / (cellCount + 0.5f);
        
        // Y axis has triangle height, repeated cellCount times
        float triangleHeight = areaSizeUv.y / cellCount;
        
        Vector2 triangleSide = Vector2(-triangleBase / 2, triangleHeight);
        
        float triangleBaseTile = triangleBase / tiling;
        Vector2 triangleSideTile = triangleSide / tiling;
        
        for (int y = 0; y < cellCount * tiling; ++y)
        {
            for (int x = 0; x < cellCount * tiling; ++x)
            {
                Triangle& t = table[x + y * cellCount * tiling];
                
                // figure out quad coordinates for this triangle and the inverse triangle pair
                Vector2 quadStart = startPixelUv - Vector2(triangleBaseTile, 0) * x + triangleSideTile * y;
                Vector2 quadEnd = quadStart - Vector2(triangleBaseTile, 0) + triangleSideTile;
                
                // special treatment for last big triangle - it moves to the right to conserve UV space
                if (x / tiling == cellCount - 1 && y / tiling == cellCount - 1 && (x % tiling) + (y % tiling) + !inverse >= tiling)
                {
                    quadStart.x += triangleBase * cellCount;
                    quadEnd.x += triangleBase * cellCount;
                }
                
                if (inverse)
                {
                    t.a = Vector2int16(quadStart);
                    t.b = Vector2int16(quadStart - Vector2(triangleBaseTile, 0));
                    t.c = Vector2int16();
                    t.d = Vector2int16(quadStart + triangleSideTile);
                }
                else
                {
                    t.a = Vector2int16();
                    t.b = Vector2int16(quadStart - Vector2(triangleBaseTile, 0));
                    t.c = Vector2int16(quadEnd);
                    t.d = Vector2int16(quadStart + triangleSideTile);
                }
            }
        }
    }
    
    void calculateTextureCoordinates(const Vector2int16& cell, const Vector2int16& atlasOffset, Vector2int16* uvs) const
    {
        int cellModX = mask - (cell.x & mask);
        int cellModY = cell.y & mask;
        int index = cellModX + cellModY * (mask + 1);
        
        uvs[0] = table[index].a + atlasOffset;
        uvs[1] = table[index].b + atlasOffset;
        uvs[2] = table[index].c + atlasOffset;
        uvs[3] = table[index].d + atlasOffset;
    }
};

namespace {
    const int kMaterialPixelDimension = 512;
    const int kAtlasPixelDimension = 4 * kMaterialPixelDimension;

    // Top
    const MaterialTextureCoordinates kMaterial_Top_High(Vector2int16(8, 8), Vector2int16(248, 248), Vector2int16(4, 4)); 
    const MaterialTextureCoordinates kMaterial_Top_Low (Vector2int16(8, 8), Vector2int16(248, 248), Vector2int16(16, 16));

    // TopSide
    const MaterialTextureCoordinates kMaterial_TopSide_High(Vector2int16(264, 8), Vector2int16(504, 68), Vector2int16(4, 1));
    const MaterialTextureCoordinates kMaterial_TopSide_Low (Vector2int16(264, 8), Vector2int16(504, 68), Vector2int16(16, 4));

    // Side
    const MaterialTextureCoordinates kMaterial_Side_High(Vector2int16(264, 68), Vector2int16(504, 188), Vector2int16(4, 2));
    const MaterialTextureCoordinates kMaterial_Side_Low (Vector2int16(264, 68), Vector2int16(504, 188), Vector2int16(16, 8));

    // WedgeVertical
    const MaterialTextureCoordinates kMaterial_WedgeVertical_High(Vector2int16(8, 332), Vector2int16(248, 504), Vector2int16(4, 2));
    const MaterialTextureCoordinates kMaterial_WedgeVertical_Low (Vector2int16(8, 332), Vector2int16(248, 504), Vector2int16(16, 8));

    // WedgeHorizontal
    const MaterialTextureCoordinates kMaterial_WedgeHorizontal_High(Vector2int16(332, 384), Vector2int16(504, 504), Vector2int16(2, 2));
    const MaterialTextureCoordinates kMaterial_WedgeHorizontal_Low (Vector2int16(332, 384), Vector2int16(504, 504), Vector2int16(8, 8));

    // Bottom
    const MaterialTextureCoordinates kMaterial_Bottom_High(Vector2int16(8, 264), Vector2int16(216, 316), Vector2int16(4, 1));
    const MaterialTextureCoordinates kMaterial_Bottom_Low (Vector2int16(8, 264), Vector2int16(216, 316), Vector2int16(16, 4));

    // WedgeCorner
    const TriangleMaterialTextureCoordinates kMaterial_WedgeCorner_High(Vector2int16(290, 204), Vector2int16(504, 354), 2, 1, false);
    const TriangleMaterialTextureCoordinates kMaterial_WedgeCorner_Low (Vector2int16(290, 204), Vector2int16(504, 354), 2, 4, false);

    // WedgeInverseCorner
    const TriangleMaterialTextureCoordinates kMaterial_WedgeInverseCorner_High(Vector2int16(290, 204), Vector2int16(504, 354), 2, 1, true);
    const TriangleMaterialTextureCoordinates kMaterial_WedgeInverseCorner_Low (Vector2int16(290, 204), Vector2int16(504, 354), 2, 4, true);
}

static Vector2int16 AtlasOffsetLookup[16] =
{
    Vector2int16(0, 0) * kMaterialPixelDimension,   // Grass
    Vector2int16(1, 0) * kMaterialPixelDimension,   // Sand
    Vector2int16(2, 0) * kMaterialPixelDimension,   // Brick
    Vector2int16(3, 0) * kMaterialPixelDimension,   // Granite
    Vector2int16(0, 1) * kMaterialPixelDimension,   // Asphault
    Vector2int16(3, 1) * kMaterialPixelDimension,   // Iron
    Vector2int16(1, 1) * kMaterialPixelDimension,   // Aluminum
    Vector2int16(2, 1) * kMaterialPixelDimension,   // Gold
    Vector2int16(2, 2) * kMaterialPixelDimension,  // WoodPlank
    Vector2int16(3, 2) * kMaterialPixelDimension,  // WoodLog
    Vector2int16(0, 3) * kMaterialPixelDimension,  // Gravel
    Vector2int16(1, 3) * kMaterialPixelDimension,  // CinderBlock
    Vector2int16(2, 3) * kMaterialPixelDimension,  // MossyStone
    Vector2int16(3, 3) * kMaterialPixelDimension,  // Cement
    Vector2int16(0, 2) * kMaterialPixelDimension,   // RedPlastic
    Vector2int16(1, 2) * kMaterialPixelDimension,   // BluePlastic
};

static const Vector3int16 kFaceDirectionLocationOffset[6] = 
{
    Vector3int16(1,0,0),
    Vector3int16(0,0,1),
    Vector3int16(-1,0,0),
    Vector3int16(0,0,-1),
    Vector3int16(0,1,0),
    Vector3int16(0,-1,0)
};

// offsets:
// Pos X 0 => 2
// Pos Z 1 => 3
// Neg X 2 => 0
// Neg Z 3 => 1
// Pos Y 4 => 5
// Neg Y 5 => 4
static const FaceDirection kOppositeSideOffset[6] = { MinusX, MinusZ, PlusX, PlusZ, MinusY, PlusY };

enum RenderPredStatus
{
    NO_RENDER = 0,
    RENDER_FORWARD = 1,
    RENDER_BACKWARD = 2,
    RENDER_BOTH = 3,
};

template<class VoxelStore>
struct SolidTerrainRenderPredicate
{
    inline bool wedgeFace(const Voxel::Cell cell)
	{
        return cell.solid.getBlock() != CELL_BLOCK_Solid && cell.solid.getBlock() != CELL_BLOCK_Empty;
    }

    inline RenderPredStatus internal(const typename VoxelStore::Region::iterator& iterator, FaceDirection direction)
	{
        Voxel::Cell centerCell = iterator.getCellAtCurrentLocation();
        Voxel::Cell neighborCell = iterator.getNeighborCell(direction);

        const BlockAxisFace& centerFace = GetOrientedFace(centerCell, direction);
        const BlockAxisFace& neighborFace = GetOrientedFace(neighborCell, kOppositeSideOffset[direction]);

        SideStatus centerStatus = getStatusFromFace(centerFace);
        SideStatus neighborStatus = getStatusFromFace(neighborFace);

        if (centerStatus != neighborStatus)
		{
            return (RenderPredStatus) (RENDER_FORWARD + (neighborStatus > centerStatus));
        }
		else if (centerStatus == SideTriangle)
		{
            BlockAxisFace::SkippedCorner mirrorSkippedCorner;
            if (direction == PlusY || direction == MinusY)
			{
                mirrorSkippedCorner = BlockAxisFace::YAxisMirror(centerFace.skippedCorner);
            }
			else
			{
                mirrorSkippedCorner = BlockAxisFace::XZAxisMirror(centerFace.skippedCorner);
            }

            if (mirrorSkippedCorner != neighborFace.skippedCorner)
			{
                return RENDER_BOTH;
            }
        }

        return NO_RENDER;
    }

    inline bool external(const typename VoxelStore::Region::iterator& iterator, FaceDirection direction)
	{
        return GetOrientedFace(iterator.getCellAtCurrentLocation(), direction).skippedCorner != BlockAxisFace::EmptyAllSkipped;
    }
};

template<class VoxelStore>
struct WaterRenderPredicate {

    inline bool wedgeFace(const Voxel::Cell cell) const
	{
        return false;
    }

    inline RenderPredStatus internal(const typename VoxelStore::Region::iterator& iterator, FaceDirection direction) const
	{
        const Voxel::Cell center = iterator.getCellAtCurrentLocation();
        const Voxel::Cell neighbor = iterator.getNeighborCell(direction);
        if (!isWedgeSideNotFull(center, direction) || !isWedgeSideNotFull(neighbor, kOppositeSideOffset[direction]))
		{
            return NO_RENDER;
        }

        // technically this could be reduced to
        // iterator.hasWaterAtCurrentLocation(), but that call is
        // moderately expensive, so it is faster to short circuit it when
        // possible.
        bool centerQualifiesToRender =
            !center.isEmpty() &&
            center.solid.getBlock() != CELL_BLOCK_Solid &&
            (center.solid.getBlock() == CELL_BLOCK_Empty ||
            iterator.hasWaterAtCurrentLocation());

        bool neighborQualifiesToRender = 
            !neighbor.isEmpty() &&
            neighbor.solid.getBlock() != CELL_BLOCK_Solid &&
            (neighbor.solid.getBlock() == CELL_BLOCK_Empty ||
            iterator.hasWaterAtNeighbor(direction));

        bool willRender = centerQualifiesToRender != neighborQualifiesToRender;
        return (RenderPredStatus) 
            (willRender + (willRender && neighborQualifiesToRender));

    }

    inline bool external(const typename VoxelStore::Region::iterator& iterator, FaceDirection direction) const
	{
        Voxel::Cell center = iterator.getCellAtCurrentLocation();
        return isWedgeSideNotFull(center, direction) && iterator.hasWaterAtCurrentLocation();
    }
};

template<class VoxelStore>
struct FaceCounter
{
    int count;

    FaceCounter()
		: count(0)
	{
	}

    void apply(const typename VoxelStore::Region::iterator& iterator, FaceDirection d, RenderPredStatus status)
	{
        count += ((status + 1) >> 1);
    }

    void wedgeFace(const typename VoxelStore::Region::iterator& iterator)
	{
        count++;
    }
};

inline Vector3int16 computeCenter(const Vector3int16& logical)
{
    return Vector3int16(
        kCELL_SIZE * (logical.x & (kXZ_CHUNK_SIZE - 1)) + centerOrig.x,
        kCELL_SIZE * (logical.y & (kY_CHUNK_SIZE - 1))  + centerOrig.y,
        kCELL_SIZE * (logical.z & (kXZ_CHUNK_SIZE - 1)) + centerOrig.z);
}

struct EdgeDistanceLookup
{
    Color4uint8 values[4];
};

struct FaceVertexLookup
{
    unsigned int indices[4];
};

static EdgeDistanceLookup gEdgeDistanceLookup[16][6];
static bool gTriangleOutlineLookup[4][4];
static FaceVertexLookup gFaceVertexLookup[6];

static void initLookupTables()
{
    for (unsigned outlineMask = 0; outlineMask < 16; outlineMask++)
    {
        for (unsigned skippedCorner = BlockAxisFace::TopRight; skippedCorner <= BlockAxisFace::FullNoneSkipped; skippedCorner++)
        {
            Vector4 edgeMin = Vector4(outlineMask & 0x1 ? 0.0f : 10.0f, outlineMask & 0x2 ? 0.0f : 10.0f, outlineMask & 0x4 ? 0.0f : 10.0f, outlineMask & 0x8 ? 0.0f : 10.0f);		
            Vector4 edgeMax = Vector4(outlineMask & 0x1 ? 4.0f : 10.0f, outlineMask & 0x2 ? 4.0f : 10.0f, outlineMask & 0x4 ? 4.0f : 10.0f, outlineMask & 0x8 ? 4.0f : 10.0f);

            unsigned skippedIndex = skippedCorner < 4 ? (skippedCorner + 3) % 4 : 4;

            const Color4uint8 outlineOffsets [] =
            {
                Color4uint8(edgeMin.x, edgeMax.y, edgeMax.z, edgeMin.w),
                Color4uint8(edgeMin.x, edgeMax.y, edgeMin.z, edgeMax.w),
                Color4uint8(edgeMax.x, edgeMin.y, edgeMin.z, edgeMax.w),
                Color4uint8(edgeMax.x, edgeMin.y, edgeMax.z, edgeMin.w),
            };

            EdgeDistanceLookup& lookup = gEdgeDistanceLookup[outlineMask][skippedCorner];

            for (unsigned int i = 0; i < 4; ++i)
            {
                if (i == skippedIndex)
                {
                    switch (skippedCorner)
                    {
                    case 0: lookup.values[i] = Color4uint8(edgeMin.x, edgeMin.y, edgeMax.z, edgeMin.w); break;
                    case 1: lookup.values[i] = Color4uint8(edgeMin.x, edgeMax.y, edgeMin.z, edgeMin.w); break;
                    case 2: lookup.values[i] = Color4uint8(edgeMin.x, edgeMin.y, edgeMin.z, edgeMax.w); break;
                    case 3: lookup.values[i] = Color4uint8(edgeMax.x, edgeMin.y, edgeMin.z, edgeMin.w); break;
                    }
                    
                }
                else
                {
                    lookup.values[i] = outlineOffsets[i];
                }
            }
        }
    }

    for (unsigned skippedCorner = BlockAxisFace::TopRight; skippedCorner <= BlockAxisFace::BottomRight; skippedCorner++)
    {
        for (unsigned incomingEdge = 0; incomingEdge < 4; incomingEdge++)
        {
            unsigned incoming = incomingEdge ^ 0x1;
            bool result = false;

            switch (skippedCorner)
            {
            case BlockAxisFace::TopRight:
                result = incoming == 1 || incoming == 2;
                break;
            case BlockAxisFace::TopLeft:
                result = incoming == 1 || incoming == 3;
                break;
            case BlockAxisFace::BottomLeft:
                result = incoming == 3 || incoming == 0;
                break;
            case BlockAxisFace::BottomRight:
                result = incoming == 0 || incoming == 2;
                break;
            }

            gTriangleOutlineLookup[skippedCorner][incomingEdge] = result;
        }
    }

    for (unsigned skippedCorner = BlockAxisFace::TopRight; skippedCorner <= BlockAxisFace::FullNoneSkipped; skippedCorner++)
    {
        unsigned int stitchingOffset = BlockAxisFace::divideTopLeftToBottomRight((BlockAxisFace::SkippedCorner)skippedCorner) ? 0 : 3;
        
        for (unsigned int i = 0; i < 4; ++i)
        {
            unsigned int stitchI = (i + stitchingOffset) % 4;
            
            gFaceVertexLookup[skippedCorner].indices[stitchI] = i;
        }

        if (skippedCorner < 4)
        {
            unsigned int i = (skippedCorner + 3) % 4;
            unsigned int stitchI = (skippedCorner + stitchingOffset) % 4;
            
            gFaceVertexLookup[skippedCorner].indices[stitchI] = i;
        }
    }
}

template <typename Vertex>
inline void outputFace(Vertex* output, const Vector3int16& center,
    const OFFSETINFOV2& offsetInfo, const BlockAxisFace& face,
    const Vector2int16 highUv[4], const Vector2int16 lowUv[4],
    const unsigned char outlineMask) 
{
    const EdgeDistanceLookup& edgeDistances = gEdgeDistanceLookup[outlineMask][face.skippedCorner];
    const Color4uint8& normal = offsetInfo.normal;
    const Color4uint8& tangent = offsetInfo.tangent;
    const FaceVertexLookup& fvl = gFaceVertexLookup[face.skippedCorner];
    
    int i0 = fvl.indices[0], i1 = fvl.indices[1], i2 = fvl.indices[2], i3 = fvl.indices[3];
    
    output[0].create(center + offsetInfo.position[i0], normal, highUv[i0], lowUv[i0], edgeDistances.values[i0], tangent);
    output[1].create(center + offsetInfo.position[i1], normal, highUv[i1], lowUv[i1], edgeDistances.values[i1], tangent);
    output[2].create(center + offsetInfo.position[i2], normal, highUv[i2], lowUv[i2], edgeDistances.values[i2], tangent);
    output[3].create(center + offsetInfo.position[i3], normal, highUv[i3], lowUv[i3], edgeDistances.values[i3], tangent);
}

template <typename Vertex>
inline void outputFace(Vertex* output, const Vector3int16& center,
    const OFFSETINFOV2& offsetInfo, const BlockAxisFace& face, const Vector2int16 highUv[4])
{
    const Color4uint8& normal = offsetInfo.normal;
    const FaceVertexLookup& fvl = gFaceVertexLookup[face.skippedCorner];
    
    int i0 = fvl.indices[0], i1 = fvl.indices[1], i2 = fvl.indices[2], i3 = fvl.indices[3];
    
    output[0].create(center + offsetInfo.position[i0], normal, highUv[i0 & 3] );
    output[1].create(center + offsetInfo.position[i1], normal, highUv[i1 & 3] );
    output[2].create(center + offsetInfo.position[i2], normal, highUv[i2 & 3] );
    output[3].create(center + offsetInfo.position[i3], normal, highUv[i3 & 3] );
}

template <typename TC> struct RenderLookup
{
    Vector3int16 uDot;
    Vector3int16 vDot;
    const TC* highRes;
    const TC* lowRes;

    inline Vector2int16 dot(const Vector3int16& cellLocation) const
    {
        return Vector2int16(
            cellLocation.x * uDot.x + cellLocation.y * uDot.y + cellLocation.z * uDot.z,
            cellLocation.x * vDot.x + cellLocation.y * vDot.y + cellLocation.z * vDot.z);
    }
};

static const RenderLookup<MaterialTextureCoordinates> kRenderLookupByFace[6][2] =
{
    {
        // +X
        {
            Vector3int16(-1, 0,-1),
            Vector3int16( 0,-1, 0),
            &kMaterial_Side_High,
            &kMaterial_Side_Low,
        },
        {
            Vector3int16(-1, 0,-1),
            Vector3int16( 0,-1, 0),
            &kMaterial_TopSide_High,
            &kMaterial_TopSide_Low,
        },
    },
    {
        // +Z 
        {
            Vector3int16( 1, 0, 1),
            Vector3int16( 0,-1, 0),
            &kMaterial_Side_High,
            &kMaterial_Side_Low,
        },
        {
            Vector3int16( 1, 0, 1),
            Vector3int16( 0,-1, 0),
            &kMaterial_TopSide_High,
            &kMaterial_TopSide_Low,
        },
    },
    {
        // -X
        {
            Vector3int16( 1, 0, 1),
            Vector3int16( 0,-1, 0),
            &kMaterial_Side_High,
            &kMaterial_Side_Low,
        },
        {
            Vector3int16( 1, 0, 1),
            Vector3int16( 0,-1, 0),
            &kMaterial_TopSide_High,
            &kMaterial_TopSide_Low,
        },
    },
    {
        // -Z
        {
            Vector3int16(-1, 0,-1),
            Vector3int16( 0,-1, 0),
            &kMaterial_Side_High,
            &kMaterial_Side_Low,
        },
        {
            Vector3int16(-1, 0,-1),
            Vector3int16( 0,-1, 0),
            &kMaterial_TopSide_High,
            &kMaterial_TopSide_Low,
        },
    },
    {
        // +Y
        {
            Vector3int16( 1, 0, 0),
            Vector3int16( 0, 0, 1),
            &kMaterial_Top_High,
            &kMaterial_Top_Low,
        },
        {
            Vector3int16( 1, 0, 0),
            Vector3int16( 0, 0, 1),
            &kMaterial_Top_High,
            &kMaterial_Top_Low,
        },
    },
    {
        // -Y
        {
            Vector3int16( 1, 0, 0),
            Vector3int16( 0, 0,-1),
            &kMaterial_Bottom_High,
            &kMaterial_Bottom_Low,
        },
        {
            Vector3int16( 1, 0, 0),
            Vector3int16( 0, 0,-1),
            &kMaterial_Bottom_High,
            &kMaterial_Bottom_Low,
        },
    }
};

static const RenderLookup<MaterialTextureCoordinates> kVerticalWedgeLookupByOrientation[4] =
{
    // NegZ
    {
        Vector3int16( 1, 1, 1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeVertical_High,
        &kMaterial_WedgeVertical_Low,
    },
    // X
    {
        Vector3int16( 1, 1,-1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeVertical_High,
        &kMaterial_WedgeVertical_Low,
    },
    // Z
    {
        Vector3int16(-1, 1,-1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeVertical_High,
        &kMaterial_WedgeVertical_Low,
    },
    // NegX
    {
        Vector3int16(-1, 1, 1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeVertical_High,
        &kMaterial_WedgeVertical_Low,
    }
};

static const RenderLookup<MaterialTextureCoordinates> kHorizontalWedgeLookupByOrientation[4] =
{		
    // NegZ
    {
        Vector3int16( 0, 0, 1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeHorizontal_High,
        &kMaterial_WedgeHorizontal_Low,
    },
    // X
    {
        Vector3int16( 1, 0, 0),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeHorizontal_High,
        &kMaterial_WedgeHorizontal_Low,
    },
    // Z
    {
        Vector3int16( 0, 0,-1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeHorizontal_High,
        &kMaterial_WedgeHorizontal_Low,
    },
    // NegX
    {
        Vector3int16(-1, 0, 0),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeHorizontal_High,
        &kMaterial_WedgeHorizontal_Low,
    }
};

static const RenderLookup<TriangleMaterialTextureCoordinates> kCornerWedgeLookupByOrientation[4] =
{		
    // NegZ
    {
        Vector3int16( 0, 0, 1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeCorner_High,
        &kMaterial_WedgeCorner_Low,
    },
    // X
    {
        Vector3int16( 1, 0, 0),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeCorner_High,
        &kMaterial_WedgeCorner_Low,
    },
    // Z
    {
        Vector3int16( 0, 0,-1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeCorner_High,
        &kMaterial_WedgeCorner_Low,
    },
    // NegX
    {
        Vector3int16(-1, 0, 0),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeCorner_High,
        &kMaterial_WedgeCorner_Low,
    }
};

static const RenderLookup<TriangleMaterialTextureCoordinates> kInverseCornerWedgeLookupByOrientation[4] =
{		
    // NegZ
    {
        Vector3int16( 0, 0, 1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeInverseCorner_High,
        &kMaterial_WedgeInverseCorner_Low,
    },
    // X
    {
        Vector3int16( 1, 0, 0),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeInverseCorner_High,
        &kMaterial_WedgeInverseCorner_Low,
    },
    // Z
    {
        Vector3int16( 0, 0,-1),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeInverseCorner_High,
        &kMaterial_WedgeInverseCorner_Low,
    },
    // NegX
    {
        Vector3int16(-1, 0, 0),
        Vector3int16( 0,-1, 0),
        &kMaterial_WedgeInverseCorner_High,
        &kMaterial_WedgeInverseCorner_Low,
    }
};

template<class VoxelStore, typename Vertex>
struct SolidTerrainRenderer
{
    Vertex* output;

    void renderHelper(
        const Voxel::Cell cell, const CellMaterial material,
        const Vector3int16& location, bool isTopSide,
        const Vector3int16& center, FaceDirection faceDirection, unsigned char outlineMask)
    {
        const OFFSETINFOV2& offsetInfo = kAxisSideLookup[faceDirection];
        const BlockAxisFace& usedFace = GetOrientedFace(cell, faceDirection);

		const RenderLookup<MaterialTextureCoordinates>& info = kRenderLookupByFace[faceDirection][isTopSide];

        Vector2int16 logicalCell = info.dot(location);
        Vector2int16 highUvs[4], lowUvs[4];

		info.highRes->calculateTextureCoordinates(logicalCell, AtlasOffsetLookup[material - 1], highUvs);
		info.lowRes->calculateTextureCoordinates(logicalCell, AtlasOffsetLookup[material - 1], lowUvs);
        
        outputFace(output, center, offsetInfo, usedFace, highUvs, lowUvs, outlineMask);

        output += 4;
    }

    inline bool faceGenerated(Voxel::Cell& currentCell, Voxel::Cell& neighborCell, FaceDirection direction, unsigned incoming)
    {
        const BlockAxisFace& centerFace = GetOrientedFace(currentCell, direction);
        const BlockAxisFace& neighborFace = GetOrientedFace(neighborCell, kOppositeSideOffset[direction]);

        SideStatus centerStatus = getStatusFromFace(centerFace);
        SideStatus neighborStatus = getStatusFromFace(neighborFace);
        if (centerStatus != SideTriangle)
            return centerStatus != neighborStatus;
        else 
            return gTriangleOutlineLookup[centerFace.skippedCorner][incoming];
    }

    unsigned char detectOutlines(const typename VoxelStore::Region::iterator& iterator, FaceDirection faceDirection, RenderPredStatus status)
    {
        unsigned char mask = 0xf;

        const static FaceDirection adjDirections [6][4] =
        {
            { PlusY,  MinusY, PlusZ,  MinusZ }, // PlusX 
            { PlusY,  MinusY, MinusX, PlusX }, // PlusZ
            { PlusY,  MinusY, MinusZ, PlusZ }, // MinusX
            { PlusY,  MinusY, PlusX,  MinusX }, // MinusZ
            { MinusZ, PlusZ,  MinusX, PlusX }, // PlusY
            { PlusZ,  MinusZ, MinusX, PlusX }, // MinusY
        };

        FaceDirection current = status == RENDER_FORWARD ? Invalid : faceDirection;
        FaceDirection next = status == RENDER_FORWARD ? faceDirection : Invalid;
        
        CellMaterial currentMaterial = iterator.getNeighborMaterial(current);

        faceDirection = status == RENDER_FORWARD ? faceDirection : kOppositeSideOffset[faceDirection];

        unsigned maskBit = 0x1;
        for (int i = 0; i < 4; i++)
        {
            FaceDirection neighborDirection = adjDirections[faceDirection][i];
            CellMaterial adjMaterial = iterator.getNeighborMaterial(current, neighborDirection);

            if (adjMaterial == currentMaterial)
            {
                Voxel::Cell neighborCell = iterator.getNeighborCell(current, neighborDirection);
                Voxel::Cell nextNeighborCell = iterator.getNeighborCell(next, neighborDirection);
                if (faceGenerated(neighborCell, nextNeighborCell, faceDirection, i))
                {
                    mask &= ~maskBit;
                }
            }

            maskBit <<= 1;
        }

        return mask;
    }

    FaceDirection rotate(FaceDirection face, CellOrientation orientation)
    {
        return face >= PlusY ? face : (FaceDirection)((face + 4 - orientation) % 4);
    }

    void apply(const typename VoxelStore::Region::iterator& iterator, FaceDirection faceDirection, RenderPredStatus status)
    {
        Vector3int16 center = computeCenter(iterator.getCurrentLocation());

        bool impossibleTopSide = faceDirection == PlusY || faceDirection == MinusY;
        if (status & RENDER_FORWARD)
        {
            unsigned char outlineMask = detectOutlines(iterator, faceDirection, RENDER_FORWARD);
            bool isTopSide = !impossibleTopSide &&
                iterator.getNeighborCell(PlusY).isEmpty();
            renderHelper(iterator.getCellAtCurrentLocation(),
                iterator.getMaterialAtCurrentLocation(),
                iterator.getCurrentLocation(), isTopSide,
                center, faceDirection, outlineMask);
        }
        if (status & RENDER_BACKWARD)
        {
            unsigned char outlineMask = detectOutlines(iterator, faceDirection, RENDER_BACKWARD);
            const Vector3int16& faceDirectionOffset = kFaceDirectionLocationOffset[faceDirection];
            bool isTopSide = !impossibleTopSide &&
                iterator.getArbitraryNeighborCell(
                    faceDirectionOffset + kFaceDirectionLocationOffset[PlusY]).isEmpty();
            center += faceDirectionOffset * kCELL_SIZE;
            renderHelper(
                iterator.getNeighborCell(faceDirection),
                iterator.getNeighborMaterial(faceDirection),
                iterator.getCurrentLocation() + faceDirectionOffset,
				isTopSide, center, kOppositeSideOffset[faceDirection], outlineMask);
        }
    }

    unsigned char detectWedgeOutlines(const typename VoxelStore::Region::iterator& iterator)
    {
        unsigned mask = 0xf;
        Voxel::Cell cell = iterator.getCellAtCurrentLocation();
        CellMaterial material = iterator.getMaterialAtCurrentLocation();
        CellBlock block = cell.solid.getBlock();
        CellOrientation orient = cell.solid.getOrientation();

        unsigned maskBit = 0x1;

        if (block == CELL_BLOCK_VerticalWedge || block == CELL_BLOCK_HorizontalWedge) 
        {
            const static FaceDirection verticalWedgeDirections [4][2] = { { PlusY, MinusZ }, { MinusY, PlusZ }, { MinusX, Invalid }, { PlusX, Invalid } };				
            const static FaceDirection horizontalWedgeDirections [4][2] = { { PlusY, Invalid }, { MinusY, Invalid }, { MinusX, MinusZ }, { PlusX, PlusZ } };

            const FaceDirection (&lookup) [4][2] = block == CELL_BLOCK_VerticalWedge ? verticalWedgeDirections : horizontalWedgeDirections;

            unsigned maskBit = 0x1;
            for (int i = 0; i < 4; i++)
            {
                FaceDirection f0 = rotate(lookup[i][0], orient);
                FaceDirection f1 = rotate(lookup[i][1], orient);
                CellMaterial adjMaterial = iterator.getNeighborMaterial(f0, f1);
                Voxel::Cell adjNextCell = iterator.getNeighborCell(f0, f1);

                if (adjMaterial == material && adjNextCell.solid.getOrientation() == orient && adjNextCell.solid.getBlock() == block)
                    mask &= ~maskBit;

                maskBit <<= 1;
            }
        }
        else
        {
            const static FaceDirection cornerWedgeDirections [4] = { PlusX, MinusY, MinusZ, PlusX };
            const static FaceDirection inverseCornerWedgeDirections [4] = { PlusY, MinusX, MinusX, PlusZ };

            const FaceDirection (&lookup)[4] = block == CELL_BLOCK_CornerWedge ? cornerWedgeDirections : inverseCornerWedgeDirections;

            for (int i = 0; i < 4; i++)
            {
                if (lookup[i] == Invalid)
                    mask &= ~maskBit;
                else
                {
                    FaceDirection f = rotate(lookup[i], orient);
                    CellMaterial adjMaterial = iterator.getNeighborMaterial(f);
                    Voxel::Cell adjNextCell = iterator.getNeighborCell(f);

                    if (adjMaterial == material && adjNextCell.solid.getOrientation() == orient && adjNextCell.solid.getBlock() == 5-block)
                        mask &= ~maskBit;
                }

                maskBit <<= 1;
            }
        }

        return mask;
    }

    void wedgeFace(const typename VoxelStore::Region::iterator& iterator)
    {
        Voxel::Cell cell = iterator.getCellAtCurrentLocation();
        CellMaterial material = iterator.getMaterialAtCurrentLocation();
        CellBlock block = cell.solid.getBlock();
        CellOrientation orient = cell.solid.getOrientation();

        Vector3int16 center = computeCenter(iterator.getCurrentLocation());

        unsigned outlineMask = detectWedgeOutlines(iterator);

        const Wedge& wedge = kWedges[block];
        const Color4uint8& normal = wedge.normals[orient];
        const Color4uint8& tangent = wedge.tangents[orient];

        unsigned char index0 = rotateCornerIndex(wedge.geom.p0, orient);
        unsigned char index1 = rotateCornerIndex(wedge.geom.p1, orient);
        unsigned char index2 = rotateCornerIndex(wedge.geom.p2, orient);
        unsigned char index3 = rotateCornerIndex(wedge.geom.p3, orient);

        OFFSETINFOV2 offsetInfo = 
        {
            CornerLookup[index0].posoffset,
            CornerLookup[index1].posoffset,
            CornerLookup[index2].posoffset,
            CornerLookup[index3].posoffset,
            normal,
            tangent
        };

        if (block == CELL_BLOCK_CornerWedge || block == CELL_BLOCK_InverseCornerWedge)
        {
            BlockAxisFace usedFace;
			const RenderLookup<TriangleMaterialTextureCoordinates>* info;
            
            if (block == CELL_BLOCK_CornerWedge)
            {
                usedFace.skippedCorner = BlockAxisFace::TopRight;
                info = &kCornerWedgeLookupByOrientation[orient];
            }
            else
            {
                usedFace.skippedCorner = BlockAxisFace::BottomLeft;
                info = &kInverseCornerWedgeLookupByOrientation[orient];
            }

            Vector2int16 logicalCell = info->dot(iterator.getCurrentLocation());
            Vector2int16 highUvs[4], lowUvs[4];

            info->highRes->calculateTextureCoordinates(logicalCell, AtlasOffsetLookup[material - 1], highUvs);
            info->lowRes->calculateTextureCoordinates(logicalCell, AtlasOffsetLookup[material - 1], lowUvs);
            
            outputFace(output, center, offsetInfo, usedFace, highUvs, lowUvs, outlineMask);

            output += 4;
        }
        else
        {
            BlockAxisFace usedFace;
            usedFace.skippedCorner = BlockAxisFace::FullNoneSkipped;
            
			const RenderLookup<MaterialTextureCoordinates>* info;

            if (block == CELL_BLOCK_VerticalWedge)
            {
                info = &kVerticalWedgeLookupByOrientation[orient];
            }
            else
            {
                info = &kHorizontalWedgeLookupByOrientation[orient];
            }

            Vector2int16 logicalCell = info->dot(iterator.getCurrentLocation());
            Vector2int16 highUvs[4], lowUvs[4];

			info->highRes->calculateTextureCoordinates(logicalCell, AtlasOffsetLookup[material - 1], highUvs);
			info->lowRes->calculateTextureCoordinates(logicalCell, AtlasOffsetLookup[material - 1], lowUvs);
            
            outputFace(output, center, offsetInfo, usedFace, highUvs, lowUvs, outlineMask);

            output += 4;
        }
    }
};

static unsigned int getWaterTextureRotation(WaterCellDirection flowDirection, FaceDirection faceDirection)
{
    static unsigned int TRANSLATION[6][6] = // indexed by [flowDirection][faceDirection]
    {
        { 2, 1, 0, 3, 1, 1 }, // NegX
        { 0, 3, 2, 1, 3, 3 }, // X
        { 0, 0, 0, 0, 0, 2 }, // NegY
        { 2, 2, 2, 2, 2, 0 }, // Y
        { 3, 2, 1, 0, 2, 0 }, // NegZ
        { 1, 0, 3, 2, 0, 2 }  // Z
    };

    return TRANSLATION[flowDirection][faceDirection];
};

template<class VoxelStore, typename Vertex>
struct WaterFaceRenderer
{
    Vertex* output;

    void apply(const typename VoxelStore::Region::iterator& iterator, FaceDirection faceDirection, RenderPredStatus status)
	{
        Voxel::Cell cell = iterator.getCellAtCurrentLocation();
        Voxel::Cell waterCell = cell;
        Vector3int16 location(iterator.getCurrentLocation());
        Vector3int16 center = computeCenter(location);

        if (status == RENDER_BACKWARD)
        {
            cell = iterator.getNeighborCell(faceDirection);
            waterCell = cell;
            location += kFaceDirectionLocationOffset[faceDirection];
            center += kFaceDirectionLocationOffset[faceDirection] * kCELL_SIZE;
            faceDirection = kOppositeSideOffset[faceDirection];
        }

        static const Vector2int16 standardUvs[4] =
        {
            Vector2int16(0,0),
            Vector2int16(kAtlasPixelDimension,0),
            Vector2int16(kAtlasPixelDimension,kAtlasPixelDimension),
            Vector2int16(0,kAtlasPixelDimension)
        };

        const OFFSETINFOV2& adjustedOffsetInfo = kAxisSideLookup[faceDirection];

        BlockAxisFace usedFace = BlockAxisFace::inverse(GetOrientedFace(cell, faceDirection));

        outputFace(output, center, adjustedOffsetInfo, usedFace, standardUvs);
        output += 4;
    }

    inline void wedgeFace(const typename VoxelStore::Region::iterator& iterator) const
	{
	}
};

template<class Predicate, class ActingDelegate, class VoxelStore>
struct EdgeSpewV2
{
    const unsigned int maxApplies;
    Predicate predicate;	
    ActingDelegate actingDelegate;
    const VoxelStore* store;

    EdgeSpewV2(const unsigned int maxApplies)
		: maxApplies(maxApplies)
	{
	}

    void handleCells(const SpatialRegion::Id& chunkPos)
	{
        Region3int16 extents = SpatialRegion::inclusiveVoxelExtentsOfRegion(chunkPos);
        
        Vector3int16 minCell = extents.getMinPos();
        Vector3int16 maxCell = extents.getMaxPos();

        typename VoxelStore::Region region = store->getRegion(minCell, maxCell);
        typename VoxelStore::Region::iterator iterator = region.begin();

        if (region.isGuaranteedAllEmpty())
		{
            return;
        }

        const unsigned int maxFaces = maxApplies;
        const unsigned int safeDistance = 7; // max quads to be output per cell
        unsigned int faceCount = 0;
        bool reachedMax = (faceCount + safeDistance) >= maxFaces;

        while (iterator != region.end() && !reachedMax)
		{
            // Here and below, face count is incremented by
            // (RenderPredStatus + 1) >> 1
            // This is assuming the following values for RenderPredStatus:
            // 0 => No render (0 faces)
            // 1 => Forward   (1 faces)
            // 2 => Back      (1 faces)
            // 3 => Both      (2 faces)

            if (RenderPredStatus s = predicate.internal(iterator, PlusX))
			{
                actingDelegate.apply(iterator, PlusX, s);
                faceCount += (s + 1) >> 1;
            }

            if (RenderPredStatus s = predicate.internal(iterator, PlusZ))
			{
                actingDelegate.apply(iterator, PlusZ, s);
                faceCount += (s + 1) >> 1;
            }

            if (RenderPredStatus s = predicate.internal(iterator, PlusY))
			{
                actingDelegate.apply(iterator, PlusY, s);
                faceCount += (s + 1) >> 1;
            }

            if (predicate.wedgeFace(iterator.getCellAtCurrentLocation()))
			{
                actingDelegate.wedgeFace(iterator);
                faceCount++;
            }
            reachedMax = faceCount + safeDistance >= maxFaces;
            ++iterator;
        }			
    }
};

template<class Pred, class VoxelStore>
static unsigned int countQuads(const VoxelStore* store, const SpatialRegion::Id& chunkPos, int facesInIB)
{
    EdgeSpewV2<Pred, FaceCounter<VoxelStore>, VoxelStore> edgeSpew(facesInIB);
    edgeSpew.store = store;
    edgeSpew.handleCells(chunkPos);
    return edgeSpew.actingDelegate.count;
}

const Vector3int16 MegaCluster::kMinCellOffset(-2,-1,-2);

const std::string MegaCluster::kTerrainTexClose    = "rbxasset://textures/terrain/diffuse";
const std::string MegaCluster::kTerrainTexFar      = "rbxasset://textures/terrain/diffusefar";
const std::string MegaCluster::kTerrainTexNormals  = "rbxasset://textures/terrain/normal";
const std::string MegaCluster::kTerrainTexSpecular = "rbxasset://textures/terrain/specular";

MegaCluster::MegaCluster(VisualEngine* visualEngine, const boost::shared_ptr<PartInstance>& part)
    : visualEngine(visualEngine)
    , useShaders(false)
    , ignoreWaterUpdatesForTesting(false)
    , storage(NULL)
{
    RBXASSERT(part->getPartType() == MEGACLUSTER_PART);
    partInstance = part;

    RBXASSERT(partInstance->getGfxPart() == NULL);
    partInstance->setGfxPart(this);

    MegaClusterInstance* mci = boost::polymorphic_downcast<MegaClusterInstance*>(part.get());

    storage = mci->getVoxelGrid();

    FASTLOG2(FLog::TerrainCellListener, "MegaCluster: connecting %p to storage %p", this, storage);
    storage->connectListener(this);
    
    connections.push_back(part->ancestryChangedSignal.connect(boost::bind(&MegaCluster::zombify, this)));

    // Detect whether we will use shaders
    useShaders = visualEngine->getDevice()->getCaps().supportsShaders;

    initLookupTables();
}

MegaCluster::~MegaCluster()
{
    unbind();

    // notify scene updater about destruction so that the pointer to cluster is no longer stored
	visualEngine->getSceneUpdater()->notifyDestroyed(this);
}

void MegaCluster::setIgnoreWaterUpdatesForTesting(bool val)
{
    ignoreWaterUpdatesForTesting = val;
}

void MegaCluster::updateEntity(bool assetsUpdated)
{
	if (!storage)
	{
		delete this;
		return;
	}

    if (!storage->isAllocated())
        return;

    std::vector<SpatialRegion::Id> chunks = storage->getNonEmptyChunks();

    // We need to generate a chunk in -X/-Y/-Z direction for every original chunk
    // to make sure all faces are present
    typedef boost::unordered_set<SpatialRegion::Id, SpatialRegion::Id::boost_compatible_hash_value> ChunkSet;
    ChunkSet allChunks;

    for (size_t i = 0; i < chunks.size(); ++i)
    {
        SpatialRegion::Id id = chunks[i];

        allChunks.insert(id);
        allChunks.insert(id + Vector3int16(-1, 0, 0));
        allChunks.insert(id + Vector3int16(0, -1, 0));
        allChunks.insert(id + Vector3int16(0, 0, -1));
    }

    for (ChunkSet::const_iterator it = allChunks.begin(); it != allChunks.end(); ++it)
        updateChunkGeometry(*it, true, true);
}

void MegaCluster::unbind()
{
	GfxPart::unbind();

	if (storage)
	{
		FASTLOG2(FLog::TerrainCellListener, "MegaCluster: disconnecting %p from storage %p", this, storage);
		storage->disconnectListener(this);

		storage = NULL;
	}
}

void MegaCluster::invalidateEntity()
{
	visualEngine->getSceneUpdater()->queueFullInvalidateMegaCluster(this);
}

void MegaCluster::terrainCellChanged(const CellChangeInfo& info)
{
    bool waterChanged = !ignoreWaterUpdatesForTesting;
    CellBlock beforeBlock = info.beforeCell.solid.getBlock();
    CellBlock afterBlock = info.afterCell.solid.getBlock();
    bool terrainChanged = beforeBlock != CELL_BLOCK_Empty || afterBlock != CELL_BLOCK_Empty;

    Vector3int16 pos(info.position);
    
    // mark edit chunk dirty
    SpatialRegion::Id chunk = SpatialRegion::regionContainingVoxel(pos);

    markDirty(chunk, terrainChanged, waterChanged);

    if (SpatialRegion::regionContainingVoxel(pos - Vector3int16(1, 1, 1)) != SpatialRegion::regionContainingVoxel(pos + Vector3int16(1, 1, 1)))
    {
        // edit cell is at the chunk boundary, need to update neighbors
        static const Vector3int16 kSmallXOffset(-1, 0, 0);
        static const Vector3int16 kSmallYOffset(0, -1, 0);
        static const Vector3int16 kSmallZOffset(0, 0, -1);
        static const Vector3int16 kBigXOffset(1, 0, 0);
        static const Vector3int16 kBigYOffset(0, 1, 0);
        static const Vector3int16 kBigZOffset(0, 0, 1);

        Vector3int16 neighborTerrainChunks[3];
        unsigned int usedNeighborTerrainChunks = 0;

        if (chunk != SpatialRegion::regionContainingVoxel(pos + kSmallXOffset)) {
            neighborTerrainChunks[usedNeighborTerrainChunks++] = kSmallXOffset;
        }
        if (chunk != SpatialRegion::regionContainingVoxel(pos + kBigXOffset)) {
            neighborTerrainChunks[usedNeighborTerrainChunks++] = kBigXOffset;
        }
        if (chunk != SpatialRegion::regionContainingVoxel(pos + kSmallYOffset)) {
            neighborTerrainChunks[usedNeighborTerrainChunks++] = kSmallYOffset;
        }
        if (chunk != SpatialRegion::regionContainingVoxel(pos + kBigYOffset)) {
            neighborTerrainChunks[usedNeighborTerrainChunks++] = kBigYOffset;
        }
        if (chunk != SpatialRegion::regionContainingVoxel(pos + kSmallZOffset)) {
            neighborTerrainChunks[usedNeighborTerrainChunks++] = kSmallZOffset;
        }
        if (chunk != SpatialRegion::regionContainingVoxel(pos + kBigZOffset)) {
            neighborTerrainChunks[usedNeighborTerrainChunks++] = kBigZOffset;
        }
        
        RBXASSERT(usedNeighborTerrainChunks <= 3);

        // mark neighbor chunks dirty
        for (unsigned int firstDim = 0; firstDim < usedNeighborTerrainChunks; ++firstDim) {
            markDirty(chunk + neighborTerrainChunks[firstDim], terrainChanged, waterChanged);
            for (unsigned int secondDim = firstDim + 1; secondDim < usedNeighborTerrainChunks; ++secondDim) {
                markDirty(chunk + neighborTerrainChunks[firstDim] + neighborTerrainChunks[secondDim], terrainChanged, waterChanged);
                for (unsigned int thirdDim = secondDim + 1; thirdDim < usedNeighborTerrainChunks; ++thirdDim) {
                    markDirty(chunk + neighborTerrainChunks[firstDim] + neighborTerrainChunks[secondDim] + neighborTerrainChunks[thirdDim], terrainChanged, waterChanged);
                }
            }
        }
    }
}

void MegaCluster::updateChunk(const SpatialRegion::Id& pos, bool isWaterChunk)
{
    updateChunkGeometry(pos, !isWaterChunk, isWaterChunk);
}

void MegaCluster::markDirty(const SpatialRegion::Id& pos, bool solidDirty, bool waterDirty)
{
    ChunkData& chunk = chunks.insert(pos);

    FASTLOG5(FLog::MegaClusterDirty, "MegaCluster: Marking chunk %dx%dx%d dirty (solid %d, water %d)", pos.value().x, pos.value().y, pos.value().z, solidDirty, waterDirty);
    
    SceneUpdater* sceneUpdater = visualEngine->getSceneUpdater();
    
    if (waterDirty && !chunk.waterDirty) {
        chunk.waterDirty = true;
        sceneUpdater->queueChunkInvalidateMegaCluster(this, pos, true);
    }

    if (solidDirty && !chunk.solidDirty) {
        chunk.solidDirty = true;
        sceneUpdater->queueChunkInvalidateMegaCluster(this, pos, false);
    }
}

void MegaCluster::updateChunkGeometry(const SpatialRegion::Id& pos, bool solidUpdate, bool waterUpdate)
{
    ChunkData& chunk = chunks.insert(pos);
    
    // Prepare render area: copy chunk with fringe from storage
    const Region3int16 extents = SpatialRegion::inclusiveVoxelExtentsOfRegion(pos);
    renderArea.loadData(storage, extents.getMinPos() + MegaCluster::kMinCellOffset);
    
    // Update solid entity
    if (solidUpdate)
    {
        if (chunk.solidEntity)
        {
            chunk.node->removeEntity(chunk.solidEntity);
            delete chunk.solidEntity;
        }
        
        chunk.solidEntity = createSolidGeometry(pos, &chunk.solidQuads);
        chunk.solidDirty = false;
        
        if (chunk.solidEntity)
            chunk.node->addEntity(chunk.solidEntity);
    }
    
    // Update water entity
    if (waterUpdate)
    {
        if (chunk.waterEntity)
        {
            chunk.node->removeEntity(chunk.waterEntity);
            delete chunk.waterEntity;
        }
        
        chunk.waterEntity = createWaterGeometry(pos, &chunk.waterQuads);
        chunk.waterDirty = false;
        
        if (chunk.waterEntity)
            chunk.node->addEntity(chunk.waterEntity);
    }
    
    // Update quad stats or destroy node if necessary
    if (!chunk.solidEntity && !chunk.waterEntity)
    {
        chunk = ChunkData();
    }
    else
    {
        chunk.node->setBlockCount((chunk.solidQuads + chunk.waterQuads) / ChunkData::kApproximateQuadsPerPart);
    }
}

void MegaCluster::generateAndReturnWaterGeometry(Voxel::Grid* voxelGrid, const SpatialRegion::Id& pos, std::vector<TerrainVertexFFP>* verticesOut)
{
    static const unsigned facesInIB = kMaxIndexBufferSize32Bit / 6;

    // Prepare render area: copy chunk with fringe from storage
    const Region3int16 extents = SpatialRegion::inclusiveVoxelExtentsOfRegion(pos);
    RenderArea renderArea;
    renderArea.loadData(voxelGrid, extents.getMinPos() + MegaCluster::kMinCellOffset);

    unsigned int quads =  countQuads<WaterRenderPredicate<RenderArea>, RenderArea>(&renderArea, pos, facesInIB); 
    if (quads == 0) return;

    verticesOut->resize(quads*4);

    EdgeSpewV2<WaterRenderPredicate<RenderArea>, WaterFaceRenderer<RenderArea, TerrainVertexFFP>, RenderArea> spew(facesInIB);
    spew.actingDelegate.output = &(*verticesOut)[0];
    spew.store = &renderArea;
    spew.handleCells(pos);
}

void MegaCluster::generateAndReturnSolidGeometry(Voxel::Grid* voxelGrid, const SpatialRegion::Id& pos, std::vector<TerrainVertexFFP>* verticesOut)
{
    static const unsigned facesInIB = kMaxIndexBufferSize32Bit / 6;

    // Prepare render area: copy chunk with fringe from storage
    const Region3int16 extents = SpatialRegion::inclusiveVoxelExtentsOfRegion(pos);
    RenderArea renderArea;
    renderArea.loadData(voxelGrid, extents.getMinPos() + MegaCluster::kMinCellOffset);

    unsigned int quads = countQuads<SolidTerrainRenderPredicate<RenderArea>, RenderArea>(&renderArea, pos, facesInIB);

    if (quads == 0) return;

    verticesOut->resize(quads*4);

    EdgeSpewV2<SolidTerrainRenderPredicate<RenderArea>, SolidTerrainRenderer<RenderArea, TerrainVertexFFP>, RenderArea> spew(facesInIB);
    spew.actingDelegate.output = &(*verticesOut)[0];
    spew.store = &renderArea;	
    spew.handleCells(pos);
}

RenderNode* MegaCluster::updateChunkNode(const SpatialRegion::Id& pos)
{
    ChunkData& chunk = chunks.insert(pos);
    
    // Update chunk node
    if (!chunk.node)
    {
        // Create chunk node
        chunk.node.reset(new RenderNode(visualEngine, RenderNode::CullMode_SpatialHash));
        
        Vector3int32 minLocation = SpatialRegion::smallestCornerOfRegionInGlobalCoordStuds(pos);
        Vector3int32 maxLocation = SpatialRegion::largestCornerOfRegionInGlobalCoordStuds(pos);

        chunk.node->setCoordinateFrame(CoordinateFrame(minLocation.toVector3()));
        chunk.node->updateWorldBounds(Extents(minLocation.toVector3(), maxLocation.toVector3()));
    }

    return chunk.node.get();
}

RenderEntity* MegaCluster::createSolidGeometry(const SpatialRegion::Id& pos, unsigned int* outQuads)
{
    const shared_ptr<IndexBuffer>& ibuf = getSharedIB();
    unsigned int facesInIB = ibuf->getElementCount() / 6;
    
    unsigned int quads = countQuads<SolidTerrainRenderPredicate<RenderArea>, RenderArea>(&renderArea, pos, facesInIB);
    
    *outQuads = quads;
    
    if (quads == 0) return NULL;

    shared_ptr<VertexBuffer> vbuf;

    if (useShaders)
    {
        vbuf = visualEngine->getDevice()->createVertexBuffer(sizeof(TerrainVertex), quads*4, GeometryBuffer::Usage_Static);

        TerrainVertex* vbptr = static_cast<TerrainVertex*>(vbuf->lock());
        
        EdgeSpewV2<SolidTerrainRenderPredicate<RenderArea>, SolidTerrainRenderer<RenderArea, TerrainVertex>, RenderArea> spew(facesInIB);
        spew.actingDelegate.output = vbptr;
        spew.store = &renderArea;
        spew.handleCells(pos);

        vbuf->unlock();
    }
    else
    {
        vbuf = visualEngine->getDevice()->createVertexBuffer(sizeof(TerrainVertexFFP), quads*4, GeometryBuffer::Usage_Static);

        TerrainVertexFFP* vbptr = static_cast<TerrainVertexFFP*>(vbuf->lock());
        
        EdgeSpewV2<SolidTerrainRenderPredicate<RenderArea>, SolidTerrainRenderer<RenderArea, TerrainVertexFFP>, RenderArea> spew(facesInIB);
        spew.actingDelegate.output = vbptr;
        spew.store = &renderArea;
        spew.handleCells(pos);

        vbuf->unlock();
    }
    
    return createGeometry(updateChunkNode(pos), vbuf, ibuf, getSolidMaterial(), RenderQueue::Id_Opaque, false);
}

RenderEntity* MegaCluster::createWaterGeometry(const SpatialRegion::Id& pos, unsigned int* outQuads)
{
    const shared_ptr<IndexBuffer>& ibuf = getSharedIB();
    unsigned int facesInIB = ibuf->getElementCount() / 6;
    
    unsigned int quads = countQuads<WaterRenderPredicate<RenderArea>, RenderArea>(&renderArea, pos, facesInIB);

    *outQuads = quads;

    if (quads == 0) return NULL;

    shared_ptr<VertexBuffer> vbuf;
    
    if (useShaders)
    {
        vbuf = visualEngine->getDevice()->createVertexBuffer(sizeof(WaterVertex), quads*4, GeometryBuffer::Usage_Static);

        WaterVertex* vbptr = static_cast<WaterVertex*>(vbuf->lock());
        
        EdgeSpewV2<WaterRenderPredicate<RenderArea>, WaterFaceRenderer<RenderArea, WaterVertex>, RenderArea> edgeSpew(facesInIB);
        edgeSpew.actingDelegate.output = vbptr;
        edgeSpew.store = &renderArea;
        edgeSpew.handleCells(pos);

        vbuf->unlock();
    }
    else
    {
        vbuf = visualEngine->getDevice()->createVertexBuffer(sizeof(TerrainVertexFFP), quads*4, GeometryBuffer::Usage_Static);

        TerrainVertexFFP* vbptr = static_cast<TerrainVertexFFP*>(vbuf->lock());
        
        EdgeSpewV2<WaterRenderPredicate<RenderArea>, WaterFaceRenderer<RenderArea, TerrainVertexFFP>, RenderArea> edgeSpew(facesInIB);
        edgeSpew.actingDelegate.output = vbptr;
        edgeSpew.store = &renderArea;
        edgeSpew.handleCells(pos);

        vbuf->unlock();
    }

	shared_ptr<Material> material = visualEngine->getWater()->getLegacyMaterial();
    
    return createGeometry(updateChunkNode(pos), vbuf, ibuf, material, RenderQueue::Id_TransparentUnsorted, true);
}

RenderEntity* MegaCluster::createGeometry(RenderNode* node, const shared_ptr<VertexBuffer>& vbuf, const shared_ptr<IndexBuffer>& ibuf, const shared_ptr<Material>& material, RenderQueue::Id renderQueueId, bool isWater)
{
    unsigned int quadCount = vbuf->getElementCount() / 4;

    shared_ptr<Geometry> geometry = visualEngine->getDevice()->createGeometry(getVertexLayout(isWater), vbuf, ibuf);

    return new RenderEntity(node, GeometryBatch(geometry, Geometry::Primitive_Triangles, quadCount * 6, quadCount * 4), material, renderQueueId);
}

const shared_ptr<VertexLayout>& MegaCluster::getVertexLayout(bool isWater)
{
    shared_ptr<VertexLayout>& vertexLayout = isWater ? vertexLayouts[1] : vertexLayouts[0];

    if (!vertexLayout)
    {
        std::vector<VertexLayout::Element> elements;

        if (useShaders)
        {
            if (isWater)
            {
                elements.push_back(VertexLayout::Element(0, offsetof(WaterVertex, pos), VertexLayout::Format_UByte4, VertexLayout::Semantic_Position));
                elements.push_back(VertexLayout::Element(0, offsetof(WaterVertex, normal), VertexLayout::Format_UByte4, VertexLayout::Semantic_Normal));
            }
            else
            {
                elements.push_back(VertexLayout::Element(0, offsetof(TerrainVertex, pos), VertexLayout::Format_UByte4, VertexLayout::Semantic_Position));
                elements.push_back(VertexLayout::Element(0, offsetof(TerrainVertex, normal), VertexLayout::Format_UByte4, VertexLayout::Semantic_Normal));
                elements.push_back(VertexLayout::Element(0, offsetof(TerrainVertex, uv), VertexLayout::Format_Short4, VertexLayout::Semantic_Texture, 0));
                elements.push_back(VertexLayout::Element(0, offsetof(TerrainVertex, edgeDistances), VertexLayout::Format_UByte4, VertexLayout::Semantic_Texture, 1));
                elements.push_back(VertexLayout::Element(0, offsetof(TerrainVertex, tangent), VertexLayout::Format_UByte4, VertexLayout::Semantic_Texture, 2));
            }
        }
        else
        {
            elements.push_back(VertexLayout::Element(0, offsetof(TerrainVertexFFP, pos), VertexLayout::Format_Float3, VertexLayout::Semantic_Position));
            elements.push_back(VertexLayout::Element(0, offsetof(TerrainVertexFFP, normal), VertexLayout::Format_Float3, VertexLayout::Semantic_Normal));
            elements.push_back(VertexLayout::Element(0, offsetof(TerrainVertexFFP, uv), VertexLayout::Format_Float2, VertexLayout::Semantic_Texture, 0));
        }

        vertexLayout = visualEngine->getDevice()->createVertexLayout(elements);
        RBXASSERT(vertexLayout);
    }
    
    return vertexLayout;
}

template <typename T> static shared_ptr<IndexBuffer> generateQuadIB(Device* device, unsigned int quads)
{
    shared_ptr<IndexBuffer> ibuf = device->createIndexBuffer(sizeof(T), quads * 6, GeometryBuffer::Usage_Static);

    T* indices = static_cast<T*>(ibuf->lock());

    for (unsigned int i = 0; i < quads; i++)
    {
        // assuming CCW
        // 1 - 0
        // | \ |
        // 2 - 3
        *(indices++) = i*4 + 0;
        *(indices++) = i*4 + 1;
        *(indices++) = i*4 + 3;

        *(indices++) = i*4 + 1;
        *(indices++) = i*4 + 2;
        *(indices++) = i*4 + 3;
    }

    ibuf->unlock();

    return ibuf;
}

const shared_ptr<IndexBuffer>& MegaCluster::getSharedIB()
{
    if (sharedIB)
        return sharedIB;

    if (visualEngine->getDevice()->getCaps().supportsIndex32)
    {
        sharedIB = generateQuadIB<unsigned int>(visualEngine->getDevice(), kMaxIndexBufferSize32Bit);
    }
    else
    {
        sharedIB = generateQuadIB<unsigned short>(visualEngine->getDevice(), kMaxIndexBufferSize16Bit);
    }

    return sharedIB;
}

#ifdef RBX_PLATFORM_IOS
static const std::string kTextureExtension = ".pvr";
#elif defined(__ANDROID__)
static const std::string kTextureExtension = ".pvr";
#else
static const std::string kTextureExtension = ".dds";
#endif

static void setupTextures(VisualEngine* visualEngine, Technique& technique)
{
    TextureManager* tm = visualEngine->getTextureManager();
    LightGrid* lightGrid = visualEngine->getLightGrid();
	SceneManager* sceneManager = visualEngine->getSceneManager();

    technique.setTexture(0, tm->load(ContentId(MegaCluster::kTerrainTexClose + kTextureExtension), TextureManager::Fallback_White), SamplerState::Filter_Linear);
    technique.setTexture(1, tm->load(ContentId(MegaCluster::kTerrainTexFar + kTextureExtension), TextureManager::Fallback_White), SamplerState::Filter_Linear);
    technique.setTexture(2, tm->load(ContentId(MegaCluster::kTerrainTexNormals + kTextureExtension), TextureManager::Fallback_NormalMap), SamplerState::Filter_Linear);
    technique.setTexture(3, tm->load(ContentId(MegaCluster::kTerrainTexSpecular + kTextureExtension), TextureManager::Fallback_Black), SamplerState::Filter_Linear);

    if (lightGrid)
    {
        technique.setTexture(4, lightGrid->getTexture(), SamplerState::Filter_Linear);
		technique.setTexture(5, lightGrid->getLookupTexture(), SamplerState(SamplerState::Filter_Point, SamplerState::Address_Clamp));
    }

	technique.setTexture(6, sceneManager->getShadowMap(), SamplerState(SamplerState::Filter_Linear, SamplerState::Address_Clamp));
}

const shared_ptr<Material>& MegaCluster::getSolidMaterial()
{
    if (solidMaterial)
        return solidMaterial;

    solidMaterial = shared_ptr<Material>(new Material());

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("MegaClusterHQVS", "MegaClusterHQGBufferFS"))
    {
        Technique technique(program, 0);

		setupTextures(visualEngine, technique);

        solidMaterial->addTechnique(technique);
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgram("MegaClusterHQVS", "MegaClusterHQFS"))
    {
        Technique technique(program, 1);

		setupTextures(visualEngine, technique);

        solidMaterial->addTechnique(technique);
    }

    if (shared_ptr<ShaderProgram> program = visualEngine->getShaderManager()->getProgramOrFFP("MegaClusterVS", "MegaClusterFS"))
    {
        Technique technique(program, 2);

		setupTextures(visualEngine, technique);

        solidMaterial->addTechnique(technique);
    }

    return solidMaterial;
}

}
}
