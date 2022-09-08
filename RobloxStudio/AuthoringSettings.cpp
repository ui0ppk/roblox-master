/**
 * AuthoringSettings.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"

// Qt Headers
#include <QDesktopServices>
#include <qmath.h>

// Roblox Headers
#include "v8datamodel/Camera.h"
#include "tool/ToolsArrow.h"
#include "tool/AdvRunDragger.h"
#include "AppDraw/Draw.h"
#include "script/script.h"

// Roblox Studio Headers
#include "AuthoringSettings.h"
#include "RobloxSettings.h"
#include "QFontBoundProp.h"
#include "QDirBoundProp.h"
#include "MobileDevelopmentDeployer.h"

#include "util/RobloxGoogleAnalytics.h"

FASTFLAG(LuaDebugger)
FASTFLAG(LuaDebuggerBreakOnError)

FASTFLAG(StudioSettingsGAEnabled)
FASTFLAG(StudioUseDraggerGrid)

RBX_REGISTER_CLASS(AuthoringSettings);
RBX_REGISTER_ENUM(AuthoringSettings::PermissionLevelShown);
RBX_REGISTER_ENUM(AuthoringSettings::HoverAnimateSpeed);
RBX_REGISTER_ENUM(AuthoringSettings::OutputLayoutMode);
RBX_REGISTER_ENUM(AuthoringSettings::ListDisplayMode);
RBX_REGISTER_ENUM(AuthoringSettings::UIStyle);
RBX_REGISTER_ENUM(AuthoringSettings::TestServerAudioBehavior);

const char* const sAuthoringSettings = "Studio";
const char* const sScriptCategoryName = "Script Editor";
const char* const sScriptColorsCategoryName = "Script Editor Colors";

// AutoSave
RBX::Reflection::BoundProp<QDir> prop_AutoSaveDir("Auto-Save Path", "Auto-Save", &AuthoringSettings::autoSaveDir);
RBX::Reflection::BoundProp<bool> prop_AutoSaveEnable("Auto-Save Enabled", "Auto-Save", &AuthoringSettings::autoSaveEnabled);
RBX::Reflection::BoundProp<int> prop_AutoSaveMinutes("Auto-Save Interval (Minutes)", "Auto-Save", &AuthoringSettings::autoSaveMinutesInterval);

// Browsing
RBX::Reflection::BoundProp<bool> prop_ShowDepricatedItems("DeprecatedObjectsShown", "Browsing", &AuthoringSettings::showDeprecated);
RBX::Reflection::EnumPropDescriptor<AuthoringSettings, AuthoringSettings::PermissionLevelShown> prop_PermissionLevelShown("PermissionLevelShown", "Browsing", &AuthoringSettings::getPermissionLevelShown, &AuthoringSettings::setPermissionLevelShown);

// Scripting - all script properties must be in sScriptCategoryName or ScriptTextEditor::onPropertyChange will not update properly
RBX::Reflection::BoundProp<int> prop_EditorTabWidth("Tab Width", sScriptCategoryName, &AuthoringSettings::editorTabWidth);
RBX::Reflection::BoundProp<bool> prop_EditorAutoIndent("Auto Indent", sScriptCategoryName, &AuthoringSettings::editorAutoIndent);
RBX::Reflection::BoundProp<QFont> prop_EditorFont("Font", sScriptCategoryName, &AuthoringSettings::editorFont);
RBX::Reflection::BoundProp<bool> prop_EditorTextWrap("Text Wrapping", sScriptCategoryName, &AuthoringSettings::editorTextWrap);

// Script Colors - all script properties must be in sScriptColorsCategoryName or ScriptSyntaxHighlighter::onPropertyChange will not update properly
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorTextColor("Text Color", sScriptColorsCategoryName, &AuthoringSettings::editorTextColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorBackgroundColor("Background Color", sScriptColorsCategoryName, &AuthoringSettings::editorBackgroundColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorSelectionColor("Selection Color", sScriptColorsCategoryName, &AuthoringSettings::editorSelectionColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorSelectionBackgroundColor("Selection Background Color", sScriptColorsCategoryName, &AuthoringSettings::editorSelectionBackgroundColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorOperatorColor("Operator Color", sScriptColorsCategoryName, &AuthoringSettings::editorOperatorColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorNumberColor("Number Color", sScriptColorsCategoryName, &AuthoringSettings::editorNumberColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorStringColor("String Color", sScriptColorsCategoryName, &AuthoringSettings::editorStringColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorCommentColor("Comment Color", sScriptColorsCategoryName, &AuthoringSettings::editorCommentColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorPreprocessorColor("Preprocessor Color", sScriptColorsCategoryName, &AuthoringSettings::editorPreprocessorColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorKeywordColor("Keyword Color", sScriptColorsCategoryName, &AuthoringSettings::editorKeywordColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorErrorColor("Error Color", sScriptColorsCategoryName, &AuthoringSettings::editorErrorColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorWarningColor("Warning Color", sScriptColorsCategoryName, &AuthoringSettings::editorWarningColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_EditorFindSelectionBackgroundColor("Find Selection Background Color", sScriptColorsCategoryName, &AuthoringSettings::editorFindSelectionBackgroundColor);

// General
RBX::Reflection::BoundProp<bool> prop_AlwaysSaveScriptChangesWhileRunning("Always Save Script Changes", "General", &AuthoringSettings::alwaysSaveScriptChangesWhileRunning);
RBX::Reflection::EnumPropDescriptor<AuthoringSettings,AuthoringSettings::ListDisplayMode> prop_BasicObjectsDisplayMode("Basic Objects Display Mode","General",&AuthoringSettings::getBasicObjectsDisplayMode,&AuthoringSettings::setBasicObjectsDisplayMode);
RBX::Reflection::BoundProp<int> prop_MaximumOutputLines("Maximum Output Lines", "General", &AuthoringSettings::maximumOutputLines);
RBX::Reflection::EnumPropDescriptor<AuthoringSettings,AuthoringSettings::OutputLayoutMode> prop_OutputLayoutMode("Output Layout Mode", "General", &AuthoringSettings::getOutputLayoutMode, &AuthoringSettings::setOutputLayoutMode);
RBX::Reflection::BoundProp<int> prop_RenderThrottlePercentage("Render Throttle Percentage", "General", &AuthoringSettings::renderThrottlePercentage);
RBX::Reflection::EnumPropDescriptor<AuthoringSettings, AuthoringSettings::UIStyle> prop_UIStyle("UI Style", "General", &AuthoringSettings::getUIStyle, &AuthoringSettings::setUIStyle);
RBX::Reflection::BoundProp<bool> prop_ClearOuputOnStart("Clear Output On Start", "General", &AuthoringSettings::clearOutputOnStart);

// Directories
RBX::Reflection::BoundProp<QDir> prop_DefaultScriptFileDir("DefaultScriptFileDir", "Directories", &AuthoringSettings::defaultScriptFileDir);
RBX::Reflection::BoundProp<QDir> prop_PluginsDir("PluginsDir", "Directories", &AuthoringSettings::pluginsDir);
RBX::Reflection::PropDescriptor<AuthoringSettings, QDir> prop_CoreScriptsDir("OverrideCoreScriptsDir", "Directories", &AuthoringSettings::getCoreScriptsDir, &AuthoringSettings::setCoreScriptsDir);
RBX::Reflection::PropDescriptor<AuthoringSettings, bool> prop_useCoreScriptsDir("OverrideCoreScripts", "Directories",  &AuthoringSettings::getOverrideCoreScripts, &AuthoringSettings::setOverrideCoreScripts);
RBX::Reflection::BoundProp<QDir> prop_RecentSavesDir("RecentSavesDir", "Directories", &AuthoringSettings::recentSavesDir);

// Colors
RBX::Reflection::BoundProp<G3D::Color3> prop_SelectColor("Select Color", "Colors", &AuthoringSettings::selectColor);
RBX::Reflection::BoundProp<G3D::Color3> prop_HoverOverColor("Hover Over Color", "Colors", &AuthoringSettings::hoverOverColor);
RBX::Reflection::PropDescriptor<AuthoringSettings, G3D::Color3> prop_PrimaryPartSelectColor("Select/Hover Color", "Primary Part", &AuthoringSettings::getPrimaryPartSelectColor, &AuthoringSettings::setPrimaryPartSelectColor);
RBX::Reflection::PropDescriptor<AuthoringSettings, float> prop_PrimaryPartLineThickness("Line Thickness", "Primary Part", &AuthoringSettings::getPrimaryPartLineThickness, &AuthoringSettings::setPrimaryPartLineThickness);

// Undo
RBX::Reflection::EnumPropDescriptor<AuthoringSettings, RBX::ChangeHistoryService::RuntimeUndoBehavior> prop_RuntimeUndoBehavior("RuntimeUndoBehavior", "Undo", &AuthoringSettings::getRuntimeUndoBehavior, &AuthoringSettings::setRuntimeUndoBehavior);

// Camera
RBX::Reflection::PropDescriptor<AuthoringSettings, float> prop_CameraKeyMoveSpeed("Camera Speed", "Camera", &AuthoringSettings::getCameraKeyMoveSpeed, &AuthoringSettings::setCameraKeyMoveSpeed);
RBX::Reflection::PropDescriptor<AuthoringSettings, float> prop_CameraShiftMoveSpeed("Camera Shift Speed", "Camera", &AuthoringSettings::getCameraShiftMoveSpeed, &AuthoringSettings::setCameraShiftMoveSpeed);
RBX::Reflection::PropDescriptor<AuthoringSettings, float> prop_CameraMouseWheelMoveSpeed("Camera Mouse Wheel Speed", "Camera", &AuthoringSettings::getCameraMouseWheelMoveSpeed, &AuthoringSettings::setCameraMouseWheelMoveSpeed);

// Tools
RBX::Reflection::PropDescriptor<AuthoringSettings, bool> prop_showDraggerGrid("Show Dragger Grid", "Tools", &AuthoringSettings::getShowDraggerGrid, &AuthoringSettings::setShowDraggerGrid);
RBX::Reflection::PropDescriptor<AuthoringSettings, bool> prop_showHoverOver("Show Hover Over", "Tools", &AuthoringSettings::getShowHoverOver, &AuthoringSettings::setShowHoverOver);
RBX::Reflection::BoundProp<bool> prop_AnimateHoverOver("Animate Hover Over", "Tools", &AuthoringSettings::animateHoverOver);
RBX::Reflection::EnumPropDescriptor<AuthoringSettings,AuthoringSettings::HoverAnimateSpeed> prop_HoverAnimateSpeed("Hover Animate Speed","Tools",&AuthoringSettings::getHoverAnimateSpeed,&AuthoringSettings::setHoverAnimateSpeed);

// Lua Debugger
RBX::Reflection::PropDescriptor<AuthoringSettings, bool> prop_LuaDebuggerEnabled("LuaDebuggerEnabled", "Lua Debugger", &AuthoringSettings::getLuaDebuggerEnableState, &AuthoringSettings::setLuaDebuggerEnableState);

// Mobile Deployment
RBX::Reflection::PropDescriptor<AuthoringSettings, int> prop_DeploymentPairing("Device Pairing Code", "Device Deployment", &AuthoringSettings::getDeviceDeploymentPairingCode, &AuthoringSettings::setDeviceDeploymentPairingCode);

// Advanced
RBX::Reflection::BoundProp<bool> prop_diagnosticsBarEnabled("Show Diagnostics Bar", "Advanced", &AuthoringSettings::diagnosticsBarEnabled);
RBX::Reflection::BoundProp<bool> prop_intellisenseEnabled("Enable Intellisense", "Advanced", &AuthoringSettings::intellisenseEnabled);
RBX::Reflection::PropDescriptor<AuthoringSettings, bool> prop_dragMultiPartsAsSinglePartEnabled("Drag Multiple Parts As Single Part", "Advanced", &AuthoringSettings::getDragMutliplePartsAsSinglePart, &AuthoringSettings::setDragMutliplePartsAsSinglePart);

// Audio
RBX::Reflection::EnumPropDescriptor<AuthoringSettings,AuthoringSettings::TestServerAudioBehavior> prop_testServerAudioBehavior("Server Audio Behavior","Audio",&AuthoringSettings::getTestServerAudioBehavior,&AuthoringSettings::setTestServerAudioBehavior);
RBX::Reflection::BoundProp<bool> prop_onlyPlayAudioInFocus("Only Play Audio from Window in Focus", "Audio", &AuthoringSettings::onlyPlayFocusWindowAudio);

namespace RBX {
namespace Reflection {
	template<>
	EnumDesc<AuthoringSettings::PermissionLevelShown>::EnumDesc()
		:EnumDescriptor("PermissionLevelShown")
	{
		addPair(AuthoringSettings::Game,         "Game" );
		addPair(AuthoringSettings::RobloxGame,   "RobloxGame" );
		addPair(AuthoringSettings::RobloxScript, "RobloxScript" );
		addPair(AuthoringSettings::Studio,       "Studio" );
		addPair(AuthoringSettings::Roblox,       "Roblox" );
	}
}}

namespace RBX {
namespace Reflection {
	template<>
	EnumDesc<AuthoringSettings::HoverAnimateSpeed>::EnumDesc()
		:EnumDescriptor("HoverAnimateSpeed")
	{
        addPair(AuthoringSettings::VerySlow,    "VerySlow" );
		addPair(AuthoringSettings::Slow,        "Slow" );
		addPair(AuthoringSettings::Medium,      "Medium" );
		addPair(AuthoringSettings::Fast,        "Fast" );
		addPair(AuthoringSettings::VeryFast,    "VeryFast" );
	}
}}

namespace RBX {
namespace Reflection {
	template<>
	EnumDesc<AuthoringSettings::ListDisplayMode>::EnumDesc()
		:EnumDescriptor("ListDisplayMode")
	{
        addPair(AuthoringSettings::Horizontal,  "Horizontal" );
        addPair(AuthoringSettings::Vertical,    "Vertical" );
	}
}}

namespace RBX {
    namespace Reflection {
        template<>
        EnumDesc<AuthoringSettings::OutputLayoutMode>::EnumDesc()
            :EnumDescriptor("OutputLayoutMode")
        {
            addPair(AuthoringSettings::OutputLayoutHorizontal, "Horizontal");
            addPair(AuthoringSettings::OutputLayoutVertical, "Vertical");
        }
    }}

namespace RBX {
    namespace Reflection {
	template<>
	EnumDesc<AuthoringSettings::UIStyle>::EnumDesc()
		:EnumDescriptor("UIStyle")
	{
		addPair(AuthoringSettings::Ribbon, 		"RibbonBar");
		addPair(AuthoringSettings::Menu, 		"SystemMenu");
	}
	}}

namespace RBX {
	namespace Reflection {
		template<>
		EnumDesc<AuthoringSettings::TestServerAudioBehavior>::EnumDesc()
			:EnumDescriptor("ServerAudioBehavior")
		{
			addPair(AuthoringSettings::Enabled,		"Enabled");
			addPair(AuthoringSettings::Muted,		"Muted");
			addPair(AuthoringSettings::OnlineGame,	"OnlineGame");
		}
	}}

AuthoringSettings::AuthoringSettings()
	:showDeprecated(false)
	,permissionLevelShown(Game)
	,defaultScriptFileDir(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/ROBLOX/Scripts")
    ,pluginsDir(AppSettings::instance().tempLocation() + "/Plugins")
	,modelPluginsDir(AppSettings::instance().tempLocation() + "/InstalledPlugins")
	,coreScriptsDir(AppSettings::instance().tempLocation() + "/CoreScriptOverrides")
	,recentSavesDir(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/ROBLOX/RecentSaves")
	,overrideCoreScripts(false)
    ,basicObjectsDisplayMode(Vertical)
    ,maximumOutputLines(5000)
    ,outputLayoutMode(OutputLayoutVertical)
    ,renderThrottlePercentage(75)
	,alwaysSaveScriptChangesWhileRunning(false)
	,clearOutputOnStart(true)
	,doSettingsChangedGAEvents(false)
    // Script Editor
#ifdef Q_WS_WIN32
    ,editorFont("Courier New",10)
#else
    ,editorFont("Courier New",14)
#endif
    ,editorTabWidth(4)
    ,editorAutoIndent(true)
    ,editorTextWrap(false)
    ,editorTextColor(G3D::Color3::black())
    ,editorBackgroundColor(G3D::Color3::white())
    ,editorSelectionColor(G3D::Color3::white())
    ,editorSelectionBackgroundColor(G3D::Color3uint8(0x6E,0xA1,0xF1))
    ,editorOperatorColor(G3D::Color3uint8(0x7F,0x7F,0x00))
    ,editorNumberColor(G3D::Color3uint8(0x00,0x7F,0x7F))
    ,editorStringColor(G3D::Color3uint8(0x7F,0x00,0x7F))
    ,editorCommentColor(G3D::Color3uint8(0x00,0x7F,0x00))
    ,editorPreprocessorColor(G3D::Color3uint8(0x7F,0x00,0x00))
    ,editorKeywordColor(G3D::Color3uint8(0x00,0x00,0x7F))
    ,editorErrorColor(G3D::Color3::red())
    ,editorWarningColor(G3D::Color3::blue())
	,editorFindSelectionBackgroundColor(G3D::Color3uint8(0xF6, 0xB9, 0x3F))
    
    // HoverOver
    ,hoverOverColor(RBX::Draw::hoverOverColor().rgb())
    ,selectColor(RBX::Draw::selectColor().rgb())
    ,animateHoverOver(true)
    ,hoverAnimateSpeed(Slow)

    // AutoSave
    , autoSaveEnabled(true)
    , autoSaveMinutesInterval(5)
    , autoSaveDir(QDesktopServices::storageLocation(QDesktopServices::DocumentsLocation) + "/ROBLOX/AutoSaves")
    // Advanced
    , diagnosticsBarEnabled(false)
	, intellisenseEnabled(true)
	//Lua debugger
	, luaDebuggerEnabled(true)	
	// Ribbon
	,uiStyle(AuthoringSettings::Ribbon)
    // Device Deployment
    ,deploymentPairingCode(0)
	// Audio Settings
	, onlyPlayFocusWindowAudio(true)
	, testServerAudioBehavior(AuthoringSettings::OnlineGame)
	, dragMultiPartsAsSinglePart(false)
{}

/**
 * Configure settings after the fast flags have loaded to do any special adjustments/handling.
 *  Must be called after RBX::GlobalAdvancedSettings::singleton()->loadState() has been called.
 */
