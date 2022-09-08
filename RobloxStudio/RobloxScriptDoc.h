/**
 * RobloxScriptDoc.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

#include <list>
#include <map>

// Qt Headers
#include <QIcon>
#include <QPlainTextEdit>
#include <QSyntaxHighlighter>
#include <QStyle>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

// Roblox Headers
#include "rbx/signal.h"
#include "rbx/BaldPtr.h"

// Roblox Studio Headers
#include "LuaSourceBuffer.h"
#include "RobloxBasicDoc.h"

namespace RBX 
{
	class Instance;
	class RunService;
	class DataModel;
	namespace Reflection
	{
		class PropertyDescriptor;
	}
}

class QTextBlock;
class RobloxScriptDoc;
class RobloxIDEDoc;
class ScriptTextEditor;
class CheckSyntaxThread;

class RBXTextUserData : public QTextBlockUserData
{
public:

	enum eBreakpointState
	{
		NO_BREAKPOINT,
		ENABLED,
		DISABLED
	};

	RBXTextUserData():m_foldState(0), m_lineState(0), m_BreakpointState(NO_BREAKPOINT) {}

	void setFoldState(int state) { m_foldState = state; }
	int getFoldState() const { return m_foldState;}

	void setLineState(int state) { m_lineState = state; }
	int getLineState() const { return m_lineState; }

	void setBreakpointState(eBreakpointState state) { m_BreakpointState = state; }
	eBreakpointState getBreakpointState() { return m_BreakpointState; }

	void setMarker(const QString& iconName) { m_MarkerIcon = iconName; }
	QString getMarker() { return m_MarkerIcon; }

private:

	QString             m_MarkerIcon;
	int					m_foldState;
	int					m_lineState;
	eBreakpointState  	m_BreakpointState;
};

class RobloxScriptDoc: public QObject, public RobloxBasicDoc
{
	Q_OBJECT

public:

	RobloxScriptDoc();
	virtual ~RobloxScriptDoc();
   
 	bool open(RobloxMainWindow *pMainWindow, const QString &fileName);
	
	IRobloxDoc::RBXCloseRequest requestClose();

	IRobloxDoc::RBXDocType docType() { return IRobloxDoc::SCRIPT; }

    QString fileName() const;
    QString displayName() const { return m_displayName; }
    QString keyName() const     { return m_keyName; }
	virtual const QIcon& titleIcon() const;
			
	bool save();
	bool saveAs(const QString &fileName);

	void setScript(shared_ptr<RBX::DataModel> dm, LuaSourceBuffer script);

	ScriptTextEditor* getTextEditor() { return m_pTextEdit; }

	QString saveFileFilters();
	
	QWidget* getViewer();
	
	bool isModified();

	void activate();

	virtual void updateUI(bool state);
		
	virtual bool doHandleAction(const QString& actionID, bool isChecked);
	bool actionState(const QString &actionID, bool &enableState, bool &checkedState);

	bool handleDrop(const QString &fileName);
	bool handlePluginAction(void *, void *) { return false; }
	void handleScriptCommand(const QString &execCommand);

	void applyEditChanges();

    static void init(RobloxMainWindow& mainWindow);

	LuaSourceBuffer getCurrentScript();
	shared_ptr<RBX::DataModel>	getDataModel() { return m_dataModel; }

	void refreshEditorFromSourceBuffer();
    
private Q_SLOTS:

	void requestScriptDeletion();
	void onSelectionChanged();
    void deActivate();
    void reloadLiveScript();
	void explainBreakpointsDisabled();
	void editingContents();
	void onScriptNamePropertyChanged(const QString& newName);
	void onScriptSourcePropertyChanged(const QString& newSource);
	
private:
	bool doClose();
	void setupScriptConnection();
	void deactivateConnection();
	void activateConnection();
	void disconnectScriptConnection();
	QString buildTabTitle();

	void onAncestryChanged(boost::shared_ptr<RBX::Instance> newParent);
	void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* desc);

	void maybeUpdateText(const QString& code);
	void removeScriptLock();

	static void setMenuVisibility(bool visible);
	
	rbx::signals::scoped_connection		m_ancestryChangedConnection;
	rbx::signals::scoped_connection		m_propertyChangedConnection;
	LuaSourceBuffer                     m_script;
	boost::shared_ptr<RBX::DataModel>	m_dataModel;

	RBX::BaldPtr<RobloxMainWindow>   	m_pMainWindow;
    RBX::BaldPtr<RobloxIDEDoc> 			m_pParentDoc;
	RBX::BaldPtr<ScriptTextEditor>  	m_pTextEdit;
	
	QString             m_fileName;
	QString             m_displayName; //to be used if file name is empty
    QString             m_keyName;

	static int          sScriptCount;
	static int          slastActivatedRibbonPage;

	bool				m_undoAvailable;
	bool				m_redoAvailable;
	bool				m_copyAvailable;
	bool				m_updatingText;
	bool                m_currentlyAttemptingToGetScriptLock;
	bool                m_haveCloudEditScriptLock;
    
	friend class ScriptTextEditor;
};

