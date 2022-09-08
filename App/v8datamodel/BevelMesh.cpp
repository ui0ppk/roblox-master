/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/BevelMesh.h"

using namespace RBX;

const char* const RBX::sBevelMesh = "BevelMesh";

REFLECTION_BEGIN();
static Reflection::PropDescriptor<BevelMesh, float> desc_bevel("Bevel", category_Data, &BevelMesh::getBevel, &BevelMesh::setBevel, RBX::Reflection::PropertyDescriptor::STREAMING);
static Reflection::PropDescriptor<BevelMesh, float> desc_roundness("Bevel Roundness", category_Data, &BevelMesh::getRoundness, &BevelMesh::setRoundness, RBX::Reflection::PropertyDescriptor::STREAMING);
static Reflection::PropDescriptor<BevelMesh, float> desc_bulge("Bulge", category_Data, &BevelMesh::getBulge, &BevelMesh::setBulge, RBX::Reflection::PropertyDescriptor::STREAMING);
REFLECTION_END();

BevelMesh::BevelMesh()
	: bevel(0.0)
	, roundness(0.0)
	, bulge(0.0)
{}

const float BevelMesh::getBevel() const
{
	return bevel;
}

void BevelMesh::setBevel(const float bev)
{ 
	bevel = bev; 
	raisePropertyChanged(desc_bevel);
}

const float BevelMesh::getRoundness() const
{
	return roundness;
}
void BevelMesh::setRoundness(const float rnd)
{
	roundness = rnd;
	raisePropertyChanged(desc_roundness);
}

const float BevelMesh::getBulge() const
{
	return bulge;
}

void BevelMesh::setBulge(const float blg)
{
	bulge = blg;
	raisePropertyChanged(desc_bulge);
}