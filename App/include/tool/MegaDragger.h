/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/Object.h"
#include "Util/G3DCore.h"
#include "Tool/DragUtilities.h"
#include "Tool/DragTypes.h"
//#include <vector>

namespace RBX {
	class Primitive;
	class PVInstance;
	class PartInstance;
	class RootInstance;
	class ContactManager;
	class InputObject;

	class MegaDragger 
	{
	private:
		weak_ptr<PartInstance> mousePart;
		PartArray dragParts;

		// Join/unjoin stuff
		bool joined;
		DRAG::JoinType joinType;

		// Workspace stuff
		RootInstance* rootInstance;
		ContactManager& contactManager;

		bool moveSafePlaceAlongLine(const Vector3& tryDrag);

	public:
		MegaDragger(PartInstance* mousePartPtr,
					const std::vector<PVInstance*>& pvInstances,
					RootInstance* rootInstance,
					DRAG::JoinType joinType = DRAG::UNJOIN_JOIN);

		MegaDragger(PartInstance* mousePartPtr,
					const PartArray& partArray,
					RootInstance* rootInstance,
					DRAG::JoinType joinType = DRAG::UNJOIN_JOIN);



		~MegaDragger();

		void setToSelection(const Workspace* workspace);

		// Drag cycle - start, continue (before every move), finish
		void startDragging();
		void continueDragging();							// every drag step/move/idle
		void finishDragging();

		// Inquiry
		bool mousePartAlive();
		bool anyDragPartAlive();

		// Part Dragger
		weak_ptr<PartInstance> getMousePart() {
			RBXASSERT(!mousePart.expired());
			return mousePart;
		}

		// Group Dragger
		void alignAndCleanParts();
		void cleanParts();
		Vector3 hitObjectOrPlane(const shared_ptr<InputObject>& inputObject);		// ignore the drag parts - find a hit point with the world
		Vector3 safeMoveYDrop(const Vector3& tryDrag);			// 1. Go directly to new location, 2. Moves down until collision - if necessary, moves up

		// Axis Tools
		Vector3 safeMoveAlongLine(const Vector3& tryDrag, bool snapToWorld = true);
		Vector3 safeRotateAlongLine(const Vector3& tryDrag);

		Vector3 safeMoveAlongLine2(const Vector3& tryDrag, bool& out_isCollided);
		Vector3 safeRotateAlongLine2(const Vector3& tryDrag, const float &angle);

		Vector3 safeMoveToMinimumHeight(float yValue);

		/**
		 * Rotates drag parts
		 * @param rotMatrix rotation matrix to apply
		 * @param respectCollisions if true then check for collision will be done 
		                            in case of collisions no rotation will be performed
		 * @return Matrix after rotation
		 */
		Matrix3 rotateDragParts(const Matrix3& rotMatrix, bool respectCollisions);

		bool moveAlongLine(const Vector3& tryDrag);

		// General Placement
		Vector3 safeMoveNoDrop(const Vector3& tryDrag);				// On initial collision - moves up
		bool safeRotate(const Matrix3& rotMatrix);
		void removeParts();
	private:
		void getPartsForDrag(G3D::Array<Primitive*>& primitives);
	};
} // namespace
