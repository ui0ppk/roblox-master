/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Commands.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/Feature.h"
#include "V8DataModel/changehistory.h"
#include "V8DataModel/MegaCluster.h"
#include "V8DataModel/ToolsModel.h"
#include "Humanoid/Humanoid.h"
#include "Network/Players.h"
#include "Tool/MegaDragger.h"
#include "V8World/World.h"
#include "V8Xml/Serializer.h"
#include "V8Xml/SerializerBinary.h"
#include "Util/SoundService.h"
#include "Util/profiling.h"
#include "V8World/ContactManager.h"
#include "V8DataModel/CSGDictionaryService.h"
#include "Script/LuaSourceContainer.h"

FASTFLAG(DebugDisplayFPS)

FASTFLAG(UserAllCamerasInLua)

namespace RBX {

BoolPropertyVerb::BoolPropertyVerb(
	const std::string& name, 
	DataModel* dataModel, 
	const char* propertyName ) :
EditSelectionVerb(name, dataModel),
propertyName(Name::declare(propertyName))
{}


static bool HasTrueProperty(const char* propertyName, shared_ptr<Instance> instance)
{
	if (Reflection::PropertyDescriptor* desc = instance->findPropertyDescriptor(propertyName))
	{
		Reflection::Property prop(*desc, instance.get());
		if (prop.isValueType<bool>())
			if (prop.getValue<bool>())
				return true;
	}
	return false;
}



bool BoolPropertyVerb::isChecked() const
{
	return std::find_if(
		selection->begin(), 
		selection->end(), 
		boost::bind(&HasTrueProperty, propertyName.c_str(), _1)
		)!=selection->end();
}



class BoolPropertyVerbSetIt
{
	const Name& propertyName;
	bool newState;
public:
	BoolPropertyVerbSetIt(const Name& propertyName, bool newState)
		: propertyName(propertyName)
		, newState(newState)
	{}
	void operator()(shared_ptr<Instance> instance)
	{
		if (Reflection::PropertyDescriptor* desc = instance->findPropertyDescriptor(propertyName.c_str()))
		{
			Reflection::Property prop(*desc, instance.get());
			if (prop.isValueType<bool>() && prop.getValue<bool>()!=newState)
			{
				prop.setValue(newState);
			}
		}
	}
};



void BoolPropertyVerb::doIt(IDataState* dataState)
{
	// TODO: refactor using boost::bind
	BoolPropertyVerbSetIt setIt(propertyName, !isChecked());
	std::for_each(selection->begin(), selection->end(), setIt);

	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		ChangeHistoryService::requestWaypoint(propertyName.c_str(), dataModel);
	}

	dataState->setDirty(true);
}

CameraCenterCommand::CameraCenterCommand(Workspace* workspace) : CameraVerb("CameraCenter", workspace) 
{
	// Force creation here, so that we don't create a service in isEnabled
	ServiceProvider::create< FilteredSelection<PVInstance> >(workspace);
}

// TODO:: consolidate this with other camera enabled stuff
bool CameraCenterCommand::isEnabled() const
{
	if (ServiceClient< FilteredSelection<PVInstance> >(workspace)->size() >= 1) {
		const Camera* worldCamera = workspace->getConstCamera();
		RBXASSERT(worldCamera);
		if (worldCamera) {
			if (worldCamera->getCameraType() == Camera::FIXED_CAMERA) {
				return true;
			}
		}
	}
	return false;
}


void CameraCenterCommand::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:CameraCenter");

	RBXASSERT(isEnabled());

	Vector3 center;
	
	ServiceClient< FilteredSelection<PVInstance> > sel(workspace);
	bool hasFocalObject = sel->size() >= 1;

	if (hasFocalObject) {
		const std::vector<PVInstance*>& items(sel->items());
		Extents extents = items[0]->computeExtentsWorld();
		for (size_t i = 1; i < items.size(); ++i) {
			extents.unionWith(items[i]->computeExtentsWorld());
		}
		center = extents.center();
	}
	else {
		center = Vector3::zero();
	}

	workspace->getCamera()->setCameraType(Camera::FIXED_CAMERA);
	workspace->getCamera()->setCameraFocusAndMaintainFocus(center, hasFocalObject);
	CameraVerb::doIt(dataState);

	// TODO: Undo/Redo
	dataState->setDirty(true);
}


void SelectAllCommand::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:SelectAll");

	dataModel->getWorkspace()->selectAllTopLevelRenderable();
}

void AllCanSelectCommand::doIt(IDataState* dataState)
{
	Workspace* workspace = dataModel->getWorkspace();
	PartInstance::setLocked(workspace, false);			// complete traversal

	// TODO: Undo/Redo
	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
	}

	dataState->setDirty(true);
}

