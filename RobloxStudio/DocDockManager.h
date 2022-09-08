/**
 * DocDockWidget.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QDockWidget>
#include <QMap>

// Roblox Headers
#include "RBX/BaldPtr.h"

// Roblox Studio Headers
#include "IRobloxDoc.h"

class RobloxMainWindow;
class DocDockWidget;

class DocDockManager : public QObject
{
    Q_OBJECT

    public:

        DocDockManager(RobloxMainWindow& mainWindow);
        virtual ~DocDockManager();

        void addDoc(IRobloxDoc& doc);
        bool removeDoc(IRobloxDoc& doc);
        bool renameDoc(IRobloxDoc& doc,const QString& text,const QString& tooltip);

        bool setCurrentDoc(IRobloxDoc& doc);
        IRobloxDoc* getCurrentDoc() const;

        void startDrag(IRobloxDoc& doc);

    Q_SIGNALS:

        void attachTab(IRobloxDoc& doc);

    private:

        typedef QMap< RBX::BaldPtr<IRobloxDoc>,RBX::BaldPtr<DocDockWidget> > tDocDockMap;

        RobloxMainWindow&   m_MainWindow;
        tDocDockMap         m_Docs;
};

