/**
 * Roblox.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "Roblox.h"

// Qt Headers
#include <QDateTime>
#include <QTimer>
#include <QThread>

// 3rd Party Headers
#include "boost/function.hpp"

// Roblox Headers
#include "v8datamodel/GameBasicSettings.h"
#include "v8datamodel/DebugSettings.h"
#include "v8datamodel/Game.h"
#include "v8datamodel/MouseCommand.h"
#include "v8datamodel/TeleportService.h"
#include "v8datamodel/DataModel.h"
#include "v8datamodel/Light.h"
#include "v8datamodel/Lighting.h"
#include "v8datamodel/HttpService.h"
#include "rbx/TaskScheduler.h"
#include "rbx/ProcessPerfCounter.h"
#include "Util/standardout.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "Util/Statistics.h"
#include "NetworkSettings.h"
#include "RenderSettingsItem.h"
#include "V8DataModel/Workspace.h"
#include "v8datamodel/CustomParticleEmitter.h"
#include "v8datamodel/ContentProvider.h"

// Roblox Studio Headers
#include "RobloxIDEDoc.h"
#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"
#include "FunctionMarshaller.h"
#include "RobloxSettings.h"
#include "AuthoringSettings.h"
#include "ExternalHandlers.h"
#include "RobloxDocManager.h"
#include "StudioUtilities.h"
#include "MachineConfig.h"
#include "RobloxUser.h"

#include "rbx/Profiler.h"

FASTFLAG(SurfaceLightEnabled)
DYNAMIC_FASTFLAG(CustomEmitterInstanceEnabled)
FASTFLAG(UsePGSSolver)

FASTFLAGVARIABLE(StudioABTestEnabled, false)

#ifdef STUDIO_ADMIN_BUILD
    #include "util/MD5Hasher.h"
#endif

bool Roblox::sInitialized = false;
rbx::signals::scoped_connection messageOutConnection;

#ifdef Q_WS_WIN
static boost::shared_ptr<CProcessPerfCounter> s_perfCounter;
#endif

#ifdef Q_WS_MAC

extern "C" {
	void writeFastLogDumpHelper(const char* fileName, int numEntries)
	{
		FLog::WriteFastLogDump(fileName, numEntries);
	}
};
#endif

//this function is being used in RBXGraphics.cpp also!
std::string macBundlePath()
{
#ifdef Q_WS_MAC
	char path[1024];
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	assert(mainBundle);
	
	CFURLRef mainBundleURL = CFBundleCopyBundleURL(mainBundle);
	assert(mainBundleURL);
	
	CFStringRef cfStringRef = CFURLCopyFileSystemPath( mainBundleURL, kCFURLPOSIXPathStyle);
	assert(cfStringRef);
	
	CFStringGetCString(cfStringRef, path, 1024, kCFStringEncodingASCII);
	
	CFRelease(mainBundleURL);
	CFRelease(cfStringRef);
	
	return std::string(path);
#endif
	return "";
}

Roblox& Roblox::Instance()
{			
	static Roblox singleton;
	return singleton;
}

Roblox::Roblox()
: m_pMarshaller(RBX::FunctionMarshaller::GetWindow())
, m_pTimer(new QTimer(this))
{
    qRegisterMetaType<RBX::MessageType>("RBX::MessageType");
	qRegisterMetaType<boost::shared_ptr<RBX::Instance> >("shared_ptr<RBX::Instance>");
	qRegisterMetaType<const RBX::Reflection::PropertyDescriptor*>("const RBX::Reflection::PropertyDescriptor*");

	connect(m_pTimer, SIGNAL(timeout()), this, SLOT(onTimeOut()));
}

Roblox::~Roblox()
{
	RBX::FunctionMarshaller::ReleaseWindow(m_pMarshaller);
}

void Roblox::startTimer()
{
	m_pTimer->start(1000);
}

void Roblox::stopTimer()
{
	m_pTimer->stop();
}

void Roblox::onTimeOut()
{
#ifdef Q_WS_WIN
	s_perfCounter->CollectData();
#endif
}

void Roblox::globalInit(const QString& urlArg, const QString& ticketArg, RBX::HttpFuture& settingsFuture)
{
	if (sInitialized)
		return;	

#ifdef STUDIO_ADMIN_BUILD
	RBX::DataModel::hash = RobloxSettings::getAdminKey().toStdString();
#endif

	messageOutConnection = RBX::StandardOut::singleton()->messageOut.connect(&onMessageOut);	

	if (FFlag::StudioABTestEnabled)
	{
		AuthenticationHelper::Instance().waitForHttpAuthentication();

		QSettings retentionData("Roblox", "Retention");
		static const char* const kBrowserTrackerId = "BrowserTrackerId";
		QString browserTrackerId = retentionData.value(kBrowserTrackerId, "").toString();

		const std::string& apiUrl = RBX::ContentProvider::getUnsecureApiBaseUrl(GetBaseURL());
		std::string abUrl = RBX::format("%s/users/get-studio-experiment-enrollments?firstStudioVisit=%s%s", apiUrl.c_str(), StudioUtilities::isFirstTimeOpeningStudio() ? "true" : "false",
			browserTrackerId.isEmpty() ? "" : RBX::format("&browserTrackerId=%s", browserTrackerId.toStdString().c_str()).c_str());
		RBX::HttpFuture abTestResult = FetchABTestDataAsync(abUrl);

		try 
		{
			std::string newBrowserTrackerId = LoadABTestFromString(abTestResult.get());
			if(browserTrackerId.isEmpty() && !newBrowserTrackerId.empty())
			{
				retentionData.setValue(kBrowserTrackerId, QString(newBrowserTrackerId.c_str()));
			}
		} 
		catch(std::exception&)
		{
			FASTLOG(FLog::Error, "Failed to load AB test data");
		}
	}

    RBX::Http::SetUseStatistics(true);

	RobloxSettings::recoverAssets();
	RobloxSettings::initWorkspaceSettings();

	RBX::MouseCommand::enableAdvArrowTool(true);

#ifdef Q_WS_WIN
	s_perfCounter = CProcessPerfCounter::getInstance();
#endif

	// Force loading of settings classes
	RBX::GameBasicSettings::singleton();
	AuthoringSettings::singleton();
	RBX::NetworkSettings::singleton();
	CRenderSettingsItem::singleton();
	RBX::DebugSettings::singleton();
			
	RBX::GlobalAdvancedSettings::singleton()->loadState(std::string());
	{
		RBX::Security::Impersonator impersonate(RBX::Security::RobloxGameScript_);
		RBX::GlobalBasicSettings::singleton()->loadState(std::string());
	}

	RBX::GameBasicSettings::singleton().setStudioMode(true);

    // now that fast flags are loaded we can reconfigure our settings if necessary
    AuthoringSettings::singleton().configureBasedOnFastFlags();
	
	RBX::Profiler::onThreadCreate("Main");

	// Initialize the TaskScheduler (after loading configs)
	RBX::TaskScheduler::singleton().setThreadCount(RBX::TaskSchedulerSettings::singleton().getThreadPoolConfig());

	Roblox::Instance().startTimer();
	
	MachineConfig::setupMachineConfig();

	RBX::TeleportService::SetCallback(&Roblox::Instance());
	RBX::TeleportService::SetBaseUrl(RobloxSettings::getBaseURL().toStdString().c_str());

	sInitialized = true;		

    if (!FFlag::SurfaceLightEnabled)
    {
        RBX::Reflection::ClassDescriptor* classDesc = &RBX::SurfaceLight::classDescriptor();
        RBX::Reflection::Metadata::Class* pMetadataDynamic = RBX::Reflection::Metadata::Reflection::singleton()->get(*classDesc, false);
        if (pMetadataDynamic)
        {
            RBX::Reflection::Metadata::Class::prop_deprecated.setValue(pMetadataDynamic, true);
        }
    }
    if (!DFFlag::CustomEmitterInstanceEnabled)
    {
        RBX::Reflection::ClassDescriptor* classDesc = &RBX::CustomParticleEmitter::classDescriptor();
        RBX::Reflection::Metadata::Class* pMetadataDynamic = RBX::Reflection::Metadata::Reflection::singleton()->get(*classDesc, false);
        if (pMetadataDynamic)
        {
            RBX::Reflection::Metadata::Class::prop_deprecated.setValue(pMetadataDynamic, true);
        }
    }

	if (!FFlag::UsePGSSolver)
	{
		RBX::Reflection::PropertyDescriptor* propDesc = RBX::Workspace::classDescriptor().findPropertyDescriptor("PGSPhysicsSolverEnabled");
		if(propDesc) {
			RBX::Reflection::Metadata::Member* pMetadataDynamic = RBX::Reflection::Metadata::Reflection::singleton()->get(*propDesc);
			if(pMetadataDynamic)
				RBX::Reflection::Metadata::Item::prop_deprecated.setValue(pMetadataDynamic, true);
		}
	}
}

void Roblox::globalShutdown()
{
	RobloxSettings::saveAssets();
#ifdef Q_WS_WIN
	s_perfCounter.reset();
#endif
	Roblox::Instance().stopTimer();
	RBX::GlobalBasicSettings::singleton()->saveState();
	RBX::Game::globalExit();
    messageOutConnection.disconnect();
}

// FunctionMarshaller help - posts worker thread message to UI thread and deals with it next time through, wait till the event is processed
void Roblox::sendAppEvent(void *pClosure)
{
	RBX::CEvent *waitEvent = ((RBX::FunctionMarshaller::Closure *) pClosure)->waitEvent;
	bool waitFlag = (waitEvent == NULL);
	
	Q_EMIT Roblox::Instance().marshallAppEvent(pClosure, waitFlag);
	
	if (waitEvent)
		waitEvent->Wait();
}

// FunctionMarshaller help - posts worker thread message to UI thread and deals with it next time through, do not wait
void Roblox::postAppEvent(void *pClosure)
{
	Q_EMIT Roblox::Instance().marshallAppEvent(pClosure, false);	
}

bool Roblox::isTeleportEnabled() const
{
	return true;
}

void Roblox::onMessageOut(const RBX::StandardOutMessage& message)
{
	Q_EMIT Roblox::Instance().newOutputMessage(message.message.c_str(), message.type);	
}

void Roblox::Teleport(const std::string& url, const std::string& ticket, const std::string& script)
{
    // Teleporting in Studio is disabled.
	RobloxIDEDoc* pPlayDoc = RobloxDocManager::Instance().getPlayDoc();
	if(!pPlayDoc)
		return;

	pPlayDoc->teleportToURL(script.c_str(), true);
}

void Roblox::doTeleport(const std::string& url, const std::string& ticket, const std::string& script)
{
    RBXASSERT(isTeleportEnabled());
	boost::function<void()> callback = boost::bind(&Roblox::Teleport, this, url, ticket, script);
	m_pMarshaller->Submit(callback);
}