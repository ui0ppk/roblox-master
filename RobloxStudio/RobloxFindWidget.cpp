/**
 * FindWidget.cpp
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"

#include <Qt>
#include <QWidget>
#include <QObject>
#include <QResource>
#include <QtCore>
#include <QDesktopWidget>
#include <QTextDocument>

#include "V8DataModel/PlayerGui.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/DataModelJob.h"
#include "V8Tree/Instance.h"
#include "script/CoreScript.h"
#include "Script/Script.h"

#include "RobloxFindWidget.h"
#include "QtUtilities.h"
#include "UpdateUIManager.h"
#include "RobloxMainWindow.h"
#include "RobloxDocManager.h"
#include "RobloxGameExplorer.h"
#include "RobloxIDEDoc.h"
#include "RobloxScriptDoc.h"
#include "IRobloxDoc.h"
#include "ScriptTextEditor.h"

RobloxFindWidget::RobloxFindWidget(QWidget *pParent)
: QWidget(pParent)
, m_pListView(NULL)
, m_pFindListModel(NULL)
, m_searchedScripts(0)
, m_matchingScripts(0)
, m_matchesFound(0)
, m_totalAttempted(0)
, m_stopPopulating(false)
{
	initWidget();
}

RobloxFindWidget::~RobloxFindWidget()
{

}

RobloxFindWidget& RobloxFindWidget::singleton()
{
	static RobloxFindWidget* findWidget = new RobloxFindWidget();
	return *findWidget;
}

void RobloxFindWidget::initWidget()
{
	m_pFindInAllScriptsButton = new QToolButton(this);
	m_pFindInAllScriptsButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/find.png"));
	
	m_pFindInAllScriptsButton->setToolTip("Find in All Scripts");
	m_pFindInAllScriptsButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pGotoLocationButton = new QToolButton(this);
	m_pGotoLocationButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/undo_square_16.png"));
	m_pGotoLocationButton->setToolTip("Goto");
	m_pGotoLocationButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pPreviousButton = new QToolButton(this);
	m_pPreviousButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/arrowleft_green_16.png"));
	m_pPreviousButton->setToolTip("Previous");
	m_pPreviousButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pNextButton = new QToolButton(this);
	m_pNextButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/arrowright_green_16.png"));
	m_pNextButton->setToolTip("Next");
	m_pNextButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pClearButton = new QToolButton(this);
	m_pClearButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/delete_x_16_h.png"));
	m_pClearButton->setToolTip("Clear");
	m_pClearButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pStopButton = new QToolButton(this);
	m_pStopButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/stop_16.png"));
	m_pStopButton->setToolTip("Stop");
	m_pStopButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	m_pStopButton->setDisabled(true);
	
	QSpacerItem *pHorizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	QFrame *seperator1 = new QFrame(this);
	seperator1->setFrameShape(QFrame::VLine);
	seperator1->setFrameShadow(QFrame::Sunken);

	QFrame *seperator2 = new QFrame(this);
	seperator2->setFrameShape(QFrame::VLine);
	seperator2->setFrameShadow(QFrame::Sunken);

	QFrame *seperator3 = new QFrame(this);
	seperator3->setFrameShape(QFrame::VLine);
	seperator3->setFrameShadow(QFrame::Sunken);

	QFrame *seperator4 = new QFrame(this);
	seperator4->setFrameShape(QFrame::VLine);
	seperator4->setFrameShadow(QFrame::Sunken);
	
	QHBoxLayout *buttonLayout = new QHBoxLayout();
	buttonLayout->setSpacing(4);
	buttonLayout->addSpacing(4);
	buttonLayout->addWidget(m_pFindInAllScriptsButton);
	buttonLayout->addWidget(seperator1);
	buttonLayout->addWidget(m_pGotoLocationButton);
	buttonLayout->addWidget(seperator2);
	buttonLayout->addWidget(m_pPreviousButton);
	buttonLayout->addWidget(m_pNextButton);
	buttonLayout->addWidget(seperator3);
	buttonLayout->addWidget(m_pClearButton);
	buttonLayout->addWidget(seperator4);
	buttonLayout->addWidget(m_pStopButton);
	buttonLayout->addItem(pHorizontalSpacer);

	m_pListView = new QListView(this);

	m_pFindListModel = new FindListModel();

	m_pListView->setModel(m_pFindListModel);

	QGridLayout *mainLayout = new QGridLayout(this);
	mainLayout->setVerticalSpacing(4);
	mainLayout->setContentsMargins(0, 0, 0, 0);
	mainLayout->addLayout(buttonLayout, 0, 0);
	mainLayout->addWidget(m_pListView, 1, 0);
	setLayout(mainLayout);

	connect(m_pListView, SIGNAL(clicked(QModelIndex)), this, SLOT(openItem(QModelIndex)));

	connect(m_pFindInAllScriptsButton, SIGNAL(clicked()), &UpdateUIManager::Instance().getMainWindow(), SLOT(showFindAllDialog()));
	connect(m_pGotoLocationButton, SIGNAL(clicked()), this, SLOT(openCurrentItem()));
	connect(m_pPreviousButton, SIGNAL(clicked()), this, SLOT(previousItem()));
	connect(m_pNextButton, SIGNAL(clicked()), this, SLOT(nextItem()));
	connect(m_pClearButton, SIGNAL(clicked()), this, SLOT(clearItems()));
	connect(m_pStopButton, SIGNAL(clicked()), this, SLOT(stopPopulatingItems()));
}

void RobloxFindWidget::findAll(const QString& findStr, bool allScripts, eFindFlags flags)
{
	if (!UpdateUIManager::Instance().getMainWindow().viewFindResultsWindowAction->isChecked())
	{
		UpdateUIManager::Instance().getMainWindow().viewFindResultsWindowAction->trigger();
		show();
	}

	resetItems();
	FindReplaceProvider::instance().getFindAllDialog()->setFindAllRunning(true);

	addItem(FindData(QString("Find all \"%1\", %2").arg(
		findStr, 
		allScripts ? "All Scripts " : "Current Script")));
	
	if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
		if (RBX::DataModel* dm = ideDoc->getDataModel())
		{
			dm->submitTask(boost::bind(&RobloxFindWidget::populateWidget, this, shared_from(dm), findStr, flags, allScripts), RBX::DataModelJob::Read);
			return;
		}

	stopPopulatingItems();
}

void RobloxFindWidget::dataModelClosing()
{
	stopPopulatingItems();
	resetItems();
}

bool RobloxFindWidget::event(QEvent* e)
{
	bool isHandled = QWidget::event(e);

	if ( e->type() == QEvent::KeyPress )
	{
		QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
		QChar key = keyEvent->key();


		if (key == Qt::Key_Up)
		{
			previousItem();
			isHandled = true;
		}
		else if (key == Qt::Key_Down)
		{
			nextItem();
			isHandled = true;
		}
		
	}

	return isHandled;
}

static void appendIfScript(shared_ptr<RBX::Instance> instance, QList<LuaSourceBuffer>& scriptList, RBX::CoreGuiService* coreGuiService)
{
	shared_ptr<RBX::Script> script = RBX::Instance::fastSharedDynamicCast<RBX::Script>(instance);
	shared_ptr<RBX::ModuleScript> moduleScript = RBX::Instance::fastSharedDynamicCast<RBX::ModuleScript>(instance);
	shared_ptr<RBX::LuaSourceContainer> lsc = RBX::Instance::fastSharedDynamicCast<RBX::LuaSourceContainer>(instance);

	if ((script || moduleScript) &&
		(!coreGuiService || !instance->isDescendantOf(coreGuiService)) &&
		lsc->getScriptId().isNull())
		scriptList.append(LuaSourceBuffer::fromInstance(instance));
}

void RobloxFindWidget::populateScriptList(bool allScripts)
{
	m_scriptList.clear();

	if (allScripts)
	{
		RobloxGameExplorer& rge =
			UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
		std::vector<QString> scriptNames;
		rge.getListOfScripts(&scriptNames);

		for (std::vector<QString>::const_iterator itr = scriptNames.begin(); itr != scriptNames.end(); ++itr)
		{
			m_scriptList.append(LuaSourceBuffer::fromContentId(
				RBX::ContentId::fromGameAssetName(itr->toStdString())));
		}

		if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
			if (RBX::DataModel* dm = ideDoc->getDataModel())
				dm->visitDescendants(boost::bind<void>(&appendIfScript, _1, boost::ref(m_scriptList), dm->find<RBX::CoreGuiService>()));
	}
	else
	{
		IRobloxDoc* doc = RobloxDocManager::Instance().getCurrentDoc();
		if (!doc)
			return;

		RobloxScriptDoc* scriptDoc = dynamic_cast<RobloxScriptDoc *>(doc);
		if (!scriptDoc)
		{
			RBX::StandardOut::singleton()->print(RBX::MESSAGE_WARNING, "No script currently available.");
			return;
		}

		m_scriptList.append(scriptDoc->getCurrentScript());
	}
}

void RobloxFindWidget::addItem(FindData item)
{
	m_pFindListModel->addItem(item);
}

void RobloxFindWidget::populateWidget(shared_ptr<RBX::DataModel> dm, const QString& findStr, eFindFlags flags, bool allScripts)
{
	populateScriptList(allScripts);

	if (m_scriptList.size() <= 0)
	{
		stopPopulatingItems();
		return;
	}

	getNextScript(0, dm, findStr, flags);
}

void RobloxFindWidget::getNextScript(int index, shared_ptr<RBX::DataModel> dm, const QString& findStr, eFindFlags flags)
{
	if (m_stopPopulating)
	{
		stopPopulatingItems();
		return;
	}

	LuaSourceBuffer buffer = m_scriptList.at(index);

	try
	{
		QString sourceText = buffer.getScriptText().c_str();
		QString hierarchy = QString::fromStdString(buffer.getFullName());
		dm->submitTask(boost::bind(&RobloxFindWidget::searchScriptChain, this, index, sourceText, dm, findStr, flags, hierarchy), RBX::DataModelJob::None);
	}
	catch (const RBX::base_exception&)
	{
		if (index + 1 < m_scriptList.count())
			dm->submitTask(boost::bind(&RobloxFindWidget::getNextScript, this, index + 1, dm, findStr, flags), RBX::DataModelJob::Read);
		else
			stopPopulatingItems();
	}
}

void RobloxFindWidget::searchScriptChain(int index, const QString& sourceText, shared_ptr<RBX::DataModel> dm, const QString& findStr, eFindFlags flags, const QString& heirarchy)
{
	if (m_stopPopulating)
	{
		stopPopulatingItems();
		return;
	}

	QTextDocument sourceDoc(sourceText);

	m_searchedScripts++;

	QTextDocument::FindFlags findFlags = 0;
	if (flags & MATCH_CASE)
		findFlags |= QTextDocument::FindCaseSensitively;
	if (flags & MATCH_WORD)
		findFlags |= QTextDocument::FindWholeWords;

	bool foundMatch = false;

	QTextCursor cursor;
	cursor = findNextInScript(sourceDoc, findStr, cursor, findFlags, flags & REG_EXP);

	while (!cursor.isNull() && !m_stopPopulating)
	{
		foundMatch = true;
		m_matchesFound++;

		LuaSourceBuffer buffer = m_scriptList.at(index);
		FindData findData(buffer, cursor.blockNumber() + 1, cursor.block().text(), heirarchy);
		findData.setPlaceData(cursor.selectionStart() - cursor.block().position(), cursor.selectionEnd() - cursor.selectionStart());
		addItem(findData);

		if (cursor.atEnd())
			break;

		cursor.movePosition(QTextCursor::NextBlock);
		
		cursor = findNextInScript(sourceDoc, findStr, cursor, findFlags, flags & REG_EXP);
	}

	if (foundMatch)
		m_matchingScripts++;

	if (index + 1 < m_scriptList.count() && !m_stopPopulating)
		dm->submitTask(boost::bind(&RobloxFindWidget::getNextScript, this, index + 1, dm, findStr, flags), RBX::DataModelJob::Read); //Need read lock
	else
		stopPopulatingItems();
}

QTextCursor RobloxFindWidget::findNextInScript(const QTextDocument& doc, const QString& findStr, const QTextCursor& cursor, QTextDocument::FindFlags flags, bool regExp)
{
	if (regExp)
		return doc.find(QRegExp(findStr), cursor, flags);
	else
		return doc.find(findStr, cursor, flags);
}

void RobloxFindWidget::previousItem()
{
	if (m_pFindListModel->rowCount() <= 2)
		return;
	
	int previousPosition = m_pListView->currentIndex().row() - 1;
	if (previousPosition >= m_pFindListModel->rowCount() - 1 || previousPosition <= 0)
		previousPosition = m_pFindListModel->rowCount() - 2;
	
	m_pListView->setCurrentIndex(m_pFindListModel->index(previousPosition));
	openCurrentItem();
}

void RobloxFindWidget::nextItem()
{
	if (m_pFindListModel->rowCount() <= 2)
		return;

	int nextPosition = m_pListView->currentIndex().row() + 1;
	if (nextPosition >= m_pFindListModel->rowCount() - 1 || nextPosition <= 0)
		nextPosition = 1;

	m_pListView->setCurrentIndex(m_pFindListModel->index(nextPosition));
	openCurrentItem();
}

void RobloxFindWidget::clearItems()
{
	m_pFindListModel->clear();
	m_pListView->reset();
	m_pListView->setCurrentIndex(QModelIndex());

	if (QDockWidget* dockWidget = dynamic_cast<QDockWidget *>(parent()))
		dockWidget->setWindowTitle("Find Results");
}

void RobloxFindWidget::resetItems()
{
	m_stopPopulating = false;
	m_pStopButton->setDisabled(false);

	m_matchesFound = 0;
	m_matchingScripts = 0;
	m_searchedScripts = 0;
	m_totalAttempted = 0;

	m_pListView->setCurrentIndex(QModelIndex());
	m_pListView->clearSelection();
	
	m_pFindListModel->clear();

	if (QDockWidget* dockWidget = dynamic_cast<QDockWidget *>(parent()))
		dockWidget->setWindowTitle("Find Results");
}

void RobloxFindWidget::stopPopulatingItems()
{
	if (!m_stopPopulating)
	{
		m_stopPopulating = true;
		
		addItem(FindData(QString("Matching lines: %1    Matching scripts: %2    Total scripts searched: %3").arg(
            QString::number(m_matchesFound),
			QString::number(m_matchingScripts),
			QString::number(m_searchedScripts))));

		if (QDockWidget* dockWidget = dynamic_cast<QDockWidget *>(parent()))
			dockWidget->setWindowTitle(QString("Find Results - %1 Matching lines").arg(QString::number(m_matchesFound)));
	}

	m_pStopButton->setDisabled(true);
	
	m_pFindListModel->updateEntireView();

	FindReplaceProvider::instance().getFindAllDialog()->setFindAllRunning(false);
}

void RobloxFindWidget::openCurrentItem()
{
	openItem(m_pListView->currentIndex());
}

void RobloxFindWidget::openItem(const QModelIndex & index)
{
	m_pFindListModel->openScript(index.row());
	setFocus(Qt::OtherFocusReason);
}

QString RobloxFindWidget::getInstanceHierarchy(shared_ptr<RBX::Instance> instance)
{
	if (!instance)
		return "";

	QString path(instance->getName().c_str());

	const RBX::Instance* pParent = instance->getParent();
	const RBX::Instance* pChild = instance.get();

	while (pParent)
	{
		path.prepend(".");
		if (pParent->getParent())
			path.prepend(pParent->getName().c_str());
		else
			path.prepend("game");
			

		pChild = pParent;
		pParent = pChild->getParent();
	}

	return path;
}

FindListModel::FindListModel()
: m_previousCount(0)
, m_runningUpdater(false)
, m_stopUpdater(false)
, m_loadedIndex(0)
{
	
}

void FindListModel::updateEntireView()
{
	m_loadedIndex = m_previousCount - 1;
	m_stopUpdater = true;
	Q_EMIT(dataChanged(index(0), index(m_loadedIndex)));
}

void FindListModel::updateView()
{
	if (m_loadedIndex >= m_previousCount - 1 || m_stopUpdater)
	{
		m_runningUpdater = false;
		m_stopUpdater = false;
		return;
	}

	m_loadedIndex = qMin(m_loadedIndex + 20, m_previousCount);
	if (rowCount() > 0)
		Q_EMIT(dataChanged(index(m_loadedIndex - 1), index(m_loadedIndex)));

	QTimer::singleShot(100, this, SLOT(updateView()));
}

void FindListModel::addItem(FindData item)
{
	QMutexLocker locker(&m_listMutex);
	m_FindList.append(item);
	m_previousCount = m_FindList.count();
	if (!m_runningUpdater)
	{
		m_runningUpdater = true;
		m_stopUpdater = false;
		QTimer::singleShot(0, this, SLOT(updateView()));
	}
}

void FindListModel::clear()
{
	QMutexLocker locker(&m_listMutex);
	m_FindList.clear();
	m_previousCount = 0;
	m_runningUpdater = false;
	m_stopUpdater = true;
	m_loadedIndex = 0;
}

int FindListModel::rowCount(const QModelIndex& parent) const
{
	return qMin(m_previousCount, m_loadedIndex + 1);
}

QVariant FindListModel::data(const QModelIndex& index, int role) const
{
	int row = index.row();

	if (row >= 0 &&
		row < m_previousCount &&
		role == Qt::DisplayRole)
	{
		QMutexLocker locker(&m_listMutex);
		return m_FindList[row].toString();
	}

	return QVariant(QVariant::Invalid);
}

Qt::ItemFlags FindListModel::flags(const QModelIndex& index) const
{
	QMutexLocker locker(&m_listMutex);
	return m_FindList[index.row()].flags();
}

int FindListModel::openScript(int row)
{
	if (row >= m_previousCount - 1 || row <= 0)
		return -1;
	
	FindData currentRow;
	{
		QMutexLocker locker(&m_listMutex);
		currentRow = m_FindList[row];
	}

	if (currentRow.getLine() == 0)
		return -1;

	LuaSourceBuffer buffer = currentRow.getInstance();

	if (shared_ptr<RBX::LuaSourceContainer> lsc = buffer.toInstance())
	{
		// decline to open script instance results if they are now linked
		if (!lsc->getScriptId().isNull() || !lsc->getParent())
		{
			return -1;
		}
	}

	RobloxScriptDoc* scriptDoc = RobloxDocManager::Instance().openDoc(buffer);
	if (!scriptDoc)
		return -1;

	RBX::BaldPtr<ScriptTextEditor> editor = scriptDoc->getTextEditor();
	if (!editor)
		return -1;

	int pos = currentRow.getPosition() + editor->document()->findBlockByNumber(currentRow.getLine() - 1).position();
	int length = currentRow.getLength();

	editor->selectText(pos, pos + length);

	return row;
}

Qt::ItemFlags FindData::flags() const
{
	if (m_line <= 0)
		return 0;

	return Qt::ItemIsSelectable | Qt::ItemIsEnabled;
}

QString FindData::toString() const
{
	if (m_line <= 0)
		return m_lineText;

	return QString("  %1(%2): %3").arg(m_hierarchy, QString::number(m_line), m_lineText);
}
