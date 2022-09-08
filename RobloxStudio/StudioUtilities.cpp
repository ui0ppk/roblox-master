/**
 * StudioUtilities.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "StudioUtilities.h"

// Qt Headers
#include <QNetworkInterface>
#include <QFile>
#include <QFileInfo>
#include <QDir>

// 3rd Party Headers
#include "boost/iostreams/copy.hpp"

// Roblox Headers
#include "network/api.h"
#include "script/script.h"
#include "script/ScriptContext.h"
#include "util/FileSystem.h"
#include "util/Hash.h"
#include "util/RobloxGoogleAnalytics.h"
#include "util/RunStateOwner.h"
#include "util/Statistics.h"
#include "v8datamodel/Camera.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/ChangeHistory.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/PartInstance.h"
#include "v8datamodel/Tool.h"
#include "v8datamodel/Workspace.h"
#include "v8world/World.h"
#include "v8datamodel/Visit.h"
#include "v8xml/Serializer.h"
#include "v8xml/XmlSerializer.h"
#include "reflection/Type.h"
#include "Client.h"
#include "ClientReplicator.h"
#include "Network/Players.h"
#include "Marker.h"

// Roblox Studio Headers
#include "QtUtilities.h"
#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"
#include "MobileDevelopmentDeployer.h"
#include "RobloxUser.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "RobloxGameExplorer.h"
#include "RobloxSettings.h"

DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)
DYNAMIC_FASTFLAG(CloudEditGARespectsThrottling)
FASTFLAGVARIABLE(TeamCreateOptimizeRemoteSelection, false)

static const char* kCloudEditScriptUrlPrefix = "CloudEditPlace:";

namespace StudioUtilities
{
	static bool sIsVideoUploading = false;
	static std::string sVideoFileName("");
    static bool IsAvatarMode = false;
    static bool IsTestMode = false;
    static rbx::atomic<int> sNumScreenShotUploads;
    static bool IsFirstTimeOpeningStudio = false;

    bool isFirstTimeOpeningStudio()
    {
        return IsFirstTimeOpeningStudio;
    }

    void setIsFirstTimeOpeningStudio(bool value)
    {
        IsFirstTimeOpeningStudio = value;
    }

	bool isConnectedToNetwork()
	{
		QList<QNetworkInterface> ifaces = QNetworkInterface::allInterfaces();
		bool result = false;

		for (int i = 0; i < ifaces.count(); i++)
		{
			QNetworkInterface iface = ifaces.at(i);
			if ( iface.flags().testFlag(QNetworkInterface::IsUp)
				 && !iface.flags().testFlag(QNetworkInterface::IsLoopBack) )
			{

			#ifdef _DEBUG
				// details of connection
				qDebug() << "name:" << iface.name() << endl
						<< "ip addresses:" << endl
						<< "mac:" << iface.hardwareAddress() << endl;
			#endif

				// this loop is important
				for (int j=0; j<iface.addressEntries().count(); j++)
				{
			#ifdef _DEBUG
					qDebug() << iface.addressEntries().at(j).ip().toString()
							<< " / " << iface.addressEntries().at(j).netmask().toString() << endl;
			#endif

					// we have an interface that is up, and has an ip address
					// therefore the link is present

					// we will only enable this check on first positive,
					// all later results are incorrect
					if (result == false)
						result = true;
				}
			}
		}
		return result;
	}
	
	bool isAvatarMode()
	{ return IsAvatarMode; }

    void setAvatarMode(bool isAvatarMode)
    { IsAvatarMode = isAvatarMode; }

    bool isTestMode()
	{ return IsTestMode; }

    void setTestMode(bool isTestMode)
    { IsTestMode = isTestMode; }
	
	void setScreenShotUploading(bool state)
	{ state ? ++sNumScreenShotUploads : --sNumScreenShotUploads; }

	bool isScreenShotUploading()
	{ return sNumScreenShotUploads > 0; }
	
	void setVideoUploading(bool state)
	{	sIsVideoUploading = state; }

	bool isVideoUploading()
	{	return sIsVideoUploading; }

	void setVideoFileName(const std::string &fileName)
	{	sVideoFileName = fileName; }

	std::string getVideoFileName()
	{	return sVideoFileName; }

	bool containsEditScript(const QString& url)
	{
		return url.contains("edit.ashx", Qt::CaseInsensitive);
	}

	bool containsJoinScript(const QString& url)
	{
		return url.contains("join.ashx", Qt::CaseInsensitive);
	}

	bool containsVisitScript(const QString& url)
	{
		return url.contains("visit.ashx", Qt::CaseInsensitive);
	}
    
    bool containsGameServerScript(const QString& url)
    {
        return url.contains("gameserver.ashx", Qt::CaseInsensitive);
    }

	std::string authenticate(std::string &domain, std::string &url, std::string &ticket)
	{
		if(domain.empty() || url.empty() || ticket.empty())
			return "";

		try
		{
			// Post our ticket back to Roblox to suggest a re-authentication
			std::string result;
			{
				std::string compound = url;
				compound += "?suggest=";
				compound += ticket;

				RBX::Http http(compound.c_str());
#ifdef Q_WS_MAC
				http.setAuthDomain(::GetBaseURL());
#else
				http.additionalHeaders["RBXAuthenticationNegotiation:"] = ::GetBaseURL();
#endif

				//http.additionalHeaders["RBXAuthenticationNegotiation:"] = domain;
				http.get(result);
			}

			// The http content is the new ticket
			return result;
		}
		catch (std::exception& e)
		{
			//Report Error!
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
			return "";
		}
	}

	RBX::ProtectedString fetchAndValidateScript(RBX::DataModel* pDataModel, const std::string& urlScript)
	{
		if(!pDataModel)
			return RBX::ProtectedString();

		RBX::Security::Impersonator impersonate(RBX::Security::COM);

		std::ostringstream data;
		if (!RBX::ContentProvider::isUrl(urlScript))
			return RBX::ProtectedString();

		std::string dataString;
		{
			try 
			{
				RBX::DataModel::LegacyLock lock(pDataModel, RBX::DataModelJob::Write);
				std::auto_ptr<std::istream> stream(RBX::ServiceProvider::create<RBX::ContentProvider>(pDataModel)->getContent(RBX::ContentId(urlScript.c_str())));
				dataString = std::string(static_cast<std::stringstream const&>(std::stringstream() << stream->rdbuf()).str());
			}

			catch(std::exception& e)
			{
				RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "ContentProvider while executing url script failed because %s", e.what());
			}
		}

		RBX::ProtectedString verifiedSource;

		try
		{
			verifiedSource = RBX::ProtectedString::fromTrustedSource(dataString);
			RBX::ContentProvider::verifyScriptSignature(verifiedSource, true);
		}
		catch(std::bad_alloc&)
		{
			throw;
		}
		catch(std::exception& e)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Script signature verification failed -- %s", e.what());
			return RBX::ProtectedString();
		}

		return verifiedSource;
	}

	void executeURLJoinScript(boost::shared_ptr<RBX::Game> pGame, std::string urlScript)
	{
		RBXASSERT(containsJoinScript(QString::fromStdString(urlScript)));

		boost::shared_ptr<RBX::DataModel> pDataModel = pGame->getDataModel();
		if(!pDataModel)
			return;

		RBX::ProtectedString verifiedSource = fetchAndValidateScript(pDataModel.get(), urlScript);
		if (verifiedSource.empty())
			return;

		try
		{		
			RBX::DataModel::LegacyLock lock(pDataModel, RBX::DataModelJob::Write);		
			if (pDataModel->isClosed())
				return;

			std::string dataString = verifiedSource.getSource();
			int firstNewLineIndex = dataString.find("\r\n");
			if (dataString[firstNewLineIndex+2] == '{')
			{
				pGame->configurePlayer(RBX::Security::COM, dataString.substr(firstNewLineIndex+2));
				if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
				{
					QMetaObject::invokeMethod(ideDoc, "refreshDisplayName", Qt::QueuedConnection);
				}

				return;
			}

			boost::shared_ptr<RBX::ScriptContext> pContext = RBX::shared_from(pDataModel->create<RBX::ScriptContext>());
			if (pContext)
				pContext->executeInNewThread(RBX::Security::COM, verifiedSource, "Start Game");	
		}
		catch (std::exception& exp)
		{
			std::string s(verifiedSource.getSource());
			std::string msg = RBX::format("executeURLJoinScript() failed because %s.  %s", exp.what(), s.substr(0,64).c_str());
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, msg.c_str()); 
		}
	}

	void executeURLScript(boost::shared_ptr<RBX::DataModel> pDataModel, std::string urlScript)
	{
		if(!pDataModel)
			return;

		RBX::ProtectedString verifiedSource = fetchAndValidateScript(pDataModel.get(), urlScript);
		if (verifiedSource.empty())
			return;

		try
		{		
			RBX::DataModel::LegacyLock lock(pDataModel, RBX::DataModelJob::Write);		
			if (pDataModel->isClosed())
				return;

			boost::shared_ptr<RBX::ScriptContext> pContext = RBX::shared_from(pDataModel->create<RBX::ScriptContext>());
			if (pContext)
				pContext->executeInNewThread(RBX::Security::COM, verifiedSource, "Start Game");	
		}
		catch (std::exception& exp)
		{
			std::string s(verifiedSource.getSource());
			std::string msg = RBX::format("executeURLScript() failed because %s.  %s", exp.what(), s.substr(0,64).c_str());
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, msg.c_str()); 
		}
	}

	void reportCloudEditJoinEvent(const char* label)
	{
		if (DFFlag::CloudEditGARespectsThrottling)
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "CloudEdit", label);
		}
		else
		{
			RBX::RobloxGoogleAnalytics::trackEventWithoutThrottling(GA_CATEGORY_STUDIO, "CloudEdit", label);
		}
	}

	static void notifyCloudEditConnectionClosed(const char* category, std::string message)
	{
		std::string label = RBX::format("Connection Closed: %s | %s", category, message.c_str());
		reportCloudEditJoinEvent(label.c_str());
		QMetaObject::invokeMethod(&(UpdateUIManager::Instance().getMainWindow()),
			"notifyCloudEditConnectionClosed",
			Qt::QueuedConnection);
	}

	static void updateCameraLocation(RBX::DataModel* dm)
	{
		if (RBX::Network::Player* p = RBX::Network::Players::findLocalPlayer(dm))
		{
			p->setCloudEditCameraCoordinateFrame(dm->getWorkspace()->getCamera()->getCameraCoordinateFrame());
		}
	}

	static void doUpdateSelection(shared_ptr<bool> debouncer, RBX::DataModel* dm)
	{
		if (RBX::Network::Player* p = RBX::Network::Players::findLocalPlayer(dm))
		{
			if (RBX::Selection* s = dm->find<RBX::Selection>())
			{
				shared_ptr<RBX::Reflection::ValueArray> va(new RBX::Reflection::ValueArray);
				for (auto& i : *s)
				{
					va->push_back(i);
				}
				RBX::Network::Player::event_cloudEditSelectionChanged.replicateEvent(p, shared_ptr<const RBX::Reflection::ValueArray>(va));
			}
		}
		if (FFlag::TeamCreateOptimizeRemoteSelection)
		{
			*debouncer = false;
		}
	}

	static void updateSelection(shared_ptr<bool> debouncer, RBX::DataModel* dm)
	{
		if (FFlag::TeamCreateOptimizeRemoteSelection)
		{
			if (*debouncer) return;
			*debouncer = true;
			dm->submitTask(boost::bind(&doUpdateSelection, debouncer, _1), RBX::DataModelJob::Write);
		}
		else
		{
			doUpdateSelection(debouncer, dm);
		}
	}

	static void onConnectionAccepted(int placeId, std::string url, shared_ptr<RBX::Instance> replicator)
	{
		using namespace RBX;
		reportCloudEditJoinEvent("Connection Accepted");
		try 
		{
			boost::shared_ptr<Network::ClientReplicator> rep = Instance::fastSharedDynamicCast<Network::ClientReplicator>(replicator);
			rep->disconnectionSignal.connect(boost::bind(&notifyCloudEditConnectionClosed, "ReplicatorDisconnect", _1));
		}
		catch (RBX::base_exception& e)
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, "onConnectionAccepted failed: %s", e.what());
		}
	}

	void joinCloudEditSession(shared_ptr<RBX::Game> pGame, shared_ptr<EntityProperties> config)
	{
		using namespace RBX;
		try
		{
			boost::shared_ptr<DataModel> dataModel = pGame->getDataModel();
			if(!dataModel)
				return;

			DataModel::LegacyLock lock(dataModel, DataModelJob::Write);
			if (dataModel->isClosed())
				return;

			reportCloudEditJoinEvent("Attempt To Connect");

			Security::Impersonator impersonate(Security::COM);
			
			Http::gameID = config->get<std::string>("GameId").get();
			int userId = RobloxUser::singleton().getUserId();
			RBX::Analytics::setUserId(userId);

			int placeId = config->get<int>("PlaceId").get();
			dataModel->setPlaceID(placeId, false);
			dataModel->setUniverseId(config->get<int>("UniverseId").get());
			ScriptContext::propScriptsDisabled.setValue(dataModel->create<ScriptContext>(), true);

			dataModel->create<ContentProvider>()->setThreadPool(16);
			Network::Players* players = dataModel->create<Network::Players>();
			// creating a visit helps suppress saving behaviors that don't work well in CloudEdit
			dataModel->create<Visit>();
			if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
			{
				ideDoc->enableUndo(true);
				QMetaObject::invokeMethod(ideDoc, "updateFromPlaceID", Qt::QueuedConnection, Q_ARG(int, placeId));
				QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER),
					"openGameFromPlaceId", Qt::QueuedConnection, Q_ARG(int, placeId));
			}

			Network::Client* client = dataModel->create<Network::Client>();
			client->configureAsCloudEditClient();
			client->connectionAcceptedSignal.connect(boost::bind(&onConnectionAccepted, placeId, _1, _2));
			client->connectionRejectedSignal.connect(boost::bind(&notifyCloudEditConnectionClosed, "Rejected", _1));
			client->connectionFailedSignal.connect(boost::bind(&notifyCloudEditConnectionClosed, "Failed", _3));
			client->setTicket(config->get<std::string>("ClientTicket").get());

			client->setGameSessionID(config->get<std::string>("SessionId").get());

			shared_ptr<Network::Player> player = 
				Instance::fastSharedDynamicCast<Network::Player>(client->playerConnect(
				userId, config->get<std::string>("MachineAddress").get(),
				config->get<int>("ServerPort").get(), 0, -1));
			player->setName(config->get<std::string>("UserName").get());
			player->setCharacterAppearance(config->get<std::string>("CharacterAppearance").get());
			if (RunService* rs = dataModel->create<RunService>())
			{
				rs->heartbeatSignal.connect(boost::bind(&updateCameraLocation, dataModel.get()));
			}
			shared_ptr<bool> selectionUpdateDebouncer(new bool(false));
			dataModel->create<Selection>()->selectionChanged.connect(boost::bind(&updateSelection, selectionUpdateDebouncer, dataModel.get()));

			// creating a client will reset the mouse command to null, set it back to adv arrow here
			dataModel->getWorkspace()->setDefaultMouseCommand();
		}
		catch (const std::exception& e) 
		{
			StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while joining CloudEdit: %s", e.what()); 
			QtUtilities::RBXMessageBox failureBox;
			failureBox.setText("Exception while trying to join CloudEdit. See Output for details.");
			failureBox.exec();
		}
	}

	void startMobileDevelopmentDeployer()
	{
		MobileDevelopmentDeployer::singleton().startPlaySessionWithRbxDevDevices();
	}
    
    void stopMobileDevelopmentDeployer()
    {
        MobileDevelopmentDeployer::singleton().disconnectClientsAndShutDown();
    }

    void convertPhysicalPropertiesIfNeeded(std::vector<boost::shared_ptr<RBX::Instance> > instances, RBX::Workspace* workspace)
	{
		if (workspace->getUsingNewPhysicalProperties())
		{
			for (size_t i = 0; i < instances.size(); ++i) 
			{
				RBX::PartInstance::convertToNewPhysicalPropRecursive(instances[i].get());
			}
		}
		else 
		{
			for (size_t i = 0; i < instances.size(); i++)
			{
				if (RBX::PartInstance::instanceOrChildrenContainsNewCustomPhysics(instances[i].get())) 
				{
					if (RobloxIDEDoc::displayAskConvertPlaceToNewMaterialsIfInsertNewModel())
					{
						RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "PhysicalProperties: Insert - Conversion", "Workspace->PhysicalPropertiesMode_New");
						workspace->setPhysicalPropertiesMode(RBX::PhysicalPropertiesMode_NewPartProperties);
						RBX::PartInstance::convertToNewPhysicalPropRecursive(RBX::DataModel::get(workspace));
					}
					break;
				}
			}
		}
		
	}

	void insertModel(boost::shared_ptr<RBX::DataModel> pDataModel, QString fileName, bool insertInto)
	{
		if (fileName.isEmpty())
			return;

		if (!pDataModel)
			throw std::runtime_error("Can't insert at this time");

		QByteArray ba = fileName.toAscii();
		const char *c_str = ba.constData();
		
		std::ifstream stream(c_str, std::ios_base::in | std::ios_base::binary);

		// The following code happens BEFORE we do a lock because we can't display a message box within a lock
		RBX::Instances instances;
        Serializer().loadInstances(stream, instances);
		
		{
			// Lock the data model temporarily to make sure insertion is legal
			RBX::DataModel::LegacyLock lock(pDataModel, RBX::DataModelJob::Write);

			if (DFFlag::MaterialPropertiesEnabled)
			{
				convertPhysicalPropertiesIfNeeded(instances, pDataModel->getWorkspace());
			}

			if (RBX::ContentProvider* cp = pDataModel->create<RBX::ContentProvider>())
			{
				RBX::LuaSourceContainer::blockingLoadLinkedScriptsForInstances(cp, instances);
			}
		}

		RBX::PromptMode promptMode = RBX::SUPPRESS_PROMPTS;
		
		if (!insertInto){
			if (instances.size() == 1)
			{
				RBX::Instance* single = instances[0].get();
				if (dynamic_cast<RBX::Tool*>(single)) 
				{
					if( QtUtilities::RBXMessageBox::question(
                            &UpdateUIManager::Instance().getMainWindow(), 
                            "Insert Model", 
                            "Put this tool into the starter pack (otherwise drop into the 3d view)?", 
                            QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::Yes )
						promptMode = RBX::PUT_TOOL_IN_STARTERPACK;
				}
			}
		}
		
		// Now lock the datamodel (after MessageBox)
		RBX::DataModel::LegacyLock lock(pDataModel, RBX::DataModelJob::Write);
		
		shared_ptr<RBX::Workspace> spWorkspace = shared_from(pDataModel ? pDataModel->getWorkspace() : NULL);
		if(!spWorkspace)
			throw std::runtime_error("Can't insert at this time");
		
		RBX::Selection* sel = RBX::ServiceProvider::create<RBX::Selection>(spWorkspace.get());
		
		RBX::Instance* requestedParent = (insertInto && (sel->size() == 1))
		? sel->front().get()
		: spWorkspace.get();
		
		RBX::InsertMode insertMode = insertInto ? RBX::INSERT_TO_TREE : RBX::INSERT_TO_3D_VIEW;
		
		try
		{			
			spWorkspace->insertInstances(instances, requestedParent, insertMode, promptMode);

			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, insertInto ? "insertIntoFromFileComplete" : "insertModelComplete");
			
			boost::shared_ptr<RBX::RunService> runService = shared_from(pDataModel->create<RBX::RunService>());
			// If we're not playing a game then select the content
			if (runService->getRunState() == RBX::RS_STOPPED)
			{
				sel->setSelection(instances.begin(), instances.end());
			}
		}
		catch (std::exception&)
		{
			std::for_each(instances.begin(), instances.end(), boost::bind(&RBX::Instance::setParent, _1, (RBX::Instance*) NULL));
			throw;
		}
	}

	void insertScript(boost::shared_ptr<RBX::DataModel> pDataModel, const QString &fileName)
	{
		shared_ptr<RBX::Workspace> spWorkspace = shared_from(pDataModel ? pDataModel->getWorkspace() : NULL);
		if(!pDataModel || !spWorkspace)
			throw std::runtime_error("Script cannot be inserted at this time");

		QFile file(fileName);
		if (!file.open(QFile::ReadOnly | QFile::Text)) 
			throw std::runtime_error("Script file cannot be opened");
		
		RBX::DataModel::LegacyLock lock(pDataModel, RBX::DataModelJob::Write);

		shared_ptr<RBX::Script> pScriptInstance = RBX::Creatable<RBX::Instance>::create<RBX::Script>();
		if (!pScriptInstance)
			throw std::runtime_error("Script instance cannot be created");
		
		// set file name as script instance name
		pScriptInstance->setName(QFileInfo(fileName).completeBaseName().toStdString());

		// read and set code from file into script instance
		QTextStream in(&file);
		pScriptInstance->setEmbeddedCode(RBX::ProtectedString::fromTrustedSource(in.readAll().toAscii().data()));

		// set appropriate parent
		RBX::Selection* pSelection = RBX::ServiceProvider::create<RBX::Selection>(spWorkspace.get());
		RBX::Instance*  pRequestedParent = (pSelection && pSelection->size() == 1) ? pSelection->front().get() : spWorkspace.get();
		pScriptInstance->setParent(pRequestedParent);

		// select the created script instance
		if (pSelection)
			pSelection->setSelection(pScriptInstance.get());
	}

	QString getDebugInfoFile(const QString& fileName, const QString& debuggerFileExt)
	{
		QString debugInfoFile;
		if (!fileName.isEmpty())
		{
			static QString filePath = QtUtilities::qstringFromStdString(RBX::FileSystem::getUserDirectory(true, RBX::DirAppData, "debugger").native());
			// get hash of the file path
			unsigned int hash = RBX::Hash::hash(QFileInfo(fileName).absoluteFilePath().toStdString());
			// create debug file path with generated hash
			debugInfoFile = filePath;
			debugInfoFile.append(QString::number(hash));
			if (!debuggerFileExt.isEmpty())
				debugInfoFile.append(debuggerFileExt);
			debugInfoFile.append(".rdbg");
		}
		return debugInfoFile;
	}

	bool checkNetworkAndUserAuthentication(bool openStartPage)
	{
		if (!StudioUtilities::isConnectedToNetwork())
		{
			// we don't have network connect
			UpdateUIManager::Instance().showErrorMessage(QObject::tr("Publish to Roblox"), QObject::tr("Feature not available in offline mode.\nPlease check internet connection and restart Roblox."));
			return false;
		}

		if (RobloxUser::singleton().getUserId() <= 0)
		{
			UpdateUIManager::Instance().showErrorMessage(QObject::tr("Log in required"), QObject::tr("You must log in to perform this action!"));
			if (openStartPage)
            {
				UpdateUIManager::Instance().getMainWindow().openStartPage(true, "showlogin=True");
				UpdateUIManager::Instance().getMainWindow().setFocus();
            }
			return false;
		}

		return true;
	}

	int translateKeyModifiers(Qt::KeyboardModifiers state, const QString &text)
	{
		// Convert modifier combos to a single integer.
		int result = 0;
		if ((state & Qt::ShiftModifier) && (text.size() == 0 || !text.at(0).isPrint() || text.at(0).isLetter() || text.at(0).isSpace()))
			result |= Qt::SHIFT;
		if (state & Qt::ControlModifier)
			result |= Qt::CTRL;
		if (state & Qt::MetaModifier)
			result |= Qt::META;
		if (state & Qt::AltModifier)
			result |= Qt::ALT;
		return result;
	}
}
