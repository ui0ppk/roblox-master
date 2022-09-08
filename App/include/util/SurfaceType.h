/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#pragma once

namespace RBX {

	// TODO - Joint.cpp uses this ordering - fix up as a class

	typedef enum {	NO_SURFACE = 0,
					GLUE,
					WELD,
					STUDS,
					INLET,
					UNIVERSAL,
					ROTATE,			// special ordering here
					ROTATE_V,
					ROTATE_P,
					NO_JOIN, // new surface type for specifically preventing joining (via ManualWeldHelper)
					NO_SURFACE_NO_OUTLINES,	 // identical to NO_SURFACE, but removes outlines
					NUM_SURF_TYPES} SurfaceType;

	inline bool IsNoSurface(SurfaceType surface)
	{
		return surface == NO_SURFACE || surface == NO_SURFACE_NO_OUTLINES;
	}

	inline bool IsRotate(SurfaceType surface)
	{
		return surface >= ROTATE && surface <= ROTATE_P;
	}

	namespace Legacy {
		// TODO: improve precedence logic
		// LEGACY from when this was separate stuff
		typedef enum {	NO_CONSTRAINT = 0,
						ROTATE_LEGACY, 
						ROTATE_P_LEGACY, 
						ROTATE_V_LEGACY, 
						NUM_CONSTRAINT_TYPES} SurfaceConstraint;

	}
} // namespace

