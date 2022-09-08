/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Workspace.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/Stats.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/GeometryService.h"
#include "V8DataModel/UserController.h"
#include "V8DataModel/DebugSettings.h"
#include "V8DataModel/Camera.h"
#include "V8DataModel/Hopper.h"
#include "V8DataModel/Accoutrement.h"
#include "V8DataModel/Flag.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/ToolsSurface.h"
#include "V8DataModel/ImageLabel.h"
#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/PluginManager.h"
#include "V8DataModel/PluginMouse.h"
#include "v8datamodel/PhysicsSettings.h"
#include "v8datamodel/MegaCluster.h"
#include "v8datamodel/UserInputService.h"
#include "v8datamodel/SurfaceGui.h"
#include "Humanoid/Humanoid.h"
#include "Tool/NullTool.h"
#include "Tool/ToolsArrow.h"
#include "Tool/DropTool.h"
#include "Network/Players.h"
#include "V8World/World.h"
#include "V8World/SleepStage.h"
#include "V8World/ContactManager.h"
#include "V8World/SpatialFilter.h"
#include "V8World/StepJointsStage.h"
#include "V8World/AssemblyStage.h"
#include "V8Kernel/Kernel.h"  
#include "V8Xml/Serializer.h"
#include "V8Xml/XmlSerializer.h"
#include "Util/UserInputBase.h"
#include "Util/Profiling.h"
#include "Util/NavKeys.h"
#include "util/RobloxGoogleAnalytics.h"
#include "Util/G3DCore.h"
#include "Script/ScriptContext.h"
#include "Script/Script.h"
#include "Script/CoreScript.h"

#include "SelectState.h"
#include "GfxBase/IAdornableCollector.h"

#include "FastLog.h"




// For Tyler's experimental fallen parts deletion
#include "Network/NetworkOwner.h"
#include "Network/Players.h"
#include "Network/api.h"

LOGGROUP(MouseCommand)

DYNAMIC_FASTFLAG(FixTouchEndedReporting)

// Cyclic Executive Experiment Logging (THESE NEED TO BE REMOVED IN THE FUTURE, YAY)
DYNAMIC_FASTFLAGVARIABLE(PreventReturnOfElevatedPhysicsFPS, false)
DYNAMIC_FASTFLAGVARIABLE(ReportElevatedPhysicsFPSToGA, true)

DYNAMIC_FASTINTVARIABLE(ElevatedPhysicsFPSReportThresholdTenths, 610)
//END CyclicExecutive Experiment Logging

FASTFLAGVARIABLE(PhysicsAnalyzerEnabled, false)
FASTFLAGVARIABLE(PGSAlwaysActiveMasterSwitch, false)
FASTFLAG(UsePGSSolver)

FASTFLAGVARIABLE(LuaControlsDisableMouse2Lock, false)

DYNAMIC_FASTFLAG(CreatePlayerGuiLocal)

FASTFLAG(FlyCamOnRenderStep)
FASTFLAG(PGSSolverFileDump)

DYNAMIC_FASTFLAG(UseStarterPlayerCharacter)
FASTFLAGVARIABLE(GamepadCursorChanges, false)
DYNAMIC_FASTFLAG(FixFallenPartsNotDeleted)
DYNAMIC_FASTFLAGVARIABLE(TrackPhysicalPropertiesGA, false);

