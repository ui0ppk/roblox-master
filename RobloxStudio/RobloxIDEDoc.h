/**
 * RobloxIDEDoc.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Qt Headers
#include <QObject>
#include <QString>
#include <QMutex>
#include <QMap>
#include <QVector>
#include <QAction>
#include <QFrame>
#include <QLabel>
#include <QPushButton>

// Roblox Headers
#include "GfxBase/ViewBase.h"
#include "v8datamodel/Game.h"
#include "tool/DragTypes.h"
#include "rbx/signal.h"
#include "rbx/BaldPtr.h"
#include "util/G3DCore.h"
#include "tool/DragTypes.h"

// Roblox Studio Headers
#include "CloudEditAdornable.h"
#include "RobloxBasicDoc.h"
#include "SelectionHighlightAdornable.h"

// 3rd. party
#include "Filewatcher/FileWatcher.h"

class XmlElement;
class RobloxView;
class RobloxTreeWidget;
class QStringList;
class QOgreWidget;
class RobloxMainWindow;
class RobloxDiagnosticsView;
class RecordToggleVerb;
class ScreenshotVerb;
class RobloxChatWidget;
class PlayersDataManager;
class EntityProperties;

class WorkspaceAnnouncementTooltip : public QFrame
{
	Q_OBJECT
public:
	WorkspaceAnnouncementTooltip(QWidget* parent);

	void showText();
	void hideText();

private:
	QPushButton*	        closeButton;
	QLabel*					toolTipLabel;
	QString					lastClosedMessage;

	void setLastClosedMessage(const QString& messageID) { lastClosedMessage = messageID; }
	QString& getLastClosedMessage() { return lastClosedMessage; }

	private Q_SLOTS:
		void closeText();
};

namespace RBX 
{
	class Verb;
	class Name;
	class DataModel;
	class SelectionChanged;
    class Heartbeat;
    class Instance;
    class ViewBase;
	class ViewRbxGfx;
    class LuaSourceContainer;

    namespace Reflection
    {
        class Variant;
        class PropertyDescriptor;
    }
	namespace Scripting
	{
		class ScriptDebugger;
	}
}

class RecentlySavedFilesHandler;

typedef QMap<QString,const RBX::Name*> tActionIDVerbMap;

struct GameState
{
    shared_ptr<RBX::Game>           m_Game;
    QVector<RBX::Verb*>             m_Verbs;
    tActionIDVerbMap                m_VerbMap;
    RBX::BaldPtr<RobloxView>        m_View;
    RBX::BaldPtr<RobloxTreeWidget>  m_TreeWidget;
	shared_ptr<PlayersDataManager>  m_PlayerDataManager;
	RBX::BaldPtr<RBX::Verb>         m_UndoVerb;
	RBX::BaldPtr<RBX::Verb>         m_RedoVerb;
#ifdef _WIN32
	RBX::BaldPtr<RecordToggleVerb>  m_RecordToggleVerb;
#endif	
    rbx::signals::scoped_connection m_ChangeHistoryConnection;
	SelectionHighlightAdornable     m_SelectionHighlightAdornable;
};

class RobloxIDEDoc: public QObject, public RobloxBasicDoc, FW::FileWatchListener
{
    Q_OBJECT

public:
	RobloxIDEDoc(RobloxMainWindow* pMainWindow);
	virtual ~RobloxIDEDoc();

    bool openFile(const QString& fileName, bool asNew);
    bool openStream(const QString& fileName, std::istream* stream, bool asNew);

	bool open(RobloxMainWindow *pMainWindow, const QString &fileName);

    void displayWorkspaceMessage();
	
	void initializeNewPlace();

    void initializeRobloxView();
	
	IRobloxDoc::RBXCloseRequest requestClose();

	IRobloxDoc::RBXDocType docType() { return IRobloxDoc::IDE; }

    QString fileName() const        { return m_fileName; }
	QString displayName() const;
	QString windowTitle() const;
    QString keyName() const         { return "RobloxIDEDoc"; }
    QString initializationScript() const { return m_initializationScript; }
    virtual const QIcon& titleIcon() const;
    virtual const QString& titleTooltip() const;

	QWidget* getViewer();
	
    RBX::DataModel* getDataModel() const
	{
		if (m_CurrentGame && m_CurrentGame->m_Game && m_CurrentGame->m_Game->getDataModel())
			return m_CurrentGame->m_Game->getDataModel().get();
		return NULL;
	}
	shared_ptr<RBX::DataModel> getEditDataModel() const
	{
		return m_EditGame.m_Game ? m_EditGame.m_Game->getDataModel() : shared_ptr<RBX::DataModel>();
	}

	bool isPlaySolo() { return (m_PlayGame.m_Game && m_PlayGame.m_Game->getDataModel()); }
	
	bool isModified();
    bool isSimulating();
    bool isLocalDoc() { return m_bIsLocalDocument; }
	
	bool save();
	bool saveAs(const QString& filename);
    bool autoSave(bool force);
    void setAutoSaveLoad();

	void resetDirty(RBX::DataModel* dataModel);

	static const char* getOpenFileFilters();
	static const char* getSaveFileFilters();
	QString openFileFilters() { return getOpenFileFilters(); }
	QString saveFileFilters() { return getSaveFileFilters(); }
	
	// Material Properties
	static bool displayAskConvertPlaceToNewMaterialsIfInsertNewModel();
	void onWorkspacePropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor);

	void activate();
	void deActivate();
		
	virtual bool doHandleAction(const QString& actionID, bool isChecked);
	bool actionState(const QString &actionID, bool &enableState, bool &checkedState);

	bool handleDrop(const QString &fileName);

	bool handlePluginAction(void *pNotifier, void *pAction);

	void handleScriptCommand(const QString &execCommand);
	void setInitializationScript(const QString& script) { m_initializationScript = script; }

	void onDMItemChanged(shared_ptr<RBX::Instance> item, const RBX::Reflection::PropertyDescriptor* descriptor);

    void enableUndo(bool enable);

    void publish();
    void setFullScreen(bool fullScreen);

    RBX::Reflection::Variant evaluateCommandBarItem(const char* itemToEvaluateshared, shared_ptr<RBX::LuaSourceContainer> script);

	RBX::Verb* getVerb(const QString& name) const;

	void initializeDataModeHash();

    void teleportToURL(QString url, bool play);
	void loadPlayDataModel(QString url, bool play, bool cloneDataModel, bool isTeleport = false);
    void closePlayDataModel();

    void notifyScriptEdited(shared_ptr<RBX::Instance> modifiedScript);

	static bool isEditMode(const RBX::Instance* context);
	bool isCloudEditSession();
	static bool getIsCloudEditSession();

	void forceViewSize(QSize viewSize);

    void setReopenLastSavedPlace(bool reopenLastSavedPlace) { m_ReopenLastSavedPlace = reopenLastSavedPlace; }
	void updateDisplayName(const QString& newDisplayName);

    static const char* lastOpenedPlaceFileSetting() { return "LastOpenedPlaceFile"; }
    static const char* lastOpenedPlaceScriptSetting() { return "LastOpenedPlaceScript"; }
    static const char* lastOpenedPlaceUserID() { return "LastOpenedPlaceUserID"; }

	void forceReloadImages(const QStringList& contentIds);

	void initServerAudioBehavior();
	void setMuteAllSounds(bool mute);

	void promptForExistingAssetId(const std::string& assetType, boost::function<void(int)> resumeFunction, boost::function<void(std::string)> errorFunction);
	boost::shared_ptr<RBX::Instance> getEditScriptByPlayInstance(RBX::Instance* instance);

	QString downloadPlaceCopy();

public Q_SLOTS:
	void exportSelection(std::string filePath = "", RBX::ExporterSaveType exportType = RBX::ExporterSaveType_Selection);
	void exportPlace() { exportSelection("", RBX::ExporterSaveType_Everything); }
    void updateFromPlaceID(int placeId);
	void refreshDisplayName();

private Q_SLOTS:
	void onDelayedInitializeDebuggerData();
	void showImportAssetDialog(QVariant resumeFunctionVar, QVariant errorFunctionVar);

private:
	virtual bool doClose();
	bool loadFromStream(std::istream* stream);
	
	void createActionIDVerbMap(GameState& gameState);
	
	void mapFilesMenu(GameState& gameState);
	void mapEditMenu(GameState& gameState);
	void mapViewMenu(GameState& gameState);
	void mapFormatMenu(GameState& gameState);
	void mapToolMenu(GameState& gameState);
	void mapAdvToolsToolbar(GameState& gameState);
	void mapRunToolbar(GameState& gameState);
	void mapCameraToolbar(GameState& gameState);
	void mapStandardToolbar(GameState& gameState);
	void mapAdvanceBuildToolbar(GameState& gameState);
	void mapInsertMenu(GameState& gameState);
	
	void preProcessVerb(const QStringList &tokens);
	void applyScriptChanges();

	void onIdeRun(bool play);
	void onIdePause();
	void onIdeReset();
	bool isIdeRunEnabled(bool play);
	bool isIdePauseEnabled();
	bool isIdeResetEnabled();

	bool isObjectRenameEnabled();
	void onObjectRename();
	
	void onWaypointChanged();
    Q_INVOKABLE void updateUndoRedo();

	void openUrl(const std::string url);
	void InitializeDefaultsFromSettings();

	void setDockViewsEnabled(bool state = true);
	void preFetchResources();

    void toggleStats(const QString& stats);
    void setStatsEnabled(const QString& stats,bool enabled, QAction* verbAction = NULL);

    void createOptionalDMServices();
	
	void setGridMode(RBX::DRAG::DraggerGridMode gridMode);

	void setDraggerGridMode(RBX::DRAG::DraggerGridMode gridMode);
	void setGridSize(int gridSize);

    void mapActionIDWithVerb(GameState& gameState,const QString &actionID, const RBX::Name &verbName);
	void mapActionIDWithVerb(GameState& gameState,const QString &actionID, const RBX::Verb *pVerb);
	void mapActionIDWithVerb(GameState& gameState,const QString &actionID, const char *verbName);
    RBX::Verb* getVerb(GameState& gameState,const QString& name) const;

	void cleanupGameState(GameState& gameState);

	void onContentDataLoaded();
    void onContentDataSaved();
	
	QString getUploadURL();
	QString getDebugInfoFile(bool isAutoSaveFile = false, const QString& debuggerFileExt = QString());

    bool saveScriptsChangedWhileTesting();
    void patchPlayModelScripts();
    void restoreEditModelScripts();
    void createScriptMapping(shared_ptr<RBX::Instance> instance);
    
    // file watcher listener
    virtual void handleFileAction(FW::WatchID watchid, const FW::String& dir, const FW::String& filename, FW::Action action);

    class PlayModifiedScriptItem
    {
    public:
        PlayModifiedScriptItem() {}
        
        boost::shared_ptr<RBX::Instance> instance;
        rbx::signals::scoped_connection	ancestryChangedConnection;
    };
    
    typedef std::list<PlayModifiedScriptItem*> tPlayModifiedScriptList;
    typedef std::map<RBX::Instance*, boost::shared_ptr<RBX::Instance> > tScriptMap;

    void onPlayModifiedAncestryChanged(boost::shared_ptr<RBX::Instance> script, boost::shared_ptr<RBX::Instance> newParent);

    tPlayModifiedScriptList& getPlayModifiedScriptList() { return m_playModifiedScriptList; }
    void clearPlayModifiedScriptList();

    tScriptMap& getScriptMap() { return m_scriptMap; }
    void addScriptMapping(shared_ptr<RBX::Instance> script, shared_ptr<RBX::DataModel> playDataModel);
    void clearScriptMap() { m_scriptMap.clear(); }    
    
    std::string getScriptIndexHierarchy(shared_ptr<RBX::Instance>,  std::vector<int>& indexHierarchy);
    shared_ptr<RBX::Instance> getScriptByIndexHierarchy(shared_ptr<RBX::DataModel> dataModel,  const std::vector<int>& indexHierarchy, const std::string& serviceName);
    
	Q_INVOKABLE void placeNameLoaded(QString json);

	void initializeVideoRecording(GameState& gameState);
	void cleanupVideoRecording(GameState& gameState);

	void resetMouseCommand(const char* mouseCommandToReset);
	shared_ptr<EntityProperties> cloudEditDetectionAndPlaceLaunch();

	struct DebuggerData
	{
		struct BreakpointData
		{
			int  line;
			bool enabled;
			//std::string condition;
		};

		struct BreakpointLineNumberOrder
		{
			bool operator()(const BreakpointData& lhs, const BreakpointData& rhs) const
			{
				return lhs.line < rhs.line;
			}
		};

		std::list<BreakpointData> breakpoints;
		std::list<std::string> watches;
	};
	typedef std::map<boost::shared_ptr<RBX::Instance>, DebuggerData> tDebuggersMap;
	void saveEditModelDebuggerData();
	void updateEditModelDebuggerData();

	void initializeDebuggerData(const QString &fileName, const shared_ptr<RBX::DataModel> dm, const tDebuggersMap& debuggersMap = tDebuggersMap());
	void updateDebuggerManager(const tDebuggersMap& debuggers);

	void updateDebuggersMap(shared_ptr<RBX::Instance> instance, tDebuggersMap& debuggersMap);
	void addToDebuggersMap(shared_ptr<RBX::Instance> script, tDebuggersMap& debuggersMap);
	DebuggerData getDebuggerData(RBX::Scripting::ScriptDebugger* debugger);

	QString getRecentSaveFile();
	void clearOutputWidget();

	void searchToolboxByDefaultSearchString();
	void onLocalPlayerAdded(shared_ptr<RBX::Instance> newPlayer);
	void onLocalPlayerNameChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor);
	Q_INVOKABLE void syncPlayerName();

	void appendTimeStampToDisplayName();
	void removeTimeStampFromDisplayName();

	void updateOnSimulationStateChange(bool run);
	void setTeamCreateWidgetEnabled(bool state);

	void initCloudEditObjects();	
	void enableDownloadPlaceCopy(bool state);

	GameState                         m_EditGame;
    GameState                         m_PlayGame;
    GameState*                        m_CurrentGame;

#ifdef _WIN32
	RecordToggleVerb                 *m_pRecordToggle;
#endif	

	rbx::signals::scoped_connection   m_ChangeHistoryConnection;
	rbx::signals::scoped_connection	  m_openUrlConnection;
	rbx::signals::scoped_connection	  m_contentDataLoadedConnection;
	rbx::signals::scoped_connection   m_localPlayerConnection;	
	rbx::signals::scoped_connection   m_workspacePropertyChangedConnection;
	rbx::signals::scoped_connection   m_dataModelItemChangedConnection;
	

	boost::shared_ptr<RBX::ViewBase>  m_GameView;
	
	QWidget                          *m_WrapperWidget;
	QOgreWidget                      *m_pQOgreWidget;
	RobloxDiagnosticsView            *m_pDiagViewWidget;
    FW::FileWatcher                  *m_pFileWatcher;
	WorkspaceAnnouncementTooltip      *m_AnnouncementWidget;

	RobloxMainWindow                 *m_pMainWindow;

	boost::scoped_ptr<RecentlySavedFilesHandler> m_pRSFHandler;
	
	QString                           m_fileName;
	QString                           m_displayName;
	QString							  m_initializationScript;
	QString                           m_windowTitlePrefix;

	bool                              m_bIsLocalDocument;

    QString                           m_AutoSaveName;
    bool                              m_IsAutoSaveLoad;
    bool                              m_RequiresSave;
    bool                              m_TestModeEnabled;
    bool                              m_ReopenLastSavedPlace;
    QMutex                            m_IOMutex;

	static int                        sIDEDocCount;	

	QByteArray						  m_initialDataModelHash;
	QByteArray                        m_autoSaveDataModelHash;

    tPlayModifiedScriptList           m_playModifiedScriptList;
    tScriptMap                        m_scriptMap;

	boost::optional<bool>             m_IsCloudEditSession;
	tDebuggersMap                     m_EditDebuggersMap;
	CloudEditAdornable                m_CloudEditAdornable;
	shared_ptr<RobloxChatWidget>      m_ChatWidget;
};

class RecentlySavedFilesHandler
{
public:
	RecentlySavedFilesHandler();
	~RecentlySavedFilesHandler();

	void addFile(const QString& fileName);
	void removeFile(const QString& fileName);

private:
	void updateFileList();
	void saveFileList();
	void readFileList();

	QStringList m_RecentlySavedFiles;
};