static void SetCanNotSelect(shared_ptr<Instance> pv)
{
	PartInstance::setLocked(pv.get(), true);
}

void CanNotSelectCommand::doIt(IDataState* dataState)
{
	FilteredSelection<PVInstance>* sel = ServiceProvider::create< FilteredSelection<PVInstance> >(dataModel);
	std::for_each(selection->begin(), selection->end(), SetCanNotSelect);

	sel->clearSelection();

	// TODO: Undo/Redo
	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
	}

	dataState->setDirty(true);
}

std::string ChatMenuCommand::getChatString(int menu1, int menu2, int menu3)
{
	return	"Chat_"	+ StringConverter<int>::convertToString(menu1)
			+ "_"	+ StringConverter<int>::convertToString(menu2)
			+ "_"	+ StringConverter<int>::convertToString(menu3);
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

FirstPersonCommand::FirstPersonCommand(DataModel* dataModel)
: Verb(dataModel, "FirstPersonIndicator")
, dataModel(dataModel)
{
}

bool FirstPersonCommand::isEnabled() const
{
	if (Humanoid* h = Humanoid::getLocalHumanoidFromContext(dataModel)) {
		return h->isFirstPerson();
	}
	else {
		return false;
	}
}

ToggleViewMode::ToggleViewMode(RBX::DataModel* dm)
: Verb(dm, "ToggleViewMode")
, dataModel(dm)
{
}

bool ToggleViewMode::isChecked() const
{
	return false;
}

bool ToggleViewMode::isEnabled() const
{
	return dataModel->getNumPlayers() > 0;
}

bool ToggleViewMode::isSelected() const
{
	return false;
}

void ToggleViewMode::doIt(RBX::IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:ToggleViewMode");
}

/////////////////////////////////////////////////////
/////////////////////////////////////////////////////

StatsCommand::StatsCommand(DataModel* dataModel) :
	Verb(dataModel, "Stats"),
	dataModel(dataModel)
{}


void StatsCommand::doIt(IDataState* dataState)
{
    dataModel->getGuiBuilder().toggleGeneralStats();
}

bool StatsCommand::isEnabled() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	return Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("StatsHud1"))!=NULL;
}

bool StatsCommand::isChecked() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	const TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("StatsHud1"));
	return statsMenu!=NULL && statsMenu->isVisible();
}


RenderStatsCommand::RenderStatsCommand(DataModel* dataModel) :
Verb(dataModel, "RenderStats"),
dataModel(dataModel)
{}


void RenderStatsCommand::doIt(IDataState* dataState)
{
	dataModel->getGuiBuilder().toggleRenderStats();
}

bool RenderStatsCommand::isEnabled() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	return Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("StatsHud1"))!=NULL;
}

bool RenderStatsCommand::isChecked() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	const TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("RenderStats"));
	return statsMenu!=NULL && statsMenu->isVisible();
}


SummaryStatsCommand::SummaryStatsCommand(DataModel* dataModel) :
Verb(dataModel, "SummaryStats"),
dataModel(dataModel)
{}

void SummaryStatsCommand::doIt(IDataState* dataState)
{
    dataModel->getGuiBuilder().toggleSummaryStats();
}

bool SummaryStatsCommand::isEnabled() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	return Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("SummaryStats"))!=NULL;
}

bool SummaryStatsCommand::isChecked() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	const TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("SummaryStats"));
	return statsMenu!=NULL && statsMenu->isVisible();
}


CustomStatsCommand::CustomStatsCommand(DataModel* dataModel) :
Verb(dataModel, "CustomStats"),
dataModel(dataModel)
{}

void CustomStatsCommand::doIt(IDataState* dataState)
{
	dataModel->getGuiBuilder().toggleCustomStats();
}

bool CustomStatsCommand::isEnabled() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	return Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("CustomStats"))!=NULL;
}

bool CustomStatsCommand::isChecked() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	const TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("CustomStats"));
	return statsMenu!=NULL && statsMenu->isVisible();
}


NetworkStatsCommand::NetworkStatsCommand(DataModel* dataModel) :
Verb(dataModel, "NetworkStats"),
dataModel(dataModel)
{
}


void NetworkStatsCommand::doIt(IDataState* dataState)
{
	dataModel->getGuiBuilder().toggleNetworkStats();
}

bool NetworkStatsCommand::isEnabled() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	return Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("NetworkStats"))!=NULL;
}

bool NetworkStatsCommand::isChecked() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	const TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("NetworkStats"));
	return statsMenu!=NULL && statsMenu->isVisible();
}

////////////////////////////////////////////////////////////////////////////

PhysicsStatsCommand::PhysicsStatsCommand(DataModel* dataModel) :
Verb(dataModel, "PhysicsStats"),
dataModel(dataModel)
{}

