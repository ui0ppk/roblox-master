#include "stdafx.h"

#include "V8DataModel/GameBasicSettings.h"
#include "Util/RobloxGoogleAnalytics.h"

using namespace RBX;

REFLECTION_BEGIN();
static const Reflection::EnumPropDescriptor<GameBasicSettings, GameBasicSettings::ControlMode> prop_controlMode("ControlMode", category_Control, &GameBasicSettings::getControlMode, &GameBasicSettings::setControlMode);
static const Reflection::EnumPropDescriptor<GameBasicSettings, GameBasicSettings::CameraMode> prop_cameraMode("CameraMode", category_Control, &GameBasicSettings::getCameraMode, &GameBasicSettings::setCameraMode, Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
static const Reflection::EnumPropDescriptor<GameBasicSettings, GameBasicSettings::TouchCameraMovementMode> prop_touchCameraMovementMode("TouchCameraMovementMode", category_Control, &GameBasicSettings::getTouchCameraMovementMode, &GameBasicSettings::setTouchCameraMovementMode);
static const Reflection::PropDescriptor<GameBasicSettings, bool> prop_TouchCameraMovementChanged("TouchCameraMovementChanged",category_Data, &GameBasicSettings::getTouchCameraMovementModeModified,&GameBasicSettings::setTouchCameraMovementModeModified, Reflection::PropertyDescriptor::CLUSTER, Security::Roblox);
static const Reflection::EnumPropDescriptor<GameBasicSettings, GameBasicSettings::ComputerCameraMovementMode> prop_computerCameraMovementMode("ComputerCameraMovementMode", category_Control, &GameBasicSettings::getComputerCameraMovementMode, &GameBasicSettings::setComputerCameraMovementMode);
static const Reflection::PropDescriptor<GameBasicSettings, bool> prop_ComputerCameraMovementChanged("ComputerCameraMovementChanged",category_Data, &GameBasicSettings::getComputerCameraMovementModeModified,&GameBasicSettings::setComputerCameraMovementModeModified, Reflection::PropertyDescriptor::CLUSTER, Security::Roblox);
static const Reflection::EnumPropDescriptor<GameBasicSettings, GameBasicSettings::TouchMovementMode> prop_touchMovementMode("TouchMovementMode", category_Control, &GameBasicSettings::getTouchMovementMode, &GameBasicSettings::setTouchMovementMode);
static const Reflection::PropDescriptor<GameBasicSettings, bool> prop_TouchMovementChanged("TouchMovementChanged",category_Data, &GameBasicSettings::getTouchMovementModeModified,&GameBasicSettings::setTouchMovementModeModified, Reflection::PropertyDescriptor::CLUSTER, Security::Roblox);
static const Reflection::EnumPropDescriptor<GameBasicSettings, GameBasicSettings::ComputerMovementMode> prop_computerMovementMode("ComputerMovementMode", category_Control, &GameBasicSettings::getComputerMovementMode, &GameBasicSettings::setComputerMovementMode);
static const Reflection::PropDescriptor<GameBasicSettings, bool> prop_ComputerMovementChanged("ComputerMovementChanged",category_Data, &GameBasicSettings::getComputerMovementModeModified,&GameBasicSettings::setComputerMovementModeModified, Reflection::PropertyDescriptor::CLUSTER, Security::Roblox);
static const Reflection::EnumPropDescriptor<GameBasicSettings, GameSettings::UploadSetting> prop_uploadVideo("VideoUploadPromptBehavior", category_Video, &GameBasicSettings::getUploadVideoSetting, &GameBasicSettings::setUploadVideoSetting, Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
static const Reflection::EnumPropDescriptor<GameBasicSettings, GameBasicSettings::RenderQualitySetting> prop_renderQuality("SavedQualityLevel", category_Appearance, &GameBasicSettings::getRenderQuality, &GameBasicSettings::setRenderQuality);
static const Reflection::EnumPropDescriptor<GameBasicSettings, GameBasicSettings::RotationType> prop_rotationType("RotationType", category_Control, &GameBasicSettings::getRotationType, &GameBasicSettings::setRotationType, Reflection::PropertyDescriptor::SCRIPTING);

static Reflection::BoundFuncDesc<GameBasicSettings, bool(std::string)> func_getTutorialState(&GameBasicSettings::getTutorialState, "GetTutorialState", "tutorialId", Security::RobloxScript);
static Reflection::BoundFuncDesc<GameBasicSettings, void(std::string, bool)> func_setTutorialState(&GameBasicSettings::setTutorialState, "SetTutorialState", "tutorialId", "value", Security::RobloxScript);
static const Reflection::PropDescriptor<GameBasicSettings, std::string> prop_CompletedTutorials("CompletedTutorials",category_Data, &GameBasicSettings::getCompletedTutorials,&GameBasicSettings::setCompletedTutorials, Reflection::PropertyDescriptor::STREAMING, Security::RobloxScript);
static const Reflection::PropDescriptor<GameBasicSettings, bool> prop_AllTutorialsDisabled("AllTutorialsDisabled",category_Data, &GameBasicSettings::getAllTutorialsDisabled,&GameBasicSettings::setAllTutorialsDisabled, Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);

static Reflection::PropDescriptor<GameBasicSettings, bool> prop_startMaximized("StartMaximized",category_Data, &GameBasicSettings::getStartMaximized, &GameBasicSettings::setStartMaximized, Reflection::PropertyDescriptor::PUBLIC_SERIALIZED, Security::RobloxScript);
static Reflection::PropDescriptor<GameBasicSettings, Vector2> prop_startScreenSize("StartScreenSize",category_Data, &GameBasicSettings::getStartScreenSize, &GameBasicSettings::setStartScreenSize, Reflection::PropertyDescriptor::PUBLIC_SERIALIZED, Security::RobloxScript);
static Reflection::PropDescriptor<GameBasicSettings, Vector2> prop_startScreenPos("StartScreenPosition",category_Data, &GameBasicSettings::getStartScreenPos, &GameBasicSettings::setStartScreenPos, Reflection::PropertyDescriptor::PUBLIC_SERIALIZED, Security::RobloxScript);

Reflection::PropDescriptor<GameBasicSettings, float> GameBasicSettings::prop_masterVolume("MasterVolume",category_Data, &GameBasicSettings::getMasterVolume, &GameBasicSettings::setMasterVolume);

static Reflection::PropDescriptor<GameBasicSettings, float> prop_mouseSensitivity("MouseSensitivity",category_Data, &GameBasicSettings::getMouseSensitivity, &GameBasicSettings::setMouseSensitivity);

static const Reflection::PropDescriptor<GameBasicSettings, bool> prop_isFullscreen("Fullscreen",category_Data, &GameBasicSettings::getFullScreenConst, &GameBasicSettings::setFullScreen, Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
static const Reflection::BoundFuncDesc<GameBasicSettings, bool()> func_inFullscreenMode(&GameBasicSettings::getFullScreen, "InFullScreen", Security::None);
static Reflection::BoundFuncDesc<GameBasicSettings, bool()> func_inStudioMode(&GameBasicSettings::inStudioMode, "InStudioMode", Security::None);

static Reflection::EventDesc<GameBasicSettings, void(bool)> event_StudioModeChanged(&GameBasicSettings::studioModeChangedSignal, "StudioModeChanged", "isStudioMode", Security::None);
static Reflection::EventDesc<GameBasicSettings, void(bool)> event_FullscreenChanged(&GameBasicSettings::fullscreenChangedSignal, "FullscreenChanged", "isFullscreen", Security::None);

static const Reflection::EnumPropDescriptor<GameBasicSettings, GameSettings::UploadSetting> prop_uploadScreenshots("ImageUploadPromptBehavior", "Screenshots", &GameBasicSettings::getPostImageSetting, &GameBasicSettings::setPostImageSetting, Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);
static Reflection::PropDescriptor<GameBasicSettings, std::string> prop_googleAnalyticsClientId("gaID", "Configuration", &GameBasicSettings::getGoogleAnalyticsClientId, &GameBasicSettings::setGoogleAnalyticsClientId, Reflection::PropertyDescriptor::CLUSTER, Security::RobloxScript); // TODO: change CLUSTER to more generic name

static const Reflection::PropDescriptor<GameBasicSettings, bool> prop_usedHideHudShortcut("UsedHideHudShortcut",category_Data, &GameBasicSettings::getUsedHideHudShortcut, &GameBasicSettings::setUsedHideHudShortcut, Reflection::PropertyDescriptor::STANDARD, Security::RobloxScript);

REFLECTION_END();

namespace RBX {
	namespace Reflection {

		template <>
		EnumDesc<GameBasicSettings::ControlMode>::EnumDesc()
		:EnumDescriptor("ControlMode")
		{	
			// Uncomment this code to get all the mouse setttings back, for now we just want classic + mouse lock switch
			/*addPair(GameBasicSettings::CONTROL_CAMLOCK, "CharacterLock");
			addPair(GameBasicSettings::CONTROL_MOUSEPAN, "DragToLook");
			addPair(GameBasicSettings::CONTROL_HYBRID, "ClickToLook");*/
			addPair(GameBasicSettings::CONTROL_MOUSELOCK, "MouseLockSwitch");
			addLegacyName("Mouse Lock Switch", GameBasicSettings::CONTROL_MOUSELOCK);
			addPair(GameBasicSettings::CONTROL_CLASSIC, "Classic");
		}

		template <>
		EnumDesc<GameBasicSettings::RenderQualitySetting>::EnumDesc()
		:EnumDescriptor("SavedQualitySetting")
		{	
			addPair(GameBasicSettings::QUALITY_AUTO, "Automatic");
			addPair(GameBasicSettings::QUALITY_1, "QualityLevel1");
			addPair(GameBasicSettings::QUALITY_2, "QualityLevel2");
			addPair(GameBasicSettings::QUALITY_3, "QualityLevel3");
			addPair(GameBasicSettings::QUALITY_4, "QualityLevel4");
			addPair(GameBasicSettings::QUALITY_5, "QualityLevel5");
			addPair(GameBasicSettings::QUALITY_6, "QualityLevel6");
			addPair(GameBasicSettings::QUALITY_7, "QualityLevel7");
			addPair(GameBasicSettings::QUALITY_8, "QualityLevel8");
			addPair(GameBasicSettings::QUALITY_9, "QualityLevel9");
			addPair(GameBasicSettings::QUALITY_10, "QualityLevel10");
		}

		template <>
		EnumDesc<GameBasicSettings::CameraMode>::EnumDesc()
		:EnumDescriptor("CustomCameraMode")
		{	
			addPair(GameBasicSettings::CAMERA_MODE_DEFAULT, "Default");
			addPair(GameBasicSettings::CAMERA_MODE_FOLLOW, "Follow");
			addPair(GameBasicSettings::CAMERA_MODE_CLASSIC, "Classic");
		}

		template <>
		EnumDesc<GameBasicSettings::TouchCameraMovementMode>::EnumDesc()
		:EnumDescriptor("TouchCameraMovementMode")
		{	
			addPair(GameBasicSettings::TOUCH_CAMERA_MOVEMENT_MODE_DEFAULT, "Default");
			addPair(GameBasicSettings::TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW, "Follow");
			addPair(GameBasicSettings::TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC, "Classic");
		}

		template <>
		EnumDesc<GameBasicSettings::ComputerCameraMovementMode>::EnumDesc()
		:EnumDescriptor("ComputerCameraMovementMode")
		{	
			addPair(GameBasicSettings::COMPUTER_CAMERA_MOVEMENT_MODE_DEFAULT, "Default");
			addPair(GameBasicSettings::COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW, "Follow");
			addPair(GameBasicSettings::COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC, "Classic");
		}

		template <>
		EnumDesc<GameBasicSettings::TouchMovementMode>::EnumDesc()
		:EnumDescriptor("TouchMovementMode")
		{	
			addPair(GameBasicSettings::TOUCH_MOVEMENT_MODE_DEFAULT, "Default");
			addPair(GameBasicSettings::TOUCH_MOVEMENT_MODE_THUMBSTICK, "Thumbstick");
			addPair(GameBasicSettings::TOUCH_MOVEMENT_MODE_DPAD, "DPad");
			addPair(GameBasicSettings::TOUCH_MOVEMENT_MODE_THUMBPAD, "Thumbpad");
			addPair(GameBasicSettings::TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE, "ClickToMove");
		}

		template <>
		EnumDesc<GameBasicSettings::ComputerMovementMode>::EnumDesc()
		:EnumDescriptor("ComputerMovementMode")
		{	
			addPair(GameBasicSettings::COMPUTER_MOVEMENT_MODE_DEFAULT, "Default");
			addPair(GameBasicSettings::COMPUTER_MOVEMENT_MODE_KBD_MOUSE, "KeyboardMouse");
			addPair(GameBasicSettings::COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE, "ClickToMove");
		}

		template <>
		EnumDesc<GameBasicSettings::RotationType>::EnumDesc()
		:EnumDescriptor("RotationType")
		{
			addPair(GameBasicSettings::ROTATION_TYPE_MOVEMENT_RELATIVE, "MovementRelative");
			addPair(GameBasicSettings::ROTATION_TYPE_CAMERA_RELATIVE, "CameraRelative");
		}
	}
}

const char *const RBX::sGameBasicSettings = "UserGameSettings";
GameBasicSettings::GameBasicSettings()
	:controlMode(CONTROL_CLASSIC)
	,renderQualitySetting(QUALITY_AUTO)
	,mouseLocked(false)
	,canMousePan(true)
	,freeLook(false)
	,allTutorialsDisabled(false)
	,uploadVideos(GameSettings::ASK)
	,uploadScreenshots(GameSettings::ASK)
	,fullscreen(false)
	,studio(false)
	,cameraMode(CAMERA_MODE_DEFAULT)
	,touchCameraMovementMode(TOUCH_CAMERA_MOVEMENT_MODE_DEFAULT)
	,touchCameraMovementModeModified(false)
	,computerCameraMovementMode(COMPUTER_CAMERA_MOVEMENT_MODE_DEFAULT)
	,computerCameraMovementModeModified(false)
	,touchMoveMode(TOUCH_MOVEMENT_MODE_DEFAULT)
	,touchMoveModeModeModified(false)
	,computerMoveMode(COMPUTER_MOVEMENT_MODE_DEFAULT)
	,computerMoveModeModeModified(false)
	,rotationType(ROTATION_TYPE_MOVEMENT_RELATIVE)
	,startScreenPos(20,20)
	,startScreenSize(800,600)
	,masterVolume(1.0f)
	,mouseSensitivity(1.0f)
	,startMaximized(true)
	,usedHideHudShortcut(false)
{
	setName("GameSettings");
}

void GameBasicSettings::setControlMode(ControlMode setting)
{
	try {
		RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set camera control mode");
	} 
	catch (RBX::base_exception& e) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set ControlMode");
		throw e;
	}

	if(controlMode != setting)
	{
		controlMode = setting;
		raisePropertyChanged(prop_controlMode);
	}
}

GameBasicSettings::CameraMode GameBasicSettings::getCameraModeWithDefault() const
{
	if (cameraMode == GameBasicSettings::CAMERA_MODE_DEFAULT) {
#if defined(RBX_PLATFORM_IOS) || defined(__ANDROID__)
		return GameBasicSettings::CAMERA_MODE_FOLLOW;
#else
		return GameBasicSettings::CAMERA_MODE_CLASSIC;
#endif
	} else {
		return cameraMode;
	}
}

void GameBasicSettings::setStartScreenPos(Vector2 value)
{
	if (startScreenPos != value)
	{
		startScreenPos = value;
		raisePropertyChanged(prop_startScreenPos);
	}
}

void GameBasicSettings::setStartScreenSize(Vector2 value)
{
	if (startScreenSize != value)
	{
		startScreenSize = value;
		raisePropertyChanged(prop_startScreenSize);
	}
}

void GameBasicSettings::setStartMaximized(bool value)
{
	if (startMaximized != value)
	{
		startMaximized = value;
		raisePropertyChanged(prop_startMaximized);
	}
}

void GameBasicSettings::setCameraMode(CameraMode setting)
{
	if(cameraMode != setting)
	{
		const char *label = "CustomCameraModeFollow";
		if(setting == CAMERA_MODE_CLASSIC) {
			label = "CustomCameraModeClassic";
		}

		if ( RobloxGoogleAnalytics::isInitialized() )
		{
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "CustomCameraMode", label);
		}

		cameraMode = setting;
		raisePropertyChanged(prop_cameraMode);
	}
}

void GameBasicSettings::setTouchCameraMovementMode(TouchCameraMovementMode setting)
{
	try {
		RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set camera movement mode for touch devices");
	} 
	catch (RBX::base_exception& e) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set TouchCameraMovementMode");
		throw e;
	}

	if(touchCameraMovementMode != setting)
	{
		const char *label = NULL;
		switch (setting) {
			case TOUCH_CAMERA_MOVEMENT_MODE_DEFAULT:
			case TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW:
				label = "TouchCameraMoveModeFollow";
				break;
			case TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC:
				label = "TouchCameraMoveModeClassic";
				break;
		}
		
		if (RobloxGoogleAnalytics::isInitialized())
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "TouchCameraMoveMode", label);

		touchCameraMovementMode = setting;
		raisePropertyChanged(prop_touchCameraMovementMode);
		setTouchCameraMovementModeModified(true);
	}
}

