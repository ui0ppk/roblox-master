#pragma once

#include "gui/GuiEvent.h"
#include "V8World/IMoving.h"
#include "V8DataModel/RootInstance.h"
#include "V8DataModel/Selection.h"
#include "Script/IScriptFilter.h"
#include "V8Tree/Service.h"
#include "V8DataModel/InputObject.h"	
#include <vector>
#include "util/runstateowner.h"
#include "util/Region3.h"
#include "util/SystemAddress.h"
#include "util/PhysicalProperties.h"
#include <boost/shared_ptr.hpp>
#include "Tool/ToolsArrow.h"

namespace RBX {

	namespace Profiling
	{
		class CodeProfiler;
	}

	class BaseScript;
	class Camera;
	class Decal;
	class Region2;
	class IAdornableCollector;
	class MouseCommand;
	class PartInstance;
	class SurfaceGui;


extern const char* const sWorkspace;


struct TouchPair
{
	typedef enum { Touch, Untouch } Type;
	TouchPair() {}
	TouchPair(const shared_ptr<PartInstance>& p1, const shared_ptr<PartInstance>& p2, Type type, SystemAddress originator = SystemAddress())
		:p1(p1)
		,p2(p2)
		,type(type)
		,originator(originator)
	{
	}
	shared_ptr<PartInstance> p1;
	shared_ptr<PartInstance> p2;
	Type type;
	SystemAddress originator;

