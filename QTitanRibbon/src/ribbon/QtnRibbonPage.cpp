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
#include <QPainter>
#include <QBasicTimer>
#include <QStyleOption>
#include <qevent.h>

#include "QtnRibbonStyle.h"
#include "QtnStyleHelpers.h"
#include "QtnRibbonPage.h"
#include "QtnRibbonGroup.h"
#include "QtnRibbonBar.h"
#include "QtnRibbonTabBar.h"
#include "QtnRibbonPrivate.h"
#include "QtnRibbonBarPrivate.h"
#include "QtnRibbonGroupPrivate.h"

#ifdef Q_OS_WIN
#include <qt_windows.h>
#endif // Q_OS_WIN

using namespace Qtitan;


static const int groupMarginRight = 2;
static const int groupMarginTop = 3;
static const int groupMarginBottom = 4;
static const int groupMarginLeft = 2;

namespace Qtitan
{
    /* RibbonPagePrivate */
    class RibbonPagePrivate : public QObject
    {
    public:
        QTN_DECLARE_PUBLIC(RibbonPage)
    public:
        explicit RibbonPagePrivate();

    public:
        int maxHeight();
        int minWidth();
        void recalcGroupWidths();
        void resetGroupsLayout();
        void cleanUpSizes();
        void reposGroups();
        bool setGroupsLayout(int width);
        void layoutGroups();

        bool validIndex(int index) const { return index >= 0 && index < m_listGroups.count(); }

        void removeGroup(int index);
        int getIndexGroup(RibbonGroup* page) const;
        virtual bool eventFilter(QObject*, QEvent* event);

    public:
        int m_minWidth;
        int	m_lastPageWidth;
        int m_lastPageOffsetY;
        int	m_scrollOffset;
        QVector<int> m_vecReducedOrder;

        QList<RibbonGroup*> m_listGroups;
        QList<QAction*> m_listShortcuts;
        RibbonTab* m_associativeTab;
        QBasicTimer m_timer;
        QString m_title;
        QString m_contextTitle;
        RibbonPage::ContextColor m_contextColor;
        QSize m_sizeGroups;
        int m_groupsHeight;
        bool m_minimazeRibbon : 1;
        bool m_doVisiblePage  : 1;
        bool m_allowPress     : 1; 
        bool m_doPopupPage    : 1;
    };
}

RibbonPagePrivate::RibbonPagePrivate()
{
    m_minWidth = -1;
    m_lastPageWidth = -1;
    m_lastPageOffsetY = 0;
    m_scrollOffset = 0;

    m_title = QObject::tr("NONAME");
    m_minimazeRibbon = false;
    m_doVisiblePage = false;  
    m_allowPress = false;
    m_doPopupPage = false;
    m_contextColor = RibbonPage::ContextColorNone;
    m_associativeTab = Q_NULL;
    m_groupsHeight = -1;
}

int RibbonPagePrivate::maxHeight()
{
    QTN_P(RibbonPage)
    int maxHeight = 0;

    for (int i = 0; i < m_listGroups.size (); i++)
    {
        RibbonGroup* group = m_listGroups.at(i);
        maxHeight = qMax(maxHeight, group->height());
    }

    return maxHeight + p.sizeHint().height() + groupMarginTop + groupMarginBottom;
}

int RibbonPagePrivate::minWidth()
{
    int minWidth = groupMarginLeft;

    for (int i = 0; i < m_listGroups.size (); i++)
    {
        RibbonGroup* group = m_listGroups.at(i);
        minWidth += group->qtn_d().minWidth() + 2;
    }
    return minWidth;
}

void RibbonPagePrivate::recalcGroupWidths()
{
    int height = -1;

    for (int i = 0; i < m_listGroups.size (); i++)
    {
        RibbonGroup* group = m_listGroups.at(i);

        if (group->qtn_d().m_widths.size () == 0)
        {
            if (height == -1)
                height = maxHeight();

            group->qtn_d().recalcWidths(height);
            m_lastPageWidth = -1;
        }
    }

    m_minWidth = -1;
}

void RibbonPagePrivate::resetGroupsLayout()
{
    for (int i = 0; i < m_listGroups.size(); i++)
    {
        RibbonGroup* group = m_listGroups.at(i);

        group->qtn_d().m_currWidthIndex = 0;
        group->qtn_d().m_trancateTitle = false;
    }

    m_scrollOffset = 0;
}