void GameBasicSettings::setTouchCameraMovementModeModified(bool value)
{
	if(touchCameraMovementModeModified != value)
	{
		touchCameraMovementModeModified = value;
		raisePropertyChanged(prop_TouchCameraMovementChanged);
	}
}

void GameBasicSettings::setComputerCameraMovementMode(ComputerCameraMovementMode setting)
{
	try {
		RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set camera movement mode for computer devices");
	} 
	catch (RBX::base_exception& e) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set ComputerCameraMovementMode");
		throw e;
	}

	if(computerCameraMovementMode != setting)
	{
		const char *label = NULL;
		switch (setting) {
			case COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW:
				label = "ComputerCameraMoveModeFollow";
				break;
			case COMPUTER_CAMERA_MOVEMENT_MODE_DEFAULT:
			case COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC:
				label = "ComputerCameraMoveModeClassic";
				break;
		}
		
		if (RobloxGoogleAnalytics::isInitialized())
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ComputerCameraMoveMode", label);

		computerCameraMovementMode = setting;
		raisePropertyChanged(prop_computerCameraMovementMode);
		setComputerCameraMovementModeModified(true);
	}
}

void GameBasicSettings::setComputerCameraMovementModeModified(bool value)
{
	if(computerCameraMovementModeModified != value)
	{
		computerCameraMovementModeModified = value;
		raisePropertyChanged(prop_ComputerCameraMovementChanged);
	}
}

