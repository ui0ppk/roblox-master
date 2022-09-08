/**
 * DocTabWidget.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "DocTabManager.h"

// Qt Headers
#include <QToolButton>
#include <QMouseEvent>
#include <QResizeEvent>

// Roblox Studio Headers
#include "RobloxMainWindow.h"
#include "RobloxDocManager.h"
#include "StudioUtilities.h"
#include "RobloxTabWidget.h"
#include "RobloxIDEDoc.h"

static const int ValidAttachRegionHeight = 256;

DocTabManager::DocTabManager(RobloxMainWindow& mainWindow)
    : QObject(&mainWindow),
      m_RubberBand(QRubberBand::Rectangle),
      m_MainWindow(mainWindow),
      m_MouseDown(false)
{
    m_TabWidget = new RobloxTabWidget(&m_MainWindow);
    
    m_RubberBand.setParent(m_TabWidget);
    m_RubberBand.move(0,0);

    // create some buttons in the upper right

    RBX::BaldPtr<QWidget> cornerWidget = new QWidget(m_TabWidget);
    RBX::BaldPtr<QLayout> layout = new QHBoxLayout(cornerWidget);
    layout->setContentsMargins(0,0,0,0);
    layout->setSpacing(0);
    cornerWidget->setLayout(layout);
    cornerWidget->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);

    m_TabWidget->setCornerWidget(cornerWidget);
    m_TabWidget->setTabsClosable(true);
    m_TabWidget->setMovable(false);  // do not enable - causing redraw problems when detaching
    m_TabWidget->setTabShape(QTabWidget::Rounded);
    m_TabWidget->setDocumentMode(true);
    m_TabWidget->installEventFilter(this);

    m_TabWidget->getTabBar().setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
	m_TabWidget->getTabBar().installEventFilter(this);

    connect(m_TabWidget,SIGNAL(currentChanged(int)),this,SLOT(onCurrentChanged(int)));
    connect(m_TabWidget,SIGNAL(tabCloseRequested(int)),this,SLOT(onTabCloseRequested(int)));
    
    m_FakeTab = new QWidget(m_TabWidget);
    m_FakeTab->hide();

	connect(qApp,SIGNAL(focusChanged(QWidget*,QWidget*)),this,SLOT(onFocusChanged(QWidget*,QWidget*)));

	m_MainWindow.setCentralWidget(m_TabWidget);
    
#ifdef Q_WS_MAC
    setupTabPageSwitchingShortcuts();
#endif
}

DocTabManager::~DocTabManager()
{
    RBXASSERT(m_Docs.isEmpty());
}

/**
 * Adds a new tab for the doc.
 */
void DocTabManager::addDoc(IRobloxDoc& doc, int insertAtIndex)
{
    RBXASSERT(m_Docs.indexOf(&doc) == -1);
    RBXASSERT(doc.getViewer());

    RBX::BaldPtr<QWidget> widget = doc.getViewer();
    RBXASSERT(widget);

	if (insertAtIndex != -1)
	{
		m_Docs.insert(insertAtIndex, &doc);
	}
	else
	{
		m_Docs.append(&doc);
	}

    m_TabWidget->setUpdatesEnabled(false);

	const int index = m_TabWidget->insertTab(insertAtIndex,widget,doc.titleIcon(),doc.displayName());
	RBXASSERT((index != -1) && (index < m_Docs.size()));
    
	m_TabWidget->setTabToolTip(index, !doc.fileName().isEmpty() ? doc.fileName() : doc.displayName());
    m_TabWidget->setCurrentWidget(widget);
    m_TabWidget->setUpdatesEnabled(true);
}

/**
 * Removes the tab for the doc but does not delete the doc.
 * 
 * @return false if the doc is not handled by this manager.
 */
bool DocTabManager::removeDoc(IRobloxDoc& doc)
{
    const int index = m_Docs.indexOf(&doc);
    if ( index == -1 )
        return false;

    m_Docs.remove(index);
    m_TabWidget->removeTab(index);
    return true;
}

/**
 * Changes the tab's window title.
 * 
 * @return false if the doc is not handled by this manager.
 */
bool DocTabManager::renameDoc(IRobloxDoc& doc,const QString& text,const QString& tooltip,const QIcon& icon)
{
    const int index = m_Docs.indexOf(&doc);
    if ( index == -1 )
        return false;

    m_TabWidget->setTabText(index,text);
    m_TabWidget->setTabToolTip(index,tooltip);
    m_TabWidget->setTabIcon(index, icon);
    return true;
}

