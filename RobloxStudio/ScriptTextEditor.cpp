/**
 * ScriptTextEditor.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "ScriptTextEditor.h"

// Qt Headers
#include <QSettings>
#include <QTextCursor>

// Roblox Headers
#include "script/ScriptContext.h"
#include "v8datamodel/DataModel.h"

// Roblox Studio Headers
#include "RobloxScriptDoc.h"
#include "RobloxMainWindow.h"
#include "AuthoringSettings.h"
#include "ScriptSyntaxHighlighter.h"
#include "ScriptSideWidget.h"
#include "FindDialog.h"
#include "UpdateUIManager.h"
#include "QtUtilities.h"
#include "StudioIntellesense.h"
#include "RobloxDocManager.h"
#include "ScriptAnalysisWidget.h"
#include "RobloxCustomWidgets.h"

FASTFLAG(LuaDebugger)
FASTFLAG(StudioSeparateActionByActivationMethod)

FASTFLAGVARIABLE(ScriptContextSearchFix, true)

static const int MaxFontZoomIn = 30;
static const int MaxFontZoomOut = 5;
static const int WikiSearchTimerInterval = 1000;
ScriptTextEditor::WikiLookup ScriptTextEditor::wikiLookup = WikiLookup(""); 

FASTINTVARIABLE(StudioSyntaxCheckTimerInterval, 500)

FASTFLAGVARIABLE(ElseAutoIndentFix, false)
FASTFLAGVARIABLE(StudioEmbeddedFindDialogEnabled, false)

// required for QList<QTextEdit::ExtraSelection>
static bool operator== (const QTextEdit::ExtraSelection& lhs, const QTextEdit::ExtraSelection& rhs)
{  return (lhs.cursor == rhs.cursor && lhs.format == rhs.format); }

// TODO - auto-format selected text

/*****************************************************************************/
// ScriptTextEditor
/*****************************************************************************/

ScriptTextEditor::ScriptTextEditor(
    RobloxScriptDoc*  pScriptDoc,
    RobloxMainWindow* pMainWindow)
    : QPlainTextEdit(pMainWindow),
      m_pScriptDoc(pScriptDoc),
      m_pSyntaxHighlighter(NULL),
      m_pSideWidget(NULL),
      m_pCheckSyntaxThread(NULL),
	  m_pFindThread(NULL),
      m_pCheckSyntaxTimer(NULL),
      m_errorLine(-1),
      m_pMainWindow(pMainWindow),
	  currentWikiSearchSelection(),
	  m_selectionMutex(QMutex::Recursive)
{
    // highlighter
    m_pSyntaxHighlighter = new ScriptSyntaxHighlighter(document());

    m_pSideWidget = new ScriptSideWidget(this);
    connect(this,SIGNAL(updateRequest(QRect,int)),m_pSideWidget,SLOT(updateArea()));
	if (FFlag::LuaDebugger)
		connect(m_pSideWidget, SIGNAL(toggleBreakpoint(int)), this, SIGNAL(toggleBreakpoint(int)));

	m_pCheckSyntaxThread = new CheckSyntaxThread(this, pScriptDoc);
    connect(this,SIGNAL(textChanged()),this,SLOT(checkSyntax()));
	if (FFlag::StudioEmbeddedFindDialogEnabled)
	{
		m_pFindThread = new FindThread(this, pScriptDoc);
		connect(this, SIGNAL(textChanged()), this, SLOT(startFind()));
	}
    
	m_pCheckSyntaxTimer = new QTimer(this);
	connect(m_pCheckSyntaxTimer,SIGNAL(timeout()),this,SLOT(onCheckSyntaxTimerTimeOut()));

    setFrameShape(QFrame::NoFrame);
    setFrameShadow(QFrame::Plain);
    setMouseTracking(true);

    // always show scroll bars - avoids flickering
	//    (-- Not too sure when does this happen? disabling for ribbon)
	if (!m_pMainWindow->isRibbonStyle())
	{
		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
	}

	// listen for changes to the editor settings
    m_PropertyChangedConnection = AuthoringSettings::singleton().propertyChangedSignal.connect(
        boost::bind(&ScriptTextEditor::onPropertyChanged,this,_1));
    onPropertyChanged(NULL);

    m_ContextMenu = new QMenu(this);
    {
        m_ContextMenu->addAction(m_pMainWindow->undoAction);
        m_ContextMenu->addAction(m_pMainWindow->redoAction);

        m_ContextMenu->addSeparator();

        m_ContextMenu->addAction(m_pMainWindow->cutAction);
        m_ContextMenu->addAction(m_pMainWindow->copyAction);
        m_ContextMenu->addAction(m_pMainWindow->pasteAction);
        m_ContextMenu->addAction(m_pMainWindow->deleteSelectedAction);

        m_ContextMenu->addSeparator();

        m_ContextMenu->addAction(m_pMainWindow->selectAllAction);
        m_ContextMenu->addAction(m_pMainWindow->goToScriptErrorAction);

        m_ContextMenu->addSeparator();
        m_ContextMenu->addAction(m_pMainWindow->commentSelectionAction);
        m_ContextMenu->addAction(m_pMainWindow->uncommentSelectionAction);
        m_ContextMenu->addAction(m_pMainWindow->toggleCommentAction);
    
		m_ContextMenu->addSeparator();
		m_ContextMenu->addAction(m_pMainWindow->expandAllFoldsAction);
		m_ContextMenu->addAction(m_pMainWindow->collapseAllFoldsAction);

        m_ContextMenu->addSeparator();
		m_ContextMenu->addAction(m_pMainWindow->zoomInAction);
		m_ContextMenu->addAction(m_pMainWindow->zoomOutAction);
        m_ContextMenu->addAction(m_pMainWindow->resetScriptZoomAction);
	}

	connect(this, SIGNAL(selectionChanged()), this, SLOT(selectionChangedSearch()));
	connect(this, SIGNAL(helpTopicChanged(const QString&)), 
		&RobloxContextualHelpService::singleton(), SLOT(onHelpTopicChanged(const QString&)));

	m_pWikiSearchTimer = new QTimer(this);
	m_pWikiSearchTimer->setSingleShot(true);
	connect(m_pWikiSearchTimer, SIGNAL(timeout()), this, SLOT(onSearchTimeout()));
}

ScriptTextEditor::~ScriptTextEditor()
{
    if (m_pCheckSyntaxThread)
	{
        delete m_pCheckSyntaxThread;
		m_pCheckSyntaxThread = NULL;
	}

	delete m_pWikiSearchTimer;

    m_PropertyChangedConnection.disconnect();
}

void ScriptTextEditor::contextMenuEvent(QContextMenuEvent* event)
{
    UpdateUIManager::Instance().updateToolBars();
	if (FFlag::LuaDebugger)
		Q_EMIT updateContextualMenu(m_ContextMenu, event->pos());
    m_ContextMenu->exec(event->globalPos());
}

void ScriptTextEditor::focusInEvent(QFocusEvent* evt)
{
    QPlainTextEdit::focusInEvent(evt);
    RobloxDocManager::Instance().setCurrentDoc(m_pScriptDoc);
	if (AuthoringSettings::singleton().intellisenseEnabled)
		connect(&Studio::Intellesense::singleton(), SIGNAL(doubleClickSignal(QListWidgetItem*)), this, SLOT(intellesenseDoubleClick(QListWidgetItem*)));
}

void ScriptTextEditor::focusOutEvent(QFocusEvent* evt)
{
    QPlainTextEdit::focusOutEvent(evt);

    if (m_pScriptDoc)
        m_pScriptDoc->applyEditChanges();
	if (AuthoringSettings::singleton().intellisenseEnabled)
		disconnect(&Studio::Intellesense::singleton(), SIGNAL(doubleClickSignal(QListWidgetItem*)), this, SLOT(intellesenseDoubleClick(QListWidgetItem*)));
}

void ScriptTextEditor::intellesenseDoubleClick(QListWidgetItem* listItem)
{
    QString blockText = document()->findBlockByNumber(textCursor().blockNumber()).text();
    int cursorPos = textCursor().positionInBlock();

    Studio::Intellesense::singleton().replaceTextWithCurrentItem(blockText, cursorPos);
    replaceText(blockText, cursorPos);
}

void ScriptTextEditor::replaceText(QString text, int cursorPos)
{
    //replace text with blockText
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::EndOfBlock);
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.insertText(text);
    //Setting cursor position
    cursor.setPosition(textCursor().position() - textCursor().positionInBlock() + cursorPos);
    cursor.endEditBlock();
    this->setTextCursor(cursor);
}

void ScriptTextEditor::setViewportMargins(
    int left,
    int top,
    int right,
    int bottom)
{
    QPlainTextEdit::setViewportMargins(left,top,right,bottom);
}

QTextBlock ScriptTextEditor::firstVisibleBlock() const
{
    return QPlainTextEdit::firstVisibleBlock();
}

QPointF ScriptTextEditor::contentOffset() const
{
    return QPlainTextEdit::contentOffset();
}

QRectF ScriptTextEditor::blockBoundingGeometry(const QTextBlock& block) const
{
    return QPlainTextEdit::blockBoundingGeometry(block);
}

void ScriptTextEditor::expandAllFolds(bool expand)
{
	m_pSideWidget->toggleAllFolds(expand);
}

void ScriptTextEditor::resizeEvent(QResizeEvent* e)
{
    QPlainTextEdit::resizeEvent(e);
    m_pSideWidget->setFixedHeight(height());
	if (FFlag::StudioEmbeddedFindDialogEnabled)
		updateEmbeddedFindPosition();
}

void ScriptTextEditor::updateEmbeddedFindPosition()
{
	if (FFlag::StudioEmbeddedFindDialogEnabled)
		FindReplaceProvider::instance().getEmbeddedFindDialog()->updateFindPosition(mapToGlobal(viewport()->pos()), viewport()->size());
}