void RibbonPagePrivate::cleanUpSizes()
{
    for (int i = 0; i < m_listGroups.size(); i++)
    {
        RibbonGroup* group = m_listGroups.at(i);
        group->qtn_d().m_reduced = false;
        group->qtn_d().m_widths.clear();
    }

    m_lastPageWidth = -1;
    m_minWidth = -1;
}

void RibbonPagePrivate::reposGroups()
{
    QTN_P(RibbonPage)
    if (RibbonBar* rb = qobject_cast<RibbonBar*>(p.parentWidget()))
    {
        QRect rect = QRect(QPoint(0, 0), p.size());

        QRect rectClient = rect;

        if (!rb->qtn_d().m_rectLogotype.isNull())
        {
            int width = rb->qtn_d().m_rectLogotype.width();
            if (rb->qtn_d().m_alignmentLogotype == Qt::AlignLeft)
                rectClient.setLeft(rectClient.left() + width);
        }

        rectClient.adjust(groupMarginLeft * 2, groupMarginTop, -groupMarginRight * 2, -groupMarginBottom);

        const bool reduced = (rectClient.width() <= m_minWidth);

        int x = rectClient.left() - m_scrollOffset;

        QList<RibbonGroup*> groupsOrdered;

        groupsOrdered.append(m_listGroups);

        QSize size;
        for (int i = 0; i < groupsOrdered.size (); i++)
        {
            RibbonGroup* group = groupsOrdered.at(i);

            if (!group->isVisible())
            {
                group->qtn_d().reposition(QRect(0, 0, 0, 0));
                continue;
            }

            if (reduced)
            {
                group->qtn_d().m_reduced = true;
                group->qtn_d().m_currWidthIndex = (int) group->qtn_d().m_widths.size () - 1;
            }

            const int currGroupWidth = group->qtn_d().m_widths[group->qtn_d().m_currWidthIndex] + 2 * group->qtn_d().m_xMargin;

            QRect rectGroup = QRect(QPoint(x, rectClient.top()), QPoint(x + currGroupWidth, rectClient.bottom()));

            group->qtn_d().reposition(rectGroup);

            x = group->qtn_d().m_reduced ? group->qtn_d().m_ribbonDefaultGroupButton->geometry().right() + 2 : group->geometry().right() + 2;

            if (rectGroup.right() <= rectClient.left() + 2 * groupMarginLeft ||
                rectGroup.left() >= rectClient.right() - 2 * groupMarginRight)
            {
                rectGroup = QRect();
                group->qtn_d().reposition(rectGroup);
            }

            if (reduced)
                group->qtn_d().m_reduced = true;

            size.setHeight(rectGroup.height());
            size.setWidth(size.width() + rectGroup.width());
        }
        m_sizeGroups = size;
    }
}

bool RibbonPagePrivate::setGroupsLayout(int width)
{
    int totalWidth = 0;

    for (int i = 0; i < m_listGroups.size (); i++)
    {
        RibbonGroup* group = m_listGroups[i];

        if (!group->isVisible())
            continue;

        group->qtn_d().m_reduced = false;

        if (group->qtn_d().m_currWidthIndex == group->qtn_d().m_widths.size() - 1 && group->qtn_d().m_widths.size() > 1)
            group->qtn_d().m_reduced = true;

        const int currGroupWidth = group->qtn_d().m_widths [group->qtn_d().m_currWidthIndex] + 2 * group->qtn_d().m_xMargin;

        totalWidth += currGroupWidth + 2;

		// BEGIN ROBLOX CHANGES
        //if (totalWidth > width)
        //    return false;
		// END ROBLOX CHANGES
    }

	// BEGIN ROBLOX CHANGES
	if (totalWidth > width)
		return false;
	// END ROBLOX CHANGES

    return true;
}

