/****************************************************************************
**
** Qtitan Library by Developer Machines (Microsoft-Ribbon implementation for Qt.C++)
** 
** Copyright (c) 2009-2011 Developer Machines (http://www.devmachines.com)
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
#include <QStyle>
#include <QApplication>
#include <QStyleOption>
#include <QWidgetAction>
#include <QPainter>
#include <QStatusBar>
#include <QBitmap>
#include <qevent.h>

#include "QtnStyleHelpers.h"
#include "QtnRibbonBackstageView.h"
#include "QtnRibbonBackstageViewPrivate.h"
#include "QtnRibbonStyle.h"
#include "QtnRibbonBar.h"

using namespace Qtitan;

const int commandGap = 6;
const int textGap = 15;
const int imageMargin = 7;

/* RibbonBackstageCloseButton */
RibbonBackstageCloseButton::RibbonBackstageCloseButton(QWidget* parent)
    : QToolButton(parent)
{
}

RibbonBackstageCloseButton::~RibbonBackstageCloseButton()
{
}

bool RibbonBackstageCloseButton::event(QEvent* event)
{
    switch(event->type()) 
    {
        case QEvent::HoverEnter:
        case QEvent::HoverLeave:
        case QEvent::HoverMove:
            if (const QHoverEvent* he = static_cast<const QHoverEvent *>(event))
            {
                QPoint pos = he->pos();

                QRect hoverRect;
                QStyleOptionToolButton opt;
                initStyleOption(&opt);

                QList<QSize> sz = opt.icon.availableSizes();
                if (!opt.icon.isNull() && sz.count() > 0)
                {
                    QPixmap pxIcon = opt.icon.pixmap(sz.at(0), QIcon::Active, QIcon::On);

                    int margin = qMax(0, (opt.rect.height() - sz.at(0).height()) / 2);
                    QPoint ptIcon(4 * margin, (opt.rect.top() + opt.rect.bottom()- pxIcon.height()) / 2);

                    opt.rect = QRect(ptIcon, pxIcon.size());

                    opt.subControls = QStyle::SC_All;
                    QStyle::SubControl hoverControl = style()->hitTestComplexControl(QStyle::CC_ToolButton, &opt, pos, this);
                    if (hoverControl == QStyle::SC_None)
                        hoverRect = QRect();
                    else
                        hoverRect = style()->subControlRect(QStyle::CC_ToolButton, &opt, hoverControl, this);

                    if (hoverRect.isNull())
                    {
                        setAttribute(Qt::WA_UnderMouse, false);
                        update();
                        return true;
                    }
                    else
                    {
                        setAttribute(Qt::WA_UnderMouse, true);
                        update();
                    }
                }
            }
            break;
        default:
            break;
    }
    return QToolButton::event(event);
}

void RibbonBackstageCloseButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    QPainter p(this);
    style()->drawPrimitive((QStyle::PrimitiveElement)RibbonStyle::PE_RibbonBackstageCloseButton, &opt, &p, this);
}

void RibbonBackstageCloseButton::mousePressEvent(QMouseEvent* event)
{
    if (testAttribute(Qt::WA_UnderMouse))
        QToolButton::mousePressEvent(event);
}


/* RibbonBackstageSeparator */
RibbonBackstageSeparator::RibbonBackstageSeparator(QWidget* parent)
    : QFrame(parent)
{
    setAttribute(Qt::WA_MouseNoMask); 
    setFrameStyle(HLine | Sunken);
}

RibbonBackstageSeparator::~RibbonBackstageSeparator()
{
}

void RibbonBackstageSeparator::setOrientation(Qt::Orientation orient)
{ 
    setFrameShape(orient == Qt::Horizontal ? HLine : VLine); 
}

Qt::Orientation RibbonBackstageSeparator::orientation() const
{ 
    return frameShape() == HLine ? Qt::Horizontal : Qt::Vertical; 
}