namespace RBX {

namespace Reflection {
template<>
EnumDesc<PhysicalPropertiesMode>::EnumDesc()
	:EnumDescriptor("PhysicalPropertiesMode")
{
	addPair(PhysicalPropertiesMode_Default, "Default");
	addPair(PhysicalPropertiesMode_Legacy, "Legacy");
	addPair(PhysicalPropertiesMode_NewPartProperties, "New");
}
}

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<Workspace, double> prop_DistributedGameTime("DistributedGameTime", category_Data, &Workspace::getDistributedGameTime, &Workspace::setDistributedGameTime, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
Reflection::PropDescriptor<Workspace, bool> Workspace::prop_StreamingEnabled("StreamingEnabled", category_Behavior, &Workspace::getNetworkStreamingEnabled, &Workspace::setNetworkStreamingEnabled, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);
Reflection::PropDescriptor<Workspace, bool> Workspace::prop_ExperimentalSolverEnabled("PGSPhysicsSolverEnabled", category_Behavior, &Workspace::getExperimentalSolverEnabled, &Workspace::setExperimentalSolverEnabled, Reflection::PropertyDescriptor::PUBLIC_SERIALIZED);
Reflection::PropDescriptor<Workspace, bool> Workspace::prop_ExpSolverEnabled_Replicate("ExpSolverEnabled_Replicate", category_Behavior, &Workspace::getExpSolverEnabled_Replicate, &Workspace::setExpSolverEnabled_Replicate, Reflection::PropertyDescriptor::STREAMING);
Reflection::PropDescriptor<Workspace, bool> Workspace::prop_FilteringEnabled("FilteringEnabled", category_Behavior, &Workspace::getNetworkFilteringEnabled, &Workspace::setNetworkFilteringEnabled, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE); 
static Reflection::PropDescriptor<Workspace, float> prop_fallenPartDestroyHeight("FallenPartsDestroyHeight", category_Behavior, &Workspace::getFallenPartDestroyHeight, &Workspace::setFallenPartDestroyHeight, Reflection::PropertyDescriptor::STANDARD_NO_SCRIPTING);
Reflection::PropDescriptor<Workspace, bool> Workspace::prop_allowThirdPartySales("AllowThirdPartySales", category_Behavior, &Workspace::getAllowThirdPartySales, &Workspace::setAllowThirdPartySales, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);

// Use ReleastTest build to ask whether or not we have a certain PhysicalProperties mode enabled
#ifndef RBX_TEST_BUILD
	Reflection::EnumPropDescriptor<Workspace, PhysicalPropertiesMode> Workspace::prop_physicalPropertiesMode("PhysicalPropertiesMode", category_Behavior, &Workspace::getPhysicalPropertiesMode, &Workspace::setPhysicalPropertiesMode, Reflection::PropertyDescriptor::STANDARD_NO_SCRIPTING);
#else
	Reflection::EnumPropDescriptor<Workspace, PhysicalPropertiesMode> Workspace::prop_physicalPropertiesMode("PhysicalPropertiesMode", category_Behavior, &Workspace::getPhysicalPropertiesMode, &Workspace::setPhysicalPropertiesMode, Reflection::PropertyDescriptor::STANDARD);
#endif

static Reflection::BoundFuncDesc<Workspace, shared_ptr<const Instances>(int)> func_getIssueGroup(&Workspace::getPhysicsAnalyzerIssue, "GetPhysicsAnalyzerIssue", "index", Security::Plugin);
static Reflection::BoundFuncDesc<Workspace, void(bool)> func_setPhysicsAnalyzerBreakOnIssue(&Workspace::setPhysicsAnalyzerBreakOnIssue, "SetPhysicsAnalyzerBreakOnIssue", "enable", Security::Plugin);
static Reflection::BoundFuncDesc<Workspace, bool()> func_getPhysicsAnalyzerBreakOnIssue(&Workspace::getPhysicsAnalyzerBreakOnIssue, "GetPhysicsAnalyzerBreakOnIssue", Security::Plugin);
static Reflection::EventDesc<Workspace, void(int) > desc_PhysicsAnalyzerIssueFound(&Workspace::luaPhysicsAnalyzerIssuesFound, "PhysicsAnalyzerIssuesFound", "count", Security::Plugin);
REFLECTION_END();

bool Workspace::showWorldCoordinateFrame = false;
bool Workspace::showHashGrid = false;
bool Workspace::showEPhysicsOwners = false;
bool Workspace::showEPhysicsRegions = false;
bool Workspace::showStreamedRegions = false;
bool Workspace::showPartMovementPath = false;
bool Workspace::showActiveAnimationAsset = false;
float Workspace::gridSizeModifier = 4.0f;

const char* const sWorkspace = "Workspace";

// static			
bool Workspace::serverIsPresent(const Instance* context)				// shortcut for RBX::Network::Players::serverIsPresent
{
	return RBX::Network::Players::serverIsPresent(context, false);		// if not in the datamodel, will go false
}

// static
bool Workspace::clientIsPresent(const Instance* context)				// shortcut for RBX::Network::Players::clientIsPresent
{
	return RBX::Network::Players::clientIsPresent(context, false);		// if not in the datamodel, will go false
}

const Workspace* Workspace::findConstWorkspace(const Instance* context)
{
	return ServiceProvider::find<Workspace>(context);
}


Workspace* Workspace::findWorkspace(Instance* context)
{
	return ServiceProvider::find<Workspace>(context);
}

Instance* Workspace::findTopInstance(Instance* context)		// finds the top level Instance in the workspace (right below the workspace)
{
	RBXASSERT(getWorldIfInWorkspace(context));
	Workspace* workspace = Workspace::getWorkspaceIfInWorkspace(context);
	RBXASSERT(workspace);
	RBXASSERT(workspace != context);

	while (context->getParent() != workspace)
	{
		context = context->getParent();
	}

	return context;
}

Workspace* Workspace::getWorkspaceIfInWorkspace(Instance* context)
{
	if (Workspace* workspace = findWorkspace(context)) {
		if ((context == workspace) || context->isDescendantOf(workspace))
		{
			return workspace;
		}
	}
	return NULL;
}

// returns getWorld if context is within the RootInstance
World* Workspace::getWorldIfInWorkspace(Instance* context)
{
	if (Workspace* workspace = getWorkspaceIfInWorkspace(context))
	{
		return workspace->getWorld();
	}
	else
	{
		return NULL;
	}
}

ContactManager* Workspace::getContactManagerIfInWorkspace(Instance* context)
{
	if (World* world = getWorldIfInWorkspace(context)) {
		return world->getContactManager();
	}
	return NULL;
}

bool Workspace::contextInWorkspace(const Instance* context)
{
	const Workspace* workspace = findConstWorkspace(context);
	RBXASSERT(!workspace || (workspace != context));
	return (workspace && context->isDescendantOf(workspace));
}

REFLECTION_BEGIN();
static Reflection::BoundFuncDesc<Workspace, void(shared_ptr<const Instances>)> workspace_makeJoints(&Workspace::makeJoints, "MakeJoints", "objects", Security::Plugin);
static Reflection::BoundFuncDesc<Workspace, void(shared_ptr<const Instances>)> workspace_breakJoints(&Workspace::breakJoints, "BreakJoints", "objects", Security::Plugin);

// single ignore instance overloads for region/ray queries
static Reflection::BoundFuncDesc<Workspace, shared_ptr<const Instances>(Region3, shared_ptr<Instance>, int)> workspace_FindParts(&Workspace::findPartsInRegion3, "FindPartsInRegion3", "region", "ignoreDescendentsInstance", shared_ptr<Instance>(), "maxParts", 20, Security::None);
static Reflection::BoundFuncDesc<Workspace, shared_ptr<const Instances>(Region3, shared_ptr<Instance>, int)> workspace_dep_FindParts(&Workspace::findPartsInRegion3, "findPartsInRegion3", "region", "ignoreDescendentsInstance", shared_ptr<Instance>(), "maxParts", 20, Security::None, Reflection::Descriptor::Attributes::deprecated(workspace_FindParts));
static Reflection::BoundFuncDesc<Workspace, bool(Region3, shared_ptr<Instance>)> workspace_RegionEmpty(&Workspace::isRegion3Empty, "IsRegion3Empty", "region", "ignoreDescendentsInstance", shared_ptr<Instance>(), Security::None);
static Reflection::BoundFuncDesc<Workspace, shared_ptr<const Reflection::Tuple>(RbxRay, shared_ptr<Instance>, bool, bool)> workspace_FindPartOnRay(&Workspace::getRayHit<Instance>, "FindPartOnRay", "ray", "ignoreDescendentsInstance", shared_ptr<Instance>(), "terrainCellsAreCubes", false, "ignoreWater", false, Security::None);
static Reflection::BoundFuncDesc<Workspace, shared_ptr<const Reflection::Tuple>(RbxRay, shared_ptr<Instance>, bool, bool)> dep_FindPartOnRay(&Workspace::getRayHit<Instance>, "findPartOnRay", "ray", "ignoreDescendentsInstance", shared_ptr<Instance>(), "terrainCellsAreCubes", false, "ignoreWater", false, Security::None, Reflection::Descriptor::Attributes::deprecated(workspace_FindPartOnRay));

// table of ignore instances overloads for region/ray queries
static Reflection::BoundFuncDesc<Workspace, shared_ptr<const Instances>(Region3, shared_ptr<const Instances>, int)> workspace_FindParts2(&Workspace::findPartsInRegion3, "FindPartsInRegion3WithIgnoreList", "region", "ignoreDescendentsTable", "maxParts", 20, Security::None);
static Reflection::BoundFuncDesc<Workspace, bool(Region3, shared_ptr<const Instances>)> workspace_RegionEmpty2(&Workspace::isRegion3Empty, "IsRegion3EmptyWithIgnoreList", "region", "ignoreDescendentsTable", Security::None);
static Reflection::BoundFuncDesc<Workspace, shared_ptr<const Reflection::Tuple>(RbxRay, shared_ptr<const Instances>, bool, bool)> workspace_FindPartOnRay2(&Workspace::getRayHit<const Instances>, "FindPartOnRayWithIgnoreList", "ray", "ignoreDescendentsTable", "terrainCellsAreCubes", false, "ignoreWater", false, Security::None);

static Reflection::RefPropDescriptor<Workspace, Instance> workspace_Terrain("Terrain", category_Behavior, &Workspace::getTerrain, NULL, Reflection::PropertyDescriptor::UI, Security::None);

// Removed - set through debugSettings
static Reflection::BoundFuncDesc<Workspace, void(bool)> workspace_SetThrottleEnabled(&Workspace::doNothing, "SetPhysicsThrottleEnabled", "value", Security::LocalUser);
	
// Repressed zoomToExtents to see if anyone is calling it...
static Reflection::BoundFuncDesc<Workspace, void()> workspace_zoomToExtents(&Workspace::zoomToExtents, "ZoomToExtents", Security::Plugin);

//static Reflection::BoundFuncDesc<Workspace, void()> workspace_setNullMouseCommand(&Workspace::setNullMouseCommand, "SetNullMouseCommand", Security::LocalUser);
static Reflection::RefPropDescriptor<Workspace, Camera> currentCameraProxyProp("CurrentCamera", category_Data, &Workspace::getCurrentCameraDangerous, &Workspace::setCurrentCamera, Reflection::PropertyDescriptor::STANDARD_NO_REPLICATE);

// Metrics
static Reflection::BoundFuncDesc<Workspace, double()> getRealPhysicsFPS(&Workspace::getRealPhysicsFPS, "GetRealPhysicsFPS", Security::None);
static Reflection::BoundFuncDesc<Workspace, int()> getPhysicsThrottling(&Workspace::getPhysicsThrottling, "GetPhysicsThrottling", Security::None);
static Reflection::BoundFuncDesc<Workspace, int()> getNumAwakeParts(&Workspace::getNumAwakeParts, "GetNumAwakeParts", Security::None);

static Reflection::BoundFuncDesc<Workspace, bool()> experimentalSolverIsEnabled(&Workspace::experimentalSolverIsEnabled, "ExperimentalSolverIsEnabled", Security::TestLocalUser);
static Reflection::BoundFuncDesc<Workspace, bool()> pgsSolverIsEnabled(&Workspace::experimentalSolverIsEnabled, "PGSIsEnabled", Security::None);

static Reflection::BoundFuncDesc<Workspace, void(shared_ptr<const Instances>, AdvArrowToolBase::JointCreationMode)> workspace_joinToOutsiders(&Workspace::joinToOutsiders, "JoinToOutsiders", "objects", "jointType", Security::None);
static Reflection::BoundFuncDesc<Workspace, void(shared_ptr<const Instances>)> workspace_unjoinFromOutsiders(&Workspace::unjoinFromOutsiders, "UnjoinFromOutsiders", "objects", Security::None);
REFLECTION_END();

Workspace::Workspace(IDataState* dataState)	 :
	VerbContainer(NULL),
	flySteps(0),
	arrowCameraControls(false),
	dataState(dataState),
	inRightMousePan(false),
    inMiddleMouseTrack(false),
	imageServerViewHack(0),
	profileDataModelStep(new Profiling::CodeProfiler("DataModel Step")),
	profileWorkspaceStep(new Profiling::CodeProfiler("Workspace Step")),
	profileWorkspaceAssemble(new Profiling::CodeProfiler("Workspace Assemble")),
	utilityCamera(Creatable<Instance>::create<Camera>()),
	distributedGameTime(0.0),
	statsSyncHttpGetTime(0),
	statsXMLLoadTime(0),
	statsJoinAllTime(0),
	adornableCollector(new IAdornableCollector()),
	firstPersonCam(false),
	leftMouseDown(false),
	show3DGrid(false),
	showAxisWidget(false),
	lastComputedWorldExtents(Vector3::zero(), Vector3::zero()),
	lastComputedWorldExtentsTime(-DBL_MAX),
	networkStreamingEnabled(false),
	experimentalSolverEnabled(false),
	expSolverEnabled_Replicate(false),
    renderingDistance(10000.f),
	networkFilteringEnabled(false),
	allowThirdPartySales(false)
{
	RBXASSERT(dataState!=NULL);
	setName("Workspace");
	FASTLOG1(FLog::GuiTargetLifetime, "Workspace created: %p", this);

	// Master switch used for turning on the PGS solver regardless of the Workspace toggle or code FFlag - for testing purposes
	getWorld()->setUsingPGSSolver(FFlag::PGSAlwaysActiveMasterSwitch);
}

Workspace::~Workspace()
{
	FASTLOG1(FLog::GuiTargetLifetime, "Workspace destroyed: %p", this);
}

// we template this to avoid heavy code duplication, but still keep the functions fast (IgnoreType is currently either Instance or Instances)
template<class IgnoreType>
shared_ptr<const Reflection::Tuple> Workspace::getRayHit(RbxRay ray, shared_ptr<IgnoreType> ignoreInstance, bool terrainCellsAreCubes, bool ignoreWaterCells)
{
	shared_ptr<Reflection::Tuple> result(new Reflection::Tuple());
	if (GeometryService* geometryService = ServiceProvider::create<GeometryService>(this))
	{
		shared_ptr<PartInstance> found;
        Vector3 surfaceNormal;
        PartMaterial surfaceMaterial = AIR_MATERIAL;
		Vector3 point = geometryService->getHitLocationPartFilterDescendents(ignoreInstance.get(), ray, found, surfaceNormal, surfaceMaterial, terrainCellsAreCubes, ignoreWaterCells);
		result->values.push_back(shared_static_cast<Instance>(found));
		result->values.push_back(point);
        result->values.push_back(surfaceNormal);
		result->values.push_back(surfaceMaterial);
	}
	return result;
}

bool Workspace::isRegion3Empty(Region3 region, shared_ptr<Instance> ignoreDescendent)
{
	shared_ptr<Instances> newInstances(new Instances());
	if (ignoreDescendent) newInstances->push_back(ignoreDescendent);
	return isRegion3Empty(region, newInstances);
}
bool Workspace::isRegion3Empty(Region3 region, shared_ptr<const Instances> ignoreDescendents)
{
	// check to see if we're blocked by a part
	shared_ptr<const Instances> blockingParts = findPartsInRegion3(region, ignoreDescendents, 1);
	if (!blockingParts.get()->empty()) return false;

	// check to see if we're blocked by terrain
	return (!getWorld()->getContactManager()->terrainCellsInRegion3(region));
}

void Workspace::joinToOutsiders(shared_ptr<const Instances> items, AdvArrowToolBase::JointCreationMode joinType)
{
	if (joinType == AdvArrowToolBase::NO_JOIN)
		return;

	PartArray partArray;
	DragUtilities::instancesToParts(*items, partArray);
	DragUtilities::joinToOutsiders(partArray);

	if (joinType == AdvArrowToolBase::WELD_ALL)
	{
		ManualJointHelper jointHelper;

		jointHelper.setSelectedPrimitives(*items);
		jointHelper.setWorkspace(this);
		jointHelper.findPermissibleJointSurfacePairs();
		jointHelper.createJoints();
	}
}

void Workspace::unjoinFromOutsiders(shared_ptr<const Instances> items)
{
	PartArray partArray;
	DragUtilities::instancesToParts(*items, partArray);
	DragUtilities::unJoinFromOutsiders(partArray);
}

shared_ptr<const Instances> Workspace::findPartsInRegion3(Region3 region, shared_ptr<Instance> ignoreDescendent, int maxCount)
{
	shared_ptr<Instances> newInstances(new Instances());
	if (ignoreDescendent) newInstances->push_back(ignoreDescendent);
	return findPartsInRegion3(region, newInstances, maxCount);
}

shared_ptr<const Instances> Workspace::findPartsInRegion3(Region3 region, shared_ptr<const Instances> ignoreDescendents, int maxCount)
{
	static const float volumeLimit = 100000.0f;
	Extents extents(region.minPos(),region.maxPos());
	if(extents.volume() > volumeLimit){
		StandardOut::singleton()->printf(MESSAGE_WARNING, "Volume between min and max exceeds limit of %f", volumeLimit);
	}

	maxCount = std::min(maxCount, 100);

	if(GeometryService* geometryService = ServiceProvider::create<GeometryService>(this)){
		G3D::Array<PartInstance*> found;
		geometryService->getPartsTouchingExtentsWithIgnore(extents, ignoreDescendents.get(), maxCount, found);

		shared_ptr<Instances> result(new Instances());
		for (int i = 0; i < found.size(); ++i) {
			result->push_back(shared_from(found[i]));
		}
		return result;
	}
	return shared_ptr<const Instances>();
}

Extents Workspace::computeExtentsWorldFast()
{
	// recompute extents if last computed is over 2 seconds old, otherwise just return last computed value
	if (lastComputedWorldExtentsTime == 0 ||
		distributedGameTime - lastComputedWorldExtentsTime > 2.0f)
	{
		lastComputedWorldExtents = RootInstance::computeExtentsWorld();
		lastComputedWorldExtentsTime = distributedGameTime;
	}

	return lastComputedWorldExtents;
}

void Workspace::setDistributedGameTime(double value)
{
	if (value != distributedGameTime)
	{
		distributedGameTime = value;
		raiseChanged(prop_DistributedGameTime);
	}
}

void Workspace::setDistributedGameTimeNoTransmit(double value)
{
	distributedGameTime = value;
}

bool Workspace::forceDrawConnectors() const
{
	RBXASSERT(currentCommand.get() != NULL);
	if (currentCommand.get()) {
		return currentCommand->drawConnectors();
	}
	else {
		return false;
	}
}

namespace {
	void sendNetworkFilteringStats()
	{
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "NetworkFilteringEnabled");
	}
} // namespace

void Workspace::setNetworkFilteringEnabled(bool value)
{
	if (value && Workspace::serverIsPresent(this))
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(&sendNetworkFilteringStats, flag);
	}
	bool changed = networkFilteringEnabled != value;
	networkFilteringEnabled = value;

