/****************************************************************************
**
** Qtitan Library by Developer Machines (Microsoft-Ribbon implementation for Qt.C++)
** 
** Copyright (c) 2009-2014 Developer Machines (http://www.devmachines.com)
**           ALL RIGHTS RESERVED
** 
**  The entire contents of this file is protected by copyright law and
**  international treaties. Unauthorized reproduction, reverse-engineering
**  and distribution of all or any portion of the code contained in this
**  file is strictly prohibited and may result in severe civil and 
**  criminal penalties and will be prosecuted to the maximum extent 
**  possible under the law.
**
**  RESTRICTIONS
**
**  THE SOURCE CODE CONTAINED WITHIN THIS FILE AND ALL RELATED
**  FILES OR ANY PORTION OF ITS CONTENTS SHALL AT NO TIME BE
**  COPIED, TRANSFERRED, SOLD, DISTRIBUTED, OR OTHERWISE MADE
**  AVAILABLE TO OTHER INDIVIDUALS WITHOUT WRITTEN CONSENT
**  AND PERMISSION FROM DEVELOPER MACHINES
**
**  CONSULT THE END USER LICENSE AGREEMENT FOR INFORMATION ON
**  ADDITIONAL RESTRICTIONS.
**
****************************************************************************/
#include <QApplication>
#include <QStyleOption>
#include <QResizeEvent>
#include <QDesktopWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPainter>
#include <QLayout>
#include <QEvent>

#include "QtnRibbonPrivate.h"
#include "QtnStyleHelpers.h"
#include "QtnRibbonGroupPrivate.h"
#include "QtnRibbonGroup.h"
#include "QtnRibbonButton.h"
#include "QtnRibbonStyle.h"
#include "QtnRibbonPage.h"

using namespace Qtitan;

static const int hMargin = 4;

static int keyTipEventNumber = -1;
static int showKeyTipEventNumber = -1;
static int hideKeyTipEventNumber = -1;
static int minimizedEventNumber = -1;

namespace Qtitan
{
    /* ExWidgetWrapperPrivate */
    class ExWidgetWrapperPrivate : public QObject
    {
    public:
        QTN_DECLARE_PUBLIC(ExWidgetWrapper)
    public:
        explicit ExWidgetWrapperPrivate();

    public:
        void init(QWidget* widget);

    public:
        QIcon m_icon;
        QWidget* m_buddy;
        QString m_labelText;
        int m_length;
        bool m_align;
    };
}; //namespace Qtitan

/* ExWidgetWrapperPrivate */
ExWidgetWrapperPrivate::ExWidgetWrapperPrivate()
    : m_buddy(Q_NULL)
{
}

void ExWidgetWrapperPrivate::init(QWidget* widget)
{
    Q_ASSERT(widget != Q_NULL);
    QTN_P(ExWidgetWrapper);

    m_buddy = widget;
    m_align = false;
    m_length = -1;
    m_buddy->setParent(&p);

    p.setMinimumWidth(m_buddy->minimumWidth());
    widget->setMinimumWidth(0);
    p.setMaximumWidth(m_buddy->maximumWidth());

    p.setFocusPolicy(m_buddy->focusPolicy());
    p.setAttribute(Qt::WA_InputMethodEnabled);
    p.setSizePolicy(m_buddy->sizePolicy());
    p.setMouseTracking(true);
    p.setAcceptDrops(true);
    p.setAttribute(Qt::WA_MacShowFocusRect, true);

    m_buddy->setFocusProxy(&p);
    m_buddy->setAttribute(Qt::WA_MacShowFocusRect, false);

    p.setToolTip(m_buddy->toolTip());
    m_buddy->setToolTip("");
}

/*!
\class Qtitan::ExWidgetWrapper
\internal
*/
ExWidgetWrapper::ExWidgetWrapper(QWidget* parent, QWidget* widget)
    : QWidget(parent)
{
    QTN_INIT_PRIVATE(ExWidgetWrapper);
    QTN_D(ExWidgetWrapper);
    d.init(widget);
}

