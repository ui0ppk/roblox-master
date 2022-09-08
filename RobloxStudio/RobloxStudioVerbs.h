/**
 * RobloxStudioVerbs.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QProcess>
#include <QMutex>

// Roblox Headers
#include "v8tree/Verb.h"
#include "v8datamodel/Commands.h"
#include "MobileDevelopmentDeployer.h"

//Studio Headers
#include "RobloxIDEDoc.h"
#include "RobloxDocManager.h"
#include "PairRbxDeviceDialog.h"
#include "ManageEmulationDeviceDialog.h"
#include "ScriptPickerDialog.h"

namespace RBX {
	class Instance;
	class DataModel;
	class VideoControl;
	class ViewBase;
	class ChangeHistoryService;
	class LuaSourceContainer;
}

class RobloxMainWindow;
class WebDialog;
class IRobloxDoc;
class InsertAdvancedObjectDialog;


class GroupSelectionVerb : public RBX::EditSelectionVerb
{
private:
	typedef RBX::EditSelectionVerb Super;

public:
	GroupSelectionVerb(RBX::DataModel* dataModel);
	virtual bool isEnabled() const;
	virtual void doIt(RBX::IDataState* dataState);
};

class UngroupSelectionVerb : public RBX::EditSelectionVerb
{
private:
	typedef RBX::EditSelectionVerb Super;

public:
	UngroupSelectionVerb(RBX::DataModel* dataModel);
	virtual bool isEnabled() const;
	virtual void doIt(RBX::IDataState* dataState);
};

//////////////////////////////////////////////////////////////////////////
// CSG Operations

class UnionSelectionVerb : public RBX::EditSelectionVerb
{
private:
	typedef RBX::EditSelectionVerb Super;

public:
	UnionSelectionVerb(RBX::DataModel* dataModel);

    void performUnion(RBX::IDataState* dataState);

	virtual bool isEnabled() const;
	virtual void doIt(RBX::IDataState* dataState);
};

class NegateSelectionVerb : public RBX::EditSelectionVerb
{
private:
	typedef RBX::EditSelectionVerb Super;

public:
	NegateSelectionVerb(RBX::DataModel* dataModel);

	virtual bool isEnabled() const;
	virtual void doIt(RBX::IDataState* dataState);
};

class SeparateSelectionVerb : public RBX::EditSelectionVerb
{
private:
	typedef RBX::EditSelectionVerb Super;

public:
	SeparateSelectionVerb(RBX::DataModel* dataModel);

	virtual bool isEnabled() const;
	virtual void doIt(RBX::IDataState* dataState);
    void performSeparate(RBX::IDataState* dataState);
};

class CutVerb : public RBX::DeleteSelectionVerb
{
public:
	CutVerb(RBX::DataModel* dataModel);
	virtual void doIt(RBX::IDataState* dataState);
};

class CopyVerb : public RBX::EditSelectionVerb
{
public:
	CopyVerb(RBX::DataModel* dataModel);
	virtual void doIt(RBX::IDataState* dataState);
};

class PasteVerb :public QObject, public RBX::Verb
{
	Q_OBJECT
public:
	PasteVerb(RBX::DataModel* dataModel, bool pasteInto);
	virtual bool isEnabled() const;
	virtual void doIt(RBX::IDataState* dataState);

private Q_SLOTS:
    void onClipboardModified();
	
private:
	void createInstancesFromClipboard(shared_ptr<RBX::Instances> items);
	void insertInstancesIntoParent(shared_ptr<RBX::Instances> items);
	void createInstancesFromClipboardDep(RBX::Instances& itemsToPaste);
	void insertInstancesIntoParentDep(RBX::Instances& itemsToPaste);
	
	bool isPasteInfoAvailable() const;

	RBX::DataModel *m_pDataModel;
	const bool      m_bPasteInto;
    bool            m_bIsPasteInfoAvailable;
};

class DuplicateSelectionVerb :public QObject, public RBX::EditSelectionVerb
{
	Q_OBJECT
public:
	DuplicateSelectionVerb(RBX::DataModel* dataModel);
	virtual void doIt(RBX::IDataState* dataState);
};

class UndoVerb: public RBX::Verb
{
public:
	UndoVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);

	virtual bool isChecked() const 
	{ return false; }

	virtual bool isEnabled() const;
	virtual std::string getText() const;
private:
	RBX::DataModel*                              m_pDataModel;
	boost::shared_ptr<RBX::ChangeHistoryService> m_pChangeHistory;
};

class RedoVerb: public RBX::Verb
{
public:
	RedoVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);

	virtual bool isChecked() const
	{ return false; }

	virtual bool isEnabled() const;
	virtual std::string getText() const;
private:
	RBX::DataModel*                              m_pDataModel;
	boost::shared_ptr<RBX::ChangeHistoryService> m_pChangeHistory;
};

class InsertModelVerb:public RBX::Verb
{
public:
	InsertModelVerb(RBX::VerbContainer* pVerbContainer, bool insertInto);
	void doIt(RBX::IDataState* dataState);

	virtual bool isChecked() const
	{ return false; }

	virtual bool isEnabled() const { return true; };

private:
	void insertModel();
	
private:
	RBX::DataModel *m_pDataModel;
	const bool      m_bInsertInto;
};

class SelectionSaveToFileVerb:public RBX::Verb
{
public:
	SelectionSaveToFileVerb(RBX::VerbContainer* pVerbContainer);

	virtual void doIt(RBX::IDataState* dataState);
	virtual bool isEnabled() const;
	
private:
	void saveToFile();
	std::auto_ptr<XmlElement> writeSelection();

	RBX::DataModel *m_pDataModel;
};

class PublishToRobloxAsVerb: public RBX::Verb
{
public:
	PublishToRobloxAsVerb(RBX::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd);
    virtual ~PublishToRobloxAsVerb();

	void doIt(RBX::IDataState* dataState);

	virtual bool isChecked() const
	{ return false; }

	virtual bool isEnabled() const;
	void initDialog();

	QDialog* getPublishDialog();
private:
	RBX::DataModel *m_pDataModel;
	RobloxMainWindow *m_pMainWindow;
	WebDialog *m_dlg;
};

class PublishToRobloxVerb : public RBX::Verb
{
public:
	PublishToRobloxVerb(RBX::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd);
	
	virtual void doIt(RBX::IDataState* dataState);
	virtual bool isChecked() const { return false; }
	virtual bool isEnabled() const;
	void onEventPublishingFinished();

private:

    void save(RBX::ContentId contentID,bool* outError,QString* outErrorTitle,QString* outErrorText);

	bool m_bIsPublishInProcess;
	RBX::DataModel *m_pDataModel;
	RobloxMainWindow *m_pMainWindow;
	QMutex publishingMutex;
};

class PublishSelectionToRobloxVerb: public RBX::Verb
{
public:
	PublishSelectionToRobloxVerb(RBX::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd);
    virtual ~PublishSelectionToRobloxVerb();

	void doIt(RBX::IDataState* dataState);
	
	virtual bool isChecked() const
	{ return false; }
	
	virtual bool isEnabled() const;
	
private:
	RBX::DataModel *m_pDataModel;
	RobloxMainWindow *m_pMainWindow;
	WebDialog* m_dlg;
};

class CreateNewLinkedSourceVerb : public QObject, public RBX::Verb
{
	Q_OBJECT
public:
	CreateNewLinkedSourceVerb(RBX::DataModel* pVerbContainer);

	virtual bool isEnabled() const;

	virtual void doIt(RBX::IDataState* dataState);
	
private:
	RBX::DataModel *m_pDataModel;

	shared_ptr<RBX::LuaSourceContainer> getLuaSourceContainer() const;
	void doItThread(std::string source, int currentGameId, boost::optional<int> groupId, QString name, bool* success);
};

class PublishAsPluginVerb: public RBX::Verb
{
public:
	PublishAsPluginVerb(RBX::VerbContainer* pVerbContainer, RobloxMainWindow* mainWnd);

	virtual void doIt(RBX::IDataState* dataState);

	virtual bool isEnabled() const;
	
private:
	RBX::DataModel *m_pDataModel;
	RobloxMainWindow *m_pMainWindow;
	boost::scoped_ptr<WebDialog> m_dlg;
};

class LaunchInstancesVerb: public RBX::Verb
{
public:
	enum SimulationType
	{
		PLAYSOLO = 1,
		SERVERONEPLAYER = 2,
		SERVERFOURPLAYERS = 5
	};

	LaunchInstancesVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);

	virtual bool isEnabled() const { return true; }


private:	
	RBX::VerbContainer *m_pVerbContainer;
};

class StartServerAndPlayerVerb: public RBX::Verb
{
public:
	StartServerAndPlayerVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);

	void launchPlayers(int numPlayers);

private:
	static void launchStudioInstances(RBX::VerbContainer *pVerbContainer, int numPlayers);
	RBX::VerbContainer *m_pVerbContainer;
};

class ServerPlayersStateInitVerb: public RBX::Verb
{
public:
	ServerPlayersStateInitVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);

	virtual bool isChecked() const;
};

class CreatePluginVerb: public RBX::Verb
{
public:
	CreatePluginVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);

	virtual bool isEnabled() const { return true; }

private:	
	RBX::VerbContainer *m_pVerbContainer;
};

class PairRbxDevVerb: public RBX::Verb
{
public:
	PairRbxDevVerb(RBX::VerbContainer* pVerbContainer, QWidget* newParent);
	void doIt(RBX::IDataState* dataState);

	virtual bool isEnabled() const { return true; }

private:
	shared_ptr<PairRbxDeviceDialog> dialog;
	RBX::DataModel *m_pDataModel;
};

class ManageEmulationDevVerb: public RBX::Verb
{
public:
	ManageEmulationDevVerb(RBX::VerbContainer* pVerbContainer, QWidget* newParent);
	void doIt(RBX::IDataState* dataState);

	virtual bool isEnabled() const { return true; }

private:
	shared_ptr<ManageEmulationDeviceDialog> dialog;
	RBX::DataModel *m_pDataModel;
};

class AudioToggleVerb: public RBX::Verb
{
public:
	AudioToggleVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);

	virtual bool isEnabled() const { return true; }
	virtual bool isChecked() const;

private:
	RBX::DataModel *m_pDataModel;
};

class AnalyzePhysicsToggleVerb: public RBX::Verb
{
public:
    AnalyzePhysicsToggleVerb(RBX::DataModel*);
    virtual void doIt(RBX::IDataState* dataState);

    void startAnalyze();
    void stopAnalyze();

    virtual bool isEnabled() const;
    virtual bool isChecked() const;

private:
    RBX::DataModel* m_pDataModel; 
};

class PlaySoloVerb: public RBX::Verb
{

public:
	PlaySoloVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);
	
	virtual bool isEnabled() const { return true; }
		
	
private:	
	RBX::DataModel *m_pDataModel;
};

class StartServerVerb: public RBX::Verb
{
	
public:
	StartServerVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);
	
	virtual bool isEnabled() const { return true; }
	
	
private:	
	RBX::DataModel *m_pDataModel;
};

class StartPlayerVerb: public RBX::Verb
{
	
public:
	StartPlayerVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);
	
	virtual bool isEnabled() const { return true; }
	
	
private:	
	RBX::DataModel *m_pDataModel;
};

class ToggleFullscreenVerb : public RBX::Verb
{
public:
	ToggleFullscreenVerb(RBX::VerbContainer* container);
	virtual void doIt(RBX::IDataState* dataState);
	virtual bool isEnabled() const;
};

class ShutdownClientVerb : public RBX::Verb
{
public:
	ShutdownClientVerb(RBX::VerbContainer* container, IRobloxDoc *pDoc);
	virtual void doIt(RBX::IDataState* dataState);
	
private:
	IRobloxDoc *m_pIDEDoc;
};

class ShutdownClientAndSaveVerb : public RBX::Verb
{
public:
	ShutdownClientAndSaveVerb(RBX::VerbContainer* container, IRobloxDoc* pDoc);
	virtual void doIt(RBX::IDataState* dataState);
	
private:
	IRobloxDoc* m_pIDEDoc;
};

class LeaveGameVerb : public RBX::Verb
{
public:
	LeaveGameVerb(RBX::VerbContainer* container, IRobloxDoc *pDoc);
	virtual void doIt(RBX::IDataState* dataState);
	
private:
	IRobloxDoc *m_pIDEDoc;
};



class ToggleAxisWidgetVerb : public RBX::Verb
{
public:
	ToggleAxisWidgetVerb(RBX::DataModel*);
	virtual void doIt(RBX::IDataState* dataState);
	virtual bool isEnabled() const  { return true; }
	virtual bool isChecked() const;
private:
	RBX::DataModel *m_pDataModel;
};

class Toggle3DGridVerb : public RBX::Verb
{
public:
	Toggle3DGridVerb(RBX::DataModel*);
	virtual void doIt(RBX::IDataState* dataState);
	virtual bool isEnabled() const;
	virtual bool isChecked() const;
private:
	RBX::DataModel *m_pDataModel;
};

class ToggleCollisionCheckVerb : public RBX::Verb
{
public:
	ToggleCollisionCheckVerb(RBX::DataModel*);
	virtual void doIt(RBX::IDataState* dataState);
	virtual bool isEnabled() const { return true; }
	virtual bool isChecked() const;
};

class ToggleLocalSpaceVerb : public RBX::Verb
{
public:
	ToggleLocalSpaceVerb(RBX::DataModel*);
	virtual void doIt(RBX::IDataState* dataState);
	virtual bool isEnabled() const { return true; }
	virtual bool isChecked() const;
private:
	RBX::DataModel* m_pDataModel;
};

class ScreenshotVerb : public QObject, public RBX::Verb
{
	Q_OBJECT
public:
	ScreenshotVerb(RBX::DataModel* pDataModel);

	virtual void doIt(RBX::IDataState*);
	virtual bool isEnabled() const;

	static void DoPostImage(shared_ptr<RBX::DataModel> spDataModel, const QString &fileName, const QString &seoStr);

	void reconnectScreenshotSignal();

private Q_SLOTS:
	void showPostImageWebDialog();
	void copyImageToClipboard();

private:
	void onScreenshotFinished(const std::string &filename);
	void onUploadSignal(bool finished);
	QString getSEOStr();


	static void PostImageFinished(std::string *pResponse, std::exception *pException, weak_ptr<RBX::DataModel> pWeakDataModel);
	static void showMessage(shared_ptr<RBX::DataModel> pWeakDataModel, const char* message);

	boost::shared_ptr<RBX::DataModel>  m_spDataModel;
	QString                            m_fileToUpload;
};

class InsertAdvancedObjectViewVerb: public RBX::Verb
{
public:
	InsertAdvancedObjectViewVerb(RBX::VerbContainer* pVerbContainer);
	virtual ~InsertAdvancedObjectViewVerb();

	void doIt(RBX::IDataState* dataState);
	virtual bool isEnabled() const;
	virtual bool isChecked() const;
};

class JointToolHelpDialogVerb: public RBX::Verb
{
    
public:
	JointToolHelpDialogVerb(RBX::VerbContainer* pVerbContainer);
	void doIt(RBX::IDataState* dataState);
    
	virtual bool isEnabled() const { return true; }
	virtual bool isChecked() const { return false; }
};

class StudioMaterialVerb: public RBX::MaterialVerb
{
public:
	StudioMaterialVerb(RBX::DataModel* dataModel);
	void doIt(RBX::IDataState* dataState);

	virtual bool isEnabled() const { return true; }
	virtual bool isChecked() const;

	static bool sMaterialActionActAsTool;
};

class StudioColorVerb: public RBX::ColorVerb
{
public:
	StudioColorVerb(RBX::DataModel* dataModel);
	void doIt(RBX::IDataState* dataState);

	virtual bool isEnabled() const { return true; }
	virtual bool isChecked() const;

	static bool sColorActionActAsTool;

private:
	void addColorToIcon();
};

class OpenToolBoxWithOptionsVerb: public QObject, public RBX::Verb
{
	Q_OBJECT
public:
	OpenToolBoxWithOptionsVerb(RBX::VerbContainer* pVerbContainer);
	~OpenToolBoxWithOptionsVerb();

	void doIt(RBX::IDataState* dataState);
	virtual bool isEnabled();

private Q_SLOTS:
	void handleDockVisibilityChanged(bool isVisible);
};

class InsertBasicObjectVerb: public RBX::Verb
{
public:
	InsertBasicObjectVerb(RBX::DataModel* dataModel);
	void doIt(RBX::IDataState* dataState);

	virtual bool isEnabled();

private:
	RBX::DataModel *m_pDataModel;
};

class JointCreationModeVerb: public RBX::Verb
{
public:
	JointCreationModeVerb(RBX::DataModel* dataModel);
	void doIt(RBX::IDataState* dataState);	

private:
	void updateMenuIcon();
	void updateMenuActions();
};

#ifdef Q_WS_WIN32   // must be Q_WS_WIN32 so moc picks it up

class RecordToggleVerb:public QObject, public RBX::Verb
{
	Q_OBJECT
public:
	RecordToggleVerb(RBX::DataModel*, RBX::ViewBase* pViewGfx);
	~RecordToggleVerb();

	virtual bool isEnabled() const;
	virtual bool isChecked() const;
	virtual bool isSelected() const;
	virtual void doIt(RBX::IDataState* dataState);

	void startRecording();
	void stopRecording(bool showUploadDialog = true);
	bool isRecording() const;

private Q_SLOTS:
	void uploadVideo();

private:
	void action();

	boost::scoped_ptr<RBX::VideoControl> m_pVideoControl;
	boost::scoped_ptr<boost::thread>     m_helperThread;

	RBX::DataModel                      *m_pDataModel; 
	boost::function<void()>              m_job;

	RBX::CEvent                          m_jobWait;
	RBX::CEvent                          m_jobDone;
	RBX::CEvent                          m_threadDone;

	bool                                 m_bStop;
	bool                                 m_bIsBusy;
};

#endif

class ExportSelectionVerb:public QObject, public RBX::Verb
{
	Q_OBJECT
public:
	ExportSelectionVerb(RBX::DataModel*);

	virtual bool isEnabled() const { return (RobloxDocManager::Instance().getPlayDoc() != NULL); }
	virtual bool isChecked() const { return false; }
	virtual bool isSelected() const { return false; }
	virtual void doIt(RBX::IDataState* dataState);

private:
	RBX::DataModel                      *m_pDataModel;
};

class ExportPlaceVerb : public QObject, public RBX::Verb
{
	Q_OBJECT
public:
	ExportPlaceVerb(RBX::DataModel*);

	virtual bool isEnabled() const { return (RobloxDocManager::Instance().getPlayDoc() != NULL); }
	virtual bool isChecked() const { return false; }
	virtual bool isSelected() const { return false; }
	virtual void doIt(RBX::IDataState* dataState);

private:
	RBX::DataModel						*m_pDataModel;
};

class LaunchHelpForSelectionVerb : public RBX::Verb
{
public:
	LaunchHelpForSelectionVerb(RBX::DataModel*);

	virtual bool isEnabled() const;
	virtual void doIt(RBX::IDataState* dataState);

private:
	RBX::DataModel						*m_pDataModel;
};

class DummyVerb : public RBX::Verb {
public:
	DummyVerb(RBX::VerbContainer* container, const char* name)
	:Verb(container, name)
	{
	}
	virtual bool isEnabled() const {return false;}
	
	virtual void doIt(RBX::IDataState* dataState) {}
};
