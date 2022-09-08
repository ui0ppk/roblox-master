/* Copyright 2003-2005 ROBLOX Corporation, All Rights Reserved */
#include "stdafx.h"

#include "V8DataModel/PartInstance.h"
#include "V8DataModel/JointInstance.h"
#include "V8DataModel/BasicPartInstance.h"
#include "V8DataModel/TouchTransmitter.h"
#include "V8DataModel/Workspace.h"
#include "V8World/Primitive.h"
#include "V8World/Contact.h"
#include "V8World/Clump.h"
#include "V8World/Assembly.h"
#include "V8World/Mechanism.h"
#include "V8World/HumanoidStage.h"
#include "V8World/RigidJoint.h"
#include "V8World/MotorJoint.h"
#include "V8World/World.h"
#include "v8world/SpatialFilter.h"
#include "V8World/Tolerance.h"
#include "V8Kernel/Body.h"
#include "V8Kernel/Connector.h"
#include "V8Kernel/Kernel.h"
#include "Tool/Dragger.h"
#include "Tool/ToolsArrow.h"
#include "GfxBase/Adorn.h"
#include "AppDraw/Draw.h"
#include "AppDraw/DrawAdorn.h"
#include "AppDraw/HitTest.h"
#include "Util/Math.h"
#include "Util/Color.h"
#include "util/RobloxGoogleAnalytics.h"
#include "Network/NetworkOwner.h"
#include "G3D/CollisionDetection.h"
#include "v8datamodel/PartCookie.h"

#include "humanoid/Humanoid.h"

#include "GfxBase/GfxPart.h"

#include "Network/Players.h"
#include "Network/api.h"
#include "v8world/ContactManager.h"
#include "Network/NetworkOwner.h"
#include "script/ScriptContext.h"
#include "script/LuaInstanceBridge.h"

DYNAMIC_FASTFLAG(HumanoidFloorPVUpdateSignal)
DYNAMIC_FASTFLAGVARIABLE(SetUpdateTimeOnClumpChanged, false)
DYNAMIC_FASTFLAGVARIABLE(SetNetworkOwnerFixAnchoring, false)
DYNAMIC_FASTFLAGVARIABLE(SetNetworkOwnerFixAnchoring2, false)
DYNAMIC_FASTFLAGVARIABLE(NetworkOwnershipRuleReplicates, false)
DYNAMIC_FASTFLAGVARIABLE(LocalScriptSpawnPartAlwaysSetOwner, false)

// Physical Material Properties
DYNAMIC_FASTFLAGVARIABLE(MaterialPropertiesEnabled, false)
SYNCHRONIZED_FASTFLAGVARIABLE(MaterialPropertiesNewIsDefault, false)
SYNCHRONIZED_FASTFLAGVARIABLE(NewPhysicalPropertiesForcedOnAll, false)
FASTFLAGVARIABLE        (AnchoredSendPositionUpdate, false)

LOGGROUP(PartInstanceLifetime)

DYNAMIC_FASTFLAG(HumanoidCookieRecursive)

//Deprecate FormFactor
DYNAMIC_FASTFLAGVARIABLE(FormFactorDeprecated,false)
DYNAMIC_FASTFLAGVARIABLE(FixShapeChangeBug, false)

DYNAMIC_FASTFLAGVARIABLE(FixFallenPartsNotDeleted, false)
DYNAMIC_FASTFLAG(CleanUpInterpolationTimestamps)

namespace
{
	bool computeNetworkOwnerIsSomeoneElseImpl(const RBX::SystemAddress& address, const RBX::SystemAddress& localAddress)
	{
		bool networkOwnerAssigned = ((address != RBX::Network::NetworkOwner::Unassigned()) && (address != RBX::Network::NetworkOwner::ServerUnassigned()));
		bool ownerIsSomeoneElse = (address != localAddress);

		return (networkOwnerAssigned && ownerIsSomeoneElse);
	}

	bool currentSecurityIdentityIsScript()
	{
		RBX::Security::Context securityContext = RBX::Security::Context::current();

		if (securityContext.identity == RBX::Security::LocalGUI_ ||
			securityContext.identity == RBX::Security::GameScript_ ||
			securityContext.identity == RBX::Security::GameScriptInRobloxPlace_ ||
			securityContext.identity == RBX::Security::RobloxGameScript_ ||
#if defined(RBX_STUDIO_BUILD)
            securityContext.identity == RBX::Security::StudioPlugin ||
#endif
			securityContext.identity == RBX::Security::CmdLine_)
		{
			return true;
		} else {
			return false;
		}
	}
}