ExWidgetWrapper::~ExWidgetWrapper()
{
    QTN_FINI_PRIVATE();
}

QString ExWidgetWrapper::text() const
{
    QTN_D(const ExWidgetWrapper);
    return d.m_labelText;
}

void ExWidgetWrapper::setText(const QString& text)
{
    QTN_D(ExWidgetWrapper);
    d.m_labelText = text;
}

void ExWidgetWrapper::setIcon(const QIcon& icon)
{
    QTN_D(ExWidgetWrapper);
    d.m_icon = icon;
}

QIcon ExWidgetWrapper::icon() const
{
    QTN_D(const ExWidgetWrapper);
    return d.m_icon;
}

void ExWidgetWrapper::setAlignWidget(bool align)
{
    QTN_D(ExWidgetWrapper);
    d.m_align = align;
}

bool ExWidgetWrapper::alignWidget() const
{
    QTN_D(const ExWidgetWrapper);
    return d.m_align;
}

void ExWidgetWrapper::setLengthLabel(int length)
{
    QTN_D(ExWidgetWrapper);
    d.m_length = length;
}

QWidget* ExWidgetWrapper::buddy() const
{
    QTN_D(const ExWidgetWrapper);
    return d.m_buddy;
}

void ExWidgetWrapper::resizeEvent(QResizeEvent* event)
{
    updateGeometries();
    QWidget::resizeEvent(event);
}

void ExWidgetWrapper::updateGeometries()
{
    QStyleOptionFrameV2 panel;
    initStyleOption(&panel);

    QRect rect = panel.rect;

    QTN_D(ExWidgetWrapper);
    if (!d.m_icon.isNull())
    {
        QSize sz = d.m_icon.actualSize(QSize(16, 16));
        rect.adjust(sz.width() + hMargin, 0, hMargin, 0);
    }

    if (!d.m_labelText.isEmpty())
    {
        if (d.m_length == -1)
        {
            QFontMetrics fm = fontMetrics();
            QSize sz = fm.size(Qt::TextHideMnemonic, d.m_labelText);
            int width =  sz.width() + fm.width(QLatin1Char('x'));
            rect.adjust(width, 0, -hMargin, 0);
            d.m_buddy->setGeometry(rect);
        }
        else
        {
            rect.adjust(d.m_length + 1, 0, -hMargin, 0);
            d.m_buddy->setGeometry(rect);
        }
    }
}

void ExWidgetWrapper::initStyleOption(QStyleOptionFrameV2* option) const
{
    QTN_D(const ExWidgetWrapper);

    option->initFrom(this);
    option->rect = contentsRect();
    option->lineWidth = qobject_cast<QLineEdit*>(d.m_buddy) ? style()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, this) : 0;
    option->midLineWidth = 0;
    option->state |= QStyle::State_Sunken;

    if (isReadOnly())
        option->state |= QStyle::State_ReadOnly;

#ifdef QT_KEYPAD_NAVIGATION
    if (hasEditFocus())
        option->state |= QStyle::State_HasEditFocus;
#endif
    option->features = QStyleOptionFrameV2::None;
}

bool ExWidgetWrapper::isReadOnly() const
{
    QTN_D(const ExWidgetWrapper);

    bool readOnly = false;
    if (QLineEdit* pEdit = qobject_cast<QLineEdit*>(d.m_buddy))
        readOnly = pEdit->isReadOnly();
    else if (QComboBox* pComboBox = qobject_cast<QComboBox*>(d.m_buddy))
        readOnly = pComboBox->isEditable();
    else if (QAbstractSpinBox* pSpinBox = qobject_cast<QAbstractSpinBox*>(d.m_buddy))
        readOnly = pSpinBox->isReadOnly();

    return readOnly;
}

void ExWidgetWrapper::focusInEvent(QFocusEvent* event)
{
    QTN_D(ExWidgetWrapper);
    QCoreApplication::sendEvent(d.m_buddy, event);
    QWidget::focusInEvent(event);
}

