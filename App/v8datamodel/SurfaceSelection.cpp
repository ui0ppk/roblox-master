/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/SurfaceSelection.h"
#include "V8DataModel/PartInstance.h"
#include "AppDraw/DrawAdorn.h"
#include "AppDraw/Draw.h"

namespace RBX {

const char* const sSurfaceSelection = "SurfaceSelection";

REFLECTION_BEGIN();
static const Reflection::EnumPropDescriptor<SurfaceSelection, NormalId>	prop_Surface("TargetSurface", category_Data, &SurfaceSelection::getSurface, &SurfaceSelection::setSurface);
REFLECTION_END();

SurfaceSelection::SurfaceSelection()
	:DescribedCreatable<SurfaceSelection, PartAdornment, sSurfaceSelection>("SurfaceSelection")
	,surfaceId(NORM_X)
{}

void SurfaceSelection::setSurface(NormalId value)
{
	if(surfaceId != value){
		surfaceId = value;
		raisePropertyChanged(prop_Surface);
	}
}

void SurfaceSelection::render3dAdorn(Adorn* adorn)
{
	if(getVisible()){
		if(shared_ptr<RBX::PartInstance> partInstance = adornee.lock()){
			DrawAdorn::partSurface(partInstance->getPart(), surfaceId, adorn, color);
		}
	}
}

}

// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag0 = 0;
    };
};