void GameBasicSettings::setTouchMovementMode(TouchMovementMode setting)
{
	try {
		RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set character movement mode for touch devices");
	} 
	catch (RBX::base_exception& e) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set TouchMovementMode");
		throw e;
	}

	if(touchMoveMode != setting)
	{
		const char *label = NULL;
		switch (setting) {
			case TOUCH_MOVEMENT_MODE_DEFAULT:
			case TOUCH_MOVEMENT_MODE_THUMBSTICK:
			default:
				label = "TouchMovementModeThumbStick";
				break;
			case TOUCH_MOVEMENT_MODE_DPAD:
				label = "TouchMovementModeDPad";
				break;
			case TOUCH_MOVEMENT_MODE_THUMBPAD:
				label = "TouchMovementModeThumbpad";
				break;
			case TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE:
				label = "TouchMovementModeClickToMove";
				break;
		}
		if (RobloxGoogleAnalytics::isInitialized())
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "TouchMovementMode", label);

		touchMoveMode = setting;
		raisePropertyChanged(prop_touchMovementMode);
		setTouchMovementModeModified(true);
	}
}

void GameBasicSettings::setTouchMovementModeModified(bool value)
{
	if(touchMoveModeModeModified != value)
	{
		touchMoveModeModeModified = value;
		raisePropertyChanged(prop_TouchMovementChanged);
	}
}

