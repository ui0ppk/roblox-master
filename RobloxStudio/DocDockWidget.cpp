/**
 * DocDockWidget.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "DocDockWidget.h"

// Qt Headers
#include <QSettings>
#include <QMouseEvent>

// Roblox Studio Headers
#include "RobloxMainWindow.h"
#include "DocDockManager.h"
#include "RobloxDocManager.h"

#include "StudioUtilities.h"

static const char* DocDockWidgetSetting = "DocDockWidget/";

DocDockWidget::DocDockWidget(
    RobloxMainWindow&   mainWindow,
    DocDockManager&     docManager,
    IRobloxDoc&         doc )
    : QDockWidget(&mainWindow),
      m_MainWindow(mainWindow),
      m_DocManager(docManager),
      m_Doc(doc),
      m_DocType(doc.docType()),
      m_IsMoving(false),
      m_Viewer(doc.getViewer()),
      m_IsSimulatingDrag(false)
{
    RBXASSERT(m_Viewer);

    setAttribute(Qt::WA_DeleteOnClose);
    setWidget(m_Viewer);
    setFloating(true);
    setFocusPolicy(Qt::StrongFocus);
    setTitle(m_Doc.displayName(),m_Doc.fileName());
    setMinimumSize(m_Viewer->minimumSize());
    setMaximumSize(m_Viewer->maximumSize());
    setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);

    connect(this,SIGNAL(topLevelChanged(bool)),this,SLOT(onTopLevelChanged(bool)));
    connect(this,SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),this,SLOT(onDockLocationChanged(Qt::DockWidgetArea)));

    // restore geometry for this type of doc
    const QSettings settings;
    const QString settingName = DocDockWidgetSetting + QString::number(m_DocType) + "/geometry";
    restoreGeometry(settings.value(settingName).toByteArray());
    
    connect(qApp,SIGNAL(focusChanged(QWidget*,QWidget*)),this,SLOT(onFocusChanged(QWidget*,QWidget*)));

    show();
}

/**
 * Saves geometry for the next time this type of doc is opened.
 *  Must call cleanup() before destruction.
 */
DocDockWidget::~DocDockWidget()
{
    RBXASSERT(!widget());

    QSettings settings;
    const QString settingName = DocDockWidgetSetting + QString::number(m_DocType) + "/geometry";
    settings.setValue(settingName,saveGeometry());
}

/**
 * Cleans up the widget and makes sure it won't be deleted.
 *  Must be called before the destructor.  We need to do an immediate clean up, we can't
 *  wait for the destructor to be called since we use deleteLater.
 */
void DocDockWidget::cleanup()
{
    if ( !widget() )
        return;

    stopDragging();

	setFloating(false);
    
    // make sure we're not the ones deleting the viewer
    setWidget(NULL);
    QCoreApplication::removePostedEvents(m_Viewer);
    m_Viewer->setParent(NULL);
    m_Viewer->hide();
    m_MainWindow.removeDockWidget(this);
}

/**
 * Simulates a window dragging operation.
 */
void DocDockWidget::startDragging()
{
    if ( !m_IsSimulatingDrag )
    {
        m_IsSimulatingDrag = true;
        m_StartPos = QCursor::pos();
        grabMouse();
        simulateDrag();
    }
}

/**
 * Begins a simulated window drag.
 *  This forces the window to be moved as the cursor is moved until the left button is released.
 */
void DocDockWidget::simulateDrag()
{
    if ( m_IsSimulatingDrag )
    {
        const QPoint& worldPos = QCursor::pos();
        const QPoint offset(
            width() / 2,
            style()->pixelMetric(QStyle::PM_TitleBarHeight) / 2 );
        move(worldPos - offset);
    }
}

/**
 * Changes the dock widget title when the doc names change.
 *  Updates the object name to the new title.
 */
void DocDockWidget::setTitle(const QString& displayName,const QString& fileName)
{
    QString title = displayName;
    if ( !fileName.isEmpty() )
        title += " - " + fileName;
    setWindowTitle(title);
    setObjectName(title);
}

/**
 * Callback for user clicking on the attach button.
 *  Attaches doc to the tab widget.
 */
void DocDockWidget::onAttachDoc()
{
    RobloxDocManager::Instance().attachDoc(m_Doc);
}

/**
 * Requests doc close so that it will be saved properly.
 */
void DocDockWidget::closeEvent(QCloseEvent* event)
{
    RBXASSERT(widget());

    if ( m_MainWindow.requestDocClose(m_Doc) )
        QDockWidget::closeEvent(event);
    else
        event->ignore();
}

/**
 * Callback when the window is being moved.
 *  This causes dragging to begin and the tab hover over location to be detected.
 */
