#include "stdafx.h"

/* Copyright 2003-2014 ROBLOX Corporation, All Rights Reserved  */
#include "V8Tree/Instance.h"
#include "V8Tree/Service.h"
#include "V8DataModel/StarterPlayerService.h"
#include "V8DataModel/PlayerScripts.h"
#include "V8DataModel/GameBasicSettings.h"
#include "V8DataModel/Folder.h"
#include "Humanoid/Humanoid.h"
#include "Network/Player.h"
#include "Network/Players.h"
#include "Util/RobloxGoogleAnalytics.h"

DYNAMIC_FASTFLAGVARIABLE(UseStarterPlayerCharacter, false)
DYNAMIC_FASTFLAGVARIABLE(UseStarterPlayerCharacterScripts, false)
DYNAMIC_FASTFLAGVARIABLE(UseStarterPlayerHumanoid, false)


namespace RBX {

const char *const sStarterPlayerService = "StarterPlayer"; 

REFLECTION_BEGIN();
static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperTouchCameraMovementMode> prop_devTouchCameraMovementMode("DevTouchCameraMovementMode", "Camera", &StarterPlayerService::getDevTouchCameraMovementMode, &StarterPlayerService::setDevTouchCameraMovementMode);
static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperComputerCameraMovementMode> prop_devComputerCameraMovementMode("DevComputerCameraMovementMode", "Camera", &StarterPlayerService::getDevComputerCameraMovementMode, &StarterPlayerService::setDevComputerCameraMovementMode);
static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperCameraOcclusionMode> prop_devCameraOcclusionMode("DevCameraOcclusionMode", "Camera", &StarterPlayerService::getDevCameraOcclusionMode, &StarterPlayerService::setDevCameraOcclusionMode);
static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperTouchMovementMode> prop_devTouchMovementMode("DevTouchMovementMode", "Controls", &StarterPlayerService::getDevTouchMovementMode, &StarterPlayerService::setDevTouchMovementMode);
static Reflection::EnumPropDescriptor<StarterPlayerService, StarterPlayerService::DeveloperComputerMovementMode> prop_devComputerMovementMode("DevComputerMovementMode", "Controls", &StarterPlayerService::getDevComputerMovementMode, &StarterPlayerService::setDevComputerMovementMode);
static Reflection::PropDescriptor<StarterPlayerService, bool> prop_enableMouseLockOption("EnableMouseLockOption", "Controls", &StarterPlayerService::getEnableMouseLockOption, &StarterPlayerService::setEnableMouseLockOption);
static Reflection::PropDescriptor<StarterPlayerService, bool> prop_autoJumpEnabled("AutoJumpEnabled", "Mobile", &StarterPlayerService::getAutoJumpEnabled, &StarterPlayerService::setAutoJumpEnabled);
static Reflection::PropDescriptor<StarterPlayerService, bool> prop_loadCharacterAppearance("LoadCharacterAppearance", "Character", &StarterPlayerService::getLoadCharacterAppearance, &StarterPlayerService::setLoadCharacterAppearance);

static Reflection::EnumPropDescriptor<StarterPlayerService, Camera::CameraMode> prop_cameraMode("CameraMode", "Camera", &StarterPlayerService::getCameraMode, &StarterPlayerService::setCameraMode);
static Reflection::PropDescriptor<StarterPlayerService, float> prop_cameraMaxZoomDistance("CameraMaxZoomDistance", "Camera", &StarterPlayerService::getCameraMaxZoomDistance, &StarterPlayerService::setCameraMaxZoomDistance);
static Reflection::PropDescriptor<StarterPlayerService, float> prop_cameraMinZoomDistance("CameraMinZoomDistance", "Camera", &StarterPlayerService::getCameraMinZoomDistance, &StarterPlayerService::setCameraMinZoomDistance);

static Reflection::PropDescriptor<StarterPlayerService, float> prop_nameDisplayDistance("NameDisplayDistance", category_Data, &StarterPlayerService::getNameDisplayDistance, &StarterPlayerService::setNameDisplayDistance);
static Reflection::PropDescriptor<StarterPlayerService, float> prop_healthDisplayDistance("HealthDisplayDistance", category_Data, &StarterPlayerService::getHealthDisplayDistance, &StarterPlayerService::setHealthDisplayDistance);
REFLECTION_END();

	namespace Reflection {

		template <>
		EnumDesc<StarterPlayerService::DeveloperTouchCameraMovementMode>::EnumDesc()
		:EnumDescriptor("DevTouchCameraMovementMode")
		{	
			addPair(StarterPlayerService::DEV_TOUCH_CAMERA_MOVEMENT_MODE_USER, "UserChoice");
			addPair(StarterPlayerService::DEV_TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC, "Classic");
			addPair(StarterPlayerService::DEV_TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW, "Follow");
		}