void AuthoringSettings::configureBasedOnFastFlags()
{
	FFlag::LuaDebugger = luaDebuggerEnabled;

	// make sure we do not enable break on error if there's no debugger
	if (!FFlag::LuaDebugger)
		FFlag::LuaDebuggerBreakOnError = false;

	if (!FFlag::StudioUseDraggerGrid)
		prop_showDraggerGrid.setEditable(false);

	setDragMutliplePartsAsSinglePart(dragMultiPartsAsSinglePart);
}

void AuthoringSettings::onPropertyChanged(const RBX::Reflection::PropertyDescriptor& pDescriptor)
{
	if (FFlag::StudioSettingsGAEnabled && doSettingsChangedGAEvents)
	{
		std::string description = pDescriptor.name.str;
		description += " On Edit";
		std::string valueString = pDescriptor.getStringValue(this);
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO_SETTINGS, description.c_str(), valueString.c_str());
	}

	Instance::onPropertyChanged(pDescriptor);
}

void AuthoringSettings::setDoSettingsChangedGAEvents(bool value)
{
	doSettingsChangedGAEvents = value;
}

void AuthoringSettings::setRuntimeUndoBehavior( RBX::ChangeHistoryService::RuntimeUndoBehavior value )
{
	RBX::ChangeHistoryService::runtimeUndoBehavior =  value;
}