void RibbonPagePrivate::layoutGroups()
{
    QTN_P(RibbonPage);
    p.setUpdatesEnabled(false);

    QRect rect = QRect(QPoint(0, 0), p.size());

    if (RibbonBar* rb = qobject_cast<RibbonBar*>(p.parentWidget()))
    {
        if (!rb->qtn_d().m_rectLogotype.isNull())
        {
            int width = rb->qtn_d().m_rectLogotype.width();
            if (rb->qtn_d().m_alignmentLogotype == Qt::AlignRight)
                rect.setRight(rect.right() - (width) );
        }
    }

    if (rect.isEmpty())
    {
        p.setUpdatesEnabled(true);
        return;
    }

    if (p.windowFlags() & Qt::Popup)
        cleanUpSizes();

    recalcGroupWidths();

    if (m_listGroups.size() == 0)
    {
        p.setUpdatesEnabled(true);
        return;
    }

    if (m_minWidth < 0)
        m_minWidth = minWidth();

    if (m_lastPageWidth != rect.width () || m_lastPageOffsetY != rect.top())
    {
        m_lastPageWidth = rect.width();
        m_lastPageOffsetY = rect.top();

        QRect rectClient = rect;
        rectClient.adjust(groupMarginLeft * 2, groupMarginTop, groupMarginRight * 2, groupMarginBottom );

        resetGroupsLayout ();

        if (rectClient.width() <= m_minWidth)
        {
            for (int i = 0; i < m_listGroups.size (); i++)
            {
                RibbonGroup* group = m_listGroups.at(i);

                group->qtn_d().m_reduced = true;
                group->qtn_d().m_currWidthIndex = (int)group->qtn_d().m_widths.size() - 1;
            }
        }
        else
        {
            bool autoResize = true;

            if (m_vecReducedOrder.size () > 0)
            {
                autoResize = false;

                bool flagOk = true;

                for (int iNextPane = 0; iNextPane <= m_vecReducedOrder.size (); iNextPane++)
                {
                    flagOk = setGroupsLayout(rectClient.width());
                    if (flagOk || iNextPane == m_vecReducedOrder.size ())
                        break;

                    int groupIndex = m_vecReducedOrder [iNextPane];
                    if (groupIndex < 0 || groupIndex >= m_listGroups.size())
                    {
                        Q_ASSERT (false);
                        flagOk = false;
                        break;
                    }

                    RibbonGroup* group = m_listGroups[groupIndex];

                    if (iNextPane < m_vecReducedOrder.size() - 1 &&
                        m_vecReducedOrder [iNextPane + 1] == -1)
                    {
                        group->qtn_d().m_reduced = true;
                        group->qtn_d().m_currWidthIndex = (int) group->qtn_d().m_widths.size() - 1;

                        iNextPane++;
                    }
                    else
                    {
                        if (group->qtn_d().m_currWidthIndex < group->qtn_d().m_widths.size() - 1)
                            group->qtn_d().m_currWidthIndex++;
                    }
                }

                if (!flagOk)
                {
                    autoResize = true;
                    resetGroupsLayout();
                }
            }

            if (autoResize)
            {
                while (true)
                {
                    if (setGroupsLayout(rectClient.width()))
                        break;

                    int maxWeightIndex = -1;
                    int maxWeight = 1;

                    for (int i = 0; i < m_listGroups.size (); i++)
                    {
                        RibbonGroup* group = m_listGroups[i];

                        int nWeight = (int) group->qtn_d().m_widths.size() - group->qtn_d().m_currWidthIndex - 1;
                        if (nWeight >= maxWeight)
                        {
                            maxWeightIndex = i;
                            maxWeight = nWeight;
                        }
                    }

                    if (maxWeightIndex < 0)
                        break;

                    RibbonGroup* group = m_listGroups[maxWeightIndex];
                    group->qtn_d().m_currWidthIndex++;
                }
            }
            reposGroups();
        }

    }
    p.setUpdatesEnabled(true);
}

void RibbonPagePrivate::removeGroup(int index)
{
    if (validIndex(index)) 
    {
        RibbonGroup* group = m_listGroups.at(index);

        group->removeEventFilter(this);
        QList<QAction*> actList = group->actions();

        for (int i = actList.size()-1; i >= 0; --i) 
        {
            QAction* a = actList.at(i);
            m_listShortcuts.removeOne(a);
        }

        m_listGroups.removeAt(index);
        delete group;
		QTN_P(RibbonPage);
		if (p.isHidden())
		{
			m_lastPageWidth = -1;
			m_minWidth = -1;
		}
		else
		{
			 layoutGroups();
		}
    }
}