void GameBasicSettings::setComputerMovementMode(ComputerMovementMode setting)
{
	try {
		RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set character movement mode for computer devices");
	} 
	catch (RBX::base_exception& e) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set ComputerMovementMode");
		throw e;
	}

	if(computerMoveMode != setting)
	{
		const char *label = NULL;
		switch (setting) {
			case COMPUTER_MOVEMENT_MODE_DEFAULT:
			case COMPUTER_MOVEMENT_MODE_KBD_MOUSE:
			default:
				label = "ComputerMovementModeKeyboardMouse";
				break;
			case COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE:
				label = "ComputerMovementModeClickToMove";
				break;
		}
		if (RobloxGoogleAnalytics::isInitialized())
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ComputerMovementMode", label);

		computerMoveMode = setting;
		raisePropertyChanged(prop_computerMovementMode);
		setComputerMovementModeModified(true);
	}
}

void GameBasicSettings::setComputerMovementModeModified(bool value)
{
	if(computerMoveModeModeModified != value)
	{
		computerMoveModeModeModified = value;
		raisePropertyChanged(prop_ComputerMovementChanged);
	}
}

RBX::GameBasicSettings::RotationType GameBasicSettings::getRotationType() const
{
	return rotationType;
}

void GameBasicSettings::setRotationType(RotationType setting)
{
	if (rotationType != setting)
	{
		rotationType = setting;
		raisePropertyChanged(prop_rotationType);
	}
}