RBX::Security::Permissions AuthoringSettings::getPermissionShown() const
{
	switch (permissionLevelShown)
	{
	default:
		RBXASSERT(false);
	case Game:
		return RBX::Security::None;
	case RobloxGame:
		return RBX::Security::RobloxPlace;
	case RobloxScript:
		return RBX::Security::RobloxScript;
	case Studio:
		return RBX::Security::LocalUser;
	case Roblox:
		return RBX::Security::Roblox;
	}
}

void AuthoringSettings::setPermissionLevelShown( PermissionLevelShown value )
{
	if (value == permissionLevelShown)
		return;
	permissionLevelShown = value;
	raiseChanged(prop_PermissionLevelShown);
}

float AuthoringSettings::getCameraKeyMoveSpeed() const
{
	return RBX::Camera::CameraKeyMoveFactor;
}

void AuthoringSettings::setCameraKeyMoveSpeed(float value)
{
	RBX::Camera::CameraKeyMoveFactor = value;
	raiseChanged(prop_CameraKeyMoveSpeed);
}

float AuthoringSettings::getCameraShiftMoveSpeed() const
{
	return RBX::Camera::CameraShiftKeyMoveFactor;
}

void AuthoringSettings::setCameraShiftMoveSpeed(float value)
{
	RBX::Camera::CameraShiftKeyMoveFactor = value;
	raiseChanged(prop_CameraShiftMoveSpeed);
}