void DocDockWidget::moveEvent(QMoveEvent* event)
{
    QDockWidget::moveEvent(event);

    // ignore move event if we're not floating
    if ( !isFloating() )
        return;

    if ( !m_IsMoving )
    {
        m_IsMoving = true;
        m_StartPos = QCursor::pos() + event->oldPos() - event->pos();
        setMouseTracking(true);
    }
    else
    {
        // only hover over if we attempted to move the window a minimum distance
        const QPoint moveDelta = QCursor::pos() - m_StartPos;
        if ( moveDelta.manhattanLength() > QApplication::startDragDistance() )
            RobloxDocManager::Instance().setDockHoverOverPos(QCursor::pos());
    }

	m_MainWindow.updateEmbeddedFindPosition();
}

/**
 * Callback when the window is being resized.
 *  This causes dragging to abort.
 */
void DocDockWidget::resizeEvent(QResizeEvent* event)
{
    QDockWidget::resizeEvent(event);
    m_StartPos = QCursor::pos();    // ignore any dragging that might have happened
    stopDragging();
}

/**
 * Callback for the mouse moving during a drag.
 *  If we're simulating a drag, move the window.
 */
void DocDockWidget::mouseMoveEvent(QMouseEvent* event)
{
    QDockWidget::mouseMoveEvent(event);

    if ( m_IsSimulatingDrag )
        simulateDrag();
    else if ( event->button() != Qt::LeftButton )
        stopDragging();
}

/**
 * Ends the window drag operation.
 *  Attempts to attach the dock to the tab widget if over the tab widget.
 *
 * @return true if the doc was attached
 */
bool DocDockWidget::stopDragging()
{
    bool attached = false;
    
    if ( m_IsMoving || m_IsSimulatingDrag )
    {
        if ( m_IsSimulatingDrag )
        {
            releaseMouse();
            m_IsSimulatingDrag = false;
        }
        
        setMouseTracking(false);
        m_IsMoving = false;

		// only attach if we attempted to move the window and didn't just click on it
        const QPoint moveDelta = QCursor::pos() - m_StartPos;
        QPoint clickPos;
        if ( moveDelta.manhattanLength() > QApplication::startDragDistance() )
            clickPos = QCursor::pos();
        else
            clickPos = QPoint(INT_MAX,INT_MAX); // don't want to attach it
            
        attached = RobloxDocManager::Instance().attemptAttach(m_Doc,clickPos);
        if ( !attached )
            RobloxDocManager::Instance().setCurrentDoc(&m_Doc);
    }
    
    return attached;
}

/**
 * Handles non-client mouse press events to begin window dragging and setting the
 *  current doc if the window is activated.
 */
bool DocDockWidget::event(QEvent* event)
{
    bool result = QDockWidget::event(event);
    
    if ( event->type() == QEvent::NonClientAreaMouseButtonRelease ||
         event->type() == QEvent::MouseButtonRelease )
    {
        // Must catch mouse events here because dock widget doesn't give em to us in mouesReleaseEvent.
        // On Mac, this is the only way to detect an end window drag event.
        RBX::BaldPtr<QMouseEvent> mouseEvent = static_cast<QMouseEvent*>(event);
        if ( mouseEvent->button() == Qt::LeftButton )
        {
            stopDragging();
            result = true;
        }
    }
    else if ( event->type() == QEvent::NonClientAreaMouseButtonPress ||
              event->type() == QEvent::MouseButtonPress )
    {
        // Reset start pos in case this is just a click.
        RBX::BaldPtr<QMouseEvent> mouseEvent = static_cast<QMouseEvent*>(event);
        if ( mouseEvent->button() == Qt::LeftButton )
            m_StartPos = QCursor::pos();
    }
    else if ( event->type() == QEvent::WindowActivate && widget() )
    {
        RobloxDocManager::Instance().setCurrentDoc(&m_Doc);
    }

    return result;
}

/**
 * Detect changes in widget focus.
 *  Only if our new focus is an ancestor of the dockwidget do we need to set the current doc.
 */
void DocDockWidget::onFocusChanged(QWidget* oldWidget,QWidget* newWidget)
{
    if ( isAncestorOf(newWidget) && widget() )
        RobloxDocManager::Instance().setCurrentDoc(&m_Doc);
}

#ifdef Q_WS_WIN32
/**
 * Windows event callback to handle the end of the window drag operation.
 *  This is the only way to detect when the mouse button has been released during a
 *  drag on Windows.
 */
bool DocDockWidget::winEvent(MSG* msg,long* result)
{
    if ( msg->message == WM_EXITSIZEMOVE )
        stopDragging();

    return QDockWidget::winEvent(msg,result);
}
#endif

/**
 * Callback when the dock is either docked or undocked (plugged or unplugged).
 *  Begin dragging when undocked.
 *  Stop dragging if it's been docked. This is just overkill.
 */
void DocDockWidget::onTopLevelChanged(bool TopLevel)
{
    if ( TopLevel )
        startDragging();
    else
        stopDragging();
}

/**
 * Callback when the dock widget docks somewhere else.
 *  Stop dragging when it's been docked.  This is just overkill.
 */
void DocDockWidget::onDockLocationChanged(Qt::DockWidgetArea area)
{
    stopDragging();
}