namespace RBX {

bool PartInstance::showContactPoints = false;
bool PartInstance::showJointCoordinates = false;
bool PartInstance::highlightSleepParts = false;
bool PartInstance::highlightAwakeParts = false;
bool PartInstance::showBodyTypes = false;
bool PartInstance::showAnchoredParts = false;
bool PartInstance::showPartCoordinateFrames = false;
bool PartInstance::showUnalignedParts = false;
bool PartInstance::showSpanningTree = false;
bool PartInstance::showMechanisms = false;
bool PartInstance::showAssemblies = false;
bool PartInstance::showReceiveAge = false;
bool PartInstance::disableInterpolation = false;
bool PartInstance::showInterpolationPath = false;

const char* const  sPart = "BasePart";

// Declare some dictionary entries
static const RBX::Name& namePart = RBX::Name::declare(sPart);
static const RBX::Name& nameCFrame = RBX::Name::declare("CFrame");
static const RBX::Name& namePosition = RBX::Name::declare("Position");

using namespace Reflection;

///////////////////////////
// New stuff - coordinate frame, Ui Position, Velocity
// Old files used PVInstance::"CoordinateFrame", "Velocity" and "RotVel", but
// no files really had any legacy velocity data - the Velocity field here will pick up the Velocity of old PVInstances for parts

REFLECTION_BEGIN();
const PropDescriptor<PartInstance, CoordinateFrame> PartInstance::prop_CFrame("CFrame", category_Data, &PartInstance::getCoordinateFrame, &PartInstance::setCoordinateFrameRoot);
static const PropDescriptor<PartInstance, Vector3> prop_PositionUi("Position", category_Data, &PartInstance::getTranslationUi, &PartInstance::setTranslationUi, PropertyDescriptor::UI);
static const PropDescriptor<PartInstance, Vector3> prop_RotationUi("Rotation", category_Data, &PartInstance::getRotationUi, &PartInstance::setRotationUi, PropertyDescriptor::UI);
const PropDescriptor<PartInstance, Vector3> PartInstance::prop_Velocity("Velocity", category_Data, &PartInstance::getLinearVelocity, &PartInstance::setLinearVelocity);
const PropDescriptor<PartInstance, Vector3> PartInstance::prop_RotVelocity("RotVelocity", category_Data, &PartInstance::getRotationalVelocity, &PartInstance::setRotationalVelocityRoot);
static const PropDescriptor<PartInstance, float> prop_Density("SpecificGravity", category_Data, &PartInstance::getSpecificGravity, NULL, Reflection::PropertyDescriptor::UI);
REFLECTION_END();

//////////////////////////

const char* category_Part = "Part ";

namespace Reflection {
template<>
EnumDesc<NetworkOwnership>::EnumDesc()
	:EnumDescriptor("NetworkOwnership")
{
	addPair(NetworkOwnership_Auto, "Automatic");
	addPair(NetworkOwnership_Manual, "Manual");
}

template<>
EnumDesc<PartInstance::FormFactor>::EnumDesc()
	:EnumDescriptor("FormFactor")  
{
	addPair(PartInstance::SYMETRIC, "Symmetric");
	addPair(PartInstance::BRICK, "Brick");
	addPair(PartInstance::PLATE, "Plate");
	addPair(PartInstance::CUSTOM, "Custom");
	addLegacyName("Block", PartInstance::BRICK);
}

template<>
EnumDesc<PartMaterial>::EnumDesc()
	:EnumDescriptor("Material")
{
	addPair(PLASTIC_MATERIAL, "Plastic");
	addPair(WOOD_MATERIAL, "Wood");
	addPair(SLATE_MATERIAL, "Slate");
	addPair(CONCRETE_MATERIAL, "Concrete");
	addPair(RUST_MATERIAL, "CorrodedMetal");
	addPair(DIAMONDPLATE_MATERIAL, "DiamondPlate");
	addPair(ALUMINUM_MATERIAL, "Foil");
	addPair(GRASS_MATERIAL, "Grass");
	addPair(ICE_MATERIAL, "Ice");
	addPair(MARBLE_MATERIAL, "Marble");
	addPair(GRANITE_MATERIAL, "Granite");
	addPair(BRICK_MATERIAL, "Brick");
	addPair(PEBBLE_MATERIAL, "Pebble");
	addPair(SAND_MATERIAL, "Sand");
	addPair(FABRIC_MATERIAL, "Fabric");
	addPair(SMOOTH_PLASTIC_MATERIAL, "SmoothPlastic");
    addPair(METAL_MATERIAL, "Metal");
    addPair(WOODPLANKS_MATERIAL, "WoodPlanks");
    addPair(COBBLESTONE_MATERIAL, "Cobblestone");
	addPair(AIR_MATERIAL, "Air");
	addPair(WATER_MATERIAL, "Water");
	addPair(ROCK_MATERIAL, "Rock");

	addPair(GLACIER_MATERIAL, "Glacier");
	addPair(SNOW_MATERIAL, "Snow");
	addPair(SANDSTONE_MATERIAL, "Sandstone");
	addPair(MUD_MATERIAL, "Mud");
	addPair(BASALT_MATERIAL, "Basalt");
	addPair(GROUND_MATERIAL, "Ground");
	addPair(CRACKED_LAVA_MATERIAL, "CrackedLava");
    addPair(NEON_MATERIAL, "Neon");

	addLegacyName("Corroded Metal", RUST_MATERIAL);
	addLegacyName("Aluminum", ALUMINUM_MATERIAL);
}

template<>
PartMaterial& Variant::convert<PartMaterial>(void)
{
	return genericConvert<PartMaterial>();
}

}//namespace Reflection

template<>
bool StringConverter<PartMaterial>::convertToValue(const std::string& text, PartMaterial& value)
{
	return Reflection::EnumDesc<PartMaterial>::singleton().convertToValue(text.c_str(),value);
}

static BoundFuncDesc<PartInstance, void()> desc_unjoin(&PartInstance::destroyJoints, "BreakJoints", Security::None);
static BoundFuncDesc<PartInstance, void()> dep_breakJoints(&PartInstance::destroyJoints, "breakJoints", Security::None, Reflection::Descriptor::Attributes::deprecated(desc_unjoin));
static BoundFuncDesc<PartInstance, void()> desc_join(&PartInstance::join, "MakeJoints", Security::None);
static BoundFuncDesc<PartInstance, void()> desc_joinOld(&PartInstance::join, "makeJoints", Security::None);
static CustomBoundFuncDesc<PartInstance, float()> desc_getMass(&PartInstance::getMassScript, "GetMass", Security::None);
static CustomBoundFuncDesc<PartInstance, float()> desc_getMassOld(&PartInstance::getMassScript, "getMass", Security::None);
static BoundFuncDesc<PartInstance, bool()> desc_isGrounded(&PartInstance::isGrounded, "IsGrounded", Security::None);
static BoundFuncDesc<PartInstance, shared_ptr<const Instances>(bool)> desc_getConnectedParts(&PartInstance::getConnectedParts, "GetConnectedParts", "recursive", false, Security::None);
static BoundFuncDesc<PartInstance, shared_ptr<Instance>()> desc_getRootPart(&PartInstance::getRootPart, "GetRootPart", Security::None);
static BoundFuncDesc<PartInstance, CoordinateFrame()> desc_getRenderingCFrame(&PartInstance::calcRenderingCoordinateFrame, "GetRenderCFrame", Security::None);

// Network Ownership API
static BoundFuncDesc<PartInstance, void(shared_ptr<Instance>)> desc_setNetOwner(&PartInstance::setNetworkOwnerScript, "SetNetworkOwner", "playerInstance", shared_ptr<Instance>(), Security::None);   
static BoundFuncDesc<PartInstance, shared_ptr<Instance>()> desc_getNetOwner(&PartInstance::getNetworkOwnerScript, "GetNetworkOwner", Security::None);
static BoundFuncDesc<PartInstance, void()> desc_setNetOwnershipAuto(&PartInstance::setNetworkOwnershipAuto, "SetNetworkOwnershipAuto", Security::None);  
static BoundFuncDesc<PartInstance, bool()> desc_getNetOwnershipIsAuto(&PartInstance::getNetworkOwnershipAuto, "GetNetworkOwnershipAuto", Security::None);  
static BoundFuncDesc<PartInstance, shared_ptr<const Reflection::Tuple>()> desc_canSetNetworkOwnershipScript(&PartInstance::canSetNetworkOwnershipScript, "CanSetNetworkOwnership", Security::None);
    
// shape, size, form factor - both UI and streaming
const PropDescriptor<PartInstance, RBX::Vector3> prop_Siz("siz", category_Part, NULL, &PartInstance::setPartSizeXml, PropertyDescriptor::LEGACY); // Used to prepare for TA14264
const PropDescriptor<PartInstance, RBX::Vector3> PartInstance::prop_Size("size", category_Part, &PartInstance::getPartSizeXml, &PartInstance::setPartSizeXml, PropertyDescriptor::STREAMING);
static const PropDescriptor<PartInstance, RBX::Vector3> prop_SizeUi("Size", category_Part, &PartInstance::getPartSizeUi, &PartInstance::setPartSizeUi, PropertyDescriptor::UI);

const PropDescriptor<PartInstance, float> PartInstance::prop_Elasticity("Elasticity", category_Part, &PartInstance::getElasticity, &PartInstance::setElasticity);
const PropDescriptor<PartInstance, float> PartInstance::prop_Friction("Friction", category_Part, &PartInstance::getFriction, &PartInstance::setFriction);
const PropDescriptor<PartInstance, PhysicalProperties> PartInstance::prop_CustomPhysicalProperties("CustomPhysicalProperties", category_Part, &PartInstance::getPhysicalProperties, &PartInstance::setPhysicalProperties);

// color, transparency, reflectance, anchored, canCollide, locked
const PropDescriptor<PartInstance, Color3> PartInstance::prop_Color("Color", category_Appearance, &PartInstance::getColor3, &PartInstance::setColor3, PropertyDescriptor::Functionality(PropertyDescriptor::UI));
const PropDescriptor<PartInstance, BrickColor> PartInstance::prop_BrickColor("BrickColor", category_Appearance, &PartInstance::getColor, &PartInstance::setColor);
const PropDescriptor<PartInstance, BrickColor> prop_BrickColorDep("brickColor", category_Appearance, &PartInstance::getColor, &PartInstance::setColor, PropertyDescriptor::Attributes::deprecated(PartInstance::prop_BrickColor));
const EnumPropDescriptor<PartInstance, PartMaterial> PartInstance::prop_renderMaterial("Material", category_Appearance, &PartInstance::getRenderMaterial, &PartInstance::setRenderMaterial);
const PropDescriptor<PartInstance, float> PartInstance::prop_Transparency("Transparency", category_Appearance, &PartInstance::getTransparencyXml, &PartInstance::setTransparency);
// DFFlagNetworkOwnershipRuleReplicates
// Temporary Hack that allows use to Replicate Network Ownerhip
static const PropDescriptor<PartInstance, bool> prop_networkOwnershipRuleBool("NetworkOwnershipRuleBool", category_Behavior, &PartInstance::getNetworkOwnershipManualReplicate, &PartInstance::setNetworkOwnershipManualReplicate, PropertyDescriptor::REPLICATE_ONLY);
// DFFlagNetworkOwnershipRuleReplicates -> Commented Out for now because cannot introduce new Enum safely
//static const EnumPropDescriptor<PartInstance, NetworkOwnership> prop_networkOwnershipRule("NetworkOwnershipRule", category_Behavior, &PartInstance::getNetworkOwnershipRule, &PartInstance::setNetworkOwnershipRule, PropertyDescriptor::REPLICATE_ONLY);
//Client side only LocalTransparencyModifier property
const PropDescriptor<PartInstance, float> PartInstance::prop_LocalTransparencyModifier("LocalTransparencyModifier",category_Data, &PartInstance::getLocalTransparencyModifier, &PartInstance::setLocalTransparencyModifier, PropertyDescriptor::HIDDEN_SCRIPTING);
const PropDescriptor<PartInstance, float> PartInstance::prop_Reflectance("Reflectance", category_Appearance, &PartInstance::getReflectance, &PartInstance::setReflectance);
const PropDescriptor<PartInstance, bool> PartInstance::prop_Locked("Locked", category_Behavior, &PartInstance::getPartLocked, &PartInstance::setPartLocked);
const PropDescriptor<PartInstance, bool> PartInstance::prop_Anchored("Anchored", category_Behavior, &PartInstance::getAnchored, &PartInstance::setAnchored);
const PropDescriptor<PartInstance, bool> PartInstance::prop_CanCollide("CanCollide", category_Behavior, &PartInstance::getCanCollide, &PartInstance::setCanCollide);

const PropDescriptor<PartInstance, Faces> prop_ResizeableFaces("ResizeableFaces", category_Behavior, &PartInstance::getResizeHandleMask, NULL, PropertyDescriptor::UI);
const PropDescriptor<PartInstance, int>  prop_ResizeIncrement("ResizeIncrement", category_Behavior, &PartInstance::getResizeIncrement, NULL, PropertyDescriptor::UI);
const BoundFuncDesc<PartInstance, bool(NormalId, int)> desc_resize(&PartInstance::resize, "Resize", "normalId", "deltaAmount", Security::None);
const BoundFuncDesc<PartInstance, bool(NormalId, int)> desc_dep_resize(&PartInstance::resize, "resize", "normalId", "deltaAmount", Security::None, Descriptor::Attributes::deprecated(desc_resize));

const BoundFuncDesc<PartInstance, shared_ptr<const Instances>()> func_getTouchingParts(&PartInstance::getTouchingParts, "GetTouchingParts", Security::None);


// Used for network communication - streamed, not in the UI
// Nuked "v1" because the "" empty string is no longer legal
const PropDescriptor<PartInstance, RBX::SystemAddress> PartInstance::prop_NetworkOwner("NetworkOwnerV3", category_Data, &PartInstance::getNetworkOwner, &PartInstance::setNetworkOwner, PropertyDescriptor::REPLICATE_ONLY);
static RemoteEventDesc<PartInstance,
    void(RBX::SystemAddress),
    rbx::remote_signal<void(RBX::SystemAddress)>,
    rbx::remote_signal<void(RBX::SystemAddress)>* (PartInstance::*)(bool)
    > event_NetworkOwnerChanged(&PartInstance::getOrCreateNetworkOwnerChangedSignal, "NetworkOwnerChanged", "systemAddress", Security::LocalUser, RemoteEventCommon::REPLICATE_ONLY, RemoteEventCommon::CLIENT_SERVER);
const PropDescriptor<PartInstance, bool> PartInstance::prop_NetworkIsSleeping("NetworkIsSleeping", category_Data, &PartInstance::getNetworkIsSleeping, &PartInstance::setNetworkIsSleeping, PropertyDescriptor::REPLICATE_ONLY);
#ifdef RBX_TEST_BUILD
EventDesc<PartInstance, void(float, float)> event_OwnershipChange(&PartInstance::ownershipChangeSignal, "OwnershipChange", "binaryAddress", "port", Security::None);
#endif

// New way of reporting touches
EventDesc<PartInstance, 
	void(shared_ptr<Instance>), 
	rbx::signal<void(shared_ptr<Instance>)>,
	rbx::signal<void(shared_ptr<Instance>)>* (PartInstance::*)(bool)> event_LocalSimulationTouched(&PartInstance::getOrCreateLocalSimulationTouchedSignal, "LocalSimulationTouched", "part");

EventDesc<PartInstance, 
	void(shared_ptr<Instance>), 
	PartInstance::TouchedSignal,
	PartInstance::TouchedSignal* (PartInstance::*)(bool)> event_Touched(&PartInstance::getOrCreateTouchedSignal, "Touched", "otherPart");

EventDesc<PartInstance, 
	void(shared_ptr<Instance>), 
	PartInstance::TouchedSignal,
	PartInstance::TouchedSignal* (PartInstance::*)(bool)> dep_Touched(&PartInstance::getOrCreateTouchedSignal, "touched", "otherPart", Reflection::Descriptor::Attributes::deprecated(event_Touched));

EventDesc<PartInstance, 
	void(shared_ptr<Instance>), 
	PartInstance::TouchedSignal,
	PartInstance::TouchedSignal* (PartInstance::*)(bool)> event_TouchEnded(&PartInstance::getOrCreateTouchedEndedSignal, "TouchEnded", "otherPart");

EventDesc<PartInstance, 
	void(shared_ptr<Instance>), 
	rbx::signal<void(shared_ptr<Instance>)>,
	rbx::signal<void(shared_ptr<Instance>)>* (PartInstance::*)(bool)> dep_StoppedTouching(&PartInstance::getOrCreateDeprecatedStoppedTouchingSignal, "StoppedTouching", "otherPart", Reflection::Descriptor::Attributes::deprecated(event_TouchEnded));

EventDesc<PartInstance, 
	void(), 
	rbx::signal<void()>,
	rbx::signal<void()>* (PartInstance::*)(bool)> event_OutfitChanged(&PartInstance::getOrCreateOutfitChangedSignal, "OutfitChanged");

// Temporary property for handling concurrent drags
const PropDescriptor<PartInstance, bool> prop_Dragging("DraggingV1", category_Behavior, &PartInstance::getDragging, &PartInstance::setDragging, PropertyDescriptor::REPLICATE_ONLY);
const PropDescriptor<PartInstance, float> PartInstance::prop_ReceiveAge("ReceiveAge", category_Part, &PartInstance::getReceiveInterval, NULL, PropertyDescriptor::HIDDEN_SCRIPTING);

Extents PartInstance::computeExtentsWorld() const
{
	return getConstPartPrimitive()->getExtentsWorld();
}

void PartInstance::addTouchTransmitter()
{
	RBXASSERT(!onDemandRead() || onDemandRead()->touchTransmitter == NULL);
	shared_ptr<TouchTransmitter> tt = Creatable<Instance>::create<TouchTransmitter>();
	tt->setParent(this);

	if (tt->getParent() == this)	// sanity check
		onDemandWrite()->touchTransmitter = tt.get();
}

void PartInstance::removeTouchTransmitter()
{
	if (!onDemandRead() || onDemandRead()->touchTransmitter == NULL)
		return;
	onDemandWrite()->touchTransmitter->setParent(NULL);
	onDemandWrite()->touchTransmitter = 0;
}

void PartInstance::incrementTouchedSlotCount()
{
	RBX_CRASH_ASSERT(!onDemandRead() || onDemandRead()->touchedSlotCount >= 0);
	if (++(onDemandWrite()->touchedSlotCount) == 1) 
		// TODO: This isn't thread-safe, as it might appear
		addTouchTransmitter();
	RBX_CRASH_ASSERT(onDemandRead()->touchedSlotCount >= 0);
}

void PartInstance::decrementTouchedSlotCount()
{
	RBX_CRASH_ASSERT(!onDemandRead() || onDemandRead()->touchedSlotCount >= 0);

	if (--(onDemandWrite()->touchedSlotCount) == 0)
		// TODO: This isn't thread-safe, as it might appear
		removeTouchTransmitter();
	RBX_CRASH_ASSERT(onDemandRead()->touchedSlotCount >= 0);
}

void PartInstance::onChildRemoved(Instance* child)
{
	Super::onChildRemoved(child);
	if (onDemandRead() && child == onDemandRead()->touchTransmitter)
		onDemandWrite()->touchTransmitter = NULL;
}

PartInstance::OnDemandPartInstance::OnDemandPartInstance(PartInstance* owner) 
	: touchTransmitter(NULL)
    , isCurrentlyStreamRemovingPart(false)
{
	touchedSlotCount = 0;
	touchedSignal.part = owner;
	touchEndedSignal.part = owner;
}

static float toSpecificGravity(PartMaterial material)
{
	// http://www.simetric.co.uk/si_materials.htm
	switch (material)
	{
		case LEGACY_MATERIAL:
		case PLASTIC_MATERIAL: 
		case SMOOTH_PLASTIC_MATERIAL: 
		case NEON_MATERIAL:
			return 0.7;
		case WOOD_MATERIAL: return 0.350;				// Pine
		case WOODPLANKS_MATERIAL: return 0.350;
		case MARBLE_MATERIAL: return 2.563;
		case SLATE_MATERIAL: return 2.691;
		case CONCRETE_MATERIAL: return 2.403;
		case GRANITE_MATERIAL: return 2.691;
		case BRICK_MATERIAL: return 1.922;
		case PEBBLE_MATERIAL: return 2.403;
		case COBBLESTONE_MATERIAL: return 2.691;
		case RUST_MATERIAL: return 7.850;
		case DIAMONDPLATE_MATERIAL: return 7.850;
		case ALUMINUM_MATERIAL: return 7.700;
		case METAL_MATERIAL: return 7.850;
		case GRASS_MATERIAL: return 0.9;
		case SAND_MATERIAL: return 1.602;
		case FABRIC_MATERIAL: return 0.7;
		case ICE_MATERIAL: return 0.919;
		case AIR_MATERIAL: return 0;
		case WATER_MATERIAL: return 1;
		case ROCK_MATERIAL: return 2.691;
		case GLACIER_MATERIAL: return 0.919;
		case SNOW_MATERIAL: return 0.9;
		case SANDSTONE_MATERIAL: return 2.691;
		case MUD_MATERIAL: return 0.9;
		case BASALT_MATERIAL: return 2.691;
		case GROUND_MATERIAL: return 0.9;
		case CRACKED_LAVA_MATERIAL: return 2.691;
		default: RBXASSERT(false); return 1;
	}
}

static const char* partName("Part");

PartInstance::PartInstance(const Vector3& initialSize)
	: DescribedNonCreatable<PartInstance, PVInstance, sPart>(partName)
	, raknetTime(0)
	, primitive(new Primitive(Geometry::GEOMETRY_BLOCK))
	, gfxPart(NULL)
	, cookie(0)
	, color(BrickColor::defaultColor())
	, transparency(0.0f)
	, reflectance(0.0f)
	, localTransparencyModifier(0.0)
	, locked(false)
	, cachedNetworkOwnerIsSomeoneElse(false)
	, formFactor(BRICK)
	, legacyPartType(BLOCK_LEGACY_PART)
{
	FASTLOG2(FLog::PartInstanceLifetime, "PartInstance created: %p, primitive: %p", this, getPartPrimitive());
    
	primitive->setSurfaceType(NORM_Y, STUDS);
	primitive->setSurfaceType(NORM_Y_NEG, INLET);
	primitive->setSize(initialSize);
	primitive->setFriction(defaultFriction());
	primitive->setElasticity(defaultElasticity());
	primitive->setSpecificGravity(toSpecificGravity(getRenderMaterial()));
	primitive->setAnchoredProperty(false);
	primitive->setPreventCollide(false);
	primitive->setOwner(this);

	onGuidChanged();
}

PartInstance::~PartInstance() 
{
	FASTLOG2(FLog::PartInstanceLifetime, "PartInstance destroyed: %p, primitive: %p", this, getPartPrimitive());

	RBXASSERT(getPartPrimitive());
	RBXASSERT(getPartPrimitive()->getClump()==NULL);
	RBXASSERT(getPartPrimitive()->getWorld() == NULL);
}

OnDemandInstance* PartInstance::initOnDemand()
{
	return new OnDemandPartInstance(this);
}

// reset on part creation and on setting of PV - only server side;
void PartInstance::resetNetworkOwnerTime(double extraTime)
{
	networkOwnerTime = Time::now<Time::Fast>() + Time::Interval(extraTime);		// three seconds takeover server side
}

bool PartInstance::networkOwnerTimeUp() const
{
	return Time::now<Time::Fast>() > networkOwnerTime;
}

// Super Hack - any PartInstance named "Torso" tries to be the assembly root - somewhat consistent with the humanoid code
void PartInstance::setName(const std::string& value)
{
	Super::setName(value);

	FASTLOG1(FLog::PartInstanceLifetime, "PartInstance %p named", this);
	FASTLOGS(FLog::PartInstanceLifetime, "Name: %s", value);

	if (value == "HumanoidRootPart")
	{
		getPartPrimitive()->setSizeMultiplier(Primitive::ROOT_SIZE);
	} else if (value == "Torso") {
		getPartPrimitive()->setSizeMultiplier(Primitive::TORSO_SIZE);
	} else {
		getPartPrimitive()->setSizeMultiplier(Primitive::DEFAULT_SIZE);
	}
}


void PartInstance::onGuidChanged()
{
	RBXASSERT(!getPartPrimitive()->getWorld());		// not in workspace!
	getPartPrimitive()->setGuid(this->getGuid());
}


//	If size.x = 3.0,
// then 1/2 size = 1.5
// snapInPart = 1.5 - 1.0 = .5
CoordinateFrame PartInstance::worldSnapLocation() const
{
	Vector3 snapInPart;
	Vector3 halfSize = getConstPartPrimitive()->getSize() * 0.5f;
	snapInPart.y = -halfSize.y;									// i.e., at the bottom of the part
	snapInPart.x = halfSize.x - Math::iFloor(halfSize.x);		// i.e. Size 
	snapInPart.z = halfSize.z - Math::iFloor(halfSize.z);

	CoordinateFrame localSnapLocation(snapInPart);

	return getCoordinateFrame() * localSnapLocation;
}


bool PartInstance::aligned(bool useGlobalGridSetting) const
{
	CoordinateFrame myCoord = worldSnapLocation();
	
	CoordinateFrame myCoordSnapped;
	
	if (!useGlobalGridSetting || AdvArrowToolBase::advGridMode == DRAG::ONE_STUD)
		myCoordSnapped = Math::snapToGrid(myCoord, Dragger::dragSnap());
	else
	{
		if (AdvArrowToolBase::advGridMode == DRAG::QUARTER_STUD)  // Note "QUARTER_STUD" == 0.2 Stud
			myCoordSnapped = Math::snapToGrid(myCoord, Vector3(0.2f, 0.1f, 0.2f)); // Vertical snapping relies on 0.1 increments
		else
			return false;
	}



	bool isAligned = Math::fuzzyEq(myCoord, myCoordSnapped);
#ifdef _DEBUG
	if (isAligned) {
		Vector3 extentPoint = getConstPartPrimitive()->getSize();
		Vector3 pointInMe = myCoord.pointToWorldSpace(extentPoint);
		Vector3 pointSnapped = myCoordSnapped.pointToWorldSpace(extentPoint);
		float delta = (pointInMe - pointSnapped).magnitude();
		RBXASSERT(delta < (Tolerance::maxOverlapOrGap() * 0.5));
	}
#endif
	return isAligned;
}



bool PartInstance::lockedInPlace() const		// true if you could not normally "pull" this part out
{
	bool weldTop = false;
	bool weldBottom = false;
	const Joint* j = getConstPartPrimitive()->getConstFirstJoint();
	while (j) {
		if (RigidJoint::isRigidJoint(j) || Joint::isMotorJoint(j))
		{
			NormalId norm = j->getNormalId(j->getPrimitiveId(getConstPartPrimitive()));
			if (norm == NORM_Y) {
				weldTop = true;
			}
			if (norm == NORM_Y_NEG) {
				weldBottom = true;
			}
		}
		j = getConstPartPrimitive()->getConstNextJoint(j);
	}
	return (weldTop && weldBottom);
}

// Determines whether the local simulation engine should report touches
bool PartInstance::reportTouches() const
{
    bool hasTouchedSignalConnections =
        onDemandRead() && (!onDemandRead()->touchedSignal.empty() || !onDemandRead()->localSimulationTouchedSignal.empty());
    
	return (
                hasTouchedSignalConnections
			||  hasTouchTransmitter()
			||	this->findConstFirstChildOfType<TouchTransmitter>()
			);
}


void PartInstance::primitivesToParts(const G3D::Array<Primitive*>& primitives,
									 std::vector<shared_ptr<PartInstance> >& parts)
{
	for (unsigned int i = 0; i < static_cast<unsigned int>(primitives.size()); ++i) {
		PartInstance* part = PartInstance::fromPrimitive(primitives[i]);
		parts.push_back(shared_from(part));
	}
}

void PartInstance::findParts(Instance* instance, std::vector<weak_ptr<PartInstance> >& parts)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance)) {
		parts.push_back(shared_from(part));
	}

	for (size_t i = 0; i < instance->numChildren(); i++) {
		findParts(instance->getChild(i), parts);
	}
}

