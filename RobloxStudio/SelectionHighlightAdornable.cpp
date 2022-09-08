#include "stdafx.h"

#include "SelectionHighlightAdornable.h"

#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"

#include "AppDraw/Draw.h"
#include "V8DataModel/Accoutrement.h"
#include "V8DataModel/FaceInstance.h"
#include "V8DataModel/MegaCluster.h"
#include "V8DataModel/ModelInstance.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Tool.h"
#include "V8DataModel/Workspace.h"

FASTFLAG(StudioDE6194FixEnabled)

using namespace RBX;

bool SelectionHighlightAdornable::getSelectionDimensions(Workspace* workspace,
	shared_ptr<Instance> instance, Part* out, float* lineThickness, bool* checkChildren)
{
	*checkChildren = false;

	if (!workspace->contains(instance.get()) ||
		Instance::fastSharedDynamicCast<MegaClusterInstance>(instance))
	{
		return false;
	}

	bool shouldRender = false;

	if (shared_ptr<ModelInstance> model = Instance::fastSharedDynamicCast<ModelInstance>(instance))
	{
		if (FFlag::StudioDE6194FixEnabled && !model->isSelectable3d())
		{
			return false;
		}

		*out = model->computePart();
		*lineThickness = .05;
		shouldRender = true;
	}
	else if (shared_ptr<FaceInstance> faceInstance = Instance::fastSharedDynamicCast<FaceInstance>(instance))
	{
		if (PartInstance* partParent = Instance::fastDynamicCast<PartInstance>(faceInstance->getParent()))
		{			
			Part tmp = partParent->getPart();
			Vector3 halfSize = tmp.gridSize / 2;
			float points[6] = { halfSize.x, halfSize.y, halfSize.z,
				-halfSize.x, -halfSize.y, -halfSize.z };
			NormalId nid = faceInstance->getFace();
			points[(nid + 3)  % 6] = points[nid];
			Vector3 maxExt(points[0], points[1], points[2]);
			Vector3 minExt(points[3], points[4], points[5]);
			Vector3 positionOffset = Vector3::zero();
			positionOffset[nid % 3] = maxExt[nid % 3];
					
			tmp.coordinateFrame.translation += tmp.coordinateFrame.rotation * positionOffset;
			tmp.gridSize = maxExt - minExt;
			*out = tmp;
			*lineThickness = .2;
			shouldRender = true;
		}
	}
	else if (shared_ptr<PartInstance> part = Instance::fastSharedDynamicCast<PartInstance>(instance))
	{
		*out = part->getPart();
		*lineThickness = .02;
		shouldRender = true;
	}
	else if (Instance::fastSharedDynamicCast<Tool>(instance) ||
		Instance::fastSharedDynamicCast<Accoutrement>(instance))
	{
		*checkChildren = true;
	}

	return shouldRender;
}

void SelectionHighlightAdornable::render(shared_ptr<Instance> instance, Part part,
	float lineThickness, Adorn* adorn)
{
	bool isFaceInstance = Instance::fastSharedDynamicCast<FaceInstance>(instance) != NULL;
	Draw::selectionBox(part, adorn, isFaceInstance ? Color3::orange() : Draw::selectColor(),
		lineThickness);
	if (shared_ptr<ModelInstance> m = Instance::fastSharedDynamicCast<ModelInstance>(instance))
	{
		if (PartInstance* primary = m->getPrimaryPartSetByUser())
		{
			Draw::selectionBox(primary->getPart(), adorn,
				ModelInstance::primaryPartSelectColor(),
				ModelInstance::primaryPartLineThickness());
		}
	}
}

void SelectionHighlightAdornable::render3dAdorn(Adorn* adorn)
{
	DataModel* dm = NULL;
	if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
	{
		dm = ideDoc->getDataModel();
	}

	RBXASSERT(dm);
	if (!dm)
	{
		return;
	}

	if (Selection* selection = dm->find<Selection>())
	{
		renderSelection(dm, *selection, adorn, &SelectionHighlightAdornable::render);
	}
}