void PhysicsStatsCommand::doIt(IDataState* dataState)
{
    dataModel->getGuiBuilder().togglePhysicsStats();
}

bool PhysicsStatsCommand::isEnabled() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	return Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("PhysicsStats")) != NULL ||
        Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("PhysicsStats2")) != NULL;
}

bool PhysicsStatsCommand::isChecked() const
{
	GuiRoot* guiRoot = dataModel->getGuiRoot();
	const TopMenuBar* statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("PhysicsStats"));
	if ( statsMenu != NULL && statsMenu->isVisible() )
        return true;

    statsMenu = Instance::fastDynamicCast<TopMenuBar>(guiRoot->findFirstChildByName("PhysicsStats2"));
    if ( statsMenu != NULL && statsMenu->isVisible() )
        return true;

    return false;
}

////////////////////////////////////////////////////////////////////////////

EngineStatsCommand::EngineStatsCommand(DataModel* dataModel) :
	Verb(dataModel, "EngineStats")
	, dataModel(dataModel)
{}


void EngineStatsCommand::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:EngineStats");

//	dataModel->getWorkspace()->getWorld()->getContactManager().getSpatialHash().doStats();
	dataModel->getWorkspace()->getWorld()->getContactManager()->doStats();
}

////////////////////////////////////////////////////////////////////////////

JoinCommand::JoinCommand(DataModel* dataModel) 
	: Verb(dataModel, "JoinCommand")
	, dataModel(dataModel)
{}

bool JoinCommand::isEnabled() const
{
	Selection* sel = ServiceProvider::find< Selection >(dataModel);
	if (sel && sel->size() == 2)
	{
		shared_ptr<Instance> i0 = sel->front();
		shared_ptr<Instance> i1 = sel->back();
		if (MotorFeature::canJoin(i0.get(), i1.get()))
		{
			return true;
		}
	}
	return false;
}

void JoinCommand::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:JoinCommand");

	Selection* sel = ServiceProvider::create< Selection >(dataModel);
	if (sel->size() == 2)
	{
		shared_ptr<Instance> i0 = sel->front();
		shared_ptr<Instance> i1 = sel->back();
		MotorFeature::join(i0.get(), i1.get());
	}
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////


RunStateVerb::RunStateVerb(std::string name, DataModel* dataModel, bool blacklisted) : 
	Verb(dataModel, name, blacklisted),
	dataModel(dataModel),
	runService(dataModel)
{
}

RunStateVerb::~RunStateVerb()
{
}

void RunStateVerb::playActionSound()
{
}

bool RunCommand::isEnabled() const
{
	RunState runState = runService->getRunState();
	bool clientIsPresent = RBX::Network::Players::clientIsPresent(dataModel);

	return (!clientIsPresent && (runState != RS_RUNNING));
}

void RunCommand::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:Run");

	RBXASSERT(isEnabled());
	runService->run();
}

bool StopCommand::isEnabled() const
{
	RunState runState = runService->getRunState();
	return (runState == RS_RUNNING);
}

void StopCommand::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:Stop");

	RBXASSERT(isEnabled());
	runService->pause();
}

bool ResetCommand::isEnabled() const
{
	RunState runState = runService->getRunState();

	return		(		(runState == RS_RUNNING ||	runState == RS_PAUSED)
					&&	!RBX::Network::Players::findLocalPlayer(dataModel)	);
}

void ResetCommand::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:Reset");

	RBXASSERT(isEnabled());
	runService->stop();
}


///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

EditSelectionVerb::EditSelectionVerb(std::string name, DataModel* dataModel)
:Verb(dataModel, name)
,dataModel(dataModel)
,workspace(shared_from(dataModel->getWorkspace()))
,selection(dataModel)
{
}

EditSelectionVerb::EditSelectionVerb(VerbContainer* container, std::string name, DataModel* dataModel)
:Verb(container, name)
,dataModel(dataModel)
,workspace(shared_from(ServiceProvider::find<Workspace>(dataModel)))
,selection(dataModel)
{
	RBXASSERT(workspace!=NULL);
}

EditSelectionVerb::~EditSelectionVerb()
{
}

bool EditSelectionVerb::isEnabled() const
{
	return selection->size()>0;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
template<SurfaceType from, SurfaceType to>
static void SurfaceSwap(shared_ptr<Instance> instance)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance.get())) {
		for (int i = 0; i < 6; ++i) {
			if (part->getSurfaceType(NormalId(i)) == from) {
                part->setSurfaceType(NormalId(i), to);
			}
		}
	}
	if (instance->getChildren())
		std::for_each(
			instance->getChildren()->begin(),
			instance->getChildren()->end(),
			boost::bind(&SurfaceSwap<from, to>, _1)
			);
}

