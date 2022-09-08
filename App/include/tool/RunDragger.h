/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

//#include "V8World/Joint.h"
#include "V8World/Contact.h"
#include "Util/Object.h"
#include "Util/G3DCore.h"
#include "Util/NormalId.h"
#include <vector>
#include "Tool/DragTypes.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Adorn.h"
#include "GfxBase/IAdornable.h"



namespace RBX {
	class Primitive;
	class PartInstance;
	class Workspace;

	class RunDragger : public IAdornable
	{
	private:
		class SnapInfo {
		public:
			Primitive*						snap;				
			size_t							mySurfaceId;
			Vector3							hitWorld;
			Vector3							lastHitWorld;
			Vector3							lastDragSnap;
			SnapInfo() 
				: snap(NULL)
				, mySurfaceId((size_t)-1)
				, hitWorld(Vector3::inf())
				, lastHitWorld(Vector3::inf())
				, lastDragSnap(Vector3::inf())
			{}
			void updateHitFromSurface(const RbxRay& mouseRay);
			void updateSurfaceFromHit();
			float hitOutsideExtents();
		};

		// Initialized
		// Poor man's version - ultimately, change all internally to shared_ptr's
		weak_ptr<PartInstance>				dragPart;
		weak_ptr<PartInstance>				snapPart;

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

		// modes

		SnapInfo createSnapSurface(Primitive* snap, G3D::Array<size_t>* ignore = NULL);
		bool moveDragPart();
		bool snapDragPart();
		void snapRotatePart();

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

	public:
		RunDragger();
		~RunDragger();
		void init(	Workspace*							_workspace,
					weak_ptr<PartInstance>				_dragPart,	
					const Vector3&						_dragPointWorld);
		void initLocal(	Workspace*						_workspace,
					    weak_ptr<PartInstance>			_dragPart,	
					    const Vector3&					_dragPointLocal);

		bool snap(const RbxRay& mouseRay);

		void rotatePartAboutSnapFaceAxis( Vector3::Axis axis, const float& angleInRads );
		void rotatePart90DegAboutSnapFaceAxis( Vector3::Axis axis );
        CoordinateFrame getSnapSurfaceCoord();

        // Note: these static functions will NOT update dragOriginalRotation
		static void turnUpright(PartInstance* part);
		static void rotatePart(PartInstance* part);
		static void tiltPart(PartInstance* part, const CoordinateFrame& camera);
	};
} // namespace