		template <>
		EnumDesc<StarterPlayerService::DeveloperComputerCameraMovementMode>::EnumDesc()
		:EnumDescriptor("DevComputerCameraMovementMode")
		{	
			addPair(StarterPlayerService::DEV_COMPUTER_CAMERA_MOVEMENT_MODE_USER, "UserChoice");
			addPair(StarterPlayerService::DEV_COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC, "Classic");
			addPair(StarterPlayerService::DEV_COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW, "Follow");
		}

		template <>
		EnumDesc<StarterPlayerService::DeveloperCameraOcclusionMode>::EnumDesc()
		:EnumDescriptor("DevCameraOcclusionMode")
		{	
			addPair(StarterPlayerService::DEV_CAMERA_OCCLUSION_MODE_ZOOM, "Zoom");
			addPair(StarterPlayerService::DEV_CAMERA_OCCLUSION_MODE_INVISI, "Invisicam");
		}

		template <>
		EnumDesc<StarterPlayerService::DeveloperTouchMovementMode>::EnumDesc()
		:EnumDescriptor("DevTouchMovementMode")
		{	
			addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_USER, "UserChoice");
			addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_THUMBSTICK, "Thumbstick");
			addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_DPAD, "DPad");
			addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_THUMBPAD, "Thumbpad");
			addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE, "ClickToMove");
			addPair(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_SCRIPTABLE, "Scriptable");
		}

		template <>
		EnumDesc<StarterPlayerService::DeveloperComputerMovementMode>::EnumDesc()
		:EnumDescriptor("DevComputerMovementMode")
		{	
			addPair(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_USER, "UserChoice");
			addPair(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_KBD_MOUSE, "KeyboardMouse");
			addPair(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE, "ClickToMove");
			addPair(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_SCRIPTABLE, "Scriptable");
		}


		// TODO: Add rest of enums here and in factoryregistration.cpp
	}


static float defaultDisplayDistance(100.0f);

StarterPlayerService::StarterPlayerService()
	: cameraMode(RBX::Camera::CAMERAMODE_CLASSIC)
	, nameDisplayDistance(defaultDisplayDistance)
	, healthDisplayDistance(defaultDisplayDistance)
	, cameraMaxZoomDistance(RBX::Camera::distanceMaxCharacter())
	, cameraMinZoomDistance(RBX::Camera::distanceMin())
	, enableMouseLockOption(true)
	, touchCameraMovementMode(StarterPlayerService::DEV_TOUCH_CAMERA_MOVEMENT_MODE_USER)
	, computerCameraMovementMode(StarterPlayerService::DEV_COMPUTER_CAMERA_MOVEMENT_MODE_USER)
	, cameraOcclusionMode(StarterPlayerService::DEV_CAMERA_OCCLUSION_MODE_ZOOM)
	, touchMovementMode(StarterPlayerService::DEV_TOUCH_MOVEMENT_MODE_USER)
	, computerMovementMode(StarterPlayerService::DEV_COMPUTER_MOVEMENT_MODE_USER)
	, autoJumpEnabled(true)
	, loadCharacterAppearance(true)
{
	Instance::setName(sStarterPlayerService);
}
 
bool StarterPlayerService::askForbidChild(const Instance* instance) const {

	if (Instance::fastDynamicCast<StarterPlayerScripts>(instance) != NULL)
		return false;

	if (DFFlag::UseStarterPlayerHumanoid) {
		bool isHumanoid = Instance::fastDynamicCast<Humanoid>(instance) != NULL;
		if (isHumanoid)
			return false;
	}

	if (DFFlag::UseStarterPlayerCharacterScripts) {
		bool isCharacterScripts = (Instance::fastDynamicCast<StarterCharacterScripts>(instance) != NULL);
		if (isCharacterScripts)
			return false;
	}

	if (DFFlag::UseStarterPlayerCharacter)
	{
		bool isModel = Instance::fastDynamicCast<ModelInstance>(instance) != NULL;
		if (isModel)
			return false;
	}

	return true;
}

bool StarterPlayerService::askAddChild(const Instance* instance) const {
	if (Instance::fastDynamicCast<StarterPlayerScripts>(instance) != NULL)
		return true;

	if (DFFlag::UseStarterPlayerHumanoid) {
		bool isHumanoid = Instance::fastDynamicCast<Humanoid>(instance) != NULL;
		if (isHumanoid)
			return true;
	} 
	
	if (DFFlag::UseStarterPlayerCharacterScripts) {
		bool isCharacterScripts = (Instance::fastDynamicCast<StarterCharacterScripts>(instance) != NULL);
		if (isCharacterScripts)
			return true;
	} 

	if (DFFlag::UseStarterPlayerCharacter)
	{
		bool isModel = Instance::fastDynamicCast<ModelInstance>(instance) != NULL;
		if (isModel)
			return true;
	}

	return false;
}


