/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#include "GfxBase/IAdornableCollector.h"
#include "v8datamodel/Camera.h"
#include "rbx/Debug.h"
#include "FastLog.h"

LOGGROUP(AdornableLifetime);
DYNAMIC_FASTFLAGVARIABLE(DontReorderScreenGuisWhenDescendantRemoving, false)

namespace RBX {


IAdornable::~IAdornable()
{
	FASTLOG1(FLog::AdornableLifetime, "Destroying adornable %p", this);
	
	if (bucket)
		bucket->onRenderableDescendantRemoving(this);
}

void IAdornable::shouldRenderSetDirty()
{
	if (bucket) {
		bucket->recomputeShouldRender(this);
	}
}

float IAdornable::calculateDepth(const Camera* camera) const
{
	return camera->dot(render3dSortedPosition());
}

//////////////////////////////////////////////////////////////

IAdornableCollector::~IAdornableCollector()
{
	FASTLOG1(FLog::AdornableLifetime, "Adornable Collector %p deleted", this);
	FASTLOG3(FLog::AdornableLifetime, "Renderables 2D: %u 3D: %u 3DSorted: %u", renderable2ds.size(), renderable3ds.size(), renderable3dSorteds.size());

	RBXASSERT(renderable2ds.size() == 0);
	RBXASSERT(renderable3ds.size() == 0);
	RBXASSERT(renderable3dSorteds.size() == 0);
}

void IAdornableCollector::recomputeShouldRender(IAdornable* iR)
{
	RBXASSERT(iR->bucket == this);

	if (iR->shouldRender2d()) {
		if (!renderable2ds.fastContains(iR)) {
			FASTLOG2(FLog::AdornableLifetime, "Collector %p: Adding 2D adorn %p", this, iR);
			renderable2ds.fastAppend(iR);
		}
	}
	else {
		if (renderable2ds.fastContains(iR)) {
			FASTLOG2(FLog::AdornableLifetime, "Collector %p: Removing 2D adorn %p", this, iR);
			renderable2ds.fastRemove(iR);
		}
	}

	if (iR->shouldRender3dAdorn()) {
		if (!renderable3ds.fastContains(iR)) {
			FASTLOG2(FLog::AdornableLifetime, "Collector %p: Adding 3D adorn %p", this, iR);
			renderable3ds.fastAppend(iR);
		}
	}
	else {
		if (renderable3ds.fastContains(iR)) {
			FASTLOG2(FLog::AdornableLifetime, "Collector %p: Removing 3D adorn %p", this, iR);
			renderable3ds.fastRemove(iR);
		}
	}

	if (iR->shouldRender3dSortedAdorn()) {
		if (!renderable3dSorteds.fastContains(iR)) {
			FASTLOG2(FLog::AdornableLifetime, "Collector %p: Adding 3DSort adorn %p", this, iR);
			renderable3dSorteds.fastAppend(iR);
		}
	}
	else {
		if (renderable3dSorteds.fastContains(iR)) {
			FASTLOG2(FLog::AdornableLifetime, "Collector %p: Removing 3DSort adorn %p", this, iR);
			renderable3dSorteds.fastRemove(iR);
		}
	}
}



void IAdornableCollector::onRenderableDescendantAdded(IAdornable* iR)
{
	RBXASSERT(iR->index2d == -1);
	RBXASSERT(iR->index3d == -1);
	RBXASSERT(iR->index3dSorted == -1);
	RBXASSERT(iR->bucket == NULL);

	iR->bucket = this;

	recomputeShouldRender(iR);
}


void IAdornableCollector::onRenderableDescendantRemoving(IAdornable* iR)
{
	RBXASSERT(iR->bucket == this);

	if (renderable2ds.fastContains(iR)) {
		FASTLOG2(FLog::AdornableLifetime, "Collector %p: Removing 2D adorn %p", this, iR);
		if (DFFlag::DontReorderScreenGuisWhenDescendantRemoving)
		{
			renderable2ds.remove(iR);
		}
		else
		{
			renderable2ds.fastRemove(iR);
		}
	}

	if (renderable3ds.fastContains(iR)) {
		FASTLOG2(FLog::AdornableLifetime, "Collector %p: Removing 3D adorn %p", this, iR);
		renderable3ds.fastRemove(iR);
	}

	if (renderable3dSorteds.fastContains(iR)) {
		FASTLOG2(FLog::AdornableLifetime, "Collector %p: Removing 3DSort adorn %p", this, iR);
		renderable3dSorteds.fastRemove(iR);
	}

	iR->bucket = NULL;

	RBXASSERT(iR->index2d == -1);
	RBXASSERT(iR->index3d == -1);
	RBXASSERT(iR->index3dSorted == -1);
}


void IAdornableCollector::render2dItems(Adorn* adorn)
{
	FASTLOG1(FLog::AdornRenderStats, "Rendering 2D Adorn Items, %u items", renderable2ds.size());
	for (int i = 0; i < renderable2ds.size(); ++i) 
	{
		renderable2ds[i]->render2d(adorn);
	}
}


void IAdornableCollector::render3dAdornItems(Adorn* adorn)
{
	FASTLOG1(FLog::AdornRenderStats, "Rendering 3D Adorn Items, %u items", renderable3ds.size());
	for (int i = 0; i < renderable3ds.size(); ++i) 
	{
		renderable3ds[i]->render3dAdorn(adorn);
	}
}

void IAdornableCollector::append3dSortedAdornItems(std::vector<AdornableDepth>& destination, const Camera* camera) const
{
	FASTLOG1(FLog::AdornRenderStats, "Rendering 3DSort Adorn Items, %u items", renderable3dSorteds.size());
	for (int i = 0; i < renderable3dSorteds.size(); ++i) 
	{
		AdornableDepth ad = { renderable3dSorteds[i], renderable3dSorteds[i]->calculateDepth(camera) };

		destination.push_back(ad);
	}
}

}  // namespace RBX