void GameBasicSettings::setRenderQuality(RenderQualitySetting value)
{
	try {
		RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set render quality level");
	} 
	catch (RBX::base_exception& e) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set SavedQualityLevel");
		throw e;
	}

	if(renderQualitySetting != value)
	{
		renderQualitySetting = value;
		raisePropertyChanged(prop_renderQuality);
	}
}

void GameBasicSettings::setMouseLock(bool isLocked)
{
		mouseLocked = isLocked;
}
bool GameBasicSettings::getTutorialState(std::string tutorialId)
{
	if(tutorialId.find(',') != std::string::npos)
		throw std::runtime_error("TutorialId's cannot contain commas");
	if(tutorialId.size() < 1)
		throw std::runtime_error("TutorialId's cannot be empty strings");
	if(allTutorialsDisabled)
		return true;

	std::map<std::string, bool>::const_iterator iter = tutorialState.find(tutorialId);
	if(iter == tutorialState.end())
		return false;
	return iter->second;
}

void GameBasicSettings::setUploadVideoSetting(GameSettings::UploadSetting setting) 
{
	if(uploadVideos != setting){
		uploadVideos = setting; 
		raisePropertyChanged(prop_uploadVideo);
	}
}

void GameBasicSettings::setPostImageSetting(GameSettings::UploadSetting setting) 
{
	if(uploadScreenshots != setting){
		uploadScreenshots = setting; 
		raisePropertyChanged(prop_uploadScreenshots);
	}
}