/**
 * Sets the current tab index, causing the doc to be set current.
 * 
 * @return false if the doc is not handled by this manager.
 */
bool DocTabManager::setCurrentDoc(IRobloxDoc& doc)
{
    const int index = m_Docs.indexOf(&doc);
    if ( index == -1 )
        return false;

    m_TabWidget->setCurrentIndex(index);
    return true;
}

/**
 * Gets the current selected tab.
 * 
 * @return doc if tab is selected, NULL if no tab is selected or doc not found
 */
IRobloxDoc* DocTabManager::getCurrentDoc() const
{
    const int index = m_TabWidget->currentIndex();
    if ( index == -1 )
        return NULL;

    if ( m_FakeTab->isVisible() )
        return NULL;

    return m_Docs[index];
}

/**
 * Callback for the current tab selected changing.
 *  If the index is valid, the doc is set current.
 */
void DocTabManager::onCurrentChanged(int index)
{
    if ( index == -1 )
    {
        RobloxDocManager::Instance().setCurrentDoc(NULL);
    }
    else if ( !m_FakeTab->isVisible() && index < m_Docs.size())
    {
        RBX::BaldPtr<IRobloxDoc> doc = m_Docs[index];
        RobloxDocManager::Instance().setCurrentDoc(doc);
    }
}

/**
 * Callback for user clicking on the close button for the tab.
 *  Requests the doc to be closed.
 */
bool DocTabManager::onTabCloseRequested(int index)
{   
    // it's possible to have already removed the doc but not the tab yet
    if ( index == -1 )
        return false;

    RBX::BaldPtr<IRobloxDoc> doc = m_Docs[index];

    if (doc->docType() == IRobloxDoc::IDE)
    {
        RobloxIDEDoc* ideDoc = static_cast<RobloxIDEDoc*>(doc.get());
        ideDoc->setReopenLastSavedPlace(false);
    }

    bool hasClosed = m_MainWindow.requestDocClose(*doc);

    if (!hasClosed)
    {
        if (doc->docType() == IRobloxDoc::IDE)
        {
            RobloxIDEDoc* ideDoc = static_cast<RobloxIDEDoc*>(doc.get());
            ideDoc->setReopenLastSavedPlace(true);
        }
    }

    return hasClosed;
}

/**
 * Callback if the user clicks on the close button in the corner widget.
 *  Requests doc to be closed.
 */
void DocTabManager::onCurrentTabCloseRequested()
{
    onTabCloseRequested(m_TabWidget->currentIndex());
}

/**
 * Callback for user clicking the detach button.
 *  Detaches tab from doc and creates a dock widget.
 */
void DocTabManager::onDetachDoc()
{
    RBX::BaldPtr<IRobloxDoc> doc = getCurrentDoc();
    if ( doc )
        RobloxDocManager::Instance().detachDoc(*doc,false);
}

/**
 * Event filter for the tab bar.
 *  Handles mouse events for dragging the tab.
 *  Handles resizing the tab widget to update the rubber band size.
 */
bool DocTabManager::eventFilter(QObject* object,QEvent* event)
{
	bool handled = false;

    if ( object == m_TabWidget && event->type() == QEvent::Resize )
    {
		handled = m_TabWidget->eventFilter(object,event);
        QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
        m_RubberBand.resize(resizeEvent->size());
    }
    else if ( object == &m_TabWidget->getTabBar() )
    {
		handled = m_TabWidget->eventFilter(object,event);
        switch ( event->type() )
        {
            case QEvent::MouseButtonPress:
                onMousePressEvent(static_cast<QMouseEvent*>(event));
                break;
            case QEvent::MouseMove:
                onMouseMoveEvent(static_cast<QMouseEvent*>(event));
                break;
            case QEvent::MouseButtonRelease:
                onMouseReleaseEvent(static_cast<QMouseEvent*>(event));
                break;
            default:
                break;
                
        }
    }
	else
	{
		handled = QObject::eventFilter(object,event);
	}

    return handled;
}

/**
 * Callback for mouse press event from the tab bar.
 *  Begins dragging if the left button is down.
 */
