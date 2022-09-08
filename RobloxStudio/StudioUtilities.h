/**
 * StudioUtilities.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// 3rd Party Headers
#include "boost/shared_ptr.hpp"

// Qt Headers
#include <QString>
#include <vector>

class EntityProperties;

namespace RBX
{
	class DataModel;
	class Game;
	class Workspace;
	class PartInstance;
    class Instance;
}

namespace StudioUtilities
{
	//Studio only command arguments
	static const char* StudioWidthArgument	= "-studioWidth";		// Force Studio to a width
	static const char* StudioHeightArgument	= "-studioHeight";		// Force Studio to a height
	static const char* EmulateTouchArgument	= "-touchEmulation";	// Emulate touch controls in Studio player and Play Solo

    bool isFirstTimeOpeningStudio();
    void setIsFirstTimeOpeningStudio(bool value);

	bool isConnectedToNetwork();

	bool isAvatarMode();
    void setAvatarMode(bool isAvatarMode);

    bool isTestMode();
    void setTestMode(bool isTestMode);

	void setScreenShotUploading(bool state);
	bool isScreenShotUploading();

	void setVideoUploading(bool state);
	bool isVideoUploading();

	void setVideoFileName(const std::string &fileName);
	std::string getVideoFileName();

	bool containsEditScript(const QString& url);
	bool containsJoinScript(const QString& url);
	bool containsVisitScript(const QString& url);
	bool containsGameServerScript(const QString& url);

	std::string authenticate(std::string &domain, std::string &url, std::string &ticket);
	void executeURLJoinScript(boost::shared_ptr<RBX::Game> pGame, std::string urlScript);
	void executeURLScript(boost::shared_ptr<RBX::DataModel> pDataModel, std::string urlScript);
	void reportCloudEditJoinEvent(const char* label);
	void joinCloudEditSession(boost::shared_ptr<RBX::Game> pGame, boost::shared_ptr<EntityProperties> config);

	void startMobileDevelopmentDeployer();
    void stopMobileDevelopmentDeployer();

    void convertPhysicalPropertiesIfNeeded(std::vector<boost::shared_ptr<RBX::Instance> > instances, RBX::Workspace* workspace);

	void insertModel(boost::shared_ptr<RBX::DataModel> pDataModel, QString fileName, bool insertInto);

	/**
	 * Inserts a script instance under the currently selected instance
	 * @param fileName   code for script instance will be read from this file and embedded into script instance
	 */
	void insertScript(boost::shared_ptr<RBX::DataModel> pDataModel, const QString &fileName);

	/**
	 * Gets debug information file (external file in which breakpoint, watch etc. related information is stored)
	 * @param fileName   file for which debug information file is required
	 * @param debuggerFileExt extension to be added with the debug info file name
	 */
	QString getDebugInfoFile(const QString& fileName, const QString& debuggerFileExt = QString());

	/**
	 * Checks whether we are connected to network and session is authenticated
	 * @param openStartPage   if true then open start page in case user is not authenticated
	 */
	bool checkNetworkAndUserAuthentication(bool openStartPage = true);

	int translateKeyModifiers(Qt::KeyboardModifiers state, const QString &text);
}
