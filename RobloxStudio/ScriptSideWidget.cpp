/**
 * ScriptSideWidget.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "ScriptSideWidget.h"

// Qt Headers
#include <QPainter>
#include <QApplication>
#include <QStyleOption>
#include <QPixmapCache>

// Roblox Studio Headers
#include "RobloxScriptDoc.h"
#include "ScriptTextEditor.h"
#include "AuthoringSettings.h"
#include "QtUtilities.h"

FASTFLAG(LuaDebugger)

ScriptSideWidget::ScriptSideWidget(ScriptTextEditor* editor)
    : QWidget(editor),
      m_pScriptEditor(editor),
      m_foldArea(QtUtilities::BranchIndicatorSize),
	  m_debugArea(0)
{
}

ScriptSideWidget::~ScriptSideWidget()
{
}

void ScriptSideWidget::updateArea()
{
    int digits = 1;
    int max    = qMax(1,m_pScriptEditor->blockCount());

    while (max >= 10)
    {
        max /= 10;
        ++digits;
    }

    QFontMetrics fm(m_font);
    int newWidth = fm.width(QLatin1Char('9')) * digits + 10 + (FFlag::LuaDebugger ? (m_debugArea + m_foldArea) : m_foldArea);;
    if (width() != newWidth)
    {
        setFixedWidth(newWidth);
        m_pScriptEditor->setViewportMargins(newWidth,0,0,0);
    }
    update();
}

void ScriptSideWidget::updateDebugArea()
{
	// first remove saved pixmap
	QPixmapCache::remove(QString("BreakpointState1"));
	QPixmapCache::remove(QString("BreakpointState2"));
	// update debug area
	m_debugArea = m_pScriptEditor->fontMetrics().height();
}

void ScriptSideWidget::toggleFold(int lineNumber, bool updateView)
{
    if (isFolded(lineNumber))
        unfold(lineNumber, updateView);
    else
        fold(lineNumber, updateView);
}

void ScriptSideWidget::fold(int lineNumber, bool updateView)
{
    QTextBlock startBlock = m_pScriptEditor->document()->findBlockByNumber(lineNumber - 1);

    if (!isFoldable(startBlock) || isFolded(lineNumber))
        return;

    QTextBlock endBlock = ScriptEditorUtils::findFoldBoundary(startBlock,false);

    QTextBlock block = startBlock.next();
    while (block.isValid() && (block.blockNumber() <= endBlock.blockNumber()))
    {
        block.setVisible(false);
        block.setLineCount(0);
        block = block.next();
    }

    m_FoldedBlocks << startBlock.blockNumber();

    QTextBlock closingBlock = endBlock;
    if (endBlock != m_pScriptEditor->document()->lastBlock())
        closingBlock = endBlock.next();

    QTextCursor cursor = m_pScriptEditor->textCursor();
    int blockNumber = cursor.block().blockNumber();
    if ( blockNumber > startBlock.blockNumber() && blockNumber <= endBlock.blockNumber() )
    {
        cursor.setPosition(endBlock.position() + endBlock.length(),QTextCursor::MoveAnchor);
        m_pScriptEditor->setTextCursor(cursor);
    }

	if (updateView)
	{
		m_pScriptEditor->document()->markContentsDirty(startBlock.position(),closingBlock.position() + closingBlock.length());
		m_pScriptEditor->updateFolds();
		m_pScriptEditor->viewport()->resize(m_pScriptEditor->viewport()->sizeHint());
		update();
	}
}

void ScriptSideWidget::unfold(int lineNumber, bool updateView)
{
    QTextBlock startBlock = m_pScriptEditor->document()->findBlockByNumber(lineNumber - 1);

    if (!isFoldable(startBlock))
        return;

    QTextBlock endBlock = ScriptEditorUtils::findFoldBoundary(startBlock,false);

    QTextBlock block = startBlock.next();
    while (block.isValid() && (block.blockNumber() <= endBlock.blockNumber()))
    {
        block.setVisible(true);
        block.setLineCount(block.layout()->lineCount());

        if (m_FoldedBlocks.contains(block.blockNumber()))
            block = ScriptEditorUtils::findFoldBoundary(block,false);

        block = block.next();
    }

    m_FoldedBlocks.removeOne(startBlock.blockNumber());

	if (updateView)
	{
		QTextBlock closingBlock = endBlock;
		if (endBlock != m_pScriptEditor->document()->lastBlock())
			closingBlock = endBlock.next();

		m_pScriptEditor->document()->markContentsDirty(startBlock.position(),closingBlock.position() + closingBlock.length());
		m_pScriptEditor->updateFolds();
		m_pScriptEditor->viewport()->resize(m_pScriptEditor->viewport()->sizeHint());
		update();
	}
}

bool ScriptSideWidget::isFolded(int lineNumber)
{
    QTextBlock block = m_pScriptEditor->document()->findBlockByNumber(lineNumber);

    if (!block.isValid())
        return false;
    return !block.isVisible();
}

bool ScriptSideWidget::isFolded(const QTextBlock& block)
{
	if (block.isValid() && m_FoldedBlocks.contains(block.blockNumber()))
		return true;
	return false;
}

bool ScriptSideWidget::isFoldable(QTextBlock& block)
{
    if (!block.isValid())
        return false;

    bool             bFold     = false;
    RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(block.userData());
    if (pUserData)
        bFold = (pUserData->getFoldState() > 0);

    return bFold;
}

void ScriptSideWidget::toggleAllFolds(bool expand)
{
    QTextBlock currentBlock = expand ? m_pScriptEditor->document()->begin() : m_pScriptEditor->document()->end().previous();
	while (currentBlock.isValid())
	{
		if (isFoldable(currentBlock))
			expand ? unfold(currentBlock.blockNumber()+1) : fold(currentBlock.blockNumber()+1);
        currentBlock = expand ? currentBlock.next() : currentBlock.previous();
	}
}

void ScriptSideWidget::paintEvent(QPaintEvent* evt)
{
	QPainter painter(this);

    painter.fillRect(rect(),QColor(Qt::lightGray).lighter(115));
    painter.setFont(m_font);

    const QFontMetrics fm(m_font);

    const int        pageBottom     = m_pScriptEditor->viewport()->height();    
    const QTextBlock currentBlock   = m_pScriptEditor->document()->findBlock(m_pScriptEditor->textCursor().position());
    const QPointF    viewportOffset = m_pScriptEditor->contentOffset();
    const int        xofs           = width() - m_foldArea;

    QTextBlock block = m_pScriptEditor->firstVisibleBlock();
    int lineCount = block.blockNumber();

    while ( block.isValid() )
    {
        lineCount += 1;

        // The top left position of the block in the document
        QPointF position = m_pScriptEditor->blockBoundingGeometry(block).topLeft() + viewportOffset;
        
        // Check if the position of the block is outside of the visible area
        if (position.y() > pageBottom)
            break;

        // We want the line number for the selected line to be bold.
        bool isBold = false;
        if (block == currentBlock)
        {
            isBold = true;
            QFont font = painter.font();
            font.setBold(true);
            painter.setFont(font);
        }

        // Draw the line number right justified at the y position of the
        // line. 3 is a magic padding number. drawText(x, y, text).
        if (block.isVisible())
        {
            const QString strNumber = QString::number(lineCount);
			const int x = width() - (FFlag::LuaDebugger ? (m_debugArea + m_foldArea) : m_foldArea) - fm.width(strNumber) - 3;
            const int y = position.y() + fm.ascent();
            painter.drawText(x,y,strNumber);
        }

        // Remove the bold style if it was set previously.
        if (isBold)
        {
            QFont font = painter.font();
            font.setBold(false);
            painter.setFont(font);
        }

        block = block.next();
    }

	// Breakpoints region
	int xofsB = width() - (m_debugArea  + m_foldArea);
	if (FFlag::LuaDebugger)
		painter.fillRect(xofsB, 0, m_debugArea, height(),QColor(Qt::lightGray).lighter(120));

    // Code Folding
    painter.fillRect(xofs,0,m_foldArea,height(),QColor(Qt::lightGray).lighter(130));
    block = m_pScriptEditor->firstVisibleBlock();
    while ( block.isValid() )
    {
        // Check if the position of the block is outside of the visible area
        const QPointF position = m_pScriptEditor->blockBoundingGeometry(block).topLeft() + viewportOffset;        
        if ( position.y() > pageBottom )
            break;

        QString blockText = block.text();
        if (block.isVisible())
        {
			if (FFlag::LuaDebugger)
			{
				RBXTextUserData* pUserData = dynamic_cast<RBXTextUserData*>(block.userData());
				if(pUserData)
				{
					if (pUserData->getBreakpointState())
						painter.drawPixmap(xofsB+1, qRound(position.y())-1, QtUtilities::getBreakpointPixmap(pUserData->getBreakpointState(), m_debugArea-2));

					if (!pUserData->getMarker().isEmpty())
					{
						QPixmap marker(pUserData->getMarker());
						painter.drawPixmap(xofsB+1, 
							               qRound(position.y())-2, 
										   marker.scaled(m_debugArea-2, 
										                 m_debugArea-2, 
														 Qt::KeepAspectRatio, 
														 Qt::SmoothTransformation));
					}
				}
			}

			if (isFoldable(block))
			{
				if (m_FoldedBlocks.contains(block.blockNumber()))
					painter.drawPixmap(xofs,qRound(position.y()),QtUtilities::getBranchIndicator(QStyle::State_Children));
				else
					painter.drawPixmap(xofs,qRound(position.y()),QtUtilities::getBranchIndicator(QStyle::State_Children | QStyle::State_Open));
			}
        }

        block = block.next();
    }

    painter.end();

    QWidget::paintEvent(evt);
}

void ScriptSideWidget::mousePressEvent(QMouseEvent* evt)
{
    if (m_foldArea > 0)
    {
        QFontMetrics fm(m_font);

        const int xofs = FFlag::LuaDebugger ? 0 : (width() - m_foldArea);
        const int fh   = fm.lineSpacing();
        const int ys   = evt->posF().y();

        int lineNumber  = 0;

        if (evt->pos().x() > xofs)
        {
            QTextBlock block                = m_pScriptEditor->firstVisibleBlock();
            const QPointF    viewportOffset = m_pScriptEditor->contentOffset();
            const int        pageBottom     = m_pScriptEditor->viewport()->height();
            
            while (block.isValid())
            {
                const QPointF position = m_pScriptEditor->blockBoundingGeometry(block).topLeft() + viewportOffset;
                if (position.y() > pageBottom)
                    break;

                if ( (position.y() < ys) && ((position.y() + fh) > ys) )
                {
					int xofF = width() - m_foldArea;
					if ( FFlag::LuaDebugger && (evt->pos().x() > xofs) && (evt->pos().x() < xofF) )
					{
						Q_EMIT toggleBreakpoint(block.blockNumber());
						update();
						break;
					}
					else if (isFoldable(block))
					{
						lineNumber = block.blockNumber() + 1;
						break;
					}
                }

                block = block.next();
            }
        }

        if (lineNumber > 0)
            toggleFold(lineNumber);
    }
}

void ScriptSideWidget::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
{
	if ( !pDescriptor || pDescriptor->category.str == sScriptCategoryName )
    {
        m_font = AuthoringSettings::singleton().editorFont;
        m_font.setFixedPitch(true);
        m_font.setStrikeOut(false);
        m_font.setUnderline(false);
        m_font.setBold(false);
        updateArea();
		if (FFlag::LuaDebugger)
			updateDebugArea();
    }
}