namespace Qtitan
{
    /* RibbonBackstageViewPrivate */
    class RibbonBackstageButtonPrivate : public QObject
    {
    public:
        QTN_DECLARE_PUBLIC(RibbonBackstageButton)
    public:
        explicit RibbonBackstageButtonPrivate();
        virtual ~RibbonBackstageButtonPrivate();
    public:
        bool m_tabStyle;
        bool m_flatStyle;
    };
};

RibbonBackstageButtonPrivate::RibbonBackstageButtonPrivate()
{
    m_tabStyle = false;
    m_flatStyle = false;
}

RibbonBackstageButtonPrivate::~RibbonBackstageButtonPrivate()
{
}

/* RibbonBackstageButton */
RibbonBackstageButton::RibbonBackstageButton(QWidget* parent)
    : QToolButton(parent)
{
    QTN_INIT_PRIVATE(RibbonBackstageButton);
}

RibbonBackstageButton::~RibbonBackstageButton()
{
    QTN_FINI_PRIVATE();
}

bool RibbonBackstageButton::tabStyle() const
{
    QTN_D(const RibbonBackstageButton);
    return d.m_tabStyle;
}

void RibbonBackstageButton::setTabStyle(bool style)
{
    QTN_D(RibbonBackstageButton);
    d.m_tabStyle = style;
    repaint();
}

bool RibbonBackstageButton::flatStyle()
{
    QTN_D(const RibbonBackstageButton);
    return d.m_flatStyle;
}

void RibbonBackstageButton::setFlatStyle(bool flat)
{
    QTN_D(RibbonBackstageButton);
    d.m_flatStyle = flat;
    repaint();
}

/*! \reimp */
void RibbonBackstageButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QTN_D(RibbonBackstageButton);

    QPainter p(this);
    StyleOptionBackstageButton option;
    initStyleOption(&option);
    option.tabStyle = d.m_tabStyle;
    option.flatStyle = d.m_flatStyle;

    style()->drawComplexControl(QStyle::CC_ToolButton, &option, &p, this);
}


/* RibbonBackstagePage */
RibbonBackstagePage::RibbonBackstagePage(QWidget* parent)
    : QWidget(parent)
{
}

RibbonBackstagePage::~RibbonBackstagePage()
{
}

/*! \reimp */
void RibbonBackstagePage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter p(this);

    QStyleOption opt;
    opt.init(this);
    p.fillRect(opt.rect, QColor(255, 255, 255));
}

/*!
\class Qtitan::RibbonBackstageViewPrivate
\internal
*/
RibbonBackstageViewPrivate::RibbonBackstageViewPrivate()
{
    m_ribbon = Q_NULL;
    m_activePage = Q_NULL;
    m_currentAction = Q_NULL;
    m_menuWidth = 132;
    m_itemsDirty = true;
    m_mouseDown = false;
    m_closePrevented = false;
    m_maxIconWidth = 0;

    m_timer.setInterval(200);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(trackMouseEvent()));
}

RibbonBackstageViewPrivate::~RibbonBackstageViewPrivate()
{
}

void RibbonBackstageViewPrivate::init(QWidget* parent)
{
    QTN_P(RibbonBackstageView);
    p.setMouseTracking(true);
    p.setVisible(false);

    p.setFont(QApplication::font("QMenu"));
    p.setPalette(QApplication::palette("QMenu"));

    m_ribbon = qobject_cast<RibbonBar*>(parent);
    if (m_ribbon)
        p.setParent(m_ribbon->parentWidget());
}