float AuthoringSettings::getCameraMouseWheelMoveSpeed() const
{
	return RBX::Camera::CameraMouseWheelMoveFactor;
}

void AuthoringSettings::setCameraMouseWheelMoveSpeed(float value)
{
	RBX::Camera::CameraMouseWheelMoveFactor = value;
	raiseChanged(prop_CameraMouseWheelMoveSpeed);
}

bool AuthoringSettings::getShowDraggerGrid() const
{
	return RBX::ArrowToolBase::showDraggerGrid;
}

void AuthoringSettings::setShowDraggerGrid(bool value)
{
	RBX::ArrowToolBase::showDraggerGrid = value;
	raiseChanged(prop_showDraggerGrid);
}

bool AuthoringSettings::getShowHoverOver() const
{
    return RBX::Draw::isHoverOver();
}

void AuthoringSettings::setShowHoverOver(bool value)
{
	RBX::Draw::showHoverOver(value);
	raiseChanged(prop_showHoverOver);
}

void AuthoringSettings::setHoverAnimateSpeed(HoverAnimateSpeed speed)
{ 
    hoverAnimateSpeed = speed; 
    raiseChanged(prop_HoverAnimateSpeed);
}

void AuthoringSettings::setBasicObjectsDisplayMode(ListDisplayMode mode)
{
    basicObjectsDisplayMode = mode;
    raiseChanged(prop_BasicObjectsDisplayMode);
}