SnapSelectionVerb::SnapSelectionVerb(DataModel* dataModel) :
	EditSelectionVerb("SnapSelection", dataModel)
{;}


bool SnapSelectionVerb::isEnabled() const
{
	if (!Super::isEnabled())
		return false;
	Selection* sel = ServiceProvider::find< Selection >(dataModel);
	return (sel && sel->size() > 0);
}

void SnapSelectionVerb::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:SnapSelection");

	ServiceProvider::create< Selection >(dataModel);
	std::for_each(selection->begin(), selection->end(), boost::bind(&SurfaceSwap<WELD, STUDS>, _1));

	// TODO: Undo/Redo...
	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
	}

	dataState->setDirty(true);
}

void UnlockAllVerb::doIt(IDataState* dataState)
{
	Workspace* workspace = dataModel->getWorkspace();
	PartInstance::setLocked(workspace, false);			// complete traversal

	// TODO: Undo/Redo
	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
	}

	dataState->setDirty(true);
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

SelectChildrenVerb::SelectChildrenVerb(DataModel* dataModel) :
	EditSelectionVerb("SelectChildrenOfSelection", dataModel)
{
	ServiceProvider::create<FilteredSelection<Instance> >(dataModel);
}

bool hasChildren(Instance* wInstance)
{
	return (wInstance->numChildren() > 0);
}

bool SelectChildrenVerb::isEnabled() const
{
	if(!Super::isEnabled())
		return false;

	FilteredSelection<Instance>* selection = ServiceProvider::find<FilteredSelection<Instance> >(dataModel);
	return selection && selection->find_if(hasChildren)!=NULL;
}

void SelectChildrenVerb::doIt(IDataState *dataState)
{
	FASTLOG(FLog::Verbs, "Gui:SelectChildrenOfSelection");

	Selection* selection = ServiceProvider::find<Selection>(dataModel);

	shared_ptr<RBX::Instances> childItems(new Instances);

	if (shared_ptr<const Instances> parentItems = selection->getSelection2())
		for (std::vector<shared_ptr<Instance> >::const_iterator parentIterator = parentItems->begin(); parentIterator != parentItems->end(); ++parentIterator)
			if (shared_ptr<const Instances> children = (*parentIterator)->getChildren2())
				for (Instances::const_iterator childIterator = children->begin(); childIterator != children->end(); ++childIterator)
					childItems->push_back(*childIterator);

	if(childItems->size() > 0)
		selection->setSelection(childItems);
	else
		debugAssertM(0, "Calling SelectChildrenOfSelection command without checking is-enabled");
}

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

void AddChildToRoot(XmlElement* root, shared_ptr<Instance> wsi, const boost::function<bool(Instance*)>& isInScope, RBX::CreatorRole creatorRole)
{
	if (XmlElement* child = wsi->writeXml(isInScope, creatorRole))
		root->addChild(child);
}

static bool isInScope(shared_ptr<Instance> selectedItem, Instance* child)
{
	if (selectedItem.get() == child)
		return true;
	return selectedItem->isAncestorOf(child);
}

static bool isContainedInSelection(Selection* sel, Instance* instance)
{
	return std::find_if(
		sel->begin(), sel->end(), 
		boost::bind(isInScope, _1, instance)) 
		!= sel->end();
}

void AddSelectionToRoot(XmlElement* root, Selection* selection, RBX::CreatorRole creatorRole)
{
	boost::function<bool(Instance*)> scopeFunction = boost::bind(isContainedInSelection, selection, _1);
	std::for_each(selection->begin(), selection->end(), boost::bind(&AddChildToRoot, root, _1, scopeFunction, creatorRole));
}

DeleteBase::DeleteBase(VerbContainer* verbContainer, DataModel* dataModel, std::string name)
	: EditSelectionVerb(verbContainer, name, dataModel)
	, rewardHopper(false)
{}

DeleteBase::DeleteBase(DataModel* dataModel, std::string name, bool rewardHopper)
	: EditSelectionVerb(name, dataModel)
	, rewardHopper(rewardHopper)
{}

static void checkForLockedScript(const shared_ptr<Instance>& descendant)
{
	if (const shared_ptr<LuaSourceContainer> lsc =
		Instance::fastSharedDynamicCast<LuaSourceContainer>(descendant))
	{
		if (Instance* editor = lsc->getCurrentEditor())
		{
			throw std::runtime_error(format("Cannot perform operation: %s is currently being edited by %s",
				lsc->getFullName().c_str(), editor->getName().c_str()));
		}
	}
}