int RibbonPagePrivate::getIndexGroup(RibbonGroup* group) const
{
    return m_listGroups.indexOf(group);
}

bool RibbonPagePrivate::eventFilter(QObject* obj, QEvent* event)
{
    bool res = QObject::eventFilter(obj, event);

    if (!qobject_cast<RibbonGroup*>(obj))
        return res;

    switch (event->type()) 
    {
        case QEvent::ActionAdded: 
            if (QActionEvent* actEvent = static_cast<QActionEvent*>(event))
            {
                m_listShortcuts.append(actEvent->action());
                if (m_associativeTab)
                    m_associativeTab->addAction(actEvent->action());
            }
            break;
        case QEvent::ActionRemoved: 
            if (QActionEvent* actEvent = static_cast<QActionEvent*>(event))
            {
                if (m_associativeTab)
                    m_associativeTab->removeAction(actEvent->action());

                int index = m_listShortcuts.indexOf(actEvent->action());
                if (index != -1)
                    m_listShortcuts.removeAt(index);
            }
            break;
        default:
            break;
    }

    return res;
}


/* RibbonPage */
RibbonPage::RibbonPage(QWidget* parent)
    : QWidget(parent)
{
    QTN_INIT_PRIVATE(RibbonPage);
}

RibbonPage::RibbonPage(QWidget* parent, const QString& title)
    : QWidget(parent)
{
    QTN_INIT_PRIVATE(RibbonPage);
    setTitle(title);
}

RibbonPage::~RibbonPage()
{
    if (RibbonBar* ribbon = qobject_cast<RibbonBar*>(parentWidget()))
        ribbon->detachPage(this);
    QTN_FINI_PRIVATE();
}

bool RibbonPage::isVisible() const
{
    QTN_D(const RibbonPage);
    return QWidget::isVisible() || (d.m_associativeTab && d.m_associativeTab->isVisible());
}

void RibbonPage::addGroup(RibbonGroup* group)
{
    insertGroup(-1, group);
}

RibbonGroup* RibbonPage::addGroup(const QString& title)
{
    return insertGroup(-1, title);
}

RibbonGroup* RibbonPage::addGroup(const QIcon& icon, const QString& title)
{
    return insertGroup(-1, icon, title);
}

void RibbonPage::insertGroup(int index, RibbonGroup* group)
{
    QTN_D(RibbonPage);

    setUpdatesEnabled(false);
    group->setParent(this);

    if (!d.validIndex(index)) 
    {
        index = d.m_listGroups.count();
        d.m_listGroups.append(group);
    } 
    else 
        d.m_listGroups.insert(index, group);

    connect(group, SIGNAL(actionTriggered(QAction*)), this, SLOT(actionTriggered(QAction*)), Qt::QueuedConnection);
    connect(group, SIGNAL(released()), this, SLOT(released()));

    group->show();

    group->installEventFilter(&d);

    d.m_lastPageWidth = -1;
    d.m_minWidth = -1;
}

RibbonGroup* RibbonPage::insertGroup(int index, const QString& title)
{
    RibbonGroup* group = new RibbonGroup(Q_NULL, title);
    insertGroup(index, group);
    return group;
}

RibbonGroup* RibbonPage::insertGroup(int index, const QIcon& icon, const QString& title)
{
    if (RibbonGroup* group = insertGroup(index, title))
    {
        group->setIcon(icon);
        return group;
    }
    return Q_NULL;
}

void RibbonPage::removeGroup(RibbonGroup* group)
{
    QTN_D(RibbonPage);
    removeGroup(d.getIndexGroup(group));
}

void RibbonPage::removeGroup(int index)
{
    QTN_D(RibbonPage);
    d.removeGroup(index);
}

QAction* RibbonPage::defaultAction() const
{
    QTN_D(const RibbonPage);
    if (d.m_associativeTab)
        return d.m_associativeTab->defaultAction();
    return Q_NULL;
}

int RibbonPage::groupCount() const
{
    QTN_D(const RibbonPage);
    return d.m_listGroups.count();
}

