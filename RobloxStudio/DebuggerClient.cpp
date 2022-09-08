/**
 * DebuggerClient.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */
#include "stdafx.h"
#include "DebuggerClient.h"

// Roblox Headers
#include "util/StandardOut.h"
#include "util/ScopedAssign.h"
#include "v8datamodel/datamodel.h"
#include "v8datamodel/changehistory.h"
#include "reflection/Property.h"
#include "Script/Script.h"

// Qt Headers
#include <QMutex>
#include <QInputDialog>
#include <QObject>

// Roblox Studio Headers
#include "RobloxScriptDoc.h"
#include "RobloxMainWindow.h"
#include "DebuggerWidgets.h"
#include "UpdateUIManager.h"
#include "ScriptTextEditor.h"
#include "RobloxDocManager.h"
#include "QtUtilities.h"
#include "RobloxSettings.h"

#define STUDIO_EMIT_SIGNAL //dummy define to know where we are emitting a signal

FASTFLAG(LuaDebugger)
FASTFLAG(LuaDebuggerBreakOnError)

FASTFLAGVARIABLE(StudioRemoveDebuggerResumeLock, false)

static const char* kBreakOnErrorModeSetting = "BreakOnErrorMode";
static const char* kBreakOnErrorDialogDisabled = "BreakOnErrorDialogDisabled";

DebuggerClientManager& DebuggerClientManager::Instance()
{
	static DebuggerClientManager instance;
	return instance;
}

DebuggerClientManager::DebuggerClientManager()
: m_pCurrentDebuggerClient(NULL)
, m_pActiveDebuggerClient(NULL)
, m_pSeparatorAction(NULL)
, m_bIgnorePauseExecution(false)
, m_bDebuggerListUpdateRequested(false)
{
	if (!FFlag::LuaDebugger)
		return;

	RobloxMainWindow& rbxMainWindow = UpdateUIManager::Instance().getMainWindow();

	//connect with debugging commands
	rbxMainWindow.stepIntoAction->setData(STEP_INTO);
	rbxMainWindow.stepOutAction->setData(STEP_OUT);
	rbxMainWindow.stepOverAction->setData(STEP_OVER);

	//TODO: modify this code to use "actionState" paradigm
	connect(rbxMainWindow.stepIntoAction, SIGNAL(triggered()), this, SLOT(onStepAction()));
	connect(rbxMainWindow.stepOutAction, SIGNAL(triggered()), this, SLOT(onStepAction()));
	connect(rbxMainWindow.stepOverAction, SIGNAL(triggered()), this, SLOT(onStepAction()));

	//add shortcuts to action help
	QtUtilities::setActionShortcuts(*(rbxMainWindow.stepIntoAction), rbxMainWindow.stepIntoAction->shortcuts());
	QtUtilities::setActionShortcuts(*(rbxMainWindow.stepOverAction), rbxMainWindow.stepOverAction->shortcuts());
	QtUtilities::setActionShortcuts(*(rbxMainWindow.stepOutAction),  rbxMainWindow.stepOutAction->shortcuts());	
}

void DebuggerClientManager::setDataModel(shared_ptr<RBX::DataModel> spDataModel)
{
	if (m_spDataModel == spDataModel)
		return;

	if (m_spDataModel)
	{
		m_pCurrentDebuggerClient = NULL;
		m_pActiveDebuggerClient = NULL;

		m_cDebuggerAddedConnection.disconnect();
		m_cDebuggerRemovedConnection.disconnect();
		m_cRunTransitionConnection.disconnect();

		updateDebugActions(false);

		std::vector<DebuggerClient *> copyDebuggerClients(debuggerClients);
		debuggerClients.clear();

		for (size_t i = 0; i < copyDebuggerClients.size(); ++i)
			delete copyDebuggerClients[i];
        
		STUDIO_EMIT_SIGNAL clearAll();
	}

	m_spDataModel = spDataModel;

	if (m_spDataModel)
	{
		//if DebuggerManager is already present then add saved breakpoints
		RBX::Scripting::DebuggerManager& debuggerManager = RBX::Scripting::DebuggerManager::singleton();

		m_cDebuggerAddedConnection = debuggerManager.debuggerAdded.connect(boost::bind(&DebuggerClientManager::onDebuggerAdded, this, _1));
		m_cDebuggerRemovedConnection = debuggerManager.debuggerRemoved.connect(boost::bind(&DebuggerClientManager::onDebuggerRemoved, this, _1));

		// also enable debugging if required
		if (!debuggerManager.getEnabled())
			debuggerManager.enableDebugging();
		
		if (FFlag::LuaDebuggerBreakOnError)
		{
			RobloxSettings settings;
			debuggerManager.setBreakOnErrorMode((RBX::Scripting::BreakOnErrorMode)settings.value(kBreakOnErrorModeSetting, (int)RBX::Scripting::BreakOnErrorMode_Never).toInt());
		}

		RBX::RunService* pRunService = m_spDataModel->create<RBX::RunService>();
		if (pRunService)
			m_cRunTransitionConnection = pRunService->runTransitionSignal.connect(boost::bind(&DebuggerClientManager::onRunTransition, this, _1));
	}
}

shared_ptr<RBX::DataModel> DebuggerClientManager::getDataModel()
{ return m_spDataModel; }

bool DebuggerClientManager::actionState(const QString &actionID, bool &enableState, bool &checkedState)
{
	if (FFlag::LuaDebuggerBreakOnError)
	{
		if (actionID == "neverBreakOnScriptErrorsAction")
		{
			checkedState = RBX::Scripting::DebuggerManager::singleton().getBreakOnErrorMode() == RBX::Scripting::BreakOnErrorMode_Never;
			enableState = true;
			return true;
		}

		if (actionID == "breakOnAllScriptErrorsAction")
		{
			checkedState = RBX::Scripting::DebuggerManager::singleton().getBreakOnErrorMode() == RBX::Scripting::BreakOnErrorMode_AllExceptions;
			enableState = true;
			return true;
		}

		if (actionID == "breakOnUnhandledScriptErrorsAction")
		{
			checkedState = RBX::Scripting::DebuggerManager::singleton().getBreakOnErrorMode() == RBX::Scripting::BreakOnErrorMode_UnhandledExceptions;
			enableState = true;
			return true;
		}
	}

	return false;
}

bool DebuggerClientManager::handleAction(const QString &actionID, bool checkedState)
{
	if (FFlag::LuaDebuggerBreakOnError)
	{
		if (actionID == "neverBreakOnScriptErrorsAction")
		{
			RBX::Scripting::DebuggerManager::singleton().setBreakOnErrorMode(RBX::Scripting::BreakOnErrorMode_Never);
			RobloxSettings().setValue(kBreakOnErrorModeSetting, (int)RBX::Scripting::BreakOnErrorMode_Never);
			return true;
		}

		if (actionID == "breakOnAllScriptErrorsAction")
		{
			RBX::Scripting::DebuggerManager::singleton().setBreakOnErrorMode(RBX::Scripting::BreakOnErrorMode_AllExceptions);
			RobloxSettings().setValue(kBreakOnErrorModeSetting, (int)RBX::Scripting::BreakOnErrorMode_AllExceptions);
			return true;
		}

		if (actionID == "breakOnUnhandledScriptErrorsAction")
		{
			RBX::Scripting::DebuggerManager::singleton().setBreakOnErrorMode(RBX::Scripting::BreakOnErrorMode_UnhandledExceptions);
			RobloxSettings().setValue(kBreakOnErrorModeSetting, (int)RBX::Scripting::BreakOnErrorMode_UnhandledExceptions);
			return true;
		}
	}

	return false;
}

void DebuggerClientManager::addDebuggerClient(DebuggerClient* pDebuggerClient)
{
	if (!m_spDataModel)
		return;

	QMutexLocker lock(&m_debuggerClientMutex);
	std::vector<DebuggerClient *>::iterator iter = debuggerClients.begin();
	while (iter != debuggerClients.end())
	{
		if (*iter == pDebuggerClient)
			return;
		++iter;
	}

	debuggerClients.push_back(pDebuggerClient);	
}

void DebuggerClientManager::removeDebuggerClient(DebuggerClient* pDebuggerClient)
{
	if (!pDebuggerClient)
		return;

	QMutexLocker lock(&m_debuggerClientMutex);
	std::vector<DebuggerClient *>::iterator iter = debuggerClients.begin();
	while (iter != debuggerClients.end())
	{
		if (*iter == pDebuggerClient)
		{
			if (pDebuggerClient == m_pCurrentDebuggerClient)
				m_pCurrentDebuggerClient = NULL;

			if (pDebuggerClient == m_pActiveDebuggerClient)
				m_pActiveDebuggerClient = NULL;

			debuggerClients.erase(iter);
			break;
		}

		++iter;
	}
}