void AuthoringSettings::setEditorFont(const QFont& font)
{
    editorFont = font;
    raiseChanged(prop_EditorFont);
}

void AuthoringSettings::setOutputLayoutMode(OutputLayoutMode mode)
{
    outputLayoutMode = mode;
    raiseChanged(prop_OutputLayoutMode);
}

bool AuthoringSettings::getLuaDebuggerEnableState() const
{ 
	return luaDebuggerEnabled; 
}

void AuthoringSettings::setLuaDebuggerEnableState(bool state)
{ 
	luaDebuggerEnabled = state; 
}

int AuthoringSettings::getDeviceDeploymentPairingCode() const
{
    return deploymentPairingCode;
}

void AuthoringSettings::setDeviceDeploymentPairingCode(int newCode)
{
	if (newCode <= 0)
	{
		raiseChanged(prop_DeploymentPairing);
	}
    else if (deploymentPairingCode != newCode)
    {
        int sizeInDecimal = MobileDevelopmentDeployer::getNumOfDigits(newCode);

		//forcing deploymentPairingCode to only have 4 digits
		deploymentPairingCode = newCode * qPow(10, MobileDevelopmentDeployer::forcedPairCodeSize() - sizeInDecimal);
        raiseChanged(prop_DeploymentPairing);
    }
}

