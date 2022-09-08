/**
 * DocTabWidget.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QRubberBand>
#include <QIcon>

// Roblox Headers
#include "RBX/BaldPtr.h"

// Roblox Studio Headers
#include "IRobloxDoc.h"

class QRubberBand;

class RobloxMainWindow;
class RobloxTabWidget;

class DocTabManager : public QObject
{
    Q_OBJECT

    public:

        DocTabManager(RobloxMainWindow& mainWindow);
        virtual ~DocTabManager();

        void addDoc(IRobloxDoc& doc, int index = -1);
        bool removeDoc(IRobloxDoc& doc);
        bool renameDoc(IRobloxDoc& doc,const QString& text,const QString& tooltip,const QIcon& icon = QIcon());

        bool setCurrentDoc(IRobloxDoc& doc);
        IRobloxDoc* getCurrentDoc() const;

        void setDockHoverOverPos(const QPoint& globalPos);
        bool attemptAttach(IRobloxDoc& doc,const QPoint& globalPos);

        void restoreAsCentralWidget();

    public Q_SLOTS:
        void showNextTabPage();
        void showPreviousTabPage();

    protected:

        virtual bool eventFilter(QObject* object,QEvent* event);

    private Q_SLOTS:

        void onCurrentChanged(int index);
        bool onTabCloseRequested(int index);
        void onCurrentTabCloseRequested();
        void onDetachDoc();
        void onFocusChanged(QWidget* oldWidget,QWidget* newWidget);
        
    private:

        void onMousePressEvent(QMouseEvent* event);
        void onMouseMoveEvent(QMouseEvent* event);
        void onMouseReleaseEvent(QMouseEvent* event);
        bool isValidHoverOver(const QPoint& globalPos);
    
        void setupTabPageSwitchingShortcuts();
        void setCurrentTabPage(int index);

        typedef QVector< RBX::BaldPtr<IRobloxDoc> > tDocList;

        tDocList                		m_Docs;
        RobloxMainWindow&       		m_MainWindow;
        QPoint                  		m_ClickPos;
        bool                    		m_MouseDown;
        int                     		m_DragTabIndex;
        QRubberBand             		m_RubberBand;
        RBX::BaldPtr<QWidget>   		m_FakeTab;
		RBX::BaldPtr<RobloxTabWidget>	m_TabWidget;
};

