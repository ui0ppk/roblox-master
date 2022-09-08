/**
 * ScriptCommandTool.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

// Qt Headers
#include <QComboBox>
#include <QListWidgetItem>

class ScriptComboBox : public QComboBox
{
Q_OBJECT
  
public:
    ScriptComboBox(QWidget* parent);

    void setCommandHistory(const QStringList& commandHistory);
    const QStringList& commandHistory() { return m_CommandHistory; }

public Q_SLOTS:
    void intellesenseDoubleClick(QListWidgetItem* listItem);

protected:
    virtual bool event(QEvent *e);

    virtual void focusInEvent(QFocusEvent *event);
    virtual void focusOutEvent(QFocusEvent *event);

private Q_SLOTS:
    void onScriptInputReturnPressed();
	void onEditorTextEdit(const QString& text);
	void onItemSelectionFromView();

private:

    void applyPlaceHolderText();
    void trimHistory(int size);

    QRect getCursorPosition();

    QStringList 				m_CommandHistory;

    QWidget*                    m_ParentWidget;

    QString                     m_OldStyle;
	QString                     m_SavedEditText;
};
