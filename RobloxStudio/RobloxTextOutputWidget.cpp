/**
 * RobloxTextOutputWidget.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxTextOutputWidget.h"

// Qt Headers
#include <QApplication>
#include <QDateTime>
#include <QThread>
#include <QScrollBar>

// Roblox Studio Headers
#include "AuthoringSettings.h"
#include "Roblox.h"

RobloxTextOutputWidget::RobloxTextOutputWidget(QWidget* parent)
    : QPlainTextEdit(parent)
{
	setReadOnly(true);
	setTextInteractionFlags(Qt::TextSelectableByMouse);

    // set up colors and font styles

    m_TextFormats[RBX::MESSAGE_OUTPUT].setFontWeight(QFont::Normal);

    m_TextFormats[RBX::MESSAGE_INFO].setFontWeight(QFont::Normal);
    m_TextFormats[RBX::MESSAGE_INFO].setForeground(Qt::blue);

    m_TextFormats[RBX::MESSAGE_WARNING].setFontWeight(QFont::Bold);
    m_TextFormats[RBX::MESSAGE_WARNING].setForeground(QColor(255,128,0));

    m_TextFormats[RBX::MESSAGE_ERROR].setFontWeight(QFont::Bold);
    m_TextFormats[RBX::MESSAGE_ERROR].setForeground(Qt::red);

	m_TextFormats[RBX::MESSAGE_SENSITIVE].setFontWeight(QFont::Bold);
	m_TextFormats[RBX::MESSAGE_SENSITIVE].setForeground(Qt::darkMagenta);

    // listen for changes to the editor settings
    m_PropertyChangedConnection = AuthoringSettings::singleton().propertyChangedSignal.connect(
        boost::bind(&RobloxTextOutputWidget::onPropertyChanged,this,_1));
    onPropertyChanged(NULL);

    connect(
        &Roblox::Instance(),SIGNAL(newOutputMessage(const QString,RBX::MessageType)), 
        this,SLOT(appendOutputText(const QString&,RBX::MessageType)) );
}

RobloxTextOutputWidget::~RobloxTextOutputWidget()
{
    m_PropertyChangedConnection.disconnect();
}

QSize RobloxTextOutputWidget::sizeHint() const
{
	return QSize(width(), 104);
}

void RobloxTextOutputWidget::contextMenuEvent(QContextMenuEvent * evt)
{
    QMenu *menu = new QMenu(this);
    QAction *a;

    a = menu->addAction(tr("&Copy\t") + QKeySequence(QKeySequence::Copy).toString(), this, SLOT(copy()));
    a->setEnabled(textCursor().hasSelection());

    menu->addSeparator();
    a = menu->addAction(tr("Select All\t") + QKeySequence(QKeySequence::SelectAll).toString(), this, SLOT(selectAll()));
    a->setEnabled(!document()->isEmpty());

    a = menu->addAction(tr("Clear Output"), this, SLOT(clear()));
    a->setEnabled(!document()->isEmpty());

    menu->exec(evt->globalPos());
    delete menu;
}

bool RobloxTextOutputWidget::event(QEvent* evt)
{
    bool handled = false;

    if ( evt->type() == QEvent::ShortcutOverride )
    {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(evt);
        if ( keyEvent->matches(QKeySequence::Copy) )
        {
            copy();
            keyEvent->accept();
            handled = true;
        }
        else if ( keyEvent->matches(QKeySequence::SelectAll) )
        {
            selectAll();
            keyEvent->accept();
            handled = true;
        }
    }
    else
    {
        handled = QPlainTextEdit::event(evt);
    }

    return handled;
}

void RobloxTextOutputWidget::onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor)
{
    if ( !pDescriptor || pDescriptor->name.str == "Maximum Output Lines" )
    {
        setMaximumBlockCount(AuthoringSettings::singleton().maximumOutputLines);
    }
}


bool RobloxTextOutputWidget::isScrollOnBottom()
{
	int verticalBarValue = verticalScrollBar()->value(), maximumValue = verticalScrollBar()->maximum();
	return verticalBarValue == maximumValue;
}

void RobloxTextOutputWidget::resizeEvent(QResizeEvent *e)
{
	bool isOnBottom = isScrollOnBottom();

	QPlainTextEdit::resizeEvent(e);

	if (isOnBottom)
	{
		int maximumValue = verticalScrollBar()->maximum();
		verticalScrollBar()->setValue(maximumValue);
	}
}

/**
 * Callback for handling a log entry coming from the engine.
 *  
 * @param   message     note that this is a copy, not a reference to handle multi-threading
 * @param   type        type of log message
 */
void RobloxTextOutputWidget::appendOutputText(const QString message,RBX::MessageType type)
{
    RBXASSERT(QThread::currentThread() == qApp->thread());

    QString text = message;

	if ( type != RBX::MESSAGE_OUTPUT )
	{
		QString dateTime = QDateTime::currentDateTime().toString("hh:mm:ss.zzz");
		text.prepend(dateTime + " - ");
	}

	bool isOnBottom = isScrollOnBottom();
	int oldScrollValue = verticalScrollBar()->value();

	QTextCursor oldTextCursor =	textCursor();

	// This will stop highlighting.  If a selection is highlighted from right to left the QT overrides 
	//  setCurrentCharFormat with whatever text color was at the cursor selection location. This terminates 
	//  highlighting before the format is set so it will be the set format, not the cursor location format.
	moveCursor(QTextCursor::End,QTextCursor::MoveAnchor);
	setCurrentCharFormat(m_TextFormats[type]);

	appendPlainText(text);

	if (oldTextCursor.hasSelection())
		setTextCursor(oldTextCursor);

	if (!isOnBottom)
		verticalScrollBar()->setValue(oldScrollValue);
}