RibbonGroup* RibbonPage::getGroup(int index) const
{
    QTN_D(const RibbonPage);
    if (index < 0 || index >= d.m_listGroups.size())
        return Q_NULL;
    return d.m_listGroups[index];
}

QList<RibbonGroup*> RibbonPage::groups() const
{
    QTN_D(const RibbonPage);
    return d.m_listGroups;
}

void RibbonPage::clearGroups()
{
    QTN_D(RibbonPage);
    for(int i = (int)d.m_listGroups.count()-1; i >= 0; i--)
        removeGroup(i);
}

/*!
\brief Sets the predefined \a color for the contextual page.
 */ 
void RibbonPage::setContextColor(ContextColor color)
{
    QTN_D(RibbonPage);
    d.m_contextColor = color;

    if (d.m_associativeTab)
    {
        d.m_associativeTab->setContextTab(color);
        repaint();
        parentWidget()->repaint();
    }
}

/*!
\brief Returns the predefined color for the contextual page.
 */ 
RibbonPage::ContextColor RibbonPage::contextColor() const
{
    QTN_D(const RibbonPage);
//    return d.m_associativeTab ? d.m_associativeTab->getContextColor() : ContextColorNone;
    return d.m_contextColor;
}

void RibbonPage::setVisible(bool visible)
{
    QTN_D(RibbonPage);
    if (d.m_associativeTab)
    {
        d.m_associativeTab->setContextTab(d.m_contextColor);
        d.m_associativeTab->setContextTextTab(d.m_contextTitle);
    }

    bool saveVisible = isVisible();

    if (d.m_minimazeRibbon && d.m_associativeTab)
    {
        if (d.m_doPopupPage)
            QWidget::setVisible(visible);

        if (!d.m_doPopupPage)
        {
            d.m_associativeTab->setVisible(visible);
            if (saveVisible != isVisible())
            {
                if (RibbonBar* ribbonBar = qobject_cast<RibbonBar*>(parentWidget()))
                {
                    QEvent ev(QEvent::LayoutRequest);
                    QApplication::sendEvent(ribbonBar, &ev);
                    ribbonBar->update();
                }
            }
        }

        if (!visible && d.m_doPopupPage)
            d.m_doPopupPage = false;
    }
    else
    {
        if (d.m_doVisiblePage)
            QWidget::setVisible(visible);
        else if (!visible)
            QWidget::setVisible(false);


        if (!d.m_doVisiblePage && d.m_associativeTab)
            d.m_associativeTab->setVisible(visible);

        if (!d.m_doVisiblePage && !visible)
        {
            if (RibbonBar* ribbon = qobject_cast<RibbonBar*>(parentWidget()))
            {
                if (ribbon->getPageCount() > 0)
                {
                    RibbonPage* page = ribbon->getPage(ribbon->currentIndexPage());
                    if (this == page)
                        ribbon->setCurrentPage(0);
                }
            }
        }

        if (saveVisible != isVisible())
        {
            if (RibbonBar* ribbonBar = qobject_cast<RibbonBar*>(parentWidget()))
            {
                QEvent ev(QEvent::LayoutRequest);
                QApplication::sendEvent(ribbonBar, &ev);
                ribbonBar->update();
            }
        }
    }
}

void RibbonPage::setTitle(const QString& title)
{
    QTN_D(RibbonPage);

    if (d.m_title == title)
        return;

    d.m_title = title;

    if (d.m_associativeTab)
        d.m_associativeTab->setTextTab(d.m_title);

    emit titleChanged(d.m_title);
}

const QString& RibbonPage::title() const
{
    QTN_D(const RibbonPage);
    return d.m_associativeTab ? d.m_associativeTab->textTab() : d.m_title;
}

void RibbonPage::setContextTitle(const QString& title)
{
    QTN_D(RibbonPage);
    d.m_contextTitle = title;

    if (d.m_associativeTab)
        d.m_associativeTab->setContextTextTab(d.m_contextTitle);
}

void RibbonPage::released()
{
    QTN_D(RibbonPage);
    if (d.m_minimazeRibbon && isVisible() && QApplication::activePopupWidget() == this)
        hide();
}

void RibbonPage::actionTriggered(QAction* action)
{
    QTN_D(RibbonPage);

    if (d.m_listShortcuts.indexOf(action) != -1)
        return;

    if (d.m_minimazeRibbon && isVisible())
    {
        if (action->menu())
            return;
        close();
    }
}