void DeleteBase::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:DeleteBase");
	
	Selection* sel = ServiceProvider::create< Selection >(dataModel);

	bool isCloudEdit = Network::Players::isCloudEdit(dataModel);

	if (sel->size() > 0)
	{
		shared_ptr<const Instances> selectionRead = sel->getSelection().read();

		if (selectionRead)
		{
			// Check for locked scripts that would be deleted, and skip the delete if they are present
			if (isCloudEdit)
			{
				for (const auto& instance : *selectionRead)
				{
					checkForLockedScript(instance);
					instance->visitDescendants(boost::bind(&checkForLockedScript, _1));
				}
			}
			sel->setSelection(shared_ptr<const Instances>());

			for (Instances::const_iterator it = selectionRead->begin(); it != selectionRead->end(); ++it)
			{
				shared_ptr<Instance> instance = *it;
				Instance* iPtr = instance.get();
				if (iPtr)
				{
					if (!iPtr->fastDynamicCast<Workspace>() && !iPtr->fastDynamicCast<Network::Players>() && !dynamic_cast<Service*>(iPtr)
						&& (!isCloudEdit || !iPtr->fastDynamicCast<Network::Player>())) {
						instance->setParent(NULL);
						//if camera is getting deleted then create new camera here itself (instead of Workspace::onHeartbeat) to correctly record change history
						if (iPtr->fastDynamicCast<RBX::Camera>())
							dataModel->getWorkspace()->replenishCamera();
					}
				}
			}
		}

		{
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
		}

		dataState->setDirty(true);
	}
}

// TODO: validate that group, followed by ungroup is completely consistent.....
void RotateAxisCommand::rotateAboutAxis(const G3D::Matrix3& rotMatrix, const std::vector<PVInstance*>& selectedInstances)
{
	if (selectedInstances.empty()) {
		debugAssertM(0, "Command should be disabled here");
		return;
	}
	else {
		MegaDragger megaDragger(NULL, selectedInstances, workspace.get());
		megaDragger.startDragging();
		megaDragger.safeRotate(rotMatrix);
		megaDragger.finishDragging();
	}
}


RotateSelectionVerb::RotateSelectionVerb(DataModel* dataModel) :
	RotateAxisCommand("SelectionRotate", dataModel)
{;}

void RotateAxisCommand::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:RotateAxis");

	FilteredSelection<PVInstance>* sel = ServiceProvider::create< FilteredSelection<PVInstance> >(dataModel);

	rotateAboutAxis(getRotationAxis(), sel->items());

	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		ChangeHistoryService::requestWaypoint(getName().c_str(), dataModel);
	}

	dataState->setDirty(true);
}

G3D::Matrix3 RotateSelectionVerb::getRotationAxis()
{	
	return Math::matrixRotateY();
}

TiltSelectionVerb::TiltSelectionVerb(DataModel* dataModel) :
	RotateAxisCommand("SelectionTilt", dataModel)
{;}

G3D::Matrix3 TiltSelectionVerb::getRotationAxis()
{
	int currentQuadrant = Math::toYAxisQuadrant(workspace->getConstCamera()->coordinateFrame());

	return Math::matrixTiltQuadrant(currentQuadrant);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void MoveUpSelectionVerb::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:MoveUpSelection");

	FilteredSelection<PVInstance>* sel = ServiceProvider::create< FilteredSelection<PVInstance> >(dataModel);

	if (sel->size()>0) {
		MegaDragger megaDragger(NULL, sel->items(), workspace.get());
		megaDragger.startDragging();
		megaDragger.safeMoveNoDrop(Vector3(0.0f, moveUpHeight, 0.0f));
		megaDragger.finishDragging();

		{
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
		}

		dataState->setDirty(true);
	}
}

MoveDownSelectionVerb::MoveDownSelectionVerb(DataModel* dataModel) :
	EditSelectionVerb("SelectionDown", dataModel)
{;}

void MoveDownSelectionVerb::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:MoveDownSelection");

	FilteredSelection<PVInstance>* sel = ServiceProvider::create< FilteredSelection<PVInstance> >(dataModel);

	if (sel->size()>0) {
		MegaDragger megaDragger(NULL, sel->items(), workspace.get());
		megaDragger.startDragging();
		megaDragger.safeMoveNoDrop(Vector3(0.0f, -1.2f, 0.0f));
		megaDragger.finishDragging();

		{
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
		}

		dataState->setDirty(true);
	}
}