void AuthoringSettings::setUIStyle( UIStyle value )
{
	if (value == uiStyle)
		return;
	uiStyle = value;
	raiseChanged(prop_UIStyle);
}

AuthoringSettings::UIStyle AuthoringSettings::getUIStyle() const
{
	return uiStyle; 
}

void AuthoringSettings::setOverrideCoreScripts(bool value)
{
	if (value != overrideCoreScripts)
	{
		overrideCoreScripts = value;
		RBX::BaseScript::adminScriptsPath = (overrideCoreScripts) ? coreScriptsDir.absolutePath().toStdString() : "";
		raiseChanged(prop_useCoreScriptsDir);
	}
}

void AuthoringSettings::setCoreScriptsDir(QDir value)
{
	if (value != coreScriptsDir)
	{
		coreScriptsDir = value;
		if (overrideCoreScripts)
		{
			RBX::BaseScript::adminScriptsPath = coreScriptsDir.absolutePath().toStdString();
		}
		raiseChanged(prop_CoreScriptsDir);
	}
}

void AuthoringSettings::setTestServerAudioBehavior(TestServerAudioBehavior value)
{
	if (value == testServerAudioBehavior)
		return;
	testServerAudioBehavior = value;
	raiseChanged(prop_testServerAudioBehavior);
}