void GameBasicSettings::setTutorialState(std::string tutorialId, bool value)
{
	if(!allTutorialsDisabled && getTutorialState(tutorialId) != value){
		tutorialState[tutorialId] = value;
		raisePropertyChanged(prop_CompletedTutorials);
	}
}

std::string GameBasicSettings::getCompletedTutorials() const
{
	std::ostringstream ostr;
	for(std::map<std::string, bool>::const_iterator iter = tutorialState.begin(), end = tutorialState.end();
		iter != end; ++iter)
	{
		if(iter->second){
			ostr << iter->first << ",";
		}
	}
	return ostr.str();
}
void GameBasicSettings::setCompletedTutorials(std::string value)
{
	int oldPos = 0;
	int pos = value.find(',', oldPos);
	while(pos != std::string::npos)
	{
		int len = pos-oldPos;
		if(len > 0){
			std::string key = value.substr(oldPos, pos-oldPos);
			tutorialState[key] = true;
		}
		oldPos = pos + 1;
		pos = value.find(',', oldPos);
	}
}

void GameBasicSettings::setAllTutorialsDisabled(bool value)
{
	if(allTutorialsDisabled != value)
	{
		allTutorialsDisabled = value;
		raisePropertyChanged(prop_AllTutorialsDisabled);
	}
}

void GameBasicSettings::setMasterVolume(float value)
{
	try {
		RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set master sound volume");
	} 
	catch (RBX::base_exception& e) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set MasterVolume");
		throw e;
	}


	value = G3D::clamp(value,0.0f,1.0f);
	if (value != masterVolume)
	{
		masterVolume = value;
		raisePropertyChanged(prop_masterVolume);
	}
}

float GameBasicSettings::getMouseSensitivity() const 
{ 
	return mouseSensitivity; 
}

void GameBasicSettings::setMouseSensitivity(float value)
{
	try {
		RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set mouse sensitivity");
	} 
	catch (RBX::base_exception& e) 
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set MouseSensitivity");
		throw e;
	}

	value = G3D::clamp(value,0.2f, 10.0f);
	if (value != mouseSensitivity)
	{
		mouseSensitivity = value;
		raisePropertyChanged(prop_mouseSensitivity);
	}
}

std::string GameBasicSettings::getGoogleAnalyticsClientId() const
{
	return googleAnalyticsClientId;
}

