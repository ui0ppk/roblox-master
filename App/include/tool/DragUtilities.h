/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */

#pragma once

#include "Util/Object.h"
#include "Util/G3DCore.h"
#include "Util/Extents.h"
#include "Tool/Dragger.h"

namespace RBX {
	class Instance;
	class PartInstance;
	class Primitive;
	class PVInstance;
	class ContactManager;
	class World;

	typedef std::vector<weak_ptr<PartInstance> > PartArray;

	class DragUtilities
	{
	private:
		static bool hitObjectOrPlane(const ContactManager& contactManager,
									const RbxRay& unitSearchRay,
									const std::vector<const Primitive*>& ignorePrims,
									Vector3& hit,
                                    bool snapToGrid = true);
		static bool hitObject(	const ContactManager& contactManager,
								const RbxRay& unitSearchRay,
								const std::vector<const Primitive*>& ignorePrims,
								Vector3& hit,
								bool snapToGrid = true);

	public:
		static bool notJoined(const PartArray& parts);
		static bool notJoinedToOutsiders(const PartArray& parts);

		static void unJoinFromOutsiders(const PartArray& parts);
		static void joinToOutsiders(const PartArray& parts);

		static void unJoin(const PartArray& parts);
		static void join(const PartArray& parts);
		static void joinWithInPartsOnly(const PartArray& parts);

		static void setDragging(const PartArray& parts);
		static void stopDragging(const PartArray& parts);

		static void clean(const PartArray& parts);
		static void move(const PartArray& parts,
						  CoordinateFrame from, 
						  CoordinateFrame to);
		static void move2(const PartArray& parts,
						  CoordinateFrame from, 
						  CoordinateFrame to);

		static void alignToGrid(PartInstance* part);		// force alignement to grid
		static void clean(PartInstance* part);				// clean up alignment if already aligned
		static void moveByDelta(PartInstance* part, const Vector3& delta, bool snapToWorld);

		static void pvsToParts(const std::vector<PVInstance*>& pvInstances, PartArray& parts);
		static void instancesToParts(const std::vector<Instance*>& instances, PartArray& parts);
		static void instancesToParts(const Instances& instances, PartArray& parts);
		static void removeDuplicateParts(PartArray& parts);

		static World* partsToPrimitives(const PartArray& parts, G3D::Array<Primitive*>& primitives);
		static void partsToPrimitives(const PartArray& parts, std::vector<const Primitive*>& primitives);
		static void partsToPrimitives(const PartArray& parts, std::vector<Primitive*>& primitives);

		static Extents computeExtents(const PartArray& parts);

		static Vector3 hitObjectOrPlane(const PartArray& parts, 
										const RbxRay& unitMouseRay, 
										const ContactManager& contactManager,
                                        bool snapToGrid = true);

		static bool hitObject(const PartArray& parts, 
										const RbxRay& unitMouseRay, 
										const ContactManager& contactManager,
										Vector3& hit,
                                        bool snapToGrid = true);

		static bool anyPartAlive(const PartArray& parts);

		// replacement for PVInstance::getPrimitives();

		static void getPrimitives2(shared_ptr<Instance> instance, std::vector<Primitive*>& primitives) {
			getPrimitives(instance.get(), primitives);
		}

		static void getPrimitives(const Instance* instance, std::vector<Primitive*>& primitives);

		static void getPrimitivesConst(const Instance* instance, std::vector<const Primitive*>& primitives);

		static Vector3 safeMoveYDrop(const PartArray& parts, const Vector3& tryDrag, ContactManager& contactManager, const float customPlaneHeight = Dragger::groundPlaneDepth() );
		static Vector3 toGrid(const Vector3 &point, const Vector3& grid = Vector3::zero());
        static Vector3 toLocalGrid(const Vector3& deltaIn);
        static Vector3 getGrid();

	};
} // namespace
