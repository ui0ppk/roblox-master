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
#ifndef QTN_RIBBONBUTTON_H
#define QTN_RIBBONBUTTON_H

#include <QStyle>
#include <QEvent>
#include <QToolButton>

#include "QtitanDef.h"

class QStyleOption;
namespace Qtitan
{
    /* PostDeleteEvent */
    class PostDeleteEvent : public QEvent
    {
    public:
        //! Constructor for the event.
        explicit PostDeleteEvent();
    public:
        // Returns the event type/number for PostDeleteEvent.
        // The type is registered on first access. Use this to detect incoming
        static QEvent::Type eventNumber();
    };


    /* RibbonButton */
    class RibbonButton : public QToolButton
    {
        Q_OBJECT
    public:
        RibbonButton(QWidget* parent = Q_NULL);
        virtual ~RibbonButton();

    public:
        virtual QSize sizeHint() const;
    private:
        Q_DISABLE_COPY(RibbonButton)
    };

    /* RibbonSeparator */
    class RibbonSeparator : public QWidget
    {
        Q_OBJECT
        Q_PROPERTY(QString text READ text WRITE setText NOTIFY textChanged USER true)
    public:
        explicit RibbonSeparator(QWidget* parent = Q_NULL);

    public:
        QString text() const;

    public Q_SLOTS:
        void setText(const QString&);

    Q_SIGNALS:
        void textChanged(const QString&);

    public:
        virtual QSize sizeHint() const;

    protected:
        virtual void paintEvent(QPaintEvent*);

    protected:
        void initStyleOption(QStyleOption* option) const;
    protected:
        QString m_text;

    private:
        Q_DISABLE_COPY(RibbonSeparator)
    };


    /* RibbonTitleButton */
    class RibbonTitleButton : public QToolButton
    {
        Q_OBJECT
    public:
        RibbonTitleButton(QWidget* parent, QStyle::SubControl subControl);
        virtual ~RibbonTitleButton();

    public:
        QStyle::SubControl getSubControl() const;

    protected:
        virtual bool event(QEvent* event);
        virtual void paintEvent(QPaintEvent*);

    private:
        QStyle::SubControl m_subControl;
    };
}; //namespace Qtitan

#endif // QTN_RIBBONBUTTON_H
