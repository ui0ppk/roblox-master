/**
 * ScriptCommandTool.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"

// Qt Headers
#include <QToolBar>
#include <QCompleter>
#include <QKeyEvent>
#include <QLineEdit>

// Roblox Headers
#include "util/RbxStringTable.h"
#include "script/ScriptContext.h"
#include "util/UDim.h"

// Roblox Studio Headers
#include "ScriptComboBox.h"
#include "RobloxDocManager.h" 
#include "RobloxIDEDoc.h"
#include "QtUtilities.h"
#include "AuthoringSettings.h"
#include "StudioIntellesense.h"
#include "StudioUtilities.h"

FASTINTVARIABLE(StudioMaxCommandHistorySize, 50)

#define COMMAND_PLACE_HOLDER "Run a command"

ScriptComboBox::ScriptComboBox(QWidget* parent)
{
    setEditable(true);
    setFocusPolicy(Qt::StrongFocus);
    completer()->setCaseSensitivity(Qt::CaseSensitive);
    completer()->setCompletionRole(Qt::DisplayRole);
    setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    setMinimumWidth(300);
    setMaximumWidth(QWIDGETSIZE_MAX);

    m_ParentWidget = parent;

    m_OldStyle = "QComboBox { color: black; }";
    applyPlaceHolderText();

	connect(lineEdit(), SIGNAL(textEdited(const QString&)), this, SLOT(onEditorTextEdit(const QString&)));
	connect(view(), SIGNAL(pressed(QModelIndex)), this, SLOT(onItemSelectionFromView()));
}

void ScriptComboBox::applyPlaceHolderText()
{
    setEditText(COMMAND_PLACE_HOLDER);
    QString styleSheet = "QComboBox { color: grey; }";
    setStyleSheet(styleSheet); 
}

void ScriptComboBox::intellesenseDoubleClick(QListWidgetItem* listItem)
{
    if (lineEdit()->hasSelectedText())
        lineEdit()->del();

    QString lineText = lineEdit()->text();
    int cursorPos = lineEdit()->cursorPosition();

    Studio::Intellesense::singleton().replaceTextWithCurrentItem(lineText, cursorPos);

    lineEdit()->selectAll();
    lineEdit()->insert(lineText);
    lineEdit()->setCursorPosition(cursorPos);
}

void ScriptComboBox::setCommandHistory(const QStringList& commandHistory)
{
    m_CommandHistory = commandHistory;

    clear();

    if (!m_CommandHistory.isEmpty())
    {
        for (int i = 0; i < m_CommandHistory.size(); ++i)
        {
            QString command = m_CommandHistory[i];
            int errorLine = 0;
            std::string errorMessage;

            if (RBX::ScriptContext::checkSyntax(command.toStdString(), errorLine, errorMessage))
                addItem(command);
        }
        setCurrentIndex(-1);
    }

    applyPlaceHolderText();
}

void ScriptComboBox::focusInEvent(QFocusEvent *event)
{
    if (currentText() == COMMAND_PLACE_HOLDER)
    {
        setEditText("");
    }

    setStyleSheet(m_OldStyle);

    QComboBox::focusInEvent(event);
	if (AuthoringSettings::singleton().intellisenseEnabled)
		connect(&Studio::Intellesense::singleton(), SIGNAL(doubleClickSignal(QListWidgetItem*)), this, SLOT(intellesenseDoubleClick(QListWidgetItem*)));
}

void ScriptComboBox::focusOutEvent(QFocusEvent *event)
{
    if (currentText() == "")
    {
        applyPlaceHolderText();
    }

    QComboBox::focusOutEvent(event);
	if (AuthoringSettings::singleton().intellisenseEnabled)
		disconnect(&Studio::Intellesense::singleton(), SIGNAL(doubleClickSignal(QListWidgetItem*)), this, SLOT(intellesenseDoubleClick(QListWidgetItem*)));
}

bool ScriptComboBox::event(QEvent* e)
{
    bool isHandled = false;
    if (e->type() == QEvent::KeyPress)
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(e);
        const int key = keyEvent->key();
        Qt::KeyboardModifiers modifiers = keyEvent->modifiers();

        if (AuthoringSettings::singleton().intellisenseEnabled && modifiers != Qt::ControlModifier)
        {
            //Check if there is selection and key is alphanumeric or special
            if (lineEdit()->hasSelectedText() && (key < Qt::Key_nobreakspace || ((key == Qt::Key_Tab || key == Qt::Key_Return) && Studio::Intellesense::singleton().isActive())))
                lineEdit()->del();

            QString lineText = lineEdit()->text();
            int cursorPos = lineEdit()->cursorPosition();

            QRect tempRect = getCursorPosition();

            if (Studio::Intellesense::singleton().activate(lineText, 0, cursorPos, keyEvent, tempRect, true, this, boost::none))
            {
                if (lineText != lineEdit()->text())
                {
                    lineEdit()->selectAll();
                    lineEdit()->insert(lineText);
                    lineEdit()->setCursorPosition(cursorPos);
                }
                return true;
            }
        }
        else if (key != Qt::Key_Control && modifiers == Qt::ControlModifier)
        {
            Studio::Intellesense::singleton().deactivate();
        }
        
        if (key == Qt::Key_Return)
        {
			onScriptInputReturnPressed();
            isHandled = true;
        }
        else if (key == Qt::Key_Up && (count() > 0))
        {
            if (currentIndex() < 0)
            {
                setCurrentIndex(count() - 1);
                isHandled = true;
            }
        }
		else if ((key == Qt::Key_Down) && (count() > 0))
		{
			// do not do anything if no selected index
			if (currentIndex() < 0)
			{
				isHandled = true;
			}
			// if last index and we have saved text
			else if ((currentIndex() == count() - 1) && !m_SavedEditText.isEmpty())
			{
				// remove current index so we get correct correct behavior for keyup
				setCurrentIndex(-1);
				lineEdit()->setText(m_SavedEditText);
				isHandled = true;
			}
		}
    }
    else if (e->type() == QEvent::MouseButtonDblClick)
    {
        isHandled = true;
    }
    else if (e->type() == QEvent::MouseButtonRelease)
    {
        isHandled = true;
    }
    
    if (!isHandled)
        isHandled = QComboBox::event(e);

    return isHandled;
}


void ScriptComboBox::onScriptInputReturnPressed()
{
    if (!RobloxDocManager::Instance().getPlayDoc())
    {
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Command cannot be executed without an open place. Please open a place and try again.");
        return;
    }

    QString toExecute = currentText();
    if (!toExecute.isEmpty())
    {
        int errorLine = 0;
        std::string errorMessage;
        if (RBX::ScriptContext::checkSyntax(toExecute.toStdString(), errorLine, errorMessage))
        {
			m_SavedEditText.clear();
            if (!m_CommandHistory.contains(toExecute))
            {
                if (m_CommandHistory.size() == 10)
                    m_CommandHistory.removeFirst();

                m_CommandHistory.append(toExecute);
            }

            if (findText(toExecute, Qt::MatchFixedString) == -1)
            {
                addItem(toExecute);
                setCurrentIndex(count() - 1);
            }
        }
        else
        {
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error in script: %s", errorMessage.c_str());
            lineEdit()->setText(toExecute);
            lineEdit()->selectAll();
            return;
        }
        RBX::StandardOut::singleton()->printf(RBX::MESSAGE_OUTPUT, STRING_BY_ID(CommandOutStringId), toExecute.toStdString().c_str());

		// convert loadfile('http://www.roblox.com/game/join.ashx')() to just the url
		// TODO: proper cmd for starting player via cmd bar?
		if (StudioUtilities::containsJoinScript(toExecute) && toExecute.contains("loadfile("))
		{
			int urlBegin = toExecute.indexOf("(")+2;
			int urlEnd = toExecute.indexOf(")")-1;
			toExecute = toExecute.mid(urlBegin, urlEnd - urlBegin);
		}

        RobloxDocManager::Instance().getPlayDoc()->handleScriptCommand(toExecute);

        if (lineEdit())
            lineEdit()->selectAll();
    }
}

QRect ScriptComboBox::getCursorPosition()
{
    QRect parentGeometry = parentWidget()->geometry();

    if (!(static_cast<QToolBar*>(parentWidget()))->isFloating())
    {
        QRect grandparentGeometry = parentWidget()->parentWidget()->geometry();
        parentGeometry.setX(parentGeometry.x() + grandparentGeometry.x());
        parentGeometry.setY(parentGeometry.y() + grandparentGeometry.y());
    }
    parentGeometry.setWidth(1);
    parentGeometry.setHeight(24);

    QFontMetrics fontMetrics = QFontMetrics(lineEdit()->font());
	parentGeometry.setX(parentGeometry.x() + 21 + fontMetrics.width(lineEdit()->text().left(lineEdit()->cursorPosition())));

    return parentGeometry;
}

void ScriptComboBox::onEditorTextEdit(const QString& text)
{
	// save text and update current index for correct ordering
	m_SavedEditText = text;
	if (currentIndex() >= 0)
	{
		int pos = lineEdit()->cursorPosition();
		setCurrentIndex(-1);
		lineEdit()->setText(m_SavedEditText);
		lineEdit()->setCursorPosition(pos);
	}
}

void ScriptComboBox::onItemSelectionFromView()
{
	// user has selected an item from combo drop down, remove saved text
	m_SavedEditText.clear(); 
}