void DebuggerClientManager::setActiveDebuggerClient(DebuggerClient* pDebuggerClient)
{
	m_pActiveDebuggerClient = pDebuggerClient;

	if (m_pActiveDebuggerClient)
		STUDIO_EMIT_SIGNAL clientActivated(m_pActiveDebuggerClient->getCurrentLine(), m_pActiveDebuggerClient->getCurrentCallStack());
	else
		STUDIO_EMIT_SIGNAL clientDeactivated();
}

RBX::Reflection::Variant DebuggerClientManager::getWatchValue(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{
	if (m_pActiveDebuggerClient)
		return m_pActiveDebuggerClient->getWatchValue(spWatch);
	return RBX::Reflection::Variant();
}

void DebuggerClientManager::addWatch(const QString& watchKey)
{
	if (m_pActiveDebuggerClient)
		m_pActiveDebuggerClient->addWatch(watchKey);
}

void DebuggerClientManager::syncBreakpointState(shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint)
{
	if (!spBreakpoint || !spBreakpoint->getParent())
		return;
	
	RBX::Scripting::ScriptDebugger* pDebugger = RBX::Instance::fastDynamicCast<RBX::Scripting::ScriptDebugger>(spBreakpoint->getParent());
	if (pDebugger)
	{
		DebuggerClient* pDebuggerClient = getDebuggerClient(shared_from(pDebugger->getScript()));
		if (pDebuggerClient)
			pDebuggerClient->syncBreakpointState(spBreakpoint->getLine());
	}
}

DebuggerClient* DebuggerClientManager::getOrCreateDebuggerClient(shared_ptr<RBX::Instance> spScript)
{
	if (!FFlag::LuaDebugger || !m_spDataModel || !m_spDataModel->isAncestorOf(spScript.get()))
		return NULL;

	DebuggerClient* pDebuggerClient = getDebuggerClient(spScript);
	if (!pDebuggerClient)
	{
		try
		{
			pDebuggerClient = new DebuggerClient(spScript);
		}
		catch (std::runtime_error const& exp)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: %s", exp.what());
		}
	}

	return pDebuggerClient;
}

DebuggerClient* DebuggerClientManager::getDebuggerClient(shared_ptr<RBX::Instance> spScript)
{
	if (FFlag::LuaDebugger && m_spDataModel)
	{
		QMutexLocker lock(&m_debuggerClientMutex);
		for (size_t ii = 0; ii < debuggerClients.size(); ++ii)
		{
			if (debuggerClients[ii]->getScript() == spScript)
				return debuggerClients[ii];
		}
	}

	return NULL;
}

const std::vector<DebuggerClient *>& DebuggerClientManager::getDebuggerClients()
{ return debuggerClients; }

void DebuggerClientManager::breakpointEncounter(DebuggerClient *pDebuggerClient)
{
	static QMutex breakpointEncounterMutex;
	QMutexLocker lock(&breakpointEncounterMutex);

	//looking for the first breakpoint hit, ignore rest of the hits
	if (m_pCurrentDebuggerClient)
	{
		// this has to be done in Main thread
		if (!m_bDebuggerListUpdateRequested)
		{
			m_bDebuggerListUpdateRequested = true;
			QMetaObject::invokeMethod(this, "updateDebuggersList", Qt::QueuedConnection);
		}
		return;
	}

	m_pCurrentDebuggerClient = pDebuggerClient;

	//open script document where we encountered first breakpoint
	m_bDebuggerListUpdateRequested = true;
	QMetaObject::invokeMethod(this, "updateScriptDocument", Qt::QueuedConnection);
}

void DebuggerClientManager::updateScriptDocument()
{
	//make sure we've a current document (important in case of Play solo)
	if (!RobloxDocManager::Instance().getCurrentDoc())
	{
		//TODO: how to ensure this doesn't go in an infinite loop? (counter?)
		QMetaObject::invokeMethod(this, "updateScriptDocument", Qt::QueuedConnection);
		return;
	}

	if (m_pCurrentDebuggerClient)
	{
		RBX::RunService* pRunState = m_spDataModel->find<RBX::RunService>();
		if(pRunState && pRunState->getRunState() == RBX::RS_RUNNING)
			pRunState->pause();

		m_pCurrentDebuggerClient->setCurrentThread(m_pCurrentDebuggerClient->getCurrentThread());

		//Break On Error Dialog
		if (m_pCurrentDebuggerClient->hasError())
			displayBreakOnErrorDialog();

		//Update document and debugger widgets
		updateDocumentData();
		STUDIO_EMIT_SIGNAL breakpointEncountered(m_pCurrentDebuggerClient->getCurrentLine(), m_pCurrentDebuggerClient);		
	}
}

void DebuggerClientManager::updateDebuggersList()
{
	//Update document and debugger widgets
	//Here, we need to emit signal first and then update document data. signal emit will make sure correct thread is set!
	STUDIO_EMIT_SIGNAL debuggersListUpdated(m_pCurrentDebuggerClient);
	updateDocumentData(false, false);	
}

void DebuggerClientManager::updateDocumentData(bool openDoc, bool updateMarkers)
{
	if (m_pCurrentDebuggerClient && m_pCurrentDebuggerClient->updateDocument(openDoc, updateMarkers))
		updateDebugActions(true);

	UpdateUIManager::Instance().updateToolBars();
	m_bDebuggerListUpdateRequested = false;
}

void DebuggerClientManager::displayBreakOnErrorDialog()
{
	bool breakOnErrorSet = RobloxSettings().value(kBreakOnErrorDialogDisabled, false).toBool();
	if (!breakOnErrorSet)
		{
			// Create Dialog Box
			RBX::Scripting::DebuggerManager::singleton();
			std::string labelMessage = std::string("The Roblox Lua Debugger caught an error and will now take you to the exception in the script.") + 
				"\n\n You can modify this behavior, when a script is open, by going to the Script Menu tab and changing the 'Break On Error' settings. \n";
			QtUtilities::RBXConfirmationMessageBox msgBox(
				QObject::tr(labelMessage.c_str()),
				QObject::tr("Keep breaking on error"),
				QObject::tr("Disable")
				);
			msgBox.setMinimumWidth(350);
			msgBox.setMinimumHeight(90);

			// Execute Dialog Box
			UpdateUIManager::Instance().setBusy(true, false);
			QApplication::setOverrideCursor(Qt::ArrowCursor);
			msgBox.exec();
			QApplication::restoreOverrideCursor();
			UpdateUIManager::Instance().setBusy(false, false);

			// {Process Dialog Response
			if (msgBox.getClickedYes())
			{
				RBX::Scripting::DebuggerManager::singleton().setBreakOnErrorMode(RBX::Scripting::DebuggerManager::singleton().getBreakOnErrorMode());
				RobloxSettings().setValue(kBreakOnErrorModeSetting, (int)RBX::Scripting::DebuggerManager::singleton().getBreakOnErrorMode());
			}
			else
			{
				RBX::Scripting::DebuggerManager::singleton().setBreakOnErrorMode(RBX::Scripting::BreakOnErrorMode_Never);
				RobloxSettings().setValue(kBreakOnErrorModeSetting, (int)RBX::Scripting::BreakOnErrorMode_Never);
			}

			if (msgBox.getCheckBoxState())
			{
				RobloxSettings().setValue(kBreakOnErrorDialogDisabled, true);
			}
		}
}

void DebuggerClientManager::startDebugging(RBX::RunTransition evt)
{
	try
	{
		if (evt.oldState == RBX::RS_STOPPED)
		{
			RBX::Scripting::DebuggerManager::singleton().setScriptAutoResume(false);
			STUDIO_EMIT_SIGNAL debuggingStarted();
		}
		else if (evt.oldState == RBX::RS_PAUSED)
		{
			resumeAllClients(true);
		}
	}
	catch (std::runtime_error const& exp)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: %s", exp.what());
	}
}

void DebuggerClientManager::pauseDebugging(RBX::RunTransition evt)
{
	try
	{
		pauseAllClients(true);
	}
	catch (std::runtime_error const& exp)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: %s", exp.what());
	}
}

void DebuggerClientManager::stopDebugging(RBX::RunTransition evt)
{
	try
	{
		//reset current debugger client
		m_pCurrentDebuggerClient = NULL;
		RBX::Scripting::DebuggerManager::singleton().reset();
		resetAllClients();

		QMetaObject::invokeMethod(this, "updateDebugActions", Qt::QueuedConnection, Q_ARG(bool, false));
		STUDIO_EMIT_SIGNAL debuggingStopped();
	}
	catch (std::runtime_error const& exp)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: %s", exp.what());
	}
}

