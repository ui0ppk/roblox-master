#include "VoxelCellUtilV1Deprecated.h"

namespace RBX { namespace V1Deprecated {

const BlockFaceInfo UnOrientedBlockFaceInfos[6] = { 
		// Solid
		{
			{
				{ BlockAxisFace::FullNoneSkipped}, // PlusX
				{ BlockAxisFace::FullNoneSkipped}, // PlusZ
				{ BlockAxisFace::FullNoneSkipped}, // MinusX
				{ BlockAxisFace::FullNoneSkipped}, // MinusZ
				{ BlockAxisFace::FullNoneSkipped}, // PlusY
				{ BlockAxisFace::FullNoneSkipped}, // MinusY
			}
		},

		// Vertical Wedge
		{
			{
				{ BlockAxisFace::TopLeft,       }, // PlusX
				{ BlockAxisFace::EmptyAllSkipped}, // PlusZ
				{ BlockAxisFace::TopRight,      }, // MinusX
				{ BlockAxisFace::FullNoneSkipped}, // MinusZ
				{ BlockAxisFace::EmptyAllSkipped}, // PlusY
				{ BlockAxisFace::FullNoneSkipped}, // MinusY
			}
		},

		// Corner Wedge
		{
			{
				{ BlockAxisFace::TopLeft,       }, // PlusX
				{ BlockAxisFace::EmptyAllSkipped}, // PlusZ
				{ BlockAxisFace::EmptyAllSkipped}, // MinusX
				{ BlockAxisFace::TopRight,      }, // MinusZ
				{ BlockAxisFace::EmptyAllSkipped}, // PlusY
				{ BlockAxisFace::TopLeft,       }, // MinusY
			}
		},

		// Inverse corner Wedge
		{
			{
				{ BlockAxisFace::FullNoneSkipped}, // PlusX
				{ BlockAxisFace::TopLeft,       }, // PlusZ
				{ BlockAxisFace::TopRight,      }, // MinusX
				{ BlockAxisFace::FullNoneSkipped}, // MinusZ
				{ BlockAxisFace::BottomLeft,    }, // PlusY
				{ BlockAxisFace::FullNoneSkipped}, // MinusY
			}
		},

		// Horizontal Wedge
		{
			{
				{ BlockAxisFace::FullNoneSkipped}, // PlusX
				{ BlockAxisFace::EmptyAllSkipped}, // PlusZ
				{ BlockAxisFace::EmptyAllSkipped}, // MinusX
				{ BlockAxisFace::FullNoneSkipped}, // MinusZ
				{ BlockAxisFace::BottomLeft,    }, // PlusY
				{ BlockAxisFace::TopLeft,       }, // MinusY
			}
		},

		// Empty Wedge
		{
			{
				{ BlockAxisFace::EmptyAllSkipped }, // PlusX
				{ BlockAxisFace::EmptyAllSkipped }, // PlusZ
				{ BlockAxisFace::EmptyAllSkipped }, // MinusX
				{ BlockAxisFace::EmptyAllSkipped }, // MinusZ
				{ BlockAxisFace::EmptyAllSkipped }, // PlusY
				{ BlockAxisFace::EmptyAllSkipped }, // MinusY
			}
		}
	};

BlockAxisFace OrientedFaceMap[ 1536 ]; // 2^8 * 6

BlockAxisFace ComputeOrientedFace(const BlockFaceInfo& wedge, FaceDirection f,
	CellOrientation orient)
{
	if(f < 4)
	{
		unsigned char index = (f + orient) % 4;

		return wedge.faces[index];
	}

	const BlockAxisFace& basis = wedge.faces[f];
	if (basis.skippedCorner == BlockAxisFace::FullNoneSkipped ||
			basis.skippedCorner == BlockAxisFace::EmptyAllSkipped) {
		return basis;
	}

	BlockAxisFace result;
	// minus Y is the same as plus y, except it is wound backwards.
	// encode this by reversing the orient iteration order for minus y:
	// 0 1 2 3 => 0 3 2 1
	result.skippedCorner = BlockAxisFace::rotate(basis.skippedCorner,
		(CellOrientation)(f == MinusY ? (4 - orient) % 4 : orient));

	return result;
}

void initBlockOrientationFaceMap()
{
	// max materials = 8 to represent that 3 legacy bits were used for material
	static int kLEGACY_MAX_MATERIALS = 8;
	static int kNUM_WEDGES = 6;

	for( int i = 0; i < MAX_CELL_BLOCKS; ++i )
	{
		const BlockFaceInfo& wedge = i >= kNUM_WEDGES ?
			UnOrientedBlockFaceInfos[0] : UnOrientedBlockFaceInfos[i];

		for( int j = 0; j < kLEGACY_MAX_MATERIALS; ++j )
		{
			for( int k = 0; k < MAX_CELL_ORIENTATIONS; ++k )
			{
				for( int l = 0; l < 6; ++l ) // # of face directions
				{	
					unsigned char mapIndex = 0;

					setCellBlock( mapIndex, (CellBlock)i );
					setCellMaterial_Deprecated( mapIndex, (CellMaterial)j );
					setCellOrientation( mapIndex, (CellOrientation)k );

					OrientedFaceMap[ mapIndex*6 + l ] = 
						ComputeOrientedFace(wedge, (FaceDirection)l, (CellOrientation)k);
				}
			}
		}
	}
}

}
}
