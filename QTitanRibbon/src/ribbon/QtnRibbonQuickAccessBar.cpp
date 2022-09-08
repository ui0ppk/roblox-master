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
#include <QMenu>
#include <QPainter>
#include <QToolButton>
#include <qevent.h>

#include "QtnRibbonStyle.h"
#include "QtnRibbonBar.h"
#include "QtnStyleHelpers.h"
#include "QtnRibbonQuickAccessBar.h"
#ifdef CUSTOMIZE_V4_WORK
#include "QtnRibbonCustomizationManager.h"
#include "QtnRibbonCustomizationManagerPrivate.h"
#endif // CUSTOMIZE_V4_WORK

using namespace Qtitan;

namespace Qtitan
{
    /* RibbonQuickAccessButton */
    class RibbonQuickAccessButton : public QToolButton
    {
    public:
        RibbonQuickAccessButton(QWidget* parent = Q_NULL);
        virtual ~RibbonQuickAccessButton();

    public:
        virtual QSize sizeHint() const;

    protected:
        virtual void paintEvent(QPaintEvent*);

    private:
        Q_DISABLE_COPY(RibbonQuickAccessButton)
    };
};

/* RibbonQuickAccessButton */
RibbonQuickAccessButton::RibbonQuickAccessButton(QWidget* parent)
    : QToolButton(parent)
{
    setObjectName(QLatin1String("QtnRibbonQuickAccessButton"));
}

RibbonQuickAccessButton::~RibbonQuickAccessButton()
{
}

QSize RibbonQuickAccessButton::sizeHint() const
{
    QSize sz = QToolButton::sizeHint();
    if (sz.isNull())
        sz = QSize(16, 16);
    return QSize(sz.height()*13/sz.width()+1, sz.height());
}

void RibbonQuickAccessButton::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event)
    QPainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    style()->drawPrimitive((QStyle::PrimitiveElement)RibbonStyle::PE_RibbonQuickAccessButton, &opt, &p, this);
}


namespace Qtitan
{
    /* QuickAccessAction */
    class QuickAccessAction : public QAction
    {
    public:
        QuickAccessAction(QObject* p, QAction* srcAction)
            : QAction(srcAction->text(), p)
            , m_srcAction(srcAction)
        {
            setCheckable(true);
            QToolBar* toolBar = qobject_cast<QToolBar*>(parent());
            Q_ASSERT(toolBar);
            setChecked(toolBar->widgetForAction(srcAction));
        }
        void update()
        {
            QToolBar* toolBar = qobject_cast<QToolBar*>(parent());
            Q_ASSERT(toolBar);
            setChecked(toolBar->widgetForAction(m_srcAction));
            setText(m_srcAction->text());
        }

    public:
        QAction* m_srcAction;
    };

    /* QuickAccessActionInvisible */
    class QuickAccessActionInvisible : public QAction
    {
    public:
        QuickAccessActionInvisible(QObject* p, QActionGroup* data)
            : QAction(p)
        {
            setObjectName(QLatin1String("__qtn_Action_Invisible"));
            m_data = data;
            setVisible(false);
        }
    public:
        QActionGroup* m_data;
    };
};


namespace Qtitan
{
    /* RibbonQuickAccessBarPrivate */
    class RibbonQuickAccessBarPrivate : public QObject
    {
    public:
        QTN_DECLARE_PUBLIC(RibbonQuickAccessBar)
    public:
        explicit RibbonQuickAccessBarPrivate();
        virtual ~RibbonQuickAccessBarPrivate();
    
    public:
        void init();
        QuickAccessAction* findQuickAccessAction(QAction* action) const;
        QAction* findBeforeAction(QAction* action) const;
        void updateAction(QAction* action);
        void setActionVisible(QAction* action, bool visible);

    public:
        RibbonQuickAccessButton* m_accessPopup;
        QMenu* m_menu;
        QAction* m_actionAccessPopup;
        QActionGroup* m_customizeGroupAction;
        QList<QAction*> m_actionList;
        bool m_removeAction    : 1;
        bool m_customizeAction : 1;
    };
};

