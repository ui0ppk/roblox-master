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
#ifndef QTN_RIBBONBACKSTAGEPVIEWRIVATE_H
#define QTN_RIBBONBACKSTAGEPVIEWRIVATE_H

#include <QObject>
#include <QTimer>
#include <QHash>

#include "QtnRibbonBackstageView.h"

class QStyleOptionMenuItem;
class QWidgetAction;

namespace Qtitan
{
    /* RibbonBackstageViewPrivate */
    class RibbonBackstageCloseButton : public QToolButton
    {
        Q_OBJECT
    public:
        RibbonBackstageCloseButton(QWidget* parent);
        ~RibbonBackstageCloseButton();

    protected:
        virtual bool event(QEvent* event);
        virtual void paintEvent(QPaintEvent* event);
        virtual void mousePressEvent(QMouseEvent* event);
    };

    class RibbonBar;
    class HackFilter;
    /* RibbonBackstageViewPrivate */
    class RibbonBackstageViewPrivate : public QObject
    {
    public:
        Q_OBJECT
        QTN_DECLARE_PUBLIC(RibbonBackstageView)
    public:
        explicit RibbonBackstageViewPrivate();
        virtual ~RibbonBackstageViewPrivate();

    public:
        void init(QWidget* parent);
        void initStyleOption(QStyleOptionMenuItem* option, const QAction* action) const;
        void layoutBackstage();
        void updateActionRects() const;
        void updateGeometryPage(QWidget* widget);

        void setCurrentAction(QAction *action);
        QAction* currentAction() const;

        QRect actionRect(QAction* act) const;

        QWidgetAction* getAction(QWidget* w) const;
        QAction* actionAt(const QPoint& p) const;

    protected Q_SLOTS:
        void trackMouseEvent();

    public:
        RibbonBar* m_ribbon; 
        QWidget* m_activePage;
        QAction* m_currentAction;

        bool m_mouseDown;
        bool m_closePrevented;

        QTimer m_timer;
        mutable bool m_itemsDirty;
        mutable int m_menuWidth;
        mutable int m_maxIconWidth;
        mutable QVector<QRect> m_actionRects;
        mutable QHash<QAction*, QWidget*> m_widgetItems;
    };
}; //namespace Qtitan


#endif // QTN_RIBBONBACKSTAGEPVIEWRIVATE_H

