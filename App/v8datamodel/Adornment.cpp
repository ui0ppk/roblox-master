/* Copyright 2003-2009 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Adornment.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/PVInstance.h"

namespace RBX {

const char* const sPartAdornment = "PartAdornment";

const Reflection::RefPropDescriptor<PartAdornment, PartInstance>	prop_partAdornee("Adornee", category_Data, &PartAdornment::getAdorneeDangerous, &PartAdornment::setAdornee);

PartAdornment::PartAdornment(const char* name)
	:DescribedNonCreatable<PartAdornment, GuiBase3d, sPartAdornment>(name)
{}
void PartAdornment::setAdornee(PartInstance* value)
{
	if(adornee.lock().get() != value){
		adornee = shared_from(value);
		raisePropertyChanged(prop_partAdornee);
	}
}



const char* const  sPVAdornment = "PVAdornment";

const Reflection::RefPropDescriptor<PVAdornment, PVInstance> prop_pvAdornee("Adornee", category_Data, &PVAdornment::getAdorneeDangerous, &PVAdornment::setAdornee);

PVAdornment::PVAdornment(const char* name)
	:DescribedNonCreatable<PVAdornment, GuiBase3d, sPVAdornment>(name)
{}
void PVAdornment::setAdornee(PVInstance* value)
{
	if(adornee.lock().get() != value){
		adornee = shared_from(value);
		raisePropertyChanged(prop_pvAdornee);
	}
}

}