	if (networkFilteringEnabled && DFFlag::CreatePlayerGuiLocal && Network::Players::frontendProcessing(this))
	{
		if (Network::Player* player = Network::Players::findLocalPlayer(this))
		{
			// this allows guis to be created if you load a character manually later
			player->createPlayerGui();
		}
	}
	if (changed && Network::Players::isCloudEdit(this))
	{
		raisePropertyChanged(prop_FilteringEnabled);
	}
}

void Workspace::setAllowThirdPartySales(bool value)
{
	bool changed = allowThirdPartySales != value;
	allowThirdPartySales = value;

	if (changed && Network::Players::isCloudEdit(this))
	{
		raisePropertyChanged(prop_allowThirdPartySales);
	}
}

// Slop - create camera here if we don't have one already
//
void Workspace::onHeartbeat(const Heartbeat& heartbeat)
{
	// IMovingManager
	onMovingHeartbeat();

	replenishCamera();


	if( terrain && terrain->getParent() != this ) 
	{
		terrain->setAndLockParent(this);
	}

	// update deferred terrain contacts/joints
	world->getContactManager()->applyDeferredTerrainChanges();

    // Step the current tool
    MouseCommand* mouseCommand = getCurrentMouseCommand();
	if(mouseCommand && idleMouseEvent && idleMouseEvent->getUserInputType() == InputObject::TYPE_MOUSEIDLE) {
        mouseCommand->onMouseIdle(idleMouseEvent);
		if (Network::Players::frontendProcessing(this)) {
			if (Network::Player* localPlayer = Network::Players::findLocalPlayer(this)) {
				if (shared_ptr<Mouse> mouse = localPlayer->getMouse()) {
                    mouse->idleSignal();
				}
			}
		}

		if (Plugin *activePlugin = PluginManager::singleton()->getActivePlugin(DataModel::get(this)))
			if (PluginMouse* mouse = activePlugin->getMouse())
				mouse->idleSignal();
	}
    
	if (!FFlag::FlyCamOnRenderStep)
	{
		// If no local character, do camera fly with arrow keys
		if (!Network::Players::findLocalCharacter(this))
		{
			if (ControllerService* service = ServiceProvider::find<ControllerService>(this)) 
			{
				if (const UserInputBase* hardwareDevice = service->getHardwareDevice()) 
				{
					NavKeys navKeys;
					if(DataModel* dm = DataModel::get(this))
						hardwareDevice->getNavKeys(navKeys,dm->getSharedSuppressNavKeys());

					if (navKeys.navKeyDown())
					{
						if (getCurrentMouseCommand())
							getCamera()->doFly(navKeys, flySteps++);
					}
					else
					{
						if(flySteps > 0)
							getCamera()->pushCameraHistoryStack();
						flySteps = 0;
					}
				}
			}
		}
	}
}

bool Workspace::askAddChild(const Instance* instance) const
{
	return dynamic_cast<const IAdornable*>(instance)!=NULL;		// TODO: Hmmm. Is this a good choice? What about RBX::Message?
}

void Workspace::onDescendantRemoving(const shared_ptr<Instance>& instance)
{
	if (IAdornable* iR = dynamic_cast<IAdornable*>(instance.get())) {
		adornableCollector->onRenderableDescendantRemoving(iR);
	}

	Super::onDescendantRemoving(instance);
}

// Note on legacyOffset
//
//	Workspace
//		TopPVInstance
//			Child
//				Child
//  
// If a child is added to TopPVInstance after the TopPVInstance is added to workspace,
// an assertion will trigger.  
// Current code always attaches to topPVInstance last for old files
//

void Workspace::onDescendantAdded(Instance* instance)
{
	Super::onDescendantAdded(instance);

	if (IAdornable* iR = dynamic_cast<IAdornable*>(instance)) {
		adornableCollector->onRenderableDescendantAdded(iR);
	}
}

bool Workspace::startDecalDrag(Decal *decal, RBX::InsertMode insertMode)
{
	ServiceClient< Selection > sel(this);
	if (decal) {
		sel->setSelection(decal);
	}
	shared_ptr<MouseCommand> result = Creatable<MouseCommand>::create<DecalTool>(this, decal, insertMode);
	this->setMouseCommand(result);
	return result != NULL;
}

bool Workspace::startPartDropDrag(const Instances& instances, bool suppressPartsAlign)
{
	ServiceClient< Selection > sel(this);
	shared_ptr<Selection> selWhenDone;
	std::vector<Instance*> dragInstances;
	for(std::vector<shared_ptr<Instance> >::const_iterator iter = instances.begin(); iter != instances.end(); ++iter){
		dragInstances.push_back(iter->get());
	}
	shared_ptr<MouseCommand> result = DropTool::createDropTool(Vector3(0,0,0),dragInstances,this,selWhenDone,suppressPartsAlign);
	this->setMouseCommand(result, true);
	return result != NULL;
}

////////////////////////////////////////////////////////////////////////////////////
//
// Camera

Camera* Workspace::getCamera() 
{
	return const_cast<Camera*>(getConstCamera());
}

const Camera* Workspace::getConstCamera() const
{
	return (currentCamera) 
				? currentCamera.get()
				: utilityCamera.get();
}