G3D::Color3 AuthoringSettings::getPrimaryPartSelectColor() const
{ return RBX::ModelInstance::primaryPartSelectColor().rgb(); }

void AuthoringSettings::setPrimaryPartSelectColor(G3D::Color3 color)
{
	// as of now we are using same color for selection and hover over
	RBX::ModelInstance::setPrimaryPartSelectColor(color);
	RBX::ModelInstance::setPrimaryPartHoverOverColor(color);
}

G3D::Color3 AuthoringSettings::getPrimaryPartHoverOverColor() const
{ return RBX::ModelInstance::primaryPartHoverOverColor().rgb(); }

void AuthoringSettings::setPrimaryPartHoverOverColor(G3D::Color3 color)
{ RBX::ModelInstance::setPrimaryPartHoverOverColor(color); }

float AuthoringSettings::getPrimaryPartLineThickness() const
{ return RBX::ModelInstance::primaryPartLineThickness(); }

void AuthoringSettings::setPrimaryPartLineThickness(float thickness)
{
	if (thickness > 0.0f && thickness < 0.05f)
	{
		RBX::ModelInstance::setPrimaryPartLineThickness(thickness);
		return;
	}

	RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Invalid line thickness. Input value must be less than 0.05");
}

bool AuthoringSettings::getDragMutliplePartsAsSinglePart() const
{ return dragMultiPartsAsSinglePart;}

void AuthoringSettings::setDragMutliplePartsAsSinglePart(bool state)
{ 
	dragMultiPartsAsSinglePart = state;
	RBX::AdvRunDragger::dragMultiPartsAsSinglePart = state; 
}