void ExWidgetWrapper::focusOutEvent(QFocusEvent* event)
{
    QTN_D(ExWidgetWrapper);
    QCoreApplication::sendEvent(d.m_buddy, event);
    QWidget::focusOutEvent(event);
}

bool ExWidgetWrapper::event(QEvent* event)
{
    QTN_D(ExWidgetWrapper);

    if (!d.m_buddy)
        return false;

    switch(event->type()) 
    {
        case QEvent::ShortcutOverride:
        case QEvent::KeyPress :
            {
                class QtnWidget : public QWidget { friend class Qtitan::ExWidgetWrapper; };
                return ((QtnWidget*)d.m_buddy)->event(event);
            }
        default:
            break;
    }
    return QWidget::event(event);
}

void ExWidgetWrapper::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QTN_D(ExWidgetWrapper);

    QPainter p(this);
    QStyleOptionFrameV2 panel;
    initStyleOption(&panel);

    if (!d.m_icon.isNull())
    {
        QRect rc = panel.rect;
        QPixmap pm;
        QSize pmSize = d.m_icon.actualSize(QSize(16, 16));
        QIcon::State state = panel.state & QStyle::State_On ? QIcon::On : QIcon::Off;
        QIcon::Mode mode;
        if (!(panel.state & QStyle::State_Enabled))
            mode = QIcon::Disabled;
        else if ((panel.state & QStyle::State_MouseOver) && (panel.state & QStyle::State_AutoRaise))
            mode = QIcon::Active;
        else
            mode = QIcon::Normal;
        pm = d.m_icon.pixmap(panel.rect.size().boundedTo(pmSize), mode, state);
        pmSize = pm.size();

        rc.setWidth(pmSize.width());
        rc.adjust(hMargin, 0, hMargin, 0);
        style()->drawItemPixmap(&p, rc, Qt::AlignCenter, pm);
        panel.rect.setLeft(panel.rect.left() + pmSize.width() + 3 + hMargin);
    }
    if (!d.m_labelText.isEmpty())
    {
        style()->drawItemText(&p, panel.rect, Qt::AlignLeft | Qt::AlignVCenter, panel.palette, 
            panel.state & QStyle::State_Enabled, d.m_labelText, QPalette::ButtonText);
    }
}

QVariant ExWidgetWrapper::inputMethodQuery(Qt::InputMethodQuery property) const
{
    QTN_D(const ExWidgetWrapper);
    return d.m_buddy->inputMethodQuery(property);
}

QSize ExWidgetWrapper::sizeHint() const
{
    QTN_D(const ExWidgetWrapper);
    QSize sz = d.m_buddy->sizeHint();
    return sz;
}

void ExWidgetWrapper::inputMethodEvent(QInputMethodEvent* event)
{
    QTN_D(const ExWidgetWrapper);
    QCoreApplication::sendEvent(d.m_buddy, (QEvent*)event);
}



/*!
\class Qtitan::RibbonDefaultGroupButton
\internal
*/
RibbonDefaultGroupButton::RibbonDefaultGroupButton(QWidget* parent, RibbonGroup* group)
    : QToolButton(parent)
{
    m_ribbonGroup = group;
    m_eventLoop = Q_NULL;
    m_hasPopup = false;

    m_ribbonGroup->adjustSize();

    ensurePolished();

    setAttribute(Qt::WA_LayoutUsesWidgetRect);
    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    setPopupMode(QToolButton::MenuButtonPopup);
    setText(m_ribbonGroup->title());
}

RibbonDefaultGroupButton::~RibbonDefaultGroupButton()
{
	// BEGIN ROBLOX CHANGES
	QObject::disconnect(m_ribbonGroup, SIGNAL(hidePopup()), this, SLOT(resetPopopGroup()));
	// END ROBLOX CHANGES
    if (m_eventLoop)
        m_eventLoop->exit();
}