void GameBasicSettings::setGoogleAnalyticsClientId(const std::string& id)
{
	googleAnalyticsClientId = id;
}

void GameBasicSettings::reset()
{
	setControlMode(CONTROL_CLASSIC);
	setRenderQuality(QUALITY_AUTO);
	setMouseLock(true);
	canMousePan = true;
	freeLook = false;
	setUploadVideoSetting(GameSettings::ASK);
	setPostImageSetting(GameSettings::ASK);
	googleAnalyticsClientId = std::string();
}

/*override*/ void GameBasicSettings::verifySetParent(const Instance* instance) const
{
	if (RBX::Security::Context::current().identity != RBX::Security::Anonymous) 
	{
		try {
			RBX::Security::Context::current().requirePermission(RBX::Security::RobloxScript, "set GameSettings parent");
		} 
		catch (RBX::base_exception& e) 
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Insufficient permissions to set GameSettings parent");
			throw e;
		}
	}

	Super::verifySetParent(instance);
}

void GameBasicSettings::recordSettingsInGA(bool touchEnabled) const
{
	const char *CameraMovement = NULL;
	const char *CharacterMovement = NULL;

	if (touchEnabled) {
		switch (touchCameraMovementMode) {
			case TOUCH_CAMERA_MOVEMENT_MODE_CLASSIC:
				CameraMovement = "TouchCameraMoveModeClassic";
				break;
			case TOUCH_CAMERA_MOVEMENT_MODE_FOLLOW:
				CameraMovement = "TouchCameraMoveModeFollow";
				break;
			case TOUCH_CAMERA_MOVEMENT_MODE_DEFAULT:
			default:
				CameraMovement = "TouchCameraMoveModeDefault";
				break;
		}
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "TouchCameraMove", CameraMovement);
		if (touchCameraMovementModeModified) {
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "TouchCameraMoveModified", CameraMovement);
		}

		switch (touchMoveMode) {
		case TOUCH_MOVEMENT_MODE_THUMBSTICK:
			CharacterMovement = "TouchMovementModeThumbStick";
			break;
		case TOUCH_MOVEMENT_MODE_DPAD:
			CharacterMovement = "TouchMovementModeDPad";
			break;
		case TOUCH_MOVEMENT_MODE_THUMBPAD:
			CharacterMovement = "TouchMovementModeThumbpad";
			break;
		case TOUCH_MOVEMENT_MODE_CLICK_TO_MOVE:
			CharacterMovement = "TouchMovementModeClickToMove";
			break;
		case TOUCH_MOVEMENT_MODE_DEFAULT:
		default:
			CharacterMovement = "TouchMovementModeDefault";
			break;
		}
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "TouchMovement", CharacterMovement);
		if (touchMoveModeModeModified) {
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "TouchMovementModified", CharacterMovement);
		}
	} else {
		switch (computerCameraMovementMode) {
			case COMPUTER_CAMERA_MOVEMENT_MODE_CLASSIC:
				CameraMovement = "ComputerCameraMoveModeClassic";
				break;
			case COMPUTER_CAMERA_MOVEMENT_MODE_FOLLOW:
				CameraMovement = "ComputerCameraMoveModeFollow";
				break;
			case COMPUTER_CAMERA_MOVEMENT_MODE_DEFAULT:
			default:
				CameraMovement = "ComputerCameraMoveModeDefault";
				break;
		}
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ComputerCameraMove", CameraMovement);
		if (computerCameraMovementModeModified) {
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ComputerCameraMoveModified", CameraMovement);
		}

		switch (computerMoveMode) {
		case COMPUTER_MOVEMENT_MODE_KBD_MOUSE:
			CharacterMovement = "ComputerMovementModeKbdMouse";
			break;
		case COMPUTER_MOVEMENT_MODE_CLICK_TO_MOVE:
			CharacterMovement = "ComputerMovementModeClickToMove";
			break;
		case COMPUTER_MOVEMENT_MODE_DEFAULT:
		default:
			CharacterMovement = "ComputerMovementModeDefault";
			break;
		}
		RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ComputerMovement", CharacterMovement);
		if (computerMoveModeModeModified) {
			RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_GAME, "ComputerMovementModified", CharacterMovement);
		}
	}
}

// Randomized Locations for hackflags
namespace RBX 
{ 
    namespace Security
    {
        unsigned int hackFlag5 = 0;
    };
};