void DebuggerClientManager::onStepAction()
{
	QAction* pAction = qobject_cast<QAction *>(sender());
	if (!pAction)
		return;

	try
	{
		RBX::DataModel::LegacyLock lock(m_spDataModel, RBX::DataModelJob::Write);

		if (m_pCurrentDebuggerClient)
			m_pCurrentDebuggerClient->resetExecution();

		STUDIO_EMIT_SIGNAL executionDataCleared();
		m_pCurrentDebuggerClient = NULL;

		eStepMode stepMode = (eStepMode)pAction->data().toInt();
		switch (stepMode)
		{
		case STEP_INTO:
			RBX::Scripting::DebuggerManager::singleton().stepInto();
			break;
		case STEP_OUT:
			RBX::Scripting::DebuggerManager::singleton().stepOut();
			break;
		case STEP_OVER:
			RBX::Scripting::DebuggerManager::singleton().stepOver();
			break;
		}

		RBX::RunService* pRunState = m_spDataModel->find<RBX::RunService>();
		if(pRunState && pRunState->getRunState() == RBX::RS_PAUSED)
			pRunState->run();

		UpdateUIManager::Instance().updateToolBars();
	}
	catch (std::runtime_error const& exp)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: %s", exp.what());
	} 

	catch (...)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: Not known");
	}
}

void DebuggerClientManager::onRunTransition(RBX::RunTransition evt)
{
	try
	{
		switch (evt.newState)
		{
		case RBX::RS_STOPPED:
			stopDebugging(evt);
			break;
		case RBX::RS_RUNNING:
			startDebugging(evt);
			break;
		case RBX::RS_PAUSED:
			pauseDebugging(evt);
			break;
		}
	}
	catch (std::runtime_error const& exp)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: %s", exp.what());
	}

	UpdateUIManager::Instance().updateToolBars();
}

void DebuggerClientManager::onDebuggerAdded(boost::shared_ptr<RBX::Instance> spInstance)
{
	if (!m_spDataModel)
		return;

	shared_ptr<RBX::Scripting::ScriptDebugger> spDebugger = RBX::Instance::fastSharedDynamicCast<RBX::Scripting::ScriptDebugger>(spInstance);
	if (spDebugger && spDebugger->getScript())
		getOrCreateDebuggerClient(shared_from(spDebugger->getScript()));
}

void DebuggerClientManager::onDebuggerRemoved(boost::shared_ptr<RBX::Instance> spInstance)
{
	shared_ptr<RBX::Scripting::ScriptDebugger> spDebugger = RBX::Instance::fastSharedDynamicCast<RBX::Scripting::ScriptDebugger>(spInstance);
	if (spDebugger && spDebugger->getScript())
	{
		DebuggerClient* pDebuggerClient = getDebuggerClient(shared_from(spDebugger->getScript()));
		if (pDebuggerClient)
		{
			// this has to be done in Main thread
			if (!m_bDebuggerListUpdateRequested)
			{
				if (pDebuggerClient->isPaused() || pDebuggerClient->hasError())
				{
					m_bDebuggerListUpdateRequested = true;
					QMetaObject::invokeMethod(this, "updateDebuggersList", Qt::QueuedConnection);
				}
			}
			delete pDebuggerClient;
		}
	}
}

void DebuggerClientManager::pauseAllClients(bool forceAll)
{
	static QMutex pauseExecutionMutex;
	QMutexLocker mlock(&pauseExecutionMutex);

	STUDIO_EMIT_SIGNAL executionDataCleared();
	RBX::Scripting::DebuggerManager::singleton().pause();
}

void DebuggerClientManager::resumeAllClients(bool forceAll)
{
	DebuggerClient *pClient = m_pCurrentDebuggerClient;
	
	if (FFlag::StudioRemoveDebuggerResumeLock)
	{
		if (pClient)
			pClient->resetExecution();

		STUDIO_EMIT_SIGNAL executionDataCleared();
		m_pCurrentDebuggerClient = NULL;

		RBX::Scripting::DebuggerManager::singleton().resume();
	}
	else
	{
		RBX::DataModel::LegacyLock lock(m_spDataModel, RBX::DataModelJob::Write);
		if (pClient)
			pClient->resetExecution();

		STUDIO_EMIT_SIGNAL executionDataCleared();
		m_pCurrentDebuggerClient = NULL;

		RBX::Scripting::DebuggerManager::singleton().resume();
	}
	
}

void DebuggerClientManager::resetAllClients()
{
	for (size_t ii = 0; ii < debuggerClients.size(); ++ii)
		debuggerClients[ii]->resetExecution();

	STUDIO_EMIT_SIGNAL executionDataCleared();
}

void DebuggerClientManager::updateDebugActions(bool add)
{
	RobloxMainWindow& rbxMainWindow = UpdateUIManager::Instance().getMainWindow();
	if (rbxMainWindow.isRibbonStyle())
		return;

	if (add)
	{
		if (!m_pSeparatorAction)
			m_pSeparatorAction = rbxMainWindow.runToolBar->insertSeparator(NULL);
		else
			rbxMainWindow.runToolBar->insertAction(NULL, m_pSeparatorAction);

			rbxMainWindow.runToolBar->insertAction(NULL, rbxMainWindow.stepIntoAction);
			rbxMainWindow.runToolBar->insertAction(NULL, rbxMainWindow.stepOverAction);
			rbxMainWindow.runToolBar->insertAction(NULL, rbxMainWindow.stepOutAction);
	}
	else
	{
		rbxMainWindow.runToolBar->removeAction(rbxMainWindow.stepOutAction);
		rbxMainWindow.runToolBar->removeAction(rbxMainWindow.stepOverAction);
		rbxMainWindow.runToolBar->removeAction(rbxMainWindow.stepIntoAction);
		rbxMainWindow.runToolBar->removeAction(m_pSeparatorAction);
	}
}

//--------------------------------------------------------------------------------------------
// DebuggerClient
//--------------------------------------------------------------------------------------------
DebuggerClient::DebuggerClient(boost::shared_ptr<RBX::Instance> script)
: m_spScript(script)
, m_pQDebuggerClient(NULL)
, m_pExtDebuggerClient(NULL)
, m_CurrentFrame(0)
, m_bIgnoreBreakpointAddRemove(false)
, m_bIsActive(false)
, m_currentThreadID(0)
{
	DebuggerClientManager::Instance().addDebuggerClient(this);

	m_spDataModel = RBX::shared_from(RBX::Instance::fastDynamicCast<RBX::DataModel>(m_spScript->getRootAncestor()));

	// addDebugger can throw
	try
	{
		//'addDebugger' returns the current debugger if it is already added
		m_spDebugger = RBX::Scripting::DebuggerManager::singleton().addDebugger(m_spScript.get());
	}
	catch (std::runtime_error const&)
	{
		// make sure we remove DebuggerClient from DebuggerClientManager and rethrow
		DebuggerClientManager::Instance().removeDebuggerClient(this);
		throw;
	}
	
	// add breakpoints
	const RBX::Scripting::ScriptDebugger::Breakpoints& breakpoints = m_spDebugger->getBreakpoints();
	for (RBX::Scripting::ScriptDebugger::Breakpoints::const_iterator iter = breakpoints.begin(); iter != breakpoints.end(); ++iter)
		STUDIO_EMIT_SIGNAL DebuggerClientManager::Instance().breakpointAdded(shared_from(iter->second));

	// add watches
	const RBX::Scripting::ScriptDebugger::Watches& watches = m_spDebugger->getWatches();
	for (RBX::Scripting::ScriptDebugger::Watches::const_iterator iter = watches.begin(); iter != watches.end(); ++iter)
		STUDIO_EMIT_SIGNAL DebuggerClientManager::Instance().watchAdded(shared_from(*iter));

	m_cConnections.push_back(m_spDebugger->resuming.connect(boost::bind(&DebuggerClient::onDebuggerResume, this)));
	m_cConnections.push_back(m_spDebugger->encounteredBreak.connect(boost::bind(&DebuggerClient::onEncounteredBreakpoint, this, _1)));
	m_cConnections.push_back(m_spDebugger->watchAdded.connect(boost::bind(&DebuggerClient::onWatchAdded, this, _1)));
	m_cConnections.push_back(m_spDebugger->breakpointAdded.connect(boost::bind(&DebuggerClient::onBreakpointAdded, this, _1)));
	m_cConnections.push_back(m_spDebugger->breakpointRemoved.connect(boost::bind(&DebuggerClient::onBreakpointRemoved, this, _1)));
	m_cConnections.push_back(m_spDebugger->watchRemoved.connect(boost::bind(&DebuggerClient::onWatchRemoved, this, _1)));
	m_cConnections.push_back(m_spDebugger->scriptErrorDetected.connect(boost::bind(&DebuggerClient::onScriptErrorDetected, this, _1, _2, _3)));

	if (RobloxScriptDoc* pDoc = RobloxDocManager::Instance().findOpenScriptDoc(
		LuaSourceBuffer::fromInstance(script)))
	{
		setDocument(pDoc);
		if (RobloxDocManager::Instance().getCurrentDoc() == pDoc)
			activate();
	}
}