const QString& RibbonPage::contextTitle() const
{
    QTN_D(const RibbonPage);
    return d.m_associativeTab ? d.m_associativeTab->contextTextTab() : d.m_title;
}

void RibbonPage::setAssociativeTab(QWidget* widget)
{
    QTN_D(RibbonPage);
    d.m_associativeTab = qobject_cast<RibbonTab*>(widget);

    if (!d.m_associativeTab)
        return;

    for (int j = 0, count = groupCount(); count > j; j++)
    {
        if (RibbonGroup* group = getGroup(j))
        {
            QList<QAction*> actList = group->actions();

            for (int i = 0, actCount = actList.size(); actCount > i; i++) 
            {
                QAction* a = actList.at(i);
                d.m_listShortcuts.append(a);
                d.m_associativeTab->addAction(a);
            }
        }
    }
}

void RibbonPage::setPageVisible(bool visible)
{
    QTN_D(RibbonPage);
    d.m_doVisiblePage = true;
    if (visible && d.m_associativeTab && !d.m_associativeTab->isHidden())
    {
        setVisible(true);
        emit activated();
    }
    else if (d.m_associativeTab && !d.m_associativeTab->isHidden())
        setVisible(false);
    d.m_doVisiblePage = false;
}

void RibbonPage::popup()
{
    QTN_D(RibbonPage);
    d.m_doPopupPage = true;
    show();
}

void RibbonPage::setRibbonMinimized(bool minimized)
{
    QTN_D(RibbonPage);
    d.m_minimazeRibbon = minimized;
}

void RibbonPage::activatingPage(bool& allow)
{
    emit activating(allow);
}

void RibbonPage::setGroupsHeight(int height)
{
    QTN_D(RibbonPage);
    d.m_groupsHeight = height;
}

bool RibbonPage::event(QEvent* event)
{
    QTN_D(RibbonPage);
    switch(event->type())
    {
        case QEvent::LayoutRequest :
            {
                if (!(windowFlags() & Qt::Popup))
                {
                    d.m_lastPageWidth = -1;
                    d.m_minWidth = -1;

                    d.layoutGroups();
                }
            }
            break;
        case QEvent::Show :
            if (d.m_minimazeRibbon && d.m_associativeTab)
            {
                d.m_associativeTab->setTrackingMode(true);
                d.m_timer.start(QApplication::doubleClickInterval(), this);
                d.m_allowPress = true;
            }
            break;
        case QEvent::Hide :
            if (d.m_minimazeRibbon && d.m_associativeTab)
            {
                d.m_associativeTab->setTrackingMode(false);
                d.m_allowPress = true;
            }
            break;
        case QEvent::Timer :
            if (d.m_minimazeRibbon && d.m_associativeTab)
            {
                QTimerEvent* pTimerEvent = (QTimerEvent*)event;
                if (d.m_timer.timerId() == pTimerEvent->timerId()) 
                {
                    d.m_allowPress = false;
                    return true;
                }
            }
            break;
        case QEvent::MouseButtonPress :
            if (d.m_minimazeRibbon && d.m_associativeTab)
            {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (!rect().contains(mouseEvent->pos())) 
                {
                    if (d.m_allowPress)
                    {
                        QWidget* clickedWidget = QApplication::widgetAt(mouseEvent->globalPos());

                        if (clickedWidget == d.m_associativeTab)
                        {
                            const QPoint targetPoint = clickedWidget->mapFromGlobal(mouseEvent->globalPos());

                            QMouseEvent evPress(mouseEvent->type(), targetPoint, mouseEvent->globalPos(), mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());
                            QApplication::sendEvent(clickedWidget, &evPress);

                            QMouseEvent eDblClick(QEvent::MouseButtonDblClick, targetPoint, mouseEvent->globalPos(), mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());
                            QApplication::sendEvent(d.m_associativeTab, &eDblClick);
                            return true;
                        }
                    }
                    else if (d.m_associativeTab && QRect(d.m_associativeTab->mapToGlobal(QPoint()), d.m_associativeTab->size()).contains(mouseEvent->globalPos()))
                        setAttribute(Qt::WA_NoMouseReplay);

                    if (QApplication::activePopupWidget() == this)
                    {
                        if (QWidget* clickedTab = qobject_cast<RibbonTab*>(QApplication::widgetAt(mouseEvent->globalPos())))
                        {
                            if (d.m_associativeTab && clickedTab != d.m_associativeTab)
                            {
                                const QPoint targetPoint = clickedTab->mapFromGlobal(mouseEvent->globalPos());

                                QMouseEvent evPress(mouseEvent->type(), targetPoint, mouseEvent->globalPos(), mouseEvent->button(), mouseEvent->buttons(), mouseEvent->modifiers());
                                QApplication::sendEvent(clickedTab, &evPress);

                                return false;
                            }
                        }
                    }
                }
            }
            break;
        default:
            break;
    }

    return QWidget::event(event);
}