	bool operator==(const TouchPair& other) const {
		return this->p1 == other.p1 && this->p2 == other.p2 && this->type == other.type;
	}
};
std::size_t hash_value(const TouchPair& prop);

class Workspace :  
	public DescribedNonCreatable<Workspace, RootInstance, sWorkspace>,
	public Service,
	public VerbContainer,
	public IMovingManager,
	public IScriptFilter
{
private:
	friend class MouseCommand;
	typedef DescribedNonCreatable<Workspace, RootInstance, sWorkspace> Super;
public:
	static bool showWorldCoordinateFrame;
	static bool showHashGrid;
	static bool showEPhysicsOwners;		// default is false
	static bool showEPhysicsRegions;	// default is false
	static bool showStreamedRegions;	// default is false
    static bool showPartMovementPath;   // default to false
    static bool showActiveAnimationAsset;     // default to false

	static Reflection::PropDescriptor<Workspace, bool> prop_StreamingEnabled;
	bool getNetworkStreamingEnabled() const { return networkStreamingEnabled; }
	void setNetworkStreamingEnabled(bool value);

	static Reflection::PropDescriptor<Workspace, bool> prop_ExperimentalSolverEnabled;
	static Reflection::PropDescriptor<Workspace, bool> prop_ExpSolverEnabled_Replicate;
	static Reflection::PropDescriptor<Workspace, bool> prop_FilteringEnabled;
	static Reflection::PropDescriptor<Workspace, bool> prop_allowThirdPartySales;
	static Reflection::EnumPropDescriptor<Workspace, PhysicalPropertiesMode> prop_physicalPropertiesMode;
	bool getExperimentalSolverEnabled() const { return experimentalSolverEnabled; }
	void setExperimentalSolverEnabled(bool value);
	bool experimentalSolverIsEnabled();  // getter for script accessibility
	bool getExpSolverEnabled_Replicate() const { return expSolverEnabled_Replicate; }
	void setExpSolverEnabled_Replicate(bool value);

	bool getUsingNewPhysicalProperties() const;
	
	PhysicalPropertiesMode getPhysicalPropertiesMode() const;
	void setPhysicalPropertiesMode( PhysicalPropertiesMode mode );
	void setPhysicalPropertiesModeNoEvents( PhysicalPropertiesMode mode );

	static float gridSizeModifier;

	float getFallenPartDestroyHeight() const;
	void setFallenPartDestroyHeight(float value);

	bool getNetworkFilteringEnabled() const { return networkFilteringEnabled; }
	void setNetworkFilteringEnabled(bool value);

	bool getAllowThirdPartySales() const { return allowThirdPartySales; }
	void setAllowThirdPartySales(bool value);

	// finds the top level Instance in the workspace (right below the workspace)
	static Instance* findTopInstance(Instance* context);		
	
	static Workspace* findWorkspace(Instance* context);
	static const Workspace* findConstWorkspace(const Instance* context);

	// returns getWorld if context is within the RootInstance
	static World* getWorldIfInWorkspace(Instance* context);
	static ContactManager* getContactManagerIfInWorkspace(Instance* context);

	// returns getWorld if context is within the RootInstance
	static Workspace* getWorkspaceIfInWorkspace(Instance* context);

	void joinToOutsiders(shared_ptr<const Instances> items, AdvArrowToolBase::JointCreationMode joinType);
	void unjoinFromOutsiders(shared_ptr<const Instances> items);

	// returns true if the context lies within the workspace
	static bool contextInWorkspace(const Instance* context);

	bool startDecalDrag(Decal *decal, RBX::InsertMode insertMode);
	bool startPartDropDrag(const Instances& instances, bool suppressPartsAlign = false);

	void createTerrain();
	void clearTerrain();

	bool getShow3DGrid() { return show3DGrid; }
	bool getShowAxisWidget() { return showAxisWidget; }

	void setShow3DGrid(bool value) { show3DGrid = value; }
	void setShowAxisWidget(bool value) { showAxisWidget = value; }

private:
	boost::shared_ptr<IAdornableCollector> adornableCollector;

	// cached world extents
	Extents									lastComputedWorldExtents;
	double									lastComputedWorldExtentsTime;

	shared_ptr<Instance>					statsItem;
	shared_ptr<Instance>					terrain;
	IDataState*								dataState;
	bool									arrowCameraControls;

	shared_ptr<MouseCommand>				currentCommand;		// current MouseCommand being used;
	shared_ptr<MouseCommand>				stickyCommand;		// last sticky command - if setting to NULL, this will be used
	shared_ptr<RBX::InputObject>			idleMouseEvent;		// copy this for the idle event

	int										flySteps;			// used to accelerated cameraFly

	bool									firstPersonCam;		// if true, local player is in first person, false otherwise

	// UGLY place to put this, only here because other mouse code is here. Put in UserController?
	bool									inRightMousePan;
    bool                                    inMiddleMouseTrack;
	bool									leftMouseDown;

	// 3d adorns in studio
	bool show3DGrid;
	bool showAxisWidget;

	shared_ptr<Camera>						currentCamera;		// points to either a camera descedent of me or utility camera
	shared_ptr<Camera>						utilityCamera;		// used if no cameras in me - not shown in tree

	// Utility arrays to reduce resizes/ memory allocation
	std::vector<shared_ptr<PartInstance> >	fallenParts;
	G3D::Array<Primitive*>					fallenPrimitives;

	// Hack - replace with some Rackspace concept.  Should only be broadcast from server to clients
	double distributedGameTime;	

	// stats the datamodel fills on on file load.
	double	statsSyncHttpGetTime;
	double	statsXMLLoadTime;
	double	statsJoinAllTime;

	// used only on server, enables data to be gradually streamed to the client
	bool networkStreamingEnabled;
	bool experimentalSolverEnabled;
	bool expSolverEnabled_Replicate;
	bool networkFilteringEnabled; 

    // disable 3rd party sales for this place
	bool allowThirdPartySales; 

	weak_ptr<SurfaceGui> lastSurfaceGUI; // used by SG to track certain cases of mouse event processing

	void setRightMousePan();
	void setMiddleMouseTrack();

    // Updates the player mouse (if one exists) with the current command.
    void updatePlayerMouseCommand();

	void clearEmptiedModels(shared_ptr<Instance>& test);
	void detachParent(Instance* instance);

public:
	// Do this on any mouse up for now - not related to "command" structure;
	void cancelRightMousePan();
	bool getRightMousePan() const { return inRightMousePan; }
	void cancelMiddleMouseTrack();
	bool getMiddleMouseTrack() const { return inMiddleMouseTrack; }
	rbx::signal<void(const TouchPair&)> stepTouch;	// called for each touch on a step
	rbx::signal<void(shared_ptr<Camera>)> currentCameraChangedSignal;
    float renderingDistance;

	void replenishCamera();

private:

	////////////////////////////////////////////////////////////////////////////////////
	// INSTANCE
	/*override*/ bool askAddChild(const Instance* instance) const;
	/*override*/ void onDescendantAdded(Instance* instance);
	/*override*/ void onDescendantRemoving(const shared_ptr<Instance>& instance);
	/*override*/ void onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider);

public:
	ContentId getCursor(); // based on current mouse command

