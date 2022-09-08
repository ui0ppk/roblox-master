/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/ToolsModel.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/ChangeHistory.h"
#include "Util/SoundService.h"
#include "SelectState.h"

namespace RBX {

const char* const sAnchorTool = "Anchor";
const char* const sLockTool = "Lock";

ModelTool::ModelTool(Workspace* workspace) :
	MouseCommand(workspace)
{
	FASTLOG1(FLog::MouseCommandLifetime, "ModelTool created: %p", this);
}

ModelTool::~ModelTool()
{
	FASTLOG1(FLog::MouseCommandLifetime, "ModelTool destroyed: %p", this);
}

void ModelTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	PartInstance* hitPart = getPart(workspace, inputObject);
	if (hitPart)
	{
		topInstance = shared_from(Workspace::findTopInstance(hitPart));
	}
	else 
	{
		topInstance.reset();
	}
}


void ModelTool::render3dAdorn(Adorn* adorn)
{
	Super::render3dAdorn(adorn);

	if (topInstance.get())
	{
		IAdornable* topRenderable = dynamic_cast<IAdornable*>(topInstance.get());
		if (topRenderable)
		{
			topRenderable->render3dSelect(adorn, SELECT_NORMAL);
		}
	}			
}


struct AnchorNode
{
	bool set;
	AnchorNode(bool set):set(set) {}

	void operator()(shared_ptr<Instance> instance) {
		PartInstance* pi = instance->fastDynamicCast<PartInstance>();
		if (pi!=NULL) {
			pi->setAnchored(set);
			return;
		}

		// try children instead
		instance->visitChildren(*this);
	}
};

void anchorAllChildren(shared_ptr<Instance> root, const bool& set) 
{
	AnchorNode anchor(set);
	anchor(root);
}

static Instance* HasUnAnchoredNode(Instance* instance) 
{
	PartInstance* pi = Instance::fastDynamicCast<PartInstance>(instance);
	if ((pi != NULL) && !pi->getAnchored()) {
		return pi;
	}

	// try children instead
	shared_ptr<const Instances> c(instance->getChildren().read());
	if (c)
	{
		Instances::const_iterator end = c->end();
		for (Instances::const_iterator iter = c->begin(); iter!=end; ++iter)
			if (HasUnAnchoredNode(iter->get()))
				return iter->get();
	}

	return NULL;
}

bool  AnchorTool::allChildrenAnchored(Instance* test) 
{
	Instance* answer = HasUnAnchoredNode(test);
	return (answer == NULL);
}

void AnchorTool::onMouseHover(const shared_ptr<InputObject>& inputObject)
{
	shared_ptr<Instance> oldInstance = topInstance;

	ModelTool::onMouseHover(inputObject);

	if (oldInstance != topInstance) {
		allAnchored = topInstance 
							? allChildrenAnchored(topInstance.get())
							: false;
	}
}

const std::string AnchorTool::getCursorName() const
{
	if (topInstance.get()) {
		if (allAnchored) {
			return "UnAnchorCursor";
		}
		else {
			return "AnchorCursor";
		}
	}			
	return "AnchorCursor";
}



shared_ptr<MouseCommand> AnchorTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	if (topInstance.get() && !PartInstance::getLocked(topInstance.get()))
	{
		// Do it here
		anchorAllChildren(topInstance, !allAnchored);
		ChangeHistoryService::requestWaypoint(sAnchorTool, workspace);

		allAnchored = allChildrenAnchored(topInstance.get());
	}
	return shared_from(this);
}

const std::string LockTool::getCursorName() const
{
	if (topInstance) {
		if (PartInstance::getLocked(topInstance.get())) {
			return "UnlockCursor";
		}
		else {
			return "LockCursor";
		}
	}			
	return "LockCursor";
}

shared_ptr<MouseCommand> LockTool::onMouseDown(const shared_ptr<InputObject>& inputObject)
{
	if (topInstance) {
		
		// Do it here
		bool wasLocked = PartInstance::getLocked(topInstance.get());
		PartInstance::setLocked(topInstance.get(), !wasLocked);
		ChangeHistoryService::requestWaypoint(sLockTool, workspace);
	}
	return shared_from(this);
}




} // namespace