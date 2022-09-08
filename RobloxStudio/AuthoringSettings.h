/**
 * AuthoringSettings.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QFont>
#include <QDir>

// Roblox Headers
#include "v8datamodel/GlobalSettings.h"
#include "v8datamodel/ChangeHistory.h"
#include "ReflectionMetadata.h"

// Script properties must be in these categories
extern const char* const sScriptCategoryName;
extern const char* const sScriptColorsCategoryName;

/**
 * This class holds settings used by game authors, especially script-related settings.
 * permissionLevelShown is used by the Object Browser to filter descriptors based on 
 * permission level. By default we only show APIs that Game Scripts can show.
 * TODO: Consider moving isShown into a class that isn't called "Settings"
 **/
extern const char* const sAuthoringSettings;
class AuthoringSettings : public RBX::GlobalAdvancedSettingsItem<AuthoringSettings, sAuthoringSettings>
{
public:
	
    AuthoringSettings(void);
    void configureBasedOnFastFlags();
	void setDoSettingsChangedGAEvents(bool);
	
	typedef enum { 
		Game, 
		RobloxGame,
		RobloxScript,
		Studio,
		Roblox			
	} PermissionLevelShown;
	PermissionLevelShown permissionLevelShown;

    typedef enum
    {
        VerySlow,
        Slow,
        Medium,
        Fast,
        VeryFast,
    } HoverAnimateSpeed;
    HoverAnimateSpeed hoverAnimateSpeed;

    typedef enum 
    {
        Horizontal,
        Vertical,        
    } ListDisplayMode;
    ListDisplayMode basicObjectsDisplayMode;

    typedef enum
    {
        OutputLayoutHorizontal,
        OutputLayoutVertical,
    } OutputLayoutMode;
    OutputLayoutMode outputLayoutMode;

	typedef enum
	{
		Ribbon,
		Menu
	} UIStyle;

	typedef enum
	{
		Enabled,
		Muted,
		OnlineGame
	} TestServerAudioBehavior;

	bool showDeprecated;
	
	QDir    defaultScriptFileDir;
	QDir    pluginsDir;
	QDir    modelPluginsDir;
	QDir    coreScriptsDir;
	QDir    recentSavesDir;
	bool	overrideCoreScripts;
    bool    alwaysSaveScriptChangesWhileRunning;
    int     maximumOutputLines;
    int     renderThrottlePercentage;
	bool    clearOutputOnStart;
    
    // Device Deployment
    int deploymentPairingCode;

    // AutoSave
    QDir    autoSaveDir;
    bool    autoSaveEnabled;
    int     autoSaveMinutesInterval;

    // Hover Over
    bool animateHoverOver;
    G3D::Color3 hoverOverColor;
    G3D::Color3 selectColor;

    // Script Editor
    QFont editorFont;
	int editorTabWidth;
    bool editorAutoIndent;
    bool editorTextWrap;

    // Script Editor Colors
    G3D::Color3 editorTextColor;
    G3D::Color3 editorBackgroundColor;
    G3D::Color3 editorSelectionColor;
    G3D::Color3 editorSelectionBackgroundColor;
    G3D::Color3 editorOperatorColor;
    G3D::Color3 editorNumberColor;
    G3D::Color3 editorStringColor;
    G3D::Color3 editorCommentColor;
    G3D::Color3 editorPreprocessorColor;
    G3D::Color3 editorKeywordColor;
    G3D::Color3 editorErrorColor;
    G3D::Color3 editorWarningColor;
	G3D::Color3 editorFindSelectionBackgroundColor;

	// Lua debugger
	bool luaDebuggerEnabled;

    //Diagnostics bar
    bool diagnosticsBarEnabled;

	//Intellisense
	bool intellisenseEnabled;

	//Audio
	bool onlyPlayFocusWindowAudio;

	TestServerAudioBehavior getTestServerAudioBehavior() const {return testServerAudioBehavior;}
	void setTestServerAudioBehavior(TestServerAudioBehavior value);

