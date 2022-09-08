/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "GfxBase/IAdornable.h"
#include "Util/IndexArray.h"

LOGGROUP(AdornRenderStats);

namespace RBX {
	class Adorn;

	class RBXInterface IAdornableCollector
	{
	friend class IAdornable;
	private:
		IndexArray<IAdornable, &IAdornable::indexFunc2d> renderable2ds;
		IndexArray<IAdornable, &IAdornable::indexFunc3d> renderable3ds;
		IndexArray<IAdornable, &IAdornable::indexFunc3dSorted> renderable3dSorteds;

	public:
		void onRenderableDescendantAdded(IAdornable* iR);
		void onRenderableDescendantRemoving(IAdornable* iR);

		void recomputeShouldRender(IAdornable* iR);
	public:
		IAdornableCollector()
		{}

		~IAdornableCollector();

		void render2dItems(Adorn* adorn);
		void render3dAdornItems(Adorn* adorn);
		void append3dSortedAdornItems(std::vector<AdornableDepth>& destination, const Camera* camera) const;
	};

}  // namespace RBX
