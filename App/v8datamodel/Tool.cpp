/* Copyright 2003-2007 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/Tool.h"
#include "V8DataModel/Backpack.h"
#include "V8DataModel/PartInstance.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/Workspace.h"
#include "V8DataModel/TimerService.h"
#include "V8DataModel/Accoutrement.h"
#include "V8DataModel/Attachment.h"
#include "Humanoid/Humanoid.h"
#include "V8DataModel/ToolMouseCommand.h"
#include "V8DataModel/Mouse.h"
#include "V8DataModel/UserInputService.h"
#include "V8DataModel/GuiService.h"
#include "Network/Players.h"
#include "Network/Player.h"
#include "Tool/DragUtilities.h"
#include "Script/Script.h"
#include "FastLog.h"

LOGGROUP(UserInputProfile)

FASTFLAG(StudioDE6194FixEnabled)

namespace RBX {

using namespace RBX::Network;

const char* const sTool = "Tool";

REFLECTION_BEGIN();
static const Reflection::PropDescriptor<Tool, CoordinateFrame> prop_Grip("Grip", category_Appearance, &Tool::getGrip, &Tool::setGrip, Reflection::PropertyDescriptor::STANDARD);
static const Reflection::PropDescriptor<Tool, Vector3> prop_GripPos("GripPos", category_Appearance, &Tool::getGripPos, &Tool::setGripPos, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Tool, Vector3> prop_GripForward("GripForward", category_Appearance, &Tool::getGripForward, &Tool::setGripForward, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Tool, Vector3> prop_GripUp("GripUp", category_Appearance, &Tool::getGripUp, &Tool::setGripUp, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Tool, Vector3> prop_GripRight("GripRight", category_Appearance, &Tool::getGripRight, &Tool::setGripRight, Reflection::PropertyDescriptor::UI);
static const Reflection::PropDescriptor<Tool, std::string> prop_ToolTip("ToolTip", category_Appearance, &Tool::getToolTip, &Tool::setToolTip);
    
static const Reflection::PropDescriptor<Tool, bool> prop_manualActivation("ManualActivationOnly", category_Behavior, &Tool::getManualActivationOnly, &Tool::setManualActivationOnly);
static const Reflection::PropDescriptor<Tool, bool> prop_CanDrop("CanBeDropped", category_Behavior, &Tool::isDroppable, &Tool::setDroppable);
static const Reflection::PropDescriptor<Tool, bool> prop_requiresHandle("RequiresHandle", category_Behavior, &Tool::getRequiresHandle, &Tool::setRequiresHandle);

Reflection::BoundProp<bool> Tool::prop_Enabled("Enabled", "State", &Tool::enabled);

// Backend events
static Reflection::EventDesc<Tool, void(shared_ptr<Instance>), Tool::special_equipped_signal> event_Equipped(
	&Tool::equippedSignal,
	"Equipped", "mouse");
static Reflection::EventDesc<Tool, void()> event_Unequipped(&Tool::unequippedSignal, "Unequipped");
static Reflection::RemoteEventDesc<Tool, void()> event_Activated(&Tool::activatedSignal, "Activated", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);
static Reflection::RemoteEventDesc<Tool, void()> event_Deactivated(&Tool::deactivatedSignal, "Deactivated", Security::None, Reflection::RemoteEventCommon::SCRIPTING, Reflection::RemoteEventCommon::CLIENT_SERVER);

static Reflection::BoundFuncDesc<Tool, void()> func_Activate(&Tool::luaActivate, "Activate", Security::None);
REFLECTION_END();

Tool::special_equipped_signal::special_equipped_signal() :
	Super(), currentlyEquipped(false), lastArg() {}

void Tool::special_equipped_signal::operator()(shared_ptr<Instance> instance)
{
	// This operator is used by EventDesc.fireEvent(). In order to track
	// the equipped/unequipped state correctly, use the equipped() or
	// unequipped() methods instead.
	throw std::runtime_error("Don't use Event.fireEvent for equipped signal!");
}

void Tool::special_equipped_signal::equipped(shared_ptr<Instance> instance)
{
	currentlyEquipped = true;
	lastArg = instance;
	Super::operator()(instance);
}

void Tool::special_equipped_signal::unequipped()
{
	currentlyEquipped = false;
	lastArg.reset();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// FRONTEND AND BACKEND

Tool::Tool() 
: DescribedCreatable<Tool, BackpackItem, sTool>()
, backendToolState(NOTHING)
, frontendActivationState(0)
, enabled(true)
, workspaceForToolMouseCommand(NULL)
, handleTouched(FLog::TouchedSignal != 0)
, droppable(true)
, toolTip("")
, requiresHandle(true)
, manualActivationOnly(false)
, ownWeld(false)
{
	Instance::setName(sTool);
}

Tool::~Tool()
{
	RBXASSERT(!handleTouched.connected());
	RBXASSERT(!characterChildAdded.connected());
	RBXASSERT(!characterChildRemoved.connected());
	RBXASSERT(!torsoChildAdded.connected());
	RBXASSERT(!torsoChildRemoved.connected());
}

void Tool::render3dSelect(Adorn* adorn, SelectState selectState)
{
	for (size_t i = 0; i < this->numChildren(); ++i)
	{
		if (IAdornable* iRenderable = dynamic_cast<IAdornable*>(this->getChild(i))) 
		{
			iRenderable->render3dSelect(adorn, selectState);
		}
	}
}

bool Tool::characterCanUnequipTool(ModelInstance *character)
{
	if (character == NULL) 
		return true;

	Humanoid* humanoid = Humanoid::modelIsCharacter(character);
	if (humanoid)
	{
		Tool *tool = Instance::fastDynamicCast<Tool>(character->findFirstChildOfType<Tool>());
		if (tool) 
			return tool->canUnequip();
	}
	return true;
}

bool Tool::isSelectable3d()
{
    return !FFlag::StudioDE6194FixEnabled || countDescendantsOfType<PartInstance>() > 0;
}

PartInstance* Tool::getHandle()
{
	return const_cast<PartInstance*>(getHandleConst());
}

const PartInstance* Tool::getHandleConst() const
{
	return Instance::fastDynamicCast<PartInstance>(findConstFirstChildByName("Handle"));
}


const CoordinateFrame Tool::getLocation() 
{
	const PartInstance* handle = getHandleConst();

	return handle ? handle->getCoordinateFrame() : CoordinateFrame();
}

// Must be called externally - changes tool parent
void Tool::dropAll(Network::Player* player)
{
	if (ModelInstance* character = player->getCharacter())
	{
		if (Workspace* workspaceLocal = Workspace::findWorkspace(character))
		{
			while (Tool* tool = character->findFirstChildOfType<Tool>())
			{			
				if (!tool->isDroppable()) tool->setParent(player->getPlayerBackpack());
				else tool->setParent(workspaceLocal);  // ok - should drop!  check it out
			}
		}
	}
}

void Tool::moveAllToolsToBackpack(Network::Player* player)
{
	if (player)
	{
		if (ModelInstance* m = player->getCharacter())
		{
			while (Tool* tool = m->findFirstChildOfType<Tool>())
			{
				tool->setParent(player->getPlayerBackpack());
			}
		}
	}
}

shared_ptr<Mouse> Tool::createMouse()
{
	shared_ptr<Mouse> mouse = Creatable<Instance>::create<Mouse>();
	mouse->setWorkspace(NULL);
	return mouse;
}


/*
						watch_touch	
	ACTIVATED				
	EQUIPPED (Weld)			
	HAS_TORSO				x
	IN_CHARACTER			x
	IN_WORKSPACE			x	-> needs touch or parent already set to proceed
	HAS_HANDLE				x	
	NOTHING
*/