void DocTabManager::onMousePressEvent(QMouseEvent* event)
{
    const int index = m_TabWidget->getTabBar().tabAt(event->pos());
    if ( index != -1 && event->button() == Qt::LeftButton )
    {
        m_DragTabIndex = index;
        m_MouseDown = true;
        m_ClickPos = event->pos();
    }

    m_TabWidget->setFocus();
}

/**
 * Callback for the mouse movement event from the tab bar.
 *  If the mouse button is down and the vertical distance is far enough, the tab is detached.
 */
void DocTabManager::onMouseMoveEvent(QMouseEvent* event)
{
    const QPoint deltaPos = event->pos() - m_ClickPos;
    
    // Make the drag distance longer than typical because Doc tabs are easy to mistakenly
    // tear off.
    const int dragDistanceFudge = 32;
    if ( m_MouseDown && deltaPos.manhattanLength() > QApplication::startDragDistance() + dragDistanceFudge)
    {
        RobloxDocManager::Instance().detachDoc(*m_Docs[m_DragTabIndex],true);
        m_MouseDown = false;
    }
}

/**
 * Callback for the mouse button release from the tab bar.
 *  Resets the mouse down flag if left button is released.
 */
void DocTabManager::onMouseReleaseEvent(QMouseEvent* event)
{
    if ( event->button() == Qt::LeftButton )
        m_MouseDown = false;
}

/**
 * Sets the current state of the mouse hover over.
 *  When a dock widget is hovered over the tab widget while dragging,
 *  the tab widget shows if the dock can be reattached.
 */
void DocTabManager::setDockHoverOverPos(const QPoint& globalPos)
{
    if ( isValidHoverOver(globalPos) )
    {
        m_RubberBand.setVisible(true);
        m_RubberBand.resize(m_TabWidget->size());

		// add tab if it's not visible yet
		if ( !m_FakeTab->isVisible() )
			m_TabWidget->addTab(m_FakeTab,"New Tab");

		// get faketab index
		int	fakeTabIndex = m_TabWidget->indexOf(m_FakeTab);
		if (fakeTabIndex == -1)
			return;

		QPoint localPointInTabWidget(m_TabWidget->getTabBar().mapFromGlobal(globalPos).x(), m_TabWidget->getTabBar().height()/2);
		int moveOnTabIndex = m_TabWidget->getTabBar().tabAt(localPointInTabWidget);
		// check if cursor is on some other tab page
		if (moveOnTabIndex != -1 && (moveOnTabIndex != fakeTabIndex))
		{
			QRect moveOnTabRect = m_TabWidget->getTabBar().tabRect(moveOnTabIndex);
			moveOnTabRect.setWidth(m_TabWidget->getTabBar().tabRect(fakeTabIndex).width());

			// make sure after moving tabs cursor will remain inside the fake tab or else we will have flicker
			if (moveOnTabRect.contains(localPointInTabWidget, true))
			{
				bool enabled = m_TabWidget->updatesEnabled();
				m_TabWidget->getTabBar().setUpdatesEnabled(false);
				m_TabWidget->getTabBar().moveTab(fakeTabIndex, moveOnTabIndex);
				// correct index to set as current
				fakeTabIndex = moveOnTabIndex;
				// while moving tabs, fake tab some how gets hidden (?) so again remove and re-insert
				m_TabWidget->removeTab(fakeTabIndex);
				m_TabWidget->insertTab(fakeTabIndex, m_FakeTab, "New Tab");
				m_TabWidget->getTabBar().setUpdatesEnabled(enabled);
			}
		}
		// set current index
		m_TabWidget->setCurrentIndex(fakeTabIndex);
    }
    else
    {
        m_RubberBand.setVisible(false);
        if ( m_FakeTab->isVisible() )
        {
			RBXASSERT(m_TabWidget->indexOf(m_FakeTab) != -1);
			m_TabWidget->removeTab(m_TabWidget->indexOf(m_FakeTab));
        }
    }
}

/**
 * Attempts to attach a dock widget.
 *  If the mouse position is inside the region allowable for reattachment,
 *  the doc is reattached to the tab bar.
 * 
 * @param   doc         doc to attach
 * @param   worldPos    position the mouse button was released
 */
bool DocTabManager::attemptAttach(IRobloxDoc& doc,const QPoint& globalPos)
{
    m_RubberBand.setVisible(false);
	int fakeTabIndex = -1;
    
    if ( m_FakeTab->isVisible() )
	{
		fakeTabIndex = m_TabWidget->indexOf(m_FakeTab);
		m_TabWidget->removeTab(fakeTabIndex);
	}
    
    if ( isValidHoverOver(globalPos) )
    {
        RobloxDocManager::Instance().attachDoc(doc, fakeTabIndex);
        return true;
    }
    else
    {
        return false;
    }
}