	// Ribbon or menu	
	UIStyle getUIStyle() const;
	void setUIStyle(UIStyle value);

	RBX::ChangeHistoryService::RuntimeUndoBehavior getRuntimeUndoBehavior() const { return RBX::ChangeHistoryService::runtimeUndoBehavior; }
	void setRuntimeUndoBehavior(RBX::ChangeHistoryService::RuntimeUndoBehavior value);

	RBX::Security::Permissions getPermissionShown() const;
	PermissionLevelShown getPermissionLevelShown() const { return permissionLevelShown; }
	void setPermissionLevelShown(PermissionLevelShown value);

	template<class DescriptorType>
	bool isShown(RBX::Reflection::Metadata::Class* metadata, const DescriptorType& descriptor)
	{
		if (descriptor.security > getPermissionShown())
			return false;
		if (RBX::Reflection::Metadata::Item::isDeprecated(metadata, descriptor) && !showDeprecated)
			return false;
		if (!metadata)
			return false;
        if (!metadata->getInsertable())
            return false;
		return metadata->isBrowsable();
	}

	template<class DescriptorType>
	bool isHiddenInBrowser(RBX::Reflection::Metadata::Item* metadata, const DescriptorType& descriptor)
	{
		if (descriptor.security > getPermissionShown())
			return true;
		if (RBX::Reflection::Metadata::Item::isDeprecated(metadata, descriptor) && !showDeprecated)
			return true;
		if (!metadata)
			return false;
		return !metadata->isBrowsable();
	}

	QDir getCoreScriptsDir() const { return coreScriptsDir; }
	void setCoreScriptsDir(QDir value);

	bool getOverrideCoreScripts() const { return overrideCoreScripts; }
	void setOverrideCoreScripts(bool value);

	void onPluginsDirChanged(const RBX::Reflection::PropertyDescriptor&);

	float getCameraKeyMoveSpeed() const;
	float getCameraShiftMoveSpeed() const;
	float getCameraMouseWheelMoveSpeed() const;
	void setCameraKeyMoveSpeed(float value);
	void setCameraShiftMoveSpeed(float value);
	void setCameraMouseWheelMoveSpeed(float value);

	bool getShowDraggerGrid() const;
	void setShowDraggerGrid(bool value);

    bool getShowHoverOver() const;
	void setShowHoverOver(bool value);
    HoverAnimateSpeed getHoverAnimateSpeed() const      { return hoverAnimateSpeed; }
    void setHoverAnimateSpeed(HoverAnimateSpeed speed);

    ListDisplayMode getBasicObjectsDisplayMode() const  { return basicObjectsDisplayMode; }
    void setBasicObjectsDisplayMode(ListDisplayMode mode);

    void setEditorFont(const QFont& font);

    OutputLayoutMode getOutputLayoutMode() const { return outputLayoutMode; }
    void setOutputLayoutMode(OutputLayoutMode mode);

	bool getLuaDebuggerEnableState() const;
	void setLuaDebuggerEnableState(bool state);
    
    int getDeviceDeploymentPairingCode() const;
	void setDeviceDeploymentPairingCode(int newCode);

	G3D::Color3 getPrimaryPartSelectColor() const;
	void setPrimaryPartSelectColor(G3D::Color3);

	G3D::Color3 getPrimaryPartHoverOverColor() const;
	void setPrimaryPartHoverOverColor(G3D::Color3);

	float getPrimaryPartLineThickness() const;
	void setPrimaryPartLineThickness(float thickness);

	bool getDragMutliplePartsAsSinglePart() const;
	void setDragMutliplePartsAsSinglePart(bool state);

private:
	void persistPluginsDir();
	UIStyle uiStyle;
	TestServerAudioBehavior testServerAudioBehavior;
	bool dragMultiPartsAsSinglePart;
	bool doSettingsChangedGAEvents;

protected:
	void onPropertyChanged(const RBX::Reflection::PropertyDescriptor& pDescriptor);
};
