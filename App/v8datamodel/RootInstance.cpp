/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/RootInstance.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Tool.h"
#include "V8DataModel/Hopper.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/Sky.h"
#include "V8DataModel/Lighting.h"
#include "V8DataModel/Team.h"
#include "V8DataModel/Teams.h"
#include "V8DataModel/SpawnLocation.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/ChangeHistory.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/ManualJointHelper.h"
#include "Tool/MegaDragger.h"
#include "V8World/World.h"
#include "V8Kernel/Constants.h"
#include "AppDraw/DrawAdorn.h"
#include "GfxBase/Part.h"
#include "Network/Players.h"
#include "Util/Math.h"
#include "Util/Units.h"
#include "Tool/DragUtilities.h"
#include "Tool/ToolsArrow.h"
#include "v8datamodel/PluginManager.h"

namespace RBX {

const char* const sRootInstance = "RootInstance";


RootInstance::RootInstance() : 	
	world(new World()),
	insertPoint(Vector3::zero()),
	viewPort(Rect2D(Vector2(800, 600)))		// this is updated on every render - just a starting value
{}


// TODO:  could delete the world first...
RootInstance::~RootInstance()
{
}


void RootInstance::setInsertPoint(const Vector3& topCenter)
{
	insertPoint = DragUtilities::toGrid(topCenter);
}

Vector3 RootInstance::computeIdeInsertPoint() 
{
	// Y >= 0.0
	insertPoint.y = std::max(0.0f, insertPoint.y);

	// Put the insert point inside the current x, z extents of the world
	float tempY = insertPoint.y;
	Extents worldExtents = computeExtentsWorld();
	insertPoint = worldExtents.clip(insertPoint);
	insertPoint.y = std::max(tempY, worldExtents.min().y);	// don't insert below current world extents

	// Snap the insertPoint to grid
	insertPoint = DragUtilities::toGrid(insertPoint);
	return insertPoint;
}

Vector3 RootInstance::computeCharacterInsertPoint(const Extents& extents) 
{
	return computeCharacterInsertPoint(extents.size());
}


Vector3 RootInstance::computeCharacterInsertPoint(const Vector3& sizeOfInsertedModel) 
{
	if (ModelInstance* model = Network::Players::findLocalCharacter(this))
	{
		Vector3 cameraLook = getConstCamera()->coordinateFrame().lookVector();
		cameraLook.y = 0.0;
		cameraLook.unitize();	// planar unit vector of camera look.

		Vector3 halfSize = sizeOfInsertedModel * 0.5f;
		halfSize.y = 0.0;

		PartArray partArray;
		PartInstance::findParts(model, partArray);
		Extents modelExtents = DragUtilities::computeExtents(partArray);

		Vector3 insertPoint =		modelExtents.bottomCenter() 
								+	(7.0f * cameraLook)				// default distance to move from character
								+	(halfSize * cameraLook);
		insertPoint = DragUtilities::toGrid(insertPoint);
		return insertPoint;
	}		
	else
	{
		RBXASSERT(0);
		return computeIdeInsertPoint();
	}
}

void RootInstance::moveCharacterToDefaultInsertPoint(ModelInstance* character, const Extents& extentsBeforeCharacter)
{
	Vector3 dropPoint(0, 100, 0);

	if (!extentsBeforeCharacter.contains(Vector3::zero()))
	{	
		dropPoint += extentsBeforeCharacter.bottomCenter();
	}

	moveToPoint(character, dropPoint, DRAG::UNJOIN_NO_JOIN);
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

void RootInstance::moveSafe(MegaDragger& megaDragger, Vector3 move, DRAG::MoveType moveType)
{
	megaDragger.startDragging();					// UnjoiningToOutsiders here
	
	if (moveType == DRAG::MOVE_DROP) {
		megaDragger.safeMoveYDrop(move);
	}
	else {
		megaDragger.safeMoveNoDrop(move);
	}

	megaDragger.finishDragging();					// Sets insertion point here
}

// Always joins on finish
void RootInstance::moveSafe(PartArray& partArray, Vector3 move, DRAG::MoveType moveType)
{
	MegaDragger megaDragger(NULL, partArray, this);

	moveSafe(megaDragger, move, moveType);
}

void RootInstance::moveToPoint(PVInstance* pv, Vector3 point, DRAG::JoinType joinType)
{
	if (this->contains(pv)) {
		if (pv->getPrimaryPart()) {
			std::vector<PVInstance*> pvInstances;
			pvInstances.push_back(pv);

			Vector3 move = (point - pv->getLocation().translation);	// drop point is 0, 100, 0

			MegaDragger megaDragger(pv->getPrimaryPart(), pvInstances, this, joinType);

			moveSafe(megaDragger, move, DRAG::MOVE_NO_DROP);
		}
	}
}

Extents RootInstance::gatherPartExtents(PartArray& partArray)
{
	Extents partsExtents = Extents::zero();

	if(!partArray.empty())
	{
		// we use this part as a starting point to build our cframe of the collection of parts from partArray
		shared_ptr<PartInstance> firstPart = partArray.front().lock();

		// if we can't lock a part, we are hosed, just bail
		if(!firstPart)
			return partsExtents;

		// loop thru our parts and construct our extents of all parts
		for(size_t i = 0; i < partArray.size(); ++i)
		{
			if(shared_ptr<PartInstance> part = partArray[i].lock())
			{
				Primitive* primitive = part->getPartPrimitive();
				Extents partExtentsInModel = primitive->getExtentsLocal().express(primitive->getCoordinateFrame(), firstPart->getCoordinateFrame());
				partsExtents.unionWith(partExtentsInModel);
			}
		}
	}

	return partsExtents;
}
void RootInstance::movePartsToCameraFocus(PartArray& partArray)
{
	Vector3 moveToLocation = getCamera()->getCameraFocus().translation;
	RBX::Plugin *activePlugin = PluginManager::singleton()->getActivePlugin(DataModel::get(this));

	bool toolPluginActive = activePlugin && activePlugin->isTool();

	if(!partArray.empty())
	{
		Extents partsExtents = gatherPartExtents(partArray);

		if(partsExtents != Extents::zero())
		{
			// gather all parts and translate to pvInstance vector
			std::vector<PVInstance*> pvInstances;
			for(size_t i = 0; i < partArray.size(); ++i)
			{
				if(shared_ptr<PartInstance> part = partArray[i].lock())
					pvInstances.push_back(part.get());
			}

			// take our extents and use the center diff from location to determine offset to move all parts
			if(!pvInstances.empty() && !toolPluginActive)
				if(shared_ptr<PartInstance> firstPart = partArray.front().lock())
				{
					CoordinateFrame location = firstPart->getCoordinateFrame();
					location.translation = location.pointToWorldSpace(partsExtents.center());

					Vector3 move = DragUtilities::toGrid(moveToLocation - location.translation);
					moveSafe(partArray, move, DRAG::MOVE_NO_DROP);
				}
		}
	}
}

void RootInstance::moveToRemoteInsertPoint(PartArray& partArray, Vector3 point)
{
	if (partArray.size() > 0)
	{
		Extents extents = DragUtilities::computeExtents(partArray);

		Vector3 moveToInsert = DragUtilities::toGrid(point - extents.bottomCenter());

		moveSafe(partArray, moveToInsert, DRAG::MOVE_DROP);
	}
}


void RootInstance::moveToCharacterInsertPoint(PartArray& partArray)
{
	if (partArray.size() > 0)
	{
		Extents extents = DragUtilities::computeExtents(partArray);

		Vector3 characterInsertPoint = computeCharacterInsertPoint(extents);

		Vector3 moveToInsert = DragUtilities::toGrid(characterInsertPoint - extents.bottomCenter());

		moveSafe(partArray, moveToInsert, DRAG::MOVE_DROP);
	}
}


void RootInstance::moveToIdeInsertPoint(PartArray& partArray, const Vector3& insertPoint)
{
	if (partArray.size() > 0)
	{
		Extents extents = DragUtilities::computeExtents(partArray);

		Vector3 current = extents.bottomCenter(); 

		Vector3 moveToInsert = DragUtilities::toGrid(insertPoint - current);

		moveSafe(partArray, moveToInsert, DRAG::MOVE_DROP);
	}
}


//////////////////////////////////////////////////
void RootInstance::insertRaw(const Instances& instances, Instance* requestedParent, PartArray& partArray, bool suppressMove)
{
	RBXASSERT(requestedParent && this->contains(requestedParent));
	publicInsertRaw(instances,requestedParent,partArray,false,suppressMove);
}

void RootInstance::focusCameraOnParts(PartArray& partArray, bool lerpCameraInStudio)
{
	if(Camera* camera = getCamera())
		if(Workspace *ws = ServiceProvider::find<Workspace>(this))
			if(World* world = ws->getWorld())
				if(world->getContactManager())
				{
					bool needsCameraAdjustment = false;
					for(size_t i = 0; i < partArray.size(); ++i)
					{
						if(shared_ptr<PartInstance> part = partArray[i].lock())
							needsCameraAdjustment = !camera->isPartInFrustum(*(part.get()));
						if(needsCameraAdjustment)
							break;
					}
					if(needsCameraAdjustment)
					{
						Extents partArrayExtents = gatherPartExtents(partArray);
						if (shared_ptr<PartInstance> partInstance = partArray.front().lock())
						{
							partArrayExtents = partArrayExtents.toWorldSpace(partInstance->getCoordinateFrame());
							if(partArrayExtents != Extents::zero())
							{
								if (lerpCameraInStudio)
									camera->lerpToExtents(partArrayExtents);
								else
									camera->zoomExtents(partArrayExtents, Camera::ZOOM_OUT_ONLY);
							}
						}
					}
				}
}

void RootInstance::publicInsertRaw(const Instances& instances, Instance* requestedParent, PartArray& partArray, bool joinPartsInInstancesOnly, bool suppressPartMove)
{
	// first, lets copy all parts into passed in partArray
	for (size_t i = 0; i < instances.size(); ++i)
		PartInstance::findParts(instances[i].get(), partArray);

	// now, parent all instances we have to the requested parent
	std::for_each(instances.begin(), instances.end(), boost::bind(&Instance::setParent, _1, requestedParent));


	if( RBX::GameBasicSettings::singleton().inStudioMode() ) // only works in studio (InsertService::Insert calls this function)
		if( !partArray.empty() )	// if we have parts, we need to move them to camera focus
			if( !suppressPartMove)  // move parts to camera focus
				movePartsToCameraFocus(partArray);

	if (!joinPartsInInstancesOnly)
		DragUtilities::join(partArray);
	else // only join together parts inside instances
		DragUtilities::joinWithInPartsOnly(partArray);
}

void RootInstance::insertToTree(const Instances& instances, Instance* requestedParent, bool suppressMove, bool lerpCameraInStudio)
{
	PartArray partArray;
	insertRaw(instances, requestedParent, partArray, suppressMove);

	if (!partArray.empty())
	{
		moveSafe(partArray, Vector3::zero(), DRAG::MOVE_NO_DROP);
	}

	focusCameraOnParts(partArray, lerpCameraInStudio);
}


void RootInstance::insertRemoteCharacterView(const Instances& instances, PartArray& partArray, const Vector3* positionHint, PromptMode promptMode, bool suppressMove)
{
	RBXASSERT(instances.size() > 0);

	if ((instances.size() == 1) && (promptMode == PUT_TOOL_IN_STARTERPACK))
	{
		Instance* single = instances[0].get();
		if (Instance::fastDynamicCast<Tool>(single))
		{
			single->setParent(ServiceProvider::create<StarterPackService>(this));
			return;
		}
	}

	insertRaw(instances, this, partArray, suppressMove);

	moveToRemoteInsertPoint(partArray, *positionHint);
}


void RootInstance::insertCharacterView(const Instances& instances, PartArray& partArray)
{
	RBXASSERT(instances.size() > 0);
	if ((instances.size() == 1))
	{
		Instance* single = instances[0].get();
		if (Instance::fastDynamicCast<HopperBin>(single))
		{
			if (RBX::Network::Player* player = RBX::Network::Players::findLocalPlayer(this))
			{
				single->setParent(player->getPlayerBackpack());
				return;
			}
		}
	}

	insertRaw(instances, this, partArray);

	// Move to the character insert point
	moveToCharacterInsertPoint(partArray);
}


void RootInstance::insertIdeView(const Instances& instances, PartArray& partArray, PromptMode promptMode, bool suppressMove)
{
	RBXASSERT(instances.size() > 0);
	if ((instances.size() == 1))
	{
		Instance* single = instances[0].get();

		if (Instance::fastDynamicCast<HopperBin>(single))
		{
			single->setParent(ServiceProvider::create<StarterPackService>(this));
			return;
		}

		if (	Instance::fastDynamicCast<Tool>(single)  
			&&	(promptMode == PUT_TOOL_IN_STARTERPACK)
			)
		{
			single->setParent(ServiceProvider::create<StarterPackService>(this));
			return;
		}
	}

	// do this before inserting
	Vector3 insertPoint = computeIdeInsertPoint();

	insertRaw(instances, this, partArray, suppressMove);


	if (!partArray.empty())
	{
		if(suppressMove)
		{
			// 3.  Move to the character insert point
			moveToIdeInsertPoint(partArray, insertPoint);
		}
	}
}



void RootInstance::insert3dView(const Instances& instances, PromptMode promptMode, bool suppressMove, const Vector3* positionHint, bool lerpCameraInStudio)
{
	RBXASSERT(instances.size() > 0);

	PartArray partArray;
	if (positionHint != NULL) {
		insertRemoteCharacterView(instances, partArray, positionHint, promptMode, suppressMove);
	}
	else if (Network::Players::findLocalCharacter(this)) {
		insertCharacterView(instances, partArray);
	}
	else {
		insertIdeView(instances, partArray, promptMode, suppressMove);
	}

	RBX::Plugin *activePlugin = PluginManager::singleton()->getActivePlugin(DataModel::get(this));

	bool toolPluginActive = activePlugin && activePlugin->isTool();
			
	if(suppressMove && !toolPluginActive)
	{
		if (partArray.size() > 0)
		{
			focusCameraOnParts(partArray, lerpCameraInStudio);
		}
	}

	//create joints automatically (should be done after moving parts else dragging may remove the joints)
	if (AdvArrowToolBase::advManualJointMode && !partArray.empty())
	{
		if(Workspace *pWorkspace = ServiceProvider::find<Workspace>(this))
		{
			std::vector<Instance*> selectedInstances;
			for(size_t ii = 0; ii < partArray.size(); ++ii)
			{
				if(shared_ptr<PartInstance> part = partArray[ii].lock())
					selectedInstances.push_back(part.get());
			}

			//create joints automatically
			ManualJointHelper jointHelper;
			jointHelper.setSelectedPrimitives(selectedInstances);
			jointHelper.setWorkspace(pWorkspace);
			jointHelper.findPermissibleJointSurfacePairs();
			jointHelper.createJoints();
		}
	}
}

void RootInstance::insertDecal(Decal *d, RBX::InsertMode insertMode)
{
	Workspace *ws = ServiceProvider::find<Workspace>(this);
	if (ws) ws->startDecalDrag(d, insertMode);
}

void RootInstance::insertSpawnLocation(SpawnLocation *s)
{
	// Checks to see if this is a team spawner - if so, it makes a Teams service and a Team object
	if (s->neutral) return;

	Teams *teams = ServiceProvider::create<Teams>(this);

	if (teams->teamExists(s->getTeamColor())) return;

	shared_ptr<Team> t = Creatable<Instance>::create<Team>();
	t->setParent(teams);
	t->setTeamColor(s->getTeamColor());
	t->setName(s->getTeamColor().name() + " Team");
}

void RootInstance::insertHopperBin(HopperBin* bin)
{
	if (RBX::Network::Player* player = RBX::Network::Players::findLocalPlayer(this))
	{
		bin->setParent(player->getPlayerBackpack());
	}
	else
	{
		bin->setParent(ServiceProvider::create<StarterPackService>(this));
	}
}

void RootInstance::removeInstances(const Instances& instances)
{
	RBXASSERT(instances.size() > 0);

}
void RootInstance::insertPasteInstances(
		const Instances& instances,
		Instance* requestedParent,
		InsertMode insertMode,				// RAW: don't move, TREE: only move up, 3D_View: insert point
		PromptMode promptMode,
		const Vector3* positionHint,
		Instances* remaining,
		bool lerpCameraInStudio)
{
	doInsertInstances(instances,
		requestedParent,
		insertMode,
		promptMode,
		positionHint, 
		remaining, 
		true,
		lerpCameraInStudio);
}

void RootInstance::doInsertInstances(const Instances& instances,
										Instance* requestedParent,
										InsertMode insertMode,				// RAW: don't move, TREE: only move up, 3D_View: insert point
										PromptMode promptMode,
										const Vector3* positionHint,
										Instances* remaining,
										bool forceSuppressMove,
										bool lerpCameraInStudio)
{
	RBXASSERT(instances.size() > 0);

	// The 'remaining' argument is optional. If the caller doesn't want
	// a list of remaining elements, then store that Instances local to this
	// function.
	Instances internalOnlyRemaining;
	if (!remaining) {
		remaining = &internalOnlyRemaining;
	}

	// Requested parent, and not in the workspace ---> hardcore insert
	if (requestedParent && !this->contains(requestedParent))
	{
        //if service is present then do not insert.
        for (size_t i = 0; i < instances.size(); ++i)
        {
            Instance* pInstance = instances[i].get();
            if (!pInstance)
                continue;
            if(dynamic_cast<RBX::Service*>(pInstance))
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Do Menu Insert->Service, to insert a Service");
            else
                pInstance->setParent(requestedParent);
        }
	}
	else
	{
		bool createWaypoint = false;
		// 1.  Peel off all items that should not be in the workspace ever
		for (size_t i = 0; i < instances.size(); ++i)
		{
			Instance* instance = instances[i].get();
			if (Sky* sky = Instance::fastDynamicCast<Sky>(instance)) 
			{
				Lighting* lighting = ServiceProvider::create<Lighting>(this);
				lighting->replaceSky(sky);
				createWaypoint = true;
			}
			else if (Instance::fastDynamicCast<Team>(instance)) 
			{
				instance->setParent(ServiceProvider::create<Teams>(this));
				createWaypoint = true;
			}
			else if (HopperBin* bin = Instance::fastDynamicCast<HopperBin>(instance))
			{
				insertHopperBin(bin);
				createWaypoint = true;
			}
			else if (SpawnLocation *loc = Instance::fastDynamicCast<SpawnLocation>(instance))
			{
				insertSpawnLocation(loc);
				remaining->push_back(instances[i]); // don't peel of SpawnLocation, it belongs in the workspace
				createWaypoint = true;
			}
			else if(dynamic_cast<RBX::Service*>(instance))// Do not allow insertion of services from Toolbox or any other place. Insert Service Dialog does not call this fn so it is fine.
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Do Menu Insert->Service, to insert a Service");
			}
			else
			{
				Decal *decal = Instance::fastDynamicCast<Decal>(instance);
				// set parent
				if (decal)
				{
					decal->setParent(requestedParent);
					if (insertMode != INSERT_TO_3D_VIEW)
					{
						// invoke Decal tool is we are inserting decal into a part
						if (Instance::fastDynamicCast<RBX::PartInstance>(requestedParent))
							insertDecal(decal, insertMode);
						return;	// Skip the default processing
					}
					else if (insertMode == INSERT_TO_3D_VIEW)
					{
						insertDecal(decal, insertMode); // create decal drag mouse command if this is a toolbox insert.
						return;	// Skip the default processing
					}
				}

				remaining->push_back(instances[i]);
				createWaypoint = true;
			}
		}
		
		if (!remaining->empty())
		{
			// 3.  If no requested parent, then it's the workspace
			requestedParent = (requestedParent == NULL) ? this : requestedParent;

			// 4.  Do the insert
			switch (insertMode)
			{
			case INSERT_RAW:
				{
					PartArray partArray;
					insertRaw(*remaining, requestedParent, partArray);
					break;
				}
			case INSERT_TO_TREE:
				{
					insertToTree(*remaining, requestedParent, forceSuppressMove, lerpCameraInStudio);
					break;
				}
			case INSERT_TO_3D_VIEW:
				{
					bool suppressMove = forceSuppressMove;
					if(!suppressMove)
					{
						if(Workspace *ws = ServiceProvider::find<Workspace>(this))
						{
							ServiceClient< Selection > selection(ws);
							shared_ptr<const Instances> selectionItems = selection->getSelection2();
							if(selectionItems)
								suppressMove = ( (int) selectionItems.get()->size() ) > 0;
						}
					}
					insert3dView(*remaining, promptMode, suppressMove, positionHint, lerpCameraInStudio);
					break;
				}
			}
		}

		if (createWaypoint)
			ChangeHistoryService::requestWaypoint("Insert", this);
	}
}

void RootInstance::insertInstances(
	const Instances& instances,
	Instance* requestedParent,
	InsertMode insertMode,				// RAW: don't move, TREE: only move up, 3D_View: insert point
	PromptMode promptMode,
	const Vector3* positionHint,
	Instances* remaining,
	bool lerpCameraInStudio)
{
	doInsertInstances(instances,
		requestedParent,
		insertMode,
		promptMode,
		positionHint, 
		remaining, 
		false,
		lerpCameraInStudio);
}

}  // namespace RBX
