/**
 * ScriptAnalysisWidget.h
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

#include <boost/unordered_map.hpp>
#include <boost/shared_ptr.hpp>

#include <QTreeWidget>

#include "rbx/signal.h"
#include "script/ScriptAnalyzer.h"
#include "v8datamodel/DataModel.h"

#include "LuaSourceBuffer.h"

class QCheckBox;
class QToolButton;
class QFrame;

class ScriptAnalysisTreeWidget: public QTreeWidget
{
	Q_OBJECT
public:
	ScriptAnalysisTreeWidget(QWidget* parent);
	void setDataModel(boost::shared_ptr<RBX::DataModel> dataModel);

	void updateResults(const LuaSourceBuffer& script, const RBX::ScriptAnalyzer::Result& result, bool showErrors, bool showWarnings);
	void reset();

	void scrollTo(const LuaSourceBuffer& script);
	void updateCategoryItem(QTreeWidgetItem* pItem);

	void instanceRemoving(boost::shared_ptr<RBX::Instance> instance);

	QTreeWidgetItem* getCategoryItem(const LuaSourceBuffer& script);
	bool hasHyperLinkText(const QPoint& pos);

	void setFilter(const LuaSourceBuffer& script, bool showErrors, bool showWarnings);
	void showAll(bool showErrors, bool showWarnings);

	QTreeWidgetItem* itemFromIndex(const QModelIndex &index) const
	{	return QTreeWidget::itemFromIndex(index); }

	using QTreeWidget::indexFromItem;

	void categoryItemBecomingLinked(QTreeWidgetItem*);
	void removeGameScriptAssets();

	rbx::signal<void(boost::shared_ptr<RBX::Instance>)> embeddedSourceRemoved;

Q_SIGNALS:
	void resultsUpdated();

private Q_SLOTS:
	void onItemActivated(QTreeWidgetItem* pItem);

private:
	/*override*/ void mousePressEvent(QMouseEvent *evt);
	/*override*/ void mouseMoveEvent(QMouseEvent * event);

	QTreeWidgetItem* getOrCreateCategoryItem(const LuaSourceBuffer& script);
	bool removeCategoryItem(const LuaSourceBuffer& script);

	QTreeWidgetItem* getOrCreateMessageItem(QTreeWidgetItem* categoryItem, int childPos);

    LuaSourceBuffer getScript(QTreeWidgetItem* item);
	void onScriptParentChanged(boost::shared_ptr<RBX::Instance> instance, boost::shared_ptr<RBX::Instance> newParent);

	Q_INVOKABLE void updateItemName(const QModelIndex& index);

	typedef boost::unordered_map<LuaSourceBuffer, QTreeWidgetItem*>          InstanceMap;

	InstanceMap                              m_InstanceMap;
	boost::shared_ptr<RBX::DataModel>        m_dataModel;
};

class LinkedSourceInstance : public boost::enable_shared_from_this<LinkedSourceInstance>
{
public:
	LinkedSourceInstance(boost::shared_ptr<RBX::LuaSourceContainer> instance);

	boost::shared_ptr<RBX::LuaSourceContainer> getInstance() {  return m_LuaSourceContainer; }
	rbx::signal<void(boost::shared_ptr<LinkedSourceInstance>)>   linkedSourceRemoved;

private:
	void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor);

	boost::shared_ptr<RBX::LuaSourceContainer>    m_LuaSourceContainer;
	rbx::signals::scoped_connection               m_cPropertyChangedConnection;
};

class ScriptAnalysisWidget: public QWidget
{
	Q_OBJECT
public:
	ScriptAnalysisWidget(QWidget* parent);

	void updateResults(const LuaSourceBuffer& script, const RBX::ScriptAnalyzer::Result& result);
    void setDataModel(boost::shared_ptr<RBX::DataModel> dataModel);

	void setCurrentScript(const LuaSourceBuffer& script);

private Q_SLOTS:
	void onResultsUpdated();
	void onToolButtonClicked();
	void onCheckBoxClicked();
	void onCurrentDocChanged();

	void onNamedAssetsLoaded(int gameId);
	void onNamedAssetModified(int gameId, const QString& assetName);

private:
	void updateTreeWidget();
	void updateButtonsTextAndState(int currentErrors = 0, int totalErrors = 0, int currentWarnings = 0, int totalWarnings = 0);
    void updateResultsInstance(const shared_ptr<RBX::Instance>& instance);

	void onInstanceRemoving(boost::shared_ptr<RBX::Instance> instance);
	void onLinkedSourceRemoved(boost::shared_ptr<LinkedSourceInstance> linkedSourceInstance);

	void updateResultsGameScriptAssets();		
	void requestAnalyzeScriptAssetSource(const QString& scriptAssetName);
	Q_INVOKABLE void analyzeScriptAssetSource(const QString& scriptAssetName, RBX::HttpFuture future);
	
	void updateResultsScriptAsset(const QString& scriptAssetName, const RBX::ScriptAnalyzer::Result& scriptResults);
	
	Q_INVOKABLE void updateResultsInternal(shared_ptr<RBX::Instance> script, const RBX::ScriptAnalyzer::Result& result);
	Q_INVOKABLE void analyzeAndUpdateResults(shared_ptr<RBX::Instance> script);

	typedef std::set<boost::shared_ptr<LinkedSourceInstance> > LinkedSourceInstanceCollection;
	LinkedSourceInstanceCollection    m_LinkedSources;

	boost::shared_ptr<RBX::DataModel> m_dataModel;
    rbx::signals::scoped_connection   m_dataModelDescendantAdded;
	rbx::signals::scoped_connection   m_dataModelDescendantRemoving;
	rbx::signals::scoped_connection   m_treeWidgetEmbeddedSourceRemoved;

	LuaSourceBuffer                  m_currentScriptInstance;
	QCheckBox*                       m_pDisplayCurrentScript;
	QToolButton*                     m_pErrorsToolButton;
	QToolButton*                     m_pWarningsToolButton;
	QFrame*                          m_pVertSeparator;
	ScriptAnalysisTreeWidget*        m_pTreeWidget;

	int                              m_currentGameId;
	bool                             m_bDisplayCurrentScriptState;
};