void Tool::setBackendToolState(int value)
{
	if (value != backendToolState)
	{
		if ((backendToolState < EQUIPPED) && (value >= EQUIPPED))
		{
			shared_ptr<Mouse> mouse = onEquipping();
			if (!mouse.get()) {
				mouse = createMouse();
			}
            currentMouse = mouse;

			equippedSignal.equipped(mouse);
		}
		if ((backendToolState >= EQUIPPED) && (value < EQUIPPED))
		{
			equippedSignal.unequipped();
			unequippedSignal();
			if (currentMouse)
			{
				currentMouse->setIcon(RBX::TextureId());
			}
			onUnequipped();
		}
		backendToolState = static_cast<ToolState>(value);
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//
// BACKEND

void Tool::connectTouchEvent()
{
	PartInstance* handle = getHandle();

	if (handle)
	{
		handleTouched = handle->onDemandWrite()->touchedSignal.connect(boost::bind(&Tool::onEvent_HandleTouched, shared_from(this), _1));
		FASTLOG3(FLog::TouchedSignal, "Connecting Tool to touched signal, instance: %p, part: %p, part signal: %p", 
			this, handle, &(handle->onDemandRead()->touchedSignal));
	}
	else
	{
		FASTLOG2(FLog::TouchedSignal, "Disconnecting handle Touched since we don't have part. Tool: %p, connection: %p", this, &handleTouched);
//		RBXASSERT(0);					// only call if >= the HAS_HANDLE state
		handleTouched.disconnect();		// just to be safe
	}
}

void Tool::rebuildBackendState()
{
	ToolState desiredState = computeDesiredState();

	const ServiceProvider* serviceProvider = ServiceProvider::findServiceProvider(this);
	RBXASSERT(serviceProvider);
	
	setDesiredState(desiredState, serviceProvider);
}

/*
	Backend:
						watch_touch		
	EQUIPPED (Weld)			
	HAS_TORSO				x
	IN_CHARACTER			x
	IN_WORKSPACE			x	-> needs touch or parent already set to proceed
	HAS_HANDLE				x	-> unless not in workspace
	NOTHING
*/

Tool::ToolState Tool::computeDesiredState()
{
	PartInstance* handle = getHandle();
	if (!handle && getRequiresHandle()) {
		return NOTHING;
	}

	if (!Workspace::contextInWorkspace(this)) {
		return NOTHING; // if tool is not in workspace, no need to listen to touch events
	}

	return computeDesiredState(getParent());
}

Tool::ToolState Tool::computeDesiredState(Instance* testParent)
{
	Humanoid* humanoid = Humanoid::modelIsCharacter(testParent);
	if (!humanoid) {
		return IN_WORKSPACE;
	}

	if (!humanoid->getTorsoSlow()) {
		return IN_CHARACTER;
	}

	if (humanoid->getUseR15())
	{
		if (Instance::fastDynamicCast<PartInstance>(humanoid->getParent()->findFirstChildByName("RightHand")) == NULL)
			return HAS_TORSO;
	} else {
		if (!(humanoid->getRightArmSlow() && humanoid->getRightShoulder())) {
			return HAS_TORSO;
		}
	}

	
	return EQUIPPED;
}


void Tool::setDesiredState(ToolState desiredState, const ServiceProvider* serviceProvider)
{
#ifdef RBXASSERTENABLED
	int numToolsInCharacter;
#endif
	RBXASSERT((numToolsInCharacter = getNumToolsInCharacter()) || 1);

	// short cut
	if (desiredState == EQUIPPED && backendToolState ==  NOTHING)
		fromNothingToEquipped(Network::Players::backendProcessing(serviceProvider));
	else if (desiredState == NOTHING && backendToolState == EQUIPPED)
		fromEquippedToNothing();
	else
	{
		ToolState currentState = backendToolState;
		// go the long way	
		if (desiredState > currentState)
		{
			do 
			{
				switch (currentState)
				{
				case EQUIPPED:									break;
				case HAS_TORSO:				upTo_Equipped();	break;
				case IN_CHARACTER:			upTo_HasTorso();	break;
				case IN_WORKSPACE:			upTo_InCharacter(); break;
				case HAS_HANDLE:			upTo_InWorkspace();	break;
				case NOTHING:				upTo_HasHandle();	break;
				}

				currentState = static_cast<ToolState>(currentState + 1);

			} while (desiredState > currentState);
		}
		else if (desiredState < currentState)
		{
			do 
			{
				switch (currentState)
				{
				case EQUIPPED:				downFrom_Equipped(Network::Players::backendProcessing(serviceProvider));	break;
				case HAS_TORSO:				downFrom_HasTorso();	break;
				case IN_CHARACTER:			downFrom_InCharacter(); break;
				case IN_WORKSPACE:			downFrom_InWorkspace();	break;
				case HAS_HANDLE:			downFrom_HasHandle();	break;
				case NOTHING:				RBXASSERT(0);	break;
				}

				currentState = static_cast<ToolState>(currentState - 1);

			} while (desiredState < currentState);
		}
		
		setBackendToolState(currentState);
	}

	RBXASSERT(numToolsInCharacter == getNumToolsInCharacter());
}

void Tool::fromNothingToEquipped(bool isBackend)
{
	// No need to call upTo_HasHandle, because we don't need handle touch events when the tool is equipped
	RBXASSERT(!handleTouched.connected());

	setBackendToolState(EQUIPPED);

	if (isBackend)
	{
		upTo_InWorkspace();
		upTo_InCharacter();
		upTo_HasTorso();
	}

	upTo_Equipped();
}

void Tool::fromEquippedToNothing()
{
	RBXASSERT(!handleTouched.connected());

	downFrom_Equipped(false);	// specify false here to indicate we don't need handle touch events
	downFrom_HasTorso();
	downFrom_InCharacter();
	downFrom_InWorkspace();
	// No need to call downFrom_HasHandle, because handle touch events should already be disconnected

	setBackendToolState(NOTHING);
}

void Tool::upTo_HasHandle()
{
	connectTouchEvent();
}

void Tool::downFrom_HasHandle()
{
	FASTLOG2(FLog::TouchedSignal, "Disconnecting handle in downFrom_HasHandle. Tool: %p, connection: %p", this, &handleTouched);
	handleTouched.disconnect();
}

void Tool::upTo_InWorkspace()
{
	RBXASSERT(ServiceProvider::findServiceProvider(this));
}

void Tool::downFrom_InWorkspace()
{
}

void Tool::upTo_InCharacter()
{
	RBXASSERT(Humanoid::modelIsCharacter(getParent()));

	if (getParent())
	{
		characterChildAdded = getParent()->onDemandWrite()->childAddedSignal.connect(boost::bind(&Tool::onEvent_AddedBackend, shared_from(this), _1));
		characterChildRemoved = getParent()->onDemandWrite()->childRemovedSignal.connect(boost::bind(&Tool::onEvent_RemovedBackend, shared_from(this), _1));
	}
	else
	{
		characterChildAdded.disconnect();
		characterChildRemoved.disconnect();
	}
}

void Tool::downFrom_InCharacter()
{
	characterChildAdded.disconnect();
	characterChildRemoved.disconnect();
}

void Tool::upTo_HasTorso()
{
	Humanoid* humanoid = Humanoid::modelIsCharacter(getParent());
	RBXASSERT(humanoid);
	PartInstance* torso = humanoid->getTorsoSlow();
	RBXASSERT(torso);
	torsoChildAdded = torso->onDemandWrite()->childAddedSignal.connect(boost::bind(&Tool::onEvent_AddedBackend, shared_from(this), _1));
	torsoChildRemoved = torso->onDemandWrite()->childRemovedSignal.connect(boost::bind(&Tool::onEvent_RemovedBackend, shared_from(this), _1));
}

void Tool::downFrom_HasTorso()
{
	torsoChildAdded.disconnect();
	torsoChildRemoved.disconnect();
}


void Tool::upTo_Equipped()
{
	FASTLOG2(FLog::TouchedSignal, "Disconnecting handle in upTo_Equipped. Tool: %p, connection: %p", this, &handleTouched);
	// Disconnect listening for touches
	handleTouched.disconnect();

	// Build the weld
	Humanoid* humanoid = Humanoid::modelIsCharacter(getParent());
	if (!humanoid)
		return;

	PartInstance* arm;
	if (humanoid->getUseR15())
	{
		arm = Instance::fastDynamicCast<PartInstance>(humanoid->getParent()->findFirstChildByName("RightHand"));
	} else {
		arm = humanoid->getRightArmSlow();
	}
	if (!arm)
		return;

	PartInstance* handle = getHandle();

	if (ownWeld)
	{
		if (handle) 
		{
			// This will be blocked by filter enabled on the client, but server will also run this code and set correct value
			handle->setCanCollide(false);
				
			buildWeld(arm, handle, humanoid->getRightArmGrip(), grip, "RightGrip");
		}
	}
	else if (Network::Players::backendProcessing(this))
	{
		if (handle)
		{
			// search for weld
			weld = Instance::fastSharedDynamicCast<Weld>(arm->findFirstChildByName2("RightGrip", false));

			if (!weld)
				armChildAdded = arm->onDemandWrite()->childAddedSignal.connect(boost::bind(&Tool::onEvent_AddedToArmBackend, shared_from(this), _1, handle));

			handle->setCanCollide(false);
		}
	}
}

void Tool::downFrom_Equipped(bool connectTouchEvent)
{
	ModelInstance* oldCharacter = NULL;

	if (weld)
	{
		armChildAdded.disconnect();

		if (PartInstance* oldArm = weld->getPart0())
		{
			oldCharacter = Instance::fastDynamicCast<ModelInstance>(oldArm->getParent());
		}
		weld->setParent(NULL);
		weld.reset();
	}

	if (PartInstance* handle = getHandle())
	{
		handle->setCanCollide(true);

		if (oldCharacter && Humanoid::modelIsCharacter(oldCharacter))
		{
			CoordinateFrame cf = oldCharacter->getLocation();
			Vector3 pos = cf.translation;
			pos.y += 4;
			pos += cf.lookVector() * 8; 
				
			handle->moveToPointNoUnjoinNoJoin(pos);				// important - otherwise may cause recursive unjoining
			handle->setRotationalVelocity(Vector3(1.0f, 1.0f, 1.0f));
		}
	}

	if (connectTouchEvent)
	{
		// Dropping - must start listening again
		this->connectTouchEvent();
	}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Backend stuff - events


// Used for both the tool and the torso
//
// Character: RightArm, Tool
// Torso: Shoulder?
//
void Tool::onEvent_AddedBackend(shared_ptr<Instance> child)
{
	if (child.get() != this)	// tool parent changed is handled in ancestor stuff, because this might not be hooked up
	{
		RBXASSERT(ServiceProvider::findServiceProvider(this));
		rebuildBackendState();
	}
}

void Tool::onEvent_RemovedBackend(shared_ptr<Instance> child)
{
	// there's a chance this is being called when a character is being removed - trap here

	if (ServiceProvider::findServiceProvider(this))
	{
		if (child.get() != this)	// tool parent changed is handled in ancestor stuff, because this might not be hooked up
		{
			if (child.get() == weld.get())
			{
				RBXASSERT(backendToolState >= EQUIPPED);
				setDesiredState(HAS_TORSO, ServiceProvider::findServiceProvider(this));	// force down, nuke the weld
			}
			rebuildBackendState();
		}
	}
}

void Tool::onChildAdded(Instance* child)
{
	if (ServiceProvider::findServiceProvider(this) && Network::Players::backendProcessing(this))
	{
		rebuildBackendState();
	}
}

void Tool::onChildRemoved(Instance* child)
{
	Super::onChildRemoved(child);
	if (ServiceProvider::findServiceProvider(this) && Network::Players::backendProcessing(this))
	{
		rebuildBackendState();
	}
}

void Tool::onEvent_AddedToArmBackend(shared_ptr<Instance> child, Instance *handle)
{
	shared_ptr<Weld> w = Instance::fastSharedDynamicCast<Weld>(child);
	if (w && w->getName() == "RightGrip" && w->getPart1() == handle)
	{
		RBXASSERT(!weld);
		weld = w;

		armChildAdded.disconnect();
	}
}

// This is a backend event
void Tool::onEvent_HandleTouched(shared_ptr<Instance> other)
{
	// this is probably going off because you are trying to JOIN a game in distributed physics mode with no server found
	RBXASSERT(Network::Players::backendProcessing(this));			
	
	if ((backendToolState == IN_WORKSPACE) && other)
	{
		Instance* touchingCharacter = other->getParent();
		if (computeDesiredState(touchingCharacter) == EQUIPPED)
		{
			if (characterCanUnequipTool(Instance::fastDynamicCast<ModelInstance>(touchingCharacter))) {

				Player *p = Players::getPlayerFromCharacter(touchingCharacter);
				if (p && canBePickedUpByPlayer(p)) {
					moveAllToolsToBackpack(p);
					setParent(touchingCharacter);
					RBXASSERT(backendToolState == EQUIPPED);

					// Move any other tool to the backpack, should the player 
					// have more than one equipped at the same time.
					boost::weak_ptr<Player> player_weak_ptr = weak_from<Player>(p);
					setTimerCallback(player_weak_ptr);
				}
			}
		}
	}
}

static void disableHopperBin(Instance* instance)
{
	HopperBin* hopperBin = instance->findFirstChildOfType<HopperBin>();

	if (hopperBin)
		hopperBin->disable();
}

// Sets a timer callback so we can move any other
// tool to the player's backpack should there
// be multiple tools equipped at one time.
//
// This happens when the player touches a tool 
// that's on the ground at the same time that he
// equips one from his backpack.
//
// This is not the perfect solution, nor it's pretty, but 
// it's less invasive than rewriting a bunch of things in
// order to fix this bug.
void Tool::setTimerCallback(weak_ptr<Network::Player> player)
{
	TimerService* callback = ServiceProvider::create<TimerService>(this);
	if (callback)
	{
		boost::function<void()> func = boost::bind(&Tool::moveOtherToolsToBackpack, 
			this, player);
		callback->delay(func, 0.2);
	}
}

// Moves tool pointed by "child" to the backpack, if it's a tool other 
// than the one we'd like to keep (toolToKeep).
//
// See Tool::setTimerCallback for more details on where this is used.
static void moveToBackpack(shared_ptr<Instance> child, Tool* toolToKeep, 
						   Backpack* backpack)
{
	Tool* tool = Instance::fastDynamicCast<Tool>(child.get());

	// We only care about the other tools
	if (!tool || tool == toolToKeep)
		return;

	// Move it to the backpack
	tool->setParent(backpack);
}

// Moves all other instances of Tool to the player's backpack.
//
// See Tool::setTimerCallback comments for more details on where this is used.
void Tool::moveOtherToolsToBackpack(weak_ptr<Network::Player> player_weak_ptr)
{
	boost::shared_ptr<Player> player = player_weak_ptr.lock();

	if (backendToolState < EQUIPPED || !player)
		return;

	Backpack* backpack = player->getPlayerBackpack();
	if (!backpack)
		return;

	ModelInstance* character = player->getCharacter();
	if (!character)
		return;

	character->visitChildren(boost::bind(&moveToBackpack, _1, this, backpack));

	disableHopperBin(backpack);
}

// Debugging
int Tool::getNumToolsInCharacter()
{
	int answer = 0;
	if (Humanoid::modelIsCharacter(this->getParent()))
	{
		Instance* character = this->getParent();
		for (size_t i = 0; i < character->numChildren(); ++i)
		{
			Instance* child = character->getChild(i);
			if (Instance::fastDynamicCast<Tool>(child))
			{
				answer++;
			}
		}
	}
	return answer;
}
	
void Tool::onAncestorChanged(const AncestorChanged& event)
{
#ifdef RBXASSERTENABLED
	int numToolsInCharacter;
#endif

	RBXASSERT((numToolsInCharacter = getNumToolsInCharacter()) || 1);

	const ServiceProvider* oldProvider = ServiceProvider::findServiceProvider(event.oldParent);
	const ServiceProvider* newProvider = ServiceProvider::findServiceProvider(event.newParent);

	if (oldProvider)
	{
		ownWeld = false;

		if (Network::Players::backendProcessing(oldProvider))
		{
			setDesiredState(NOTHING, oldProvider);
		}
		else
		{
			setBackendToolState(NOTHING);
		}
	}

	Super::onAncestorChanged(event);

	if (newProvider)
	{
		bool frontendProcessing = Network::Players::frontendProcessing(newProvider);
		bool backendProcessing = Network::Players::backendProcessing(newProvider);

		RBXASSERT(ServiceProvider::findServiceProvider(this) == newProvider);

		if (backendProcessing)
		{
			Player* player = NULL;

			if (frontendProcessing) // solo play
				ownWeld = true;
			else
			{
				// if this tool is being parented to NPC, backend will take care of everything
				if (Humanoid::modelIsCharacter(getParent())) 
				{
					player = Network::Players::getPlayerFromCharacter(getParent());
					if(!player)
					{
						ownWeld = true;
					}
				}
			}


			// backend should own weld for all tools not being equipped from back backpack
			if (player && event.oldParent != player->getPlayerBackpack())
				ownWeld = true;

			rebuildBackendState();
		}
		else // frontend
		{
			Player* player = Players::findLocalPlayer(this);
			if (player && (getParent() == player->getCharacter()) /*&& getHandle()*/) 
			{
				// frontend should not create weld if tool came from outside the players backpack
				if (event.oldParent == player->getPlayerBackpack())
					ownWeld = true;
					
				rebuildBackendState();
			}
				
		}
	}

	// Checking to make sure that we don't have setParent byproducts here
	RBXASSERT(numToolsInCharacter == getNumToolsInCharacter());
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Frontend only stuff
//


void Tool::onUnequipped()
{
	if (workspaceForToolMouseCommand) {
		workspaceForToolMouseCommand->setDefaultMouseCommand();
		workspaceForToolMouseCommand = NULL;
	}
}


shared_ptr<Mouse> Tool::onEquipping()
{
	if (getParent()){
		if(Network::Players::frontendProcessing(this)){
			Player* player = Players::findLocalPlayer(this);
			if(player && player->getCharacter() == getParent()){
				workspaceForToolMouseCommand = ServiceProvider::find<Workspace>(this);
				shared_ptr<ToolMouseCommand> toolMouseCommand = Creatable<MouseCommand>::create<ToolMouseCommand>(workspaceForToolMouseCommand, this);
				workspaceForToolMouseCommand->setMouseCommand(toolMouseCommand);
                currentToolMouseCommand = toolMouseCommand;
				return toolMouseCommand->getMouse();
			}
		}
	}

	shared_ptr<Mouse> nothing;
	return nothing;
}
    
void Tool::setMousePositionForInputType()
{
    if (RBX::UserInputService* inputService = RBX::ServiceProvider::find<RBX::UserInputService>(this))
    {
        if (inputService->getGamepadEnabled() )
        {
			if (currentMouse)
			{
				currentMouse->update(inputService->getCurrentMousePosition());
			}

			if (currentToolMouseCommand)
			{
				currentToolMouseCommand->onMouseDown(inputService->getCurrentMousePosition());
			}
        }
        // if we are a touch device, make sure mouse points to center of all touches
        else if (inputService->getTouchEnabled())
        {
            UserInputService::InputObjectVector currentTouches = inputService->getCurrentTouches();
            RBX::StandardOut::singleton()->printf(MESSAGE_INFO, "current num of touches is %lu",currentTouches.size());
            if (currentTouches.size() > 0)
            {
                Vector2 avgTouchPosition = Vector2::zero();
                int numOfTouches = 0;
                for(UserInputService::InputObjectVector::iterator iter = currentTouches.begin(); iter != currentTouches.end(); ++iter)
                {
                    avgTouchPosition += (*iter)->get2DPosition();
                    numOfTouches++;
                }
                avgTouchPosition = avgTouchPosition/numOfTouches;

                shared_ptr<InputObject> fakeMouseActivatePosition = RBX::Creatable<RBX::Instance>::create<RBX::InputObject>(InputObject::TYPE_MOUSEMOVEMENT, InputObject::INPUT_STATE_CHANGE,
                                                                        Vector3(avgTouchPosition.x,avgTouchPosition.y,0), Vector3::zero(), RBX::DataModel::get(this));
                
                if (currentMouse)
                {
                    currentMouse->update(fakeMouseActivatePosition);
                }
                
                if (currentToolMouseCommand)
                {
                    currentToolMouseCommand->onMouseDown(fakeMouseActivatePosition);
                }
            }
        }
    }
}
    
void Tool::setManualActivationOnly(bool value)
{
    if (value != manualActivationOnly)
    {
        manualActivationOnly = value;
        raisePropertyChanged(prop_manualActivation);
    }
}

void Tool::luaActivate()
{
	if (backendToolState == EQUIPPED)
	{
        setMousePositionForInputType();
        
        activate(true);
	}
	else
	{
		RBX::StandardOut::singleton()->printf(MESSAGE_WARNING, "Tool:Activate() called from script when tool is not equipped. Tool will not be activated.");
	}
}

void Tool::activate(bool manuallyActivated)
{
    if (!manuallyActivated && getManualActivationOnly())
    {
        return;
    }
    
	FASTLOG(FLog::UserInputProfile, "Tool::activate");
	RBXASSERT(Network::Players::frontendProcessing(this));
	event_Activated.fireAndReplicateEvent(this);
}


void Tool::deactivate()
{
	RBXASSERT(Network::Players::frontendProcessing(this));
	event_Deactivated.fireAndReplicateEvent(this);
}

// Local Event - must be from outside
void Tool::onLocalClicked()
{
	RBXASSERT(Network::Players::frontendProcessing(this));

	if (Player* player = Players::findLocalPlayer(this))
	{
		if (characterCanUnequipTool(player->getCharacter()))
		{
			if (this->getParent() == player->getPlayerBackpack())
			{
				RBXASSERT(player);
				moveAllToolsToBackpack(player);
				setParent(player->getCharacter());
			}
			else
			{
				setParent(player->getPlayerBackpack());
			}
		}
	}

	// Confirm I'm not starting with a bad parent/child relationship - i.e., >1 tool in a character
	RBXASSERT(this->getNumToolsInCharacter() < 2);
}

void Tool::onLocalOtherClicked()
{
	RBXASSERT(Network::Players::frontendProcessing(this));

	if (Player* player = Players::findLocalPlayer(this))
	{
		if (characterCanUnequipTool(player->getCharacter()))
		{
			if (this->getParent() != player->getPlayerBackpack())
				setParent(player->getPlayerBackpack());
		}
	}
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

void cleanUpZeroColumn(Matrix3& r)
{
	bool zeroColumn = false;

	for (int i = 0; i < 3; ++i) {
		Vector3 c = r.column(i);						// Hack - prevent matrices with all 0 rows
		if (c.squaredMagnitude() < 1e-6f) {
			zeroColumn = true;
			break;
		}
	}

	if (zeroColumn) {
		r.setColumn(0, Vector3(0,-1,0));		//right
		r.setColumn(1, Vector3(1,0,0));			// up
		r.setColumn(2, Vector3(0,0,-1));		// front - corresponds to 0,0,1 from properties
	}
}


void Tool::setGrip(const CoordinateFrame& value)
{
	if (value != grip)
	{
		grip = value;

		cleanUpZeroColumn(grip.rotation);		// MEGA HACK!!!

		Math::orthonormalizeIfNecessary(grip.rotation);

		raisePropertyChanged(prop_Grip);

		if (weld != NULL)
		{
			// all this does not occur client side
			RBXASSERT(Network::Players::backendProcessing(this));
			weld->setC1(grip);
		}
	}
}

// Auxiliary UI props
const Vector3 Tool::getGripPos() const
{
	return grip.translation;
}

const Vector3 Tool::getGripForward() const
{
	return grip.lookVector();
}

const Vector3 Tool::getGripUp() const
{
	return grip.upVector();
}

const Vector3 Tool::getGripRight() const
{
	return grip.rightVector();
}

void Tool::setGripPos(const Vector3& v)
{
	setGrip(CoordinateFrame(grip.rotation, v));
}

void Tool::setGripForward(const Vector3& v)
{
	CoordinateFrame c(grip);

	//Vector3 oldX = c.rightVector();
	Vector3 oldY = c.upVector();

	Vector3 z = -Math::safeDirection(v);

	Vector3 x = oldY.cross(z);
	x.unitize();

	Vector3 y = z.cross(x);
	y.unitize();

	c.rotation.setColumn(0, x);
	c.rotation.setColumn(1, y);
    c.rotation.setColumn(2, z);

	setGrip(c);
}

void Tool::setGripUp(const Vector3& v)
{
	CoordinateFrame c(grip);

	Vector3 oldX = c.rightVector();
	//Vector3 oldZ = -c.lookVector();

	Vector3 y = Math::safeDirection(v);

	Vector3 z = oldX.cross(y);
	z.unitize();

	Vector3 x = y.cross(z);
	x.unitize();

	c.rotation.setColumn(0, x);
	c.rotation.setColumn(1, y);
    c.rotation.setColumn(2, z);

	setGrip(c);
}

void Tool::setGripRight(const Vector3& v)
{
	CoordinateFrame c(grip);

	Vector3 oldY = c.upVector();
	//Vector3 oldZ = -c.lookVector();

	Vector3 x = Math::safeDirection(v);

	Vector3 z = x.cross(oldY);
	z.unitize();

	Vector3 y = z.cross(x);
	y.unitize();

	c.rotation.setColumn(0, x);
	c.rotation.setColumn(1, y);
    c.rotation.setColumn(2, z);

	setGrip(c);
}

void Tool::setToolTip(std::string value)
{
	if(value.size() > ContentFilter::MAX_CONTENT_FILTER_SIZE)																										
		value = value.substr(0, ContentFilter::MAX_CONTENT_FILTER_SIZE);																									
																																									
	if(!ProfanityFilter::ContainsProfanity(value) || getRobloxLocked())
	{																									
		if(toolTip != value)
		{																																
			toolTip = value;																														
			raisePropertyChanged(prop_ToolTip);																														
		}																																									
	}
}

void Tool::setRequiresHandle(bool value)
{
	if (requiresHandle != value)
	{
		requiresHandle = value;
		raisePropertyChanged(prop_requiresHandle);
	}
}

Attachment *Tool::findFirstAttachmentByName(const Instance *part, const std::string& findName)
{
	if (!part || !part->getChildren())
		return NULL;

	const Instances& children = *part->getChildren();

	for (size_t i = 0; i < children.size(); ++i) 
	{
		Attachment *attach = Instance::fastDynamicCast<Attachment>(children[i].get());
		if (attach == NULL)
			continue;

		if (attach->getName() == findName) {
			return attach;
		}
	}

	return NULL;
}


Attachment *Tool::findFirstAttachmentByNameRecursive(const Instance *part, const std::string& findName)
{
	if (!part)
		return NULL;

	// breadth-first search
	Attachment *child = findFirstAttachmentByName(part, findName);
	if (child != NULL)
		return child;

	if (!part->getChildren())
		return NULL;
	const Instances& children = *part->getChildren();

	for (size_t i = 0; i < children.size(); ++i) {
		child = findFirstAttachmentByNameRecursive(children[i].get(), findName);
		if (child != NULL)
			return child;
	}

	return NULL;
}


} // namespace