// ToDo: Hack - using render3dAdorn as a delayed trigger here
// do this on render3dAdorn - if no camera as a descendant, then create one
void Workspace::replenishCamera()
{
	if (currentCamera && this->isAncestorOf(currentCamera.get()))
	{
		return;	// ok - no replenishment necessary
	}
	else
	{
		shared_ptr<Camera> childCamera = shared_from<Camera>(findFirstChildOfType<Camera>());
		if (!childCamera)
		{
			childCamera = shared_polymorphic_downcast<Camera>(utilityCamera->clone(EngineCreator));
			childCamera->setParent(this);
		}

		setCurrentCamera(childCamera.get());
	}
}

//reflection only - breaks the const barrier
Camera* Workspace::getCurrentCameraDangerous() const
{
	return currentCamera.get();
}

void destroyIfNotCurrent(shared_ptr<Instance> destroy, const Camera* current)
{
	RBXASSERT(current);
	if (Instance::fastDynamicCast<Camera>(destroy.get())) {
		if (destroy.get() != current) {
			destroy->setParent(NULL);
		}
	}
}

// Note - for now only doing children - 
// TODO:  go through everything?
void Workspace::setCurrentCamera(Camera *value) 
{
	if(!serverIsPresent(this) || Network::Players::isCloudEdit(this)){
		if (value != currentCamera.get())
		{
			currentCamera = shared_from<Camera>(value);

			this->raisePropertyChanged(currentCameraProxyProp);
			if (value)
			{
				visitChildren(boost::bind(&destroyIfNotCurrent, _1, value));
			}
			currentCameraChangedSignal(currentCamera);
		}
	}
}

void Workspace::setTerrain(Instance* terrain)
{
	FASTLOG1(FLog::MegaClusterInit, "Setting terrain on workspace, %p", terrain);
	this->terrain = shared_from(terrain);
	this->raisePropertyChanged(workspace_Terrain);
	if( terrain )
	{
		this->terrain->lockParent();
		this->terrain->setLockedParent(this);
	}
}

Instance* Workspace::getTerrain() const
{
	return terrain.get();
}

void Workspace::createTerrain()
{
	if(!terrain)
	{
		FASTLOG(FLog::MegaClusterInit, "Terrain doesn't exist - creating terrain instance explicitly");
		shared_ptr<Instance> t = Creatable<Instance>::create<MegaClusterInstance>();
		PartInstance* part = static_cast<PartInstance*>(t.get());

		Vector3 rbxSize = part->getPartSizeXml();
		CoordinateFrame clusterFrame(Vector3(-2,rbxSize.y/2, -2));
		part->setCoordinateFrame(clusterFrame);

		t->setAndLockParent(this);
		RBXASSERT(terrain);
	}
}

void Workspace::clearTerrain()
{
	if(terrain)
	{
		FASTLOG(FLog::MegaClusterInit, "Clearing terrain (from client replicator?)");

		terrain->unlockParent();
		terrain->setParent(NULL);
		terrain.reset();
	}
}

void Workspace::onWrapMouse(const Vector2& wrapMouseDelta)
{
	getCamera()->onMousePan(wrapMouseDelta);
}

// This is a scripting call - tries to make things pretty as well
void Workspace::zoomToExtents()
{
	getCamera()->zoomExtents();
}

bool Workspace::setImageServerView(bool bIsPlace)
{
	if(bIsPlace)
		return false; // place rendering. don't fiddle with the camera.

	// use the ThumnailCamera if it is the child of the first child of the workspace.
	Instance *model = NULL;
	
	if(numChildren() > 0)
	{
		model = getChild(0);
	}

	if(model)
	{
		Camera *camera = Instance::fastDynamicCast<Camera>(model->findFirstChildByName("ThumbnailCamera"));

		if(camera)
		{
			setCurrentCamera(camera);
			return false; // found camera. done.
		}
	}

	// default behavior:

	replenishCamera();

	HopperBin* superHack = NULL;

	for (size_t i = 0; i < this->numChildren(); ++i) 
	{
		HopperBin* hopperBin = this->queryTypedChild<HopperBin>(i);
		IHasLocation* iLocation = this->queryTypedChild<IHasLocation>(i);
		IAdornable* iRenderable = this->queryTypedChild<IAdornable>(i);
		if (iLocation && iRenderable)
		{
			CoordinateFrame modelCoord = iLocation->getLocation();

			getCamera()->setImageServerViewNoLerp(modelCoord);
		}

		// The hack part #1 - find a hopperBin in the workspace
		if (hopperBin)
		{
			superHack = hopperBin;
		}
	}
	
	// The hack part #2 - if hopperBin found - put it in the StarterPackService so it will be drawn
	if (superHack)
	{
		StarterPackService* starterPackService = ServiceProvider::create<StarterPackService>(this);
	
		superHack->setParent(starterPackService);
	}

	// The hack part #3 - toggle this bit - when drawing, hopperBins will draw full screen in this situation
	imageServerViewHack = (imageServerViewHack > 0) ? 0 : 1;			// super hack - for now, on setting image server view, renders all hopper bins full screen
	return true;
}


/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

void Workspace::selectAllTopLevelRenderable()
{
	ServiceClient< Selection > sel(this);

	sel->clearSelection();

	for (size_t i = 0; i < numChildren(); i++) {			// 1. Top Level Only
		Instance* child = this->getChild(i);			
		if (dynamic_cast<IAdornable*>(child)) {			// 2. Only IAdornable
			if (!PartInstance::getLocked(child))			// 3. Not Locked
			{
				sel->addToSelection(child);
			}
		}
	}
}

void Workspace::joinAllHack()			// Joins all primitives - called after a file read
{
	world->joinAll();
}


template<bool make>
static void wrapper(shared_ptr<Instance> instance)
{
	if (PartInstance* p = Instance::fastDynamicCast<PartInstance>(instance.get()))
		if (make)
			p->join();
		else
			p->destroyJoints();
	else
		instance->visitChildren(&wrapper<make>);
}


void Workspace::makeJoints(shared_ptr<const Instances> instances)
{
	std::for_each(instances->begin(), instances->end(), &wrapper<true>);
}

void Workspace::breakJoints(shared_ptr<const Instances> instances)
{
	std::for_each(instances->begin(), instances->end(), &wrapper<false>);
}


/////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////


void Workspace::start()
{
	assemble();

	RBXASSERT(!getCurrentMouseCommand()->captured());

	// makes sure building tools are not greyed out
	shared_ptr<RBX::CoreGuiService> coreGuiService = shared_from(ServiceProvider::find<RBX::CoreGuiService>(this));
	if(coreGuiService)
		if(RBX::ImageLabel* frame = Instance::fastDynamicCast<RBX::ImageLabel>(coreGuiService->findFirstChildByName2("Frame",true).get()))
			frame->setZIndex(1);
}


void Workspace::stop()
{
	// makes sure building tools are grayed out (not simulating, therefore building tools don't work currently)
	shared_ptr<RBX::CoreGuiService> coreGuiService = shared_from(ServiceProvider::find<RBX::CoreGuiService>(this));
	if(coreGuiService)
		if(RBX::ImageLabel* frame = Instance::fastDynamicCast<RBX::ImageLabel>(coreGuiService->findFirstChildByName2("Frame",true).get()))
			frame->setZIndex(10);

	RBXASSERT(!getCurrentMouseCommand()->captured());

	updateDistributedGameTime();
}


void Workspace::reset() 
{
	stop();
	world->reset();
}

void Workspace::detachParent(Instance* test)
{
	shared_ptr<Instance> oldParent = shared_from<Instance>(test->getParent());

	#ifdef _DEBUG
		std::vector<weak_ptr<PartInstance> > parts;
		PartInstance::findParts(test, parts);
		RBXASSERT(parts.empty());
	#endif

	test->setParent(NULL);
	clearEmptiedModels(oldParent);
}

// Will delete test if:
// 1.  It is not the workspace
// 2.  It's a model
// 3.  It's not a character (contains no humanoid)
// 4.  Contains no parts directly as children
// 5.  Contains no models directly as children

void Workspace::clearEmptiedModels(shared_ptr<Instance>& test)
{
	if (test.get() == this)	// 1.
		return;

	if (ModelInstance* model = Instance::fastDynamicCast<ModelInstance>(test.get())) // 2.
	{
		if (!Humanoid::modelIsCharacter(model)) // 3.
		{
			if (	!model->findFirstChildOfType<PartInstance>()		// 4., 5.
				&&	!model->findFirstChildOfType<ModelInstance>()	)
			{
				if (DFFlag::UseStarterPlayerCharacter)
				{
					Network::Player *pPlayer = Network::Players::getPlayerFromCharacter(model);
					if (pPlayer)
						pPlayer->onCharacterDied();
				}

				detachParent(model);
			}
		}
	}
	if (Accoutrement* accoutrement = Instance::fastDynamicCast<Accoutrement>(test.get()))
	{
		if (!accoutrement->findFirstChildOfType<PartInstance>())		// 4.
		{
			detachParent(accoutrement);
		}
	}
	if (BackpackItem* backpackItem = Instance::fastDynamicCast<BackpackItem>(test.get()))
	{
		if (!backpackItem->findFirstChildOfType<PartInstance>())		// 4.
		{
			detachParent(backpackItem);
		}
	}
}