DebuggerClient::~DebuggerClient()
{
	DebuggerClientManager::Instance().removeDebuggerClient(this);

	for (std::list<rbx::signals::connection>::iterator iter=m_cConnections.begin(); iter != m_cConnections.end(); ++iter)
		(*iter).disconnect();
	m_cConnections.clear();

	if (m_pQDebuggerClient)
	{
		// we can have same text editor being used for a different script, so remove all connections as deletion can be delayed
		m_pQDebuggerClient->deActivate();
		// now, request deletion
		m_pQDebuggerClient->deleteLater();
	}
}

void DebuggerClient::setDocument(IRobloxDoc *pDocument)
{
	if (pDocument && m_pQDebuggerClient)
		return;

	if (pDocument)
	{
		ScriptTextEditor* pTextEdit = dynamic_cast<ScriptTextEditor*>(pDocument->getViewer());
		if (pTextEdit)
		{
			m_pQDebuggerClient = new QDebuggerClient(this, pTextEdit);

			//also mark breakpoints in the text editor
			RBX::Scripting::ScriptDebugger::Breakpoints breakpoints = m_spDebugger->getBreakpoints();
			for (RBX::Scripting::ScriptDebugger::Breakpoints::const_iterator iter = breakpoints.begin(); iter != breakpoints.end(); ++iter)
			{
				RBX::Scripting::DebuggerBreakpoint* pBreakpoint = RBX::Instance::fastDynamicCast<RBX::Scripting::DebuggerBreakpoint>(iter->second);
				if(pBreakpoint)
					m_pQDebuggerClient->updateBreakpointState(pBreakpoint->getLine() - 1, RBX::Scripting::DebuggerBreakpoint::prop_Enabled.getValue(pBreakpoint) ? 
				                                                            RBXTextUserData::ENABLED : RBXTextUserData::DISABLED);
			}

			if (m_pExtDebuggerClient)
				m_pQDebuggerClient->setExternalDebuggerClient(m_pExtDebuggerClient);
		}

		//clear code lines as now we will be having text editor to read the values from
		m_ScriptLines.clear();
	}
	else
	{
		if (m_pQDebuggerClient)
			m_pQDebuggerClient->deleteLater();
		m_pQDebuggerClient = NULL;
	}
}

void DebuggerClient::activate()
{
	if (m_bIsActive)
		return;

	if (m_pQDebuggerClient)
		m_pQDebuggerClient->activate();
	DebuggerClientManager::Instance().setActiveDebuggerClient(this);

	m_bIsActive = true;
}

void DebuggerClient::deActivate()
{
	if (!m_bIsActive)
		return;

	if (m_pQDebuggerClient)
		m_pQDebuggerClient->deActivate();
	DebuggerClientManager::Instance().setActiveDebuggerClient(NULL);

	m_bIsActive = false;
}

boost::shared_ptr<RBX::Instance> DebuggerClient::getScript()
{ return m_spScript; }

boost::shared_ptr<RBX::Instance> DebuggerClient::getScript(int frameNo)
{ 
	if (m_pExtDebuggerClient && (m_pExtDebuggerClient != this))
		return m_pExtDebuggerClient->getScript(frameNo);

	if ((frameNo >= 0) && (frameNo < (int)m_CallStackAtPausedLine.size()) && m_CallStackAtPausedLine[frameNo].script)
		return m_CallStackAtPausedLine[frameNo].script;

	return getScript();
}

bool DebuggerClient::updateDocument(bool openDoc, bool updateMarkers)
{
	boost::shared_ptr<RBX::Instance> spScript = getScript(getCurrentFrame());
	if (!spScript)
		return false;

	DebuggerClient* pLuaDebuggerClient(this);
	if (spScript != getScript())
	{
		//if scripts are different then set the external debugger for querying values
		pLuaDebuggerClient = DebuggerClientManager::Instance().getDebuggerClient(spScript);
		//make sure we've a valid debugger client (script can be deleted by now)
		if (!pLuaDebuggerClient)
			return false;
		pLuaDebuggerClient->setExternalDebuggerClient(this);
	}

	if (openDoc)
		RobloxDocManager::Instance().openDoc(LuaSourceBuffer::fromInstance(spScript));

	if (updateMarkers)
	{
		int textEditLine = getCurrentLine() - 1;
		pLuaDebuggerClient->setMarker(textEditLine, ":/16x16/images/Studio 2.0 icons/16x16/debugger_arrow.png", true);
		pLuaDebuggerClient->highlightLine(textEditLine);
	}
	
	return true;
}

void DebuggerClient::setExternalDebuggerClient(DebuggerClient* pExtDebuggerClient)
{
	RBXASSERT(pExtDebuggerClient != this); // just want to see when does this happen?
	if (pExtDebuggerClient != this)
	{
		m_pExtDebuggerClient = pExtDebuggerClient;
		if (m_pQDebuggerClient)
			m_pQDebuggerClient->setExternalDebuggerClient(pExtDebuggerClient);
	}
}

RBX::Reflection::Variant DebuggerClient::getWatchValue(boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch)
{
	// if we have an external client set then get value from the external client
	if (m_pExtDebuggerClient && (m_pExtDebuggerClient != this))
		return m_pExtDebuggerClient->getWatchValue(spWatch);

	if ((isPaused() || hasError()) && m_spDebugger->isAncestorOf(spWatch.get()))
	{
		RBX::DataModel::LegacyLock lock(m_spDataModel, RBX::DataModelJob::Write);
		RBX::Reflection::Variant var = m_spDebugger->getWatchValue(spWatch.get(), getCurrentFrame());		
		return var;
	}
	return RBX::Reflection::Variant();
}

void DebuggerClient::addWatch(const QString& watchKey)
{
	if (watchKey.isEmpty())
		return;

	//first check if watch is already present
	const RBX::Scripting::ScriptDebugger::Watches& watches = m_spDebugger->getWatches();
	for (RBX::Scripting::ScriptDebugger::Watches::const_iterator iter = watches.begin(); iter != watches.end(); ++iter)
	{
		if (QString::compare((*iter)->getExpression().c_str(), watchKey, Qt::CaseSensitive) == 0)
		{
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Watch expression '%s' is already available", qPrintable(watchKey));
			return;
		}
	}

	RBX::DataModel::LegacyLock lock(m_spDataModel, RBX::DataModelJob::Write);
	boost::shared_ptr<RBX::Scripting::DebuggerWatch> spWatch = m_spDebugger->addWatch(watchKey.toStdString());

	try
	{
		spWatch->checkExpressionSyntax();
	}
	catch (std::runtime_error const& exp)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO, "Exception: %s", exp.what());
	}
}

void DebuggerClient::highlightLine(int textEditLine)
{ 
	if (m_pQDebuggerClient) 
		m_pQDebuggerClient->highlightLine(textEditLine); 
}

int DebuggerClient::getCurrentLine()
{ 
	if (m_CallStackAtPausedLine.size() > 0 && m_CurrentFrame < (int)m_CallStackAtPausedLine.size())
		return m_CallStackAtPausedLine[m_CurrentFrame].currentline;

	return (m_spDebugger ? m_spDebugger->getCurrentLine() : -1);
}

void DebuggerClient::setCurrentFrame(int frame)
{
	LuaSourceBuffer scriptToOpen;
	if (m_pExtDebuggerClient)
	{
		DebuggerClientManager::Instance().setActiveDebuggerClient(m_pExtDebuggerClient);

		scriptToOpen = LuaSourceBuffer::fromInstance(m_pExtDebuggerClient->getScript(frame));
		m_pExtDebuggerClient->setCurrentFrame(frame);
	}
	else
	{
		m_CurrentFrame = frame;

		if (m_CallStackAtPausedLine.size() > 0 && m_CurrentFrame < (int)m_CallStackAtPausedLine.size())
		{
			DebuggerClientManager::Instance().setActiveDebuggerClient(this);
			scriptToOpen = LuaSourceBuffer::fromInstance(m_CallStackAtPausedLine[m_CurrentFrame].script);			
		}
	}

	if (scriptToOpen.empty())
		scriptToOpen = LuaSourceBuffer::fromInstance(m_spScript);

	RobloxDocManager::Instance().openDoc(scriptToOpen);
}

void DebuggerClient::setCurrentLine(int textEditLine)
{
	if (m_pQDebuggerClient)
		m_pQDebuggerClient->setCurrentLine(textEditLine);
}

void DebuggerClient::setMarker(int textEditLine, const QString& marker, bool setBlockCurrent)
{
	if (m_pQDebuggerClient) 
		m_pQDebuggerClient->setMarker(textEditLine, marker, setBlockCurrent); 
}