void RibbonBackstageViewPrivate::initStyleOption(QStyleOptionMenuItem* option, const QAction* action) const
{
    if (!option || !action)
        return;

    QTN_P(const RibbonBackstageView);
    option->initFrom(&p);
    option->palette = p.palette();
    option->state = QStyle::State_None;

    if (p.window()->isActiveWindow())
        option->state |= QStyle::State_Active;
    if (p.isEnabled() && action->isEnabled()
        && (!action->menu() || action->menu()->isEnabled()))
        option->state |= QStyle::State_Enabled;
    else
        option->palette.setCurrentColorGroup(QPalette::Disabled);

    option->font = action->font().resolve(p.font());
    option->fontMetrics = QFontMetrics(option->font);

    if (m_currentAction && m_currentAction == action && !m_currentAction->isSeparator()) 
        option->state |= QStyle::State_Selected | (m_mouseDown ? QStyle::State_Sunken : QStyle::State_None);

    option->menuItemType = QStyleOptionMenuItem::Normal;
    if (const QWidgetAction* pageAct = qobject_cast<const QWidgetAction*>(action))
    {
        option->menuItemType = QStyleOptionMenuItem::SubMenu;
        if (pageAct->defaultWidget() == m_activePage)
            option->state |= QStyle::State_HasFocus;
    }

    if (action->isIconVisibleInMenu())
        option->icon = action->icon();
    QString textAndAccel = action->text();

#ifndef QT_NO_SHORTCUT
    if (textAndAccel.indexOf(QLatin1Char('\t')) == -1) {
        QKeySequence seq = action->shortcut();
        if (!seq.isEmpty())
            textAndAccel += QLatin1Char('\t') + seq.toString();
    }
#endif
    option->text = textAndAccel;
//    option->tabWidth = d->tabWidth;
    option->maxIconWidth = m_maxIconWidth;
    option->menuRect = p.rect();
}

void RibbonBackstageViewPrivate::layoutBackstage()
{
    QTN_P(RibbonBackstageView);
    if (!m_ribbon)
        return;

    int backstageViewTop = 0;

    if (m_ribbon->isVisible())
        backstageViewTop = m_ribbon->backstageViewTop();

    QWidget* parent = p.parentWidget();
    QRect rect = parent->rect();
    rect.setTop(rect.top() + backstageViewTop);
    p.setGeometry(rect);
    p.raise();
    updateGeometryPage(m_activePage);
}

void RibbonBackstageViewPrivate::updateActionRects() const
{
    QTN_P(const RibbonBackstageView);

    if (!m_itemsDirty)
        return;

    p.ensurePolished();
    QList<QAction*> actions = p.actions();
    //let's reinitialize the buffer
    m_actionRects.resize(actions.count());
    m_actionRects.fill(QRect());

    QStyle *style = p.style();
    QStyleOption opt;
    opt.init(&p);

    const int icone = style->pixelMetric(QStyle::PM_SmallIconSize, &opt, &p);

    QFontMetrics qfm = p.fontMetrics();

    int actionHeight = 26;
    int pageHeight = qMax(37, qfm.height() + 10);

    for (int i = 0, count = actions.count(); i < count; i++)
    {
        QAction *action = actions.at(i);
        if (action->isSeparator() || !action->isVisible() /*|| m_widgetItems.contains(action)*/)
            continue;

        QIcon is = action->icon();
        if (!is.isNull())
            m_maxIconWidth = qMax<uint>(m_maxIconWidth, icone + 4);
    }
    
    for (int i = 0, count = actions.count(); i < count; i++)
    {
        QAction* action = actions.at(i);
        if (!action->isVisible() ||  action->isSeparator())
            continue; // we continue, this action will get an empty QRect

        //let the style modify the above size..
        QStyleOptionMenuItem opt;
        initStyleOption(&opt, action);

        const QFontMetrics& fm = opt.fontMetrics;

        QRect br(fm.boundingRect(action->text()));
        int width = br.width() + commandGap + textGap + commandGap + 6;

        actionHeight = qMax(actionHeight, qMax(fm.height() + 6, qfm.height() + 6));
        width += m_maxIconWidth + imageMargin;
        m_menuWidth = qMax(m_menuWidth, width);
    }

    int top = 7;
    for (int i = 0, count = actions.count(); i < count; i++)
    {
        QAction* action = actions.at(i);
        if (!action->isVisible() ||  action->isSeparator())
            continue; // we continue, this action will get an empty QRect

        if (qobject_cast<QWidgetAction*>(action))
        {
            m_actionRects[i] = QRect(QPoint(0, top), QPoint(m_menuWidth, top + pageHeight));
            top += pageHeight + 2;
        }
        else
        {
            m_actionRects[i] = QRect(QPoint(commandGap, top), QPoint(m_menuWidth - commandGap, top + actionHeight));
            top += actionHeight + 2;
        }
    }

    if (!m_activePage)
    {
        for (int i = 0, count = actions.count(); i < count; i++)
        {
            if (QWidgetAction* action = qobject_cast<QWidgetAction*>(actions.at(i)))
            {
                ((RibbonBackstageView&)p).setActivePage(action->defaultWidget());
                break;
            }
        }
    }
    m_itemsDirty = false;
}