void Workspace::handleFallenParts()
{
	RBXASSERT(fallenParts.size() == 0);
	RBXASSERT(fallenPrimitives.size() == 0);

	world->computeFallen(fallenPrimitives);

	PartInstance::primitivesToParts(fallenPrimitives, fallenParts);

	// if we're in a client running distributed physics, then we might not have the authority to remove parts
	//      so we just pass them all over to server when they fall off the ends of the world
	if (Network::Players::getGameMode(this) == Network::DPHYS_CLIENT)
	{
		for (size_t i = 0; i < fallenParts.size(); ++i)
		{
			shared_ptr<PartInstance> part = fallenParts[i];
			if (!DFFlag::FixFallenPartsNotDeleted)
			{
				// This property does not replicate and means nothing to the Client.

				//old comment - had good intention: // make sure server has enough time to process the parts we're sending
				part->resetNetworkOwnerTime(3.0);
			}

			part->setNetworkOwnerAndNotify(RBX::Network::NetworkOwner::Server());
		}
	}
	else // otherwise, we summarily execute them, since we have the authority to do so
	{
		for (size_t i = 0; i < fallenParts.size(); ++i)
		{
			shared_ptr<PartInstance> part = fallenParts[i];
			shared_ptr<Instance> oldParent = shared_from<Instance>(part->getParent());
			part->setParent(NULL);
			clearEmptiedModels(oldParent);
		}
	}

	fallenParts.resize(0);
	fallenPrimitives.fastClear();
}

void Workspace::assemble()
{
	world->assemble();
	RBXASSERT(world->isAssembled());
}


void Workspace::updateDistributedGameTime()
{
	RunService* runService = ServiceProvider::create<RunService>(this);
	if (serverIsPresent(this)) {
		setDistributedGameTime(runService->gameTime());
	}
	else {
		setDistributedGameTimeNoTransmit(runService->gameTime());	// update but don't send
	}
}

int Workspace::updatePhysicsStepsRequiredForCyclicExecutive(float timeInterval)
{
	return world->updateStepsRequiredForCyclicExecutive( timeInterval );
}

float Workspace::physicsStep(bool longStep, float timeInterval, int numThreads)
{
	RBXASSERT(world->isAssembled());				// testing - assemble in 

	if (longStep)
	{
		updateDistributedGameTime();

		// update deferred terrain contacts/joints
		world->getContactManager()->applyDeferredTerrainChanges();
	}

    if( FFlag::PGSSolverFileDump )
    {
        RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(DataModel::get( this ));
        int id = 0;
        if( localPlayer ) id = localPlayer->getUserID();
        getWorld()->setUserId( id );
    }

    world->setPhysicsAnalyzerEnabled( RBX::PhysicsSettings::singleton().getPhysicsAnalyzerState() );

	// Step the world
	timeInterval = world->step(longStep, distributedGameTime, timeInterval, numThreads);

    if( FFlag::PhysicsAnalyzerEnabled && world->getKernel()->pgsSolver.getInconsistentBodyPairs().size() > 0 )
    {
        luaPhysicsAnalyzerIssuesFound( world->getKernel()->pgsSolver.getInconsistentBodies().size() );
    }

	// Copy all Primitives into a collection of shared_ptr<>.
	// If we don't do this, then a PartInstance may get collected 
	// during the event firing. This would lead to stale Primitive
	// pointers.
	std::vector<TouchPair> touchReportingParts;
	const int size = world->getTouchInfoFromLastStep().size();
	{
		const G3D::Array<World::TouchInfo>& source = world->getTouchInfoFromLastStep();
		touchReportingParts.resize(size);
		for (int i = 0; i < size; ++i)
		{
			const World::TouchInfo& src = source[i];
			TouchPair& dst = touchReportingParts[i];

			if (DFFlag::FixTouchEndedReporting) 
			{
				dst.p1 = src.pi1;
				dst.p2 = src.pi2;
			} 
			else 
			{
				dst.p1 = shared_from(PartInstance::fromPrimitive(src.p1));
				dst.p2 = shared_from(PartInstance::fromPrimitive(src.p2));
			}
			dst.type = src.type == World::TouchInfo::Touch ? TouchPair::Touch : TouchPair::Untouch;
		}
	}

	world->clearTouchInfoFromLastStep();

	RBXASSERT(world->isAssembled());

	handleFallenParts();

	for (int i = 0; i < size; ++i) 
	{
		const TouchPair& info = touchReportingParts[i];

		if (info.type == TouchPair::Touch)
		{
			info.p1->reportTouch(info.p2);
			if(info.p1->onDemandRead())
				info.p1->onDemandWrite()->localSimulationTouchedSignal(info.p2);
		}
		else
		{
			info.p1->reportUntouch(info.p2);
			if(info.p1->onDemandRead())
				info.p1->onDemandWrite()->deprecatedStoppedTouchingSignal(info.p2);
		}

		stepTouch(info);
	}

	return timeInterval;
}

// Undo any stickiness here
void Workspace::setDefaultMouseCommand()
{
	stickyCommand.reset();
	setMouseCommand(shared_ptr<MouseCommand>());
}

shared_ptr<NewNullTool> newNullTool(Workspace* workspace)
{
	return Creatable<MouseCommand>::create<NewNullTool>(workspace);
}

// Undo any stickiness here
void Workspace::setNullMouseCommand()
{
	stickyCommand.reset();
	currentCommand = shared_ptr<MouseCommand>(newNullTool(this));
    updatePlayerMouseCommand();
}

void Workspace::setMouseCommand(shared_ptr<MouseCommand> newMouseCommand, bool allowPluginOverride)
{
	FASTLOG2(FLog::MouseCommand, "Set mouse command: %p, old command: %p", newMouseCommand.get(), currentCommand.get());

	RBX::DataModel *dataModel = (DataModel*)ServiceProvider::findServiceProvider(this);
	RBX::Plugin *activePlugin = PluginManager::singleton()->getActivePlugin(dataModel);

	if ((!newMouseCommand || allowPluginOverride) && activePlugin && activePlugin->isTool())
	{
		FASTLOG1(FLog::MouseCommand, "Rejecting because of plugin override, plugin: %p", activePlugin);
		return;
	}

	if (newMouseCommand.get() == NULL)								// The MouseCommand has been used. Get a new Tool
	{
		if (stickyCommand.get()) {
			newMouseCommand = stickyCommand.get()->isSticky();	// pull a copy of the stickyCommand - returns a copy if isSticky
			FASTLOG2(FLog::MouseCommand, "Have sticky command %p, generating new mouse command: %p", stickyCommand.get(), newMouseCommand.get() );
		}
	}

	if (newMouseCommand.get()  == NULL) 
	{
		if (Network::Players::findLocalPlayer(this) == NULL || Network::Players::isCloudEdit(this))
		{
			newMouseCommand = Creatable<MouseCommand>::create<AdvArrowTool>(this);
			FASTLOG1(FLog::MouseCommand, "Generating new arrow tool: %p", newMouseCommand.get() );
		}
		else
		{
			newMouseCommand = newNullTool(this);
			FASTLOG1(FLog::MouseCommand, "Setting as null tool: %p", newMouseCommand.get() );
		}
	}

	RBXASSERT(newMouseCommand.get());

	if (newMouseCommand != currentCommand) 
	{
		RBXASSERT((currentCommand.get() == NULL) || !currentCommand.get()->captured());
		FASTLOG2(FLog::MouseCommand, "Current command update, new: %p, old: %p", newMouseCommand.get(), currentCommand.get());
		currentCommand = newMouseCommand;
		shared_ptr<MouseCommand> sticky = newMouseCommand->isSticky();
		if (sticky.get() != NULL)
		{
			FASTLOG2(FLog::MouseCommand, "Sticky command replaced too: %p, old: %p", sticky.get(), stickyCommand.get());
			stickyCommand = sticky;
		}

		if (activePlugin && activePlugin->isTool())
		{
			PluginManager::singleton()->activate(NULL, dataModel);
		}

		updatePlayerMouseCommand();
	}

	FASTLOG(FLog::MouseCommand, "Set mouse command: Done");
}

void Workspace::updatePlayerMouseCommand()
{
	if (Network::Player* player = Network::Players::findLocalPlayer(this))
	{
		if (shared_ptr<Mouse> playerMouse = player->getMouse())
		{
			playerMouse->setWorkspace(this);
		}
	}
}

void Workspace::render2d(Adorn* adorn)
{
	getCurrentMouseCommand()->render2d(adorn);

	adornableCollector->render2dItems(adorn);
}

ContentId Workspace::getCursor()
{
	if (UserInputService* userInputService = ServiceProvider::find<UserInputService>(this))
	{
		ContentId gameCursor = userInputService->getCurrentMouseIcon();
		if( gameCursor.isNull() )
		{
			return getCurrentMouseCommand()->getCursorId();
		}
		return gameCursor;
	}
	else
	{
		return getCurrentMouseCommand()->getCursorId();
	}
}