QString DebuggerClient::getSourceCodeAtLine(int breakpointLine)
{
	if (breakpointLine > 0)
	{
		//there's an offset of 1 between breakpoint line and text editor block number
		if (m_pQDebuggerClient)
			return m_pQDebuggerClient->getSourceCodeAtLine(breakpointLine-1);

		try
		{
			if (m_ScriptLines.isEmpty())
			{
				QString scriptCode = LuaSourceBuffer::fromInstance(m_spScript).getScriptText().c_str();
				m_ScriptLines = scriptCode.split("\n");
			}

			if ((breakpointLine - 1) < m_ScriptLines.size())
				return m_ScriptLines[breakpointLine-1]; 
		}
		catch (std::runtime_error const&)
		{
			m_ScriptLines.clear();
		}
	}
	return "";
}

void DebuggerClient::syncBreakpointState(int breakpointLine)
{
	if (!m_pQDebuggerClient)
		return;
	
	RBXTextUserData::eBreakpointState state(RBXTextUserData::NO_BREAKPOINT);
	RBX::Scripting::DebuggerBreakpoint* pBreakpoint = m_spDebugger->findBreakpoint(breakpointLine);
	if (pBreakpoint)
		state = pBreakpoint->isEnabled() ? RBXTextUserData::ENABLED : RBXTextUserData::DISABLED;

	//there's an offset of 1 between breakpoint line and text editor block number
	m_pQDebuggerClient->updateBreakpointState(breakpointLine - 1, state);
}

void DebuggerClient::pauseExecution()
{
	m_pExtDebuggerClient = NULL;
	if (!isPaused())
	{
		m_spDebugger->pause();
		if (m_bIsActive && m_pQDebuggerClient)
			m_pQDebuggerClient->updateEditor();
	}
}

void DebuggerClient::resumeExecution()
{
	m_pExtDebuggerClient = NULL;
	if (m_spDebugger && m_spDebugger->isDebugging())
	{
		m_spDebugger->resume();
		if (m_bIsActive && m_pQDebuggerClient)
			m_pQDebuggerClient->updateEditor();
	}
}

void DebuggerClient::resetExecution()
{
	if (m_pQDebuggerClient)
	{
		m_pQDebuggerClient->clearLineHighlight();
		m_pQDebuggerClient->setMarker(m_spDebugger->getCurrentLine() - 1, "", false);
	}

	boost::shared_ptr<RBX::Instance> spScript = getScript(m_CurrentFrame);
	if (spScript != getScript())
	{
		DebuggerClient* pLuaDebuggerClient = DebuggerClientManager::Instance().getDebuggerClient(spScript);
		if (pLuaDebuggerClient && pLuaDebuggerClient->m_pQDebuggerClient)
		{
			pLuaDebuggerClient->m_pQDebuggerClient->clearLineHighlight();
			pLuaDebuggerClient->m_pQDebuggerClient->setMarker(pLuaDebuggerClient->m_spDebugger->getCurrentLine() - 1, "", false);
		}
	}

	m_pExtDebuggerClient = NULL;

	m_CallStackAtPausedLine.clear();
	m_CurrentFrame = 0;
	m_ErrorMessage = "";
	m_currentThreadID = 0;
}

bool DebuggerClient::isDebugging()
{
	if (m_spDebugger) 
		return m_spDebugger->isDebugging();
	return false;
}

bool DebuggerClient::isPaused()
{
	if (m_spDebugger) 
		return m_spDebugger->isPausedThread(m_currentThreadID);
	return false;
}

bool DebuggerClient::hasError()
{
	if (m_spDebugger)
		return m_spDebugger->isErrorThread(m_currentThreadID);
	return false;
}

void DebuggerClient::syncScriptWithTextEditor(const BreakpointDetails& textEditBreakpoints)
{
	//RBX::DataModel::LegacyLock lock(m_spDataModel, RBX::DataModelJob::Write);

	//delete existing breakpoints
	RBX::Scripting::ScriptDebugger::Breakpoints breakpoints = m_spDebugger->getBreakpoints();
	for (RBX::Scripting::ScriptDebugger::Breakpoints::iterator iter = breakpoints.begin(); iter != breakpoints.end(); ++iter)
		(iter->second)->setParent(NULL);

	//recreate all the breakpoints again!
	boost::shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint;
	for (BreakpointDetails::const_iterator iter = textEditBreakpoints.begin(); iter != textEditBreakpoints.end(); ++iter)
	{
		spBreakpoint = m_spDebugger->setBreakpoint((*iter).line);
		if (spBreakpoint)
		{					
			RBX::Scripting::DebuggerBreakpoint::prop_Enabled.setValue(spBreakpoint.get(), (*iter).isEnabled);
			//RBX::Scripting::DebuggerBreakpoint::prop_Condition.setValue(spBreakpoint.get(), condition); - TODO: add support for condition
		}
	}
}

void DebuggerClient::syncTextEdtiorWithScript()
{
    if (!m_spDebugger)
        return;
    
	RBX::Scripting::ScriptDebugger::Breakpoints breakpoints = m_spDebugger->getBreakpoints();
	for (RBX::Scripting::ScriptDebugger::Breakpoints::iterator iter = breakpoints.begin(); iter != breakpoints.end(); ++iter)
		syncBreakpointState(iter->first);
}

void DebuggerClient::toggleBreakpoint(int breakpointLine)
{
	if(!isValid())
		return;

	RBX::ScopedAssign<bool> ignoreBreakpointAddRemove(m_bIgnoreBreakpointAddRemove, true);
	bool isBreakpointAdded = false;

	RBX::DataModel::LegacyLock lock(m_spDataModel, RBX::DataModelJob::Write);
	RBX::Scripting::DebuggerBreakpoint* pBreakpoint = m_spDebugger->findBreakpoint(breakpointLine);
	if(!pBreakpoint)
	{
		pBreakpoint = m_spDebugger->setBreakpoint(breakpointLine).get();
		STUDIO_EMIT_SIGNAL DebuggerClientManager::Instance().breakpointAdded(shared_from(pBreakpoint));
		isBreakpointAdded = true;
	}
	else
	{
		STUDIO_EMIT_SIGNAL DebuggerClientManager::Instance().breakpointRemoved(shared_from(pBreakpoint));
		pBreakpoint->setParent(NULL);
	}

	//update state in text editor also
	if (m_pQDebuggerClient)
		m_pQDebuggerClient->updateBreakpointState(breakpointLine - 1, isBreakpointAdded ? RBXTextUserData::ENABLED : RBXTextUserData::NO_BREAKPOINT);
}

void DebuggerClient::toggleBreakpointState(int breakpointLine)
{
	RBX::Scripting::DebuggerBreakpoint* pBreakpoint = m_spDebugger->findBreakpoint(breakpointLine);
	if (pBreakpoint)
	{
		RBX::DataModel::LegacyLock lock(m_spDataModel, RBX::DataModelJob::Write);
		RBX::Scripting::DebuggerBreakpoint::prop_Enabled.setValue(pBreakpoint, !pBreakpoint->isEnabled());
	}
}

bool DebuggerClient::isValid()
{ return (m_spDataModel && m_spScript && m_spDebugger); }

int DebuggerClient::getCurrentThread()
{ return m_currentThreadID; }

void DebuggerClient::setCurrentThread(int threadID)
{ 
	if (m_pQDebuggerClient && (threadID != m_currentThreadID))
		m_pQDebuggerClient->clearLineHighlight();

	m_currentThreadID = threadID;
	m_spDebugger->setCurrentThread(threadID);

	const RBX::Scripting::ScriptDebugger::PausedThreads& pausedThreads = getPausedThreads();
	RBX::Scripting::ScriptDebugger::PausedThreads::const_iterator iter = pausedThreads.find(threadID);
	if (iter != pausedThreads.cend())
	{
		m_CallStackAtPausedLine = iter->second.callStack;
		m_ErrorMessage = iter->second.errorMessage;
		//set current frame
		if (m_CallStackAtPausedLine.size() > 0)
			m_CurrentFrame = m_CallStackAtPausedLine[0].frame;
	}
}

bool DebuggerClient::isHighlightingRequired()
{ return ((m_CallStackAtPausedLine.size() > 0) && (m_CallStackAtPausedLine[0].frame == m_CurrentFrame) && (getScript() == getScript(getCurrentFrame()))); }

bool DebuggerClient::isTopFrameCurrent()
{  return ((m_CallStackAtPausedLine.size() > 0) && (m_CallStackAtPausedLine[0].frame == m_CurrentFrame)); }

const RBX::Scripting::ScriptDebugger::PausedThreads& DebuggerClient::getPausedThreads()
{ return m_spDebugger->getPausedThreads(); }

void DebuggerClient::onDebuggerResume()
{ resetExecution(); }

void DebuggerClient::onEncounteredBreakpoint(int breakpointLine)
{
	if(breakpointLine <= 0)
		return;

	if (!m_currentThreadID)
		m_currentThreadID = m_spDebugger->getCurrentThread();

	//notify manager about the breakpoint
	DebuggerClientManager::Instance().breakpointEncounter(this);
}