bool PartInstance::nonNullInWorkspace(const shared_ptr<PartInstance>& part)
{
	bool answer = (part.get() && part->getPartPrimitive()->getWorld());
	RBXASSERT(!answer || part->getPartPrimitive()->inPipeline());
	return answer;
}

bool PartInstance::partIsLegacyCustomPhysProperties(const PartInstance* part)
{
	return (!part->getConstPartPrimitive()->getConstGeometry()->isTerrain() &&
			!part->getPhysicalProperties().getCustomEnabled() &&
			(!Math::fuzzyEq(part->getFriction(), PartInstance::defaultFriction(), Math::epsilonf()) ||
			 !Math::fuzzyEq(part->getElasticity(), PartInstance::defaultElasticity(), Math::epsilonf())));
}


void convertPartsToNewGANotify(DataModel* dataModel)
{
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "PartInstance_ConvertToNewPhysicalProp", 	
				boost::lexical_cast<std::string>(dataModel->getPlaceID()).c_str(), 0, false);
}

void PartInstance::convertToNewPhysicalPropRecursive(RBX::Instance* instance)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance)) 
	{
		if (partIsLegacyCustomPhysProperties(part))
		{
			if (DataModel* dm = DataModel::get(part))
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&convertPartsToNewGANotify, dm));
			}

			PhysicalProperties defaultProperty = MaterialProperties::getPrimitivePhysicalProperties(part->getPartPrimitive());
			PhysicalProperties physicalProperty = PhysicalProperties(defaultProperty.getDensity(), part->getFriction(), part->getElasticity());
			part->setPhysicalProperties(physicalProperty);
		}
	}

	for (size_t i = 0; i < instance->numChildren(); i++) {
		convertToNewPhysicalPropRecursive(instance->getChild(i));
	}
}

bool PartInstance::instanceOrChildrenContainsNewCustomPhysics(RBX::Instance* instance)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance))
	{
		if (!part->getPartPrimitive()->getGeometry()->isTerrain() &&
			part->getPhysicalProperties().getCustomEnabled())
		{
			return true;
		}
	}

	for (size_t i = 0; i < instance->numChildren(); i++)
	{
		if(instanceOrChildrenContainsNewCustomPhysics(instance->getChild(i)))
			return true;
	}

	return false;
}

Vector3 PartInstance::xmlToUiSize(const Vector3& xmlSize) const
{
	Vector3 uiSize = xmlSize;

	if (!DFFlag::FormFactorDeprecated) {
		switch (getFormFactor())
		{
		case BRICK:
			uiSize.y = xmlSize.y / brickHeight();
			break;
		case PLATE:
			uiSize.y = xmlSize.y / plateHeight();	//   /= 0.4;
			break;
		default:
			break;
		}
	}
	return uiSize;
}

Vector3 PartInstance::uiToXmlSize(const Vector3& uiSize) const
{
	// >= minimium size (default 1.0) (no longer require int on all sides)
	Vector3 gridUiSize, newRbxSize;

	if (!DFFlag::FormFactorDeprecated) {
		switch (getFormFactor())
		{
		case BRICK:
			gridUiSize = Math::iRoundVector3(uiSize.max(getMinimumUiSize()));
			newRbxSize = gridUiSize;
			newRbxSize.y = gridUiSize.y * brickHeight();
			break;
		case PLATE:
			gridUiSize = Math::iRoundVector3(uiSize.max(getMinimumUiSize()));
			newRbxSize = gridUiSize;
			newRbxSize.y = gridUiSize.y * plateHeight();
			break;
		case SYMETRIC:
			newRbxSize = Math::iRoundVector3(uiSize.max(getMinimumUiSize()));
			break;
		case CUSTOM:
		default:
			newRbxSize = uiSize.max(getMinimumUiSizeCustom());
			break;
		}
	} else {
		newRbxSize = uiSize.max(getMinimumUiSizeCustom());
	}


	return newRbxSize;
}


void PartInstance::onCameraNear(float distance)
{
	if (distance < cameraTransparentDistance()) {
		setLocalTransparencyModifier(1.0);
	}
	else if (distance < cameraTranslucentDistance()) {
		setLocalTransparencyModifier(0.5);
	}
	else {
		setLocalTransparencyModifier(0.0);
	}
}

void PartInstance::onClumpChanged()
{
	if (getGfxPart())
		getGfxPart()->onClumpChanged(this);

    if (onDemandRead())
    	onDemandWrite()->clumpChangedSignal(shared_from(this));

	if (DFFlag::SetUpdateTimeOnClumpChanged)
		setLastUpdateTime(Time::nowFast());

	if (renderingCoordinateFrame)
		renderingCoordinateFrame->clearHistory();
}


void PartInstance::onSleepingChanged(bool sleeping)
{
	// Prevent this from being destroyed because of a side-effect of sleepingChangedSignal
	// TODO: This may be removable if we have user slots, like scripts, run asynchronously.
	shared_ptr<Instance> keep(shared_from(this));

	RBXASSERT(sleeping == getSleeping());
	if(onDemandRead())
		onDemandWrite()->sleepingChangedSignal(sleeping);

	if(getGfxPart())
		getGfxPart()->onSleepingChanged(sleeping, this);

	shouldRenderSetDirty();
}

void PartInstance::onBuoyancyChanged( bool value )
{
    if (onDemandRead())
    	onDemandWrite()->buoyancyChangedSignal(value);
}
    
bool PartInstance::isInContinousMotion()
{
	if (Mechanism* m = getPartPrimitive()->getMechanism()) 
	{
		// Use root of mechanism to determine motion
		Primitive* mechRoot = m->getMechanismPrimitive();
		PartInstance* mechRootPart = PartInstance::fromPrimitive(mechRoot);

		return mechRootPart->renderingCoordinateFrame && mechRootPart->renderingCoordinateFrame->isBeingMovedByInterpolator();
	}

	return false;
}

bool PartInstance::askSetParent(const Instance* instance) const
{
	return Instance::fastDynamicCast<ModelInstance>(instance)!=NULL;
}


PartInstance* PartInstance::fromPrimitive(Primitive* p)
{
	return const_cast<PartInstance*>(fromConstPrimitive(p));
}


const PartInstance* PartInstance::fromConstPrimitive(const Primitive* p)
{
	return p 
		? rbx_static_cast<PartInstance*>(p->getOwner())
		: NULL;
}


Clump* PartInstance::getClump()
{
	Primitive* p = this->getPartPrimitive();
	RBXASSERT(p);
	return p ? p->getClump() : NULL;
}

const Clump* PartInstance::getConstClump() const
{
	const Primitive* p = this->getConstPartPrimitive();
	RBXASSERT(p);
	return p ? p->getConstClump() : NULL;
}


PartInstance* PartInstance::fromAssembly(Assembly* a)
{
	RBXASSERT(a);
	Primitive* p = a->getAssemblyPrimitive();
	return PartInstance::fromPrimitive(p);
}

const PartInstance* PartInstance::fromConstAssembly(const Assembly* a)
{
	RBXASSERT(a);
	const Primitive* p = a->getConstAssemblyPrimitive();
	return PartInstance::fromConstPrimitive(p);
}

// destroy explicit (manual) joints and implicit (auto) joints 
void PartInstance::destroyJoints()
{
	if (World* world = Workspace::getWorldIfInWorkspace(this)) {
		world->destroyAutoJoints(getPartPrimitive());
	}
}

// only destroy auto joints
void PartInstance::destroyImplicitJoints()
{
	if (World* world = Workspace::getWorldIfInWorkspace(this)) {
		world->destroyAutoJoints(getPartPrimitive(), false);
	}
}

void PartInstance::join()
{
	if (World* world = Workspace::getWorldIfInWorkspace(this)) {
		world->createAutoJoints(getPartPrimitive());
	}
}

bool PartInstance::computeSurfacesNeedAdorn() const
{
	for (int i = 0; i < 6; ++i) {
		SurfaceType type = getSurfaceType(NormalId(i));
		if (type == ROTATE 
			|| type == ROTATE_P
			|| type == ROTATE_V) 
		{
			return true;
		}
	}
	return false;
}

Part PartInstance::getPart()
{
	Vector6<SurfaceType> surf6;
	for (int i = 0; i < 6; i++) {
		surf6[i] = getSurfaceType(NormalId(i));
	}

	return Part(
		getPartType(),
		getConstPartPrimitive()->getSize(),
		Color4(getColor3(), alpha()),
		surf6,
		calcRenderingCoordinateFrame());
}

void PartInstance::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider, newProvider);
}

void PartInstance::updatePrimitiveState()
{
	Workspace* workspace = Workspace::getWorkspaceIfInWorkspace(this);
	World* world = workspace ? workspace->getWorld() : NULL;
    World* oldWorld = getPartPrimitive()->getWorld();

	if (world != oldWorld)
	{
		RBXASSERT(oldWorld || (getPartPrimitive()->getClump()==NULL));

		if (oldWorld)
		{			
			Primitive* prim = getPartPrimitive();
			// signal stop touching
			for (int i = 0; i < prim->getNumContacts(); i++)
			{
				if (prim->getContact(i)->isInContact())
					reportUntouch(shared_from(fromPrimitive(prim->getContactOther(i))));
			}

			setMovingManager(NULL);
			oldWorld->removePrimitive(prim, getIsCurrentlyStreamRemovingPart());
		}

		if (world)
		{
			world->insertPrimitive(getPartPrimitive());
            
			// The part is likely to be in non-sleeping state since CFrame setup marks the part as moving
			// if the part is grounded it won't move; forcing the part to sleep eliminates useless awake-asleep
			// transitions that would happen otherwise.
			if (getPartPrimitive()->computeIsGrounded())
				forceSleep();

			setMovingManager(workspace);

			const RBX::SystemAddress localAddress = RBX::Network::Players::findLocalSimulatorAddress(this);

			// assign network ownership, this allows the part to be simulated asap
			// this assignment is only temporary until it gets updated by server network owner job
			if (getNetworkOwner() == RBX::Network::NetworkOwner::Unassigned())
			{
				if (RBX::Network::Player* player = RBX::Network::Players::findLocalPlayer(this))
				{
					CoordinateFrame temp;
					// TODO WHEN REMOVING THIS FLAG:
					// Must refactor function so that this Unassigned() logic is ran as a break-away function
					if (DFFlag::LocalScriptSpawnPartAlwaysSetOwner)
					{
						getPartPrimitive()->setNetworkOwner(localAddress);
					}
					else if (player->hasCharacterHead(temp))
					{
						// TODO: check in region?
						getPartPrimitive()->setNetworkOwner(localAddress);
					}
				}
				else
				{
					getPartPrimitive()->setNetworkOwner(RBX::Network::NetworkOwner::ServerUnassigned());
				}
			}

			cachedNetworkOwnerIsSomeoneElse = computeNetworkOwnerIsSomeoneElseImpl(getNetworkOwner(), localAddress);
		}
	}
}

// Into / out of the World (not datamodel)
void PartInstance::onAncestorChanged(const AncestorChanged& event)
{
	Super::onAncestorChanged(event);

	// If there are any physics Joints connected to this PartInstance's primitive 
	// that are not a decendent of the PartInstance, process the JointInstance as if
	// it were a child (special case for Joints since they can essentially have 2 parents,
	// and are processed as such by the physics pipeline)
	Primitive* prim = getPartPrimitive();
	if (prim->getNumJoints() > 0 && prim->findWorld() != Workspace::getWorldIfInWorkspace(this))
	{
		std::vector<Joint*> processJoints;
		for (int i = 0; i < prim->getNumJoints(); i++)
		{
			if (prim->getJoint(i)->getJointOwner())
				processJoints.push_back(prim->getJoint(i));
		}
		for (unsigned int i = 0; i < processJoints.size(); i++)
		{
			JointInstance* jointInstance = static_cast<JointInstance*>(processJoints[i]->getJointOwner());
			jointInstance->onCoParentChanged();
		}
	}

	updatePrimitiveState();
	updateHumanoidCookie();
}


bool PartInstance::shouldRender3dAdorn() const
{
	return ((getTransparencyUi() < 1.0f) &&
			(computeSurfacesNeedAdorn()
			||	PartInstance::showContactPoints
            ||	PartInstance::showJointCoordinates
			||	PartInstance::highlightSleepParts
			||	PartInstance::highlightAwakeParts
			||	PartInstance::showBodyTypes
			||	PartInstance::showPartCoordinateFrames
			||	PartInstance::showAnchoredParts
			||	PartInstance::showUnalignedParts
			||	PartInstance::showSpanningTree
			||	Workspace::showEPhysicsOwners
			||	Workspace::showEPhysicsRegions
			||	PartInstance::showMechanisms
			||	PartInstance::showAssemblies
			||	PartInstance::showReceiveAge
			||  PartInstance::showInterpolationPath));
}


