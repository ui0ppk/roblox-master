/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/SurfaceType.h"
#include "Util/Vector6.h"
#include "G3D/Vector3.h"
#include "G3D/Color4.h"
#include "G3D/CoordinateFrame.h"

// Simple description of a part suitable for drawing, etc.  Build Instance on top of this.
// Low level.

namespace RBX {

	enum PartType {		BALL_PART = 0, 
						BLOCK_PART, 
						CYLINDER_PART,
						TRUSS_PART,
						WEDGE_PART,
						PRISM_PART,
						PYRAMID_PART,
						PARALLELRAMP_PART,
						RIGHTANGLERAMP_PART,
						CORNERWEDGE_PART,
						MEGACLUSTER_PART,
						OPERATION_PART };

class Part {
public:
	// alpha order for simplification on dialogs

	PartType						type;			// hash code hashes this block of data
	G3D::Vector3					gridSize;	
	G3D::Color4						color;
	Vector6<SurfaceType>			surfaceType;
    G3D::CoordinateFrame			coordinateFrame;

	Part() {}

	Part(PartType _type, 
			const G3D::Vector3& _gridSize, 
			const G3D::Color4 _color, 
	        const G3D::CoordinateFrame& c) : 
		type(_type),
		gridSize(_gridSize),
		color(_color),
		surfaceType(NO_SURFACE), 
        coordinateFrame(c)
	{}

	Part(PartType type, 
			const G3D::Vector3& gridSize, 
			const G3D::Color4 color,
			const Vector6<SurfaceType>& surfaceType, 
            const G3D::CoordinateFrame& c) :
		type(type),
		gridSize(gridSize),
		color(color),
		surfaceType(surfaceType), 
        coordinateFrame(c)
	{}

};

} // namespace