void DebuggerClient::onScriptErrorDetected(int errorLine, std::string errorMessage, RBX::Scripting::ScriptDebugger::Stack stack)
{
	if (errorLine <= 0)
		return;

	//save current stack information and error message
	m_CallStackAtPausedLine = stack;
	m_ErrorMessage = errorMessage;

	if (!m_currentThreadID)
		m_currentThreadID = m_spDebugger->getCurrentThread();

	//set current frame
	if (m_CallStackAtPausedLine.size() > 0)
		m_CurrentFrame = m_CallStackAtPausedLine[0].frame;
	//notify manager about the breakpoint
	DebuggerClientManager::Instance().breakpointEncounter(this);
}

void DebuggerClient::onBreakpointAdded(shared_ptr<RBX::Instance> spInstance)
{
	if (m_bIgnoreBreakpointAddRemove)
		return;

	shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint = RBX::Instance::fastSharedDynamicCast<RBX::Scripting::DebuggerBreakpoint>(spInstance);
	if (!spBreakpoint || (spBreakpoint->getLine() < 0))
		return;

	STUDIO_EMIT_SIGNAL DebuggerClientManager::Instance().breakpointAdded(spBreakpoint);

	if (m_pQDebuggerClient)
	{
		m_pQDebuggerClient->updateBreakpointState(spBreakpoint->getLine() - 1, 
		                                          RBX::Scripting::DebuggerBreakpoint::prop_Enabled.getValue(spBreakpoint.get()) ? 
					                              RBXTextUserData::ENABLED : RBXTextUserData::DISABLED);
	}
}

void DebuggerClient::onBreakpointRemoved(shared_ptr<RBX::Instance> spInstance)
{
	if (m_bIgnoreBreakpointAddRemove)
		return;

	shared_ptr<RBX::Scripting::DebuggerBreakpoint> spBreakpoint = RBX::Instance::fastSharedDynamicCast<RBX::Scripting::DebuggerBreakpoint>(spInstance);
	if (!spBreakpoint || (spBreakpoint->getLine() <= 0))
		return;
		
	STUDIO_EMIT_SIGNAL DebuggerClientManager::Instance().breakpointRemoved(spBreakpoint);
	if (m_pQDebuggerClient)
		m_pQDebuggerClient->updateBreakpointState(spBreakpoint->getLine() - 1);		
}

void DebuggerClient::onWatchAdded(shared_ptr<RBX::Instance> spInstance)
{
	shared_ptr<RBX::Scripting::DebuggerWatch> spWatch = RBX::Instance::fastSharedDynamicCast<RBX::Scripting::DebuggerWatch>(spInstance);
	if (spWatch)
		STUDIO_EMIT_SIGNAL DebuggerClientManager::Instance().watchAdded(spWatch);
}

void DebuggerClient::onWatchRemoved(shared_ptr<RBX::Instance> spInstance)
{
	shared_ptr<RBX::Scripting::DebuggerWatch> spWatch = RBX::Instance::fastSharedDynamicCast<RBX::Scripting::DebuggerWatch>(spInstance);
	if (spWatch)
		STUDIO_EMIT_SIGNAL DebuggerClientManager::Instance().watchRemoved(spWatch);
}

void DebuggerClient::onScriptStopped()
{
	if (m_pQDebuggerClient)
		m_pQDebuggerClient->clearLineHighlight();
	m_CallStackAtPausedLine.clear();
	m_CurrentFrame = 0;
}

bool DebuggerClient::getValue(const QString& key, RBX::Reflection::Variant& value, int frame)
{
	bool result = false;
		
	try 
	{
		RBX::DataModel::LegacyLock lock(m_spDataModel, RBX::DataModelJob::Write);
		value = m_spDebugger->getKeyValue(key.toStdString(), frame > 0 ? frame : m_CurrentFrame);
		if (!value.isVoid())
			result = true;
	}
	catch (...)
	{
		result = false;
	}

	return result;
}

bool DebuggerClient::isKeyDefined(boost::shared_ptr<const RBX::Reflection::ValueMap> variables, const QString &key, RBX::Reflection::Variant &value)
{
	if(variables && !variables->empty())
	{
		for(RBX::Reflection::ValueMap::const_iterator iter = variables->begin(); iter != variables->end(); ++iter)
		{
			if (QString::compare(iter->first.c_str(), key, Qt::CaseSensitive) == 0)
			{
				value = iter->second;
				return true;
			}
		}
	}
	return false;
}

bool DebuggerClient::actionState(const QString &actionID, bool &enableState, bool &checkedState)
{
	static QString defaultActiveActions("insertBreakpointAction deleteBreakpointAction toggleBreakpointStateAction addWatchAction");
	if (defaultActiveActions.contains(actionID))
	{
		checkedState = false;
		enableState = true;
		return true;
	}

	if (DebuggerClientManager::Instance().actionState(actionID, enableState, checkedState))
		return true;

	static QString defaultDebugActions("stepIntoAction stepOverAction stepOutAction");
	if (defaultDebugActions.contains(actionID))
	{
		DebuggerClient* pClient = this;
		if (m_pExtDebuggerClient)
			pClient = m_pExtDebuggerClient;

		checkedState = false;
		enableState = pClient->isPaused() && !pClient->hasError();
		return true;
	}

	return false;
}

bool DebuggerClient::handleAction(const QString &actionID, bool checkedState)
{
	if (DebuggerClientManager::Instance().handleAction(actionID, checkedState))
		return true;

	return false;
}

//--------------------------------------------------------------------------------------------
// QDebuggerClient
//--------------------------------------------------------------------------------------------
QDebuggerClient::QDebuggerClient(DebuggerClient* pDebuggerClient, ScriptTextEditor* pTextEdit)
: m_pDebuggerClient(pDebuggerClient)
, m_pExtDebuggerClient(NULL)
, m_pBreakpointMenuAction(NULL)
, m_pSeparatorAction1(NULL)
, m_pSeparatorAction2(NULL)
, m_pTextEdit(pTextEdit)
, m_lastBlockCount(pTextEdit->blockCount())
, m_isModifiedByKeyBoard(false)
, m_ignoreAutoRepeatEvent(true)
{
	m_pToolTipWidget = new DebuggerToolTipWidget(m_pTextEdit);
}

QDebuggerClient::~QDebuggerClient()
{	
	m_pToolTipWidget->deleteLater();
	m_pToolTipWidget = NULL;
	m_pTextEdit = NULL;//this will get deleted as a part of ScriptDoc
}

void QDebuggerClient::activate()
{	
	connect(m_pTextEdit, SIGNAL(toggleBreakpoint(int)), this, SLOT(onToggleBreakpoint(int)));
	connect(m_pTextEdit, SIGNAL(showToolTip(const QPoint&, const QString&)), this, SLOT(onShowToolTip(const QPoint&, const QString&)));
	connect(m_pTextEdit, SIGNAL(updateContextualMenu(QMenu*, QPoint)), this, SLOT(onUpdateContextualMenu(QMenu*, QPoint)));
	connect(m_pTextEdit->document(), SIGNAL(contentsChanged()), this, SLOT(onContentsChanged()));

	RobloxMainWindow& rbxMainWindow = UpdateUIManager::Instance().getMainWindow();
	connect(rbxMainWindow.insertBreakpointAction, SIGNAL(triggered()), this, SLOT(onInsertBreakpoint()));
	connect(rbxMainWindow.deleteBreakpointAction, SIGNAL(triggered()), this, SLOT(onDeleteBreakpoint()));
	connect(rbxMainWindow.toggleBreakpointStateAction, SIGNAL(triggered()), this, SLOT(onToggleBreakpointState()));
	connect(rbxMainWindow.addWatchAction, SIGNAL(triggered()), this, SLOT(onAddWatch()));

	if (m_pDebuggerClient->isHighlightingRequired())
		highlightLine(m_pDebuggerClient->getCurrentLine() - 1);
	m_pTextEdit->installEventFilter(this);	
}

void QDebuggerClient::deActivate()
{
	m_pTextEdit->removeEventFilter(this);
	cleanupContextMenu();

	disconnect(m_pTextEdit, SIGNAL(toggleBreakpoint(int)), this, SLOT(onToggleBreakpoint(int)));
	disconnect(m_pTextEdit, SIGNAL(showToolTip(const QPoint&, const QString&)), this, SLOT(onShowToolTip(const QPoint&, const QString&)));
	disconnect(m_pTextEdit, SIGNAL(updateContextualMenu(QMenu*, QPoint)), this, SLOT(onUpdateContextualMenu(QMenu*, QPoint)));
	disconnect(m_pTextEdit->document(), SIGNAL(contentsChanged()), this, SLOT(onContentsChanged()));

	RobloxMainWindow& rbxMainWindow = UpdateUIManager::Instance().getMainWindow();
	disconnect(rbxMainWindow.insertBreakpointAction, SIGNAL(triggered()), this, SLOT(onInsertBreakpoint()));
	disconnect(rbxMainWindow.deleteBreakpointAction, SIGNAL(triggered()), this, SLOT(onDeleteBreakpoint()));
	disconnect(rbxMainWindow.toggleBreakpointStateAction, SIGNAL(triggered()), this, SLOT(onToggleBreakpointState()));
	disconnect(rbxMainWindow.addWatchAction, SIGNAL(triggered()), this, SLOT(onAddWatch()));
}

