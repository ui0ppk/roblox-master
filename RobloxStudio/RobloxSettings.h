/**
 * RobloxSettings.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QSettings>

class AppSettings 
{
public:
	static AppSettings& instance();

	QString contentFolder() { return m_contentFolder; }
#ifdef STUDIO_ADMIN_BUILD
	QString adminKey() { return m_adminKey; }
#endif
	QString builtInPluginsFolder() { return m_builtInPluginsFolder; }
	QString baseURL() { return m_baseURL; }
	QString tempLocation() { return m_tempLocation; }
	bool isScriptAssetUploadEnabled() { return m_bIsScriptAssetUploadEnabled; }
	bool isAnimationAssetUploadEnabled() { return m_bIsAnimationAssetUploadEnabled; }
	bool isImageModelAssetUploadEnabled() { return m_bIsImageModelAssetUploadEnabled; }
	bool showCrashMenu() { return m_bShowStudioCrashMenu; }
	
private:
	AppSettings();

	QString m_contentFolder;
	QString m_builtInPluginsFolder;
	QString m_baseURL;
	QString m_tempLocation;
#ifdef STUDIO_ADMIN_BUILD
	QString m_adminKey;
#endif
	bool m_bIsScriptAssetUploadEnabled;
	bool m_bIsAnimationAssetUploadEnabled;
	bool m_bIsImageModelAssetUploadEnabled;
	bool m_bShowStudioCrashMenu;
};

class RobloxSettings : public QSettings
{
Q_OBJECT
	
public:
	RobloxSettings();
	static void saveAssets();
	static void recoverAssets();
	static QString getAssetFolder();
	static QString getBuiltInPluginsFolder();
	static QString getResourcesFolder(); // Resources such as ReflectionMetadata.xml
	static QString getBaseURL();
	static QString getApiBaseURL();
#ifdef STUDIO_ADMIN_BUILD
	static QString getAdminKey();
#endif
	static QString getVersionString();
	static QString getTempLocation();
	static void initWorkspaceSettings();
	static bool showCrashMenu();	
private:
	RobloxSettings(const RobloxSettings&);                 // Prevent copy-construction
	RobloxSettings& operator=(const RobloxSettings&);      // Prevent assignment		
};	