bool CharacterCommand::isEnabled() const
{
	return false;
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////
// CAMERA

CameraVerb::CameraVerb(std::string name, Workspace* _workspace) 
	: Verb(_workspace, name)
	, workspace(_workspace)	
	, selection(_workspace)
{}


Camera* CameraVerb::getCamera()
{
	return workspace->getCamera();
}

const Camera* CameraVerb::getCamera() const
{
	return workspace->getConstCamera();
}

void CameraVerb::doIt(IDataState* dataState)
{
	RBXASSERT(dataState!=NULL);

	dataState->setDirty(true);
}

bool CameraPanLeftCommand::isEnabled() const {return true;}
void CameraPanLeftCommand::doIt(IDataState* dataState)		{FASTLOG(FLog::Verbs, "Gui:CameraPanLeft"); getCamera()->panUnits(-1); CameraVerb::doIt(dataState);}

bool CameraPanRightCommand::isEnabled() const 	{return true;}
void CameraPanRightCommand::doIt(IDataState* dataState)		{FASTLOG(FLog::Verbs, "Gui:CameraPanRight"); getCamera()->panUnits(1); CameraVerb::doIt(dataState);}

bool CameraTiltUpCommand::isEnabled() const 	{return getCamera()->canTilt(-1);}
void CameraTiltUpCommand::doIt(IDataState* dataState)		{FASTLOG(FLog::Verbs, "Gui:CameraTiltUp"); if (getCamera()->tiltUnits(-1)) CameraVerb::doIt(dataState);}

bool CameraTiltDownCommand::isEnabled() const 	{return getCamera()->canTilt(1);}
void CameraTiltDownCommand::doIt(IDataState* dataState)		{FASTLOG(FLog::Verbs, "Gui:CameraTildDown"); if (getCamera()->tiltUnits(1)) CameraVerb::doIt(dataState);}

bool CameraZoomInCommand::isEnabled() const 	{return getCamera()->canZoom(true);}
void CameraZoomInCommand::doIt(IDataState* dataState)		
{
	FASTLOG(FLog::Verbs, "Gui:CameraZoomIn"); 
	if (getCamera()->zoom(1)) 
		CameraVerb::doIt(dataState);
}

bool CameraZoomOutCommand::isEnabled() const 	{return getCamera()->canZoom(false);}
void CameraZoomOutCommand::doIt(IDataState* dataState)
{
	FASTLOG(FLog::Verbs, "Gui:CameraZoomOut");
	if (getCamera()->zoom(-1)) 
		CameraVerb::doIt(dataState);
}

CameraZoomExtentsCommand::CameraZoomExtentsCommand(Workspace* workspace) : CameraVerb("CameraZoomExtents", workspace) 
{
	// Force creation here, so that we don't create a service in isEnabled
	ServiceProvider::create< FilteredSelection<PVInstance> >(workspace);
}

// TODO:: consolidate this with other camera enabled stuff
bool CameraZoomExtentsCommand::isEnabled() const
{
    
	if (ServiceClient< FilteredSelection<PVInstance> >(workspace)->size() >= 1)
	{
		const Camera* worldCamera = workspace->getConstCamera();
		RBXASSERT(worldCamera);

		if (!worldCamera)
			return false;

        if (FFlag::UserAllCamerasInLua && worldCamera->hasClientPlayer())
        {
            return false;
        }
        
		RBX::Network::Players* players = ServiceProvider::create<Network::Players>(worldCamera);
		
		// If there is a local player not in fixed camera mode, dont allow zoom extents
		// Otherwise, do
		if (players && !(players->getLocalPlayer() && worldCamera->getCameraType() != Camera::FIXED_CAMERA)) 
			return true;
	}
	return false;
}
void CameraZoomExtentsCommand::doIt(IDataState* dataState )
{
	FASTLOG(FLog::Verbs, "Gui:CameraZoomExtents"); 
	RBXASSERT(isEnabled());

	ServiceClient< FilteredSelection<PVInstance> > sel(workspace);

	Extents extents(Vector3(0,0,0), Vector3(0,0,0));

	if (sel->size() >= 1)
	{
		const std::vector<PVInstance*>& items(sel->items());

		if(MegaClusterInstance* terrain = Instance::fastDynamicCast<MegaClusterInstance>(items[0]))
		{
			if(terrain->getNonEmptyCellCount() > 0)
				extents = items[0]->computeExtentsWorld();
		}
		else
			extents = items[0]->computeExtentsWorld();

		for (size_t i = 1; i < items.size(); ++i)
		{
			if(MegaClusterInstance* terrain = Instance::fastDynamicCast<MegaClusterInstance>(items[i]))
			{
				if(terrain->getNonEmptyCellCount() > 0)
					extents.unionWith(items[i]->computeExtentsWorld());
			}
			else
				extents.unionWith(items[i]->computeExtentsWorld());
		}
	}
	else
	{
		// no selection, use old behavior
		getCamera()->zoomExtents();
		CameraVerb::doIt(dataState);
		return;
	}

	if(extents != Extents(Vector3(0,0,0), Vector3(0,0,0)))
	{
		workspace->getCamera()->setCameraType(Camera::FIXED_CAMERA);
		workspace->getCamera()->zoomExtents(extents, Camera::ZOOM_IN_OR_OUT); 
		CameraVerb::doIt(dataState);

		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace);
		dataState->setDirty(true);
	}
}


