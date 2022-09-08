/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/IndexArray.h"
#include "Util/Selectable.h"
#include "V8Tree/Instance.h"
#include "SelectState.h"

namespace RBX {
	class IAdornableCollector;
	class Adorn;
	class Camera;

	class RBXInterface IAdornable
		: public Selectable
	{
	friend class IAdornableCollector;

	private:
		int		index2d;
		int		index3d;
		int		index3dSorted;
		int&	indexFunc2d() {return index2d;}
		int&	indexFunc3d() {return index3d;}
		int&	indexFunc3dSorted() {return index3dSorted;}

		IAdornableCollector*		bucket;

	protected:
		virtual bool shouldRender2d() const {return false;}
		virtual bool shouldRender3dAdorn() const {return false;}
		virtual bool shouldRender3dSortedAdorn() const {return false;}



	public:
		IAdornable() : bucket(NULL), index2d(-1), index3d(-1), index3dSorted(-1)
		{}

		~IAdornable();

		void shouldRenderSetDirty();		// sets this IAdornable dirty
		float calculateDepth(const Camera* camera) const;	// calculates the depth based on camera

		virtual bool isVisible(const Rect2D& rect) const { return true; }

		virtual void renderBackground2d(Adorn* adorn) {}
		virtual void renderBackground2dContext(Adorn* adorn, const Instance* context) { renderBackground2d(adorn); }
		virtual void render2d(Adorn* adorn) {}
		virtual void render2dContext(Adorn* adorn, const Instance* context) { render2d(adorn); }
		virtual void render3dAdorn(Adorn* adorn) {}
		virtual void render3dSortedAdorn(Adorn* adorn) {}
		virtual void render3dSelect(Adorn* adorn, SelectState selectState) {}

		virtual Vector3 render3dSortedPosition() const { return Vector3(0,0,0); }
	};

	struct AdornableDepth
	{
		IAdornable* adornable;
		float depth;

		bool operator<(const AdornableDepth& o) const
		{
			return depth > o.depth;
		}
	};

} // namespace

