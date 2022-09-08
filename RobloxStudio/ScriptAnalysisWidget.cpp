/**
 * ScriptAnalysisWidget.cpp
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "ScriptAnalysisWidget.h"

#include <QHeaderView>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QSpacerItem>
#include <QTextBlock>
#include <QStyledItemDelegate>
#include <QStyleOptionViewItemV4>
#include <QTextDocument>
#include <QPainter>
#include <QDesktopServices>
#include <QToolButton>
#include <QFrame>

#include "RobloxDocManager.h"
#include "ScriptTextEditor.h"
#include "UpdateUIManager.h"
#include "RobloxMainWindow.h"
#include "RobloxScriptDoc.h"
#include "RobloxSettings.h"
#include "RobloxGameExplorer.h"

#include "v8datamodel/PlayerGui.h"

const char* kSAShowErrorsKey    = "rbxSAShowErrors";
const char* kSAShowWarningsKey  = "rbxSAShowWarnings";

const int   kSADocLeftMargin    = 4;
const int   kSADocTopMargin     = 4;

FASTFLAGVARIABLE(StudioUpdateSAResultsInUIThread, false)

//--------------------------------------------------------------------------------------------
// ScriptMessageItem
//--------------------------------------------------------------------------------------------
class ScriptMessageItem: public QTreeWidgetItem
{
public:
	ScriptMessageItem()
	: m_bIsError(false)
    , m_warningCode(0)
	{
	}

	void update(const RBX::ScriptAnalyzer::Warning& warning)
	{
		m_location = warning.location;
        m_warningCode = warning.code;
		m_bIsError = false;

        QString text = Qt::escape(warning.text.c_str());
		
		setText(0, QString("<a href=\"http://\">%1</a>: (%2,%3) %4").arg(getWarningCodeString(m_warningCode)).arg(m_location.begin.line+1).arg(m_location.begin.column+1).arg(text));
	}

	void update(const RBX::ScriptAnalyzer::Error& error)
	{
		m_location = error.location;
        m_warningCode = 0;
		m_bIsError = true;

        QString text = Qt::escape(error.text.c_str());

		setText(0, QString("<font color=\"red\">Error: (%1,%2) %3</font>").arg(m_location.begin.line+1).arg(m_location.begin.column+1).arg(text));
	}

	bool hasHyperLinkText(const QPoint& pos)
	{
		if (!isError())
		{
			ScriptAnalysisTreeWidget* pTreeWidget = dynamic_cast<ScriptAnalysisTreeWidget*>(treeWidget());
			if (pTreeWidget)
			{
				QRect rect = pTreeWidget->visualItemRect(this);
				QFontMetrics fm(pTreeWidget->font());
				rect.setRight(rect.left() + fm.width(getWarningCodeString(0)) + kSADocLeftMargin);
				return rect.contains(pos);
			}
		}

		return false;
	}

	QVariant data(int column, int role) const
	{
		if (role == Qt::ToolTipRole)
		{
			QString colText(text(0));
			if (!colText.isEmpty())
				return colText.remove(QRegExp("<[^>]*>"));
		}
		return QTreeWidgetItem::data(column, role);
	}

	void launchURL()
	{
        if (!m_bIsError && m_warningCode > 0)
        {
			UpdateUIManager::Instance().setDockVisibility(eDW_CONTEXTUAL_HELP, true);
            QMetaObject::invokeMethod(&RobloxContextualHelpService::singleton(), "onHelpTopicChanged", Q_ARG(QString, QString("Script_Analysis#%1").arg(getWarningCodeString(m_warningCode))));
        }
	}

	const RBX::ScriptAnalyzer::Location& getLocation() { return m_location; }
	bool isError() const { return m_bIsError; }

    static QString getWarningCodeString(int code) { return QString().sprintf("W%03d", code); }

private:
	RBX::ScriptAnalyzer::Location     m_location;
    int                               m_warningCode;
	bool                              m_bIsError;
};

//--------------------------------------------------------------------------------------------
// ScriptCategoryItem
//--------------------------------------------------------------------------------------------
class ScriptCategoryItem: public QTreeWidgetItem
{
public:
	ScriptCategoryItem(const LuaSourceBuffer& script)
	{
		QFont boldFont(font(0));
		boldFont.setBold(true);
		setFont(0, boldFont);
		setText(0, QString("<b>%1</b>").arg(script.getFullName().c_str()));

		if (script.toInstance())
			m_cPropertyChangedConnection = script.toInstance()->propertyChangedSignal.connect(boost::bind(&ScriptCategoryItem::onPropertyChanged, this, _1));
	}

	~ScriptCategoryItem()
	{
		m_cPropertyChangedConnection.disconnect();
	}

	void updateItemVisibility(bool showErrors, bool showWarnings)
	{
		if ((!showErrors && !showWarnings) || !childCount())
		{
			setHidden(true);
			return;
		}

		int numChild = childCount(), currentChild = 0, numChildModified = 0;
		while (currentChild < numChild)
		{
			ScriptMessageItem* pMessageItem = dynamic_cast<ScriptMessageItem*>(child(currentChild++));
			if (pMessageItem)
			{
				bool isVisbile = pMessageItem->isError() ? showErrors : showWarnings;
				pMessageItem->setHidden(!isVisbile);
				if (isVisbile)
					++numChildModified;
			}
		}

		setHidden(!numChildModified);
	}

	void populateErrorsAndWarningsCount(int& currentErrors, int& totalErrors, int& currentWarnings, int& totalWarnings)
	{
		int numChild = childCount(), currentChild = 0;
		while (currentChild < numChild)
		{
			ScriptMessageItem* pMessageItem = dynamic_cast<ScriptMessageItem*>(child(currentChild++));
			if (pMessageItem)
			{
				if (pMessageItem->isError())
				{
					++totalErrors;
					if (!isHidden() && !pMessageItem->isHidden())
						currentErrors++;
				}
				else
				{
					++totalWarnings;
					if (!isHidden() && !pMessageItem->isHidden())
						currentWarnings++;
				}
			}
		}
	}

	QVariant data(int column, int role) const
	{
		if (role == Qt::ToolTipRole)
		{
			QString colText(text(0));
			if (!colText.isEmpty())
				return colText.remove(QRegExp("<[^>]*>"));
		}
		return QTreeWidgetItem::data(column, role);
	}

private:
	void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
	{	
		if (*pDescriptor==RBX::Instance::desc_Name)
		{
			ScriptAnalysisTreeWidget* pTreeWidget = dynamic_cast<ScriptAnalysisTreeWidget*>(treeWidget());
			if (pTreeWidget)
			{
				if (FFlag::StudioUpdateSAResultsInUIThread)
					QMetaObject::invokeMethod(pTreeWidget, "updateItemName", Qt::QueuedConnection, Q_ARG(QModelIndex, pTreeWidget->indexFromItem(this)));
				else
					pTreeWidget->updateCategoryItem(this);
			}
		}
		if (pDescriptor->name == "LinkedSource")
		{
			ScriptAnalysisTreeWidget* pTreeWidget = dynamic_cast<ScriptAnalysisTreeWidget*>(treeWidget());
			if (pTreeWidget)
				pTreeWidget->categoryItemBecomingLinked(this);
		}
	}

	rbx::signals::scoped_connection   m_cPropertyChangedConnection;
};

//--------------------------------------------------------------------------------------------
// ScriptMessageItemDelegate
//--------------------------------------------------------------------------------------------
class ScriptMessageItemDelegate: public QStyledItemDelegate
{
	ScriptAnalysisTreeWidget* m_pTreeWidget;
	QTextDocument*            m_pRenderingDoc;
	mutable int               m_docHeight;
public:
	ScriptMessageItemDelegate(ScriptAnalysisTreeWidget* parent)
	: QStyledItemDelegate(parent)
	, m_pTreeWidget(parent)
	, m_docHeight(-1)
	{
		m_pRenderingDoc = new QTextDocument(parent);
		m_pRenderingDoc->setUndoRedoEnabled(false);
        m_pRenderingDoc->setDocumentMargin(0);
	}

	bool editorEvent(QEvent *evt, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index)
	{
		if (evt->type() == QEvent::MouseButtonRelease)
		{
			ScriptMessageItem* pItem = dynamic_cast<ScriptMessageItem*>(m_pTreeWidget->itemFromIndex(index));
			if (pItem)
			{
				// if the dock widget is already visible or user has clicked on hyperlink, update URL
				if (UpdateUIManager::Instance().getDockWidget(eDW_CONTEXTUAL_HELP)->isVisible() ||
					pItem->hasHyperLinkText(m_pTreeWidget->mapFromGlobal(QCursor::pos())))
				{
					pItem->launchURL();
					return true;
				}
			}
		}
		return false;
	}

	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QStyleOptionViewItemV4 optionV4 = option;
		initStyleOption(&optionV4, index);

		if (m_docHeight < 0)
		{
			// NOTE: Using QStyledItemDelegate::sizeHint we can get wrong results on Mac in Ribbon mode
			// so using the default height computation - (fontMetrics.height + margin) will be the height
			m_docHeight = optionV4.fontMetrics.height() + kSADocTopMargin;
#ifdef Q_WS_WIN
			// on windows in ribbon style the margin is smaller
			if (UpdateUIManager::Instance().getMainWindow().isRibbonStyle())
				m_docHeight = m_docHeight - kSADocTopMargin/2;
#endif
		}

		m_pRenderingDoc->setHtml(optionV4.text);
		m_pRenderingDoc->setTextWidth(optionV4.rect.width());
        
		return QSize(m_pRenderingDoc->idealWidth(), m_docHeight);
	}

	void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QStyleOptionViewItemV4 optionV4 = option;
		initStyleOption(&optionV4, index);

		m_pRenderingDoc->setDefaultFont(option.font);
		QString htmlText(optionV4.text);
		if (optionV4.state & QStyle::State_Selected)
		{
			ScriptMessageItem* pItem = dynamic_cast<ScriptMessageItem*>(m_pTreeWidget->itemFromIndex(index));
			if (pItem && pItem->isError())
			{
				// blue with red is not readable, change the font color
				htmlText = QString("<font color=\"yellow\">%1</font>").arg(optionV4.text.remove(QRegExp("<[^>]*>")));
			}
			else
			{
				htmlText = QString("<font color=\"white\">%1</font>").arg(optionV4.text);
			}
		}
		m_pRenderingDoc->setHtml(htmlText);

		optionV4.text = "";
		m_pTreeWidget->style()->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter, m_pTreeWidget);

		painter->save();
		QRect textRect = m_pTreeWidget->style()->subElementRect(QStyle::SE_ItemViewItemText, &optionV4);
		painter->setFont(optionV4.font);
		painter->translate(textRect.topLeft() + QPoint(kSADocLeftMargin, 1));
		painter->setClipRect(textRect.translated(-textRect.topLeft()));
		m_pRenderingDoc->documentLayout()->draw(painter, QAbstractTextDocumentLayout::PaintContext());
		painter->restore();
	}
};

//--------------------------------------------------------------------------------------------
// ScriptAnalysisTreeWidget
//--------------------------------------------------------------------------------------------
ScriptAnalysisTreeWidget::ScriptAnalysisTreeWidget(QWidget* parent)
: QTreeWidget(parent)
{
	setUniformRowHeights(true);
	setAlternatingRowColors(true);
	setRootIsDecorated(true);
	setColumnCount(1);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setHeaderHidden(true);
	setMouseTracking(true);

	header()->setStretchLastSection(true);
    header()->setResizeMode(0,QHeaderView::ResizeToContents);

	setItemDelegate(new ScriptMessageItemDelegate(this));

	connect(this, SIGNAL(itemActivated(QTreeWidgetItem*, int)), this, SLOT(onItemActivated(QTreeWidgetItem*)));
}

void ScriptAnalysisTreeWidget::setDataModel(boost::shared_ptr<RBX::DataModel> dataModel)
{
	RBXASSERT(m_dataModel == NULL);
	m_dataModel = dataModel;
}

void ScriptAnalysisTreeWidget::updateResults(const LuaSourceBuffer& script, const RBX::ScriptAnalyzer::Result& result, bool showErrors, bool showWarnings)
{
	if (script.empty())
		return;

	setUpdatesEnabled(false);

	if (!result.error && !result.warnings.size())
	{
		removeCategoryItem(script);
	}
	else
	{
		int currentChild = 0;
		ScriptMessageItem* messageItem = NULL;
		ScriptCategoryItem* categoryItem = static_cast<ScriptCategoryItem*>(getOrCreateCategoryItem(script));

		if (result.error)
		{
			messageItem = static_cast<ScriptMessageItem*>(getOrCreateMessageItem(categoryItem, currentChild++));
			messageItem->update(result.error.get());
			if (!messageItem->parent())
				categoryItem->addChild(messageItem);
			messageItem->setHidden(!showErrors);
		}

		for (size_t i = 0; i < result.warnings.size(); ++i)
		{
			messageItem = static_cast<ScriptMessageItem*>(getOrCreateMessageItem(categoryItem, currentChild++));
			messageItem->update(result.warnings[i]);
			if (!messageItem->parent())
				categoryItem->addChild(messageItem);
			messageItem->setHidden(!showWarnings);
		}

		while (currentChild < categoryItem->childCount())
		{
			QTreeWidgetItem* pItem = categoryItem->child(currentChild);
			if (pItem)
				delete pItem;
		}

		categoryItem->setExpanded(true);
		categoryItem->updateItemVisibility(showErrors, showWarnings);
	}

	setUpdatesEnabled(true);
	Q_EMIT resultsUpdated();
}

void ScriptAnalysisTreeWidget::reset()
{
	m_dataModel = boost::shared_ptr<RBX::DataModel>();
	m_InstanceMap.clear();

	while (QTreeWidgetItem* pItem = topLevelItem(0))
		delete pItem;
}

void ScriptAnalysisTreeWidget::scrollTo(const LuaSourceBuffer& script)
{
	QTreeWidgetItem* pItem = getCategoryItem(script);
	if (pItem)
		scrollToItem(pItem, QAbstractItemView::PositionAtTop);
}

void ScriptAnalysisTreeWidget::updateCategoryItem(QTreeWidgetItem* pItem)
{
	LuaSourceBuffer script = getScript(pItem);
	if (!script.empty())
	{
		pItem->setText(0, QString("<b>%1</b>").arg(script.getFullName().c_str()));
		Q_EMIT resultsUpdated();
	}
}

void ScriptAnalysisTreeWidget::updateItemName(const QModelIndex& index)
{
	QTreeWidgetItem* pItem = itemFromIndex(index);
	if (pItem)
	{
		LuaSourceBuffer script = getScript(pItem);
		if (!script.empty())
		{
			RBX::DataModel::LegacyLock lock(m_dataModel, RBX::DataModelJob::Read);
			pItem->setText(0, QString("<b>%1</b>").arg(script.getFullName().c_str()));

			Q_EMIT resultsUpdated();
		}
	}
}

void ScriptAnalysisTreeWidget::onItemActivated(QTreeWidgetItem* pItem)
{
	if (hasHyperLinkText(QCursor::pos()))
		return;

	ScriptMessageItem* pBaseItem = dynamic_cast<ScriptMessageItem*>(pItem);
	if(pBaseItem)
	{
		LuaSourceBuffer script = getScript(pItem->parent());
		if (!script.empty())
		{
			IRobloxDoc* pDocument = RobloxDocManager::Instance().openDoc(script);
			if (pDocument)
			{
				ScriptTextEditor* pTextEdit = dynamic_cast<ScriptTextEditor*>(pDocument->getViewer());
				if (pTextEdit)
					pTextEdit->selectText(pBaseItem->getLocation());
			}
		}

		scrollToItem(pBaseItem);
	}
}

QTreeWidgetItem* ScriptAnalysisTreeWidget::getCategoryItem(const LuaSourceBuffer& script)
{
	InstanceMap::iterator iter = m_InstanceMap.find(script);
	if (iter != m_InstanceMap.end())
		return iter->second;
	return NULL;
}

bool ScriptAnalysisTreeWidget::hasHyperLinkText(const QPoint& pos)
{
	QPoint actualPos(mapFromGlobal(pos));
	ScriptMessageItem* pItem = dynamic_cast<ScriptMessageItem*>(itemAt(actualPos));
	if (pItem)
		return pItem->hasHyperLinkText(actualPos);
	return false;
}

void ScriptAnalysisTreeWidget::setFilter(const LuaSourceBuffer& script, bool showErrors, bool showWarnings)
{
	int currentChild = 0;
	int numChild = topLevelItemCount();
	QTreeWidgetItem* pCategoryItemToSearch = getCategoryItem(script), *pCurrentCategoryItem = NULL;
	while (currentChild < numChild)
	{
		pCurrentCategoryItem = topLevelItem(currentChild++);
		if (pCurrentCategoryItem != pCategoryItemToSearch)
		{
			pCurrentCategoryItem->setHidden(true);
		}
		else
		{
			ScriptCategoryItem* pItem = dynamic_cast<ScriptCategoryItem*>(pCurrentCategoryItem);
			if (pItem)
				pItem->updateItemVisibility(showErrors, showWarnings);
		}
	}
}

void ScriptAnalysisTreeWidget::showAll(bool showErrors, bool showWarnings)
{
	int currentChild = 0;
	int numChild = topLevelItemCount();
	while (currentChild < numChild)
	{
		ScriptCategoryItem* pItem = dynamic_cast<ScriptCategoryItem*>(topLevelItem(currentChild++));
		if (pItem)
			pItem->updateItemVisibility(showErrors, showWarnings);
	}
}

void ScriptAnalysisTreeWidget::mousePressEvent(QMouseEvent *evt)
{
	if (!itemAt(evt->pos()))
		setCurrentItem(NULL);
	else
		QTreeWidget::mousePressEvent(evt);
}

void ScriptAnalysisTreeWidget::mouseMoveEvent(QMouseEvent * evt)
{
	Qt::CursorShape prevCursor = cursor().shape();
	hasHyperLinkText(evt->globalPos()) ? setCursor(Qt::PointingHandCursor) : unsetCursor();
	if (prevCursor != cursor().shape())
		viewport()->update();
	QTreeWidget::mouseMoveEvent(evt);
}

QTreeWidgetItem* ScriptAnalysisTreeWidget::getOrCreateCategoryItem(const LuaSourceBuffer& script)
{
	QTreeWidgetItem* pCategoryItem = getCategoryItem(script);
	if (pCategoryItem)
		return pCategoryItem;

	int indexToInsert = 0;
	int numChild = topLevelItemCount();
	QTreeWidgetItem *pCurrentItem = NULL;

	while (indexToInsert < numChild)
	{		
		pCurrentItem = topLevelItem(indexToInsert);		
		if (script.getFullName() < pCurrentItem->text(0).toStdString())
				break;
		++indexToInsert;
	}

	pCategoryItem = new ScriptCategoryItem(script);
	invisibleRootItem()->insertChild(indexToInsert, pCategoryItem);

	m_InstanceMap[script] = pCategoryItem;

	return pCategoryItem;
}

QTreeWidgetItem* ScriptAnalysisTreeWidget::getOrCreateMessageItem(QTreeWidgetItem* categoryItem, int childPos)
{
	int numChild = categoryItem->childCount();
	if (childPos < numChild)
		return categoryItem->child(childPos);
	return new ScriptMessageItem;
}

bool ScriptAnalysisTreeWidget::removeCategoryItem(const LuaSourceBuffer& script)
{
	QTreeWidgetItem* categoryItem = getCategoryItem(script);
	if (!categoryItem)
		return false;

	m_InstanceMap.erase(script);

	delete categoryItem;
	return true;
}

LuaSourceBuffer ScriptAnalysisTreeWidget::getScript(QTreeWidgetItem* item)
{
	for (InstanceMap::iterator iter = m_InstanceMap.begin(); iter != m_InstanceMap.end(); ++iter)
	{
		if (iter->second == item)
			return iter->first;
	}
	return LuaSourceBuffer();
}

void ScriptAnalysisTreeWidget::instanceRemoving(boost::shared_ptr<RBX::Instance> script)
{
	if (removeCategoryItem(LuaSourceBuffer::fromInstance(script)))
		Q_EMIT resultsUpdated();
}

void ScriptAnalysisTreeWidget::categoryItemBecomingLinked(QTreeWidgetItem* item)
{
	LuaSourceBuffer script = getScript(item);

	embeddedSourceRemoved(script.toInstance());

	if (FFlag::StudioUpdateSAResultsInUIThread)
	{
		// this will make sure we are removing the category item as the result is empty
		RBX::ScriptAnalyzer::Result result;
		QMetaObject::invokeMethod(qobject_cast<ScriptAnalysisWidget*>(parent()), "updateResultsInternal", Qt::AutoConnection,
			Q_ARG(shared_ptr<RBX::Instance>, script.toInstance()), Q_ARG(RBX::ScriptAnalyzer::Result, result));
	}
	else
	{
		if (removeCategoryItem(script))
			Q_EMIT resultsUpdated();
	}
}

void ScriptAnalysisTreeWidget::removeGameScriptAssets()
{
	bool enabled = updatesEnabled(), updateRequired = false;
	setUpdatesEnabled(false);
	InstanceMap::iterator iter = m_InstanceMap.begin();
	while (iter != m_InstanceMap.end())
	{
		LuaSourceBuffer script((iter++)->first);		
		if (script.isNamedAsset())
			updateRequired |= removeCategoryItem(script);
	}
	if (updateRequired)
		Q_EMIT resultsUpdated();
	setUpdatesEnabled(enabled);
}

//--------------------------------------------------------------------------------------------
// LinkedSourceInstance
//--------------------------------------------------------------------------------------------
LinkedSourceInstance::LinkedSourceInstance(boost::shared_ptr<RBX::LuaSourceContainer> instance)
: m_LuaSourceContainer(instance)
{
	m_cPropertyChangedConnection = m_LuaSourceContainer->propertyChangedSignal.connect(boost::bind(&LinkedSourceInstance::onPropertyChanged, this, _1));
}

void LinkedSourceInstance::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
{
	// if we've removed linked source from the script, request results to be updated
	if ((pDescriptor->name == "LinkedSource") && m_LuaSourceContainer->getScriptId().isNull())
		linkedSourceRemoved(shared_from_this());
}

//--------------------------------------------------------------------------------------------
// ScriptAnalysisWidget
//--------------------------------------------------------------------------------------------
ScriptAnalysisWidget::ScriptAnalysisWidget(QWidget* parent)
: QWidget(parent)
, m_currentGameId(-1)
, m_bDisplayCurrentScriptState(true)
{
	// register HttpFuture so it can used in QInvokeMethod
	qRegisterMetaType<RBX::HttpFuture>("RBX::HttpFuture");

	m_pTreeWidget = new ScriptAnalysisTreeWidget(this);

	m_pVertSeparator = new QFrame(this);
	m_pVertSeparator->setFrameShape(QFrame::VLine);
    m_pVertSeparator->setFrameShadow(QFrame::Sunken);
	m_pVertSeparator->setVisible(false);

	m_pErrorsToolButton = new QToolButton(this);
	m_pErrorsToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	m_pErrorsToolButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/stop_16.png"));
	m_pErrorsToolButton->setCheckable(true);
	m_pErrorsToolButton->setChecked(RobloxSettings().value(kSAShowErrorsKey, true).toBool());

	QFrame *pLine2 = new QFrame(this);
	pLine2->setFrameShape(QFrame::VLine);
    pLine2->setFrameShadow(QFrame::Sunken);

	m_pWarningsToolButton = new QToolButton(this);
	m_pWarningsToolButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	m_pWarningsToolButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/bug_error.png"));
	m_pWarningsToolButton->setCheckable(true);
	m_pWarningsToolButton->setChecked(RobloxSettings().value(kSAShowWarningsKey, true).toBool());

	QSpacerItem *pHorizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

	m_pDisplayCurrentScript = new QCheckBox(this);
	m_pDisplayCurrentScript->setVisible(false);
	m_pDisplayCurrentScript->setText(tr("Display only current script"));

	QHBoxLayout* pComboLabelLayout = new QHBoxLayout;
	pComboLabelLayout->setContentsMargins(2, 2, 2, 2);
	pComboLabelLayout->addWidget(m_pErrorsToolButton);
	pComboLabelLayout->addWidget(pLine2);
	pComboLabelLayout->addWidget(m_pWarningsToolButton);
	pComboLabelLayout->addWidget(m_pVertSeparator);
	pComboLabelLayout->addWidget(m_pDisplayCurrentScript);
	pComboLabelLayout->addItem(pHorizontalSpacer);
	
	QGridLayout *pGridLayout = new QGridLayout(this);
	pGridLayout->setSpacing(4);
	pGridLayout->setContentsMargins(0, 0, 0, 0);
	pGridLayout->addLayout(pComboLabelLayout, 0, 0, 1, 1);
	pGridLayout->addWidget(m_pTreeWidget, 1, 0, 1, 1);

	connect(m_pTreeWidget, SIGNAL(resultsUpdated()), this, SLOT(onResultsUpdated()));
	connect(m_pDisplayCurrentScript, SIGNAL(released()), this, SLOT(onCheckBoxClicked()));
	connect(m_pErrorsToolButton, SIGNAL(released()), this, SLOT(onToolButtonClicked()));
	connect(m_pWarningsToolButton, SIGNAL(released()), this, SLOT(onToolButtonClicked()));
	connect(&RobloxDocManager::Instance(), SIGNAL(currentDocChanged()),
			this,                          SLOT(onCurrentDocChanged()));

	updateButtonsTextAndState();
}

void ScriptAnalysisWidget::updateResults(const LuaSourceBuffer& script, const RBX::ScriptAnalyzer::Result& result)
{
	RBXASSERT(!FFlag::StudioUpdateSAResultsInUIThread || (QThread::currentThread() == qApp->thread()));
	// results must be updated irrespective of simulation mode
    bool visible = (script == m_currentScriptInstance) || m_currentScriptInstance.empty();
    bool showErrors = m_pErrorsToolButton->isChecked(), showWarnings = m_pWarningsToolButton->isChecked();
    m_pTreeWidget->updateResults(script, result, visible ? showErrors : false, visible ? showWarnings : false);
}

void ScriptAnalysisWidget::setDataModel(boost::shared_ptr<RBX::DataModel> dataModel)
{
    m_dataModel = boost::shared_ptr<RBX::DataModel>();
    m_dataModelDescendantAdded.disconnect();
	m_dataModelDescendantRemoving.disconnect();

	RobloxGameExplorer& gameExplorer = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
	m_currentGameId = -1;

	m_treeWidgetEmbeddedSourceRemoved.disconnect();
	m_LinkedSources.clear();

	disconnect(&gameExplorer, SIGNAL(namedAssetsLoaded(int)), this, SLOT(onNamedAssetsLoaded(int)));
	disconnect(&gameExplorer, SIGNAL(namedAssetModified(int, const QString&)), this, SLOT(onNamedAssetModified(int, const QString&)));		

	m_pTreeWidget->reset();
	updateButtonsTextAndState();

    if (dataModel && dataModel->isStudio() && (UpdateUIManager::Instance().getMainWindow().getBuildMode() != BM_BASIC))
    {
		m_dataModel = dataModel;
		m_pTreeWidget->setDataModel(dataModel);

        m_dataModelDescendantAdded = dataModel->getOrCreateDescendantAddedSignal()->connect(boost::bind(&ScriptAnalysisWidget::updateResultsInstance, this, _1));
		
		m_dataModelDescendantRemoving = dataModel->getOrCreateDescendantRemovingSignal()->connect(boost::bind(&ScriptAnalysisWidget::onInstanceRemoving, this, _1));

		m_treeWidgetEmbeddedSourceRemoved = m_pTreeWidget->embeddedSourceRemoved.connect(boost::bind(&ScriptAnalysisWidget::updateResultsInstance, this, _1));
		connect(&gameExplorer, SIGNAL(namedAssetsLoaded(int)), this, SLOT(onNamedAssetsLoaded(int)));
		connect(&gameExplorer, SIGNAL(namedAssetModified(int, const QString&)), this, SLOT(onNamedAssetModified(int, const QString&)));

		// analyze instances and update results
        RBX::DataModel::LegacyLock lock(dataModel, RBX::DataModelJob::Write);
        dataModel->visitDescendants(boost::bind(&ScriptAnalysisWidget::updateResultsInstance, this, _1));
    }
}

void ScriptAnalysisWidget::setCurrentScript(const LuaSourceBuffer& script)
{
	if (m_currentScriptInstance == script)
		return;

	m_currentScriptInstance = script;

	m_pDisplayCurrentScript->setVisible(!m_currentScriptInstance.empty());
	m_pVertSeparator->setVisible(!m_currentScriptInstance.empty());
	m_pDisplayCurrentScript->setChecked(!m_currentScriptInstance.empty() ? m_bDisplayCurrentScriptState : false);

	// update results
	updateTreeWidget();
}

void ScriptAnalysisWidget::onInstanceRemoving(boost::shared_ptr<RBX::Instance> instance)
{
	LuaSourceBuffer script = LuaSourceBuffer::fromInstance(instance);
	if (!script.empty())
	{
		LinkedSourceInstanceCollection::iterator iter = m_LinkedSources.begin();
		while (iter != m_LinkedSources.end())
		{
			if ((*iter)->getInstance() == instance)
			{
				m_LinkedSources.erase(iter);
				break;
			}
			++iter;
		}

		if (FFlag::StudioUpdateSAResultsInUIThread)
		{
			// this will make sure we are removing the category item as the result is empty
			RBX::ScriptAnalyzer::Result result;
			QMetaObject::invokeMethod(this, "updateResultsInternal", Qt::AutoConnection,
				Q_ARG(shared_ptr<RBX::Instance>, instance), Q_ARG(RBX::ScriptAnalyzer::Result, result));
		}
		else
		{
			m_pTreeWidget->instanceRemoving(instance);
		}
	}
}

void ScriptAnalysisWidget::onResultsUpdated()
{
	int numChild = m_pTreeWidget->topLevelItemCount(), currentItem = 0;
	int currentErrors = 0, totalErrors = 0, currentWarnings = 0, totalWarnings = 0;
	ScriptCategoryItem *pCategoryItem = NULL;
	while (currentItem < numChild)
	{
		pCategoryItem = dynamic_cast<ScriptCategoryItem*>(m_pTreeWidget->topLevelItem(currentItem++));
		if (pCategoryItem)
			pCategoryItem->populateErrorsAndWarningsCount(currentErrors, totalErrors, currentWarnings, totalWarnings);
	}
	updateButtonsTextAndState(currentErrors, totalErrors, currentWarnings, totalWarnings);
}

void ScriptAnalysisWidget::onToolButtonClicked()
{
	updateTreeWidget();

	// update settings
	RobloxSettings settings;
	if (sender() == m_pWarningsToolButton)
		settings.setValue(kSAShowWarningsKey, m_pWarningsToolButton->isChecked());
	else
		settings.setValue(kSAShowErrorsKey, m_pErrorsToolButton->isChecked());
}

void ScriptAnalysisWidget::onCheckBoxClicked()
{
	m_bDisplayCurrentScriptState = m_pDisplayCurrentScript->isChecked();
	updateTreeWidget(); 
}

void ScriptAnalysisWidget::onCurrentDocChanged()
{
	if (RobloxScriptDoc* rsd = dynamic_cast<RobloxScriptDoc*>(RobloxDocManager::Instance().getCurrentDoc()))
	{
		setCurrentScript(rsd->getCurrentScript());
	}
	else
	{
		setCurrentScript(LuaSourceBuffer());
	}
}

void ScriptAnalysisWidget::updateButtonsTextAndState(int currentErrors, int totalErrors, int currentWarnings, int totalWarnings)
{
	m_pErrorsToolButton->setText(QString("%1 of %2 Errors").arg(currentErrors).arg(totalErrors));
	m_pErrorsToolButton->setEnabled(totalErrors > 0);
	m_pWarningsToolButton->setText(QString("%1 of %2 Warnings").arg(currentWarnings).arg(totalWarnings));
	m_pWarningsToolButton->setEnabled(totalWarnings > 0);
}

void ScriptAnalysisWidget::updateTreeWidget()
{
	m_pTreeWidget->setUpdatesEnabled(false);
	if (!m_pDisplayCurrentScript->isChecked())
	{
		// currentIndex == 1 - All Scripts
		m_pTreeWidget->showAll(m_pErrorsToolButton->isChecked(), m_pWarningsToolButton->isChecked());
	}
	else
	{
		// currrentIndex == 0 - Current Scripts
		m_pTreeWidget->setFilter(m_currentScriptInstance, m_pErrorsToolButton->isChecked(), m_pWarningsToolButton->isChecked());
	}
	m_pTreeWidget->setUpdatesEnabled(true);

	onResultsUpdated();
}

template <typename T> static const T* findFirstAncestorByType(const RBX::Instance* instance)
{
    while (instance)
	{
		if (const T* result = instance->fastDynamicCast<T>())
            return result;

		instance = instance->getParent();
	}

    return NULL;
}

void ScriptAnalysisWidget::updateResultsInstance(const shared_ptr<RBX::Instance>& instance)
{
    if (!instance)
		return;
	
	if (!instance->isA<RBX::Script>() && !instance->isA<RBX::ModuleScript>())
        return;

	if (findFirstAncestorByType<RBX::CoreGuiService>(instance.get()))
        return;

    if (instance && (instance->isA<RBX::Script>() || instance->isA<RBX::ModuleScript>()))
    {
		LuaSourceBuffer script = LuaSourceBuffer::fromInstance(instance);
		if (script.isCodeEmbedded())
        {
            RBX::ScriptAnalyzer::Result result = RBX::ScriptAnalyzer::analyze(
				m_dataModel.get(), instance, script.getScriptText());

			if (FFlag::StudioUpdateSAResultsInUIThread)
			{
				QMetaObject::invokeMethod(this, "updateResultsInternal", Qt::AutoConnection, 
					          Q_ARG(shared_ptr<RBX::Instance>, instance), Q_ARG(RBX::ScriptAnalyzer::Result, result));
			}
			else
			{
				updateResults(script, result);
			}
        }
		else if (!script.toInstance()->getScriptId().isNull())
		{
			boost::shared_ptr<LinkedSourceInstance> linkedSource(new LinkedSourceInstance(script.toInstance()));
			linkedSource->linkedSourceRemoved.connect(boost::bind(&ScriptAnalysisWidget::onLinkedSourceRemoved, this, _1));
			m_LinkedSources.insert(linkedSource);
		}
    }
}

void ScriptAnalysisWidget::onNamedAssetsLoaded(int gameId)
{
	if (m_currentGameId > 0)
	{
		m_pTreeWidget->removeGameScriptAssets();
		m_LinkedSources.clear();
	}

	m_currentGameId = gameId;

	if (m_currentGameId > 0)
	{
		updateResultsGameScriptAssets();
	}
}

void ScriptAnalysisWidget::onNamedAssetModified(int gameId, const QString& assetName)
{
	if (m_currentGameId == gameId)
		requestAnalyzeScriptAssetSource(assetName);
}

void ScriptAnalysisWidget::onLinkedSourceRemoved(boost::shared_ptr<LinkedSourceInstance> linkedSourceInstance)
{
	LinkedSourceInstanceCollection::iterator iter = m_LinkedSources.find(linkedSourceInstance);
	if (iter != m_LinkedSources.end())
	{
		if (FFlag::StudioUpdateSAResultsInUIThread)
		{
			QMetaObject::invokeMethod(this, "analyzeAndUpdateResults", Qt::QueuedConnection, 
				                      Q_ARG(shared_ptr<RBX::Instance>, linkedSourceInstance->getInstance()));
		}
		else
		{
			updateResultsInstance(linkedSourceInstance->getInstance());
		}

		m_LinkedSources.erase(iter);
	}
}

void ScriptAnalysisWidget::updateResultsGameScriptAssets()
{
	std::vector<QString> scriptNames;
	UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER).getListOfScripts(&scriptNames);
	// update all script assets
	std::for_each(scriptNames.begin(), scriptNames.end(), boost::bind(&ScriptAnalysisWidget::requestAnalyzeScriptAssetSource, this, _1));
}

static void invokeAnalyzeScriptAssetSource(ScriptAnalysisWidget* analysisWidget, const QString& scriptAssetName, RBX::HttpFuture future)
{
	QMetaObject::invokeMethod(analysisWidget, "analyzeScriptAssetSource", Qt::QueuedConnection,
		                      Q_ARG(QString, scriptAssetName), Q_ARG(RBX::HttpFuture, future));
}

void ScriptAnalysisWidget::requestAnalyzeScriptAssetSource(const QString& scriptAssetName)
{
	RobloxGameExplorer& gameExplorer = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
	RBX::ContentId scriptContentId = RBX::ContentId::fromGameAssetName(scriptAssetName.toStdString());

	RBX::HttpFuture future = LuaSourceBuffer::getScriptAssetSource(scriptContentId, gameExplorer.getCurrentGameId());
	if (future.is_ready())
	{
		analyzeScriptAssetSource(scriptAssetName, future);
	}
	else
	{
		future.then(boost::bind(&invokeAnalyzeScriptAssetSource, this, scriptAssetName, _1));
	}
}

void ScriptAnalysisWidget::analyzeScriptAssetSource(const QString& scriptAssetName, RBX::HttpFuture future)
{
	RobloxGameExplorer& gameExplorer = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
	if (m_currentGameId != gameExplorer.getCurrentGameId())
		return;

	std::string scriptCode;
	try
	{
		scriptCode = future.get();
	}
	catch (const RBX::base_exception&)
	{
		return;
	}

	// create a fake script instance for analysis
	shared_ptr<RBX::Instance> scriptInstance = RBX::Creatable<RBX::Instance>::create<RBX::Script>();

	RBX::DataModel::LegacyLock lock(m_dataModel.get(), RBX::DataModelJob::Write);
	RBX::ScriptAnalyzer::Result result = RBX::ScriptAnalyzer::analyze(m_dataModel.get(), scriptInstance, scriptCode);
	
	// update results in main thread
	updateResultsScriptAsset(scriptAssetName, result);
}

void ScriptAnalysisWidget::updateResultsScriptAsset(const QString& scriptAssetName, const RBX::ScriptAnalyzer::Result& scriptResults)
{		
	RBX::ContentId contentId = RBX::ContentId::fromGameAssetName(scriptAssetName.toStdString());
	if (!contentId.isNull())
		updateResults(LuaSourceBuffer::fromContentId(contentId), scriptResults);
}

void ScriptAnalysisWidget::updateResultsInternal(shared_ptr<RBX::Instance> script, const RBX::ScriptAnalyzer::Result& result)
{	
	RBX::DataModel::LegacyLock lock(m_dataModel.get(), RBX::DataModelJob::Write);
	updateResults(LuaSourceBuffer::fromInstance(script), result); 
}

void ScriptAnalysisWidget::analyzeAndUpdateResults(shared_ptr<RBX::Instance> script)
{
	RBX::DataModel::LegacyLock lock(m_dataModel.get(), RBX::DataModelJob::Write);
	updateResultsInstance(script);
}