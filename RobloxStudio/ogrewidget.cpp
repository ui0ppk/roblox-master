/**
 * ogrewidget.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "ogrewidget.h"
#include "rbx/TaskScheduler.h"

// Qt Headers
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QUrl>
#include <QTimer>

// Roblox Headers
#include "FastLog.h"
#include "util/Http.h"

// Roblox Studio Headers
#include "RobloxSettings.h"
#include "RobloxCustomWidgets.h"
#include "CommonInsertWidget.h"
#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"
#include "InsertObjectListWidgetItem.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "V8DataModel/InputObject.h"

#include "Util/RobloxGoogleAnalytics.h"
#include "Util/PhysicalProperties.h"
#include "v8datamodel/Workspace.h"
#include "RobloxPluginHost.h"
#include "StudioUtilities.h"

LOGGROUP(TaskSchedulerTiming)
LOGGROUP(RenderRequest)
FASTFLAG(GoogleAnalyticsTrackingEnabled)
DYNAMIC_FASTFLAGVARIABLE(BackTabInputInStudio, false)
FASTFLAGVARIABLE(DontSwallowInputForStudioShortcuts, false)
DYNAMIC_FASTFLAG(MaterialPropertiesEnabled)

static RBX::KeyCode keyCodeTOUIKeyCode(int keyCode);
static RBX::ModCode modifiersToUIModCode(int modifier);

QOgreWidget::QOgreWidget(const QString& name,QWidget *parent)
: QWidget(NULL)
, m_pRobloxView(NULL)
, m_bIgnoreEnterEvent(0)
, m_bIgnoreLeaveEvent(false)
, m_bUpdateInProgress(false)
, m_bMouseCommandInvoked(false)
, m_hasApplicationFocus(true)
, m_bRobloxViewInitialized(false)
, m_luaTextBoxHasFocus(false)
{
    setObjectName(name);

	//set default states
	setAttribute(Qt::WA_PaintOnScreen, true);
	setAttribute(Qt::WA_OpaquePaintEvent, true);
	setAttribute(Qt::WA_NoSystemBackground, true);

	setFocusPolicy(Qt::StrongFocus);
	setAutoFillBackground(true);
	setAcceptDrops(true);
	setMinimumSize(100, 100);
    setMaximumSize(3800,2100);
    setSizePolicy(QSizePolicy::Maximum,QSizePolicy::Maximum);
    setContextMenuPolicy(Qt::PreventContextMenu);
}

void QOgreWidget::setRobloxView(RobloxView *rbxView)
{	
	m_pRobloxView = rbxView;

	if (!m_pRobloxView)
		return;

	QPoint mousePos = mapFromGlobal(QCursor::pos());
	if (rect().contains(mousePos))
		enterEvent(NULL);
	else 
		leaveEvent(NULL);
}

void QOgreWidget::activate()
{
	qApp->installEventFilter(this);
    
    // reset bounds to force a resize if parent or size changed
	// Roblox view can already be destroyed in resetting case
	if(m_pRobloxView)
		m_pRobloxView->setBounds(width(),height());

}

void QOgreWidget::deActivate()
{
	FASTLOG(FLog::RenderRequest, "QOgreWidget deactivated, OGRE_VIEW_UPDATE removed");
	if (RBX::TaskScheduler::singleton().isCyclicExecutive())
	{
		//NOTHING!
	}
	else
	{
		QCoreApplication::removePostedEvents(this, OGRE_VIEW_UPDATE);
	}
	qApp->removeEventFilter(this);
}

bool QOgreWidget::eventFilter(QObject * watched, QEvent * evt)
{
	QEvent::Type eventType = evt->type();

	if (eventType == QEvent::ApplicationActivate)
		m_hasApplicationFocus = true;
	else if (eventType == QEvent::ApplicationDeactivate)
		m_hasApplicationFocus = false;

	// whenever popup widgets are shown, QOgreWidget receives only enter and leave event but no mouse move event
	// so, do not handle enter/leave event in all such cases
	// e.g Color Picker Widget which comes up from the Properties Window
	if ((watched != this ) && watched->isWidgetType() && (static_cast<QWidget *>(watched)->windowFlags() & Qt::Popup) == Qt::Popup)
	{
		// In case of Floating Dock Widget, do not show ArrowCursor when the mouse is over Ogre Window
		if(watched->inherits("QDockWidget")) 
			return QWidget::eventFilter(watched, evt);

		if (eventType == QEvent::Show)
		{
			m_bIgnoreEnterEvent++;
		}
		else if (eventType == QEvent::Hide)
		{		
			--m_bIgnoreEnterEvent;
			if (!m_bIgnoreEnterEvent)
			{
				QPoint mousePos = mapFromGlobal(QCursor::pos());
				if (rect().contains(mousePos))
					enterEvent(NULL);
			}	
		}
		else if (eventType == QEvent::MouseButtonRelease)
		{
			// on close of a popup, mouse events are replayed (button doesn't matter) on the widget under the mouse
			// make sure we disable it on the popup as well as it's parent to avoid any unnecessary event propagation
			QWidget* pWidget = static_cast<QWidget *>(watched);
			if (pWidget->rect().contains(pWidget->mapFromGlobal(QCursor::pos())))
			{
				pWidget->setAttribute(Qt::WA_NoMouseReplay);
				if (pWidget->parent() && pWidget->parent()->isWidgetType())
					static_cast<QWidget *>(pWidget->parent())->setAttribute(Qt::WA_NoMouseReplay);
			}
		}
	}

	if (FFlag::DontSwallowInputForStudioShortcuts)
	{
		if (watched == this && hasFocus())
		{
			if (evt->type() == QEvent::ShortcutOverride)
			{
				bool processed = false;

				QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
				int keyCode = keyEvent->key() | StudioUtilities::translateKeyModifiers(keyEvent->modifiers(), keyEvent->text());

				if (UpdateUIManager::Instance().getMainWindow().isShortcut(QKeySequence(keyCode))) 
				{
					processed = true;
				}
				
				//ShorcutOverride is sent for every key press, not just shortcuts. We don't want to fire handleKeyEvent in KeyPressed because then studio shortcuts get swallowed.
				handleKeyEvent((QKeyEvent*)evt, RBX::InputObject::TYPE_KEYBOARD, RBX::InputObject::INPUT_STATE_BEGIN, processed);
			} 
			else if (evt->type() == QEvent::KeyRelease) 
			{
				handleKeyEvent((QKeyEvent*)evt, RBX::InputObject::TYPE_KEYBOARD, RBX::InputObject::INPUT_STATE_END);
			}
		}
	}

	// This is needed for Modal Dialog Show/Hide on Mac	
#ifdef Q_WS_MAC
	if ( eventType == QEvent::ActivationChange || eventType == QEvent::FocusIn || eventType == QEvent::FocusOut )
	{
		if (rect().contains(mapFromGlobal(QCursor::pos())))
			isActiveWindow() ? enterEvent(NULL) : leaveEvent(NULL);
	}
	else if (eventType == QEvent::Hide && watched->inherits("QDialog"))
	{
		if (rect().contains(mapFromGlobal(QCursor::pos())) && isActiveWindow())
			m_bIgnoreLeaveEvent = true;
	}
#else
	if( eventType == QEvent::ActivationChange ) 
	{
		if (rect().contains(mapFromGlobal(QCursor::pos())))
			isActiveWindow() ? enterEvent(NULL) : leaveEvent(NULL);
	}
#endif

	return QWidget::eventFilter(watched, evt);
}

bool QOgreWidget::event(QEvent * evt)
{
    if (m_pRobloxView)
    {
        if (evt->type() == QEvent::FocusOut)
        {
            m_pRobloxView->handleFocus(false);
            setLuaTextBoxHasFocus(false);
        }
        else if (evt->type() == QEvent::FocusIn)
        {
            m_pRobloxView->handleFocus(true);
        }
    }

    // If a lua text box as focus then prevent all shortcut overrides
    // from executing.
    if (m_luaTextBoxHasFocus)
    {
        if (evt->type() == QEvent::ShortcutOverride)
        {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
            keyEvent->accept();
            return true;
        }
    }

	if (evt->type() == QEvent::KeyPress)
	{
		if (static_cast<QKeyEvent*>(evt)->modifiers().testFlag(Qt::ControlModifier) && (static_cast<QKeyEvent*>(evt)->key() == Qt::Key_Tab))
		{
			evt->accept();
			QCoreApplication::sendEvent(&UpdateUIManager::Instance().getMainWindow(), evt);
			return true;
		}
	}
    
	FASTLOG(FLog::TaskSchedulerTiming, "QT Event fired through QOgreWidget");
	if ((evt->type() != OGRE_VIEW_UPDATE) || !m_pRobloxView)
	{
		FASTLOG1(FLog::RenderRequest, "OgreWidget Event returns before updateView. RBXView: %d", (int)m_pRobloxView);
		return QWidget::event(evt);
	}

	if (!m_bUpdateInProgress)
	{	
		m_bUpdateInProgress = true;
		m_pRobloxView->updateView();
		m_bUpdateInProgress = false;
	}

	return true;
}

void QOgreWidget::closeEvent(QCloseEvent *)
{
	if (m_pRobloxView) 
		delete m_pRobloxView;
	m_pRobloxView = NULL;
}

void QOgreWidget::enterEvent(QEvent *)
{	
	if(!m_pRobloxView)
		return;

#ifdef Q_WS_MAC
	if ( QApplication::activeWindow() != &UpdateUIManager::Instance().getMainWindow() )
		return;
#endif

	//inform rbx view about the change
	m_pRobloxView->handleMouseInside(true);

	//enable mouse tracking
	setMouseTracking(true);
}

void QOgreWidget::leaveEvent(QEvent *)
{
#ifdef Q_WS_MAC
	if (m_bIgnoreLeaveEvent)
	{
		m_bIgnoreLeaveEvent = false;
		return;
	}
#endif

	if(!m_pRobloxView)
		return;

	//inform rbx view about the change
	m_pRobloxView->handleMouseInside(false);

	//disable mouse tracking
	setMouseTracking(false);

#ifdef Q_WS_MAC
	setCursor(Qt::ArrowCursor);
#endif
}

void QOgreWidget::focusOutEvent(QFocusEvent* focusEvent)
{
    QWidget::focusOutEvent(focusEvent);

	if (m_pRobloxView)
		m_pRobloxView->resetKeyState();
}

void QOgreWidget::resizeEvent(QResizeEvent *evt)
{
	if (!m_pRobloxView) 
		return;

	m_pRobloxView->setBounds(evt->size().width(), evt->size().height());

	QPoint mousePos = mapFromGlobal(QCursor::pos());
	if (rect().contains(mousePos))
		enterEvent(NULL);
}

void QOgreWidget::mousePressEvent(QMouseEvent *evt)
{
	if (!m_pRobloxView)
		return;

	QPoint currentPos = evt->pos();

	RBX::InputObject::UserInputType mouseEventType = RBX::InputObject::TYPE_NONE;
	switch(evt->button())
	{
	case Qt::LeftButton:
		mouseEventType = RBX::InputObject::TYPE_MOUSEBUTTON1;
		break;
	case Qt::RightButton:
		mouseEventType = RBX::InputObject::TYPE_MOUSEBUTTON2;
		break;
    case Qt::MiddleButton:
		mouseEventType = RBX::InputObject::TYPE_MOUSEBUTTON3;
		break;
    default:
            break;
            
	}

	m_pRobloxView->handleMouse(mouseEventType,
        RBX::InputObject::INPUT_STATE_BEGIN,
		currentPos.x(), 
		currentPos.y(), 
		modifiersToUIModCode(evt->modifiers()));
	evt->accept();
}

void QOgreWidget::mouseReleaseEvent(QMouseEvent *evt)
{
	if (!m_pRobloxView)
		return;

	RBX::InputObject::UserInputType mouseEventType = RBX::InputObject::TYPE_NONE;
	switch(evt->button())
	{
	case Qt::LeftButton:
		mouseEventType = RBX::InputObject::TYPE_MOUSEBUTTON1;
		break;
	case Qt::RightButton:
		mouseEventType = RBX::InputObject::TYPE_MOUSEBUTTON2;
		break;
    case Qt::MiddleButton:
		mouseEventType = RBX::InputObject::TYPE_MOUSEBUTTON3;
		break;
    default:
        break;
            
	}

    evt->accept();

	QPoint currentPos = evt->pos();
	m_pRobloxView->handleMouse(
        mouseEventType,
        RBX::InputObject::INPUT_STATE_END,
		currentPos.x(), 
		currentPos.y(), 
		modifiersToUIModCode(evt->modifiers()) );
}

void QOgreWidget::mouseMoveEvent(QMouseEvent *evt)
{
	FASTLOG(FLog::TaskSchedulerTiming, "QOgreWidget::mouseMoveEvent firing");
	if (m_pRobloxView)
	{	
		lastMovePoint = evt->pos();
		lastMoveModCode = modifiersToUIModCode(evt->modifiers());
		m_pRobloxView->handleMouse(RBX::InputObject::TYPE_MOUSEMOVEMENT, RBX::InputObject::INPUT_STATE_CHANGE, lastMovePoint.x(), lastMovePoint.y(), lastMoveModCode);
	}
	evt->accept();
}

void QOgreWidget::handleKeyEvent(QKeyEvent * evt, RBX::InputObject::UserInputType eventType,
    RBX::InputObject::UserInputState eventState, bool processed)
{
	if (eventType == RBX::InputObject::TYPE_KEYBOARD && (!m_pRobloxView || evt->isAutoRepeat()))
	{
		if (!FFlag::DontSwallowInputForStudioShortcuts)
		{
			if (eventState == RBX::InputObject::INPUT_STATE_BEGIN)
				Super::keyPressEvent(evt);
			else if(eventState == RBX::InputObject::INPUT_STATE_END)
				Super::keyReleaseEvent(evt);
		}
		return;
	}

	std::string textString = evt->text().toStdString();

	m_pRobloxView->handleKey(eventType, eventState,
        keyCodeTOUIKeyCode(evt->key()), modifiersToUIModCode(evt->modifiers()),
        (textString.length() > 0) ? textString.c_str()[0] : NULL,
		processed);

	if (!FFlag::DontSwallowInputForStudioShortcuts)
	{
		evt->accept();
	}
}

void QOgreWidget::keyPressEvent(QKeyEvent * evt)
{
	if (FFlag::DontSwallowInputForStudioShortcuts)
	{
		if (!m_pRobloxView || evt->isAutoRepeat())
		{
			Super::keyPressEvent(evt);
			return;
		}
		evt->accept();
	}
	else
	{
		handleKeyEvent(evt, RBX::InputObject::TYPE_KEYBOARD, RBX::InputObject::INPUT_STATE_BEGIN);
	}
}

bool QOgreWidget::focusNextPrevChild(bool next)
{
	return false;
}

void QOgreWidget::keyReleaseEvent(QKeyEvent * evt)
{
	if (FFlag::DontSwallowInputForStudioShortcuts)
	{
		if (!m_pRobloxView || evt->isAutoRepeat())
		{
			Super::keyReleaseEvent(evt);
			return;
		}
		evt->accept();
	}
	else
	{
		handleKeyEvent(evt, RBX::InputObject::TYPE_KEYBOARD, RBX::InputObject::INPUT_STATE_END);
	}
}

void QOgreWidget::wheelEvent(QWheelEvent *evt)
{
    if (!m_pRobloxView)
		return;

	QPoint currentPos = evt->pos();
	m_pRobloxView->handleScrollWheel(evt->delta(), currentPos.x(), currentPos.y());
	evt->accept();
}

void QOgreWidget::paintEvent(QPaintEvent *evt)
{
    if (!m_bRobloxViewInitialized)
        if (RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
        {
            m_bRobloxViewInitialized = true;
            playDoc->initializeRobloxView();
        }
    

	if (!m_pRobloxView)
		return QWidget::paintEvent(evt);

	m_pRobloxView->requestUpdateView();	

	evt->accept();
}

void QOgreWidget::dragEnterEvent(QDragEnterEvent *evt)
{
	if (!isValidDrag(evt))
		return QWidget::dragEnterEvent(evt);

	const QMimeData *pMimeData = evt->mimeData();
	if (pMimeData && pMimeData->hasUrls())
	{
		QList<QUrl> urlList = pMimeData->urls();
		shared_ptr<const RBX::Instances> insertedInstances = m_pRobloxView->handleDropOperation(urlList.at(0).toString(), evt->pos().x(), evt->pos().y(), m_bMouseCommandInvoked);

		if (RobloxIDEDoc* playDoc = RobloxDocManager::Instance().getPlayDoc())
			if (RBX::DataModel* dataModel = playDoc->getDataModel())
				UpdateUIManager::Instance().getMainWindow().getPluginHost()->handleDragEnterEvent(dataModel, shared_ptr<const RBX::Instances>(insertedInstances), RBX::Vector2(evt->pos().x(), evt->pos().y()));
	}
	else 
	{
		QListWidget *pListWidget = static_cast<QListWidget *>(evt->source());
		QList<QListWidgetItem *> itemList = pListWidget->selectedItems();
		if (itemList.size() <= 0)
			return QWidget::dragEnterEvent(evt);

		InsertObjectListWidgetItem *pListWidgetItem = dynamic_cast<InsertObjectListWidgetItem *>(itemList.at(0));
		if (pListWidgetItem)
        {
            if (FFlag::GoogleAnalyticsTrackingEnabled)
            {
				RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "InsertObject", pListWidgetItem->text().toStdString().c_str());
            }

			m_pRobloxView->handleDropOperation(pListWidgetItem->getInstance(), evt->pos().x(), evt->pos().y(), m_bMouseCommandInvoked);
        }
	}

#ifdef Q_WS_MAC
	if (m_bMouseCommandInvoked)
	{
		QApplication::setOverrideCursor(Qt::ClosedHandCursor);
		enterEvent(NULL);
	}
	else 
	{
		QApplication::setOverrideCursor(Qt::DragCopyCursor);
		leaveEvent(NULL);
	}
#endif

	evt->acceptProposedAction();
}

void QOgreWidget::dragMoveEvent(QDragMoveEvent *evt)
{
	if(!m_pRobloxView) 
		return QWidget::dragMoveEvent(evt);

	if (m_bMouseCommandInvoked)
	{
		lastMovePoint = evt->pos();
		m_pRobloxView->handleMouse(RBX::InputObject::TYPE_MOUSEMOVEMENT,
            RBX::InputObject::INPUT_STATE_CHANGE,
            lastMovePoint.x(), lastMovePoint.y(), lastMoveModCode);
	}

	evt->acceptProposedAction();
}

void QOgreWidget::dropEvent(QDropEvent *evt)
{
	const QMimeData *pMimeData = evt->mimeData();
	if (!m_pRobloxView || !pMimeData)
		return QWidget::dropEvent(evt);

#ifdef Q_WS_MAC
	QApplication::restoreOverrideCursor();
#endif

	if (m_bMouseCommandInvoked)
	{
		QPoint currentPos = evt->pos();
		m_pRobloxView->handleMouse(RBX::InputObject::TYPE_MOUSEBUTTON1,
            RBX::InputObject::INPUT_STATE_END, currentPos.x(), currentPos.y(), (RBX::ModCode)0);
	}

	evt->acceptProposedAction();
	setFocus();
}

void QOgreWidget::dragLeaveEvent(QDragLeaveEvent *evt)
{
	if(!m_pRobloxView) 
		return QWidget::dragLeaveEvent(evt);

	m_pRobloxView->cancelDropOperation(m_bMouseCommandInvoked);

#ifdef Q_WS_MAC
	QApplication::restoreOverrideCursor();
	leaveEvent(NULL);
#endif

	evt->accept();
}

#ifdef Q_WS_WIN
bool QOgreWidget::winEvent(MSG * msg, long * result)
{
	if ( (msg->message == WM_KEYUP) && //key up
		 (msg->wParam == VK_SNAPSHOT) && //print screen key
		 (UpdateUIManager::Instance().getMainWindow().getBuildMode() == BM_BASIC) )//build mode is basic
	{
		//invoke snapshot command
		QMetaObject::invokeMethod(UpdateUIManager::Instance().getMainWindow().screenShotAction, "triggered", Qt::QueuedConnection);
		return true;
	}

	return QWidget::winEvent(msg, result);
}
#endif

bool QOgreWidget::isValidDrag(QDragEnterEvent *evt)
{
	bool result = false;

	const QMimeData *pMimeData = evt->mimeData();
	if (m_pRobloxView && pMimeData)
	{
		// All drags from HTML should be called via window.external.Insert in JS
		if (pMimeData->hasFormat("text/html"))
			return false;

		if (pMimeData->hasUrls())
		{
			QList<QUrl> urlList = pMimeData->urls();
			QString urlQStr = urlList.at(0).toString();
			QString fileStr = urlList.at(0).toLocalFile();
			if (!urlQStr.isEmpty() && fileStr.isEmpty())
				result = true;
		}
		else 
		{
			QWidget *pSourceWidget = evt->source();
			if (pSourceWidget && (pSourceWidget->objectName() == "InsertObjectListWidget"))
				result = true;
		}
	}

	return result;
}

static RBX::KeyCode keyCodeTOUIKeyCode(int keyCode)
{
	/// Handle all albhabets
	if (keyCode >= 65 && keyCode <= 90)
		return RBX::KeyCode(keyCode - Qt::Key_A + 'a');

	// Handle Numbers
	if(keyCode >= Qt::Key_0 && keyCode <= Qt::Key_9)
		return RBX::KeyCode(keyCode);

	// Handle F1 to F15
	if(keyCode >= Qt::Key_F1 && keyCode <= Qt::Key_F15)
		return RBX::KeyCode(keyCode - 0xFFFF16);


	RBX::KeyCode rbxKey = RBX::SDLK_UNKNOWN;

	// Handle Special Unordered Keys Here
	switch(keyCode)
	{
	case Qt::Key_CapsLock:
		rbxKey = RBX::SDLK_CAPSLOCK;
		break;
	case Qt::Key_Backspace :
		rbxKey = RBX::SDLK_BACKSPACE;
		break;
	case Qt::Key_Up :
		rbxKey = RBX::SDLK_UP;
		break;
	case Qt::Key_Down :
		rbxKey = RBX::SDLK_DOWN;
		break;
	case Qt::Key_Left :
		rbxKey = RBX::SDLK_LEFT;
		break;
	case Qt::Key_Right :
		rbxKey = RBX::SDLK_RIGHT;
		break;
	case Qt::Key_Insert :
		rbxKey = RBX::SDLK_INSERT;
		break;
	case Qt::Key_Delete :
		rbxKey = RBX::SDLK_DELETE;
		break;
	case Qt::Key_Home :
		rbxKey = RBX::SDLK_HOME;
		break;
	case Qt::Key_End :
		rbxKey = RBX::SDLK_END;
		break;
	case Qt::Key_PageUp :
		rbxKey = RBX::SDLK_PAGEUP;
		break;
	case Qt::Key_PageDown :
		rbxKey = RBX::SDLK_PAGEDOWN;
		break;
	case Qt::Key_Space :
		rbxKey = RBX::SDLK_SPACE;
		break;
	case Qt::Key_Control:
		rbxKey = RBX::SDLK_LCTRL;	
		break;
	case Qt::Key_Alt:
		rbxKey = RBX::SDLK_LALT;		
		break;
	case Qt::Key_Shift:
		rbxKey = RBX::SDLK_LSHIFT;
		break;
	case Qt::Key_Escape:
		rbxKey = RBX::SDLK_ESCAPE;
		break;
	case Qt::Key_Minus:
		rbxKey = RBX::SDLK_MINUS;
		break;
	case Qt::Key_Equal:
		rbxKey = RBX::SDLK_EQUALS;
		break;
	case Qt::Key_Tab:
		rbxKey = RBX::SDLK_TAB;
		break;
	case Qt::Key_Backtab:
		{
			if (DFFlag::BackTabInputInStudio)
			{
				rbxKey = RBX::SDLK_TAB;
				break;
			}
		}
	case Qt::Key_BracketLeft:
		rbxKey = RBX::SDLK_LEFTBRACKET;
		break;
	case Qt::Key_BracketRight:
		rbxKey = RBX::SDLK_RIGHTBRACKET;
		break;
	case Qt::Key_Return:
	case Qt::Key_Enter:
		rbxKey = RBX::SDLK_RETURN;
		break;
	case Qt::Key_Semicolon:
		rbxKey = RBX::SDLK_SEMICOLON;
		break;
	case Qt::Key_QuoteLeft:
		rbxKey = RBX::SDLK_BACKQUOTE;
		break;
	case Qt::Key_Apostrophe:
		rbxKey = RBX::SDLK_QUOTE;
		break;
	case Qt::Key_QuoteDbl:
		rbxKey = RBX::SDLK_QUOTEDBL;
		break;
	case Qt::Key_Backslash:
		rbxKey = RBX::SDLK_BACKSLASH;
		break;
	case Qt::Key_Comma:
		rbxKey = RBX::SDLK_COMMA;
		break;
	case Qt::Key_Period:
		rbxKey = RBX::SDLK_PERIOD;
		break;
	case Qt::Key_Slash:
		rbxKey = RBX::SDLK_SLASH;
		break;
	case Qt::Key_multiply:
		rbxKey = RBX::SDLK_KP_MULTIPLY;
		break;
	case Qt::Key_NumLock:
		rbxKey = RBX::SDLK_NUMLOCK;
		break;
	case Qt::Key_ScrollLock:
		rbxKey = RBX::SDLK_SCROLLOCK;
		break;
	case Qt::Key_Asterisk:
		rbxKey = RBX::SDLK_ASTERISK;
		break;
	case Qt::Key_Plus:
		rbxKey = RBX::SDLK_PLUS;
		break;
	}

	return rbxKey;
}

static RBX::ModCode modifiersToUIModCode(int modifier)
{
	unsigned int modCode = 0;

	if (modifier & Qt::ShiftModifier)
	{
		modCode = modCode | RBX::KMOD_LSHIFT;
	} 
	if (modifier & Qt::ControlModifier)
	{
		modCode = modCode | RBX::KMOD_LCTRL;
	} 
	if (modifier & Qt::AltModifier)
	{
		modCode = modCode | RBX::KMOD_LALT;
	} 

	return (RBX::ModCode) modCode;
}