RibbonQuickAccessBarPrivate::RibbonQuickAccessBarPrivate()
{
    m_accessPopup = Q_NULL;
    m_menu = Q_NULL;
    m_actionAccessPopup  = Q_NULL;
    m_customizeGroupAction = Q_NULL;
    m_removeAction = false;
    m_customizeAction = false;
}

RibbonQuickAccessBarPrivate::~RibbonQuickAccessBarPrivate()
{
}

void RibbonQuickAccessBarPrivate::init()
{
    QTN_P(RibbonQuickAccessBar);

    m_accessPopup = new RibbonQuickAccessButton(&p);
    m_accessPopup->setPopupMode(QToolButton::InstantPopup);
    m_menu = new QMenu(&p);
    m_accessPopup->setMenu(m_menu);

    m_actionAccessPopup = p.addWidget(m_accessPopup);
    m_actionAccessPopup->setObjectName(QLatin1String("__qtn_Quick_Access_Button"));

    m_customizeGroupAction = new QActionGroup(&p);
    m_customizeGroupAction->setExclusive(false);
    QObject::connect(m_customizeGroupAction, SIGNAL(triggered(QAction*)), &p, SLOT(customizeAction(QAction*)));

    QObject::connect(m_menu, SIGNAL(aboutToShow()), &p, SLOT(aboutToShowCustomizeMenu()));
    QObject::connect(m_menu, SIGNAL(aboutToHide()), &p, SLOT(aboutToHideCustomizeMenu()));

    p.addAction(new QuickAccessActionInvisible(&p, m_customizeGroupAction));
}

QuickAccessAction* RibbonQuickAccessBarPrivate::findQuickAccessAction(QAction* action) const
{
    QList<QAction*> list = m_customizeGroupAction->actions();
    for (int i = 0; i < list.count(); ++i)
    {
        QuickAccessAction* act = dynamic_cast<QuickAccessAction*>(list[i]);
        if (act && action == act->m_srcAction)
            return act;
    }
    return Q_NULL;
}

QAction* RibbonQuickAccessBarPrivate::findBeforeAction(QAction* action) const
{
    QList<QAction*> list = m_customizeGroupAction->actions();
    bool find = false;
    for (int i = 0, count = list.count(); i < count; ++i)
    {
        if (find)
        {
            if (QuickAccessAction* beforeAct = dynamic_cast<QuickAccessAction*>(list[i]))
            {
                if (beforeAct->isChecked())
                    return beforeAct->m_srcAction;
            }
        }
        if (!find && action == list[i])
            find = true;
    }
    return Q_NULL;
}

void RibbonQuickAccessBarPrivate::updateAction(QAction* action)
{
    if (QuickAccessAction* wrapper = findQuickAccessAction(action))
        wrapper->update();
}

void RibbonQuickAccessBarPrivate::setActionVisible(QAction* action, bool visible)
{
    QTN_P(RibbonQuickAccessBar);
    if (QuickAccessAction* wrapper = findQuickAccessAction(action))
    {
        if (visible)
        {
            if (m_customizeAction)
            {
                QAction* beforeAct = findBeforeAction(wrapper);
                p.insertAction(beforeAct, action);
            }
            else
                p.addAction(action);
        }
        else
        {
            p.removeAction(action);
            m_removeAction = false;
        }
        wrapper->update();
        p.adjustSize();
    }
}


/* RibbonQuickAccessBar */
RibbonQuickAccessBar::RibbonQuickAccessBar(QWidget* parent)
    : QToolBar(parent)
{
    QTN_INIT_PRIVATE(RibbonQuickAccessBar);
    QTN_D(RibbonQuickAccessBar);
    d.init();
}

RibbonQuickAccessBar::~RibbonQuickAccessBar()
{
    QTN_FINI_PRIVATE();
}

