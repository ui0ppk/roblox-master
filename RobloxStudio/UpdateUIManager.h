/**
 * UpdateUIManager.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once 

// Qt Headers
#include <QMap>
#include <QBitArray>
#include <QStringList>
#include <QVector>
#include <QSize>
#include <QPointer>
#include <QButtonGroup>

// 3rd Party Headers
#include "boost/function.hpp"

// Roblox Headers
#include "rbx/Debug.h"

// Roblox Studio Headers
#include "IRobloxDoc.h"

class QDockWidget;
class QBitArray;
class QLabel;
class QAction;
class QShowEvent;
class QHideEvent;
class QMutex;
class QProgressBar;
class QEventLoop;
class QStackedWidget;

class RobloxMainWindow;
class RobloxTreeWidget;
class UpdateToolbarThread;
class RobloxPlayersWidget;
class RobloxChatWidget;

class UpdateUIManager: public QObject
{
	Q_OBJECT

public:
	static QString defaultCommands;
	static UpdateUIManager& Instance();

	RobloxMainWindow& getMainWindow() const	{ return *m_pRobloxMainWindow; }

	QAction* getDockAction(const EDockWindowID id) const;
	QAction* getAction(const QString &actionName);
	template<class T> 
	T& getViewWidget(EDockWindowID id) const
	{
		RBXASSERT(m_ViewWidgets.contains(id));
		RBXASSERT(m_ViewWidgets[id]);
		QWidget* widget = m_ViewWidgets[id];
		RBXASSERT(dynamic_cast<T*>(widget));
		return static_cast<T&>(*widget);
	}
	void setupDockWidgetsViewMenu(QAction& insertBefore);

	void setMenubarEnabled(bool state);

	void updateAction(QAction& action);
	void updateActions(const QList<QAction*>& actions);
	
	void pauseStatusBarUpdate();
	void resumeStatusBarUpdate();

    void setBusy(bool busy, bool useWaitCursor = true);
    bool isBusy() const { return m_BusyState != 0; }
    QProgressBar* showProgress(const QString& message,int min = 0,int max = 0);
    void hideProgress(QProgressBar* progressBar);
    void waitForLongProcess(const QString& message,boost::function<void()> userWorkFunc);
    void updateProgress(QProgressBar* progressBar,int value = -1);

	void commonContextMenuActions(QList<QAction*>& commonActions, bool insertIntoPasteMode = true );

	void setStatusLabels(QString sTimeLabel, QString sFpsLabel);
	QList<QString> getDockActionNames() const;
	void setDefaultApplicationState();
	bool toggleDockView(const QString& actionName);
    void updateBuildMode();
	void updateDockActionsCheckedStates();
	bool get3DAxisEnabled();
	void set3DAxisEnabled(bool value);
	bool get3DGridEnabled();
	void set3DGridEnabled(bool value);
	void saveDocksGeometry();
	void loadDocksGeometry();
	void setDockVisibility(EDockWindowID dockId, bool visible);
	QDockWidget* getDockWidget(EDockWindowID dockId);

    bool isLongProcessInProgress();

	QWidget* getExplorerWidget();
	RobloxPlayersWidget* getPlayersWidget();
	QStackedWidget* getChatWidgetStack();

public Q_SLOTS:
	void onMenuShow();
	void onMenuHide();
    void showErrorMessage(QString title, QString message);

	void onQuickInsertFocus();
	void updateToolBars();

private Q_SLOTS:
	void updateStatusBar();
	void onToolBarsUpdate();    

	void filterExplorer();
	void updateActionWidgetVisibility();

private:
	
	UpdateUIManager();
    virtual ~UpdateUIManager();
	
	virtual bool eventFilter(QObject * watched, QEvent * evt);
	void shutDown();
	void init(RobloxMainWindow *pMainWindow);
	void initDocks();
	
    void updateToolbarTask();
	
	void modifyActionState(QAction& action, bool enabledState, bool checkedState);
	void modifyButtonGroupState(QButtonGroup &pButtonGroup);

    void setupStatusBar();
	void setupSlots();
	
	QString getDockGeometryKeyName(EDockWindowID dockId) const;

	void configureDockWidget(QDockWidget* dockWidget);

    void exectueProgressFunction(
        boost::function<void()> userWorkFunc,
        QEventLoop*             eventLoop,
        QMutex*                 eventLoopMutex,
        bool*                   done );

	QLabel*                 m_pTaskSchedulerSBLabel;
	QLabel*                 m_pTimeSBLabel;
	QLabel*                 m_pCPUSBLabel;
	QLabel*                 m_pRAMSBLabel;
	QLabel*                 m_pFPSSBLabel;

	RobloxMainWindow*       m_pRobloxMainWindow;
	int                     m_PauseStatusBar;
	bool                    m_isRunning;

	struct SDockData
	{
		SDockData() {}

		SDockData(QAction* updateAction, bool defaultVisible, QSize defaultSize = QSize())
			: UpdateAction(updateAction), DefaultVisible(defaultVisible), DefaultSize(defaultSize)
		{}

		QAction*  UpdateAction;
		bool      DefaultVisible;
		QSize	  DefaultSize;
	};
	QMap<EDockWindowID,SDockData>       m_DockData;
	QMap<EDockWindowID,QDockWidget*>    m_DockWidgets;
	QMap<EDockWindowID,QWidget*>        m_ViewWidgets;
	QMap<QString, EDockWindowID>		m_actionDockMap;
	QBitArray                           m_dockRestoreStates;

    QVector<QProgressBar*>              m_ProgressBars;
    QVector<QLabel*>                    m_ProgressLabels;
    int                                 m_BusyState;
    QMap<QAction*,bool>                 m_BusyEnabledActions;
    QPointer<QWidget>                   m_BusyFocusWidget;

	QPointer<RobloxPlayersWidget>       m_PlayersWidget;
	QPointer<QStackedWidget>            m_ChatWidgetStack;

    static bool                         m_longProcessInProgress;

#if WINVER >= 0x601
    // support for Visa/Win7 task bar progress and icons

    void configureTaskBar(QProgressBar* progressBar);
    QProgressBar*                       m_taskProgressBar;
    bool                                m_isTaskBarInitialized;
    ITaskbarList3*                      m_taskbar;
    int                                 m_totalProgress;
    bool                                m_isIndeterminateProgress;
   
#endif

	// TODO - fix this, friends bad
	//define RobloxMainWindow as friend
	friend class RobloxMainWindow;
};