void RibbonBackstageViewPrivate::updateGeometryPage(QWidget* widget)
{
    if (!widget)
        return;

    QTN_P(const RibbonBackstageView);
    QSize minimumSize = widget->minimumSize();
    int top = (bool)p.style()->styleHint((QStyle::StyleHint)RibbonStyle::SH_RibbonBackstageHideTabs) ? 0 : 2; 

    QRect rcContent(QPoint(m_menuWidth, top), QPoint(p.rect().right(), p.rect().bottom()));
    rcContent.setRight(qMax(rcContent.right(), rcContent.left() + minimumSize.width()));
    rcContent.setBottom(qMax(rcContent.bottom(), rcContent.top() + minimumSize.height()));
    widget->setGeometry(rcContent);
}

static inline bool isAncestor(const QObject* obj, const QObject* child)
{
    while (child) 
    {
        if (child == obj)
            return true;
        child = child->parent();
    }
    return false;
}

void RibbonBackstageViewPrivate::setCurrentAction(QAction* action)
{
    QTN_P(RibbonBackstageView);
    if (m_currentAction)
        p.update(actionRect(m_currentAction));

    m_currentAction = action;

    if (action) 
    {
        if (!action->isSeparator()) 
            p.update(actionRect(action));
    }
}

QAction* RibbonBackstageViewPrivate::currentAction() const
{
    return m_currentAction;
}

QRect RibbonBackstageViewPrivate::actionRect(QAction* act) const
{
    QTN_P(const RibbonBackstageView);
    QList<QAction*> actions = p.actions();
    int index = actions.indexOf(act);
    if (index == -1)
        return QRect();

    updateActionRects();

    //we found the action
    return m_actionRects.at(index);
}

QWidgetAction* RibbonBackstageViewPrivate::getAction(QWidget* w) const
{
    QTN_P(const RibbonBackstageView);
    QList<QAction*> actions = p.actions();
    for (int i = 0, count = actions.count(); i < count; i++)
    {
        QWidgetAction* action = dynamic_cast<QWidgetAction*>(actions.at(i));
        if (action && action->defaultWidget() == w)
            return action;
    }
    return Q_NULL;
}

QAction* RibbonBackstageViewPrivate::actionAt(const QPoint& pt) const
{
    QTN_P(const RibbonBackstageView);
    QList<QAction*> actions = p.actions();
    for(int i = 0; i < m_actionRects.count(); i++) 
    {
        if (m_actionRects.at(i).contains(pt))
            return actions.at(i);
    }
    return 0;
}

void RibbonBackstageViewPrivate::trackMouseEvent()
{
    QTN_P(RibbonBackstageView);
    if (m_currentAction)
    {
        QPoint pt = p.mapFromGlobal(QCursor::pos());
        QRect rc = p.rect();
        rc.setWidth(m_menuWidth);
        if (!rc.contains(pt))
            setCurrentAction(Q_NULL);
    }
}


/* RibbonBackstageView */
RibbonBackstageView::RibbonBackstageView(QWidget* parent)
    : QWidget(parent)
{
    QTN_INIT_PRIVATE(RibbonBackstageView);
    QTN_D(RibbonBackstageView);
    d.init(parent);
}

RibbonBackstageView::~RibbonBackstageView()
{
    QTN_FINI_PRIVATE();
}

bool RibbonBackstageView::isClosePrevented() const
{
    QTN_D(const RibbonBackstageView);
    return d.m_closePrevented;
}

void RibbonBackstageView::setClosePrevented(bool prevent)
{
    QTN_D(RibbonBackstageView);
    d.m_closePrevented = prevent;
}