// Video recording code moved to GUI


TurnOnManualJointCreation::TurnOnManualJointCreation(DataModel* dataModel)
: Verb(dataModel, "TurnOnManualJointCreation")
, dataModel(dataModel)
{
}

void TurnOnManualJointCreation::doIt(IDataState* dataState)
{
	if(AdvArrowTool::advManualJointMode)
	{	
		FASTLOG(FLog::Verbs, "Gui:TurnOffManualJointCreation"); 
		AdvArrowTool::advManualJointMode = false;
	}
	else
	{
		FASTLOG(FLog::Verbs, "Gui:TurnOnManualJointCreation"); 
		AdvArrowTool::advManualJointMode = true;
	}

	AdvArrowTool::advCreateJointsMode = true;
}

SetDragGridToOne::SetDragGridToOne(DataModel* dataModel)
: Verb(dataModel, "SetDragGridToOne")
, dataModel(dataModel)
{
}

SetDragGridToOneFifth::SetDragGridToOneFifth(DataModel* dataModel)
: Verb(dataModel, "SetDragGridToOneFifth")
, dataModel(dataModel)
{
}

SetDragGridToOff::SetDragGridToOff(DataModel* dataModel)
: Verb(dataModel, "SetDragGridToOff")
, dataModel(dataModel)
{
}

SetGridSizeToTwo::SetGridSizeToTwo(DataModel* dataModel)
: Verb(dataModel, "SetGridSizeToTwo")
, workspace(dataModel->getWorkspace())
{
}

SetGridSizeToFour::SetGridSizeToFour(DataModel* dataModel)
: Verb(dataModel, "SetGridSizeToFour")
, workspace(dataModel->getWorkspace())
{
}

SetGridSizeToSixteen::SetGridSizeToSixteen(DataModel* dataModel)
: Verb(dataModel, "SetGridSizeToSixteen")
, workspace(dataModel->getWorkspace())
{
}

SetManualJointToWeak::SetManualJointToWeak(DataModel* dataModel)
: Verb(dataModel, "SetManualJointToWeak")
, dataModel(dataModel)
{
}

SetManualJointToStrong::SetManualJointToStrong(DataModel* dataModel)
: Verb(dataModel, "SetManualJointToStrong")
, dataModel(dataModel)
{
}

SetManualJointToInfinite::SetManualJointToInfinite(DataModel* dataModel)
: Verb(dataModel, "SetManualJointToInfinite")
, dataModel(dataModel)
{
}

bool applyColor(Instance* instance, RBX::BrickColor color)
{
	bool applied = false;
	if (PartInstance* pInstance = dynamic_cast<PartInstance*>(instance))
	{
		pInstance->setColor(color);
		applied = true;
	}
	else if (ModelInstance* pModelInstance = dynamic_cast<ModelInstance*>(instance))
	{
		std::vector<PartInstance*> childParts = pModelInstance->getDescendantPartInstances();
		for (std::vector<PartInstance*>::const_iterator iter = childParts.begin(); iter != childParts.end(); ++iter)
		{
			applied |= applyColor(*iter, color);
		}
	}
	return applied;
}

RBX::BrickColor ColorVerb::m_currentColor = RBX::BrickColor::brick_194;
void ColorVerb::doIt( IDataState* dataState )
{
	bool applied = false;
	FilteredSelection<Instance>* sel = ServiceProvider::create< FilteredSelection<Instance> >(dataModel);
	for (std::vector<Instance*>::const_iterator iter = sel->begin(); iter != sel->end(); ++iter)
		applied |= applyColor(*iter, ColorVerb::getCurrentColor());

	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
	}

	dataState->setDirty(true);
}

bool applyMaterial(Instance* instance, PartMaterial material)
{
	bool applied = false;
	if (PartInstance* pInstance = dynamic_cast<PartInstance*>(instance))
	{
		pInstance->setRenderMaterial(material);
		applied = true;
	}
	else if (ModelInstance* pModelInstance = dynamic_cast<ModelInstance*>(instance))
	{
		std::vector<PartInstance*> childParts = pModelInstance->getDescendantPartInstances();
		for (std::vector<PartInstance*>::const_iterator iter = childParts.begin(); iter != childParts.end(); ++iter)
		{
			applied |= applyMaterial(*iter, material);
		}
	}
	return applied;
}
PartMaterial MaterialVerb::m_currentMaterial = PLASTIC_MATERIAL;
void MaterialVerb::doIt( IDataState* dataState )
{
	bool applied = false;
	FilteredSelection<Instance>* sel = ServiceProvider::create< FilteredSelection<Instance> >(dataModel);
	for (std::vector<Instance*>::const_iterator iter = sel->begin(); iter != sel->end(); ++iter)
		applied |= applyMaterial(*iter, MaterialVerb::getCurrentMaterial());

	{
		DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
		ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
	}

	dataState->setDirty(true);
}