void StarterPlayerService::setCameraMode(RBX::Camera::CameraMode value)
{
	if(cameraMode != value)
	{
		cameraMode = value;
		raisePropertyChanged(prop_cameraMode);
	}
}

void StarterPlayerService::setEnableMouseLockOption(bool setting)
{
	if(enableMouseLockOption != setting)
	{
		enableMouseLockOption = setting;
		raisePropertyChanged(prop_enableMouseLockOption);
	}
}

void StarterPlayerService::setAutoJumpEnabled(bool value)
{
	if (autoJumpEnabled != value)
	{
		autoJumpEnabled = value;
		raisePropertyChanged(prop_autoJumpEnabled);
	}
}

void StarterPlayerService::setLoadCharacterAppearance(bool value)
{
	if (DFFlag::UseStarterPlayerCharacter)
	{
		if (loadCharacterAppearance != value)
		{
			loadCharacterAppearance = value;
			raisePropertyChanged(prop_loadCharacterAppearance);
		}
	}
}

void StarterPlayerService::setDevTouchCameraMovementMode(DeveloperTouchCameraMovementMode setting)
{
	if(touchCameraMovementMode != setting)
	{
		touchCameraMovementMode = setting;
		raisePropertyChanged(prop_devTouchCameraMovementMode);
	}
}

void StarterPlayerService::setDevComputerCameraMovementMode(DeveloperComputerCameraMovementMode setting)
{
	if(computerCameraMovementMode != setting)
	{
		computerCameraMovementMode = setting;
		raisePropertyChanged(prop_devComputerCameraMovementMode);
	}
}

void StarterPlayerService::setDevCameraOcclusionMode(DeveloperCameraOcclusionMode setting)
{
	if(cameraOcclusionMode != setting)
	{
		cameraOcclusionMode = setting;
		raisePropertyChanged(prop_devCameraOcclusionMode);
	}
}

void StarterPlayerService::setDevTouchMovementMode(DeveloperTouchMovementMode setting)
{
	if(touchMovementMode != setting)
	{
		touchMovementMode = setting;
		raisePropertyChanged(prop_devTouchMovementMode);
	}
}

void StarterPlayerService::setDevComputerMovementMode(DeveloperComputerMovementMode setting)
{
	if(computerMovementMode != setting)
	{
		computerMovementMode = setting;
		raisePropertyChanged(prop_devComputerMovementMode);
	}
}



void StarterPlayerService::setNameDisplayDistance(float value)
{
	float legalValue = G3D::max(value, 0.0f);
	if(nameDisplayDistance != legalValue)
	{
		nameDisplayDistance = legalValue;
		raisePropertyChanged(prop_nameDisplayDistance);
	}
}

void StarterPlayerService::setHealthDisplayDistance(float value)
{
	float legalValue = G3D::max(value, 0.0f);
	if(healthDisplayDistance != legalValue)
	{
		healthDisplayDistance = legalValue;
		raisePropertyChanged(prop_healthDisplayDistance);
	}
}

void StarterPlayerService::setCameraMaxZoomDistance(float value)
{
	value = G3D::clamp(value, cameraMinZoomDistance, Camera::distanceMaxCharacter());
	if(cameraMaxZoomDistance != value)
	{
		cameraMaxZoomDistance = value;
		raisePropertyChanged(prop_cameraMaxZoomDistance);
	}
}

void StarterPlayerService::setCameraMinZoomDistance(float value)
{
	value = G3D::clamp(value, Camera::distanceMin(), cameraMaxZoomDistance);
	if(cameraMinZoomDistance != value)
	{
		cameraMinZoomDistance = value;
		raisePropertyChanged(prop_cameraMinZoomDistance);
	}
}

void StarterPlayerService::setupPlayer(shared_ptr<Instance> instance)
{
	if (Network::Player* player = Instance::fastDynamicCast<Network::Player>(instance.get()))
	{
		player->setCameraMode(cameraMode);
		player->setCameraMaxZoomDistance(cameraMaxZoomDistance);
		player->setCameraMinZoomDistance(cameraMinZoomDistance);
		player->setHealthDisplayDistance(healthDisplayDistance);
		player->setNameDisplayDistance(nameDisplayDistance);
		player->setAutoJumpEnabled(autoJumpEnabled);
	}
}

void StarterPlayerService::onServiceProvider(ServiceProvider* oldProvider, ServiceProvider* newProvider)
{
	Super::onServiceProvider(oldProvider, newProvider);

	if (newProvider && RBX::Network::Players::backendProcessing(newProvider)) 
	{
		Network::Players* players = ServiceProvider::find<Network::Players>(newProvider);
		if (players)
			players->visitDescendants(boost::bind(&StarterPlayerService::setupPlayer, this, _1));

		if (DataModel* dataModel = DataModel::get(newProvider))
			workspaceLoadedConnection = dataModel->workspaceLoadedSignal.connect(boost::bind(&StarterPlayerService::setupPlayerScripts, this));
	}
}