void PartInstance::render3dAdorn(Adorn* adorn) 
{
	if(getTransparencyUi() >= 1.0f)
		return;
    
    if (getPartPrimitive()->getWorld() == NULL)
        return;

	if (computeSurfacesNeedAdorn()) {
		Draw::partAdorn(
			getPart(), 
			adorn, 
			Color3::gray()
		);
	}

	if (PartInstance::highlightSleepParts && getPartPrimitive()->getAssembly())
	{
		Sim::AssemblyState sleepStatus = getPartPrimitive()->getAssembly()->getAssemblyState();
		if (	(sleepStatus != Sim::ANCHORED) 
			&&	(sleepStatus != Sim::AWAKE))
		{
			Color3 sleepColor;
			switch (sleepStatus)
			{
				case Sim::RECURSIVE_WAKE_PENDING:	sleepColor = Color3::purple();	break;
				case Sim::WAKE_PENDING:				sleepColor = Color3::purple();	break;
				case Sim::SLEEPING_CHECKING:		sleepColor = Color3::orange();	break;
				case Sim::SLEEPING_DEEPLY:			sleepColor = Color3::yellow();	break;
				default: RBXASSERT(0);	break;
			}
			Draw::selectionBox(
				getPart(),
				adorn,
				sleepColor);
		}
	}

	if (PartInstance::highlightAwakeParts && getPartPrimitive()->getAssembly())
	{	
		Sim::AssemblyState sleepStatus = getPartPrimitive()->getAssembly()->getAssemblyState();
		if (	(sleepStatus != Sim::ANCHORED)
			&&	(sleepStatus != Sim::SLEEPING_DEEPLY)	) 
		{
			Color3 wakeColor;
			switch (sleepStatus)
			{
				case Sim::AWAKE:					wakeColor = Color3::red();	break;
				case Sim::SLEEPING_CHECKING:		wakeColor = Color3::orange();	break;
				case Sim::RECURSIVE_WAKE_PENDING:	wakeColor = Color3::purple();		break;
				case Sim::WAKE_PENDING:				wakeColor = Color3::purple();		break;
				default: RBXASSERT(0);	break;
			}
			Draw::selectionBox(
				getPart(),
				adorn,
				wakeColor);
		}
	}

	if (PartInstance::showBodyTypes)
	{	
		Body* body = getPartPrimitive()->getBody();
		Workspace* workspace = Workspace::getWorkspaceIfInWorkspace(this);
		World* world = workspace ? workspace->getWorld() : NULL;

		if ( body && world  )
		{
			SimBody* simBody = body->getRootSimBody();
			if (simBody->isInKernel())
			{
				if ( simBody->isFreeFallBody() )
					Draw::selectionBox(	getPart(), adorn, Color3::green());
				else if ( simBody->isRealTimeBody() )
					Draw::selectionBox(	getPart(), adorn, Color3::red());
				else if ( simBody->isJointBody() )
					Draw::selectionBox(	getPart(), adorn, Color3::blue());
				else {
					RBXASSERT( simBody->isContactBody() );
					if (simBody->isSymmetricContact())
					{
						if (simBody->isVerticalContact())
							Draw::selectionBox(	getPart(), adorn, Color3::brown());
						else
							Draw::selectionBox(	getPart(), adorn, Color3::orange());
					}
					else
						Draw::selectionBox(	getPart(), adorn, Color3::yellow());
				}
			}
		}
	}


	if (PartInstance::showPartCoordinateFrames) {
		renderCoordinateFrame(adorn);
	}

	if (PartInstance::showAnchoredParts && getAnchored()) {
		DrawAdorn::partSurface(
			getPart(),
			Math::getClosestObjectNormalId(Vector3(0, -1, 0), getCoordinateFrame().rotation),
			adorn,
			G3D::Color4(Color3::gray(), alpha()));
	}

	if (PartInstance::showUnalignedParts && !aligned()) {
		Draw::selectionBox(
			getPart(),
			adorn,
			Color3::yellow());
	}

	if (PartInstance::showSpanningTree) {
		if (getPartPrimitive()->getMechanism())
		{
			if (Mechanism::isMechanismRootPrimitive(getPartPrimitive()))
			{
				adorn->setObjectToWorldMatrix(getCoordinateFrame().translation);
				Vector3 boxSize(Vector3::one() * 0.40);
				bool primitiveAnchoredInEngine = Assembly::computeIsGroundingPrimitive(getPartPrimitive());

				adorn->box(		AABox(-boxSize, boxSize),
								(primitiveAnchoredInEngine ? Color3::red() : Color3::orange())	
							);
			}
			else if (Mechanism::getRootMovingPrimitive(getPartPrimitive())==getPartPrimitive())
			{
				adorn->setObjectToWorldMatrix(getCoordinateFrame().translation);
				Vector3 boxSize(Vector3::one() * 0.40);
				adorn->box(		AABox(-boxSize, boxSize),
					(Color3::blue())	
					);
			}
			else if (Assembly::isAssemblyRootPrimitive(getPartPrimitive()))
			{	
				adorn->setObjectToWorldMatrix(getCoordinateFrame().translation);
				adorn->sphere(	Sphere(Vector3::zero(), 0.4), Color3::yellow() * .8f );
			}
			else if (Clump::isClumpRootPrimitive(getPartPrimitive()))
			{
				adorn->setObjectToWorldMatrix(getCoordinateFrame().translation);
				adorn->cylinderAlongX(0.125f, 0.25f, Color3::orange());
			}
		}
	}
	if (PartInstance::showContactPoints) {
		  for (int i = 0; i < getPartPrimitive()->getNumContacts(); ++i) {
			  Contact* c = getPartPrimitive()->getContact(i);
			  for (int j = 0; j < c->numConnectors(); ++j) {
				  Vector3 loc, normal;
				  float length;
				  ContactConnector* connector = c->getConnector(j);
				  if (!connector->isInKernel())
					  continue;
				  Color3 color = Color3::white();
				  if (connector->isSecondPass())
					  color = Color3::red();
				  else if (connector->isRealTime())
					  color = Color3::orange();
				  else if (connector->isJoint())
					  color = Color3::blue();
				  else if (connector->isContact())
				  {
					  color = Color3::yellow();
					  if (connector->isRestingContact())
						  color *= 0.5;
				  }

				  connector->getLengthNormalPosition(loc, normal, length);
				  if (length < 0.0f) {
					  adorn->setObjectToWorldMatrix(CoordinateFrame(loc));
					  adorn->ray( RbxRay::fromOriginAndDirection(Vector3::zero(), normal * length * -4.0f), color * .8f );
					  adorn->sphere(	Sphere(Vector3::zero(), 0.5), color * .8f );
				  }
			  }
		  }
	}

    if (PartInstance::showJointCoordinates) {

        // Helpful visualization function: shows the center of mass
        adorn->setObjectToWorldMatrix(getCoordinateFrame().pointToWorldSpace(getPartPrimitive()->getGeometry()->getCofmOffset()));
        adorn->sphere(Sphere(Vector3::zero(), 0.2), Color3::orange() * .8f );

        adorn->setObjectToWorldMatrix(getCoordinateFrame());
        adorn->ray( RbxRay::fromOriginAndDirection( Vector3::zero(), Vector3::unitX() * 2.0f ), Color3::red() * .5f );
        adorn->ray( RbxRay::fromOriginAndDirection( Vector3::zero(), Vector3::unitY() * 2.0f ), Color3::green() * .5f );
        adorn->ray( RbxRay::fromOriginAndDirection( Vector3::zero(), Vector3::unitZ() * 2.0f ), Color3::blue() * .5f );

        for (int i = 0; i < getPartPrimitive()->getNumJoints(); ++i) {
            Primitive* primitive = getPartPrimitive();
            Joint* joint = primitive->getJoint(i);
            if (!Joint::isSpringJoint(joint))
                continue;
            for (int j = 0; j < 2; ++j) {
                adorn->setObjectToWorldMatrix(joint->getJointWorldCoord(j));
                adorn->ray( RbxRay::fromOriginAndDirection( Vector3::zero(), Vector3::unitX() * 2.0f ), Color3::red() * .5f );
                adorn->ray( RbxRay::fromOriginAndDirection( Vector3::zero(), Vector3::unitY() * 2.0f ), Color3::green() * .5f );
                adorn->ray( RbxRay::fromOriginAndDirection( Vector3::zero(), Vector3::unitZ() * 2.0f ), Color3::blue() * .5f );
            }
        }
    }

	if (Workspace::showEPhysicsOwners) {
		if (this->getPartPrimitive()->getAssembly()) {
			if (Primitive* movingRoot = Mechanism::getRootMovingPrimitive(this->getPartPrimitive())) {
				const RBX::SystemAddress& systemAddress = PartInstance::fromPrimitive(movingRoot)->getNetworkOwner();
				Color3 color = RBX::Network::NetworkOwner::colorFromAddress(systemAddress);
				
				Assembly* a = movingRoot->getAssembly();
                World* world = getPartPrimitive()->getWorld();
				if (!world->getSpatialFilter()->addressMatch(a))
					if (a->getFilterPhase() == Assembly::Sim_BufferZone)
						color = Color::red();

				Draw::selectionBox(
					getPart(),
					adorn,
					color
					);
			}
		}
	}

	if (PartInstance::showInterpolationPath)
	{
		if (renderingCoordinateFrame)
			renderingCoordinateFrame->renderPath(adorn);
	}

    if (PartInstance::showAssemblies) {
		if (Assembly* a = this->getPartPrimitive()->getAssembly()) {
			Draw::selectionBox(
				getPart(),
				adorn,
				RBX::Color::colorFromPointer(a));
		}
	}
    if (PartInstance::showMechanisms) {
		if (Mechanism* m = this->getPartPrimitive()->getMechanism()) {
			Draw::selectionBox(
				getPart(),
				adorn,
				RBX::Color::colorFromPointer(m));
		}
	}
}

void PartInstance::render3dSelect(Adorn* adorn, SelectState selectState) 
{
	Draw::selectionBox(
		getPart(),
		adorn,
		selectState,
		0.02f);
}





bool PartInstance::hitTestImpl(const RBX::RbxRay& worldRay, Vector3& worldHitPoint) 
{
	RBX::RbxRay rayInPartCoords = getCoordinateFrame().toObjectSpace(worldRay);	

	Vector3 hitPointInPartCoords;

	bool answer = HitTest::hitTest(getPart(), rayInPartCoords, hitPointInPartCoords, 1.0);

	if (answer)
	{
		worldHitPoint = getCoordinateFrame().pointToWorldSpace(hitPointInPartCoords);
		return true;
	}
	return false;
}



// TODO:  Change this - should occur on a surface by surface basis

void PartInstance::onSurfaceChanged(NormalId surfId)
{
	shouldRenderSetDirty();
}

////////////////////////////////////////////////////////////////////
//
// Sets and gets

// Server side - any pv change sets network owner to Server and resets the clock
void PartInstance::onPVChangedFromReflection()
{
	if (getNetworkOwnershipRule() == NetworkOwnership_Manual)
	{
		// We want the user to be able to to set CFrame of the part
		// withouth having to set Owner to Server
		return;
	}
	if (Network::NetworkOwner::isClient(getNetworkOwner())) 
	{			
		if (Workspace::serverIsPresent(this)) 
		{
			if (Network::Players::getDistributedPhysicsEnabled()) 
			{	
				// do this only if this part is not humanoid
				if(ModelInstance* m = Instance::fastDynamicCast<ModelInstance>(this->getParent())) {
					if (Network::Players::getPlayerFromCharacter(m)) 
					{
						return;
					}
				}

				resetNetworkOwnerTime(3.0);								// wait 3 seconds if PV set while already a client
				setNetworkOwnerAndNotify(Network::NetworkOwner::Server());
			}
		}
	}
}

shared_ptr<const Instances> PartInstance::getTouchingParts()
{
	RBXASSERT(getPartPrimitive());

	if (ContactManager* contactManager = Workspace::getContactManagerIfInWorkspace(this))
		return contactManager->getPartCollisions(getPartPrimitive());

	return shared_ptr<const Instances>();
}

void PartInstance::safeMove()
{
	RBXASSERT(getPartPrimitive());
	RBXASSERT_IF_VALIDATING(!getPartPrimitive()->hasAutoJoints());

	if (ContactManager* contactManager = Workspace::getContactManagerIfInWorkspace(this)) {
		G3D::Array<Primitive*> temp;
		temp.append(getPartPrimitive());
		Dragger::safeMoveNoDrop(temp, Vector3::zero(), *contactManager);
	}
}

void PartInstance::addInterpolationSample(const CoordinateFrame& value, const Velocity& velocity, const RemoteTime& timeStamp, Time now, float localTimeOffest, int numNodesAhead)
{
	createRenderingCoordinateFrame();
	renderingCoordinateFrame->setValue(this, value, velocity, timeStamp, now, localTimeOffest, numNodesAhead);
}

void PartInstance::setInterpolationDelay(float value)
{
	if (renderingCoordinateFrame)
    	renderingCoordinateFrame->setTargetDelay(value);
}

// All position changes from script go through this function
void PartInstance::setPhysics(const CoordinateFrame& value)
{
	if (value != getPartPrimitive()->getCoordinateFrame())
	{
		RBXASSERT_IF_VALIDATING(Math::isOrthonormal(value.rotation));

		getPartPrimitive()->setCoordinateFrame(value);
	}
}

void PartInstance::setPhysics(const PV& pv)
{
	getPartPrimitive()->setVelocity(pv.velocity);
	setPhysics(pv.position);
}

void PartInstance::setCoordinateFrame(const CoordinateFrame& value)
{
	if (value != this->getCoordinateFrame())
	{
		CoordinateFrame orthoValue(value);
		if (Math::hasNanOrInf(orthoValue)) {
			// TODO: ASSERT This makes bad parts disappear silently.
			//       This is the desired behavior when reading a file,
			//       but if we try to set the CF from a script the desired
			//       behaviour would be to throw an exception.
			// Put the part way down so that Workspace will delete it
			orthoValue = CoordinateFrame(Vector3(0,-FLT_MAX,0));
		}
		else {
			Math::orthonormalizeIfNecessary(orthoValue.rotation);
		}

		if (renderingCoordinateFrame)
		{
			renderingCoordinateFrame->clearHistory();
			renderingCoordinateFrame->setRenderedFrame(value);
		}

		setPhysics(orthoValue);
		onPVChangedFromReflection();
		raisePropertyChanged(prop_CFrame);
		raisePropertyChanged(prop_PositionUi);
        raisePropertyChanged(prop_RotationUi);

		if(onDemandRead())
			onDemandWrite()->cframeChangedFromReflectionSignal();
	}
}

CoordinateFrame PartInstance::computeRenderingCoordinateFrame(PartInstance* mechanismRootPart, const Time& t)
{
	RBXASSERT(this->getConstPartPrimitive()->getConstMechanism() == mechanismRootPart->getConstPartPrimitive()->getConstMechanism());
	RBXASSERT(mechanismRootPart->getConstPartPrimitive()->getConstMechanism()->getConstMechanismPrimitive() == mechanismRootPart->getConstPartPrimitive());

	if (this == mechanismRootPart) 
	{
		if (renderingCoordinateFrame)
		{
			World* world = getPartPrimitive()->getWorld();

			if (world && world->getUiStepId() > renderingCoordinateFrame->getUiStepId())
			{
				renderingCoordinateFrame->setUiStepId(world->getUiStepId());
				return renderingCoordinateFrame->computeValue(this, t);
			} 
			else
			{
				return renderingCoordinateFrame->getLastComputedValue();
			}
		}
		else
		{
			return getCoordinateFrame();
		}
	}
	else 
	{
		CoordinateFrame meInRoot = mechanismRootPart->getCoordinateFrame().toObjectSpace(this->getCoordinateFrame());

		return mechanismRootPart->computeRenderingCoordinateFrame(mechanismRootPart, t) * meInRoot;
	}
}

CoordinateFrame PartInstance::calcRenderingCoordinateFrame()
{
	if (DFFlag::CleanUpInterpolationTimestamps)
	{
		return calcRenderingCoordinateFrame(TaskScheduler::singleton().lastCyclcTimestamp);
	}
	
	return calcRenderingCoordinateFrame(Time::nowFast());
}