/*!
\brief Returns a pointer to the QAction that is associated with the customize button of Ribbon's Quick Access Bar.
*/ 
QAction* RibbonQuickAccessBar::actionCustomizeButton() const
{
    QTN_D(const RibbonQuickAccessBar);
    return d.m_actionAccessPopup;
}

/*!
\brief Sets the visibility of the visual representation of action on the quick access toolbar. Parameter \a action is action, parameter \a visible is a visibility.
*/ 
void RibbonQuickAccessBar::setActionVisible(QAction* action, bool visible)
{
    QTN_D(RibbonQuickAccessBar);
    d.setActionVisible(action, visible);
}

/*!
\brief Returns the visibility of the visual representation for the given \a action.
*/ 
bool RibbonQuickAccessBar::isActionVisible(QAction* action) const
{
    QTN_D(const RibbonQuickAccessBar);
    if (QuickAccessAction* wrapper = d.findQuickAccessAction(action))
        return wrapper->isChecked();
    return false;
}

/*!
\brief Returns the count of the elements visible on Ribbon's Quick Access Bar.
*/ 
int RibbonQuickAccessBar::visibleCount() const
{
    QTN_D(const RibbonQuickAccessBar);
    int visibleCount = 0;
    QList<QAction*> list = d.m_customizeGroupAction->actions();
    for (int i = 0, count = list.count(); i < count; ++i)
    {
        if (QuickAccessAction* beforeAct = dynamic_cast<QuickAccessAction*>(list[i]))
            if (beforeAct->isChecked())
                visibleCount++; 
    }
    return visibleCount;
}

QList<QAction*> RibbonQuickAccessBar::actions()
{
	QTN_D(const RibbonQuickAccessBar);
	QList<QAction*> list = d.m_customizeGroupAction->actions(), result;
	for (int i = 0, count = list.count(); i < count; ++i)
    {
        if (QuickAccessAction* wrapper = dynamic_cast<QuickAccessAction*>(list[i]))
			result.append(wrapper->m_srcAction);
	}
	return result;
}

void RibbonQuickAccessBar::removeAllInstances(QAction* action)
{
	QTN_D(RibbonQuickAccessBar);
    if (QuickAccessAction* wrapper = d.findQuickAccessAction(action))
	{
		d.m_actionList.removeAll(wrapper);
		d.m_customizeGroupAction->removeAction(wrapper);
		delete wrapper;
	}

	QToolBar::removeAction(action);
}

/*!
\brief Returns the recommended size for Ribbon's Quick Access Bar.
\reimp
*/ 
QSize RibbonQuickAccessBar::sizeHint() const
{
    const int heightTabs = style()->pixelMetric((QStyle::PixelMetric)RibbonStyle::PM_RibbonTabsHeight, 0, this);
	if (RibbonBar* ribbonBar = qobject_cast<RibbonBar*>(parentWidget()))
		return QSize(qMin(QToolBar::sizeHint().width(), ribbonBar->availableWidth()), heightTabs+1).expandedTo(QApplication::globalStrut());
    return QSize(QToolBar::sizeHint().width(), heightTabs+1).expandedTo(QApplication::globalStrut());
}

/*! \internal */
void RibbonQuickAccessBar::customizeAction(QAction* action)
{
    QTN_D(RibbonQuickAccessBar);
    d.m_customizeAction = true;
    if (QuickAccessAction* act = dynamic_cast<QuickAccessAction*>(action))
        setActionVisible(act->m_srcAction, !widgetForAction(act->m_srcAction));
    d.m_customizeAction = false;

    QEvent event(QEvent::LayoutRequest);
    QApplication::sendEvent(parentWidget(), &event);
    parentWidget()->update();
}

/*! \internal */
void RibbonQuickAccessBar::aboutToShowCustomizeMenu()
{
    QTN_D(RibbonQuickAccessBar);
    d.m_menu->clear();

    d.m_menu->setSeparatorsCollapsible(false);
    d.m_menu->addSeparator()->setText(tr("Customize Quick Access Toolbar"));

    QListIterator<QAction *> itAction(d.m_actionList);
    while (itAction.hasNext())
    {
        QAction* action = itAction.next();
        d.m_menu->addAction(action);
    }

    emit showCustomizeMenu(d.m_menu);
}