QAction* RibbonBackstageView::addAction(const QIcon& icon, const QString& text)
{
    QAction* action = new QAction(icon, text, this);
    addAction(action);
    return action;
}

QAction* RibbonBackstageView::addPage(QWidget* widget)
{
    QWidgetAction* action = new QWidgetAction(this);
    action->setDefaultWidget(widget);
    widget->setFont(font());
    action->setText(widget->windowTitle());
    addAction(action);
    return action;
}

void RibbonBackstageView::setActivePage(QWidget* widget)
{
    QTN_D(RibbonBackstageView);

    if (d.m_activePage)
    {
        if (QWidgetAction* act = d.getAction(d.m_activePage))
            update(d.actionRect(act));
    }

    QList<QAction*> acts = actions();
    for (int i = 0, count = acts.count(); i < count; i++)
    {
        if (QWidgetAction* action = dynamic_cast<QWidgetAction*>(acts.at(i)))
        {
            QWidget* w = action->defaultWidget();
            if (w == widget)
            {
                d.m_activePage = widget;
                d.updateGeometryPage(widget);
                w->setVisible(true);
            }
            else
                w->setVisible(false);
        }
    }
    if (d.m_activePage)
    {
        if (QWidgetAction* act = d.getAction(d.m_activePage))
            update(d.actionRect(act));
    }
}

QWidget* RibbonBackstageView::getActivePage() const
{
    QTN_D(const RibbonBackstageView);
    return d.m_activePage;
}

QRect RibbonBackstageView::actionGeometry(QAction* act) const
{
    QTN_D(const RibbonBackstageView);
    return d.actionRect(act);
}

void RibbonBackstageView::initStyleOption(StyleOptionRibbonBackstage* opt) const
{
    QTN_D(const RibbonBackstageView);
    opt->init(this);
    opt->menuWidth = d.m_menuWidth;
    opt->rect.adjust(0, 0, -1, -1);
}

void RibbonBackstageView::open()
{
    QTN_D(RibbonBackstageView);

    ensurePolished(); // Get the right font
    emit aboutToShow();

    d.updateActionRects();

    show();
}

bool RibbonBackstageView::event(QEvent* event)
{
    QTN_D(RibbonBackstageView);
    switch (event->type())
    {
        case QEvent::Show :
            {
                d.layoutBackstage();
                if (d.m_ribbon)
                {
                    parentWidget()->installEventFilter(this);
                    QEvent ev(QEvent::LayoutRequest);
                    QApplication::sendEvent(d.m_ribbon, &ev);
                    d.m_ribbon->repaint();
                }
                d.m_timer.start();
                setFocus();
            }
            break;
        case QEvent::Hide :
            {
                if (d.m_ribbon)
                {
                    parentWidget()->removeEventFilter(this);
                    QEvent ev(QEvent::LayoutRequest);
                    QApplication::sendEvent(d.m_ribbon, &ev);
                    d.m_ribbon->repaint();
                }
                d.m_timer.stop();
                emit aboutToHide();
            }
            break;
        case QEvent::KeyPress :
            switch(static_cast<QKeyEvent*>(event)->key())
            {
                case Qt::Key_Escape :
                        close();
                    break;
                case Qt::Key_Up :
                    break;
                case Qt::Key_Down :
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }


    return QWidget::event(event);
}

bool RibbonBackstageView::eventFilter(QObject* object, QEvent* event)
{
    QTN_D(RibbonBackstageView);
    if (d.m_ribbon && parentWidget() == object && isVisible())
    {
        switch (event->type())
        {
            case QEvent::Resize : 
            case QEvent::Move: 
                {
                    d.m_itemsDirty = event->type() != QEvent::Move;
                    d.layoutBackstage();
                }
                break;
            default:
                break;
        }
    }
    return QWidget::eventFilter(object, event);
}

void RibbonBackstageView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QTN_D(RibbonBackstageView);

    d.updateActionRects();

    QPainter p(this);
    QRegion emptyArea = QRegion(rect());

    StyleOptionRibbonBackstage opt;
    initStyleOption(&opt);
    style()->drawPrimitive((QStyle::PrimitiveElement)RibbonStyle::PE_Backstage, &opt, &p, this);

    QList<QAction*> acts = actions();

    //draw the items that need updating..
    for (int i = 0, count = acts.count(); i < count; ++i) 
    {
        QAction* action = acts.at(i);
        QRect adjustedActionRect = d.m_actionRects.at(i);
        if (!event->rect().intersects(adjustedActionRect) /*|| d.m_widgetItems.value(action)*/)
            continue;

        //set the clip region to be extra safe (and adjust for the scrollers)
        QRegion adjustedActionReg(adjustedActionRect);
        emptyArea -= adjustedActionReg;
        p.setClipRegion(adjustedActionReg);

        QStyleOptionMenuItem opt;
        d.initStyleOption(&opt, action);
        opt.rect = adjustedActionRect;
        opt.text = action->text();

        opt.menuItemType = QStyleOptionMenuItem::Normal;
        if (QWidgetAction* pageAct = qobject_cast<QWidgetAction*>(action))
        {
            opt.menuItemType = QStyleOptionMenuItem::SubMenu;
            if (pageAct->defaultWidget() == d.m_activePage)
                opt.state |= QStyle::State_HasFocus;
        }
        style()->drawControl(QStyle::CE_MenuItem, &opt, &p, this);
    }
}