CoordinateFrame PartInstance::calcRenderingCoordinateFrame(const Time& t)
{
	Mechanism* m = getPartPrimitive()->getMechanism();
	bool networkOwnerIsSomeoneElse = computeNetworkOwnerIsSomeoneElse();
	if (DFFlag::HumanoidFloorPVUpdateSignal && !PartInstance::disableInterpolation && m && (!networkOwnerIsSomeoneElse)) 
	{
		PartInstance* mechRootPart = PartInstance::fromPrimitive(m->getMechanismPrimitive());
		Humanoid* humanoid = RBX::Humanoid::modelIsCharacter(mechRootPart->getParent());
		if (humanoid && humanoid->getLastFloor())
		{
			const shared_ptr<PartInstance>& floorPart = humanoid->getLastFloor();
			if (floorPart)
			{
				Vector3 localDeltaToChar = floorPart->getCoordinateFrame().pointToObjectSpace(getCoordinateFrame().translation);
				return CoordinateFrame(getCoordinateFrame().rotation) + floorPart->calcRenderingCoordinateFrameImpl(t).pointToWorldSpace(localDeltaToChar);
			}
		}
	}
	return calcRenderingCoordinateFrameImpl(t);
}

CoordinateFrame PartInstance::calcRenderingCoordinateFrameImpl(const Time& t)
{
	Mechanism* m = getPartPrimitive()->getMechanism();

	if (!PartInstance::disableInterpolation && m) 
	{
		Primitive* mechRoot = m->getMechanismPrimitive();
		PartInstance* mechRootPart = PartInstance::fromPrimitive(mechRoot);
		
		// See computeNetworkOwnerIsSomeoneElse(); we inline this here to avoid redundant computation of mech primitive
		if (mechRootPart->cachedNetworkOwnerIsSomeoneElse)
		{
			return computeRenderingCoordinateFrame(mechRootPart, t);
		}
	}

	const CoordinateFrame& cf = getCoordinateFrame();

	if (renderingCoordinateFrame)
		renderingCoordinateFrame->setRenderedFrame(cf);

	return cf;
}

bool PartInstance::computeNetworkOwnerIsSomeoneElse() const
{
	if (const Mechanism* m = getConstPartPrimitive()->getConstMechanism()) {
        // Use root of mechanism (not Assembly, as done previously) to determine network ownership
		const Primitive* mechRoot = m->getConstMechanismPrimitive();
		const PartInstance* mechRootPart = PartInstance::fromConstPrimitive(mechRoot);
		
		return mechRootPart->cachedNetworkOwnerIsSomeoneElse;
	}
	else {
		return false;
	}
}

// A bunch of heuristic rules to categorize a part as projectile
bool PartInstance::isProjectile() const
{
	if (getLinearVelocity().squaredMagnitude() < 25.0 * 25.0)		// slow objects are not projectiles
		return false;
	
	const Primitive* p = getConstPartPrimitive();

	const Primitive* clumpRoot = p->getRoot<Primitive>();
	if (clumpRoot->numChildren() > 0)				// clumps of more than one parts are not projectiles
		return false;
	RBXASSERT(p == clumpRoot);
	const Clump* c = p->getConstClump();
	const Clump* assemblyRoot = c->getRootClump();
	if (assemblyRoot->numChildren() > 0)			// Assemblies of more than one clumps are not projectiles
		return false;
	RBXASSERT(c == assemblyRoot);
	const Assembly* a = p->getConstAssembly();
	const Assembly* mechanismRoot = Mechanism::getConstMovingAssemblyRoot(a);
	if (mechanismRoot->numChildren() > 0)			// Mechanisms of more than one assemblies are not projectiles
		return false;
	RBXASSERT(a == mechanismRoot);

	return true;
}

const CoordinateFrame& PartInstance::getCoordinateFrame() const
{
	return getConstPartPrimitive()->getCoordinateFrame();
}

void PartInstance::setCoordinateFrameRoot(const CoordinateFrame& value)
{
	Primitive* prim = getPartPrimitive();
	PartInstance* part = this;
	Mechanism* m = prim->getMechanism();

	if (currentSecurityIdentityIsScript() && m != NULL)
	{
		prim = m->getMechanismPrimitive();
		part = PartInstance::fromPrimitive(prim);
	}

	part->setCoordinateFrame(value);
}

void PartInstance::setTranslationUi(const Vector3& set)
{
	CoordinateFrame current = getCoordinateFrame();
	if (current.translation != set)
    {
		current.translation = set;
		destroyJoints();
		setCoordinateFrame(current);
		safeMove();
		join();
	}
}

const Vector3& PartInstance::getTranslationUi() const
{
	return getConstPartPrimitive()->getCoordinateFrame().translation;		// getBody()->getPos();
}

/**
 * Sets the rotation of the part using euler angles (in degrees).
 */
void PartInstance::setRotationUi(const Vector3& set)
{
    CoordinateFrame current = getCoordinateFrame();
    Matrix3 rotation = Matrix3::fromEulerAnglesXYZ(Math::degreesToRadians(set.x),Math::degreesToRadians(set.y),Math::degreesToRadians(set.z));
    if ( rotation != current.rotation )
    {
        current.rotation = rotation;
		destroyJoints();
		setCoordinateFrame(current);
		safeMove();
		join();
    }
}

/**
 * Gets the rotation of the part using euler angles (in degrees).
 */
const Vector3 PartInstance::getRotationUi() const
{
    CoordinateFrame current = getCoordinateFrame();

    float x;
    float y;
    float z;
    current.rotation.toEulerAnglesXYZ(x,y,z);

    Vector3 v(
        Math::radiansToDegrees(x),
        Math::radiansToDegrees(y),
        Math::radiansToDegrees(z) );
    return v;
}

void PartInstance::setLinearVelocity(const Vector3& set)
{

	Primitive* prim = getPartPrimitive();
	PartInstance* part = this;
	Mechanism* m = prim->getMechanism();

	if (currentSecurityIdentityIsScript() && m != NULL)
	{
		prim = m->getMechanismPrimitive();
		part = PartInstance::fromPrimitive(prim);
	}

	Velocity vel = prim->getBody()->getVelocity();
	if (vel.linear != set) {
		vel.linear = set;
		prim->setVelocity(vel);
		part->onPVChangedFromReflection();
		part->raisePropertyChanged(prop_Velocity);
	}
}

float PartInstance::getMass() const
{
	return getConstPartPrimitive()->getConstBody()->getMass();
}

RBX::Vector3 PartInstance::getPrincipalMoment() const
{
	return getConstPartPrimitive()->getConstBody()->getIBodyV3();

}

int PartInstance::getMassScript(lua_State* L)
{
	if (DFFlag::MaterialPropertiesEnabled)
	{
		DataModel* dm = DataModel::get(&ScriptContext::getContext(L));
		bool newMaterialPropertiesEnabled = dm->getWorkspace()->getUsingNewPhysicalProperties();
		lua_pushnumber(L, getPartPrimitive()->getCalculateMass(newMaterialPropertiesEnabled));
		return 1;
	}
	lua_pushnumber(L, getMass());
	return 1;
}

const Velocity& PartInstance::getVelocity() const
{
	return getConstPartPrimitive()->getPV().velocity;		//getBody()->getVelocity();
}

const Vector3& PartInstance::getLinearVelocity() const
{
	return getConstPartPrimitive()->getPV().velocity.linear;			//getBody()->getVelocity().linear;
}

void PartInstance::setRotationalVelocityRoot(const Vector3& set)
{
	Primitive* prim = getPartPrimitive();
	PartInstance* part = this;
	Mechanism* m = prim->getMechanism();

	if (currentSecurityIdentityIsScript() && m != NULL)
	{
		prim = m->getMechanismPrimitive();
		part = PartInstance::fromPrimitive(prim);
	}

	part->setRotationalVelocity(set);
}

void PartInstance::setRotationalVelocity(const Vector3& set)
{
	Velocity vel = getPartPrimitive()->getBody()->getVelocity();
	if (vel.rotational != set) {
		vel.rotational = set;
		getPartPrimitive()->setVelocity(vel);
		onPVChangedFromReflection();
		raisePropertyChanged(prop_RotVelocity);
	}
}

const Vector3& PartInstance::getRotationalVelocity() const
{
	return getConstPartPrimitive()->getPV().velocity.rotational;			// getBody()->getVelocity().rotational;
}


const Vector3& PartInstance::getPartSizeXml() const
{
	return getConstPartPrimitive()->getSize();
}

Vector3 PartInstance::getPartSizeUi() const
{
	return getConstPartPrimitive()->getSize();
}

void PartInstance::setPartSizeXml(const Vector3& rbxSize)
{
	if (DFFlag::FixShapeChangeBug)
	{
		// !hasThreeDimensionalSize = "requires all dimensions to be equal" (only spheres now)
		// since this requirement might cause a size change we need a special case for it
		if ((!hasThreeDimensionalSize() && rbxSize.xxx() != getPartSizeXml()))
		{
			RBXASSERT_VERY_FAST(!getPartPrimitive()->hasAutoJoints());

			getPartPrimitive()->setSize(rbxSize.xxx());

			raisePropertyChanged(prop_Size);

			refreshPartSizeUi();
		}
		else if (rbxSize != getPartSizeXml())
		{
			RBXASSERT_VERY_FAST(!getPartPrimitive()->hasAutoJoints());

			getPartPrimitive()->setSize(rbxSize);

			raisePropertyChanged(prop_Size);

			refreshPartSizeUi();
		}
	}
	else
	{
		if (rbxSize != getPartSizeXml())
		{
			RBXASSERT_VERY_FAST(!getPartPrimitive()->hasAutoJoints());

			Vector3 newRbxSize = hasThreeDimensionalSize() ? rbxSize : rbxSize.xxx();

			// current smallest size is plate height (0.4) and grid x,z (1.0)
			// TODO: ASSERT
			//RBXASSERT(newRbxSize.max(Vector3(1.0, plateHeight(), 1.0)) == newRbxSize);

			getPartPrimitive()->setSize(newRbxSize);

			raisePropertyChanged(prop_Size);

			refreshPartSizeUi();
		}
	}
}
void PartInstance::refreshPartSizeUi()
{
	raisePropertyChanged(prop_SizeUi);
}

void PartInstance::setPartSizeUnjoined(const Vector3& uiSize)
{
	RBXASSERT_VERY_FAST(!getPartPrimitive()->hasAutoJoints());
	Vector3 newRbxSize = this->uiToXmlSize(uiSize);
	setPartSizeXml(newRbxSize);
	safeMove();			// only moves if in-world
}


void PartInstance::setPartSizeUi(const Vector3& uiSize)
{
	destroyJoints();
		// Snap to nearest alignment:
		Vector3 clampedSize = uiToXmlSize(xmlToUiSize(uiSize));
		if(clampedSize != getPartSizeXml()){
			setPartSizeXml(clampedSize );
		}
		else if(clampedSize != uiSize){
			refreshPartSizeUi();
		}
		safeMove();			// only moves if in-world
	join();
}

bool PartInstance::getDragging() const
{
	return getConstPartPrimitive()->getDragging();
}

void PartInstance::setDragging(bool value)
{
	if (value != getPartPrimitive()->getDragging()) {
		getPartPrimitive()->setDragging( value );
		raiseChanged(prop_Dragging);
	}
}

bool PartInstance::getNetworkIsSleeping() const
{
	return getConstPartPrimitive()->getNetworkIsSleeping();
}

// Only set through replication, or in the filter stage
//
void PartInstance::setNetworkIsSleeping(bool value)
{
	// if a change occurs, then onNetworkIsSleepingChanged will be called, which raises a prop_NetworkIsSleeping change
	getPartPrimitive()->setNetworkIsSleeping(value, value?Time::nowFast():Time());
}
    
void PartInstance::onNetworkIsSleepingChanged(Time now)
{
    // This call ensures that the rendering data has the latest coordinate frame for interpolation purposes.
    // Without this, parts may go to sleep without updating the renderingCoordinateFrame, which will result
    // in an interpolated coordinate frame at an intermediate, inappropriate location between proper end points
	// (which often results in floating parts).
	if (getNetworkIsSleeping())
	{
		RBXASSERT(!now.isZero()); // setNetworkIsSleeping passes 0 for speed when waking up.
		if (DFFlag::CleanUpInterpolationTimestamps)
		{
			if (renderingCoordinateFrame && renderingCoordinateFrame->getLocalToRemoteTimeOffset() > 0.0)
				addInterpolationSample(getCoordinateFrame(), getVelocity(), now - Time::Interval(renderingCoordinateFrame->getLocalToRemoteTimeOffset()), now, 0.0f);
		}
		else
		{
			createRenderingCoordinateFrame();
			addInterpolationSample(getCoordinateFrame(), getVelocity(), now - Time::Interval(renderingCoordinateFrame->getLocalToRemoteTimeOffset()), now, 0.0f);
		}
	}
	raiseChanged(prop_NetworkIsSleeping);
}

const RBX::SystemAddress PartInstance::getNetworkOwner() const
{
	RBXASSERT(getConstPartPrimitive());
	return getConstPartPrimitive()->getNetworkOwner();
}

void PartInstance::setNetworkOwnerNotifyIfServer(const RBX::SystemAddress value, bool ownershipBecomingManual)
{
	if (ownershipBecomingManual)
	{
		setNetworkOwnershipRule(NetworkOwnership_Manual);
	}

	if (RBX::Network::Players::serverIsPresent(this, false))
	{
		setNetworkOwnerAndNotify(value);
	}
	else
	{
		setNetworkOwner(value);
	}
}

void PartInstance::setNetworkOwnershipRuleIfServer(NetworkOwnership value)
{
	if (Network::Players::serverIsPresent(this, false))
	{
		setNetworkOwnershipRule(value);
	}
}

void PartInstance::setNetworkOwner(const RBX::SystemAddress value)
{
    const RBX::SystemAddress currentOwner = getConstPartPrimitive()->getNetworkOwner();
	if (value != currentOwner) {
		RBXASSERT(value != Network::NetworkOwner::Unassigned());

        const RBX::SystemAddress localAddress = RBX::Network::Players::findLocalSimulatorAddress(this);

        if (value == localAddress)
        {
            // we will be new owner, no longer need interpolation
			if (renderingCoordinateFrame)
				renderingCoordinateFrame->clearHistory();
        }

		cachedNetworkOwnerIsSomeoneElse = computeNetworkOwnerIsSomeoneElseImpl(value, localAddress);

        if (Network::NetworkOwner::isClient(localAddress))
        {
            if (currentOwner == localAddress)
            {
                // if we switch from client to others, we change the recorded last cframe to the local value
                // this is needed for cross packet compression in path-based movement
                setLastCFrame(getCoordinateFrame());
                setLastUpdateTime(Time::nowFast());

				if (renderingCoordinateFrame)
					renderingCoordinateFrame->clearHistory();
            }
        }

		getPartPrimitive()->setNetworkOwner(value);

#ifdef RBX_TEST_BUILD
		ownershipChangeSignal((float)value.getAddress(), (float)value.getPort());
#endif
	}
}

void PartInstance::setNetworkOwnerAndNotify(const RBX::SystemAddress value)
{
	const RBX::SystemAddress oldOwner = getConstPartPrimitive()->getNetworkOwner();
	setNetworkOwner(value);

	notifyNetworkOwnerChanged(oldOwner);
}