PartMaterial MaterialVerb::parseMaterial(const std::string materialString)
{
	if(materialString == "Plastic")
	{
		return RBX::PLASTIC_MATERIAL;
	}
	else if(materialString == "Slate")
	{
		return RBX::SLATE_MATERIAL;
	}
	else if(materialString == "Wood")
	{
		return RBX::WOOD_MATERIAL;
	}
	else if(materialString == "Concrete")
	{
		return RBX::CONCRETE_MATERIAL;
	}
	else if(materialString =="CorrodedMetal")
	{
		return RBX::RUST_MATERIAL;
	}
	else if(materialString == "DiamondPlate")
	{
		return RBX::DIAMONDPLATE_MATERIAL;
	}
	else if(materialString == "Foil")
	{
		return RBX::ALUMINUM_MATERIAL;
	}
	else if(materialString == "Grass")
	{
		return RBX::GRASS_MATERIAL;
	}
	else if(materialString =="Ice")
	{
		return RBX::ICE_MATERIAL;
	}
	else if(materialString == "Brick")
	{
		return RBX::BRICK_MATERIAL;
	}
	else if(materialString == "Sand")
	{
		return RBX::SAND_MATERIAL;
	}
	else if(materialString == "Fabric")
	{
		return RBX::FABRIC_MATERIAL;
	}
	else if(materialString == "Granite")
	{
		return RBX::GRANITE_MATERIAL;
	}
	else if(materialString == "Marble")
	{
		return RBX::MARBLE_MATERIAL;
	}
	else if(materialString == "Pebble")
	{
		return RBX::PEBBLE_MATERIAL;
	}
    else if(materialString == "SmoothPlastic")
    {
        return RBX::SMOOTH_PLASTIC_MATERIAL;
    }
	else if(materialString == "Neon")
	{
		return NEON_MATERIAL;
	}
    else if(materialString == "WoodPlanks")
    {
        return RBX::WOODPLANKS_MATERIAL;
    }
    else if(materialString == "Cobblestone")
    {
        return RBX::COBBLESTONE_MATERIAL;
    }
    else if(materialString == "Metal")
    {
        return RBX::METAL_MATERIAL;
    }
	else 
		return RBX::PLASTIC_MATERIAL;
}

bool applyAnchor(Instance* instance, bool anchor)
{
	bool applied = false;
	if (PartInstance* pInstance = dynamic_cast<PartInstance*>(instance))
	{
		pInstance->setAnchored(anchor);
		applied = true;
	}
	else if (ModelInstance* pModelInstance = dynamic_cast<ModelInstance*>(instance))
	{
		std::vector<PartInstance*> childParts = pModelInstance->getDescendantPartInstances();
		for (std::vector<PartInstance*>::const_iterator iter = childParts.begin(); iter != childParts.end(); ++iter)
		{
			applied |= applyAnchor(*iter, anchor);
		}
	}
	return applied;
}
void AnchorVerb::doIt( IDataState* dataState )
{
	if (m_selection->size() > 0)
	{
		bool setAnchored = !isChecked(), applied = false;
		for (std::vector<Instance*>::const_iterator iter = m_selection->begin(); iter != m_selection->end(); ++iter)
			applied |= applyAnchor(*iter, setAnchored);

		{
			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			ChangeHistoryService::requestWaypoint(getName().c_str(), workspace.get());
		}

		dataState->setDirty(true);
	}
}

bool AnchorVerb::isChecked() const
{
	if (!dataModel)
		return false;
	
	bool hasUnanchoredInstance = false;

	for (std::vector<Instance*>::const_iterator iter = m_selection->begin(); iter != m_selection->end(); ++iter)
	{
		if (!RBX::AnchorTool::allChildrenAnchored(*iter))
		{
			hasUnanchoredInstance = true;
			break;
		}
	}

	return !hasUnanchoredInstance;
}

AnchorVerb::AnchorVerb( DataModel* dataModel ) 
	: EditSelectionVerb("AnchorVerb", dataModel)
{
	// Put this here to avoid requiring edit lock on isChecked
	m_selection = ServiceProvider::create< FilteredSelection<RBX::Instance> >(dataModel);
}
} // namespace