/*! \internal */
void RibbonQuickAccessBar::aboutToHideCustomizeMenu()
{
    QTN_D(RibbonQuickAccessBar);
    d.m_menu->clear();
}

/*! \reimp */
bool RibbonQuickAccessBar::event(QEvent* event)
{
    if ((QEvent::Hide == event->type() || QEvent::Show == event->type()) && parentWidget())
    {
        adjustSize();
        parentWidget()->adjustSize();
    }
    return QToolBar::event(event);
}

/*! \reimp */
void RibbonQuickAccessBar::actionEvent(QActionEvent* event)
{
    QToolBar::actionEvent(event);

    QTN_D(RibbonQuickAccessBar);
    if (d.m_actionAccessPopup)
    {
        if (event->type() == QEvent::ActionAdded)
        {
            if (!d.m_removeAction)
                removeAction(d.m_actionAccessPopup);

            QuickAccessAction* quickAccessAction = d.findQuickAccessAction(event->action());
            if (event->action() != d.m_actionAccessPopup && !quickAccessAction && 
                !dynamic_cast<QuickAccessActionInvisible*>(event->action()))
            {
                QuickAccessAction* act = new QuickAccessAction(this, event->action());

                bool addActionToMenu = true;
#ifdef CUSTOMIZE_V4_WORK
                if (RibbonCustomizationManager::isCustomizeMode())
                {
                    if (RibbonBar* ribbonBar = qobject_cast<RibbonBar*>(parentWidget()))
                    {
                        if (RibbonCustomizationManager* manager = ribbonBar->customizeManager())
                        {
                            QMap<QToolBar*, QList<QAction*>> mapToolBars = manager->qtn_d().m_ribbonManager->defaultToolBars();
                            QList<QAction*> defaultActions = mapToolBars.value(this);
                            if (!defaultActions.contains(event->action()))
                                addActionToMenu = false;
                        }
                    }
                }
#endif // CUSTOMIZE_V4_WORK

                if (addActionToMenu)
                    d.m_actionList.append(act);
//                    d.m_menu->addAction(act);

                d.m_customizeGroupAction->addAction(act);
                adjustSize();
            }
            else if (quickAccessAction)
                quickAccessAction->update();
        }
        else if (event->type() == QEvent::ActionRemoved)
        {
            if (event->action() == d.m_actionAccessPopup)
            {
                d.m_removeAction = true;
                addAction(d.m_actionAccessPopup);
                d.m_removeAction = false;
            }
            else if (QuickAccessActionInvisible* actInvisible = dynamic_cast<QuickAccessActionInvisible*>(event->action()))
            {
//                d.m_menu->clear();
                d.m_actionList.clear();
                QList<QAction*> actList = actInvisible->m_data->actions();
                for (int i = actList.size()-1; i >= 0; i--) 
                {
                    QAction* actionWrapper = actList[i];
                    d.m_customizeGroupAction->removeAction(actionWrapper);
                    delete actionWrapper;
                }
                delete actInvisible;
                addAction(new QuickAccessActionInvisible(this, d.m_customizeGroupAction));
            }
            else
                d.updateAction(event->action());

        }
        else if (event->type() == QEvent::ActionChanged)
        {
            if (event->action() == d.m_actionAccessPopup)
                d.m_accessPopup->setDefaultAction(d.m_actionAccessPopup);
            else if(!dynamic_cast<QuickAccessAction*>(event->action()))
                d.updateAction(event->action());
        }
    }
}

/*! \reimp */
void RibbonQuickAccessBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (RibbonBar* ribbonBar = qobject_cast<RibbonBar*>(parentWidget()))
    {
        QPainter p(this);
        StyleRibbonQuickAccessBar opt;
        opt.init(this);
        opt.quickAccessBarPosition = ribbonBar->quickAccessBarPosition();
        style()->drawControl(QStyle::CE_ToolBar, &opt, &p, this);
    }
}