void PartInstance::notifyNetworkOwnerChanged(const RBX::SystemAddress oldOwner)
{
	const RBX::SystemAddress currentOwner = getConstPartPrimitive()->getNetworkOwner();
	if (oldOwner != currentOwner)
	{
		/*
		Who to notify:
									old owner		new owner
		server->client									x		
		client->client					x				x
		client->server					x				x
		c->s (triggered by client)						x
		*/
		
		Reflection::EventArguments args(1);

		if (Network::Players::backendProcessing(this))
		{
			if (oldOwner == Network::NetworkOwner::ServerUnassigned())
			{
				// if previous owner is ServerUnassigned, notify everyone owner is now assigned
				raiseChanged(prop_NetworkOwner);
				return;
			}

			if (!Network::NetworkOwner::isServer(oldOwner))
			{
				// notify old owner the part will be owned by someone else
				args[0] = Network::NetworkOwner::AssignedOther();
				raiseEventInvocation(event_NetworkOwnerChanged, args, &oldOwner);
			}

			if (!Network::NetworkOwner::isServer(currentOwner))
			{
				// notify new owner to take control
				args[0] = currentOwner;
				raiseEventInvocation(event_NetworkOwnerChanged, args, &currentOwner);
			}
		}
		else
		{
			// client should only be changing owner to server
			RBXASSERT(currentOwner == Network::NetworkOwner::Server());

			args[0] = currentOwner;
			raiseEventInvocation(event_NetworkOwnerChanged, args, NULL);
		}

	}
}

rbx::remote_signal<void(RBX::SystemAddress)>* PartInstance::getOrCreateNetworkOwnerChangedSignal(bool create)
{
    return NULL;
}

void PartInstance::processRemoteEvent(const EventDescriptor& descriptor, const EventArguments& args, const SystemAddress& source)
{
    if (descriptor == event_NetworkOwnerChanged)
    {
        SystemAddress address = args[0].cast<SystemAddress>();
        
        setNetworkOwner(address);
		if (DFFlag::FixFallenPartsNotDeleted)
		{
			// networkOwnerTime does NOTHING on the Client
			// When a part falls bellow "Kill Zone" they are pushed to server for ownership
			// and then the server deletes them on the next frame.
			// Without a networkOwnerTime(out) we may run NetworkOwnerJob which will return ownership to the Client.
			resetNetworkOwnerTime(3.0f);
		}
		
    }
    else
    {
        Super::processRemoteEvent(descriptor, args, source);
    }
}

bool PartInstance::getCanCollide() const
{
	return !getConstPartPrimitive()->getPreventCollide();
}

void PartInstance::setCanCollide(bool value)
{
	bool preventCollide = !value;
	if (preventCollide != getPartPrimitive()->getPreventCollide()) {
		getPartPrimitive()->setPreventCollide( preventCollide );
		raiseChanged(prop_CanCollide);
	}
}

bool PartInstance::getAnchored() const
{
	return getConstPartPrimitive()->getAnchoredProperty();
}

void PartInstance::setAnchored(bool value)
{
	if (value != getPartPrimitive()->getAnchoredProperty()) {
		std::vector<Primitive*> futureMechRoots; 
		RootPrimitiveOwnershipData ownershipData;

		if (value)
		{
			gatherResetOwnershipDataPreAnchor(futureMechRoots, ownershipData);
		}
		else if (DFFlag::SetNetworkOwnerFixAnchoring2)
		{
			gatherResetOwnershipDataPreUnanchor(futureMechRoots);
		}

		getPartPrimitive()->setAnchoredProperty( value );

		if (value)
		{
			updateOwnershipSettingsPostAnchor(futureMechRoots, ownershipData);
		}
		else if (DFFlag::SetNetworkOwnerFixAnchoring2)
		{
			updateOwnershipSettingsPostUnAnchor(futureMechRoots);
		}
		else
		{
			updateResetOwnershipSettingsOnUnAnchor();
		}
		// Only need this to happen when setting to TRUE, because otherwise physics update take care of this.
		if (FFlag::AnchoredSendPositionUpdate && value)
		{
			// Anchored will prevent position updates from syncing up between clients.
			// This sends one final position update to make sure anchored parts stay anchored in same place on 
			// all clients.
			const RBX::SystemAddress localAddress = RBX::Network::Players::findLocalSimulatorAddress(this);
			if (this->getNetworkOwner() == localAddress)
			{
				raiseChanged(prop_CFrame);
			}
		}
		raiseChanged(prop_Anchored);
	}
}

bool PartInstance::isGrounded()
{
    return (primitive != NULL) && primitive->computeIsGrounded();
}

static void getConnectedPartsVisitor(Primitive *prim, shared_ptr<Instances> &inst)
{
    if (prim != NULL && PartInstance::fromPrimitive(prim) != NULL)
    {
        inst->push_back(shared_from(PartInstance::fromPrimitive(prim)));
    }
}

void PartInstance::getConnectedPartsRecursiveImpl(shared_ptr<Instances> &objs, boost::unordered_set<PartInstance*> &seen) const
{
    const Workspace* ws = Workspace::findConstWorkspace(this);
    const Instance* terrain = ws != NULL ? ws->getTerrain() : NULL;
    for (int i = 0; i < primitive->getNumJoints(); ++i)
    {
        Primitive* otherNeighbor = primitive->getJointOther(i);
        if (otherNeighbor != NULL  &&
            (Joint::isKinematicJoint(primitive->getJoint(i))))
        {
            PartInstance* otherPart = PartInstance::fromPrimitive(otherNeighbor);
            if (otherPart != NULL && otherPart != terrain && seen.find(otherPart) == seen.end())
            {
                seen.insert(otherPart);
                objs->push_back(shared_from(otherPart));
                otherPart->getConnectedPartsRecursiveImpl(objs, seen);
            }
        }
    }
}

void PartInstance::gatherResetOwnershipDataPreUnanchor(std::vector<Primitive*>& previousOwnershipRoots)
{
	Primitive* partPrim = getPartPrimitive();
	for (int i = 0; i < partPrim->getNumJoints(); ++i)
	{
		Primitive* otherNeighbor = partPrim->getJointOther(i);
		if (otherNeighbor)
		{
			if (Joint::isRigidJoint(partPrim->getJoint(i)))
			{
				// Kinematic Joint neighbors will be 
				if (Assembly* assembly = otherNeighbor->getAssembly())
				{
					for (int i = 0; i < assembly->numChildren(); i++)
					{
						if (Primitive* rootChildAssemblyPrim = assembly->getTypedChild<Assembly>(i)->getAssemblyPrimitive()->getRootMovingPrimitive())
						{
							previousOwnershipRoots.push_back(rootChildAssemblyPrim);
						}
					}
				}
			}
			else if (!otherNeighbor->computeIsGrounded())
			{
				// Free Motion joints will be their own assembly, make sure to Add their Root.
				if (Primitive* rootMovingPrim = otherNeighbor->getRootMovingPrimitive())
					previousOwnershipRoots.push_back(rootMovingPrim);
			}
		}
	}
}

void PartInstance::updateOwnershipSettingsPostUnAnchor(std::vector<Primitive*>& previousOwnershipRoots)
{
	RBX::SystemAddress consistentAddress;
	bool hasConsistentOwner;
	bool hasConsistentManualOwnershipRule;
	PartInstance::checkConsistentOwnerAndRuleResetRoots(consistentAddress, hasConsistentOwner, hasConsistentManualOwnershipRule, previousOwnershipRoots);
	Primitive* currentRootMovingPrim = getPartPrimitive()->getRootMovingPrimitive();
	if (hasConsistentOwner)
	{
		for (unsigned int i = 0; i < previousOwnershipRoots.size(); i++)
		{
			Primitive* rootPrim = previousOwnershipRoots[i]->getRootMovingPrimitive();
			if (previousOwnershipRoots[i] == rootPrim)
			{
				PartInstance::fromPrimitive(rootPrim)->setNetworkOwnerNotifyIfServer(consistentAddress, hasConsistentManualOwnershipRule);
			}
		}

		if (currentRootMovingPrim)
			PartInstance::fromPrimitive(currentRootMovingPrim)->setNetworkOwnerNotifyIfServer(consistentAddress, hasConsistentManualOwnershipRule);
	}

}

void PartInstance::gatherResetOwnershipDataPreAnchor(std::vector<Primitive*>& futureMechRoots, RootPrimitiveOwnershipData& ownershipData)
{
	Primitive* partPrim = getPartPrimitive();
	if (Primitive* rootMovingPrimitive = partPrim->getRootMovingPrimitive())
	{
		ownershipData.ownershipManual = rootMovingPrimitive->getNetworkOwnershipRuleInternal() == NetworkOwnership_Manual;
		ownershipData.ownerAddress = rootMovingPrimitive->getNetworkOwner();
		if (DFFlag::SetNetworkOwnerFixAnchoring2)
		{
			// The anchoring Body is not the current root. Which means after
			// anchoring the parent Primitive and the Child primitives may become roots.
			// Collect all the Children
			Assembly* assembly = partPrim->getAssembly();
			for (int i = 0; i < assembly->numChildren(); i++)
			{
				if (Assembly* childAssembly = assembly->getTypedChild<Assembly>(i))
				{
					Primitive* assemblyRoot = childAssembly->getAssemblyPrimitive();
					if (!assemblyRoot->computeIsGrounded())
						futureMechRoots.push_back(assemblyRoot);
				}
			}
			if (Assembly* parentAssembly = assembly->getTypedParent<Assembly>())
			{
				Primitive* assemblyRoot = parentAssembly->getAssemblyPrimitive();
				if (!assemblyRoot->computeIsGrounded())
					futureMechRoots.push_back(assemblyRoot);
			}
		}
		else if (DFFlag::SetNetworkOwnerFixAnchoring)
		{
			for (int i = 0; i < partPrim->numChildren(); i++)
			{
				if (Primitive* primChild = partPrim->getTypedChild<Primitive>(i))
				{
					if (!primChild->getAnchoredProperty())
						futureMechRoots.push_back(primChild);
				}
			}
			// Add the parent
			if (Primitive* primParent = partPrim->getTypedParent<Primitive>())
			{
				if (!primParent->getAnchoredProperty())
					futureMechRoots.push_back(primParent);
			}
		}
		else
		{
			Assembly* rootAssembly = rootMovingPrimitive->getAssembly();
			for (int i = 0; i < rootAssembly->numChildren(); i++)
			{
				if (Assembly* childAssembly = rootAssembly->getTypedChild<Assembly>(i))
				{
					futureMechRoots.push_back(childAssembly->getAssemblyPrimitive());
				}
			}
		}
		if (DFFlag::NetworkOwnershipRuleReplicates)
			ownershipData.prim = rootMovingPrimitive;
		else
			PartInstance::fromPrimitive(rootMovingPrimitive)->setNetworkOwnershipRule(NetworkOwnership_Auto);
	}
	else
	{
		ownershipData.prim = NULL;
	}
}

void PartInstance::updateOwnershipSettingsPostAnchor(std::vector<Primitive*>& futureMechRoots, RootPrimitiveOwnershipData& ownershipData)
{
	if (DFFlag::NetworkOwnershipRuleReplicates && ownershipData.prim)
	{
		// Reset ownership of old Root afte Anchor
		PartInstance::fromPrimitive(ownershipData.prim)->setNetworkOwnershipRuleIfServer(NetworkOwnership_Auto);
	}
	for (size_t i = 0; i < futureMechRoots.size(); i++)
	{
		if (DFFlag::SetNetworkOwnerFixAnchoring2)
		{
			PartInstance* futureMechRootPart = NULL;
			if (Primitive* rootMovingPrim = futureMechRoots[i]->getRootMovingPrimitive())
				futureMechRootPart = PartInstance::fromPrimitive(rootMovingPrim);

			if (futureMechRootPart)
			{
				if (DFFlag::NetworkOwnershipRuleReplicates)
				{
					futureMechRootPart->setNetworkOwnerNotifyIfServer(ownershipData.ownerAddress, ownershipData.ownershipManual);
				}
				else
				{
					futureMechRootPart->setNetworkOwner(ownershipData.ownerAddress);
					if ((RBX::Network::Players::serverIsPresent(this) || DFFlag::NetworkOwnershipRuleReplicates) 
						&& ownershipData.ownershipManual)
					{
						futureMechRootPart->setNetworkOwnershipRule(NetworkOwnership_Manual);
					}
				}
			}
		}
		else
		{
			RBXASSERT(Mechanism::isMovingAssemblyRoot(futureMechRoots[i]->getAssembly()));
			PartInstance* futureMechRootPart = PartInstance::fromPrimitive(futureMechRoots[i]);
			if (DFFlag::NetworkOwnershipRuleReplicates)
			{
				futureMechRootPart->setNetworkOwnerNotifyIfServer(ownershipData.ownerAddress, ownershipData.ownershipManual);
			}
			else
			{
				futureMechRootPart->setNetworkOwner(ownershipData.ownerAddress);
				if ((RBX::Network::Players::serverIsPresent(this) || DFFlag::NetworkOwnershipRuleReplicates) 
					&& ownershipData.ownershipManual)
				{
					futureMechRootPart->setNetworkOwnershipRule(NetworkOwnership_Manual);
				}
			}
		}
	}
}

void PartInstance::updateResetOwnershipSettingsOnUnAnchor()
{
	std::vector<Primitive*> ownershipRoots;
	if (Assembly* currentAssembly = getPartPrimitive()->getAssembly())
	{
		if (!currentAssembly->computeIsGrounded())
		{
			if (DFFlag::SetNetworkOwnerFixAnchoring)
			{
				for (int i = 0; i < getPartPrimitive()->numChildren(); i++)
				{
					if (Primitive* unanchorPrimChild = getPartPrimitive()->getTypedChild<Primitive>(i))
					{
						if (!unanchorPrimChild->getAnchoredProperty())
							ownershipRoots.push_back(unanchorPrimChild);
					}
				}
				if (Primitive* unanchorParentPrim = getPartPrimitive()->getTypedParent<Primitive>())
				{
					if (!unanchorParentPrim->getAnchoredProperty())
						ownershipRoots.push_back(unanchorParentPrim);
				}
			}
			else
			{
				for (int i = 0; i < currentAssembly->numChildren(); i++)
				{
					if (Primitive* rootChildAssemblyPrim = currentAssembly->getTypedChild<Assembly>(i)->getAssemblyPrimitive())
					{
						ownershipRoots.push_back(rootChildAssemblyPrim);
					}
				}
			}
			// Process for consistency
			RBX::SystemAddress addressToPass;
			bool hasConsistentAddress;
			bool hasConsistentOwnership;
			PartInstance::checkConsistentOwnerAndRuleResetRoots(addressToPass, hasConsistentAddress, hasConsistentOwnership, ownershipRoots);

			if (hasConsistentAddress)
			{
				Primitive* rootMovingPrim = getPartPrimitive()->getRootMovingPrimitive();
				PartInstance* rootMovingPrimPart = PartInstance::fromPrimitive(rootMovingPrim);
				if (rootMovingPrim && !PartInstance::isPlayerCharacterPart(rootMovingPrimPart))
				{
					if (DFFlag::NetworkOwnershipRuleReplicates)
					{
						rootMovingPrimPart->setNetworkOwnerNotifyIfServer(addressToPass, hasConsistentOwnership);
					}
					else
					{
						rootMovingPrimPart->setNetworkOwner(addressToPass);
						if ((RBX::Network::Players::serverIsPresent(this) || DFFlag::NetworkOwnershipRuleReplicates) 
							&& hasConsistentOwnership)
						{
							rootMovingPrimPart->setNetworkOwnershipRule(NetworkOwnership_Manual);
						}
					}
				}
			}
		}
	}
}
    
