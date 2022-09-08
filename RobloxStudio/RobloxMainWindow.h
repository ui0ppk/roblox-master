/**
 * RobloxMainWindow.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once 

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Qt Headers
#include <QMutex>
#include <QEvent>
#include <QThread>
#include <QWaitCondition>
#include <QStringList>
#include <QBitArray>
#include <QTimer>
#include <QMainWindow>
#include <QComboBox>
#include <QString>
#include <QTime>

// Boost Headers
#include <boost/scoped_ptr.hpp>

// Roblox Headers
#include "v8datamodel/StudioPluginHost.h"
#include "rbx/Debug.h"
#include "rbx/BaldPtr.h"
#include "tool/DragTypes.h"
#include "Roblox.h"
#include "StudioAnalytics.h"
#include "util/Http.h"

// Roblox Studio Headers
#include "ui_RBXMainWindow.h"
#include "IRobloxDoc.h"
#include "ScriptComboBox.h"
#include "RobloxRibbonMainWindow.h"

static const char* FileLocationArgument = "-fileLocation";
static const char* ScriptArgument       = "-script";
static const char* UrlArgument          = "-url";
static const char* TicketArgument       = "-ticket";
static const char* EventArgument        = "-event";
static const char* LaunchPlayerArgument = "-launchPlayerMode";
static const char* IDEArgument          = "-ide";

class QLabel;
class QSplashScreen;
class QWebView;

class CustomToolButton;
class RobloxInputConfigDialog;
class RobloxPropertyWidget;
class RobloxPluginHost;
class RobloxSettingsDialog;
class RobloxDiagnosticsView;
class RobloxScriptReview;
class RobloxTaskScheduler;
class RobloxWebDoc;
class InsertObjectWidget;
class InsertServiceDialog;
class RobloxToolBox;
class RobloxTextOutputWidget;
class ShortcutHelpDialog;
class WebDialog;

namespace RBX
{
	class CEvent;
}

enum eBuildMode
{
    BM_BASIC,       // build mode
    BM_ADVANCED,    // edit mode
    BM_LASTMODE,
};

struct InsertObjectItem
{
	std::string contentId;
	std::vector<weak_ptr<RBX::Instance> > weakInstances;

	InsertObjectItem(std::string contentId, std::vector<weak_ptr<RBX::Instance> > weakInstances)
	{
		this->contentId = contentId;
		this->weakInstances = weakInstances;
	}
};

class RobloxMainWindow: public RobloxRibbonMainWindow
{
	Q_OBJECT

	StudioAnalytics studioAnalytics;

public:

	static const int MAX_DOC_WINDOWS = 10; // the app will only remember the first opened doc windows in window menu.

	RobloxMainWindow(const QMap<QString, QString> argMap);
    virtual ~RobloxMainWindow();

    QStackedWidget& treeWidgetStack() const             { return *stackedWidget; }
    CustomToolButton& materialToolButton() const        { return *m_pMaterialToolButton;  }
    CustomToolButton& fillColorToolButton() const       { return *m_pFillColorToolButton; }
    RobloxPluginHost& pluginHost() const                { return *m_pPluginHost; }
    QAction& fullScreenAction() const                   { return *actionFullScreen; }
    void enableScriptCommandInput(bool enabled)         { m_pScriptComboBox->setEnabled(enabled); }
    void setGridMode(RBX::DRAG::DraggerGridMode gridMode);

	StudioAnalytics* getStudioAnalytics() { return &studioAnalytics; }

	static RobloxMainWindow* get(QObject* context);

    QString getDialogTitle() const;

    InsertServiceDialog& insertServiceDialog() const { return *m_pInsertServiceDlg; }

	// This does not belong in here, but will be removed as soon as QT counters are removed
	static void sendCounterEvent( std::string counterName, bool fireImmediately = true);

	void saveApplicationStates();
	void loadApplicationStates();

    bool requestDocClose(IRobloxDoc& doc, bool closeIfLastDoc = true);
	Q_INVOKABLE void closePlayDoc();
    Q_INVOKABLE void saveAndClose();
    Q_INVOKABLE void forceClose();

	// pass pointer in invokable mode since making it a metatype is a pain
    Q_INVOKABLE bool requestDocClose(IRobloxDoc* doc) { return requestDocClose(*doc); }

    bool handleFileOpen(
        const QString&          fileName,
        IRobloxDoc::RBXDocType  type,
        const QString&          script = "" );

    void updateWindowTitle();
	void updateRecentFilesList(const QStringList& files);
	void updateInternalWidgetsState(QAction* pAction, bool enabledState, bool checkedState);

	void setToolbarPosition();
	RobloxWebDoc* getConfigureWebDoc();
	void closeConfigureWebDoc();
	void closePublishGameWindow();

    RobloxTextOutputWidget* getOutputWidget()   { return m_pTextOutput; }

    RobloxPluginHost* getPluginHost() const 	{ return m_pPluginHost; }
	void setPluginHost(RobloxPluginHost* val) 	{ m_pPluginHost = val; }

	eBuildMode getBuildMode() const 			{ return m_BuildMode; }
	bool isRibbonStyle() const 					{ return m_isRibbon; }

	QAction* getActionByName(const QString& actionName);

	QAction* currentOpenedfiles[MAX_DOC_WINDOWS];

	bool commonSlotShortcut(QAction* action, bool isChecked);

    void onIDEDocViewInitialized();

	void setActionCounterSendingEnabled(bool value) { preventActionCounterSending = !value; }

	void updateShortcutSet();
	bool isShortcut(const QKeySequence& keySequence);

	void updateEmbeddedFindPosition();

	virtual void moveEvent(QMoveEvent * event);

	void trackToolboxInserts(RBX::ContentId contentId, const RBX::Instances& instances);
	void shutDownCloudEditServer();

public Q_SLOTS:

    bool fileSave(IRobloxDoc* pDoc = NULL);

	bool openRecentFile(const QString& fileName);
	void openStartPage(bool checked, QString optionalQueryParams = "");
	
	//common slot for handling document related actions
	void commonSlot(bool isChecked);

	bool commonSlotQuickAccess(QAction* action);

	void sendActionCounterEvent(const QString& action);

	void notifyCloudEditConnectionClosed();
	void cloudEditStatusChanged(int placeId, int universeId);

private Q_SLOTS:

	void logIntervalCounter();
	void sendOffCounters();

    //function marshaling
	void processAppEvent(void *pClosure);

	void fileNew();
	void fileOpen();
	void fileClose();
	bool fileSaveAs(IRobloxDoc *pDoc=NULL);
	bool filePublishedProjects();
	bool openRecentFile();
	void publishGame();
	void fileOpenRecentSaves();

	void showInsertServiceDialog();
	void showFindAllDialog();

	void instanceDump();

	void openSettingsDialog();
	void openPluginsFolder();
	void managePlugins();

	void about();
	void onlineHelp();
    void shortcutHelp();
	void openObjectBrowser(bool checked);
	void fastLogDump();

	void toggleFullScreen(bool change);

	void executeScriptFile();

	void causeCrash();

	void onCustomToolButton(const QString& selectedItem);

	void onMenuActionHovered(QAction* action);

    void onMinuteTimer();

    void onDeleteSplashScreen();

    void toggleBuildMode();
    
    void onCommandToolBarTopLevelChanged(bool topLevel);

	void loadDefaultApplicationState();

	void cleanupPlayersAndServers();

	void cookieConstraintCheckerLoadFinished(bool ok);

	void checkInsertedObjects();

private:
	std::set<QString> shortcutSet;

    bool checkUpdater(bool showUpdateOptionDialog, const QString& initDoneEventName) const;
    
    virtual bool eventFilter(QObject * watched, QEvent * evt);
    virtual void closeEvent(QCloseEvent* evt);
    virtual void contextMenuEvent(QContextMenuEvent* evt);
    
	//drag-drop related override
    virtual void dragEnterEvent(QDragEnterEvent *evt);
    virtual void dragMoveEvent(QDragMoveEvent *evt);
    virtual void dropEvent(QDropEvent *evt);
    virtual void dragLeaveEvent(QDragLeaveEvent *evt);

	void setupLogging();
    void setupViewMenu();
	void setupSlots();
	void setupCommandToolBar();
	void setupCounterSender();
    void setupActionCounters();
    void initializeUI();

	void assignAccelerators();

	void setWindowLayout();

	void parseCommandLineOptions(const QMap<QString,QString> argMap);
	void handleCommandLineOptions();
	void handleFileOpenSendCounter(const QString& fileName);
	bool verifyFilePermissions(const QString &fileName);

    QAction& getViewAction(EDockWindowID ID) const;

    void setBuildMode(eBuildMode buildMode);
	void setupCustomToolButton();

    void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor);

	void saveDefaultApplicationState();

    void trackUserActive();
    void onAnalyticTracked();
    void keepAliveAnalyticsSession();
	void updateRecentFilesUI();

	bool commonSlotHelper(const QString& objectName, bool isChecked);

	void checkInternetConnectionSendCounter(RBX::Http& robloxRequest, RBX::Http& externalRequest);
    
	// Args that may be passed in
	QString					fileLocationArg;
	QString					scriptArg;
	QString					urlArg;
	QString					ticketArg;
	QString					startEventArg;
    QString                 showEventArg;
    QString                 readyEventArg;

    ScriptComboBox*      m_pScriptComboBox;
    CustomToolButton*       m_pFillColorToolButton;
    CustomToolButton*       m_pMaterialToolButton;

    RBX::BaldPtr<InsertServiceDialog>           m_pInsertServiceDlg;
    RBX::BaldPtr<RobloxSettingsDialog>          m_pSettingsDialog;
    RBX::BaldPtr<ShortcutHelpDialog>            m_pShortcutHelpDialog;
    RBX::BaldPtr<RobloxInputConfigDialog>       m_pInputConfigDialog;
    RBX::BaldPtr<QSplashScreen>                 m_splashScreen;

    RobloxPluginHost*                m_pPluginHost;
	boost::scoped_ptr<RobloxWebDoc>  m_managePluginsDoc;
	boost::scoped_ptr<RobloxWebDoc>  m_configureGameDoc;
	WebDialog*                       m_publishGameDialog;

	// Application state args
	static QString			sGeometryKey;
	static QString			sWindowStateKey;
	static bool				sIsAppRunning;
	static bool				preventActionCounterSending;

    static const QString    NEW_PLACE_FILENAME;     // Used to indicate to the new process (when opening multiple docs) that this is an empty place we're opening
	static const int		MAX_RECENT_FILES = 6; 	// the app will remember the 15 most recent opened files, but only show the 6 latest in the file menu
	static const int		MAX_RECENT_FILES_SHOWN = 6;
	
	QAction*				recentOpenedFiles[MAX_RECENT_FILES];
	QAction*				separator;   // a separator for recently opened files.
	QTimer*		            m_pMinutesPlayedTimer;

    rbx::signals::scoped_connection m_PropertyChangedConnection;
    rbx::signals::scoped_connection m_AnalyticTrackedConnection;
	boost::scoped_ptr<RBX::CEvent> m_cloudEditAwaitingShutdown;

    RobloxTextOutputWidget* m_pTextOutput;

	QTimer*		            m_pCounterSenderTimer;
    QTime					m_lastAnalyticKeepAliveTime;
	QTime					m_lastAnalyticTrackedTime;

    eBuildMode              m_BuildMode;

	WebDialog*				m_pPublishedProjectsWebDialog;	

    bool                    m_IsInitialized;
	int                     m_AutoSaveAccum;

	bool					m_isRibbon; // whether we're in ribbon bar mode or classic
    bool                    m_fileOpenHandled;
	bool                    m_IgnoreCloudEditDisconnect;
	QWebView*               m_cookieConstraintChecker;
	QEventLoop*             m_cookieConstraintCheckDone;

	std::queue<InsertObjectItem> m_insertObjectItems;

    // TODO - fix this, friends bad
	friend class RbxWorkspace;
	friend class ShutdownClientVerb;
	friend class RobloxBrowser;
public:
	PersistentVariable m_editScriptActions, m_mouseActions, m_inserts;
};