double Workspace::getRealPhysicsFPS(void)
{
	RunService* runService = ServiceProvider::create<RunService>(this);
	double realPhysicsFPS = runService->smoothFps() * getWorld()->getEnvironmentSpeed();
	double reportedPhysicsFPS = realPhysicsFPS;
	if (DFFlag::ReportElevatedPhysicsFPSToGA && realPhysicsFPS > ((double) DFInt::ElevatedPhysicsFPSReportThresholdTenths / 10.0))
	{
		if (DFFlag::PreventReturnOfElevatedPhysicsFPS)
		{
			reportedPhysicsFPS = 60.0f;
		}

		DataModel* dm = DataModel::get(this);
		if (dm)
		{
			int placeID = dm->getPlaceID();

			if (realPhysicsFPS > 100.0)
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "ElevatedPhysicsFPSDetected_100", 
					boost::lexical_cast<std::string>(placeID).c_str(), 0, false));
			}
			else if (realPhysicsFPS > 90.0)
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "ElevatedPhysicsFPSDetected_90", 
					boost::lexical_cast<std::string>(placeID).c_str(), 0, false));
			}
			else if (realPhysicsFPS > 80.0)
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "ElevatedPhysicsFPSDetected_80", 
					boost::lexical_cast<std::string>(placeID).c_str(), 0, false));
			}
			else if (realPhysicsFPS > 70.0)
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "ElevatedPhysicsFPSDetected_70", 
					boost::lexical_cast<std::string>(placeID).c_str(), 0, false));
			}
			else if (realPhysicsFPS > 65.0)
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "ElevatedPhysicsFPSDetected_65", 
					boost::lexical_cast<std::string>(placeID).c_str(), 0, false));
			}
			else
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "ElevatedPhysicsFPSDetected_SetMinimum", 
					boost::lexical_cast<std::string>(placeID).c_str(), 0, false));
			}
		}
	}
	return reportedPhysicsFPS;

}

int Workspace::getPhysicsThrottling(void)
{
	return (int)(100 * getWorld()->getEnvironmentSpeed());
}

int Workspace::getNumAwakeParts(void)
{
    Kernel* kernel = getWorld()->getKernel();
	int awakePartCount = 
          kernel->numFreeFallBodies()
		+ kernel->numContactBodies()
		+ kernel->numJointBodies()
		+ kernel->numRealTimeConnectors()
		+ kernel->numLeafBodies();

	return awakePartCount;
}


void Workspace::render3dAdorn(Adorn* adorn)
	{
	// Hack - viewport is updated here
	viewPort = adorn->getViewport();

	// Standard Adornment for all items that have adornment
	adornableCollector->render3dAdornItems(adorn);

	if (Workspace::showWorldCoordinateFrame) {
		adorn->setObjectToWorldMatrix( CoordinateFrame() );
		adorn->axes(
			G3D::Color3::red(), 
			G3D::Color3::green(),
			G3D::Color3::blue(), 
			50.0);
	}

	if (Workspace::showHashGrid) {
		Vector3 gridLow = 4.0 * Vector3(7,1,3);
		Vector3 gridHigh = gridLow + Vector3(4,4,4);
		adorn->setObjectToWorldMatrix(CoordinateFrame());
		adorn->box( AABox(gridLow, gridHigh) );
	}

		Network::Player* localPlayer = Network::Players::findLocalPlayer(this);

	if(show3DGrid && (!localPlayer || Network::Players::isCloudEdit(this)))
				RBX::DrawAdorn::zeroPlaneGrid(adorn, *getCamera(), gridSizeModifier, 0.05, Color3(0.3f,0.3f,0.3f), Color3(0.4f,0.4f,0.4f));

		if(showAxisWidget)
			RBX::DrawAdorn::axisWidget(adorn, *getCamera());
	}

void Workspace::append3dSortedAdorn(std::vector<AdornableDepth>& sortedAdorn)
{
	adornableCollector->append3dSortedAdornItems(sortedAdorn, getConstCamera());
}

bool Workspace::hasModalGuiObjects()
{
	bool isModal = false;
	if (Network::Player* player = Network::Players::findLocalPlayer(this))
		if (PlayerGui* playerGui = player->findFirstChildOfType<PlayerGui>()) 
			isModal = playerGui->findModalGuiObject();

	if(!isModal)
	{
		shared_ptr<RBX::CoreGuiService> coreGuiService = shared_from(ServiceProvider::find<RBX::CoreGuiService>(this));
		if(coreGuiService)
			isModal = coreGuiService->findModalGuiObject();
	}

	return isModal;
}

void Workspace::requestFirstPersonCamera(bool firstPersonOn, bool cameraTransitioning, int controlMode)	
{
	ControllerService* service = ServiceProvider::find<ControllerService>(this);
	if (!service) 
		return;

	UserInputBase* userInput = service->getHardwareDevice();
	if (!userInput) 
		return;

	UserInputService* userInputService = ServiceProvider::find<UserInputService>(this);
	if (!userInputService)
		return;

	if (getConstCamera()->getCameraType() == Camera::LOCKED_CAMERA)  // let go of mouse pan if we're in LOCKED_CAMERA mode
	{
		inRightMousePan = false;
		userInputService->setMouseWrapMode(UserInputService::WRAP_AUTO);
	}
	else if( ( (firstPersonOn && !cameraTransitioning) || RBX::GameBasicSettings::singleton().mouseLockedInMouseLockMode() ) && !hasModalGuiObjects() )
	{
		userInput->centerCursor();
		userInputService->setMouseWrapMode(UserInputService::WRAP_CENTER);
	}
	else if(RBX::GameBasicSettings::singleton().inHybridMode() && !inRightMousePan && !inMiddleMouseTrack)
		userInputService->setMouseWrapMode(UserInputService::WRAP_HYBRID);
	else 
	{
		if (!RBX::GameBasicSettings::singleton().inMousepanMode() && !inRightMousePan && !inMiddleMouseTrack)
			userInputService->setMouseWrapMode(UserInputService::WRAP_AUTO);
	}
}

void Workspace::setMiddleMouseTrack()
{	
	if (this->getConstCamera()->getCameraType() != Camera::LOCKED_CAMERA)
	{
		inMiddleMouseTrack = true;

		if(UserInputService* userInputService = ServiceProvider::find<UserInputService>(this))
			userInputService->setMouseWrapMode(UserInputService::WRAP_CENTER);
	}
}

void Workspace::cancelMiddleMouseTrack()
{
	inMiddleMouseTrack = false;
	if(!firstPersonCam) // don't change this if we are current in first person (Still need to wrap the same)
		if(UserInputService* userInputService = ServiceProvider::find<UserInputService>(this))
			userInputService->setMouseWrapMode(UserInputService::WRAP_AUTO);
}

void Workspace::setRightMousePan()
{	
	if (FFlag::LuaControlsDisableMouse2Lock)
	{
		Camera::CameraType camType = this->getConstCamera()->getCameraType();
		if (camType != Camera::LOCKED_CAMERA && camType != Camera::CUSTOM_CAMERA)
		{
			inRightMousePan = true;

			if(UserInputService* userInputService = ServiceProvider::find<UserInputService>(this)) 
			{
				userInputService->setMouseWrapMode(UserInputService::WRAP_CENTER);
			}
		}
	}
	else if (this->getConstCamera()->getCameraType() != Camera::LOCKED_CAMERA)
	{
		inRightMousePan = true;

		if(UserInputService* userInputService = ServiceProvider::find<UserInputService>(this)) 
		{
			userInputService->setMouseWrapMode(UserInputService::WRAP_CENTER);
		}
	}
}

void Workspace::cancelRightMousePan()
{
	inRightMousePan = false;
	if(UserInputService* userInputService = ServiceProvider::find<UserInputService>(this))
	{
		if(!firstPersonCam) // don't change this if we are current in first person (Still need to wrap the same)
				userInputService->setMouseWrapMode(UserInputService::WRAP_AUTO);
	}
}

GuiResponse Workspace::handleSurfaceGui(const shared_ptr<InputObject>& event)
{
	Camera* cam = this->getCamera();

	if (SurfaceGui::numInstances() && event->isMouseEvent() && cam)
	{
		RBX::Network::Player* player = Network::Players::findLocalPlayer(this);

		// 'Tool punch-though concept': surfaceGUI is clickable if the character is not wielding a tool
		// --or-- if the character has a tool active and he is within a certain distance of the SG.
		bool ignoreSGDist = !(player && player->getCharacter() && player->getCharacter()->findFirstChildOfType<Tool>());

		Instances ignore;

		// Ignore terrain to avoid expensive raycasts - terrain never has SG objects
		if (Instance* terrain = getTerrain())
			ignore.push_back(shared_from(terrain));

		// The ray sometimes hits the character's hat when in first person mode, preventing us from clicking anything on SGs
		// first person is set in lua, so check distance
		float distanceToCharacter = (cam->getCameraFocus().translation - cam->getCameraCoordinateFrame().translation).magnitude();
		if (player && player->getCharacter() && distanceToCharacter <= 0.5f)
		{
			ignore.push_back(shared_from(player->getCharacter()));
		}

		// Cast a ray to get the first intersecting part
		Vector2 p = event->get2DPosition();
		Ray ray = cam->worldRay(p.x, p.y);
		ray.direction() *= 1000.f;

		GeometryService* geometryService = ServiceProvider::create<GeometryService>(this);
		RBXASSERT(geometryService);

		shared_ptr<PartInstance> part;
		Vector3 surfaceNormal;
        PartMaterial surfaceMaterial;
		Vector3 point = geometryService->getHitLocationPartFilterDescendents<const Instances>(&ignore, ray, part, surfaceNormal, surfaceMaterial, false, false);

		// Find the SG object on the hit surface
		SurfaceGui* sg = 0;

		if (part)
		{
			int faceId = 0;
			Surface surf = part->getSurface(ray, faceId);

			if (surf.getPartInstance())
				sg = SurfaceGui::findSurfaceGui(part.get(), surf.getNormalId());
		}

		// Send various unfocus events to the last active SG if necessary
		if (SurfaceGui* last = lastSurfaceGUI.lock().get())
			if (sg != last && DataModel::get(last)) // if it's inside the dataModel
				last->unProcess();

		lastSurfaceGUI = shared_from(sg);

		// Send the current event to the found SG
		if (sg)
		{
			GuiResponse resp = sg->process3d(event, point, ignoreSGDist);

			if (FFlag::GamepadCursorChanges)
			{
				return resp;
			}
			else
			{
			     if (resp.wasSunk())
			     	return resp;
		    }
	    }
	}

	return GuiResponse::notSunk();
}