void ScriptTextEditor::homeEvent(QKeyEvent* e)
{
	QTextCursor currentCursor = textCursor();

	QString left = currentCursor.block().text().left(currentCursor.positionInBlock());
	int moveOffset = left.indexOf(QRegExp("\\S"));

	if (moveOffset < 0)
		currentCursor.movePosition(QTextCursor::StartOfBlock, (e->modifiers() == Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
	else
		currentCursor.setPosition(currentCursor.block().position() + moveOffset, (e->modifiers() == Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
	
	setTextCursor(currentCursor);
}

void ScriptTextEditor::endEvent(QKeyEvent* e)
{
	QTextCursor currentCursor = textCursor();

	QString right = currentCursor.block().text().right(currentCursor.block().text().length() - currentCursor.positionInBlock());
	int moveOffset = right.lastIndexOf(QRegExp("\\S"));
		
	if (moveOffset < 0)
		currentCursor.movePosition(QTextCursor::EndOfBlock, (e->modifiers() == Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
	else
		currentCursor.setPosition(currentCursor.position() + moveOffset + 1, (e->modifiers() == Qt::ShiftModifier) ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);

	setTextCursor(currentCursor);
}

bool ScriptTextEditor::event(QEvent* e)
{
    if (e->type() == QEvent::ToolTip)
    {
        processTooltip(static_cast<QHelpEvent*>(e));
        return true;
    }
	else if (e->type() == QEvent::KeyPress)
	{
		if (static_cast<QKeyEvent*>(e)->modifiers().testFlag(Qt::ControlModifier) && (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Tab))
		{
			e->accept();
			QCoreApplication::sendEvent(&UpdateUIManager::Instance().getMainWindow(), e);
			return true;
		}
	}
	else if (e->type() == EXTRASELECTION_UPDATE)
	{
		updateFindSelection();
	}
    return QPlainTextEdit::event(e);
}

void ScriptTextEditor::mousePressEvent(QMouseEvent *e)
{
	QPlainTextEdit::mousePressEvent(e);

	QTextCursor cursor = textCursor();
	m_originalCursorPosition = cursor.position();
}

void ScriptTextEditor::keyPressEvent(QKeyEvent* e)
{
	if (!e || !document() || isReadOnly())
        return;

    const int key = e->key();

    Qt::KeyboardModifiers modifiers = e->modifiers();

    if (AuthoringSettings::singleton().intellisenseEnabled && modifiers != Qt::ControlModifier)
    {
        //Check if there is selection and key is alphanumeric or special
        if (textCursor().hasSelection() && key < Qt::Key_nobreakspace)
            textCursor().removeSelectedText();

		int blockNumber = textCursor().blockNumber();
        QString blockText = document()->findBlockByNumber(blockNumber).text();
        int cursorPos = textCursor().positionInBlock();

        QRect cursorGeometry = cursorRect();
        QPoint topLeftCursorPos = mapToGlobal(cursorGeometry.topLeft());
        QFontMetrics fontMetrics = QFontMetrics(font());

        int cursorHeight = cursorGeometry.height();

		cursorGeometry.setX(topLeftCursorPos.x() + (fontMetrics.averageCharWidth() * 2) + 40);
        
#ifdef __APPLE__
		cursorGeometry.setX(cursorGeometry.x() + 5);
#endif
        
        cursorGeometry.setY(topLeftCursorPos.y());

        cursorGeometry.setWidth(1);
        cursorGeometry.setHeight(cursorHeight);

        if (Studio::Intellesense::singleton().activate(blockText, blockNumber, cursorPos, e, cursorGeometry, false, this, m_analysisResult))
        {
            if (blockText != document()->findBlockByNumber(blockNumber).text())
                replaceText(blockText, cursorPos);

            return;
        }
    }
    else if (key != Qt::Key_Control && modifiers == Qt::ControlModifier)
    {
        Studio::Intellesense::singleton().deactivate();
    }

    if ((key == Qt::Key_Tab) || (key == Qt::Key_Backtab))
    {
        if (tabSelection(e->key() != Qt::Key_Backtab))
        {
            e->accept();
            return;
        }
        else if ( key == Qt::Key_Backtab )
        {
            int pos = textCursor().position() - 1;    
            QString text = toPlainText();
            if ( pos > 0 && toPlainText()[pos] == '\t' )
            {
                textCursor().deletePreviousChar();
                e->accept();
                return;
            }
        }
    }
    else if (key == Qt::Key_Insert)
    {
        setOverwriteMode(!overwriteMode());
        e->accept();
        return;
    }
	else if (key == Qt::Key_Home && modifiers != Qt::ControlModifier )				
	{
		homeEvent(e);
		e->accept();
		return;
	}
	else if (key == Qt::Key_End && modifiers != Qt::ControlModifier )
	{
		endEvent(e);
		e->accept();
		return;
	}
    else if ( key == Qt::Key_Return || key == Qt::Key_Backspace )
    {
        // Treat Shift+Return as only Return (DE5218, DE5215)
		if ( key == Qt::Key_Return && modifiers == Qt::ShiftModifier )
			e->setModifiers(Qt::NoModifier);

        QTextCursor currentTextCursor = textCursor();
        RBXASSERT(!currentTextCursor.isNull());

        QTextBlock startBlock = currentTextCursor.block();
        RBXASSERT(startBlock.isValid());
        RBXASSERT(startBlock.userData());

        RBXTextUserData& userData = static_cast<RBXTextUserData&>(*startBlock.userData());
        QTextBlock endBlock = ScriptEditorUtils::findFoldBoundary(startBlock,false);

        // if the block is folded and the block ending is valid, unfold if necessary
        if ( (userData.getFoldState() > 0) &&
             endBlock.isValid() &&
             (startBlock != endBlock) &&
             !endBlock.isVisible() )
        {
            // if cursor is at the end then just set it at the end of end block
            if ( key == Qt::Key_Return &&
                 !currentTextCursor.hasSelection() && 
                 currentTextCursor.atBlockEnd() )
            {
                currentTextCursor.setPosition(endBlock.position() + endBlock.length() - 1);
                setTextCursor(currentTextCursor);
            }

            const int lineNumber = startBlock.blockNumber() + 1;
            m_pSideWidget->unfold(lineNumber);
            e->accept();
            return;
        }
        // if we didn't unfold then we should auto indent next line
        else if ( key == Qt::Key_Return && AuthoringSettings::singleton().editorAutoIndent )
        {
            QPlainTextEdit::keyPressEvent(e);
            autoIndent();
            e->accept();
            return;
        }
    }
	else if (FFlag::LuaDebugger && key == Qt::Key_F9)
	{
        QTextBlock block = textCursor().block();
		if (block.isValid())
		{
			//let debugger handle it
			Q_EMIT toggleBreakpoint(block.blockNumber());
			e->accept();
			return;
		}
	}

    QPlainTextEdit::keyPressEvent(e);
}

void ScriptTextEditor::wheelEvent(QWheelEvent* event)
{
	// if mouse scroll wheel + control => zoom text
    if ( event->modifiers() & Qt::ControlModifier )
    {        
        if ( event->delta() > 0 )
            zoom(1);
        else if ( event->delta() < 0 )
            zoom(-1);

        m_pMainWindow->zoomInAction->setEnabled(AuthoringSettings::singleton().editorFont.pointSize() < MaxFontZoomIn);    
        m_pMainWindow->zoomOutAction->setEnabled(AuthoringSettings::singleton().editorFont.pointSize() > MaxFontZoomOut);
        event->accept();
    }
    else
    {
        QPlainTextEdit::wheelEvent(event);
    }
}

void ScriptTextEditor::zoom(int delta, bool setDeltaAsValue)
{
    QFont font = AuthoringSettings::singleton().editorFont;
    int fontSize = setDeltaAsValue ? delta : qBound(MaxFontZoomOut, font.pointSize() + delta, MaxFontZoomIn);
    font.setPointSize(fontSize);
    AuthoringSettings::singleton().setEditorFont(font);
}

bool ScriptTextEditor::tabSelection(bool bPrepend)
{
    bool eventAccepted = false;

    QTextCursor cursor = textCursor();

    if (!cursor.isNull() && cursor.hasSelection())
    {
        int  posAnchor          = cursor.anchor();
        int  pos                = cursor.position();
        bool bDownwardSelection = (posAnchor < pos);

        int startPos = cursor.selectionStart();
        int endPos   = cursor.selectionEnd();

        QTextBlock startBlock = document()->findBlock(startPos);
        QTextBlock endBlock   = document()->findBlock(endPos);

        if (startBlock.blockNumber() != endBlock.blockNumber())
        {
            bool bEndInMid = (endPos > endBlock.position());
            if (!bEndInMid)
                endBlock = endBlock.previous();

            QString    modifiedText;
            QTextBlock tmpBlock = startBlock;
            while (tmpBlock.isValid() && tmpBlock.blockNumber() <= endBlock.blockNumber())
            {
                if (bPrepend)
                {
                    modifiedText.append("\t");
                    modifiedText.append(tmpBlock.text());
                }
                else
                {
                    QString tmpStr = tmpBlock.text();
                    if (tmpStr.startsWith("\t"))
                    {
                        tmpStr.remove(0,1);
                    }
                    else if (tmpStr.startsWith(" "))
                    {
                        for (int ii = 0 ; ii < AuthoringSettings::singleton().editorTabWidth ; ii++)
                        {
                            if (tmpStr.startsWith(" "))
                                tmpStr.remove(0,1);
                        }
                    }
                    modifiedText.append(tmpStr);
                }

                modifiedText.append("\n");
                bool isLastBlock = (tmpBlock == document()->lastBlock());
                if (isLastBlock || (!bDownwardSelection && bEndInMid && (tmpBlock.blockNumber() == endBlock.blockNumber())))
                    modifiedText.remove(modifiedText.length() - 1,1);  // Remove the extra "\n"

                if (isLastBlock)
                    break;

                tmpBlock = tmpBlock.next();
            }

            if (!modifiedText.isEmpty())
            {
                replaceSelection(startPos,endPos,modifiedText);
                eventAccepted = true;
            }
        }
    }

    return eventAccepted;
}

void ScriptTextEditor::replaceSelection(
    int            startPos,
    int            endPos,
    const QString& text)
{
    if (text.isEmpty())
        return;

    QTextCursor      cursor      = textCursor();
    const QTextBlock startBlock  = document()->findBlock(startPos);
    QTextBlock       endBlock    = document()->findBlock(endPos);
    const int        anchorPos   = cursor.anchor();
    const int        currentPos  = cursor.position();
    const bool       isDownwards = (anchorPos <= currentPos);
    const bool       isEndInMid  = (endPos > endBlock.position());
	bool isLastBlock = false;

    if (!isEndInMid)
        endBlock = endBlock.previous();

    // Correct the selection which may be incomplete
    if (isDownwards)
    {
        cursor.setPosition(anchorPos);
        if (anchorPos > startBlock.position())
            cursor.movePosition(QTextCursor::StartOfBlock,QTextCursor::MoveAnchor);

        if (endBlock != document()->lastBlock())
        {
            if (currentPos > endBlock.position())
                cursor.setPosition(endBlock.next().position(),QTextCursor::KeepAnchor);
            else
                cursor.setPosition(currentPos,QTextCursor::KeepAnchor);

            cursor.movePosition(QTextCursor::StartOfBlock,QTextCursor::KeepAnchor);
        }
        else
        {
            cursor.setPosition(currentPos,QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::EndOfBlock,QTextCursor::KeepAnchor);
			isLastBlock = true;
        }
    }
    else
    {
        cursor.setPosition(anchorPos);
        if ((anchorPos > endBlock.position()) && isEndInMid)
            cursor.movePosition(QTextCursor::EndOfBlock,QTextCursor::MoveAnchor);

        cursor.setPosition(currentPos,QTextCursor::KeepAnchor);
        if (currentPos > startBlock.position())
            cursor.movePosition(QTextCursor::StartOfBlock,QTextCursor::KeepAnchor);
    }

    // Insert text and restore selection
    int anchorBefore = cursor.anchor();
    int posBefore = cursor.position();
    cursor.insertText(text);
    int posAfter = cursor.position();

    if (isDownwards)
    {
        cursor.setPosition(anchorBefore,QTextCursor::MoveAnchor);
        cursor.setPosition(isLastBlock ? posAfter : posAfter-1,QTextCursor::KeepAnchor);
    }
    else
    {
        cursor.setPosition(posAfter,QTextCursor::MoveAnchor);
        cursor.setPosition(posBefore,QTextCursor::KeepAnchor);
    }

    setTextCursor(cursor);
}

/**
 * Auto indents the next line based on the current block level.
 *  For regular lines, the previous line's tabs will be used.
 *  For ending blocks, one tab will be removed.
 */
void ScriptTextEditor::autoIndent()
{
    const QString    data             = toPlainText();
    QTextCursor      cursor           = textCursor();
    const int        cursorPosition   = cursor.position();
    const QTextBlock previousBlock    = cursor.block().previous();
    const int        previousPosition = previousBlock.position();
    QString          indent;

    if (previousBlock.isValid())
    {
        RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(previousBlock.userData());
        if (pUserData)
        {
            const int foldState = pUserData->getFoldState();

            const QString text         = previousBlock.text().trimmed() + ' ';
            const bool    isElse       = text.startsWith("else ") || text.startsWith("elseif ");
            int           requiredTabs = 0;

            // if block is not closed/ended yet or special case else
            if ((foldState < 0) || isElse)
            {
                if (isElse)
                {
                    // start at previous block
                    QTextBlock block     = previousBlock.previous();
                    int        foldState = 0;

                    while (block.isValid())
                    {
                        const RBXTextUserData* userData = dynamic_cast<RBXTextUserData*>(block.userData());
                        if (userData)
                        {
                            // add current block's foldState
                            foldState += userData->getFoldState();

                            // if we have positive fold state
                            if (foldState > 0)
                            {
                                QString line    = block.text();
                                QString trimmed = line.trimmed() + ' ';
                                if (trimmed.startsWith("if ") || (FFlag::ElseAutoIndentFix && trimmed.startsWith("if(")))
                                {
                                    while (line[requiredTabs] == '\t')
                                        ++requiredTabs;

                                    break;
                                }
                            }
                        }

                        block = block.previous();
                    }
                }
                else
                {
                    requiredTabs = ScriptEditorUtils::noOfStartingTabs(
                            ScriptEditorUtils::findFoldBoundary(previousBlock,true));
                }

                // make indent string
                while (indent.length() < requiredTabs)
                    indent.append('\t');

                // select the text to replace
                cursor.setPosition(previousPosition,QTextCursor::MoveAnchor);
                cursor.setPosition(cursorPosition,QTextCursor::KeepAnchor);

                QString modifiedText = previousBlock.text();

                // remove tabs at the beginning, we're going to set them correctly
                while (modifiedText.startsWith('\t'))
                    modifiedText = modifiedText.remove(0,1);

                modifiedText.prepend(indent); // add tabs at beginning
                modifiedText.append('\n');
                modifiedText.append(indent);  // add tabs at end

                if (isElse)
                    modifiedText.append('\t');

                cursor.insertText(modifiedText);
                cursor.clearSelection();
            }
            else
            {
                const int noOfOpeningTabs = ScriptEditorUtils::noOfStartingTabs(
                        ScriptEditorUtils::findFoldBoundary(previousBlock,true));
                while (indent.length() < noOfOpeningTabs)
                    indent.append('\t');

                // if we're opening a new block, indent one more
                if (foldState > 0)
                {
                    if (getDocFoldState() > 0)
                    {
                        autoCompleteBlock(text, indent);
                        return;
                    }
                    indent.append('\t');
                }

                cursor.insertText(indent);
            }
        }
    }
}

void ScriptTextEditor::autoCompleteBlock(const QString& text, QString& indent)
{
    QTextCursor cursor = textCursor();
    QString foldEnd = QString("end");
	QRegExp commentMatchingExp = QRegExp("(--)?\\[=*\\[");
    if (text.contains(QRegExp("\\brepeat\\b")) && !text.contains(QRegExp("\\buntil\\b")))
        foldEnd = QString("until");
	else if ((commentMatchingExp.indexIn(text) > -1) && !text.contains(commentMatchingExp.cap().replace('[', ']')))
		foldEnd = QString(commentMatchingExp.cap().replace('[', ']'));
    else if (text.contains("{") && !text.contains("}"))
        foldEnd = QString("}");
    else if (text.contains("(") && !text.contains(")"))
        foldEnd = QString(")");
	else if (text.contains("function") && text.count("(") > text.count(")"))
		foldEnd = QString("end)");

    indent.append(QString("\t\n") + indent + foldEnd);
    cursor.insertText(indent);
    cursor.movePosition(cursor.PreviousBlock);
    cursor.movePosition(cursor.EndOfBlock);
    setTextCursor(cursor);
}

void ScriptTextEditor::checkSyntax()
{
    // Hide any visible tooltip
    if (QToolTip::isVisible())
        QToolTip::hideText();
    
    if (m_pCheckSyntaxTimer)
        m_pCheckSyntaxTimer->start(FInt::StudioSyntaxCheckTimerInterval);
    else if (m_pCheckSyntaxThread)
        QTimer::singleShot(0,this,SLOT(onCheckSyntax()));
}

void ScriptTextEditor::onCheckSyntax()
{
    if (m_pCheckSyntaxThread)
        m_pCheckSyntaxThread->requestRun();
}

void ScriptTextEditor::startFind()
{
	if (QToolTip::isVisible())
		QToolTip::hideText();

	m_findSelectionFormat.setBackground(QBrush(QtUtilities::toQColor(AuthoringSettings::singleton().editorFindSelectionBackgroundColor)));

	m_pFindThread->setAutoFindNext(false);
	clearFoundItems();

	QTimer::singleShot(0,this,SLOT(onFind()));
}

void ScriptTextEditor::onFind()
{
	if (m_pFindThread)
		m_pFindThread->requestRun();
}

/**
 * Set the current error message.
 *  This method is invoked.
 *  
 * @param errorLine     line in text, 0 based
 * @param errorMessage  error text, must be copy because of multi-threading
 */
void ScriptTextEditor::setErrorLine(
    int     errorLine,
    QString errorMessage)
{
	// make sure we do not show error for invalid line
	if (FFlag::LuaDebugger && errorLine < 0 && m_errorLine < 0)
		return;

    QList<QTextEdit::ExtraSelection> extraSelections;

    QString message = errorMessage;
    int visibleLine = errorLine;

    QTextBlock errorBlock = document()->findBlockByNumber(visibleLine);
    if ( errorBlock.isValid() )
    {
	    // if block isn't visible, modify error message
        if ( !errorBlock.isVisible() )
            message = "Error in block: " + errorMessage;

	    // search up until we find a block that is actually visible
        //  this will always succeed if we hit the top level
        while ( !errorBlock.isVisible() || errorBlock.text().length() == 0 )
            errorBlock = ScriptEditorUtils::findFoldBoundary(errorBlock.previous(),true);

        visibleLine = errorBlock.blockNumber();

	    // squiggly underline the bad line
        QTextEdit::ExtraSelection selection;
        selection.format = m_ErrorFormat;
        selection.cursor = QTextCursor(errorBlock);
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }

    // if the block is invalid, this will clear any old error line
    setExtraSelections(extraSelections);

    // if this is a new error, report to status bar
    if ( visibleLine >= 0 &&
         ( m_visibleErrorLine != visibleLine || m_errorMessage != message) )
    {
        m_pMainWindow->statusBar()->showMessage(message,5000);
    }

    m_errorMessage      = message;
    m_errorLine         = errorLine;
    m_visibleErrorLine  = visibleLine;

    m_pMainWindow->goToScriptErrorAction->setEnabled(m_errorLine >= 0);
}

void ScriptTextEditor::setAnalysisResult(const RBX::ScriptAnalyzer::Result& analysisResults)
{
	m_scriptMessageForRange.clear();
	m_errorLine = -1;
	removeFromExtraSelections(m_AnalysisSelections);

	QList<QTextEdit::ExtraSelection> extraSelections;
	RBX::ScriptAnalyzer::Location loc;
	int startPos = -1, endPos = -1;
	bool atEof = false;

	if (analysisResults.error)
	{
		RBX::ScriptAnalyzer::Error error = analysisResults.error.get();
		loc = error.location;

		populateValuesFromLocation(loc, startPos, endPos, atEof);
		if (appendExtraSelection(m_ErrorFormat, startPos, endPos, atEof, m_AnalysisSelections))
		{
			// if this is a new error, report to status bar
			QString message = error.text.c_str();
			m_pMainWindow->statusBar()->showMessage(message, 5000);

			// save error information (if we need to show tooltip for the entire error line, then set startPos and endPos as 0)
			ScriptMessage sm = { 0, message, startPos, endPos };
			m_scriptMessageForRange.insert(std::make_pair(std::make_pair(startPos, endPos), sm));

			m_errorLine = loc.begin.line;
		}
	}
	else
	{
		m_analysisResult = analysisResults.intellesenseAnalysis;
	}

	for (size_t i = 0; i < analysisResults.warnings.size(); ++i)
	{
		RBX::ScriptAnalyzer::Warning warning = analysisResults.warnings[i];
		loc = warning.location;

		populateValuesFromLocation(loc, startPos, endPos, atEof);
		if (appendExtraSelection(m_WarningFormat, startPos, endPos, atEof, m_AnalysisSelections))
		{
			QString message;
			message.sprintf("W%03d: %s", warning.code, warning.text.c_str());

			ScriptMessage sm = {warning.code, message, startPos, endPos};
			m_scriptMessageForRange.insert(std::make_pair(std::make_pair(startPos, endPos), sm));		
		}
	}

	// update editor to refresh errors/warnings status
	update();

	// update goToError action state
	m_pMainWindow->goToScriptErrorAction->setEnabled(m_errorLine >= 0);
	// update analysis widget
	ScriptAnalysisWidget& scriptAnalysis = UpdateUIManager::Instance().getViewWidget<ScriptAnalysisWidget>(eDW_SCRIPT_ANALYSIS);
	scriptAnalysis.updateResults(m_pScriptDoc->getCurrentScript(), analysisResults);
}

bool ScriptTextEditor::appendExtraSelection(const QTextCharFormat& format, int startPos, int endPos, bool atEof, QList<QTextEdit::ExtraSelection>& appendList)
{
	if (!atEof && ((startPos < 0) || (endPos < 0) || (endPos < startPos)))
		return false;

	QTextEdit::ExtraSelection selection;
	selection.format = format;
	selection.cursor = textCursor();
	selection.cursor.clearSelection();

	if (atEof)
	{
        selection.cursor.setPosition(startPos, QTextCursor::MoveAnchor);
		selection.cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
		selection.cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
	}
	else
	{
		selection.cursor.setPosition(startPos, QTextCursor::MoveAnchor);
		selection.cursor.setPosition(endPos, QTextCursor::KeepAnchor);
	}

	addToExtraSelections(selection);
	appendList.append(selection);

	return true;
}

void ScriptTextEditor::processTooltip(QHelpEvent* evt)
{
    if (!evt)
        return;

	// Assumption: showToopTip signal being used only from DebuggerClient so it should be safe to use return value of the signal 
	// we can have issues with this approach, if the same signal is connected to multiple slots.
	bool handled = false;
	if (FFlag::LuaDebugger)
		handled = Q_EMIT showToolTip(evt->globalPos(), getVariableAtPos(evt->globalPos()));

	if (handled || m_scriptMessageForRange.empty())
	{
		QToolTip::hideText();
		evt->ignore();
		return;
	}

	QPoint pos = viewport()->mapFromGlobal(evt->globalPos());
	QTextCursor cursor = cursorForPosition(pos);
	int cursorPos = cursor.position();
	
	// multimap is required if we've overlaping range!
	std::pair<MessageCollection::const_iterator, MessageCollection::const_iterator> ret = m_scriptMessageForRange.equal_range(Range(cursorPos, cursorPos));
	if (ret.first != ret.second)
	{
		for (MessageCollection::const_iterator iter=ret.first; iter != ret.second; ++iter)
		{
			if ((cursorPos >= iter->first.first) &&  (cursorPos <= iter->first.second))
			{
				QToolTip::showText(evt->globalPos(), iter->second.text);
				evt->accept();
				handled = true;
				break;
			}
		}
	}
	
	if (!handled)
	{
		QToolTip::hideText();
		evt->ignore();
	}
}

void ScriptTextEditor::commentSelection()
{
	bool isBlockUnfolded = false;
	updateSelection(&isBlockUnfolded);

    QTextCursor cursor = textCursor();
    if ( cursor.isNull() )
        return;

	const int        startPos    = cursor.selectionStart();
    const int        endPos      = cursor.selectionEnd();
    const QTextBlock startBlock  = document()->findBlock(startPos);
    QTextBlock       endBlock    = document()->findBlock(endPos);
    const int        anchorPos   = cursor.anchor();
    const int        currentPos  = cursor.position();
    const bool       isDownwards = (anchorPos <= currentPos);

    bool isEndInMid = (endPos > endBlock.position());

    if ( !isEndInMid )
        endBlock = endBlock.previous();

    QString modifiedText;
    QTextBlock tmpBlock = startBlock;

    while ( tmpBlock.isValid() && tmpBlock.blockNumber() <= endBlock.blockNumber() )
    {
		// check if we need to unfold the block
        RBXTextUserData* userData = dynamic_cast<RBXTextUserData*>(tmpBlock.userData());
		if ( userData && userData->getFoldState() && m_pSideWidget->isFolded(tmpBlock) )
		{
			const int lineNumber = tmpBlock.blockNumber() + 1;
			m_pSideWidget->unfold(lineNumber, false);
			isBlockUnfolded = true;
		}

        modifiedText.append("--");
        modifiedText.append(tmpBlock.text());
        modifiedText.append("\n");

        bool isLastBlock = (tmpBlock == document()->lastBlock());
        if ( isLastBlock ||
            (!isDownwards && isEndInMid && (tmpBlock.blockNumber() == endBlock.blockNumber())))
        {
            modifiedText.remove(modifiedText.length() - 1,1); // Remove the extra "\n"
        }

        if ( isLastBlock )
            break;

        tmpBlock = tmpBlock.next();
    }

    replaceSelection(startPos,endPos,modifiedText);

	// now update view
	updateViewAndCursor(isBlockUnfolded);
}

void ScriptTextEditor::uncommentSelection()
{
    QTextCursor cursor = textCursor();
    if ( cursor.isNull() )
        return;

    const int        startPos    = cursor.selectionStart();
    const int        endPos      = cursor.selectionEnd();
    const QTextBlock startBlock  = document()->findBlock(startPos);
    QTextBlock       endBlock    = document()->findBlock(endPos);
    const int        anchorPos   = cursor.anchor();
    const int        currentPos  = cursor.position();
    const bool       isDownwards = (anchorPos <= currentPos);

    bool isEndInMid = (endPos > endBlock.position());
    if (!isEndInMid)
        endBlock = endBlock.previous();

	bool isBlockUnfolded = false;
    QString modifiedText;
    QTextBlock tmpBlock = startBlock;
    while (tmpBlock.isValid() && tmpBlock.blockNumber() <= endBlock.blockNumber())
    {
		RBXTextUserData* userData = dynamic_cast<RBXTextUserData*>(tmpBlock.userData());
		if ( userData && userData->getFoldState() && m_pSideWidget->isFolded(tmpBlock) )
		{
			const int lineNumber = tmpBlock.blockNumber() + 1;
			m_pSideWidget->unfold(lineNumber, false);
			isBlockUnfolded = true;
		}

        QString text = tmpBlock.text();
        if (text.simplified().startsWith("--"))
            text.remove(text.indexOf("--"),2);
        modifiedText.append(text);
        modifiedText.append("\n");

        bool isLastBlock = (tmpBlock == document()->lastBlock());
        if (isLastBlock ||
            (!isDownwards && isEndInMid && (tmpBlock.blockNumber() == endBlock.blockNumber())))
        {
            modifiedText.remove(modifiedText.length() - 1,1); // Remove the extra "\n"
        }

        if (isLastBlock)
            break;

        tmpBlock = tmpBlock.next();
    }

    replaceSelection(startPos,endPos,modifiedText);

	// now update view
	updateViewAndCursor(isBlockUnfolded);
}

void ScriptTextEditor::toggleCommentSelection()
{
    QTextCursor cursor = textCursor();
    if ( cursor.isNull() )
        return;

    const int        startPos    = cursor.selectionStart();
    const int        endPos      = cursor.selectionEnd();
    const QTextBlock startBlock  = document()->findBlock(startPos);
    QTextBlock       endBlock    = document()->findBlock(endPos);
    bool             commented   = true;

    bool isEndInMid = (endPos > endBlock.position());
    if (!isEndInMid)
        endBlock = endBlock.previous();

    QString modifiedText;
    QTextBlock tmpBlock = startBlock;
    while (tmpBlock.isValid() && tmpBlock.blockNumber() <= endBlock.blockNumber())
    {
        QString text = tmpBlock.text();
        if (!text.startsWith("--"))
        {
            commented = false;
            break;
        }

        bool isLastBlock = (tmpBlock == document()->lastBlock());
        if (isLastBlock)
            break;

        tmpBlock = tmpBlock.next();
    }

    if (commented)
        uncommentSelection();
    else
        commentSelection();
}

void ScriptTextEditor::updateSelection(bool* isBlockUnfolded)
{
	QTextCursor cursor = textCursor();
    if ( cursor.isNull() )
        return;

	int        startPos    = cursor.selectionStart();
    int        endPos      = cursor.selectionEnd();
    QTextBlock startBlock  = document()->findBlock(startPos);
    QTextBlock endBlock    = document()->findBlock(endPos);
    
    RBXTextUserData* userData = dynamic_cast<RBXTextUserData*>(startBlock.userData());
    if ( userData && userData->getFoldState() )
    {
		if ( m_pSideWidget->isFolded(startBlock) )
		{
			const int lineNumber = startBlock.blockNumber() + 1;
			m_pSideWidget->unfold(lineNumber, false);
			if (isBlockUnfolded)
				*isBlockUnfolded = true;
		}

        endBlock = ScriptEditorUtils::findFoldBoundary(endBlock,false);
        if ( endBlock == document()->lastBlock() )
            endPos = endBlock.position();
        else
            endPos = endBlock.next().position() - 1;

		const bool  isDownwards = (cursor.anchor() <= cursor.position());
		if ( isDownwards )
		{
			cursor.setPosition(startPos,QTextCursor::MoveAnchor);
			cursor.movePosition(QTextCursor::StartOfBlock,QTextCursor::MoveAnchor);
			cursor.setPosition(endPos,QTextCursor::KeepAnchor);
			cursor.movePosition(QTextCursor::EndOfBlock,QTextCursor::KeepAnchor);
		}
		else
		{
			cursor.setPosition(endPos,QTextCursor::MoveAnchor);
			cursor.movePosition(QTextCursor::EndOfBlock,QTextCursor::MoveAnchor);
			cursor.setPosition(startPos,QTextCursor::KeepAnchor);
			cursor.movePosition(QTextCursor::StartOfBlock,QTextCursor::KeepAnchor);
		}

        setTextCursor(cursor);
    }
}

/**
 * Goes to the last error line and ensures visible.
 */
void ScriptTextEditor::goToError()
{
    if ( m_errorLine < 0 )
        return;

    QTextCursor cursor = textCursor();
    QTextBlock errorBlock = document()->findBlockByNumber(m_errorLine);
    cursor.setPosition(errorBlock.position());
    setTextCursor(cursor);

    ensureVisible(errorBlock);
}

/**
 * Ensures the given block is visible by unfolding parents.
 */
void ScriptTextEditor::ensureVisible(const QTextBlock& block)
{
    QTextBlock current = block;
    while ( current.isValid() && !current.isVisible() )
    {
        QTextBlock parent = ScriptEditorUtils::findFoldBoundary(current.previous(),true);
        m_pSideWidget->unfold(parent.blockNumber() + 1);  
        current = parent;
    }

    ensureCursorVisible();
    updateFolds();
}

/**
 * When updating due to fold changes we need to check syntax again.
 */
void ScriptTextEditor::updateFolds()
{
    m_pCheckSyntaxThread->requestRun();
    update();
}

void ScriptTextEditor::expandAllFolds()
{
    expandAllFolds(true);
}

void ScriptTextEditor::collapseAllFolds()
{
    expandAllFolds(false);
}


void ScriptTextEditor::goToLine()
{
	FindReplaceProvider::instance().getGotoLineDialog()->activate(textCursor().blockNumber() + 1, document()->blockCount());
}

void ScriptTextEditor::find()
{
	if (FFlag::StudioEmbeddedFindDialogEnabled)
	{
		FindReplaceProvider::instance().getEmbeddedFindDialog()->activate();
	}
	else
	{
		FindReplaceProvider::instance().getReplaceDialog()->hide();
		FindReplaceProvider::instance().getFindDialog()->show();
		FindReplaceProvider::instance().getFindDialog()->activateWindow();
		FindReplaceProvider::instance().getFindDialog()->setFocus();
	}
}

void ScriptTextEditor::moveNext(bool forward)
{
	QMutexLocker lock(&m_selectionMutex);
	QTextCursor cursor = textCursor();
	if (m_foundItems.size() > 0)
	{
		std::map<int, int>::iterator locationIter = forward ? m_foundItems.lower_bound(cursor.selectionEnd()) : m_foundItems.lower_bound(cursor.selectionStart());
		if (forward)
		{
			if (locationIter != m_foundItems.end())
			{
				cursor.setPosition(locationIter->first, QTextCursor::MoveAnchor);
				cursor.setPosition(locationIter->second, QTextCursor::KeepAnchor);
			}
			else
			{
				cursor.setPosition(m_foundItems.begin()->first, QTextCursor::MoveAnchor);
				cursor.setPosition(m_foundItems.begin()->second, QTextCursor::KeepAnchor);
			}
		}
		else
		{
			std::map<int, int>::reverse_iterator reverseLocationIter(locationIter);

			if (reverseLocationIter != m_foundItems.rend())
			{
				cursor.setPosition(reverseLocationIter->first, QTextCursor::MoveAnchor);
				cursor.setPosition(reverseLocationIter->second, QTextCursor::KeepAnchor);
			}
			else
			{
				cursor.setPosition(m_foundItems.rbegin()->first, QTextCursor::MoveAnchor);
				cursor.setPosition(m_foundItems.rbegin()->second, QTextCursor::KeepAnchor);
			}
		}

		this->setTextCursor(cursor);
	}
}

void ScriptTextEditor::replaceNext(const QRegExp& rx, const QString& text)
{
	QMutexLocker lock(&m_selectionMutex);
	if (m_foundItems.size() == 0)
		return;

	QTextCursor cursor = textCursor();

	int startPos = cursor.selectionStart();
	std::map<int, int>::iterator locationIter = m_foundItems.lower_bound(startPos);

	if (locationIter == m_foundItems.end())
		locationIter = m_foundItems.begin();

	cursor.beginEditBlock();
	cursor.setPosition(locationIter->first, QTextCursor::MoveAnchor);
	cursor.setPosition(locationIter->second, QTextCursor::KeepAnchor);
	QString updatedString = cursor.selectedText().replace(rx, text);
	cursor.removeSelectedText();
	cursor.insertText(updatedString);
	cursor.endEditBlock();
	
	moveNext(true);
}

void ScriptTextEditor::replaceAll(const QRegExp& rx, const QString& text)
{
	QMutexLocker lock(&m_selectionMutex);
	if (m_foundItems.size() == 0)
		return;

	QTextCursor cursor = textCursor();

	cursor.beginEditBlock();
	blockSignals(true);
	
	
	for (std::map<int, int>::reverse_iterator iter = m_foundItems.rbegin(); iter != m_foundItems.rend(); ++iter)
	{
		cursor.setPosition(iter->first, QTextCursor::MoveAnchor);
		cursor.setPosition(iter->second, QTextCursor::KeepAnchor);
		QString updatedString = cursor.selectedText().replace(rx, text);
		cursor.removeSelectedText();
		cursor.insertText(updatedString);
	}

	blockSignals(false);
	cursor.endEditBlock();

	startFind();
}

void ScriptTextEditor::findNext(bool moveCursorToBeginningOfSelection)
{
	if (FFlag::StudioEmbeddedFindDialogEnabled)
	{
		if (moveCursorToBeginningOfSelection)
		{
			QTextCursor cursor = textCursor();
			cursor.setPosition(cursor.selectionStart());
			setTextCursor(cursor);
		}

		moveNext(true);
	}
	else
	{
		FindReplaceProvider::instance().find(true);
		if (!FindReplaceProvider::instance().getReplaceDialog()->isVisible() &&
			!FindReplaceProvider::instance().getFindDialog()->isVisible() &&
			!isActiveWindow() )
		{
			activateWindow();
			setFocus();
		}
	}
}

void ScriptTextEditor::findPrevious()
{
	if (FFlag::StudioEmbeddedFindDialogEnabled)
	{
		moveNext(false);
	}
	else
	{
		FindReplaceProvider::instance().find(false);
		if ( !FindReplaceProvider::instance().getReplaceDialog()->isVisible() &&
			!FindReplaceProvider::instance().getFindDialog()->isVisible() &&
			!isActiveWindow() )
		{
			activateWindow();
			setFocus();
		}
	}
}

void ScriptTextEditor::replace()
{
	if (FFlag::StudioEmbeddedFindDialogEnabled)
	{
		FindReplaceProvider::instance().getEmbeddedFindDialog()->activate(true);
	}
	else
	{
		FindReplaceProvider::instance().getFindDialog()->hide();
		FindReplaceProvider::instance().getReplaceDialog()->show();
		FindReplaceProvider::instance().getReplaceDialog()->activateWindow();
		FindReplaceProvider::instance().getReplaceDialog()->setFocus();
	}
}

bool ScriptTextEditor::doHandleAction(const QString& actionID)
{
    bool handled = true;

    QString ignoredActions = "undoAction redoAction copyAction cutAction pasteAction ";
    if (!FFlag::StudioSeparateActionByActivationMethod && ignoredActions.contains(actionID) )
        return handled;

	if (actionID == "undoAction")
		undo();
	else if (actionID == "redoAction")
		redo();
	else if (actionID == "copyAction")
		copy();
	else if (actionID == "cutAction")
		cut();
	else if (actionID == "pasteAction")
		paste();
    else if ( actionID == "findAction" )
        find();
    else if ( actionID == "replaceAction" )
        replace();
	else if ( actionID == "findNextAction" )
        findNext();
    else if ( actionID == "findPreviousAction" )
        findPrevious();
    else if ( actionID == "goToScriptErrorAction" )
        goToError();
	else if ( actionID == "goToLineAction" )
		goToLine();
    else if ( actionID == "commentSelectionAction" )
        commentSelection();
    else if ( actionID == "uncommentSelectionAction" )
        uncommentSelection();
    else if ( actionID == "toggleCommentAction" )
        toggleCommentSelection();
    else if ( actionID == "expandAllFoldsAction" )
        expandAllFolds();
    else if ( actionID == "collapseAllFoldsAction" )
        collapseAllFolds();
    else if ( actionID == "deleteSelectedAction" )
        textCursor().removeSelectedText();
    else if ( actionID == "selectAllAction" )
        selectAll();
    else if ( actionID == "zoomInAction" )
        zoom(1);
    else if ( actionID == "zoomOutAction" )
        zoom(-1);
    else if ( actionID == "resetScriptZoomAction" )
#ifdef Q_WS_MAC
        zoom(14, true);
#else
        zoom(10, true);
#endif
    else
        handled = false;

    return handled;
}

bool ScriptTextEditor::isPasteEnabled()
{
    QClipboard      *pClipboard = QApplication::clipboard();
    if (pClipboard)
    {
        const QMimeData *pMimeData  = pClipboard->mimeData();
        if (pMimeData && pMimeData->hasFormat("text/plain"))
            return true;
    }
    return false;
}

bool ScriptTextEditor::actionState(const QString& actionID,bool& enableState,bool& checkedState)
{
	if (isReadOnly())
	{
        // disable actions which can edit script code
		static QString editableActionList = "undoAction redoAction pasteAction cutAction copyAction commentSelectionAction uncommentSelectionAction "
			                                "deleteSelectedAction selectAllAction toggleCommentAction "
							                "simulationRunAction simulationPlayAction simulationStopAction simulationResetAction ";

		if (editableActionList.contains(actionID))
		{
			enableState = false;
			checkedState = false;
			return true;
		}
	}

	static QString disableActionList = "groupSelectionAction ungroupSelectionAction pasteIntoAction";
    if (disableActionList.contains(actionID))
    {
        enableState = false;
        checkedState = false;
        return true;
    }

    if ( actionID == "findNextAction" || actionID == "findPreviousAction" )
		enableState = FFlag::StudioEmbeddedFindDialogEnabled || FindReplaceProvider::instance().isFindNextEnabled();
    else if ( actionID == "goToScriptErrorAction" )
        enableState = m_errorLine >= 0;
    else if ( actionID == "undoAction" )
        enableState = document()->isUndoAvailable();
    else if ( actionID == "redoAction" )
        enableState = document()->isRedoAvailable();
    else if ( actionID == "pasteAction" )
        enableState = isPasteEnabled();
    else if ( actionID == "selectAllAction" )
        enableState = true;
    else if ( actionID == "zoomInAction" )
        enableState = AuthoringSettings::singleton().editorFont.pointSize() < MaxFontZoomIn;    
    else if ( actionID == "zoomOutAction" )
        enableState = AuthoringSettings::singleton().editorFont.pointSize() > MaxFontZoomOut;
    else
    {
        static QString requireSelectionActions =
            "cutAction copyAction commentSelectionAction uncommentSelectionAction deleteSelectedAction";
	    if ( requireSelectionActions.contains(actionID) )
		    enableState = textCursor().hasSelection();
        else
            return false;
    }

    return true;
}

void ScriptTextEditor::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
{
    bool updateText = false;

    // if there is a color change, update the syntax colors
    if ( !pDescriptor || pDescriptor->category.str == sScriptColorsCategoryName )
    {
        updateText = true;

		// update viewport palette before setting stylesheet or else cursor color will not be changed
		QPalette pt = viewport()->palette();
		pt.setColor(QPalette::Text, QtUtilities::toQColor(AuthoringSettings::singleton().editorTextColor));
		viewport()->setPalette(pt);

        m_pSyntaxHighlighter->onPropertyChanged(pDescriptor);
        setStyleSheet(constructStyleSheet());
		// update error format background color
		QColor errorColor = QtUtilities::toQColor(AuthoringSettings::singleton().editorErrorColor);

		QColor warningColor = QtUtilities::toQColor(AuthoringSettings::singleton().editorWarningColor);
		// first update selection
		QList<QTextEdit::ExtraSelection> currentSelections = extraSelections();
		for (int ii = 0; ii < currentSelections.size(); ++ii)
		{
			if (currentSelections[ii].format == m_ErrorFormat)
			{
				currentSelections[ii].format.setUnderlineColor(errorColor);
			}
			else if (currentSelections[ii].format == m_WarningFormat)
			{
				currentSelections[ii].format.setUnderlineColor(warningColor);
			}
		}
		setExtraSelections(currentSelections);
		// now update member variables

		m_ErrorFormat.setUnderlineColor(errorColor);

		m_WarningFormat.setUnderlineColor(warningColor);
    }

    if ( !pDescriptor || pDescriptor->category.str == sScriptCategoryName )
    {
        m_pSideWidget->onPropertyChanged(pDescriptor);

        updateText = true;

        // set the new font, make sure it's fixed pitch
        QFont font = AuthoringSettings::singleton().editorFont;
        font.setFixedPitch(true);
        font.setUnderline(false);
        font.setStrikeOut(false);
        font.setBold(false);
        setFont(font);

        m_pSideWidget->onPropertyChanged(pDescriptor);

		// adjust size of tabs based on font width and size of tab stop
        const QFontMetrics metrics(font);
        const int spaceWidth = metrics.width(' ');
        setTabStopWidth(AuthoringSettings::singleton().editorTabWidth * spaceWidth);

        if ( AuthoringSettings::singleton().editorTextWrap )
            setLineWrapMode(QPlainTextEdit::WidgetWidth);
        else
            setLineWrapMode(QPlainTextEdit::NoWrap);

		// adjust error text color
        m_ErrorFormat.setFont(font);
		m_ErrorFormat.setProperty(QTextFormat::FullWidthSelection,true);
		m_ErrorFormat.setFontUnderline(true);
		m_ErrorFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

		// adjust warning text
		m_WarningFormat.setFont(font);
		m_WarningFormat.setProperty(QTextFormat::FullWidthSelection,true);
		m_WarningFormat.setFontUnderline(true);
		m_WarningFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    }

    // clear and reset the text to change colors and formating
    if ( updateText )
    {
        m_pSyntaxHighlighter->setFont(font());
        m_pSyntaxHighlighter->rehighlight();
    }
}

/**
 * Checks if a character is part of a valid word.
 *  Characters that are not alpha numeric or an underscore break up words.
 *  
 * @param   c   character to check
 * @return  true if the character is part of a word
 */
bool ScriptTextEditor::isWordCharacter(const QChar& c)
{
    if ( c == '_' || c.isLetterOrNumber() )
        return true;
    else
        return false;
}

/**
 * Finds a regular expression in a document text block.
 * 
 * @param   block       block to search
 * @param   expression  regular expression to search for
 * @param   offset      offset in block
 * @param   option      search options
 * @param   cursor      output cursor
 * @return  true if found
 */
bool ScriptTextEditor::findInBlock(
    const QTextBlock&           block, 
    const QRegExp&              expression, 
    int                         offset,
    QTextDocument::FindFlags    options, 
    QTextCursor&                cursor )
{
    const QRegExp expr(expression);
    QString text = block.text();
    text.replace(QChar::Nbsp,QLatin1Char(' '));

    int idx = -1;
    while ( offset >=0 && offset <= text.length() ) 
    {
        // move to next character index
        idx = (options & QTextDocument::FindBackward) ? expr.lastIndexIn(text,offset) : expr.indexIn(text,offset);
        if ( idx == -1 )
            return false;

        if ( options & QTextDocument::FindWholeWords ) 
        {
            // check for word breaks
            const int start = idx;
            const int end = start + expr.matchedLength();
            if ( (start != 0 && isWordCharacter(text.at(start - 1))) || 
                 (end != text.length() && isWordCharacter(text.at(end))) )
            {
                //if this is not a whole word, continue the search in the string
                offset = (options & QTextDocument::FindBackward) ? idx - 1 : end + 1;
                idx = -1;
                continue;
            }
        }

        //we have a hit, return the cursor for that.
        break;
    }

    if ( idx == -1 )
        return false;

    // generate output cursor
    cursor = QTextCursor(block.docHandle(),block.position() + idx);
    cursor.setPosition(cursor.position() + expr.matchedLength(),QTextCursor::KeepAnchor);
    return true;
}

/**
 * Finds a regular expression in a text document.
 * 
 * @param   doc     document to search
 * @param   expr    regular expression
 * @param   from    starting cursor location
 * @param   option  search options
 * @return  cursor if found
 */
QTextCursor ScriptTextEditor::findInDoc(
    const QTextDocument&        doc,
    const QRegExp&              expr,
    const QTextCursor&          from,
    QTextDocument::FindFlags    options )
{
    if ( expr.isEmpty() )
        return QTextCursor();

    int pos = 0;
    if ( !from.isNull() ) 
    {
        // skip over current selection
        if ( options & QTextDocument::FindBackward )
            pos = from.selectionStart();
        else
            pos = from.selectionEnd();
    }

    // the cursor is positioned between characters, so for a backward search
    //  do not include the character given in the position.
    if ( options & QTextDocument::FindBackward ) 
    {
        --pos ;
        if ( pos < 0 )
            return QTextCursor();
    }

    QTextCursor cursor;
    QTextBlock block = doc.findBlock(pos);

    if ( options & QTextDocument::FindBackward )
    {
        // search backwards
        int blockOffset = pos - block.position();
        while ( block.isValid() )
        {
            if ( findInBlock(block,expr,blockOffset,options,cursor) )
                return cursor;
            block = block.previous();
            blockOffset = block.length() - 1;
        }
    } 
    else
    {
        // search forwards
        int blockOffset = qMax(0, pos - block.position());
        while ( block.isValid() ) 
        {
            if ( findInBlock(block,expr,blockOffset,options,cursor) )
                return cursor;
            blockOffset = 0;
            block = block.next();
        }
    }

    return QTextCursor();
}

bool ScriptTextEditor::find(bool loop,const QRegExp& regExp,QTextDocument::FindFlags flags)
{
	if (FFlag::StudioEmbeddedFindDialogEnabled)
	{
		clearFoundItems();
		m_pFindThread->setAutoFindNext(true);
		m_pFindThread->setData(regExp, flags);
		m_pFindThread->requestRun();

		return false;
	}
	else
	{
		QTextCursor cursor = findInDoc(*document(),regExp,textCursor(),flags);
		if ( cursor.isNull() && loop )
		{
			cursor = textCursor();
			if ( flags & QTextDocument::FindBackward )
			{
				UpdateUIManager::Instance().getMainWindow().statusBar()->showMessage("Search wrapped to end",2000);
				cursor.movePosition(QTextCursor::End);
			}
			else
			{
				UpdateUIManager::Instance().getMainWindow().statusBar()->showMessage("Search wrapped to start",2000);
				cursor.movePosition(QTextCursor::Start);            
			}

			cursor = findInDoc(*document(),regExp,cursor,flags);
		}

		if ( !cursor.isNull() )
		{
			setTextCursor(cursor);
			ensureVisible(cursor.block());
		}

		return !cursor.isNull();
	}
}

void ScriptTextEditor::replace(const QString& text,const QRegExp& regExp,QTextDocument::FindFlags flags)
{
    textCursor().insertText(text);
}

QString ScriptTextEditor::getSelectedText()
{
    return textCursor().selectedText();
}

bool ScriptTextEditor::hasSelection()
{
    return textCursor().hasSelection();
}

void ScriptTextEditor::goToStart(QTextDocument::FindFlags flags)
{
    QTextCursor cursor = textCursor();

	// The start of the search will be the end of the document when we
	// are searching backwards.
    if ( flags & QTextDocument::FindBackward )
    {
        cursor.movePosition(QTextCursor::End);
    }
    else
    {
        cursor.movePosition(QTextCursor::Start);            
    }

    setTextCursor(cursor);
}

int ScriptTextEditor::getDocFoldState(int blockNumber)
{
    int foldCount = 0;
    QTextBlock iterBlock = document()->findBlockByLineNumber(0);

    if (!blockNumber)
        blockNumber = document()->blockCount();

    while (iterBlock.isValid() && iterBlock.blockNumber() <= blockNumber)
    {
        RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(iterBlock.userData());
        if (pUserData)
            foldCount += pUserData->getFoldState();
        iterBlock = iterBlock.next();
    }
    return foldCount;
}

QString ScriptTextEditor::constructStyleSheet()
{
	QString styleSheet;

	// set style sheet so we can change the text color, background color,
	//  selected text color, and selected background color
	styleSheet = QString(STRINGIFY(
							QPlainTextEdit
							{
							color: %1;
							background: %2;
							selection-color: %3;
							selection-background-color: %4;
							} )).
							arg(QtUtilities::toQColor(AuthoringSettings::singleton().editorTextColor).name()).
							arg(QtUtilities::toQColor(AuthoringSettings::singleton().editorBackgroundColor).name()).
							arg(QtUtilities::toQColor(AuthoringSettings::singleton().editorSelectionColor).name()).
							arg(QtUtilities::toQColor(AuthoringSettings::singleton().editorSelectionBackgroundColor).name());

	return styleSheet;
}

QString ScriptTextEditor::getVariableAtPos(const QPoint& globalPos)
{
	QPoint pos = viewport()->mapFromGlobal(globalPos);
	QTextCursor cursor = cursorForPosition(pos);
	if (cursor.block().isValid())
	{
		// get the cursor position before selecting the block text
		int cursorPos = cursor.position() - cursor.block().position();
		// select the entire block
		cursor.select(QTextCursor::BlockUnderCursor);
		// get block rect
		QRect blockRect = cursorRect(cursor); 
		blockRect.setLeft(0);
		// if mouse position is inside the block rect then only proceed
		if (blockRect.contains(pos))
		{				
			// get block text under the cursor
			QString blockText = cursor.selectedText();
			if (cursorPos > 0 && cursorPos < blockText.size() && blockText.at(cursorPos).isLetterOrNumber())
			{
				int startPos = variableStartPos(blockText, cursorPos);
				int endPos   = variableEndPos(blockText, cursorPos);
				if (endPos > startPos)
					return blockText.mid(startPos, endPos - startPos);
			}
		}
	}

	return QString();
}

int ScriptTextEditor::variableStartPos(const QString &text, int startPos)
{
	// variable in Lua can have letter, number, underscore and member variables can be accessed via dot
    for ( ; startPos >= 0; startPos--) 
	{
		int tempPos = startPos;
		if (startPos > 0 && text.at(startPos) == '.') 
			tempPos = startPos-1;

		if (!isWordCharacter(text.at(tempPos)))
			return startPos+1;
    }

    return 0;
}

int ScriptTextEditor::variableEndPos(const QString &text, int endPos)
{
	for ( ; endPos < text.size(); endPos++)
	{
		if (!isWordCharacter(text.at(endPos)))
			return endPos;
	}

	return text.size();
}

void ScriptTextEditor::moveCursorToLine(int line)
{
	QTextCursor cursor = textCursor();
	if (document()->blockCount() < line)
		return;

	int position = document()->findBlockByNumber(line).position();

	cursor.setPosition(position);

	setTextCursor(cursor);
	ensureVisible(cursor.block());
}

void ScriptTextEditor::selectText(int startPosition, int endPosition)
{
	QTextCursor cursor = textCursor();
	cursor.setPosition(startPosition);
	cursor.setPosition(endPosition, QTextCursor::KeepAnchor);
	setTextCursor(cursor);
	ensureVisible(cursor.block());
}

void ScriptTextEditor::selectText(const RBX::ScriptAnalyzer::Location& location, bool trySettingBlocksVisible)
{
	int startPos = getStartPosition(location);
	int endPos   = getEndPosition(location);

	if (startPos >= 0 && endPos >= 0 && (endPos >= startPos))
	{
		selectText(startPos, endPos);
	}
	else if (trySettingBlocksVisible)
	{
		// if we couldn't get valid start and end pos, then try to unfold blocks
		QTextBlock startBlock = document()->findBlockByNumber(location.begin.line);
		if (startBlock.isValid() && !startBlock.isVisible())
			ensureVisible(startBlock);
		QTextBlock endBlock = document()->findBlockByNumber(location.end.line);
		if (endBlock.isValid() && !endBlock.isVisible())
			ensureVisible(endBlock);
		// again try to select text
		selectText(location, false);
	}
	
	centerCursor();
}

bool ScriptTextEditor::addToExtraSelections(const QTextEdit::ExtraSelection& extraSelection, bool prepend)
{
	QList<QTextEdit::ExtraSelection> currentSelections = extraSelections();
	if (currentSelections.contains(extraSelection))
		return false;

	prepend ? currentSelections.push_front(extraSelection) : currentSelections.append(extraSelection);
	setExtraSelections(currentSelections);
	return true;
}

void ScriptTextEditor::removeFromExtraSelections(QList<QTextEdit::ExtraSelection> selectionsToRemove)
{
	QList<QTextEdit::ExtraSelection> currentSelections = extraSelections();
	for (int ii = 0; ii < selectionsToRemove.size(); ++ii)
		currentSelections.removeOne(selectionsToRemove.at(ii));
	setExtraSelections(currentSelections);
}

void ScriptTextEditor::updateViewAndCursor(bool resizeRequired)
{
	// if block was unfolded, then we will need to resize the viewport
	if ( resizeRequired )
	{
		viewport()->resize(viewport()->sizeHint());
		// viewport gets updated only after a draw call so delay cursor visibility
		// if we don't, then incorrect scrolling will happen
		QTimer::singleShot(0,this,SLOT(updateCursorVisibility()));
	}
	else
	{
		// directly update cursor
		updateCursorVisibility();
	}

	// initiate syntax check
	updateFolds();
}

void ScriptTextEditor::updateCursorVisibility()
{  ensureCursorVisible(); }

void ScriptTextEditor::updateWikiLookup(const char* key, std::string value)
{
	std::pair<int, std::string> lookupPair = wikiLookup[key];
	lookupPair.first++;
	if(lookupPair.first > 1) 
		lookupPair.second = "";
	else
		lookupPair.second = value;
	wikiLookup[key] = lookupPair;
}

void ScriptTextEditor::setupWikiLookup()
{
	for(RBX::Reflection::ClassDescriptor::ClassDescriptors::const_iterator iter = RBX::Reflection::ClassDescriptor::all_begin();
		iter!=RBX::Reflection::ClassDescriptor::all_end();++iter)
	{
		std::string className = ((*iter)->name).toString();
		updateWikiLookup((*iter)->name.c_str(), "API:Class/" + className);
		
		for(RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::PropertyDescriptor>::Collection::const_iterator propIter = (*iter)->begin<RBX::Reflection::PropertyDescriptor>();
			propIter!=(*iter)->end<RBX::Reflection::PropertyDescriptor>();++propIter)
			if(className.compare((*propIter)->owner.name.toString()) == 0)
				updateWikiLookup((*propIter)->name.c_str(), ("API:Class/" + className + "/" + (*propIter)->name.toString()).c_str());

		for(RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::FunctionDescriptor>::Collection::const_iterator funcIter = (*iter)->begin<RBX::Reflection::FunctionDescriptor>();
			funcIter!=(*iter)->end<RBX::Reflection::FunctionDescriptor>();++funcIter)
			if(className.compare((*funcIter)->owner.name.toString()) == 0)
				updateWikiLookup((*funcIter)->name.c_str(), ("API:Class/" + className + "/" + (*funcIter)->name.toString()).c_str());

		for(RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::YieldFunctionDescriptor>::Collection::const_iterator yfuncIter = (*iter)->begin<RBX::Reflection::YieldFunctionDescriptor>();
			yfuncIter!=(*iter)->end<RBX::Reflection::YieldFunctionDescriptor>();++yfuncIter)
			if(className.compare((*yfuncIter)->owner.name.toString()) == 0)
				updateWikiLookup((*yfuncIter)->name.c_str(), ("API:Class/" + className + "/" + (*yfuncIter)->name.toString()).c_str());

		for(RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::EventDescriptor>::Collection::const_iterator eventIter = (*iter)->begin<RBX::Reflection::EventDescriptor>();
			eventIter!=(*iter)->end<RBX::Reflection::EventDescriptor>();++eventIter)
			if(className.compare((*eventIter)->owner.name.toString()) == 0)
				updateWikiLookup((*eventIter)->name.c_str(), ("API:Class/" + className + "/" + (*eventIter)->name.toString()).c_str());

		for(RBX::Reflection::MemberDescriptorContainer<RBX::Reflection::CallbackDescriptor>::Collection::const_iterator callbackIter = (*iter)->begin<RBX::Reflection::CallbackDescriptor>();
			callbackIter!=(*iter)->end<RBX::Reflection::CallbackDescriptor>();++callbackIter)
			if(className.compare((*callbackIter)->owner.name.toString()) == 0)
				updateWikiLookup((*callbackIter)->name.c_str(), ("API:Class/" + className + "/" + (*callbackIter)->name.toString()).c_str());
	}

	for(std::vector< const RBX::Reflection::EnumDescriptor* >::const_iterator enumIter = RBX::Reflection::EnumDescriptor::enumsBegin();
		enumIter!=RBX::Reflection::EnumDescriptor::enumsEnd();++enumIter)
	{
		std::string enumName = (*enumIter)->name.toString();
		updateWikiLookup((*enumIter)->name.c_str(), "API:Enum/" + enumName);

		for(std::vector< const RBX::Reflection::EnumDescriptor::Item* >::const_iterator itemItor = (*enumIter)->begin();
			itemItor!=(*enumIter)->end(); ++itemItor)
		{
			std::string itemName = (*itemItor)->name.toString();
			updateWikiLookup((*itemItor)->name.c_str(), "API:Enum/" + enumName);
		}
	}
}

void ScriptTextEditor::selectionChangedSearch()
{
	if (textCursor().hasSelection())
	{
		QString selection = textCursor().selectedText();
		if(selection.isEmpty())
			m_pWikiSearchTimer->stop();
		else
		{
			if (!m_pWikiSearchTimer->isActive())
			{
				m_pWikiSearchTimer->start(WikiSearchTimerInterval);
				currentWikiSearchSelection = selection;
			}
			else if (QString::compare(selection, currentWikiSearchSelection) != 0)
			{
				m_pWikiSearchTimer->start(WikiSearchTimerInterval);
				currentWikiSearchSelection = selection;
			}
		}
	}	
}

void ScriptTextEditor::onSearchTimeout()
{
	if (!currentWikiSearchSelection.isEmpty())
	{
		QString searchTerm = currentWikiSearchSelection;
		std::pair<int, std::string> lookupPair = wikiLookup[searchTerm.toStdString().c_str()];

		if (lookupPair.first == 1)
			searchTerm = QString(lookupPair.second.c_str());
		else 
		{
			// Don't search for special characters!
			if (FFlag::ScriptContextSearchFix && searchTerm.contains(QRegExp("[^0-9a-zA-Z]")))
				return;

			searchTerm.prepend("Special:Search?search=");
			searchTerm.append("&fulltext=Search&studiomode=true");
		}

		Q_EMIT(helpTopicChanged(searchTerm));
	}
}

void ScriptTextEditor::populateValuesFromLocation(const RBX::ScriptAnalyzer::Location& location, int& startPos, int& endPos, bool& atEof)
{
	atEof   = (location.begin.line + 1 == (unsigned int)document()->lineCount() && (location.begin.column == location.end.column));
	if (!atEof)
	{
		startPos = getStartPosition(location);
		endPos   = getEndPosition(location);
	}
	else
	{
		QTextBlock beginBlock = document()->findBlockByLineNumber(location.begin.line);
		startPos = beginBlock.position();
		endPos   = startPos + beginBlock.length();
	}
}

int ScriptTextEditor::getStartPosition(const RBX::ScriptAnalyzer::Location& location)
{
	QTextBlock beginBlock = document()->findBlockByNumber(location.begin.line);
	if (beginBlock.isValid() && beginBlock.isVisible())
		return beginBlock.position() + location.begin.column;
	return -1;
}

int ScriptTextEditor::getEndPosition(const RBX::ScriptAnalyzer::Location& location)
{
	QTextBlock endBlock = document()->findBlockByNumber(location.end.line);
	if (endBlock.isValid() && endBlock.isVisible())
		return endBlock.position() + location.end.column;
	return -1;
}

void ScriptTextEditor::onCheckSyntaxTimerTimeOut()
{
    m_pCheckSyntaxTimer->stop();
    onCheckSyntax();
}

void ScriptTextEditor::addFoundItem(int start, int end)
{
	QMutexLocker lock(&m_selectionMutex);
	m_foundItems.insert(std::pair<int, int>(start, end));
	
	std::map<int,int>::iterator firstAfter = m_extraSelectionsToAdd.lower_bound(start);

	if (m_extraSelectionsToAdd.size() > 0 && firstAfter != m_extraSelectionsToAdd.begin())
	{
		--firstAfter;

		int previousStart = firstAfter->first;
		int previousEnd = firstAfter->second;

		if (start <= previousEnd && end >= previousStart)
		{
			start = qMin(start, previousStart);
			end = qMax(end, previousEnd);

			m_extraSelectionsToAdd.erase(firstAfter);
		}
	}

	m_extraSelectionsToAdd.insert(std::pair<int, int>(start, end));
}

void ScriptTextEditor::clearFoundItems()
{
	QMutexLocker lock(&m_selectionMutex);

	m_foundItems.clear();

	for (QList<QTextEdit::ExtraSelection>::iterator iter = m_currentFoundItems.begin(); iter != m_currentFoundItems.end(); ++iter)
		m_extraSelectionsToRemove.insert(std::pair<int, QTextEdit::ExtraSelection>(iter->cursor.selectionStart(), *iter));

	m_extraSelectionsToAdd.clear();
	m_currentFoundItems.clear();
	QApplication::postEvent(this, new RobloxCustomEvent(EXTRASELECTION_UPDATE));
}

void ScriptTextEditor::updateFindSelection()
{
	QApplication::removePostedEvents(this, EXTRASELECTION_UPDATE);

	RBX::Time timeCheck = RBX::Time::now<RBX::Time::Fast>();

	bool selectionsUpdated = false;

	QTextEdit::ExtraSelection selection;
	selection.cursor = textCursor();

	int firstVisibleBlockPosition = firstVisibleBlock().position();
	QMutexLocker lock(&m_selectionMutex);

	while ((RBX::Time::now<RBX::Time::Fast>() - timeCheck).msec() < 10)
	{
		if (m_extraSelectionsToAdd.empty())
			break;

		std::map<int, int>::iterator selectionToMake = m_extraSelectionsToAdd.lower_bound(firstVisibleBlockPosition);

		if (selectionToMake == m_extraSelectionsToAdd.end())
			selectionToMake = m_extraSelectionsToAdd.begin();

		selection.format = m_findSelectionFormat;


		selection.cursor.setPosition(selectionToMake->first, QTextCursor::MoveAnchor);
		selection.cursor.setPosition(selectionToMake->second, QTextCursor::KeepAnchor);

		m_currentSelections.append(selection);
		m_currentFoundItems.append(selection);

		m_extraSelectionsToAdd.erase(selectionToMake);
	}

	timeCheck = RBX::Time::now<RBX::Time::Fast>();

	while ((RBX::Time::now<RBX::Time::Fast>() - timeCheck).msec() < 10)
	{
		for (int i = 0; i < 100; ++i)
		{
			if (m_extraSelectionsToRemove.empty())
				break;

			std::multimap<int, QTextEdit::ExtraSelection>::iterator selectionToRemove = m_extraSelectionsToRemove.lower_bound(firstVisibleBlockPosition);

			if (selectionToRemove == m_extraSelectionsToRemove.end())
				selectionToRemove = m_extraSelectionsToRemove.begin();

			m_currentSelections.removeOne(selectionToRemove->second);

			m_extraSelectionsToRemove.erase(selectionToRemove);
		}

		setExtraSelections(m_currentSelections);

		if (m_extraSelectionsToRemove.empty())
			break;
	}
	

	if (!m_extraSelectionsToAdd.empty() || !m_extraSelectionsToRemove.empty())
		QApplication::postEvent(this, new RobloxCustomEvent(EXTRASELECTION_UPDATE));
}

/*****************************************************************************/
// ScriptTraversalThread
/*****************************************************************************/

ScriptTraversalThread::ScriptTraversalThread(ScriptTextEditor *parent, RobloxScriptDoc *document)
	:QThread(parent),
	m_pScriptEditor(parent),
	m_pDocument(document),
	restart(false),
	abort(false)
{}

ScriptTraversalThread::~ScriptTraversalThread()
{
	mutex.lock();
	abort = true;
	condition.wakeOne();
	mutex.unlock();

	wait();
}

void ScriptTraversalThread::requestRun()
{
	if (!isRunning())
	{
		start(IdlePriority);
	}
	else
	{
		restart = true;
		condition.wakeOne();
	}
}

void ScriptTraversalThread::run()
{
	while (true)
	{
		if (abort)
			return;

		doWork();

		if (!restart)
			condition.wait(&mutex);

		restart = false;
	}
}


/*****************************************************************************/
// CheckSyntaxThread
/*****************************************************************************/

void CheckSyntaxThread::doWork()
{
	shared_ptr<RBX::DataModel> dm = m_pDocument->getDataModel();

	void* resultPtr = NULL;

	{
		RBX::DataModel::LegacyLock l(dm.get(), RBX::DataModelJob::Write);

		std::string code = m_pScriptEditor->toPlainText().toStdString();
		shared_ptr<RBX::Instance> scriptInstance = m_pDocument->getCurrentScript().toInstance();
		// Linked script source won't have an instance, so use a fake one to appease the analyzer
		if (!scriptInstance)
		{
			scriptInstance = RBX::Creatable<RBX::Instance>::create<RBX::Script>();
		}

		RBX::ScriptAnalyzer::Result result = RBX::ScriptAnalyzer::analyze(dm.get(), scriptInstance, code);
		QMetaObject::invokeMethod(m_pScriptEditor, "setAnalysisResult", Qt::QueuedConnection, Q_ARG(RBX::ScriptAnalyzer::Result, result));
	}
}

/*****************************************************************************/
// FindThread
/*****************************************************************************/

void FindThread::setData(const QRegExp& regExp,QTextDocument::FindFlags flags)
{
	restart = true;

	toSearch = regExp;
	searchFlags = flags;
}

bool FindThread::findInBlockNoCursor(
	const QString&				text, 
	int							startOfBlock,
	const QRegExp&              expression, 
	int                         offset,
	QTextDocument::FindFlags    options, 
	int&						start,
	int&						end)

{
	const QRegExp expr(expression);
	
	int idx = -1;
	while ( offset >=0 && offset <= text.length() ) 
	{
		// move to next character index
		idx = (options & QTextDocument::FindBackward) ? expr.lastIndexIn(text,offset) : expr.indexIn(text,offset);
		if ( idx == -1 )
			return false;

		if ( options & QTextDocument::FindWholeWords ) 
		{
			// check for word breaks
			const int start = idx;
			const int end = start + expr.matchedLength();
			if ( (start != 0 && ScriptTextEditor::isWordCharacter(text.at(start - 1))) || 
				(end != text.length() && ScriptTextEditor::isWordCharacter(text.at(end))) )
			{
				//if this is not a whole word, continue the search in the string
				offset = end + 1;
				idx = -1;
				continue;
			}
		}

		//we have a hit, return the cursor for that.
		break;
	}

	if ( idx == -1 )
		return false;

	// generate output cursor
	start = startOfBlock + idx;
	end = start + expr.matchedLength();
	return true;
}

void FindThread::doWork()
{
	m_pScriptEditor->clearFoundItems();

	if (toSearch.isEmpty())
	{
		QMetaObject::invokeMethod(FindReplaceProvider::instance().getEmbeddedFindDialog(), "setNumberOfFoundResults", Q_ARG(int, -1));
		QMetaObject::invokeMethod(m_pScriptEditor, "onCheckSyntax");
		return;
	}

	shared_ptr<RBX::DataModel> dm = m_pDocument->getDataModel();

	RBX::DataModel::LegacyLock lock(dm.get(), RBX::DataModelJob::Write);

	QTextBlock block;

	int offset = 0;
	block = m_pScriptEditor->firstVisibleBlock();
	m_searchStartBlockLine = block.blockNumber();
	m_searchLastBlockLine = -1;

	int start, end;
	QString blockText = block.text();
	int blockStart = block.position();

	RBX::Time timeCheck = RBX::Time::now<RBX::Time::Fast>();

	bool hasFoundItem = false;

	while (m_searchStartBlockLine != m_searchLastBlockLine)
	{
		{
			QMutexLocker lock(&mutex);
			if (restart || abort)
				return;
		}

		if (findInBlockNoCursor(blockText, blockStart, toSearch, offset, searchFlags, start, end))
		{
			offset = start - block.position() + 1;
			m_pScriptEditor->addFoundItem(start, end);
			hasFoundItem = true;
		}
		else
		{
			offset = 0;

			blockStart = blockStart + block.length();

			block = block.next();
			if (!block.isValid())
			{
				block = m_pScriptEditor->document()->firstBlock();
				blockStart = 0;
			}

			blockText = block.text();
			blockText.replace(QChar::Nbsp,QLatin1Char(' '));
			
			m_searchLastBlockLine = block.blockNumber();
		}

		if (hasFoundItem && (RBX::Time::now<RBX::Time::Fast>() - timeCheck).msec() > 20)
		{
			timeCheck = RBX::Time::now<RBX::Time::Fast>();
			hasFoundItem = false;
			QApplication::postEvent(m_pScriptEditor, new RobloxCustomEvent(EXTRASELECTION_UPDATE));
		}
	}

	if (hasFoundItem)
		QApplication::postEvent(m_pScriptEditor, new RobloxCustomEvent(EXTRASELECTION_UPDATE));
	
	QMetaObject::invokeMethod(FindReplaceProvider::instance().getEmbeddedFindDialog(), "setNumberOfFoundResults", Q_ARG(int, m_pScriptEditor->numberOfFoundItems()));

	QMetaObject::invokeMethod(m_pScriptEditor, "onCheckSyntax");
	if (FindReplaceProvider::instance().getEmbeddedFindDialog()->hasFocus())
		QMetaObject::invokeMethod(m_pScriptEditor, "findNext", Q_ARG(bool, true));
}

/*****************************************************************************/
// ScriptEditorUtils
/*****************************************************************************/

QTextBlock ScriptEditorUtils::findFoldBoundary(
    QTextBlock endBlock,
    bool       bUp)
{
    QTextBlock blockFound = endBlock;
    int        nFolds     = 0;

    while (blockFound.isValid())
    {
        RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(blockFound.userData());
        if (pUserData)
            nFolds += pUserData->getFoldState();

        if (bUp)
        {
            if (nFolds >= 0)
                break;

            blockFound = blockFound.previous();
        }
        else
        {
            if (nFolds <= 0)
                break;

            blockFound = blockFound.next();
        }
    }

    return blockFound;
}

int ScriptEditorUtils::noOfStartingTabs(QTextBlock block)
{
    if (!block.isValid())
        return -1;

    int nTabs = 0;
    QString text = block.text();
    while (text.mid(nTabs,1) == "\t")
    {
        nTabs++;
    }

    return nTabs;
}