shared_ptr<const Instances> PartInstance::getConnectedParts(bool recursive)
{
    const Workspace* ws = Workspace::findConstWorkspace(this);
    const Instance* terrain = ws != NULL ? ws->getTerrain() : NULL;
    shared_ptr<Instances> result(new Instances());
    if (!recursive)
    {
        result->push_back(shared_from(this));
        for (int i = 0; i < primitive->getNumJoints(); ++i)
        {
            Primitive* otherNeighbor = primitive->getJointOther(i);
            if (otherNeighbor != NULL && PartInstance::fromPrimitive(otherNeighbor) != NULL &&
                PartInstance::fromPrimitive(otherNeighbor) != terrain &&
                (Joint::isKinematicJoint(primitive->getJoint(i))))
            {
                result->push_back(shared_from(PartInstance::fromPrimitive(otherNeighbor)));
            }
        }
    } else { // recursive
        if (primitive != NULL && primitive->getAssembly() != NULL)
        {
            if (!primitive->getAssembly()->computeIsGrounded())
            {
                primitive->getAssembly()->visitPrimitives(boost::bind(&getConnectedPartsVisitor, _1, result));
            } else {
                boost::unordered_set<PartInstance*> seen = boost::unordered_set<PartInstance*>();
                // add ourself to the list of parts
                result->push_back(shared_from(this));
                seen.insert(this);
                getConnectedPartsRecursiveImpl(result, seen);
            }
        }
    }
    return result;
}

shared_ptr<Instance> PartInstance::getRootPart()
{
	PartInstance* part = NULL;
	if (primitive != NULL && primitive->getAssembly() != NULL)
	{	
		part = PartInstance::fromPrimitive(primitive->getAssembly()->getAssemblyPrimitive());
	}
	return shared_from(part);
}

void PartInstance::setPartLocked(bool value)
{
	if (value != locked)
	{
		locked = value;
		raisePropertyChanged(prop_Locked);
	}
}

bool PartInstance::getLocked(Instance* instance)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance)) {
		return part->getPartLocked();
	}
	else {
		for (size_t i = 0; i < instance->numChildren(); i++) {
			if (getLocked(instance->getChild(i))) {
				return true;
			}
		}
		return false;
	}
}

void PartInstance::setLocked(Instance* instance, bool value)
{
	if (PartInstance* part = Instance::fastDynamicCast<PartInstance>(instance)) {
		part->setPartLocked(value);
	}
	else {
		for (size_t i = 0; i < instance->numChildren(); i++) {
			setLocked(instance->getChild(i), value);
		}
	}
}

void PartInstance::printNetAPIDisabledMessage()
{
	StandardOut::singleton()->printf(MESSAGE_WARNING, "SetNetworkOwnership API Disabled");
}

PartMaterial PartInstance::getRenderMaterial() const
{
	return getConstPartPrimitive()->getPartMaterial();
}

void PartInstance::setRenderMaterial(PartMaterial value)
{
	if(value != getPartPrimitive()->getPartMaterial())
	{
		getPartPrimitive()->setSpecificGravity(toSpecificGravity(value));
		// Has to be in this order for the correct SpecificGravity to be used
		getPartPrimitive()->setPartMaterial(value);
		raisePropertyChanged(prop_Density);
		raisePropertyChanged(prop_renderMaterial);
	}
}

const bool PartInstance::getNetworkOwnershipManualReplicate() const
{
	return getNetworkOwnershipRule() == NetworkOwnership_Manual;
}

void PartInstance::setNetworkOwnershipManualReplicate(bool value) 
{
	if (value != getNetworkOwnershipManualReplicate())
	{
		if (value)
		{
			setNetworkOwnershipRule(NetworkOwnership_Manual);
		}
		else
		{
			setNetworkOwnershipRule(NetworkOwnership_Auto);
		}
	}
}

const NetworkOwnership PartInstance::getNetworkOwnershipRule() const
{
	return getConstPartPrimitive()->getNetworkOwnershipRuleInternal();
}

void PartInstance::setNetworkOwnershipRule(NetworkOwnership value)
{
	if (getNetworkOwnershipRule() != value)
	{
		getPartPrimitive()->setNetworkOwnershipRuleInternal(value);
		if (DFFlag::NetworkOwnershipRuleReplicates && RBX::Network::Players::serverIsPresent(this, false))
		{
			raisePropertyChanged(prop_networkOwnershipRuleBool);
			//DFFlagNetworkOwnershipRuleReplicates
			//raisePropertyChanged(prop_networkOwnershipRule);
		}
	}
}    

void PartInstance::setNetworkOwnershipAuto()
{
	Primitive* rootPrim;
	std::string statusMessage;
	if (canSetNetworkOwnership(rootPrim, statusMessage))
	{
		PartInstance::fromPrimitive(rootPrim)->setNetworkOwnershipRule(NetworkOwnership_Auto);
	}
	else
	{
		throw std::runtime_error(statusMessage.c_str());
	}
}

bool PartInstance::getNetworkOwnershipAuto()
{
	Primitive* rootPrim;
	std::string statusMessage;
	if (canSetNetworkOwnership(rootPrim, statusMessage))
	{
		return (rootPrim->getNetworkOwnershipRuleInternal() == NetworkOwnership_Auto);
	}
	else
	{
		throw std::runtime_error(statusMessage.c_str());
	}

	return true;
}

void PartInstance::setNetworkOwnerScript(shared_ptr<Instance> playerInstance)
{
	Primitive* rootPrim;
	std::string statusMessage;
	if (canSetNetworkOwnership(rootPrim, statusMessage))
	{
		shared_ptr<Network::Player> player = Instance::fastSharedDynamicCast<Network::Player>(playerInstance);
		if (!player && playerInstance)
		{
			throw std::runtime_error("SetNetworkOwner only takes player or 'nil' instance as an argument.");
		}
		PartInstance* rootPart = PartInstance::fromPrimitive(rootPrim);
		if (Network::Players::backendProcessing(this, false) && Network::Players::frontendProcessing(this, false))
		{
			// Do nothing, PLAY SOLO
			return;
		}
		if (DataModel* dm = DataModel::get(this))
		{
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "PartInstance_SetNetworkOwnerScript", 
				boost::lexical_cast<std::string>(dm->getPlaceID()).c_str(), 0, false));

			if (!player)
			{
				boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "PartInstance_SetNetworkOwnerScript_TOSERVER", 
					boost::lexical_cast<std::string>(dm->getPlaceID()).c_str(), 0, false));
			}
		}

		RBX::SystemAddress ownerAddress;
		if (player)
		{
			ownerAddress = player->getRemoteAddressAsRbxAddress();
		}
		else
		{
			ownerAddress = RBX::Network::NetworkOwner::Server();
		}

		if (rootPart)
		{
			rootPart->setNetworkOwnershipRule(NetworkOwnership_Manual);
			rootPart->setNetworkOwnerAndNotify(ownerAddress);
		}
	}
	else
	{
		throw std::runtime_error(statusMessage.c_str());
	}
}
    
    
shared_ptr<Instance> PartInstance::getNetworkOwnerScript()
{
	Primitive* rootPrim;
	std::string statusMessage;
	if (canSetNetworkOwnership(rootPrim, statusMessage))
	{
		if(Network::Players::serverIsPresent(this))
		{
			if (DataModel* dm = DataModel::get(this))
			{
				static boost::once_flag flag = BOOST_ONCE_INIT;
				boost::call_once(flag, boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_GAME, "PartInstance_GetNetworkOwnerScript", 
					boost::lexical_cast<std::string>(dm->getPlaceID()).c_str(), 0, false));
			}

			RBX::SystemAddress networkOwner = rootPrim->getNetworkOwner();
			shared_ptr<Instance> player = Network::Players::findPlayerWithAddress(networkOwner, this);
			return player;

		}
		else if (Network::Players::backendProcessing(this, false) && Network::Players::frontendProcessing(this, false))
		{
			// we're in PlaySolo. Do nothing.
		}
	}
	else
	{
		throw std::runtime_error(statusMessage.c_str());
	}
    return shared_ptr<Instance>();
}

bool PartInstance::canSetNetworkOwnership(Primitive*& rootPrimitive, std::string& statusMessage)
{
	rootPrimitive = NULL;
	statusMessage.clear();

	if(!Workspace::getWorkspaceIfInWorkspace(this))
	{
		statusMessage = std::string("Can only call Network Ownership API on a part that is descendent of Workspace");
		return false;
	}

	if(Network::Players::clientIsPresent(this))
	{
		statusMessage = std::string("Network Ownership API can only be called from the Server.");
		return false;
	} 

	if (getPartPrimitive()->getGeometry()->isTerrain())
	{
		statusMessage = std::string("Network Ownership API cannot be used on Terrain");
		return false;
	}

	rootPrimitive = getPartPrimitive()->getRootMovingPrimitive();
	if (!rootPrimitive)
	{
		statusMessage = std::string("Network Ownership API cannot be called on Anchored parts or parts welded to Anchored parts.");
		return false;
	}

	return true;
}

shared_ptr<const Reflection::Tuple> PartInstance::canSetNetworkOwnershipScript()
{
	shared_ptr<Reflection::Tuple> args = rbx::make_shared<Reflection::Tuple>();

	Primitive* placeholderPointer;
	std::string statusMessage;
	bool canSet = canSetNetworkOwnership(placeholderPointer, statusMessage);
	args->values.push_back(canSet);
	if (!canSet)
	{
		args->values.push_back(statusMessage);
	}


	return args;	
}

bool PartInstance::isPlayerCharacterPart(PartInstance* part)
{
	// This is how we check during NetworkOwnerJob
	if (part)
	{
		if(ModelInstance* m = Instance::fastDynamicCast<ModelInstance>(part->getParent()))
		{
			if (Humanoid::modelIsCharacter(m) && Network::Players::getPlayerFromCharacter(m))
			{
				return true;
			}
		}
	}
	return false;
}

void PartInstance::checkConsistentOwnerAndRuleResetRoots(RBX::SystemAddress& consistentAddress, bool& hasConsistentOwner, bool& hasConsistentManualOwnershipRule, std::vector<Primitive*>& primRoots)
{
	bool ownerInitialized = false;
	consistentAddress                = Network::NetworkOwner::Unassigned();
	hasConsistentOwner				 = true;
	hasConsistentManualOwnershipRule = true;
	for (size_t i = 0; i < primRoots.size(); i++)
	{
		PartInstance* primRootPart = PartInstance::fromPrimitive(primRoots[i]);
		if (PartInstance::isPlayerCharacterPart(primRootPart))
		{
			// We discard Players from this process, since they should not be affected
			// We still want to set the ownership of the PlayerCharacter to auto
			primRootPart->setNetworkOwnershipRule(NetworkOwnership_Auto);
			continue;
		}
		if (!ownerInitialized)
		{
			consistentAddress = primRoots[i]->getNetworkOwner();
			ownerInitialized = true;
		}
		if (primRoots[i]->getNetworkOwner() != consistentAddress)
		{
			hasConsistentOwner = false;
		}
		if (primRoots[i]->getNetworkOwnershipRuleInternal() != NetworkOwnership_Manual)
		{
			hasConsistentManualOwnershipRule = false;
		}

		if (!DFFlag::SetNetworkOwnerFixAnchoring2)
		{
			if (DFFlag::NetworkOwnershipRuleReplicates)
				primRootPart->setNetworkOwnershipRuleIfServer(NetworkOwnership_Auto);
			else
				primRootPart->setNetworkOwnershipRule(NetworkOwnership_Auto);
		}
	}

	if (DFFlag::SetNetworkOwnerFixAnchoring2)
	{
		// NetworkOwnershipRule should only ever be set to Auto from the server
		// Allowing the Client to decide ownership is Auto can lead to weird race conditions
		// Imagine you have a configuration like this:
		//				            Where A and B are joined by a Hinge, B and C are also joined by a hinge
		//	[A]-[B]-[C] 			Our replication system Queues properties in the order the first change happened.
		//							This causes the server to send the final state of this assembly to the Client
		//                          but because it arrives first, when the anchor event arrives, the ownership is reset to Auto.
		
		// If primRoots is a redundant set of objects, we want to setAuto
		// after we figure out consistency in ownership, so that we don't alter the result.
		for (size_t i = 0; i < primRoots.size(); i++)
		{
			if (DFFlag::NetworkOwnershipRuleReplicates)
				PartInstance::fromPrimitive(primRoots[i])->setNetworkOwnershipRuleIfServer(NetworkOwnership_Auto);
			else
				PartInstance::fromPrimitive(primRoots[i])->setNetworkOwnershipRule(NetworkOwnership_Auto);
		}
	}

	if (!ownerInitialized)
	{
		// If the Network Address was never assigned, that means we were
		// creating joints in a Player character, so we need to not run logic.
		hasConsistentOwner = false;
		hasConsistentManualOwnershipRule = false;
	}
}

void PartInstance::setTransparency(float value)
{
	if (value != transparency) {
		transparency = value;
		raisePropertyChanged(prop_Transparency);
		shouldRenderSetDirty();
	}
}

void PartInstance::setLocalTransparencyModifier(float value)
{
	if (value != localTransparencyModifier) {
		localTransparencyModifier = value;
		raisePropertyChanged(prop_LocalTransparencyModifier);
	}
}

void PartInstance::setReflectance(float value)
{
	if (value != reflectance)
	{
		reflectance = value;
		raisePropertyChanged(prop_Reflectance);
	}
}

void PartInstance::setColor(BrickColor value)
{
	if (value != color)
	{
		color = value;
		raisePropertyChanged(prop_BrickColor);
		raisePropertyChanged(prop_Color);
	}
}

void PartInstance::setFriction(float friction) 
{
	float newVal = G3D::clamp(friction, -FLT_MIN, FLT_MAX);
	if (newVal != getPartPrimitive()->getFriction()) {
		getPartPrimitive()->setFriction(newVal);
		raisePropertyChanged(prop_Friction);
	}
}

float PartInstance::getFriction() const						
{
	return getConstPartPrimitive()->getFriction();
}

void PartInstance::setPhysicalProperties(const PhysicalProperties& prop)
{
	if (prop != getPartPrimitive()->getPhysicalProperties())
	{
		getPartPrimitive()->setPhysicalProperties(prop);
		raisePropertyChanged(prop_CustomPhysicalProperties);
	}
}

const PhysicalProperties& PartInstance::getPhysicalProperties() const
{
	return getConstPartPrimitive()->getPhysicalProperties();
}

void PartInstance::setElasticity(float elasticity) 
{
	float newVal = G3D::clamp(elasticity, 0.0f, 1.0f);
	if (newVal != getPartPrimitive()->getElasticity()) {
		getPartPrimitive()->setElasticity(newVal);
		raisePropertyChanged(prop_Elasticity);
	}
}

float PartInstance::getElasticity() const						
{
	return getConstPartPrimitive()->getElasticity();
}

float PartInstance::getSpecificGravity() const
{
	return getConstPartPrimitive()->getSpecificGravity();
}

bool PartInstance::resize(NormalId localNormalId, int amount)
{
	destroyJoints();
	bool result = resizeImpl(localNormalId, amount);
	join();
	return result;
}

bool PartInstance::resizeFloat(NormalId localNormalId, float amount, bool checkIntersection)
{
	destroyJoints();
	bool result = advResizeImpl(localNormalId, amount, checkIntersection);
	join();
	return result;
}