/**
 * Checks if a location is valid for attaching a dock to a tab.
 *  Only the upper left corner is valid.
 */
bool DocTabManager::isValidHoverOver(const QPoint& globalPos)
{
    const QPoint mainLocalPos = m_MainWindow.mapFromGlobal(globalPos);
    const QWidget* child = m_MainWindow.childAt(mainLocalPos);
    
    // check if we're visible first
    if ( child && (child == m_TabWidget || m_TabWidget->isAncestorOf(child)) )
    {
        // check if the location is inside the valid region
        QRect worldRect(m_MainWindow.mapToGlobal(m_TabWidget->pos()),m_TabWidget->size() / 2);
        const int validHeight = qMin(ValidAttachRegionHeight,m_TabWidget->height());
        worldRect.setHeight(validHeight);
        return worldRect.contains(globalPos);
    }
    else
        return false;
}

/**
 * Detect changes in widget focus.
 *  Only if our new focus is an ancestor of the tab manager do we set the current tab
 *  as the current doc.
 */
void DocTabManager::onFocusChanged(QWidget* oldWidget,QWidget* newWidget)
{
    if ( m_TabWidget->isAncestorOf(newWidget) && 
         m_TabWidget->currentIndex() != -1 &&
         m_TabWidget->currentIndex() < m_Docs.size() )
    {
        RBX::BaldPtr<IRobloxDoc> doc = m_Docs[m_TabWidget->currentIndex()];
        RobloxDocManager::Instance().setCurrentDoc(doc);
    }
}

/**
 * Resets the central widget for the main window back to the tab manager.
 */
void DocTabManager::restoreAsCentralWidget()
{
    if ( m_MainWindow.centralWidget() != m_TabWidget )
    {        
        QWidget* oldCentralWidget = m_MainWindow.centralWidget();
        m_MainWindow.setCentralWidget(m_TabWidget);
        QApplication::removePostedEvents(oldCentralWidget);  // don't allow to be deleted
        m_TabWidget->show();

        IRobloxDoc* playDoc = RobloxDocManager::Instance().getPlayDoc();
        RBXASSERT(playDoc);
        addDoc(*playDoc);
    }
}

/**
 * Sets up tab page switching shortcuts on Mac (standard shortcut Ctrl+Tab is not supported in Qt on Mac)
 * Supported standard shortcuts on Mac:
 *    - Cmd + Shift + Arrow Key
 *    - Cmd + { or }
 */
void DocTabManager::setupTabPageSwitchingShortcuts()
{
    QList<QKeySequence> shortcuts;
    QAction* nextTabAction = new QAction(&m_MainWindow);
    shortcuts.append(QKeySequence::NextChild);
    shortcuts.append(QKeySequence(Qt::CTRL+Qt::ShiftModifier+Qt::Key_Right));
    nextTabAction->setShortcuts(shortcuts);
    connect(nextTabAction,  SIGNAL(triggered()), this, SLOT(showNextTabPage()));
    
    shortcuts.clear();
    QAction* prevTabAction = new QAction(&m_MainWindow);
    shortcuts.append(QKeySequence::PreviousChild);
    shortcuts.append(QKeySequence(Qt::CTRL+Qt::ShiftModifier+Qt::Key_Left));
    prevTabAction->setShortcuts(shortcuts);
    connect(prevTabAction,  SIGNAL(triggered()), this, SLOT(showPreviousTabPage()));
    
    m_MainWindow.addAction(nextTabAction);
    m_MainWindow.addAction(prevTabAction);
}

void DocTabManager::showNextTabPage()
{
    if (m_TabWidget->count() > 0)
        setCurrentTabPage(m_TabWidget->currentIndex()+1);
}

void DocTabManager::showPreviousTabPage()
{
    if (m_TabWidget->count() > 0)
        setCurrentTabPage(m_TabWidget->currentIndex()-1);
}

void DocTabManager::setCurrentTabPage(int index)
{
    if (index > (m_TabWidget->count() - 1))
        index = 0;
    else if (index < 0)
        index = m_TabWidget->count() - 1;
    m_TabWidget->setCurrentIndex(index);
    m_MainWindow.setFocus();
}