	bool hasModalGuiObjects();

	void setTerrain(Instance* terrain);
	Instance* getTerrain() const;

	bool forceDrawConnectors() const;	// should the rendering engine show connectors?

	void append3dSortedAdorn(std::vector<AdornableDepth>& sortedAdorn);
	////////////////////////////////////////////////////////////////////////////////////
	// 
	// IAdornable
	/*override*/ void render2d(Adorn* adorn);
	/*override*/ void render3dAdorn(Adorn* adorn);
	/*override*/ void render3dSelect(Adorn* adorn, SelectState selectState) {}	// override ModelInstance and do nothing

	// single ignore instance
	shared_ptr<const Instances> findPartsInRegion3(Region3 region, shared_ptr<Instance> ignoreDescendent, int maxCount);
	bool isRegion3Empty(Region3 region, shared_ptr<Instance> ignoreDescendent);

	// table of ignore instances
	shared_ptr<const Instances> findPartsInRegion3(Region3 region, shared_ptr<const Instances> ignoreDescendents, int maxCount);
	bool isRegion3Empty(Region3 region, shared_ptr<const Instances> ignoreDescendents);

	// result->at(0) is the part found ( shared_ptr<PartInstance> )
	// result->at(1) is the point of intersection (Vector3)
	template<class IgnoreType>
	shared_ptr<const Reflection::Tuple> getRayHit(RbxRay ray, shared_ptr<IgnoreType> ignore, bool terrainCellsAreCubes = false, bool ignoreWaterCells = false);


	boost::shared_ptr<IAdornableCollector>& getAdornableCollector() { return adornableCollector; }

	// IScriptFilter
	/*override*/ bool scriptShouldRun(BaseScript* script);

	/*override*/ Extents computeExtentsWorldSlow() const {return RootInstance::computeExtentsWorld();}
	Extents computeExtentsWorldFast();

private:
	/*override*/ Extents computeExtentsWorld() {RBXASSERT(this->computeNumParts() < 25);	return RootInstance::computeExtentsWorld();}		// make sure nobody is calling this directly on the workspace?
	/*override*/ const ModelInstance* getCameraOwnerModel() const {return this;}

	rbx::signals::scoped_connection heartbeatConnection;
	void onHeartbeat(const Heartbeat& heartbeat);
	rbx::signals::scoped_connection setDefaultMouseCommandConnection;

public:
	boost::scoped_ptr<Profiling::CodeProfiler> profileDataModelStep;
	boost::scoped_ptr<Profiling::CodeProfiler> profileWorkspaceStep;
	boost::scoped_ptr<Profiling::CodeProfiler> profileWorkspaceAssemble;

	Workspace(IDataState* dataState);
	virtual ~Workspace();

	World* getWorld() {return world.get();}
	const World* getWorld() const {return world.get();}

	IDataState& getDataState() const { return *dataState; }