bool PartInstance::resizeImpl(NormalId localNormalId, int amount)
{
	RBXASSERT_VERY_FAST(!getPartPrimitive()->hasAutoJoints());

	if (amount == 0) {
		return false;
	}
	else {
		Vector3 originalSizeXml = getPartSizeXml();
		CoordinateFrame originalPosition = getCoordinateFrame();

		Vector3 deltaSizeUi; 
		float posNeg = (localNormalId < NORM_X_NEG) ? 1.0f : -1.0f;

		if (hasThreeDimensionalSize()) {
			deltaSizeUi[localNormalId % 3] = (float)amount;
		}
		else {
			if (!DFFlag::FormFactorDeprecated) 
			{
				RBXASSERT(getFormFactor() == PartInstance::SYMETRIC);
			}
			deltaSizeUi = amount * Vector3(1,1,1);
		}  

		Vector3 newSizeUi = xmlToUiSize(originalSizeXml) + deltaSizeUi;
		Vector3 newSizeXml = uiToXmlSize(newSizeUi);			// will snap on setting uiSize;
		Vector3 deltaSizeXml = newSizeXml - originalSizeXml;
		Vector3 deltaSizeWorld = originalPosition.rotation * deltaSizeXml;
		Vector3 newTranslation = originalPosition.translation + (0.5f * deltaSizeWorld * posNeg);
		CoordinateFrame newCoord(originalPosition.rotation, newTranslation);

		setCoordinateFrame(newCoord);
		setPartSizeXml(newSizeXml);

		if (ContactManager* contactManager = Workspace::getContactManagerIfInWorkspace(this)) {
			if (Dragger::intersectingWorldOrOthers(	*this, 
													*contactManager, 
													Tolerance::maxOverlapAllowedForResize(),
													Dragger::maxDragDepth())) {
				
				setCoordinateFrame(originalPosition);
				setPartSizeXml(originalSizeXml);
				return false;
			}
		}
		return true;
	}
}

bool PartInstance::advResizeImpl(NormalId localNormalId, float amount, bool checkIntersection)
{
	RBXASSERT_VERY_FAST(!getPartPrimitive()->hasAutoJoints());

	// Blow out if the requested change is less than the min resize increment.
	if ( fabs(amount) < getMinimumResizeIncrement() ) {
		return false;
	}
	else {
		// Set the form factor to CUSTOM if resizing a part off the 1 stud grid (or 1.2 or 0.4 stud grid in the y-direction)
		if(!DFFlag::FormFactorDeprecated && getFormFactor() != PartInstance::CUSTOM)
		{
			float fractPart, intPart, resizeIncrement;
			switch(getFormFactor())
			{
				case PartInstance::BRICK:
                    resizeIncrement = (localNormalId == NORM_Y || localNormalId == NORM_Y_NEG) ? 1.2f : 1.0f;
					break;
				case PartInstance::PLATE:
					resizeIncrement = (localNormalId == NORM_Y || localNormalId == NORM_Y_NEG) ? 0.4f : 1.0f;
					break;
				case PartInstance::SYMETRIC:
				default:
					resizeIncrement = 1.0f;
					break;
			}

			fractPart = modff (fabs(amount)/resizeIncrement, &intPart);
			if(fractPart > 0.0001f)
			{
				BasicPartInstance* aBPI = this->fastDynamicCast<BasicPartInstance>();
				if(aBPI)
				{
					//aBPI->setFormFactorUi(PartInstance::CUSTOM);
				}
			}
		}

		Vector3 originalSizeXml = getPartSizeXml();
		CoordinateFrame originalPosition = getCoordinateFrame();

		Vector3 deltaSizeUi; 
		float posNeg = (localNormalId < NORM_X_NEG) ? 1.0f : -1.0f;

		if (hasThreeDimensionalSize()) {
			deltaSizeUi[localNormalId % 3] = (float)amount;
		}
		else {
			if (!DFFlag::FormFactorDeprecated) 
			{
				RBXASSERT(getFormFactor() == PartInstance::SYMETRIC);
			}
			deltaSizeUi = amount * Vector3(1,1,1);
		}

		Vector3 newSizeUi = originalSizeXml + deltaSizeUi;
		Vector3 newSizeXml;
		newSizeXml.x = newSizeUi.x < getMinimumXOrZDimension() ? getMinimumXOrZDimension() : newSizeUi.x;
		newSizeXml.y = newSizeUi.y < getMinimumYDimension() ? getMinimumYDimension() : newSizeUi.y;
		newSizeXml.z = newSizeUi.z < getMinimumXOrZDimension() ? getMinimumXOrZDimension() : newSizeUi.z;
		Vector3 deltaSizeXml = newSizeXml - originalSizeXml;
		Vector3 deltaSizeWorld = originalPosition.rotation * deltaSizeXml;
		Vector3 newTranslation = originalPosition.translation + (0.5f * deltaSizeWorld * posNeg);
		CoordinateFrame newCoord(originalPosition.rotation, newTranslation);

		setCoordinateFrame(newCoord);
		setPartSizeXml(newSizeXml);

		if (checkIntersection)
		{
			if (ContactManager* contactManager = Workspace::getContactManagerIfInWorkspace(this)) {
				if (Dragger::intersectingWorldOrOthers(	*this, 
														*contactManager, 
														Tolerance::maxOverlapAllowedForResize(),
														Dragger::maxDragDepth())) {
					
					setCoordinateFrame(originalPosition);
					setPartSizeXml(originalSizeXml);
					return false;
				}
			}
		}

		return true;
	}
}

float PartInstance::getMinimumYDimension(void) const
{
	if (DFFlag::FormFactorDeprecated) {
		return 0.2f;
	} else {
		switch (getFormFactor())
		{
		case BRICK:
			return 1.2f;
		case PLATE:
			return 0.4f;
		case CUSTOM:
			return 0.2f;
		default:
			return 1.0f;
		}
	}
}

float PartInstance::getMinimumXOrZDimension(void) const
{
	if (DFFlag::FormFactorDeprecated) {
		return 0.2f;
	} else {
		switch (getFormFactor())
		{
		case CUSTOM:
			return 0.2f;
		default:
			return 1.0f;
		}
	}
}



// This is in Grid Location units - parts have their corner at 0,0,0
Surface PartInstance::getSurface(const RBX::RbxRay& gridRay, int& surfaceId)
{
	const CoordinateFrame& currentCoord = getCoordinateFrame();

	RBX::RbxRay localRay = currentCoord.toObjectSpace(gridRay);

	Extents extents = getPartPrimitive()->getExtentsLocal();
	Vector3 hitPoint;

	G3D::CollisionDetection::collisionLocationForMovingPointFixedAABox(
							localRay.origin(),
							localRay.direction(),
							G3D::AABox(extents.min(), extents.max()),
							hitPoint);

	for (int i = 0; i < 3; ++i) {
		if (hitPoint[i] == extents.max()[i]) {
			surfaceId = i;
			return Surface(this, (NormalId)surfaceId);
		}
		if (hitPoint[i] == extents.min()[i]) {
			surfaceId = 3+i;
			return Surface(this, (NormalId)surfaceId);
		}
	}
	RBXASSERT_IF_VALIDATING(0);
	return Surface(NULL, NORM_X);
}

SurfaceType PartInstance::getSurfaceType(NormalId surfId) const
{
	return getConstPartPrimitive()->getSurfaceType(surfId);
}

void PartInstance::setSurfaceType(NormalId surfId, SurfaceType type)
{
	if (type != getPartPrimitive()->getSurfaceType(surfId))
	{
		getPartPrimitive()->setSurfaceType(surfId, type);
		onSurfaceChanged(surfId);
		raiseSurfacePropertyChanged(Surface::getSurfaceTypeStatic(surfId));
	}
}

LegacyController::InputType PartInstance::getInput(NormalId surfId) const
{
	return getConstPartPrimitive()->getConstSurfaceData(surfId).inputType;
}

void PartInstance::setSurfaceInput(NormalId surfId, LegacyController::InputType inputType)
{
	RBXASSERT(getPartPrimitive());
	SurfaceData current = getPartPrimitive()->getSurfaceData(surfId);
	if (inputType != current.inputType)
	{
		current.inputType = inputType;
		getPartPrimitive()->setSurfaceData(surfId, current);
		onSurfaceChanged(surfId);
		raiseSurfacePropertyChanged(Surface::getSurfaceInputStatic(surfId));
	}
}

float PartInstance::getParamA(NormalId surfId) const
{
	RBXASSERT(getConstPartPrimitive());
	return getConstPartPrimitive()->getConstSurfaceData(surfId).paramA;
}

float PartInstance::getParamB(NormalId surfId) const
{
	RBXASSERT(getConstPartPrimitive());
	return getConstPartPrimitive()->getConstSurfaceData(surfId).paramB;
}

void PartInstance::setParamA(NormalId surfId, float value)
{
	SurfaceData current = getPartPrimitive()->getSurfaceData(surfId);
	if (current.paramA != value)
	{
		current.paramA = value;
		getPartPrimitive()->setSurfaceData(surfId, current);
		onSurfaceChanged(surfId);
		raiseSurfacePropertyChanged(Surface::getParamAStatic(surfId));
	}
}

void PartInstance::setParamB(NormalId surfId, float value)
{
	SurfaceData current = getPartPrimitive()->getSurfaceData(surfId);
	if (current.paramB != value)
	{
		current.paramB = value;
		getPartPrimitive()->setSurfaceData(surfId, current);
		onSurfaceChanged(surfId);
		raiseSurfacePropertyChanged(Surface::getParamBStatic(surfId));
	}
}

bool PartInstance::containedByFrustum(const RBX::Frustum& frustum) const
{
	return frustum.containsAABB(getConstPartPrimitive()->getExtentsLocal(), getCoordinateFrame());
}

bool PartInstance::intersectFrustum(const RBX::Frustum& frustum) const
{
    return frustum.intersectsAABB(getConstPartPrimitive()->getExtentsLocal(), getCoordinateFrame());
}

bool PartInstance::isStandardPart() const
{
	// Inlets on bottom, all sides flat, anything on top
	return (	(getSurfaceType(NORM_Y_NEG) == INLET)
		&&	(getSurfaceType(NORM_Z_NEG) == NO_SURFACE)
		&&	(getSurfaceType(NORM_Z) == NO_SURFACE)
		&&	(getSurfaceType(NORM_X_NEG) == NO_SURFACE)
		&&	(getSurfaceType(NORM_X) == NO_SURFACE)	);
}

namespace Reflection
{
	template<>
	const Type& Type::getSingleton<CoordinateFrame>()
	{
		static TType<CoordinateFrame> type("CoordinateFrame");
		return type;
	}
}

static float getComponent(const XmlElement* element, const XmlTag& name)
{
	float result = 0.0f;
	if(const XmlElement* valueElement = element->findFirstChildByTag(name)){
		bool gotComponent = valueElement->getValue(result);
		RBXASSERT(gotComponent);
		return result;
	}
	return result;
}

namespace Reflection {
template<>
void RBX::TypedPropertyDescriptor<CoordinateFrame>::readValue(DescribedBase* instance, const XmlElement* element, IReferenceBinder& binder) const
{
	if (!element->isXsiNil())
	{
		CoordinateFrame c;
		c.translation.x = getComponent(element, tag_X);
		c.translation.y = getComponent(element, tag_Y);
		c.translation.z = getComponent(element, tag_Z);
		c.rotation[0][0] = getComponent(element, tag_R00);
		c.rotation[0][1] = getComponent(element, tag_R01);
		c.rotation[0][2] = getComponent(element, tag_R02);
		c.rotation[1][0] = getComponent(element, tag_R10);
		c.rotation[1][1] = getComponent(element, tag_R11);
		c.rotation[1][2] = getComponent(element, tag_R12);
		c.rotation[2][0] = getComponent(element, tag_R20);
		c.rotation[2][1] = getComponent(element, tag_R21);
		c.rotation[2][2] = getComponent(element, tag_R22);
		setValue(instance, c);
	}
}

template<>
void RBX::TypedPropertyDescriptor<CoordinateFrame>::writeValue(const DescribedBase* instance, XmlElement* element) const
{
	// Write the data out in accordance with ROBLOX Schema
	G3D::CoordinateFrame c = getValue(instance);
	element->addChild(new XmlElement(tag_X, c.translation.x));
	element->addChild(new XmlElement(tag_Y, c.translation.y));
	element->addChild(new XmlElement(tag_Z, c.translation.z));
	element->addChild(new XmlElement(tag_R00, c.rotation[0][0]));
	element->addChild(new XmlElement(tag_R01, c.rotation[0][1]));
	element->addChild(new XmlElement(tag_R02, c.rotation[0][2]));
	element->addChild(new XmlElement(tag_R10, c.rotation[1][0]));
	element->addChild(new XmlElement(tag_R11, c.rotation[1][1]));
	element->addChild(new XmlElement(tag_R12, c.rotation[1][2]));
	element->addChild(new XmlElement(tag_R20, c.rotation[2][0]));
	element->addChild(new XmlElement(tag_R21, c.rotation[2][1]));
	element->addChild(new XmlElement(tag_R22, c.rotation[2][2]));
}

template<>
int TypedPropertyDescriptor<CoordinateFrame>::getDataSize(const DescribedBase* instance) const 
{
    return sizeof(CoordinateFrame);
}

template<>
bool RBX::TypedPropertyDescriptor<CoordinateFrame>::hasStringValue() const {
	return false;
}

template<>
std::string RBX::TypedPropertyDescriptor<CoordinateFrame>::getStringValue(const DescribedBase* instance) const{
	return Super::getStringValue(instance);
}

template<>
bool RBX::TypedPropertyDescriptor<CoordinateFrame>::setStringValue(DescribedBase* instance, const std::string& text) const {
	return Super::setStringValue(instance, text);
}
}//namespace Reflection

void PartInstance::fireOutfitChanged()
{
	OutfitChangedSignalData data;
	combinedSignal(OUTFIT_CHANGED, &data);
	if(onDemandRead())
		onDemandWrite()->outfitChangedSignal();
}

void PartInstance::reportTouch(const shared_ptr<PartInstance>& other)
{
	if (hasTouchTransmitter() && onDemandWrite()->touchTransmitter->checkTouch(other))
		onDemandWrite()->touchedSignal(other);
}
void PartInstance::reportUntouch(const shared_ptr<PartInstance>& other)
{
	if (hasTouchTransmitter() && onDemandWrite()->touchTransmitter->checkUntouch(other))
		onDemandWrite()->touchEndedSignal(other);
}

void PartInstance::setIsCurrentlyStreamRemovingPart() {
	onDemandWrite()->isCurrentlyStreamRemovingPart = true;
}

bool PartInstance::getIsCurrentlyStreamRemovingPart() const {
	return onDemandRead() && onDemandRead()->isCurrentlyStreamRemovingPart;
}

    Humanoid* PartInstance::findAncestorModelWithHumanoid()
    {
        Instance* current = getParent();
        
        while (current)
        {
            if (Humanoid* humanoid = Humanoid::modelIsCharacter(current))
                return humanoid;
            
            current = current->getParent();
        }
        
        return NULL;
    }

	void PartInstance::updateHumanoidCookie()
	{
        if (DFFlag::HumanoidCookieRecursive)
        {
            if (findAncestorModelWithHumanoid())
    			cookie |= PartCookie::IS_HUMANOID_PART;
            else
    			cookie &= ~PartCookie::IS_HUMANOID_PART;
        }
        else
        {
			Instance* parent = getParent();

			// Only treat parts as humanoid parts if they're not directly under workspace
			if (parent && Humanoid::modelIsCharacter(parent) && !Instance::isA<Workspace>(parent))
			{
				cookie |= PartCookie::IS_HUMANOID_PART;
			}
			else
			{
				cookie &= ~PartCookie::IS_HUMANOID_PART;
			}
		}
    }

	void PartInstance::humanoidChanged()
	{
		updateHumanoidCookie();
		Super::humanoidChanged();
	}
    
    float PartInstance::getReceiveInterval() const
    {
        return renderingCoordinateFrame ? renderingCoordinateFrame->getSampleInterval() : 0.f;
    }

	void PartInstance::createRenderingCoordinateFrame()
	{
		if (!renderingCoordinateFrame)
		{
			renderingCoordinateFrame.reset(new PathInterpolatedCFrame());
			renderingCoordinateFrame->setRenderedFrame(getCoordinateFrame());
		}
	}

} // namespace