QSize RibbonPage::sizeHint() const
{
    QTN_D(const RibbonPage);
    return d.m_sizeGroups;
}

void RibbonPage::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QTN_D(RibbonPage);

    QWidget* parent = this->parentWidget();
    if (parent && (d.m_minimazeRibbon || !qobject_cast<RibbonBar*>(parent)))
    {
        QPainter p(this);
        StyleOptionRibbon opt;
        opt.init(this);
        style()->drawControl((QStyle::ControlElement)RibbonStyle::CE_RibbonGroups, &opt, &p, this->parentWidget());
    }
}

void RibbonPage::changeEvent(QEvent* event)
{
    QTN_D(RibbonPage);
    switch (event->type()) 
    {
        case QEvent::StyleChange:
            {
                d.cleanUpSizes();
                QEvent ev(QEvent::LayoutRequest);
                QApplication::sendEvent(this, &ev);
            }
            break;
        case QEvent::FontChange: 
            {
                d.cleanUpSizes();
                for (int i = 0; i < d.m_listGroups.size (); i++)
                {
                    RibbonGroup* group = d.m_listGroups.at(i);
                    group->setFont(font());
                }
                QEvent ev(QEvent::LayoutRequest);
                QApplication::sendEvent(this, &ev);
            }
            break;
        default:
            break;
    };
    return QWidget::changeEvent(event);
}

void RibbonPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);
    QTN_D(RibbonPage);
    d.layoutGroups();
}

void RibbonPage::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    QTN_D(RibbonPage);

    QSize delta = event->size() - event->oldSize();

    if (delta.isNull())
        return;

//    d.m_lastPageWidth = -1;
//    d.m_minWidth = -1;

    d.layoutGroups();
}


#ifdef Q_OS_WIN
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
bool RibbonPage::nativeEvent(const QByteArray& eventType, void* message, long* result)
{
    bool res = QWidget::nativeEvent(eventType, message, result);
    MSG* msg = static_cast<MSG *>(message);
#else
bool RibbonPage::winEvent(MSG* message, long* result)
{
    bool res = QWidget::winEvent(message, result);
    MSG* msg = static_cast<MSG *>(message);
#endif
    if (!res)
    {
        if (msg->message == WM_LBUTTONDOWN) 
        {
            if (QWidget* activePopupWidget = QApplication::activePopupWidget())
            {
                if (activePopupWidget == this)
                {
                    POINT curPos = msg->pt;
                    QPoint globalPos(curPos.x, curPos.y);

                    QPoint pos = mapFromGlobal(globalPos);

                    QToolButton* toolButton = qobject_cast<QToolButton*>(activePopupWidget->childAt(pos));

                    if (toolButton && toolButton->isEnabled())
                    {
                        if (QAction* action = toolButton->defaultAction())
                        {
                            if (action->menu())
                            {
                                int button = Qt::LeftButton;
                                int state = 0;

                                pos = toolButton->mapFromGlobal(globalPos);

                                QMouseEvent e(QEvent::MouseButtonPress, pos, globalPos,
                                    Qt::MouseButton(button),
                                    Qt::MouseButtons(state & Qt::MouseButtonMask),
                                    Qt::KeyboardModifiers(state & Qt::KeyboardModifierMask));

                                res = QApplication::sendEvent(toolButton, &e);
                                res = res && e.isAccepted();
                            }
                        }
                    }
                }
            }
        }
    }
    return res;
}
#endif