void QDebuggerClient::highlightLine(int textEditLine)
{ QMetaObject::invokeMethod(this, "highlightLine_MT", Qt::AutoConnection, Q_ARG(int, textEditLine)); }

void QDebuggerClient::clearLineHighlight()
{ QMetaObject::invokeMethod(this, "clearLineHighlight_MT", Qt::AutoConnection); }

void QDebuggerClient::setCurrentLine(int textEditLine)
{ QMetaObject::invokeMethod(this, "setCurrentLine_MT", Qt::AutoConnection, Q_ARG(int, textEditLine)); }

void QDebuggerClient::setMarker(int textEditLine, const QString& marker, bool setBlockCurrent)
{ QMetaObject::invokeMethod(this, "setMarker_MT", Qt::AutoConnection, Q_ARG(int, textEditLine), Q_ARG(QString, marker), Q_ARG(bool, setBlockCurrent)); }

void QDebuggerClient::updateEditor()
{
	if (m_pTextEdit)
		m_pTextEdit->update();
}

QString QDebuggerClient::getSourceCodeAtLine(int textEditLine)
{
	QString text;
	QTextBlock currentBlock = m_pTextEdit->document()->findBlockByNumber(textEditLine);
	if(currentBlock.isValid())
		text = currentBlock.text();

	return text;
}

void QDebuggerClient::updateBreakpointState(int textEditLine, RBXTextUserData::eBreakpointState state)
{ updateTextUserData(textEditLine, state);	}

void QDebuggerClient::onInsertBreakpoint()
{
	QTextBlock block = m_pTextEdit->textCursor().block();
	if (block.isValid())
	{
		m_pDebuggerClient->toggleBreakpoint(block.blockNumber()+1);
		updateTextUserData(block.blockNumber(), RBXTextUserData::ENABLED);
	}
}

void QDebuggerClient::onDeleteBreakpoint()
{
	QTextBlock block = m_pTextEdit->textCursor().block();
	if (block.isValid())
	{
		m_pDebuggerClient->toggleBreakpoint(block.blockNumber()+1);
		updateTextUserData(block.blockNumber(), RBXTextUserData::NO_BREAKPOINT);
	}
}

void QDebuggerClient::onToggleBreakpoint(int textEditLine)
{	m_pDebuggerClient->toggleBreakpoint(textEditLine+1); }

void QDebuggerClient::onToggleBreakpointState()
{
	QTextBlock block = m_pTextEdit->textCursor().block();
	if (block.isValid())
		m_pDebuggerClient->toggleBreakpointState(block.blockNumber()+1);
}

void QDebuggerClient::onAddWatch()
{
	QString watchExpression(m_pTextEdit->textCursor().selectedText());
	// if there's no selected text then get user input
	if (watchExpression.isEmpty())
		watchExpression = QInputDialog::getText(m_pTextEdit, tr("Watch"), tr("Watch Expression:"), QLineEdit::Normal, watchExpression, NULL, Qt::WindowTitleHint | Qt::WindowSystemMenuHint);
	// if we've got an input, add watch
	if (!watchExpression.isEmpty())
	{
		m_pDebuggerClient->addWatch(watchExpression);
		// make sure watch widget is visible
		DebuggerViewsManager::Instance().showDockWidget(DebuggerViewsManager::eDV_WATCH);
	}
}

void QDebuggerClient::highlightLine_MT(int textEditLine)
{
	if(!m_pTextEdit || textEditLine < 0 || textEditLine > m_pTextEdit->blockCount())
		return;

	// do not highlight if there's no breakpoint hit or error
	DebuggerClient* pDebuggerClient = m_pExtDebuggerClient ? m_pExtDebuggerClient : m_pDebuggerClient;
	if (!pDebuggerClient || (!pDebuggerClient->isPaused() && !pDebuggerClient->hasError()))
		return;

	if (m_pToolTipWidget)
		m_pToolTipWidget->hideToolTip();

	QTextBlock blockToSet = m_pTextEdit->document()->findBlockByNumber(textEditLine);
	if(!blockToSet.isValid())
		return;

	//make block current
	setCurrentBlock(blockToSet);

	//highlight block
	highlightBlock(blockToSet);
}

void QDebuggerClient::clearLineHighlight_MT()
{
	// Clear text edit extra selections
	m_pTextEdit->removeFromExtraSelections(m_extraSelections);
	m_extraSelections.clear();
}

void QDebuggerClient::setCurrentLine_MT(int textEditLine)
{
	if(!m_pTextEdit || textEditLine < 0 || textEditLine > m_pTextEdit->blockCount())
		return;

	QTextBlock block = m_pTextEdit->document()->findBlockByNumber(textEditLine);
	if(!block.isValid())
		return;

	//make block current
	setCurrentBlock(block);
}

void QDebuggerClient::setMarker_MT(int textEditLine, QString marker, bool setBlockCurrent)
{
	if(!m_pTextEdit || textEditLine < 0 || textEditLine > m_pTextEdit->blockCount())
		return;

	QTextBlock block = m_pTextEdit->document()->findBlockByNumber(textEditLine);
	if(!block.isValid())
		return;

	//set marker
	RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(block.userData());
	if (pUserData)
		pUserData->setMarker(marker);

	//make block current
	if (setBlockCurrent)
		setCurrentBlock(block);
}

void QDebuggerClient::setCurrentBlock(const QTextBlock &blockToSet)
{
	QTextCursor cursor = m_pTextEdit->textCursor();
	cursor.setPosition(blockToSet.position());
	m_pTextEdit->setTextCursor(cursor);
}

void QDebuggerClient::highlightBlock(const QTextBlock &blockToHighlight)
{
	QTextEdit::ExtraSelection selection;
	selection.format.setProperty(QTextFormat::FullWidthSelection, true);
	QColor backgroundColor(QColor(Qt::yellow).lighter(180));
	DebuggerClient* pDebuggerClient = m_pExtDebuggerClient ? m_pExtDebuggerClient : m_pDebuggerClient;
	if (pDebuggerClient->hasError())
		backgroundColor = QColor(Qt::red).lighter(180);
	selection.format.setBackground(QBrush(backgroundColor));
	selection.cursor = QTextCursor(blockToHighlight);
	selection.cursor.clearSelection();

	if (m_pTextEdit->addToExtraSelections(selection, true))
		m_extraSelections.append(selection);
}

bool QDebuggerClient::onShowToolTip(const QPoint& pos, const QString& key)
{
	DebuggerClient* pDebuggerClient = m_pExtDebuggerClient ? m_pExtDebuggerClient : m_pDebuggerClient;
	if (pDebuggerClient->hasError() && pDebuggerClient->isTopFrameCurrent())
	{		
		QTextCursor cursor = m_pTextEdit->cursorForPosition(m_pTextEdit->viewport()->mapFromGlobal(pos));
		if (cursor.block().isValid() && (cursor.block().blockNumber() == (pDebuggerClient->getCurrentLine() - 1)) && !pDebuggerClient->getErrorMessage().empty())
		{			
			m_pToolTipWidget->showToolTip(pos, "Error", pDebuggerClient->getErrorMessage());
			QTreeWidgetItem* pItem = m_pToolTipWidget->topLevelItem(0);
			if (pItem)
				pItem->setIcon(0, QIcon(":/16x16/images/Studio 2.0 icons/16x16/stop_16.png"));
			return true;
		}
	}

	//if empty key or no debugging, hide any visible tooltip
	// variable name can start with a letter or '_'
	if (key.isEmpty() || (!key.at(0).isLetter() && key.at(0) != '_') || !pDebuggerClient->isDebugging() || !(pDebuggerClient->isPaused() || pDebuggerClient->hasError()))
	{
		m_pToolTipWidget->hideToolTip();
		return false;
	}

	//if the key hasn't been changed just mouse is move then move the tooltip
	if (m_pToolTipWidget->isKeyValid(key))
	{
		m_pToolTipWidget->showAt(pos);
		return true;
	}

	//hide if tooptip is already visible
	m_pToolTipWidget->hideToolTip();
	
	//now get the value corresponding to the key and show tooltip
	RBX::Reflection::Variant value;
	if (!pDebuggerClient->getValue(key, value, m_pDebuggerClient->getCurrentFrame()))
		return false;
	m_pToolTipWidget->showToolTip(pos, key, value);
	return true;
}

