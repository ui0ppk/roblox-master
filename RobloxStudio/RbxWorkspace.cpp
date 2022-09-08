/**
 * RbxWorkspace.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RbxWorkspace.h"

// Qt Headers
#include <QMessageBox>
#include <QDrag>
#include <QMimeData>
#include <QUrl>
#include <QDir>
#include <QDesktopServices>
#include <QStringList>
#include <QWebPage>

// Roblox Headers
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Selection.h"
#include "v8datamodel/Workspace.h"
#include "v8datamodel/Commands.h"
#include "v8datamodel/ContentProvider.h"
#include "v8datamodel/Tool.h"
#include "v8datamodel/Decal.h"
#include "v8datamodel/Visit.h"
#include "v8datamodel/GameBasicSettings.h"
#include "V8DataModel/PartOperationAsset.h"
#include "v8datamodel/PlayerGui.h"
#include "v8datamodel/VehicleSeat.h"
#include "v8datamodel/KeyframeSequence.h"
#include "v8datamodel/KeyframeSequenceProvider.h"
#include "V8DataModel/NonReplicatedCSGDictionaryService.h"
#include "v8tree/Instance.h"
#include "v8tree/Service.h"
#include "v8xml/XmlSerializer.h"
#include "v8xml/Serializer.h"
#include "v8xml/SerializerBinary.h"
#include "v8xml/WebParser.h"
#include "Network/Players.h"
#include "Network/api.h"
#include "script/script.h"
#include "util/FileSystem.h"
#include "util/HttpAsync.h"

// Roblox Studio Headers
#include "RbxContent.h"
#include "RobloxMainWindow.h"
#include "RobloxPluginHost.h"
#include "RobloxToolBox.h"
#include "AuthenticationHelper.h"
#include "UpdateUIManager.h"
#include "RobloxGameExplorer.h"
#include "RobloxSettings.h"
#include "StudioUtilities.h"
#include "UpdateUIManager.h"
#include "RobloxStudioVerbs.h"
#include "RobloxIDEDoc.h"
#include "RobloxDocManager.h"
#include "RobloxWebDoc.h"
#include "WebDialog.h"
#include "CommonInsertWidget.h"
#include "RobloxView.h"

#include "Util/RobloxGoogleAnalytics.h"
#include "QtUtilities.h"

// Publish Selection to Roblox
bool RbxWorkspace::isScriptAssetUploadEnabled = false;
bool RbxWorkspace::isAnimationAssetUploadEnabled = false;
bool RbxWorkspace::isImageModelAssetUploadEnabled = false;

FASTFLAG(GoogleAnalyticsTrackingEnabled)
FASTFLAG(StudioCSGAssets)

FASTFLAGVARIABLE(StudioEnableGameAnimationsTab, false)
FASTFLAGVARIABLE(StudioRecordToolboxInsert, true)
FASTFLAGVARIABLE(StudioRemoveInsertEvent, false)
FASTFLAGVARIABLE(StudioExposeSessionID, true)

FASTINTVARIABLE(StudioPartSizeCameraDistRatio, 30)

//enable this code for mac when we have the video recording capability available
#ifdef Q_WS_WIN32
static const std::string sVideoTitlePrefix("<media:title type=\"plain\">");
static const std::string sVideoTitlePostfix("</media:title>");

class UploadVideoThread : public QThread
{
public:
	UploadVideoThread(boost::shared_ptr<RBX::DataModel> pDataModel, const std::string &videoFileName, const std::string &youtubeToken, 
					  const std::string &videoTitle, const std::string &videoSEOInfo, bool siteSEO);

	~UploadVideoThread();

	virtual void run();

private:
	boost::shared_ptr<RBX::DataModel> m_pDataModel;

	std::string m_videoFileName;
	std::string m_youtubeToken;
	std::string m_videoTitle;
	std::string m_videoSEOInfo;
	
	bool m_bSiteSEO;
};

class VideoUploadStatus
{
public:
	VideoUploadStatus()
	{	StudioUtilities::setVideoUploading(true); }

	~VideoUploadStatus()
	{	StudioUtilities::setVideoUploading(false); }
};
#endif

RbxWorkspace::RbxWorkspace( QObject *parent, RBX::DataModel *dm ) 
: QObject(parent)
, m_pDataModel(dm)
, m_pParent(parent)
, m_screenShotInProgressMsgBox(NULL)
{
	m_pContent = new RbxContent(this);
}

RbxWorkspace::~RbxWorkspace()
{
	delete m_pContent;
}

// Publish To Roblox
bool RbxWorkspace::SaveUrl(const QString &saveUrl)
{
	if (!AuthenticationHelper::Instance().verifyUserAndAuthenticate() || 
		!AuthenticationHelper::validateMachine())
		return false;
	
	try 
	{	
		std::string urlStr = saveUrl.toStdString();

		QString url(urlStr.c_str());
		url = url.mid(url.indexOf("assetid=") + 8);
		url = url.left(url.indexOf("&"));
		int placeId = url.toInt();

		RBX::HttpFuture universeFuture;
		if (placeId > 0)
		{
			universeFuture = RBX::HttpAsync::get(
				RBX::format("%s/universes/get-universe-containing-place?placeId=%d",
				RBX::ContentProvider::getUnsecureApiBaseUrl(RobloxSettings::getBaseURL().toStdString()).c_str(), placeId));
		}

		{
			RBX::DataModel::LegacyLock legacylock(m_pDataModel, RBX::DataModelJob::Write);
			RBX::Visit* visit = RBX::ServiceProvider::create<RBX::Visit>(m_pDataModel);
			visit->setUploadUrl(urlStr);  // Set the upload URL so that future publishes go to here as well.

			if (FFlag::StudioCSGAssets)
				RBX::PartOperationAsset::publishAll(m_pDataModel);

			m_pDataModel->save(RBX::ContentId(urlStr));

			if (placeId > 0)
			{
				m_pDataModel->setPlaceID(placeId, false);
				RobloxMainWindow::get(this)->getStudioAnalytics()->reportPublishStats(m_pDataModel);

				try
				{
					shared_ptr<const RBX::Reflection::ValueTable> v(new RBX::Reflection::ValueTable);
					RBX::WebParser::parseJSONTable(universeFuture.get(), v);
					int universeId = v->at("UniverseId").cast<int>();
					m_pDataModel->setUniverseId(universeId);
					if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
					{
						ideDoc->updateFromPlaceID(placeId);
					}
				}
				catch (...)
				{
				}

				QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER),
					"openGameFromPlaceId", Qt::QueuedConnection, Q_ARG(int, placeId));

				if (RobloxDocManager::Instance().getPlayDoc())
					QMetaObject::invokeMethod(RobloxDocManager::Instance().getPlayDoc(),
					"updateFromPlaceID", Qt::QueuedConnection, Q_ARG(int, placeId));
			}
		}

		if (!RobloxDocManager::Instance().getPlayDoc()->isLocalDoc())
			RobloxDocManager::Instance().getPlayDoc()->resetDirty(m_pDataModel);

		return true;
	}
	catch (std::exception&)
	{
		RobloxMainWindow::sendCounterEvent("QTStudioPublishSaveUrlFailure", false);

        if (FFlag::GoogleAnalyticsTrackingEnabled)
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "StudioPushSaveUrlFailure");
		}

		return false;
	}
}

bool RbxWorkspace::Save()
{
	if (!AuthenticationHelper::Instance().verifyUserAndAuthenticate() || 
		!AuthenticationHelper::validateMachine())
		return false;

	try 
	{
		RobloxIDEDoc* pIDEDoc = RobloxDocManager::Instance().getPlayDoc();
		if (!pIDEDoc)
			return false;

		RBX::Verb* pVerb = pIDEDoc->getVerb("PublishToRobloxVerb");
		PublishToRobloxVerb* pPublishVerb = dynamic_cast<PublishToRobloxVerb*>(pVerb);

		if (!pPublishVerb)
			return false;

		pPublishVerb->doIt(pIDEDoc->getDataModel());

		return true;
	}
	catch (std::exception&)
	{
		RobloxMainWindow::sendCounterEvent("QTStudioPublishSaveFailure", false);

        if (FFlag::GoogleAnalyticsTrackingEnabled)
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "StudioPublishSaveFailure");
		}

		return false;
	}
}

#ifdef ENABLE_EDIT_PLACE_IN_STUDIO
// To start edit of a place
bool RbxWorkspace::StartGame(const QString &ticket, const QString &url, const QString &script)
{
	static int errorCount = 0;
	Hide();
	bool result = UpdateUIManager::Instance().getMainWindow().handleFileOpen("", IRobloxDoc::IDE, script);
	if (result)
	{
		errorCount = 0;
		Close();
	}
	else
	{
		Show();
		errorCount++;
		QMessageBox::critical(
			NULL, 
			tr("ROBLOX Studio"), 
			errorCount <= 2 ? 
				tr("There was a problem opening your place.  Please try again.")
				: tr("Oh no!  It seems we're still unable to open this place.  Please restart your application and try again.  If you continue to encounter this error, please contact customer service.")
			);
	}
	return result;
}
#endif

void RbxWorkspace::InstallPlugin(int assetId, int assetVersion)
{
	QString url = QString("%1/asset/?id=%2").arg(RobloxSettings::getBaseURL()).arg(assetId);
	RBX::Http http(qPrintable(url));
	boost::function<void(bool)> completionCallback = 
		boost::bind(&RbxWorkspace::pluginInstallCompleteHelper, 
			boost::weak_ptr<RbxWorkspace>(shared_from_this()), _1, assetId);
	http.get(boost::bind(RobloxPluginHost::processInstallPluginFromWebsite,
		assetId, assetVersion, _1, _2, completionCallback));
}

QString RbxWorkspace::GetInstalledPlugins()
{
	return RobloxPluginHost::getPluginMetadataJson();
}

void RbxWorkspace::pluginInstallCompleteHelper(weak_ptr<RbxWorkspace> weakWorkspace, bool succeeded, int assetId)
{
	if (shared_ptr<RbxWorkspace> workspace = weakWorkspace.lock())
	{
		Q_EMIT workspace->PluginInstallComplete(succeeded, assetId);
	}
}

void RbxWorkspace::SetPluginEnabled(int assetId, bool enabled)
{
	RobloxPluginHost::setPluginEnabled(assetId, enabled);
}

void RbxWorkspace::UninstallPlugin(int assetId)
{
	RobloxPluginHost::uninstallPlugin(assetId);
}

// Publish Selection To Roblox
QObject* RbxWorkspace::WriteSelection()
{
	if (!AuthenticationHelper::Instance().verifyUserAndAuthenticate() || 
		!AuthenticationHelper::validateMachine())
	{
		return NULL;
	}

	delete m_pContent;
	m_pContent = new RbxContent(this);

	try 
	{
		shared_ptr<RBX::Script> script;
		if (RbxWorkspace::isScriptAssetUploadEnabled)
		{
			RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Read);
			RBX::Selection* sel = RBX::ServiceProvider::find<RBX::Selection>(m_pDataModel);
			if (sel && sel->size()==1)
				script = RBX::Instance::fastSharedDynamicCast<RBX::Script>(sel->front());
		}

		if (script)
		{
			RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Read);
			if (script->isCodeEmbedded())
				m_pContent->data << script->getEmbeddedCode().get().getSource();
			else
			{
				RBX::BaseScript::Code code = script->requestCode();
				if (code.loaded)
					m_pContent->data << code.script.get().getSource();
			}			
		}
		else
		{
			RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);

            if (FFlag::StudioCSGAssets)
                RBX::PartOperationAsset::publishSelection(m_pDataModel);

			RBX::Selection* sel = RBX::ServiceProvider::create<RBX::Selection>(getWorkspace());
			RBX::CSGDictionaryService* dictionaryService = RBX::ServiceProvider::create<RBX::CSGDictionaryService>(m_pDataModel);
			RBX::NonReplicatedCSGDictionaryService* nrDictionaryService = RBX::ServiceProvider::create<RBX::NonReplicatedCSGDictionaryService>(m_pDataModel);

			if (dictionaryService && nrDictionaryService)
				for (RBX::Instances::const_iterator iter = sel->begin(); iter != sel->end(); ++iter)
				{
					dictionaryService->retrieveAllDescendants(*iter);
					nrDictionaryService->retrieveAllDescendants(*iter);
				}

            RBX::Instances instances(sel->begin(), sel->end());
            RBX::SerializerBinary::serialize(m_pContent->data, instances);

			if (dictionaryService && nrDictionaryService)
				for (RBX::Instances::const_iterator iter = sel->begin(); iter != sel->end(); ++iter)
				{
					dictionaryService->storeAllDescendants(*iter);
					nrDictionaryService->storeAllDescendants(*iter);
				}
		}
	}
	catch (std::exception &e) 
	{
		RobloxMainWindow::sendCounterEvent("QTStudioPublishSelectionFailure", false);

        if (FFlag::GoogleAnalyticsTrackingEnabled)
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ERROR, "StudioPublishSelectionFailure");
		}

		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
		return NULL;
	}	
	
	return m_pContent;
}

void RbxWorkspace::writeToStream(const XmlElement* root, std::ostream& stream)
{
	TextXmlWriter machine(stream);
	machine.serialize(root);
}

// Do not confuse the DataModel Workspace with the RbxWorkspace. We are using RbxWorkspace as a Javascript Bridge Object
RBX::Workspace* RbxWorkspace::getWorkspace() const
{
	return m_pDataModel ? m_pDataModel->getWorkspace() : NULL;
}

std::auto_ptr<XmlElement> RbxWorkspace::writeSelectionToXml()
{
	std::auto_ptr<XmlElement> root(Serializer::newRootElement());
	{
		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
		RBX::Selection* sel = RBX::ServiceProvider::create<RBX::Selection>(getWorkspace());
		RBX::AddSelectionToRoot(root.get(), sel, RBX::SerializationCreator);
		if (sel && sel->size()==1 && dynamic_cast<RBX::KeyframeSequence*>(sel->front().get()))
		{
			root->addAttribute(tag_assettype, "animation");
		}
	}
	return root;
}

QString RbxWorkspace::GetStudioSessionID()
{
	return QString(FFlag::StudioExposeSessionID ? RBX::RobloxGoogleAnalytics::getSessionId().c_str() : "");
}

bool RbxWorkspace::Insert(const QString& urlQStr)
{	
	try
	{
		if (!m_pDataModel)
			throw std::runtime_error("Can't insert at this time");	
				
		QByteArray ba = urlQStr.toLocal8Bit();
		const char *url = ba.data();
		
		bool success = RBX::Http::trustCheck(url);
		std::string urlString = urlQStr.toStdString();
				
        if (FFlag::GoogleAnalyticsTrackingEnabled)
        {
            // Get the assetID from the url and use it as the event label.
            QString url(urlString.c_str());
            QStringList urlSplit = url.split("?id=");
            if (!urlSplit.empty())
            {
                QString assetID = urlSplit.last();
				if (!FFlag::StudioRemoveInsertEvent)
					RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "Insert", assetID.toStdString().c_str());
				RobloxMainWindow::get(this)->m_inserts.increment();
            }
        }

		insert(RBX::ContentId(urlString), false);

		RobloxMainWindow::sendCounterEvent("StudioWorkspaceInsertCounter", false);
	}
	catch (std::exception const& e)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
		return false;
	}
	return true;
}

// InsertInto:	TREE_VIEW, SUPPRESS_PROMPTS
// Insert:      3D_VIEW, ALLOW_PROMPTS

void RbxWorkspace::insert(std::istream& stream, bool insertInto)
{
	if (!m_pDataModel)
		throw std::runtime_error("Can't insert at this time");
	
	// The following code happens BEFORE we do a lock because we can't display a message box within a lock
	RBX::Instances instances;
	
    Serializer().loadInstances(stream, instances);

	insert(instances, insertInto);
}

void RbxWorkspace::insert(RBX::Instances& instances, bool insertInto)
{	
	{
		// Lock the data model temporarily to make surRBX::e insertion is legal
		RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);

		if (RBX::ContentProvider* cp = m_pDataModel->create<RBX::ContentProvider>())
		{
			RBX::LuaSourceContainer::blockingLoadLinkedScriptsForInstances(cp, instances);
		}
	}

	RBX::PromptMode promptMode = RBX::SUPPRESS_PROMPTS;
	if (!insertInto)
	{
		if (instances.size() == 1)
		{
			RBX::Instance* single = instances[0].get();
			if (RBX::Instance::fastDynamicCast<RBX::Tool>(single)) 
			{				
				if ( QMessageBox::question(
                        &UpdateUIManager::Instance().getMainWindow(), 
                        "Insert Tool", 
                        "Put this tool into the starter pack (otherwise drop into the 3d view)?", 
                        QMessageBox::Yes | QMessageBox::No, QMessageBox::No ) == QMessageBox::Yes )
					promptMode = RBX::PUT_TOOL_IN_STARTERPACK;
			}
		}
	}
	
	// Now lock the datamodel (after MessageBox)
	RBX::DataModel::LegacyLock lock(m_pDataModel, RBX::DataModelJob::Write);
	
	shared_ptr<RBX::Workspace> spWorkspace = shared_from(getWorkspace());
	if(!spWorkspace)
		throw std::runtime_error("Can't insert at this time");

	if (DFFlag::MaterialPropertiesEnabled)
	{
		StudioUtilities::convertPhysicalPropertiesIfNeeded(instances, spWorkspace.get());
	}
	
	RBX::Selection* sel = RBX::ServiceProvider::create<RBX::Selection>(spWorkspace.get());
	
	RBX::Instance* requestedParent = 0;

    requestedParent = spWorkspace.get();
	
	RBX::InsertMode insertMode =  RBX::INSERT_TO_3D_VIEW;
	
	// if we have a selection make sure we can invoke insert decal tool
	if ((instances.size() == 1) && RBX::Instance::fastDynamicCast<RBX::Decal>(instances[0].get()) && (sel->size() == 1))
	{
		requestedParent = sel->front().get();
		insertMode = RBX::Instance::fastDynamicCast<RBX::PartInstance>(requestedParent) ? RBX::INSERT_TO_TREE : RBX::INSERT_TO_3D_VIEW;
	}
	
	try
	{
		// get position to insert at
		bool isOnPart = false;
		RBX::Vector3 pos = InsertObjectWidget::getInsertLocation(shared_from(m_pDataModel), NULL, &isOnPart);
		spWorkspace->insertInstances(instances, requestedParent, insertMode, promptMode, isOnPart ? &pos : NULL);

		if (promptMode != RBX::PUT_TOOL_IN_STARTERPACK)
		{
			// zoom camera at the inserted instances
			RBX::PartArray parts;
			for (size_t i = 0; i < instances.size(); ++i) 
				RBX::PartInstance::findParts(instances[i].get(), parts);
			if (parts.size() > 0)
			{
				RBX::Extents ext = RBX::DragUtilities::computeExtents(parts);
				if (!ext.isNanInf() && !ext.isNull())
				{
					RBX::Camera* camera = spWorkspace->getCamera();
					float cameraDistance = fabsf((camera->getCameraCoordinateFrame().translation - ext.center()).magnitude());
					// if extents are out of camera frustum or size of the part is small, we need to zoom in/out
					if (!camera->frustum().containsAABB(ext) || 
						((cameraDistance > 0) && ext.longestSide()/cameraDistance < FInt::StudioPartSizeCameraDistRatio/100.0f))
					{
						ext.expand(ext.longestSide());
						camera->zoomExtents(ext, RBX::Camera::ZOOM_IN_OR_OUT);
					}
				}
			}
		}
		
		boost::shared_ptr<RBX::RunService> runService = shared_from(m_pDataModel->create<RBX::RunService>());
		// If we're not playing a game then select the content
		if (runService->getRunState() == RBX::RS_STOPPED)
		{
			sel->setSelection(instances.begin(), instances.end());
			RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
			if (playDoc)
			{
				UpdateUIManager::Instance().updateToolBars();
			}
		}
	}
	catch (std::exception&)
	{
		std::for_each(instances.begin(), instances.end(), boost::bind(&RBX::Instance::setParent, _1, (RBX::Instance*) NULL));
		throw;
	}
}


void RbxWorkspace::insert(RBX::ContentId contentId, bool insertInto)
{
	if (!m_pDataModel)
		throw std::runtime_error("Can't insert at this time");

	// load instances in a different thread
	bool hasError = false;
	RBX::Instances instances;
	UpdateUIManager::Instance().waitForLongProcess("Inserting...", 
		boost::bind(&RbxWorkspace::loadContent, this, contentId, boost::ref(instances), boost::ref(hasError)));

	if (hasError || instances.size() < 1)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, "Selected object cannot be inserted at this time.");
		return;
	}

	if (FFlag::StudioRecordToolboxInsert)
		UpdateUIManager::Instance().getMainWindow().trackToolboxInserts(contentId, instances);

	// now insert instances
	insert(instances, insertInto);
}

void RbxWorkspace::loadContent(RBX::ContentId contentId, RBX::Instances& instances, bool& hasError)
{
	try
	{
		std::auto_ptr<std::istream> stream(RBX::ServiceProvider::create<RBX::ContentProvider>(m_pDataModel)->getContent(contentId));
		Serializer().loadInstances(*stream, instances);
		hasError = false;
	}
	catch (...)
	{
		hasError = true;
	}
}

#ifdef ENABLE_DRAG_DROP_TOOLBOX
bool RbxWorkspace::StartDrag(const QString& urlQStr)
{
	try
	{
		if (!m_pDataModel)
			throw std::runtime_error("Can't insert at this time");	
				
		bool success = RBX::Http::trustCheck(urlQStr.toLocal8Bit().data());
		if(success)
		{
			QDrag *pDrag = new QDrag(qobject_cast<QWidget*>(m_pParent));
			QMimeData *pMimeData = new QMimeData;
			
			QList<QUrl> listUrl;
			listUrl << urlQStr;
			
			pMimeData->setUrls(listUrl);
			pDrag->setMimeData(pMimeData);
			
			QPixmap pixMap(1, 1);
			pDrag->setPixmap(pixMap);
			
#ifdef Q_WS_MAC
			pDrag->exec();	
#else
			pDrag->exec(Qt::CopyAction);
#endif
			pDrag->deleteLater();
		}	
		
	}
	catch (std::exception const& e)
	{
		RBX::StandardOut::singleton()->print(RBX::MESSAGE_ERROR, e.what());
		return false;
	}
	
	return true;
	
}
#endif

bool RbxWorkspace::OpenRecentFile(const QString &recentFileName)
{
	return UpdateUIManager::Instance().getMainWindow().openRecentFile(recentFileName);
}



bool RbxWorkspace::OpenPicFolder()
{
	QString userPictureFolder = QtUtilities::qstringFromStdString(RBX::FileSystem::getUserDirectory(true, RBX::DirPicture).native());
	QDir pictureDir(userPictureFolder);

	if (!pictureDir.exists())
		pictureDir.mkpath(userPictureFolder);

	QDesktopServices::openUrl(QUrl::fromLocalFile(userPictureFolder));
	
	return true;
}

void RbxWorkspace::PostImage(bool doPost, int postSetting, const QString &fileName, const QString &seoStr)
{
	RBX::GameSettings::singleton().setPostImageSetting((RBX::GameSettings::UploadSetting)postSetting);
	if (doPost)
		ScreenshotVerb::DoPostImage(shared_from(m_pDataModel), fileName, seoStr);
}

#ifdef Q_WS_WIN32
bool RbxWorkspace::OpenVideoFolder()
{
	QString userVideoFolder = QtUtilities::qstringFromStdString(RBX::FileSystem::getUserDirectory(true, RBX::DirVideo).native());
	QDir videoDir(userVideoFolder);

	if (!videoDir.exists())
		videoDir.mkpath(userVideoFolder);

	QDesktopServices::openUrl(QUrl::fromLocalFile(userVideoFolder));
	
	return true;
}

void RbxWorkspace::UploadVideo(const QString &token, bool doPost, int postSetting, const QString &title) 
{
	RBX::GameBasicSettings::singleton().setUploadVideoSetting((RBX::GameSettings::UploadSetting)postSetting);
	if (doPost != 1)
		return;

	RBXASSERT(m_pDataModel);
	RBXASSERT(!StudioUtilities::isVideoUploading());
	
	std::string fileName(StudioUtilities::getVideoFileName());
	RBXASSERT(!fileName.empty());

	//get video title
	std::string videoTitle("");
	if (title.isEmpty())
		videoTitle = "ROBLOX ROCKS!";
	else
		videoTitle= title.toStdString();	
	
	//get SEO information
	std::string videoSEOInfo("");
	bool siteSEO(false);

	if (m_pDataModel->isVideoSEOInfoSet())
	{
		videoSEOInfo = m_pDataModel->getVideoSEOInfo();
		siteSEO = true;
	}
	else
	{
		siteSEO = false;

		int placeId = m_pDataModel->getPlaceID();
		if (placeId > 0) 
		{
			QString str(QString("To play this game, please visit: http://www.roblox.com/item.aspx?id=%1&amp;rbx_source=youtube&amp;rbx_medium=uservideo").arg(placeId));
			videoSEOInfo = str.toStdString();
		} 
	}

	// thread will be destroyed once the upload is over
	UploadVideoThread *pUploadVideoThread = new UploadVideoThread(RBX::shared_from(m_pDataModel), fileName, token.toStdString(), videoTitle, videoSEOInfo, siteSEO);
	pUploadVideoThread->start();
}

UploadVideoThread::UploadVideoThread(boost::shared_ptr<RBX::DataModel> pDataModel, const std::string &videoFileName, const std::string &youtubeToken, 
								 const std::string &videoTitle, const std::string &videoSEOInfo, bool siteSEO)
: QThread(&UpdateUIManager::Instance().getMainWindow())
, m_pDataModel(pDataModel)
, m_videoFileName(videoFileName)
, m_youtubeToken(youtubeToken)
, m_videoTitle(videoTitle)
, m_videoSEOInfo(videoSEOInfo)
, m_bSiteSEO(siteSEO)
{
	connect(this, SIGNAL(finished()), this, SLOT(deleteLater()));
}

UploadVideoThread::~UploadVideoThread()
{
	//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,  "UploadVideoThread::~UploadVideoThread");
}

void UploadVideoThread::run()
{
	//RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,  "UploadVideoThread::run");
	if (!m_pDataModel)
		return;

	//create a dummy variable, to set the video upload status
	VideoUploadStatus dummy;

	//show appropriate messages while uploading
	shared_ptr<RBX::CoreGuiService> coreGuiService = RBX::shared_from(m_pDataModel->find<RBX::CoreGuiService>());
	if(coreGuiService)
		m_pDataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Uploading video ...", 0), RBX::DataModelJob::Write);

	RBX::Http http(RBX::format("http://uploads.gdata.youtube.com/feeds/api/users/default/uploads"));
	try
	{
		std::string request1;
		if (!m_bSiteSEO)
		{
			std::string requestTmp(
				"--f93dcbA3\r\n"
				"Content-Type: application/atom+xml; charset=UTF-8\r\n"
				"\r\n"
				"<?xml version=\"1.0\"?>\r\n"
				"<entry xmlns=\"http://www.w3.org/2005/Atom\"\r\n"
				"xmlns:media=\"http://search.yahoo.com/mrss/\"\r\n"
				"xmlns:yt=\"http://gdata.youtube.com/schemas/2007\">\r\n"
				"<media:group>\r\n"
				"<media:title type=\"plain\">" + m_videoTitle + "</media:title>\r\n"
				"<media:description type=\"plain\">\r\n"
				"" + m_videoSEOInfo + "\r\n"
				"For more games visit http://www.roblox.com\r\n"
				"</media:description>\r\n"
				"<media:category\r\n"
				"scheme=\"http://gdata.youtube.com/schemas/2007/categories.cat\">Games\r\n"
				"</media:category>\r\n"
				"<media:keywords>ROBLOX, video, free game, online virtual world</media:keywords>\r\n"
				"</media:group>\r\n"
				"</entry>\r\n"
				"--f93dcbA3\r\n"
				"Content-Type: video/avi\r\n"
				"Content-Transfer-Encoding: binary\r\n"
				"\r\n"
				);
			request1 = requestTmp;
		}
		else
		{
			int s = m_videoSEOInfo.find(sVideoTitlePrefix) + sVideoTitlePrefix.length();
			int e = m_videoSEOInfo.find(sVideoTitlePostfix);
			m_videoSEOInfo = m_videoSEOInfo.replace(s, e - s, m_videoTitle);

			request1 = RBX::format("--f93dcbA3\r\n"
				"Content-Type: application/atom+xml; charset=UTF-8\r\n"
				"\r\n"
				"%s\r\n"
				"--f93dcbA3\r\n"
				"Content-Type: video/avi\r\n"
				"Content-Transfer-Encoding: binary\r\n"
				"\r\n", m_videoSEOInfo.c_str());
		}

		std::stringstream buffer;		
		buffer << request1;

		RBXASSERT(!m_videoFileName.empty());
		std::ifstream file(m_videoFileName.c_str(), std::ios::in | std::ios::binary);
		if ( file.is_open() )
		{
			buffer << file.rdbuf();
			file.close();
		}
		else
		{
			if(coreGuiService)
				m_pDataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Failed to upload video", 2), RBX::DataModelJob::Write);
			return;
		}

		std::string request2("\r\n--f93dcbA3--\r\n");

		buffer << request2;

		buffer << '\0';

		http.additionalHeaders["Authorization"] = "AuthSub token=\"" + m_youtubeToken + "\"";
		http.additionalHeaders["GData-Version"] = "2";
		http.additionalHeaders["X-GData-Key"] = "key=AI39si5sZKe6qAobFgnT9UFGXq9bBO7mUCsK3_cWy_LJmgKDtl-GOMHNNV_Bh7Jk7KqDX7vI8D30jFHwnu8RJcDmcJN47yPW7A";
		http.additionalHeaders["Slug"] = "roblox.avi";
		http.additionalHeaders["Connection"] = "close";
		http.additionalHeaders["Content-Length"] = RBX::format("%d", buffer.str().length());

		std::string response;
		http.post(buffer, "multipart/related; boundary=\"f93dcbA3\"", false, response);

		// Check the response to see if the upload succeeded.
		// TODO: better way of checking if the upload succeeded from youtube?
		int pos = response.find("videoid");
		if(coreGuiService)
		{
			if (pos == std::string::npos)
				m_pDataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Failed to upload video", 2), RBX::DataModelJob::Write);
			else
				m_pDataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Video uploaded to YouTube", 2), RBX::DataModelJob::Write);;
		}
	}

	catch (...) 
	{
		if(coreGuiService)
			m_pDataModel->submitTask(boost::bind(&RBX::CoreGuiService::displayOnScreenMessage, coreGuiService, 1, "Failed to upload video", 2), RBX::DataModelJob::Write);
	}
}
#endif
bool RbxWorkspace::TakeThumbnail(QString selector)
{
	m_selectorForFileUpload = selector;
	if (m_screenShotInProgressMsgBox)
		return false;

	// Hide the current web dialog
	Hide();

	// Pop up a message box that they can click OK on to take a photo
	QMessageBox* msgBox = new QMessageBox();
	msgBox->setAttribute( Qt::WA_DeleteOnClose );
	msgBox->setStandardButtons( QMessageBox::Ok );
	msgBox->setWindowTitle( tr("Take Snapshot") );
	msgBox->setIconPixmap(QPixmap(":/16x16/images/Studio 2.0 icons/16x16/picture-icon.png"));
	msgBox->setWindowFlags(Qt::WindowStaysOnTopHint | Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
	msgBox->setText("<p align='center'>Press OK when you've positioned the camera, and are ready to take a snapshot.</p>");
	msgBox->setModal( false );
	msgBox->show(); // Has to be before moving of msg box or we get wrong dimensions

	// Position the message box
	RobloxMainWindow& mainWindow = UpdateUIManager::Instance().getMainWindow();
	float winHeight = mainWindow.height();
	winHeight /= 1.3f;
	float winWidth = mainWindow.width();
	winWidth = winWidth - msgBox->frameGeometry().width();
	winWidth /= 2.0f;
	msgBox->move(winWidth, winHeight);

	// On OK Click, handle the thumbnail
	connect(msgBox, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(handleTakeThumbnail(QAbstractButton*)));

	return true;
}
void RbxWorkspace::onScreenShotFinished_MT(QString fileName)
{
	WebDialog* pWebDialog = dynamic_cast<WebDialog*>(m_pParent);

	// Delete our processing dialog
	if (m_screenShotInProgressMsgBox)
	{
		m_screenShotInProgressMsgBox->hide();
		m_screenShotInProgressMsgBox->deleteLater();
		m_screenShotInProgressMsgBox = NULL;
	}

	// Re-show the web page
	Show();

	// Add the file to the page's input
	pWebDialog->getWebPage()->setUploadFile(m_selectorForFileUpload, fileName);

	// Tell the page we've added the thumb
	QImage image(fileName);
	Q_EMIT thumbnailTaken(image.width(), image.height());
}
void RbxWorkspace::onScreenshotFinished(const std::string &fileName)
{	
	// Re-enable other handling of screenshots 
	RobloxIDEDoc* pIDEDoc = RobloxDocManager::Instance().getPlayDoc();
	ScreenshotVerb* pVerb = dynamic_cast<ScreenshotVerb*>(pIDEDoc->getVerb("Screenshot"));
	shared_ptr<RBX::DataModel> pDataModel = shared_from(pIDEDoc->getDataModel());
	pDataModel->screenshotReadySignal.disconnectAll();
	pVerb->reconnectScreenshotSignal();

	QString qFileName = QString::fromStdString(fileName);

	// Switch it to the main thread
	if (QThread::currentThread() != qApp->thread())
	{	
		QMetaObject::invokeMethod(
            this,
            "onScreenShotFinished_MT",
            Qt::QueuedConnection,
            Q_ARG(QString, qFileName));
		return;
	}
	else
	{
		RBXASSERT(0);// Should never be here, but if so, handle it
		onScreenShotFinished_MT(qFileName);	
	}
}
void RbxWorkspace::handleTakeThumbnail(QAbstractButton*)
{
	RobloxIDEDoc* pIDEDoc = RobloxDocManager::Instance().getPlayDoc();
	if (!pIDEDoc)
		return;

	// Disabling other handling of screenshots
	shared_ptr<RBX::DataModel> pDataModel = shared_from(pIDEDoc->getDataModel());
	pDataModel->screenshotReadySignal.disconnectAll();
	pDataModel->screenshotReadySignal.connect(boost::bind(&RbxWorkspace::onScreenshotFinished, this, _1));
	pDataModel->submitTask( boost::bind(&RBX::DataModel::TakeScreenshotTask, weak_ptr<RBX::DataModel>(pDataModel)), RBX::DataModelJob::Write );

	// Give them a processing dialog
	m_screenShotInProgressMsgBox = new QMessageBox(&UpdateUIManager::Instance().getMainWindow());
	m_screenShotInProgressMsgBox->setAttribute( Qt::WA_DeleteOnClose ); 
	m_screenShotInProgressMsgBox->setStandardButtons( QMessageBox::NoButton );
	m_screenShotInProgressMsgBox->setWindowTitle( tr("Processing...") );
	m_screenShotInProgressMsgBox->setText( tr("Snapshot in progress...") );
	m_screenShotInProgressMsgBox->setWindowModality(Qt::NonModal);
	m_screenShotInProgressMsgBox->show();
}

void RbxWorkspace::OpenUniverse(int universeId)
{
	QAction* action = UpdateUIManager::Instance().getDockAction(eDW_GAME_EXPLORER);
	if (!action->isChecked())
	{
		action->activate(QAction::Trigger);
	}
    QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER),
        "openGameFromGameId", Qt::QueuedConnection, Q_ARG(int, universeId));
}

void RbxWorkspace::PublishAsNewUniverse()
{
	QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER),
		"publishGameToNewSlot", Qt::QueuedConnection);
}

void RbxWorkspace::PublishAsNewGroupUniverse(int groupId)
{
	QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER),
		"publishGameToNewGroupSlot", Qt::QueuedConnection, Q_ARG(int, groupId));
}

void RbxWorkspace::PublishToUniverse(int universeId)
{
	QMetaObject::invokeMethod(&UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER),
		"publishGameToExistingSlot", Qt::QueuedConnection, Q_ARG(int, universeId));
}

void RbxWorkspace::ImportAsset(int assetId)
{
	if (!FFlag::StudioEnableGameAnimationsTab)
		return;

	ImportAssetDialog* pImportDlg = dynamic_cast<ImportAssetDialog*>(m_pParent);
	if (pImportDlg)
	{
		pImportDlg->setAssetId(assetId);
		pImportDlg->close();
	}
}

QString RbxWorkspace::GetGameAnimations(int universeId)
{
	if (!FFlag::StudioEnableGameAnimationsTab)
		return QString();

	RobloxGameExplorer& gameExplorer = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
	return gameExplorer.getAnimationsDataJson();
}