	double getStatsSyncHttpGetTime() const {return statsSyncHttpGetTime;}
	double getStatsXMLLoadTime() const {return statsXMLLoadTime;}
	double getStatsJoinAllTime() const {return statsJoinAllTime;}
	double getStatsFileTimeTotal() const { return statsSyncHttpGetTime + statsXMLLoadTime + statsJoinAllTime; }

	void setStatsSyncHttpGetTime(double value) {statsSyncHttpGetTime = value;}
	void setStatsXMLLoadTime(double value) {statsXMLLoadTime = value;}
	void setStatsJoinAllTime(double value) {statsJoinAllTime = value;}

	double getDistributedGameTime() const {return distributedGameTime;}
	void setDistributedGameTime(double value);
	void setDistributedGameTimeNoTransmit(double value);

	double getRealPhysicsFPS(void);
	int getPhysicsThrottling(void);
	int getNumAwakeParts(void);

	/////////////////////////////////////////////////////////////////
	// Mouse Commands
	//
	// TODO: refactor: Move into ToolManager Service

	MouseCommand* getCurrentMouseCommand() {
		RBXASSERT(currentCommand.get()!=NULL);
		return currentCommand.get();
	}

	void cancelMouseCommand();

	void setMouseCommand(shared_ptr<MouseCommand> newMouseCommand, bool allowPluginOverride = false);
	void setDefaultMouseCommand();
	void setNullMouseCommand();

	void requestFirstPersonCamera(bool firstPersonOn, bool cameraTransitioning, int controlMode);	

	////////////////////////////////////////////////////////////////////////////////////
	//
	// Camera
	//
	// CAMERA OWNER
	/*override*/ Camera* getCamera();	
	/*override*/ const Camera* getConstCamera() const;	
	
	// Don't use these - internal pointer to the current active camera, could be NULL
	Camera* getCurrentCameraDangerous() const;
	void setCurrentCamera(Camera *value); 

	///////////////////////////////////////////////////////////////////////////////
	//
	// Video Control
	//

	// Used by image server - standard ~45 degree view looking down -z, centered
	// Returns true if it creates a new ThumbnailCamera
	bool setImageServerView(bool bIsPlace);
	int imageServerViewHack;			// super hack - for now, on setting image server view, renders all hopper bins full screen
	void zoomToExtents();

	void onWrapMouse(const Vector2& wrapMouseDelta);

	//////////////////////////////////////////////////////////////
	//
	// Processing of mouse and commands
	//
	/*override*/ virtual GuiResponse process(const shared_ptr<InputObject>& inputObject);	

	//////////////////////////////////////////////////////////////
	//
	// Selection
	//
	void selectAllTopLevelRenderable();

	//////////////////////////////////////////////////////////////
	//
	// Engine management
private:
	void handleFallenParts();
	void updateDistributedGameTime();

public:
	static bool serverIsPresent(const Instance* context);		// shortcut for RBX::Network::Players::serverIsPresent
	static bool clientIsPresent(const Instance* context);		// shortcut for RBX::Network::Players::clientIsPresent

	void start();
	void stop();
	void reset();		
	int updatePhysicsStepsRequiredForCyclicExecutive(float timeInterval);
	float physicsStep(bool longStep, float timeInterval, int numThreads);
	void assemble();

	void doNothing(bool value) {}

	void joinAllHack();			// Joins all primitives - called after a file read

	void makeJoints(shared_ptr<const Instances> instances);
	void breakJoints(shared_ptr<const Instances> instances);

    GuiResponse handleSurfaceGui(const shared_ptr<InputObject>& event);

    //////////////////////
    //
    // Physics analyzer
    //
public:
    void setPhysicsAnalyzerBreakOnIssue( bool enable );
    bool getPhysicsAnalyzerBreakOnIssue( );
    rbx::signal<void(int)> luaPhysicsAnalyzerIssuesFound;
    shared_ptr<const Instances> getPhysicsAnalyzerIssue( int group );
};

} // namespace
