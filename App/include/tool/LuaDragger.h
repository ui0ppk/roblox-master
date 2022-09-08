/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "V8Tree/Instance.h"

namespace RBX {

	class Joint;
	class PartInstance;
	class ContactManager;
	class RunDragger;

	extern const char* const sLuaDragger;

	class LuaDragger
		: public DescribedCreatable<LuaDragger, Instance, sLuaDragger>
	{
	private:
		typedef enum {NO_PARTS, MOUSE_DOWN, DRAGGING, MOUSE_UP_DRAGGED, MOUSE_UP_NO_DRAG} DragPhase;
		DragPhase dragPhase;

		std::vector<shared_ptr<Joint> > jointsIMade;
		weak_ptr<PartInstance> rootPart;
		std::auto_ptr<RunDragger> runDragger;		// only if we have one part

		float hitPointHeight;

		typedef std::vector<weak_ptr<PartInstance> >	WeakParts;
		WeakParts dragParts;
		weak_ptr<PartInstance> mousePart;
		Vector3	pointOnMousePart;

		void tryStartDragging(const RbxRay& unitMouseRay);
		void startDragging();
		void doDrag(const RbxRay& unitMouseRay);
		bool getSnapHitPoint(PartInstance* part, const RbxRay& unitMouseRay, Vector3& hitPoint);
		ContactManager& getContactManager(PartInstance* partInstance);

		const float breakFreeDistance() {return 1.5f;}		// distance in studs at the mouse down point before movement

		void addPart(shared_ptr<const Instance> part);
		/*override*/ bool askSetParent(const Instance* instance) const { return false; }

	public:
		LuaDragger();
		~LuaDragger();

		void mouseDownPublic(shared_ptr<Instance> _mousePart,
							 Vector3 _pointOnMousePart,
							 shared_ptr<const Instances> _dragParts);	

		void mouseDown(	shared_ptr<PartInstance> _mousePart,
						const Vector3& _pointOnMousePart,
						const std::vector<weak_ptr<PartInstance> > _dragParts);	

		void mouseMove(RbxRay mouseRay);		// inefficient, but easier to have just one version

		void mouseUp();

		const WeakParts& getParts() {return dragParts;}

		void rotateOnSnapFace(Vector3::Axis, const Matrix3& rotMatrix);
		void axisRotate(Vector3::Axis axis);

		bool isDragging() const {return dragPhase == DRAGGING;}

		bool didDrag() const {return dragPhase == MOUSE_UP_DRAGGED;}

		void alignPartToGrid( void );
	};

} // namespace RBX