bool RibbonDefaultGroupButton::isShowPopup() const
{
    return m_hasPopup;
}

void RibbonDefaultGroupButton::setVisible(bool visible)
{
    QToolButton::setVisible(visible);
}

void RibbonDefaultGroupButton::resetReducedGroup()
{
    if (m_ribbonGroup->qtn_d().m_reduced)
    {
        m_ribbonGroup->qtn_d().m_reduced = false;
        m_ribbonGroup->show();
    }
}

void RibbonDefaultGroupButton::resetPopopGroup()
{
	// BEGIN ROBLOX CHANGES
	QObject::disconnect(m_ribbonGroup, SIGNAL(hidePopup()), this, SLOT(resetPopopGroup()));
	// END ROBLOX CHANGES
    if (m_eventLoop)
        m_eventLoop->exit(); 
    m_hasPopup = false;
    repaint();
}

QSize RibbonDefaultGroupButton::sizeHint() const
{
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    return QSize(style()->pixelMetric((QStyle::PixelMetric)RibbonStyle::PM_RibbonReducedGroupWidth, &opt, this), m_ribbonGroup->sizeHint().height());
}

bool RibbonDefaultGroupButton::event(QEvent* event)
{
    if (event->type() == PostDeleteEvent::eventNumber())
    {
        delete this;
        return true;
    }
    return QToolButton::event(event);
}

void RibbonDefaultGroupButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this );
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    if (m_hasPopup)
        opt.features |= QStyleOptionToolButton::HasMenu;
    QRect rcGroup = opt.rect;
    rcGroup.setBottom(rcGroup.bottom() - 1);
    opt.rect = rcGroup;
    style()->drawControl((QStyle::ControlElement)RibbonStyle::CE_ReducedGroup, &opt, &p, this);
}

void RibbonDefaultGroupButton::mousePressEvent(QMouseEvent* event)
{
    if (!m_ribbonGroup)
        return;

    if (m_ribbonGroup->qtn_d().m_reduced && !m_hasPopup )
    {
        QStyleOptionToolButton opt;
        initStyleOption(&opt);
        if (event->button() == Qt::LeftButton && popupMode() == QToolButton::MenuButtonPopup) 
        {
            QRect popupr = style()->subControlRect(QStyle::CC_ToolButton, &opt, QStyle::SC_ToolButtonMenu, this);
            if (popupr.isValid() && popupr.contains(event->pos())) 
            {
                QPoint pnt(0, 0);
                QRect rc = rect();

                QRect screen = QApplication::desktop()->availableGeometry(this);
                int h = rc.height();
                if (this->mapToGlobal(QPoint(0, rc.bottom())).y() + h <= screen.height()) 
                    pnt = this->mapToGlobal(rc.bottomLeft());
                else 
                    pnt = this->mapToGlobal(rc.topLeft() - QPoint(0, h));

                QSize size = m_ribbonGroup->sizeHint();
                const int desktopFrame = style()->pixelMetric(QStyle::PM_MenuDesktopFrameWidth, 0, this);

                if (pnt.x() + size.width() - 1 > screen.right() - desktopFrame)
                    pnt.setX(screen.right() - desktopFrame - size.width() + 1);
                if (pnt.x() < screen.left() + desktopFrame)
                    pnt.setX(screen.left() + desktopFrame);

                m_hasPopup = true;
                m_ribbonGroup->setGeometry(QRect(pnt, size));
                m_ribbonGroup->show();
                repaint();

                QObject::connect(m_ribbonGroup, SIGNAL(hidePopup()), this, SLOT(resetPopopGroup()));
                QEventLoop eventLoop;
                m_eventLoop = &eventLoop;
                (void) eventLoop.exec();
				// BEGIN ROBLOX CHANGES
                //QObject::disconnect(m_ribbonGroup, SIGNAL(hidePopup()), this, SLOT(resetPopopGroup()));
				// END ROBLOX CHANGES
                m_eventLoop = Q_NULL;
                return;
            }
        }
    }
}