GuiResponse Workspace::process(const shared_ptr<InputObject>& event)
{
	FASTLOG1(FLog::MouseCommand, "Workspace::Process, eventType: %u", event->getUserInputType());

	DataModel *dataModel = (DataModel*)ServiceProvider::findServiceProvider(this);
	Plugin *activePlugin = PluginManager::singleton()->getActivePlugin(dataModel);
	if (activePlugin)
	{
		FASTLOG1(FLog::UserInputProfile, "Passing event to plugin mouse, plugin: %p", activePlugin);
		activePlugin->getMouse()->update(event);
	}

    GuiResponse resp = handleSurfaceGui(event);
    if( resp.wasSunk() )
        return resp;

	RBXASSERT(currentCommand.get() != NULL);
	shared_ptr<MouseCommand> processingCommand = currentCommand;

	if (event->isMouseEvent()) // copy last mouse position
	{	
		idleMouseEvent = Creatable<Instance>::create<InputObject>(*event);
		idleMouseEvent->setInputType(InputObject::TYPE_MOUSEIDLE);
	}

	switch (event->getUserInputType())
	{
	default:
		{
			FASTLOG(FLog::MouseCommand, "Return GuiResponse::notSunk");
			return GuiResponse::notSunk();
		}
	case InputObject::TYPE_FOCUS:
		{
			cancelRightMousePan();
			break;
		}
	case InputObject::TYPE_KEYBOARD:
		{
			RBXASSERT(event->isKeyDownEvent() || event->isKeyUpEvent());

			if (event->isKeyDownEvent())
            {
                FASTLOG(FLog::MouseCommand, "Handling Key up");
                processingCommand->onPeekKeyDown(event); // for Hopper keyboard object
                if (processingCommand->captured())
                {
                    setMouseCommand(processingCommand->onKeyDown(event));
                    break;
                }
                else
                    return GuiResponse::notSunk();
			}
            else if(event->isKeyUpEvent())
            {
                FASTLOG(FLog::MouseCommand, "Handling Key up");
                processingCommand->onPeekKeyUp(event); // for Hopper keyboard object
                if (processingCommand->captured())
                {
                    setMouseCommand(processingCommand->onKeyUp(event));
                    break;
                }
                else
                    return GuiResponse::notSunk();
			}
			break;
		}
	case InputObject::TYPE_MOUSEBUTTON2:
		{
			RBXASSERT(event->isRightMouseDownEvent() || event->isRightMouseUpEvent());

			if (event->isRightMouseDownEvent())
			{
                setRightMousePan();
                if (processingCommand->captured())  // ignore - re-entrant from window
                    break;
                setMouseCommand(processingCommand->onRightMouseDown(event));
            }
			else if(event->isRightMouseUpEvent())
            {
                FASTLOG(FLog::UserInputProfile, "Canceling Right Mouse pan");
                cancelRightMousePan();

                #ifdef STUDIO_CAMERA_CONTROL_SHORTCUTS
                    if( Camera* camera = getCamera() )
                    {
                        FASTLOG(FLog::UserInputProfile, "Pushing camera history");
                        camera->pushCameraHistoryStack();
                    }
                #endif

                FASTLOG1(FLog::UserInputProfile, "Passing right up to processing command: %p", processingCommand.get());
                setMouseCommand(processingCommand->onRightMouseUp(event));
			}
			break;
		}
	case InputObject::TYPE_MOUSEBUTTON1:
		{
			RBXASSERT(event->isLeftMouseDownEvent() || event->isLeftMouseUpEvent());

			if(event->isLeftMouseDownEvent())
            {
                leftMouseDown = true;
                if (processingCommand->captured())  // ignore - re-entrant from window
                    break;

                FASTLOG1(FLog::MouseCommand, "Processing Mouse down on %p:", processingCommand.get());

                shared_ptr<MouseCommand> mousedownCommand = processingCommand->onMouseDown(event);
                setMouseCommand(mousedownCommand);
            }
            else if(event->isLeftMouseUpEvent())
            {
                FASTLOG1(FLog::UserInputProfile, "Passing left up to processing command: %p", processingCommand.get());
                leftMouseDown = false;
                FASTLOG1(FLog::MouseCommand, "Processing Mouse up on %p:", processingCommand.get());
                shared_ptr<MouseCommand> mouseupCommand = processingCommand->onMouseUp(event);
                setMouseCommand(mouseupCommand);
			}
			break;
		}
	case InputObject::TYPE_MOUSEBUTTON3:
		{
			if(event->isMiddleMouseDownEvent())
			{
				setMiddleMouseTrack();
				break;
			}
			else if (event->isMiddleMouseUpEvent())
			{
				cancelMiddleMouseTrack();
				#ifdef STUDIO_CAMERA_CONTROL_SHORTCUTS
					if( Camera* camera = getCamera() )
					{
						FASTLOG(FLog::UserInputProfile, "Pushing camera history");
						camera->pushCameraHistoryStack();
					}
				#endif
				break;
			}
        }
	case InputObject::TYPE_MOUSEWHEEL:
		{
			RBXASSERT(event->isMouseWheelBackward() || event->isMouseWheelForward());

			if (event->isMouseWheelForward())
			processingCommand->onMouseWheelForward(event);
			else if(event->isMouseWheelBackward())
			processingCommand->onMouseWheelBackward(event);
			break;
		}
	case InputObject::TYPE_MOUSEMOVEMENT:
		{
			if (processingCommand->captured())
			{
				FASTLOG1(FLog::UserInputProfile, "Passing mouse move to processing command: %p", processingCommand.get());
				processingCommand->onMouseMove(event);
			}
			else							
			{
				FASTLOG1(FLog::UserInputProfile, "Passing mouse hover to processing command: %p", processingCommand.get());
				processingCommand->onMouseHover(event);
			}
			break;
		}
	case InputObject::TYPE_MOUSEDELTA:
		{
			if (processingCommand->captured())
				processingCommand->onMouseDelta(event);
			break;
		}
	}

	FASTLOG(FLog::UserInputProfile, "Done with workspace process");
	FASTLOG1(FLog::MouseCommand, "Workspace::Process finish, captured: %u", processingCommand->captured());

	RBXASSERT(processingCommand.get() != NULL);
	if (processingCommand->captured()) {
		return GuiResponse::sunkWithTarget(this);
	}
	else {
		return GuiResponse::sunk();
	}
}

void Workspace::setNetworkStreamingEnabled(bool value)
{
	bool changed = value != networkStreamingEnabled;
	networkStreamingEnabled = value;
	if (changed && Network::Players::isCloudEdit(this))
	{
		raisePropertyChanged(prop_StreamingEnabled);
	}
}

void Workspace::setExperimentalSolverEnabled(bool value) 
{ 
	if (experimentalSolverEnabled != value)
	{
		experimentalSolverEnabled = value; 
		raiseChanged(prop_ExperimentalSolverEnabled);
	}

	if (expSolverEnabled_Replicate != experimentalSolverEnabled)
	{
		expSolverEnabled_Replicate = experimentalSolverEnabled;
		raiseChanged(prop_ExpSolverEnabled_Replicate);
	}
}

void Workspace::setExpSolverEnabled_Replicate(bool value) 
{ 
	if (expSolverEnabled_Replicate != value)
	{
		expSolverEnabled_Replicate = value; 
		raiseChanged(prop_ExpSolverEnabled_Replicate);
	}
	if (experimentalSolverEnabled != expSolverEnabled_Replicate)
	{
		experimentalSolverEnabled = expSolverEnabled_Replicate; 
		raiseChanged(prop_ExperimentalSolverEnabled);
	}

	getWorld()->setUsingPGSSolver(expSolverEnabled_Replicate && FFlag::UsePGSSolver);
	// Master switch used for turning on the PGS solver regardless of the Workspace toggle or code FFlag - for testing purposes
	getWorld()->setUsingPGSSolver(getWorld()->getUsingPGSSolver() || FFlag::PGSAlwaysActiveMasterSwitch);

	if (expSolverEnabled_Replicate && FFlag::UsePGSSolver)
	{
		DataModel* dm = DataModel::get(this);
		if (dm)
		{
			int placeID = dm->getPlaceID();
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "PGSSolverActivated", 
				boost::lexical_cast<std::string>(placeID).c_str(), 0, false));
		}
	}
}

// Calling this function CallOnce prevents unecessary logic from being run
// multiple times after the call once is expired.
void callGAForPhysicalProperties(const DataModel* dm, PhysicalPropertiesMode mode)
{
	int placeID = dm->getPlaceID();
	std::string gaMessage = "PhysicalPropertiesMode_";
	if (mode == PhysicalPropertiesMode_Legacy)
	{
		gaMessage += "Legacy";
	}
	else if (mode == PhysicalPropertiesMode_Default)
	{
		gaMessage += "Default";
	}
	else
	{
		gaMessage += "New";
	}

	RobloxGoogleAnalytics::trackEvent( GA_CATEGORY_GAME, gaMessage.c_str(), boost::lexical_cast<std::string>(placeID).c_str());
}