void StarterPlayerService::setupPlayerScripts()
{
	RBX::StarterPlayerScripts* currPlayerScripts= findFirstChildOfType<RBX::StarterPlayerScripts>();
	if (!currPlayerScripts)
	{
		shared_ptr<RBX::StarterPlayerScripts> playerScripts = Creatable<Instance>::create<RBX::StarterPlayerScripts>();
		currPlayerScripts = playerScripts.get();
		if (currPlayerScripts) {
			playerScripts->setParent(this);
		}			
	}

	if (DFFlag::UseStarterPlayerCharacterScripts)
	{
		RBX::StarterPlayerScripts* pStarterCharacterScripts = findFirstChildOfType<RBX::StarterCharacterScripts>();
		if (!pStarterCharacterScripts)
		{
			shared_ptr<RBX::StarterCharacterScripts> characterScripts = Creatable<Instance>::create<RBX::StarterCharacterScripts>();
			currPlayerScripts = characterScripts.get();
			if (currPlayerScripts) {
				currPlayerScripts->setParent(this);
			}			
		}
	}
}

void StarterPlayerService::recordSettingsInGA() const
{
	const char *CameraMovement = NULL;
	const char *CameraOcclusion = NULL;
	const char *CharacterMovement = NULL;

	switch (touchCameraMovementMode) {
		case DEV_TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC:
			CameraMovement = "TouchCameraMoveModeClassic";
			break;
		case DEV_TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW:
			CameraMovement = "TouchCameraMoveModeFollow";
			break;
		case DEV_TOUCH_CAMERA_MOVEMENT_MODE_USER:
		default:
			CameraMovement = "TouchCameraMoveModeUser";
			break;
	}
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DevTouchCameraMove", CameraMovement);

	switch (touchMovementMode) {
	case DEV_TOUCH_MOVEMENT_MODE_THUMBSTICK:
		CharacterMovement = "TouchMovementModeThumbStick";
		break;
	case DEV_TOUCH_MOVEMENT_MODE_DPAD:
		CharacterMovement = "TouchMovementModeDPad";
		break;
	case DEV_TOUCH_MOVEMENT_MODE_THUMBPAD:
		CharacterMovement = "TouchMovementModeThumbpad";
		break;
	case DEV_TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE:
		CharacterMovement = "TouchMovementModeClickToMove";
		break;
	case DEV_TOUCH_MOVEMENT_MODE_SCRIPTABLE:
		CharacterMovement = "TouchMovementModeScriptable";
		break;
	case DEV_TOUCH_MOVEMENT_MODE_USER:
	default:
		CharacterMovement = "TouchMovementModeUser";
		break;
	}
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DevTouchMovement", CharacterMovement);

	switch (computerCameraMovementMode) {
		case DEV_COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC:
			CameraMovement = "ComputerCameraMoveModeClassic";
			break;
		case DEV_COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW:
			CameraMovement = "ComputerCameraMoveModeFollow";
			break;
		case DEV_COMPUTER_CAMERA_MOVEMENT_MODE_USER:
		default:
			CameraMovement = "ComputerCameraMoveModeUser";
			break;
	}
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DevComputerCameraMove", CameraMovement);

	switch (computerMovementMode) {
	case DEV_COMPUTER_MOVEMENT_MODE_KBD_MOUSE:
		CharacterMovement = "ComputerMovementModeKbdMouse";
		break;
	case DEV_COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE:
		CharacterMovement = "ComputerMovementModeClickToMove";
		break;
	case DEV_COMPUTER_MOVEMENT_MODE_SCRIPTABLE:
		CharacterMovement = "ComputerMovementModeScriptable";
		break;
	case DEV_COMPUTER_MOVEMENT_MODE_USER:
	default:
		CharacterMovement = "ComputerMovementModeUser";
		break;
	}
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DevComputerMovement", CharacterMovement);

	if (enableMouseLockOption) {
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DevComputerEnableMouseLock", "True");
	} else {
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DevComputerEnableMouseLock", "False");
	}

	switch (cameraOcclusionMode) {
	case DEV_CAMERA_OCCLUSION_MODE_INVISI:
		CameraOcclusion = "DevCameraOcclusionInvisi";
		break;
	case DEV_CAMERA_OCCLUSION_MODE_ZOOM:
	default:
		CameraOcclusion = "DevCameraOcclusionZoom";
		break;
	}
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "DevCameraOcclusion", CameraOcclusion);
}




} // Namespace
