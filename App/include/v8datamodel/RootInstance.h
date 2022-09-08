#pragma once

#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/ICameraOwner.h"
#include "Tool/DragUtilities.h"
#include "Tool/DragTypes.h"
#include "Util/InsertMode.h"

namespace RBX {

	class World;
	class MegaDragger;
	class HopperBin;
	class SpawnLocation;
	class Decal;

	extern const char* const sRootInstance;
	class RootInstance
		: public Reflection::Described<RootInstance, sRootInstance, ModelInstance>
		, public ICameraOwner
	{
	private:
		Vector3 insertPoint;
		Vector3 computeIdeInsertPoint();
		Vector3 computeCharacterInsertPoint(const Extents& extents);

		////////////////////////////////////////////////////
		Extents gatherPartExtents(PartArray& partArray);

		void moveSafe(MegaDragger& megaDragger, Vector3 move, DRAG::MoveType moveType);
		void moveSafe(PartArray& partArray, Vector3 move, DRAG::MoveType moveType);
		void moveToCharacterInsertPoint(PartArray& partArray);
		void moveToIdeInsertPoint(PartArray& partArray, const Vector3& insertPoint);
		void moveToRemoteInsertPoint(PartArray& partArray, Vector3 remotePoint);

		void insertRaw(const Instances& instances, Instance* requestedParent, PartArray& partArray, bool suppressMove = false);
		void insertToTree(const Instances& instances, Instance* requestedParent, bool suppressMove = false, bool lerpCameraInStudio = true);

		void insert3dView(const Instances& instances, PromptMode promptMode, bool suppressMove, const Vector3* positionHint = NULL, bool lerpCameraInStudio = true);
		void insertRemoteCharacterView(const Instances& instances, PartArray& partArray, const Vector3* positionHint, PromptMode promptMode, bool suppressMove = false);
		void insertCharacterView(const Instances& instances, PartArray& partArray);
		void insertIdeView(const Instances& instances, PartArray& partArray, PromptMode promptMode,
						   bool suppressMove);

		void insertHopperBin(HopperBin* bin);
		void insertSpawnLocation(SpawnLocation *s);
		
		void insertDecal(Decal *d, RBX::InsertMode insertMode);

		void doInsertInstances(
			const Instances& instances,
			Instance* requestedParent,
			InsertMode insertMode,				// RAW: don't move, TREE: only move up, 3D_View: insert point
			PromptMode promptMode,
			const Vector3* positionHint = NULL,
			Instances* remaining = NULL,
			bool forceSuppressMove = false,
			bool lerpCameraInStudio = true);

	protected:

		/////////////////////////////////////////////////////////////
		// Instance 

		Rect2D viewPort;		
		std::auto_ptr<World> world;	

		RootInstance();
		~RootInstance();

	public:
		void insertInstances(
			const Instances& instances,
			Instance* requestedParent,
			InsertMode insertMode,				// RAW: don't move, TREE: only move up, 3D_View: insert point
			PromptMode promptMode,
			const Vector3* positionHint = NULL,
			Instances* remaining = NULL,
			bool lerpCameraInStudio = true);

		// Instead of putting parts in the camera's focus, we focus the camera on where parts were pasted
		void insertPasteInstances(
			const Instances& instances,
			Instance* requestedParent,
			InsertMode insertMode,				// RAW: don't move, TREE: only move up, 3D_View: insert point
			PromptMode promptMode,
			const Vector3* positionHint = NULL,
			Instances* remaining = NULL,
			bool lerpCameraInStudio = true);

		Vector3 computeCharacterInsertPoint(const Vector3& sizeOfInsertedModel); 

		void removeInstances(const Instances& instances);
		void publicInsertRaw(const Instances& instances, Instance* requestedParent, PartArray& partArray, bool joinPartsInInstancesOnly = false, bool suppressPartMove = true);

		void moveCharacterToDefaultInsertPoint(ModelInstance* character, const Extents& extentsBeforeCharacter);

		void setInsertPoint(const Vector3& topCenter);

		void moveToPoint(PVInstance* pv, Vector3 point, DRAG::JoinType joinType);

		void movePartsToCameraFocus(PartArray& partArray);
		void focusCameraOnParts(PartArray& partArray, bool lerpCameraInStudio = true);

		World* getWorld() { return world.get(); }

		const Rect2D& getViewPort() { return viewPort; }
	};
}  // namespace RBX