void RibbonDefaultGroupButton::mouseReleaseEvent(QMouseEvent* event)
{
	// BEGIN ROBLOX CHANGES
	//m_hasPopup = false;
    m_hasPopup = QApplication::activePopupWidget();
	// END ROBLOX CHANGES
    QToolButton::mouseReleaseEvent(event);
}


/*!
\class Qtitan::RibbonGroupOption
\internal
*/
RibbonGroupOption::RibbonGroupOption(QWidget* parent)
    : QToolButton(parent)
{
}

RibbonGroupOption::~RibbonGroupOption()
{
}

QString RibbonGroupOption::text() const
{
    return "";
}

QSize RibbonGroupOption::sizeHint() const
{
    return QSize(15, 14);
}

void RibbonGroupOption::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    opt.iconSize = opt.icon.actualSize(QSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX));
    style()->drawPrimitive((QStyle::PrimitiveElement)RibbonStyle::PE_RibbonOptionButton, &opt, &p, this);
}

void RibbonGroupOption::actionEvent(QActionEvent* event)
{
    QToolButton::actionEvent(event);
    if (event->type() == QEvent::ActionChanged)
    {
        QAction* action = event->action();
        setPopupMode(action->menu() ? QToolButton::MenuButtonPopup : QToolButton::DelayedPopup);
    }
}


/*!
\class Qtitan::RibbonGroupScroll
\internal
*/
RibbonGroupScroll::RibbonGroupScroll(QWidget* parent, bool scrollLeft)
    : QToolButton(parent)
{
    m_scrollLeft = scrollLeft;
}

RibbonGroupScroll::~RibbonGroupScroll()
{
}

void RibbonGroupScroll::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    opt.arrowType = m_scrollLeft ? Qt::LeftArrow : Qt::RightArrow;
    style()->drawPrimitive((QStyle::PrimitiveElement)RibbonStyle::PE_RibbonGroupScrollButton, &opt, &p, this);
}


/*!
\class Qtitan::KeyTipEvent
\internal
*/
KeyTipEvent::KeyTipEvent(RibbonKeyTip* kTip) 
    : QEvent(KeyTipEvent::eventNumber())
{
    keyTip = kTip;
}

RibbonKeyTip* KeyTipEvent::getKeyTip() const
{
    return keyTip;
}

QEvent::Type KeyTipEvent::eventNumber()
{
    if (keyTipEventNumber < 0)
        keyTipEventNumber = QEvent::registerEventType();
    return (QEvent::Type) keyTipEventNumber;
}

/*!
\class Qtitan::ShowKeyTipEvent
\internal
*/
ShowKeyTipEvent::ShowKeyTipEvent(QWidget* w) 
    : QEvent(ShowKeyTipEvent::eventNumber())
{
    widget = w;
}

QWidget* ShowKeyTipEvent::getWidget() const
{
    return widget;
}

QEvent::Type ShowKeyTipEvent::eventNumber()
{
    if (showKeyTipEventNumber < 0)
        showKeyTipEventNumber = QEvent::registerEventType();
    return (QEvent::Type) showKeyTipEventNumber;
}

/*!
\class Qtitan::HideKeyTipEvent
\internal
*/
HideKeyTipEvent::HideKeyTipEvent() 
    : QEvent(HideKeyTipEvent::eventNumber())
{
}

QEvent::Type HideKeyTipEvent::eventNumber()
{
    if (hideKeyTipEventNumber < 0)
        hideKeyTipEventNumber = QEvent::registerEventType();
    return (QEvent::Type) hideKeyTipEventNumber;
}

/*!
\class Qtitan::HideKeyTipEvent
\internal
*/
MinimizedEvent::MinimizedEvent() 
    : QEvent(MinimizedEvent::eventNumber())
{
}

QEvent::Type MinimizedEvent::eventNumber()
{
    if (minimizedEventNumber < 0)
        minimizedEventNumber = QEvent::registerEventType();
    return (QEvent::Type) minimizedEventNumber;
}