void QDebuggerClient::onContentsChanged()
{
	if (m_lastBlockCount != m_pTextEdit->blockCount())
	{
		m_pDebuggerClient->syncScriptWithTextEditor(getBreakpointDetails());
		m_lastBlockCount = m_pTextEdit->blockCount();		
	}
	else if (!m_isModifiedByKeyBoard)
	{
		//update only if contents are not changed by keyboard, it must have been changed by an action (Undo/Redo/Comment Selection etc)
		removeBreakpointState();
		m_pDebuggerClient->syncTextEdtiorWithScript();
	}
}

void QDebuggerClient::updateTextUserData(int textEditLine, RBXTextUserData::eBreakpointState state)
{
	QTextBlock block = m_pTextEdit->document()->findBlockByNumber(textEditLine);
	if(block.isValid())
	{
		RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(block.userData());
		if(pUserData)
		{
			pUserData->setBreakpointState(state);
			updateEditor();
		}
	}
}

void QDebuggerClient::modifyContextualMenu(QMenu* pContextualMenu)
{
	if (!pContextualMenu || m_pBreakpointMenuAction)
		return;

	RobloxMainWindow& rbxMainWindow = UpdateUIManager::Instance().getMainWindow();

	QMenu* pBreakpointMenu = new QMenu(tr("Breakpoint"), pContextualMenu);
	pBreakpointMenu->setObjectName("BreakpointMenu");
	pBreakpointMenu->addAction(rbxMainWindow.insertBreakpointAction);
	pBreakpointMenu->addAction(rbxMainWindow.deleteBreakpointAction);
	pBreakpointMenu->addAction(rbxMainWindow.toggleBreakpointStateAction);

	connect(pBreakpointMenu, SIGNAL(aboutToShow()), &UpdateUIManager::Instance(), SLOT(onMenuShow()));

	m_pBreakpointMenuAction = pContextualMenu->insertMenu(rbxMainWindow.commentSelectionAction, pBreakpointMenu);
	m_pSeparatorAction1 = pContextualMenu->insertSeparator(rbxMainWindow.commentSelectionAction);

	pContextualMenu->insertAction(rbxMainWindow.commentSelectionAction, rbxMainWindow.addWatchAction);
	m_pSeparatorAction2 = pContextualMenu->insertSeparator(rbxMainWindow.commentSelectionAction);
}

void QDebuggerClient::cleanupContextMenu()
{
	QMenu* pContextualMenu = m_pTextEdit->getContextualMenu();
	if (pContextualMenu && m_pBreakpointMenuAction)
	{
		pContextualMenu->removeAction(m_pBreakpointMenuAction); delete m_pBreakpointMenuAction;
		m_pBreakpointMenuAction = NULL;

		QMenu* pBreakpointMenu = pContextualMenu->findChild<QMenu*>("BreakpointMenu");
		if (pBreakpointMenu)
		{
			disconnect(pBreakpointMenu, SIGNAL(aboutToShow()), &UpdateUIManager::Instance(), SLOT(onMenuShow()));
			delete pBreakpointMenu;
		}

		pContextualMenu->removeAction(m_pSeparatorAction1); delete m_pSeparatorAction1; m_pSeparatorAction1 = NULL;
		pContextualMenu->removeAction(m_pSeparatorAction2); delete m_pSeparatorAction2; m_pSeparatorAction2 = NULL;
		pContextualMenu->removeAction(UpdateUIManager::Instance().getMainWindow().addWatchAction);
	}
}

void QDebuggerClient::onUpdateContextualMenu(QMenu* pContextualMenu, QPoint pos)
{
	if (!pContextualMenu)
		return;

	// if required, update contextual menu 
	modifyContextualMenu(pContextualMenu);

	// check if we need to update cursor position
	QTextCursor clickedCursor      = m_pTextEdit->cursorForPosition(pos);
	QTextCursor selectedTextCursor = m_pTextEdit->textCursor();	

	int startPos = selectedTextCursor.selectionStart();
	int endPos   = selectedTextCursor.selectionEnd();

	if ( selectedTextCursor.hasSelection() && (clickedCursor.position() >= startPos) && (clickedCursor.position() <= endPos) )
	{
		// if clicked inside the selected text
		QTextBlock startBlock = m_pTextEdit->document()->findBlock(startPos);
		QTextBlock endBlock   = m_pTextEdit->document()->findBlock(endPos);

		if (startBlock.blockNumber() != endBlock.blockNumber())
		{
			// if blocks number are different i.e. multiple line selection, then disable breakpoint and watch actions
			m_pBreakpointMenuAction->setEnabled(false);
			UpdateUIManager::Instance().getMainWindow().addWatchAction->setEnabled(false);
			return;
		}
	}
	else
	{
		// if clicked outside selected text then set clicked position and remove selection if any
		m_pTextEdit->setTextCursor(clickedCursor);
	}

	// now update relevent actions
	QTextBlock block = m_pTextEdit->textCursor().block();
	if (block.isValid())
	{
		RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(block.userData());
		if(pUserData)
		{
			RBXTextUserData::eBreakpointState state = pUserData->getBreakpointState();

			RobloxMainWindow& rbxMainWindow = UpdateUIManager::Instance().getMainWindow();
			rbxMainWindow.insertBreakpointAction->setVisible(state == RBXTextUserData::NO_BREAKPOINT);
			rbxMainWindow.deleteBreakpointAction->setVisible(state != RBXTextUserData::NO_BREAKPOINT);
			rbxMainWindow.toggleBreakpointStateAction->setVisible(state != RBXTextUserData::NO_BREAKPOINT);

			//update action text appropriately
			if (state == RBXTextUserData::ENABLED)
			{
				rbxMainWindow.toggleBreakpointStateAction->setText(tr("Disable Breakpoint"));
				rbxMainWindow.toggleBreakpointStateAction->setToolTip(tr("Disable Breakpoint"));
			}
			else if (state == RBXTextUserData::DISABLED)
			{
				rbxMainWindow.toggleBreakpointStateAction->setText(tr("Enable Breakpoint"));
				rbxMainWindow.toggleBreakpointStateAction->setToolTip(tr("Enable Breakpoint"));
			}
			
			m_pBreakpointMenuAction->setEnabled(true);
			rbxMainWindow.addWatchAction->setEnabled(true);
		}
	}
}

bool QDebuggerClient::eventFilter(QObject *obj, QEvent *evt)	
{
	if (obj == m_pTextEdit)
	{
		// For scenario mentioned in DE7694: to avoid accidental modification of script text keep on ignoring auto repeat events till it stops
		if (m_ignoreAutoRepeatEvent && ((evt->type() == QEvent::KeyPress) || (evt->type() == QEvent::KeyRelease)))
		{			
			if (static_cast<QKeyEvent*>(evt)->isAutoRepeat())
			{
				evt->accept();
				return true;
			}
			else
			{
				m_ignoreAutoRepeatEvent = false;
			}
		}

		if (evt->type() == QEvent::FocusOut)
		{
			m_ignoreAutoRepeatEvent = true;
		}
		else if ((evt->type() == QEvent::KeyPress) || (evt->type() == QEvent::ShortcutOverride))
		{
			m_isModifiedByKeyBoard = true;
			//if either of the modifiers are set, then user might be using a shortcut or else ignore all contents changed by keyboard
			Qt::KeyboardModifiers modifier = static_cast<QKeyEvent *>(evt)->modifiers();
			if (modifier & Qt::ShiftModifier || modifier & Qt::ControlModifier || modifier & Qt::AltModifier)			
				m_isModifiedByKeyBoard = false;
		}
		else if (evt->type() == QEvent::KeyRelease)
		{
			m_isModifiedByKeyBoard = false;
		}
	}

	return false;
}

DebuggerClient::BreakpointDetails QDebuggerClient::getBreakpointDetails()
{
	DebuggerClient::BreakpointDetails textEditBreakpoints;

	QTextBlock block = m_pTextEdit->document()->firstBlock();
	while (block.isValid())
	{
		RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(block.userData());
		if(pUserData)
		{
			RBXTextUserData::eBreakpointState state = pUserData->getBreakpointState();
			if (state != RBXTextUserData::NO_BREAKPOINT)
				textEditBreakpoints.push_back(DebuggerClient::BreakpointDetail(block.blockNumber() + 1, state == RBXTextUserData::ENABLED ? true: false));
		}

		block = block.next();
	}

	return textEditBreakpoints;
}

void QDebuggerClient::removeBreakpointState()
{
	QTextBlock block = m_pTextEdit->document()->firstBlock();
	while (block.isValid())
	{
		RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(block.userData());
		if(pUserData)
			pUserData->setBreakpointState(RBXTextUserData::NO_BREAKPOINT);

		block = block.next();
	}
}
