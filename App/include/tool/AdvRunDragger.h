/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8World/Contact.h"
#include "Util/Object.h"
#include "Util/G3DCore.h"
#include "Util/NormalId.h"
#include <vector>
#include "Tool/DragTypes.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"
#include "GfxBase/IAdornable.h"

//#define DEBUG_MULTIPLE_PARTS_DRAG

namespace RBX {
	class Primitive;
	class PartInstance;
	class Workspace;
	typedef std::vector<weak_ptr<PartInstance> >	WeakParts;
	typedef std::vector<CoordinateFrame>            Locations;

	class AdvRunDragger : public IAdornable
	{
	private:
		class SnapInfo {
		public:
			Primitive*						snap;				
			NormalId						surface;
			size_t							mySurfaceId;
			Vector3							hitWorld;
			Vector3							lastDragSnap;
			SnapInfo() 
				: snap(NULL)
				, surface(NORM_UNDEFINED)
				, mySurfaceId((size_t)-1)
				, hitWorld(Vector3::inf())
				, lastDragSnap(Vector3::inf())
			{}
			void updateHitFromSurface(const RbxRay& mouseRay);
			void updateSurfaceFromHit();
			float hitOutsideExtents();
		};

		// Initialized
		// Poor man's version - ultimately, change all internally to shared_ptr's
		weak_ptr<PartInstance>				dragPart;
		std::vector<Primitive*>				dragParts;	
		WeakParts							weakDragParts;
        weak_ptr<PartInstance>				snapPart;

		bool								isAdornable;
		Workspace*							workspace;
		Primitive*			 				drag;	
		Vector3								dragPointLocal;
		Matrix3								dragOriginalRotation;

		// Carry forward data
		SnapInfo							snapInfo;

		// Set on every snap() call
		RbxRay								mouseRay;

		// Computed every snap() call
		NormalId							dragSurface;		// computed
		// Angled surface testing...
		size_t								myDragSurfaceId;	// computed
		Vector3								dragHitLocal;		// computed

		Vector3								snapGridOrigin;
		bool								snapGridOriginNeedsUpdating;

		// modes
		DRAG::DraggerGridMode				gridMode;
		bool								jointCreateMode;

		// new multiple drag
		boost::shared_ptr<RBX::PartInstance> tempPart;
		Locations                            originalLocations;
		G3D::Array<Primitive*>               savedPrimsForMultiDrag;
		PartInstance*                        primaryPart;
		bool                                 handleMultipleParts;
		CoordinateFrame                      tempOriginalCF;

		SnapInfo createSnapSurface(Primitive* snap, G3D::Array<size_t>* ignore = NULL);
		bool moveDragPart();
		bool snapDragPart(bool supressGridSettings = false);
		bool pushDragPart(const Vector3& snapNormal);
        
		void findSafeY();
		bool notTried(Primitive* check, const G3D::Array<Primitive*>& tried);
		bool adjacent(Primitive* p0, Primitive* p1);
		SnapInfo rayHitsPart(const G3D::Array<Primitive*>& triedSnap, bool forceAdjacent);
		SnapInfo bestProximatePart(const G3D::Array<Primitive*>& triedSnap, 
								   Contact::ProximityTest proximityTest);
		bool fallOffEdge();

		bool fallOffPart(bool& snapped);
		bool colliding();
		bool rayHitsCloserPart();
		bool tooCloseToCamera();

		SnapInfo findSnap(const G3D::Array<Primitive*>& triedSnap);
		void findNoSnapPosition(const CoordinateFrame& original);

		void snapInfoFromSnapPart();
		void snapPartFromSnapInfo();

		void savePrimsForMultiDrag();
		Contact* getFirstContact(Primitive*& prim);
		Contact* getNextContact(Primitive*& prim, Contact* c);

	public:
		AdvRunDragger();
		~AdvRunDragger();
		void init(	Workspace*							_workspace,
					weak_ptr<PartInstance>				_dragPart,	
					const Vector3&						_dragPointWorld);
		void initLocal(	Workspace*						_workspace,
					    weak_ptr<PartInstance>			_dragPart,	
					    const Vector3&					_dragPointLocal,
						WeakParts						_dragParts);

		bool snap(const RbxRay& mouseRay);
        bool snapGroup(const RbxRay& _mouseRay);
        bool getSnapHitPoint(PartInstance* part, const RbxRay& unitMouseRay, Vector3& hitPoint);

		void rotatePartAboutSnapFaceAxis( Vector3::Axis axis, const float& angleInRads );
		void rotatePart90DegAboutSnapFaceAxis( Vector3::Axis axis );

        // Note: these will NOT update dragOriginalRotation
		static void turnUpright(PartInstance* part);
		static void rotatePart(PartInstance* part);
		static void tiltPart(PartInstance* part, const CoordinateFrame& camera);

		void setGridMode( DRAG::DraggerGridMode mode ) { gridMode = mode; }
		DRAG::DraggerGridMode getGridMode( void ) { return gridMode; }
		void setJointCreateMode( bool mode ) { jointCreateMode = mode; }
		bool getJointCreateMode( void ) { return jointCreateMode; }
        CoordinateFrame getSnapSurfaceCoord( void );
	
#ifdef DEBUG_MULTIPLE_PARTS_DRAG
		// IAdornable
		bool shouldRender3dAdorn() const { return true; }
		void render3dAdorn(Adorn* adorn);
#else
		// IAdornable
		bool shouldRender3dAdorn() const { return false; }
#endif

		static bool dragMultiPartsAsSinglePart;
	};
} // namespace

