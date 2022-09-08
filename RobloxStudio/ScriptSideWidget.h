/**
 * ScriptSideWidget.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QWidget>
#include <QList>
#include <QStyle>

// Roblox Headers
#include "reflection/Property.h"

class ScriptTextEditor;

class QTextBlock;
class QRegExp;
class QFont;

class ScriptSideWidget : public QWidget
{
    Q_OBJECT

public:

    ScriptSideWidget(ScriptTextEditor* editor);
    virtual ~ScriptSideWidget();

    // Toggles block fold i.e. if a block is folded then it will be unfolded and vice versa
    void toggleFold(int lineNumber, bool updateView = true);
    void unfold(int lineNumber, bool updateView = true);

	void toggleAllFolds(bool expand);

	bool isFolded(const QTextBlock& block);

    void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor);

Q_SIGNALS:
	void toggleBreakpoint(int lineNumber);

public Q_SLOTS:

    void updateArea();
	void updateDebugArea();

protected:

    virtual void paintEvent(QPaintEvent* evt);
    virtual void mousePressEvent(QMouseEvent* evt);

private:

    void fold(int lineNumber, bool updateView = true);
    bool isFolded(int lineNumber);
    bool isFoldable(QTextBlock& block);
    QTextBlock findFoldClosing(QTextBlock& block);
    
    ScriptTextEditor* m_pScriptEditor;
    QRegExp           m_startFoldExp;
    QRegExp           m_endFoldExp;
    QFont             m_font;
    int               m_foldArea;
	int               m_debugArea;

    QList<int> m_FoldedBlocks;
};