void RibbonBackstageView::actionEvent(QActionEvent* event)
{
    QWidget::actionEvent(event);

    QTN_D(RibbonBackstageView);
    if (event->type() == QEvent::ActionAdded)
    {
        if (QWidgetAction* wa = qobject_cast<QWidgetAction*>(event->action())) 
        {
            QWidget *widget = wa->requestWidget(this);
            if (widget)
                d.m_widgetItems.insert(wa, widget);
        }
    } 
    else if (event->type() == QEvent::ActionRemoved)
    {
        if (QWidgetAction* wa = qobject_cast<QWidgetAction*>(event->action())) 
        {
            if (QWidget *widget = d.m_widgetItems.value(wa))
                wa->releaseWidget(widget);
        }
        d.m_widgetItems.remove(event->action());
    } 
    else if (event->type() == QEvent::ActionChanged)
    {
    }
}

void RibbonBackstageView::mousePressEvent(QMouseEvent* event)
{
    QTN_D(RibbonBackstageView);

    if (event->button() != Qt::LeftButton)
        return;

    QRect rc = rect();
    rc.setRight(rc.left() + d.m_menuWidth);
    if (rc.contains(event->pos()))
    {
        if (QWidgetAction* act = qobject_cast<QWidgetAction*>(d.actionAt(event->pos())))
        {
            if (act && (act->isSeparator() || !act->isEnabled()))
                return;
            if (QWidget* w = act->defaultWidget())
                setActivePage(w);
        }
    }
}

void RibbonBackstageView::mouseMoveEvent(QMouseEvent* event)
{
    QTN_D(RibbonBackstageView);

    QRect rc = rect();
    rc.setRight(rc.left() + d.m_menuWidth);
    if (rc.contains(event->pos()))
    {
        QAction* action = d.actionAt(event->pos());

        if (action)
        {
            d.setCurrentAction(action);
            repaint();
        }
        else
        {
            d.setCurrentAction(Q_NULL);
        }
        setFocus();
    }
}

void RibbonBackstageView::mouseReleaseEvent(QMouseEvent* event)
{
    QTN_D(RibbonBackstageView);

    if (event->button() != Qt::LeftButton)
        return;

    QRect rc = rect();
    rc.setRight(rc.left() + d.m_menuWidth);
    if (rc.contains(event->pos()))
    {
        QAction* action = d.actionAt(event->pos());

        if (action && (action->isSeparator() || !action->isEnabled() || d.m_widgetItems.contains(action)))
            return;

        if (action && action == d.m_currentAction) 
        {
            if (!isClosePrevented())
                close();
            action->activate(QAction::Trigger);
        }
    }
}