bool Workspace::getUsingNewPhysicalProperties() const
{
	if (DFFlag::TrackPhysicalPropertiesGA)
	{
		if (const DataModel* dm = DataModel::get(this))
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;				
			boost::call_once(flag, boost::bind(&callGAForPhysicalProperties, dm, getWorld()->getPhysicalPropertiesMode()));
		}
	}

	return getWorld()->getUsingNewPhysicalProperties();
}

void Workspace::setPhysicalPropertiesMode(PhysicalPropertiesMode mode)
{	
	RunService* rs = ServiceProvider::find<RunService>(this);
	if (getWorld()->getPhysicalPropertiesMode() != mode)
	{
		// Prevents the flag from being flipping while in RunTime
		if (rs && rs->getRunState() != RS_RUNNING && rs->getRunState() != RS_PAUSED)
		{
			getWorld()->setPhysicalPropertiesMode(mode);
			raiseChanged(prop_physicalPropertiesMode);
		}
		else
		{
			RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "Cannot change PhysicalPropertiesMode during Runtime");
		}
	}
}

void Workspace::setPhysicalPropertiesModeNoEvents(PhysicalPropertiesMode mode)
{
	if (getWorld()->getPhysicalPropertiesMode() != mode)
	{
		getWorld()->setPhysicalPropertiesMode(mode);
	}
}


PhysicalPropertiesMode Workspace::getPhysicalPropertiesMode() const
{
	return getWorld()->getPhysicalPropertiesMode();
}

// A bit dangerous, but we know the runService is here during the onServiceProvider call

class WorkspaceStatsItem : public Stats::Item
{
public:
	WorkspaceStatsItem()
	{
		setName("Workspace");
	}
	static shared_ptr<WorkspaceStatsItem> create(const Workspace* workspace, const World* world, const RunService* runService)
	{
		shared_ptr<WorkspaceStatsItem> result = Creatable<Instance>::create<WorkspaceStatsItem>();

		result->createChildItem<double>("FPS", boost::bind(&RunService::smoothFps, runService));
		result->createChildItem<float>("Environment Speed %", boost::bind(&World::getEnvironmentSpeedPercent, world));

		Stats::Item* pDataModelStep = result->createBoundChildItem(*workspace->profileDataModelStep);
			Stats::Item* pWorkspaceStep = pDataModelStep->createBoundChildItem(*workspace->profileWorkspaceStep);
				Stats::Item* pWorldStep = pWorkspaceStep->createBoundChildItem(world->getProfileWorldStep());
				std::vector<RBX::Profiling::CodeProfiler*> worldProfilers;
				world->loadProfilers(worldProfilers);
				for (size_t i = 0; i < worldProfilers.size(); ++i)
				{
					pWorldStep->createBoundChildItem(*worldProfilers[i]);
				}
			pDataModelStep->createBoundChildItem(*workspace->profileWorkspaceAssemble);

		Stats::Item* w = result->createChildItem("World");
		w->createChildItem<int>("Primitives", boost::bind(&World::getNumPrimitives, world));
		w->createChildItem<int>("Joints", boost::bind(&World::getNumJoints, world));
		w->createChildItem<int>("Contacts", boost::bind(&World::getNumContacts, world));

		Stats::Item* contacts = result->createChildItem("Contacts");
		contacts->createChildItem<int>("CtctStageCtcts", boost::bind(&World::getMetric, world, IWorldStage::NUM_CONTACTSTAGE_CONTACTS));
		contacts->createChildItem<int>("SteppingCtcts", boost::bind(&World::getMetric, world, IWorldStage::NUM_STEPPING_CONTACTS));
		contacts->createChildItem<int>("TouchingCtcts", boost::bind(&World::getMetric, world, IWorldStage::NUM_TOUCHING_CONTACTS));
		contacts->createChildItem<int>("MaxTreeDepth", boost::bind(&World::getMetric, world, IWorldStage::MAX_TREE_DEPTH));
		contacts->createChildItem<int>("# link(p)", boost::bind(&World::getNumLinkCalls, world));
		contacts->createChildItem<int>("Hash Nodes Out", boost::bind(&World::getNumHashNodes, world));
		contacts->createChildItem<int>("Max Bucket Size", boost::bind(&World::getMaxBucketSize, world));

		Stats::Item* kernelItem = result->createChildItem("Kernel");
		const Kernel* kernel = world->getKernel();
		// First two are here for fun and to throw off the competition
		kernelItem->createChildItem<int>("SolverIterations", boost::bind(&Kernel::fakeDeceptiveSolverIterations, kernel));
		kernelItem->createChildItem<int>("MatrixSize", boost::bind(&Kernel::fakeDeceptiveMatrixSize, kernel));
//		kernelItem->createChildItem<int>("projJobs", boost::bind(&Kernel::numProjectileBodies, kernel));
		kernelItem->createChildItem<int>("Bodies", boost::bind(&Kernel::numBodies, kernel));
		kernelItem->createChildItem<int>("Constraints", boost::bind(&Kernel::numConnectors, kernel));
		kernelItem->createChildItem<int>("Points", boost::bind(&Kernel::numPoints, kernel));

		Stats::Item* file = result->createChildItem("File Operations");
		file->createChildItem<double>("Total Load Time", boost::bind(&Workspace::getStatsFileTimeTotal, workspace));
		file->createChildItem<double>("SyncHttpGet Time", boost::bind(&Workspace::getStatsSyncHttpGetTime, workspace));
		file->createChildItem<double>("XML Load Time", boost::bind(&Workspace::getStatsXMLLoadTime, workspace));
		file->createChildItem<double>("Join All Time", boost::bind(&Workspace::getStatsJoinAllTime, workspace));

		return result;
	}
};


void Workspace::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	if (oldProvider)
	{
		heartbeatConnection.disconnect();
		setDefaultMouseCommandConnection.disconnect();

		if (statsItem) {
			statsItem->setParent(NULL);
			statsItem.reset();
		}
	}

	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider)
	{
		RunService* runService = ServiceProvider::create<RunService>(newProvider);
		RBXASSERT(runService);
		heartbeatConnection = runService->heartbeatSignal.connect(boost::bind(&Workspace::onHeartbeat, this, _1));

		Stats::StatsService* stats = ServiceProvider::create<Stats::StatsService>(newProvider);
		if (stats) {
			statsItem = WorkspaceStatsItem::create(this, world.get(), runService);
			statsItem->setParent(stats);
		}

		setDefaultMouseCommandConnection = PluginManager::singleton()->allPluginsDeactivatedSignal.connect(boost::bind(&Workspace::setDefaultMouseCommand, this));
	}
}



/*	When scripts run in the workspace

	BaseScript:		Backend Processing		(runs backend)
	LocalScript:	In local character		(runs local)
*/

bool Workspace::scriptShouldRun(BaseScript* script)
{
	RBXASSERT(isAncestorOf(script));

	bool answer = false;

	if (script->fastDynamicCast<LocalScript>())
	{
		{
			//Either we're in the old mode (LocalScripts run on Client), or this script is eligible to be run ClientSide
			ModelInstance* character = Network::Players::findLocalCharacter(this);
			answer = (character && script->isDescendantOf(character));
			if(answer){
				script->setLocalPlayer(shared_from(Network::Players::getPlayerFromCharacter(character)));
			}

		}
	}
	else
	{
		answer = Network::Players::backendProcessing(this);
	}

	return answer;
}

std::size_t hash_value(const TouchPair& p)
{
	std::size_t result = boost::hash<const RBX::PartInstance*>()(p.p1.get());
	boost::hash_combine(result, p.p2.get());
	boost::hash_combine(result, p.type);
	return result;
}	

bool Workspace::experimentalSolverIsEnabled()
{ 
	return getWorld()->getKernel()->getUsingPGSSolver(); 
}

float Workspace::getFallenPartDestroyHeight() const
{
	return getWorld()->getFallenPartDestroyHeight();
}

void Workspace::setFallenPartDestroyHeight(float value)
{
	value = G3D::clamp(value, -50000.0f, 50000.0f);
	if (getWorld()->getFallenPartDestroyHeight() != value) 
	{
		getWorld()->setFallenPartDestroyHeight(value);
		raisePropertyChanged(prop_fallenPartDestroyHeight);
	}
}

//////////////////////
//
// Physics analyzer
//
shared_ptr<const Instances> Workspace::getPhysicsAnalyzerIssue( int group )
{
    shared_ptr< Instances > instances(new Instances);

    if( !getPhysicsAnalyzerBreakOnIssue() ) 
    {
        return instances;
    }

    const auto& islands = getWorld()->getKernel()->pgsSolver.getInconsistentBodies();
    if( islands.size() > ( size_t )group )
    {
        const auto& bodies = islands[ group ];
        for( size_t i = 0; i < bodies.size(); i++ )
        {
            boost::uint64_t id = bodies[ i ];
            auto* prim = getWorld()->getPrimitiveFromBodyUID( id );
            shared_ptr< Instance > partInstance = shared_from( static_cast<Instance*>( PartInstance::fromPrimitive(prim) ) );
            instances->push_back( partInstance );
        }
    }

    return instances;
}

void Workspace::setPhysicsAnalyzerBreakOnIssue( bool enable )
{
    if( getExperimentalSolverEnabled() )
    {
        getWorld()->getKernel()->pgsSolver.setPhysicsAnalyzerBreakOnIssue( enable );
    }
}

bool Workspace::getPhysicsAnalyzerBreakOnIssue( )
{
    if( getExperimentalSolverEnabled() )
    {
        return getWorld()->getKernel()->pgsSolver.getPhysicsAnalyzerBreakOnIssue( );
    }
    return false;
}

} // namespace
