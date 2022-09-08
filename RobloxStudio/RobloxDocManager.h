/**
 * RobloxDocManager.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QObject>
#include <QMap>
#include <QIcon>

// 3rd Party Headers
#include "boost/shared_ptr.hpp"

// Roblox Headers
#include "rbx/BaldPtr.h"
#include "V8DataModel/DataModel.h"

// Roblox Studio Headers
#include "IRobloxDoc.h"
#include "LuaSourceBuffer.h"

class QPoint;

class RobloxMainWindow;
class RobloxObjectBrowserDoc;
class RobloxWebDoc;
class RobloxScriptDoc;
class DocTabManager;
class DocDockManager;
class RobloxIDEDoc;

namespace RBX
{
	class Instance;
    class DataModel;
}

typedef QMap< QString,RBX::BaldPtr<IRobloxDoc> > tKeyDocMap;
typedef QList< RBX::BaldPtr<RobloxScriptDoc> > tScriptDocs;
typedef QList<boost::shared_ptr<RBX::Instance> > tPlayModifiedScriptList;

class RobloxDocManager : public QObject
{
    Q_OBJECT

public:
	
	static RobloxDocManager& Instance();

	void initialize(RobloxMainWindow& mainWindow);
	void shutDown();

    bool requestCloseAllDocs();
    bool requestCloseAndSaveAllDocs();
    void closeAllDocs();

    void registerOpenedScriptDoc(RobloxScriptDoc& doc, LuaSourceBuffer script);
	void deregisterScriptDoc(RobloxScriptDoc& doc);
    void closeDoc(const LuaSourceBuffer& script);
	RobloxObjectBrowserDoc* getObjectBrowserDoc();

    const tScriptDocs& getOpenScriptDocs() { return m_OpenScriptDocs; }
	RobloxScriptDoc* findOpenScriptDoc(LuaSourceBuffer script);
    
    tPlayModifiedScriptList& getPlayModifiedScriptList() { return m_PlayModifiedScriptList; }

	IRobloxDoc* getOrCreateDoc(IRobloxDoc::RBXDocType docType);
	RobloxScriptDoc* openDoc(LuaSourceBuffer script);
	
    void removeDoc(IRobloxDoc& doc);

	void configureDocWidget(IRobloxDoc& doc);
	
	void setCurrentDoc(IRobloxDoc* doc);
    IRobloxDoc* getCurrentDoc() const       { return m_pCurrentDoc; }
    RobloxIDEDoc* getPlayDoc() const        { return m_pPlayDoc; }
	RobloxWebDoc* getStartPageDoc() const   { return m_pWebDoc; }

    int getDocCount() const                 { return m_KeyDocMap.size(); }

	void updateWindowMenu();
    void setDocTitle(IRobloxDoc& doc,const QString& displayName,const QString& fileName,const QIcon& icon = QIcon());

    void detachDoc(IRobloxDoc& doc,bool dragging);
    void attachDoc(IRobloxDoc& doc, int index = -1);

    void setDockHoverOverPos(const QPoint& globalPos);
    bool attemptAttach(IRobloxDoc& doc,const QPoint& globalPos);

    void setPlayDocCentralWidget();
    void restoreCentralWidget();

Q_SIGNALS:
	void currentDocChanged();

public Q_SLOTS:

	void openDoc(shared_ptr<RBX::Instance> script, int lineNumber);
	
    void onActivateWindow();

private:

	RobloxDocManager();
	virtual ~RobloxDocManager();

    void activateDoc(int index);
    RBX::BaldPtr<RobloxMainWindow>          m_pMainWindow;
	tKeyDocMap			                    m_KeyDocMap;
	RBX::BaldPtr<IRobloxDoc>                m_pCurrentDoc;
	RBX::BaldPtr<RobloxIDEDoc>              m_pPlayDoc;
	RBX::BaldPtr<RobloxObjectBrowserDoc>    m_pObjectBrowser;
	RBX::BaldPtr<RobloxWebDoc>              m_pWebDoc;
	tScriptDocs				                m_OpenScriptDocs;
    RBX::BaldPtr<DocTabManager>             m_DocTabManager;
    RBX::BaldPtr<DocDockManager>            m_DocDockManager;
    tPlayModifiedScriptList                 m_PlayModifiedScriptList;
};
