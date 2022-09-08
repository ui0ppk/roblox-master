/**
 * FindDialog.cpp
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "FindDialog.h"

// Qt Headers
#include <QLineEdit>
#include <QRadioButton>
#include <QCheckBox>
#include <QTextDocument>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QKeyEvent>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QTextBlock>

// Roblox Studio Headers
#include "RobloxMainWindow.h"
#include "UpdateUIManager.h"
#include "ScriptTextEditor.h"
#include "RobloxFindWidget.h"
#include "RobloxDocManager.h"
#include "RobloxScriptDoc.h"

FASTFLAG(StudioEmbeddedFindDialogEnabled)

BasicFindDialog::BasicFindDialog(QWidget *parent) 
: QDialog(parent,Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
, m_pFindListener(0)
, m_pFindEdit(0)
, m_pFindNextButton(0)
, m_pFindPreviousButton(0)
, m_pCloseButton(0)
, m_pDownRadioButton(0)
, m_pCaseCheckBox(0)
, m_pWholeCheckBox(0)
, m_pRegExCheckBox(0)
{
    setWindowFlags(Qt::WindowStaysOnTopHint | windowFlags());
}

BasicFindDialog::~BasicFindDialog()
{
	m_pFindListener = NULL;
}

void BasicFindDialog::setFindListener(IFindListener* listener)
{
    m_pFindListener = listener;
}

void BasicFindDialog::setFocus()
{
	QDialog::setFocus();
	if(isActiveWindow())
	{
		m_pFindEdit->setFocus();
		if(m_pFindEdit->text().size() > 0)
		{
			m_pFindEdit->setCursorPosition(m_pFindEdit->text().size());
			m_pFindEdit->selectAll();
		}
	}
}

void BasicFindDialog::setVisible(bool visible)
{
	QDialog::setVisible(visible);
	if ( visible )
	{
		if ( m_pFindListener )
        {
            if ( m_pFindListener->hasSelection() )
                m_pFindEdit->setText(m_pFindListener->getSelectedText());
            else
                m_pFindEdit->setText(FindReplaceProvider::instance().getSearchStr());
        }

		m_pFindEdit->setFocus();
	}
}

bool BasicFindDialog::isFindNextEnabled()
{
	return (m_pFindEdit->text().size() > 0);
}

void BasicFindDialog::textToFindChanged() 
{
	bool bEnable = m_pFindEdit->text().size() > 0;
    m_pFindNextButton->setEnabled(bEnable);
    m_pFindPreviousButton->setEnabled(bEnable);
	UpdateUIManager::Instance().getMainWindow().findNextAction->setEnabled(bEnable);
    UpdateUIManager::Instance().getMainWindow().findPreviousAction->setEnabled(bEnable);
}

bool BasicFindDialog::getSearchParameters(bool forward,QRegExp& outRegExp,QTextDocument::FindFlags& outflags)
{
    QString toSearch = m_pFindEdit->text();
	FindReplaceProvider::instance().setSearchStr(toSearch);
	if ( toSearch.isEmpty() )
		return false;

    outflags = 0;

    bool down = m_pDownRadioButton->isChecked();
	if( (!down && forward) || (down && !forward) )
		outflags |= QTextDocument::FindBackward;

    if(m_pCaseCheckBox->isChecked())
        outflags |= QTextDocument::FindCaseSensitively;
    if(m_pWholeCheckBox->isChecked())
        outflags |= QTextDocument::FindWholeWords;

    outRegExp.setPattern(toSearch);

    if ( !m_pRegExCheckBox->isChecked() )
        outRegExp.setPatternSyntax(QRegExp::FixedString);
    outRegExp.setCaseSensitivity(m_pCaseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

    if ( !outRegExp.isValid() )
        return false;

    return true;
}

bool BasicFindDialog::find(bool forward,bool loop) 
{
    QRegExp regExp;
    QTextDocument::FindFlags flags;
    
    if ( !getSearchParameters(forward,regExp,flags) )
        return false;

    if ( m_pFindListener )
        return m_pFindListener->find(loop,regExp,flags);
    else
        return false;
}

void BasicFindDialog::keyPressEvent(QKeyEvent* evt)
{
    QAction* previous = UpdateUIManager::Instance().getMainWindow().findPreviousAction;
    if ( evt->matches(QKeySequence::FindPrevious) )
    {
        previous->trigger();
        evt->accept();
        return;
    }

    QAction* next = UpdateUIManager::Instance().getMainWindow().findNextAction;
    if ( evt->matches(QKeySequence::FindNext) )
    {
        next->trigger();
        evt->accept();
        return;
    }

	QDialog::keyPressEvent(evt);
}

FindAllDialog::FindAllDialog(QWidget *parent)
:   BasicFindDialog(parent),
    m_pLookInComboBox(NULL)
{
	initDialog();
}

void FindAllDialog::initDialog()
{
	QLabel *findLabel = new QLabel("Find what: ");
	m_pFindEdit = new QLineEdit;
	QLabel *lookInLabel = new QLabel("Look in: ");
        
	m_pFindAllButton = new QPushButton("Find All");
	m_pFindAllButton->setEnabled(false);
	m_pCloseButton = new QPushButton("Close");

	//Find Options
	m_pFindOptions = new QGroupBox("Find Options");
	m_pFindOptions->setCheckable(true);
	m_pFindOptions->setFlat(true);

	m_pCaseCheckBox = new QCheckBox("Match case");
	m_pWholeCheckBox = new QCheckBox("Match whole word only");
	m_pRegExCheckBox = new QCheckBox("Regular expression");

	QVBoxLayout *findOptionsLayout = new QVBoxLayout();
	findOptionsLayout->addWidget(m_pCaseCheckBox);
	findOptionsLayout->addWidget(m_pWholeCheckBox);
	findOptionsLayout->addWidget(m_pRegExCheckBox);

	m_pFindOptions->setLayout(findOptionsLayout);

	m_pFindLayout = new QGridLayout();
	m_pFindLayout->addWidget(findLabel, 0, 0);
	m_pFindLayout->addWidget(m_pFindEdit, 1, 0);
	m_pFindLayout->addWidget(lookInLabel, 4, 0);

	QVBoxLayout *firstColLayout = new QVBoxLayout();
	firstColLayout->addLayout(m_pFindLayout);
	firstColLayout->addWidget(m_pFindOptions);

	QVBoxLayout *buttonsLayout = new QVBoxLayout();
	buttonsLayout->addWidget(m_pFindAllButton);
	buttonsLayout->addWidget(m_pCloseButton);

	QGridLayout *mainLayout = new QGridLayout();
	mainLayout->addLayout(firstColLayout, 0, 0);
	mainLayout->addLayout(buttonsLayout, 0, 1, Qt::AlignTop);
	setLayout(mainLayout);

	setWindowTitle("Find in all scripts");

	setMaximumHeight(minimumHeight());
	setMaximumWidth(minimumWidth());
	
	connect(m_pFindAllButton, SIGNAL(clicked()), this, SLOT(findAll()));
	connect(m_pFindEdit, SIGNAL(textChanged(QString)), this, SLOT(textToFindChanged()));
	connect(m_pCloseButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(m_pFindOptions, SIGNAL(toggled(bool)), this , SLOT(onFindOptionsToggled(bool)));
}

void FindAllDialog::showEvent(QShowEvent * event)
{
    // This cludgy code is added to fix a QT bug on Mac. Combobox should be
    // initialized when its parent already created. Otherwise it displays a weird
    // frame around drowpdown list.
    m_pLookInComboBox = new QComboBox(this);
    m_pLookInComboBox->addItem("All Scripts");
    m_pLookInComboBox->addItem("Current Script");
    m_pFindLayout->addWidget(m_pLookInComboBox, 5, 0);
    
    BasicFindDialog::showEvent(event);
}

RobloxFindWidget::eFindFlags FindAllDialog::getCurrentFlags()
{
	RobloxFindWidget::eFindFlags flagSetter = RobloxFindWidget::NO_FLAGS;

	if (!m_pFindOptions->isChecked())
		return flagSetter;

	if (m_pCaseCheckBox->isChecked())
		flagSetter = flagSetter | RobloxFindWidget::MATCH_CASE;

	if (m_pWholeCheckBox->isChecked())
		flagSetter = flagSetter | RobloxFindWidget::MATCH_WORD;

	if (m_pRegExCheckBox->isChecked())
		flagSetter = flagSetter | RobloxFindWidget::REG_EXP;

	return flagSetter;
}

void FindAllDialog::findAll()
{
	FindReplaceProvider::instance().setSearchStr(m_pFindEdit->text());
	RobloxFindWidget::singleton().findAll(m_pFindEdit->text(), (m_pLookInComboBox->currentIndex() == 0), getCurrentFlags());
	close();
}

void FindAllDialog::textToFindChanged() 
{
	m_pFindAllButton->setEnabled(m_pFindEdit->text().size() > 0);
}

void FindAllDialog::onFindOptionsToggled(bool on)
{
	m_pFindOptions->setMinimumWidth(m_pFindOptions->width());

	m_pCaseCheckBox->setVisible(on);
	m_pWholeCheckBox->setVisible(on);
	m_pRegExCheckBox->setVisible(on);

	//Need to adjust size but children have not resized yet
	QMetaObject::invokeMethod(this, "adjustSize", Qt::QueuedConnection);
}

void FindAllDialog::setFindAllRunning(bool value)
{
	if (value)
	{
		m_pFindAllButton->setText("Stop");
		disconnect(m_pFindAllButton, SIGNAL(clicked()), this, SLOT(findAll()));
		connect(m_pFindAllButton, SIGNAL(clicked()), &RobloxFindWidget::singleton(), SLOT(stopPopulatingItems()));
	}
	else
	{
		m_pFindAllButton->setText("Find All");
		disconnect(m_pFindAllButton, SIGNAL(clicked()), &RobloxFindWidget::singleton(), SLOT(stopPopulatingItems()));
		connect(m_pFindAllButton, SIGNAL(clicked()), this, SLOT(findAll()));
	}
}


FindDialog::FindDialog(QWidget *parent) 
: BasicFindDialog(parent)
{
	initDialog();
}

void FindDialog::initDialog()
{
	QLabel *findLabel = new QLabel("Find what: ");
	m_pFindEdit = new QLineEdit;

	m_pFindNextButton = new QPushButton("Find Next");
	m_pFindNextButton->setEnabled(false);
    m_pFindPreviousButton = new QPushButton("Find Previous");
	m_pFindPreviousButton->setEnabled(false);
	m_pCloseButton = new QPushButton("Close");

	m_pCaseCheckBox = new QCheckBox("Match case");
	m_pWholeCheckBox = new QCheckBox("Match whole word only");
	m_pRegExCheckBox = new QCheckBox("Regular expression");

	QGroupBox *directionBox = new QGroupBox("Direction");
	QRadioButton *upRadioButton = new QRadioButton("Up");
	m_pDownRadioButton = new QRadioButton("Down");
	QHBoxLayout *directionLayout = new QHBoxLayout();
	directionLayout->addWidget(upRadioButton);
	directionLayout->addWidget(m_pDownRadioButton);
	directionBox->setLayout(directionLayout);

	m_pDownRadioButton->setChecked(true);

	QGridLayout *findLayout = new QGridLayout();
	findLayout->addWidget(findLabel, 0, 0);
	findLayout->addWidget(m_pFindEdit, 0, 1);

	QVBoxLayout *conditionsLayout = new QVBoxLayout();
	conditionsLayout->addWidget(m_pWholeCheckBox);
	conditionsLayout->addWidget(m_pCaseCheckBox);
	conditionsLayout->addWidget(m_pRegExCheckBox);

	QHBoxLayout *secondRowLayout = new QHBoxLayout();
	secondRowLayout->addLayout(conditionsLayout);
	secondRowLayout->addWidget(directionBox);

	QVBoxLayout *firstColLayout = new QVBoxLayout();
	firstColLayout->addLayout(findLayout);
	firstColLayout->addLayout(secondRowLayout);

	QVBoxLayout *buttonsLayout = new QVBoxLayout();
	buttonsLayout->addWidget(m_pFindNextButton);
    buttonsLayout->addWidget(m_pFindPreviousButton);
	buttonsLayout->addWidget(m_pCloseButton);

	QGridLayout *mainLayout = new QGridLayout();
	mainLayout->addLayout(firstColLayout, 0, 0);
	mainLayout->addLayout(buttonsLayout, 0, 1, Qt::AlignTop);
	setLayout(mainLayout);

	setWindowTitle("Find");

	connect(m_pFindEdit, SIGNAL(textChanged(QString)), this, SLOT(textToFindChanged()));
    connect(m_pFindNextButton, SIGNAL(clicked()), this, SLOT(findNext()));
    connect(m_pFindPreviousButton, SIGNAL(clicked()), this, SLOT(findPrevious()));
    connect(m_pCloseButton, SIGNAL(clicked()), this, SLOT(close()));
}

ReplaceDialog::ReplaceDialog(QWidget *parent)
: BasicFindDialog(parent)
{
	initDialog();
}

void ReplaceDialog::initDialog()
{
	QLabel *findLabel = new QLabel("Find what: ");
	m_pFindEdit = new QLineEdit;

	QLabel *replaceLabel = new QLabel("Replace with: ");
	m_pReplaceEdit = new QLineEdit;

	m_pFindNextButton = new QPushButton("Find Next");
	m_pFindNextButton->setEnabled(false);
    m_pFindPreviousButton = new QPushButton("Find Previous");
	m_pFindPreviousButton->setEnabled(false);
	m_pReplaceButton = new QPushButton("Replace");
	m_pReplaceButton->setEnabled(false);
	m_pReplaceAllButton = new QPushButton("Replace All");
	m_pReplaceAllButton->setEnabled(false);
	m_pCloseButton = new QPushButton("Close");

	m_pCaseCheckBox = new QCheckBox("Match case");
	m_pWholeCheckBox = new QCheckBox("Match whole word only");
	m_pRegExCheckBox = new QCheckBox("Regular expression");

	QGroupBox *directionBox = new QGroupBox("Direction");
	QRadioButton *upRadioButton = new QRadioButton("Up");
	m_pDownRadioButton = new QRadioButton("Down");
	QHBoxLayout *directionLayout = new QHBoxLayout();
	directionLayout->addWidget(upRadioButton);
	directionLayout->addWidget(m_pDownRadioButton);
	directionBox->setLayout(directionLayout);

	m_pDownRadioButton->setChecked(true);

	QGridLayout *findLayout = new QGridLayout();
	findLayout->addWidget(findLabel, 0, 0);
	findLayout->addWidget(m_pFindEdit, 0, 1);
	findLayout->addWidget(replaceLabel, 1, 0);
	findLayout->addWidget(m_pReplaceEdit, 1, 1);

	QVBoxLayout *conditionsLayout = new QVBoxLayout();
	conditionsLayout->addWidget(m_pWholeCheckBox);
	conditionsLayout->addWidget(m_pCaseCheckBox);
	conditionsLayout->addWidget(m_pRegExCheckBox);

	QHBoxLayout *secondRowLayout = new QHBoxLayout();
	secondRowLayout->addLayout(conditionsLayout);
	secondRowLayout->addWidget(directionBox);

	QVBoxLayout *firstColLayout = new QVBoxLayout();
	firstColLayout->addLayout(findLayout);
	firstColLayout->addLayout(secondRowLayout);

	QVBoxLayout *buttonsLayout = new QVBoxLayout();
	buttonsLayout->addWidget(m_pFindNextButton);
    buttonsLayout->addWidget(m_pFindPreviousButton);
	buttonsLayout->addWidget(m_pReplaceButton);
	buttonsLayout->addWidget(m_pReplaceAllButton);
	buttonsLayout->addWidget(m_pCloseButton);

	QGridLayout *mainLayout = new QGridLayout();
	mainLayout->addLayout(firstColLayout, 0, 0);
	mainLayout->addLayout(buttonsLayout, 0, 1, Qt::AlignTop);
	setLayout(mainLayout);

	setWindowTitle("Replace");

	connect(m_pFindEdit, SIGNAL(textChanged(QString)), this, SLOT(textToFindChanged()));
    connect(m_pFindNextButton, SIGNAL(clicked()), this, SLOT(findNext()));
    connect(m_pFindPreviousButton, SIGNAL(clicked()), this, SLOT(findPrevious()));
    connect(m_pCloseButton, SIGNAL(clicked()), this, SLOT(close()));
	connect(m_pReplaceButton, SIGNAL(clicked()), this, SLOT(replace()));
    connect(m_pReplaceAllButton, SIGNAL(clicked()), this, SLOT(replaceAll()));
}

void ReplaceDialog::replace() 
{
	if ( !m_pFindListener )
		return;

    QRegExp regExp;
    QTextDocument::FindFlags flags;
    
    if ( !getSearchParameters(true,regExp,flags) )
        return;

    if ( m_pFindListener->hasSelection() && isSelectionValid() )
        m_pFindListener->replace(m_pReplaceEdit->text(),regExp,flags);

    find(true,true);
}

void ReplaceDialog::replaceAll() 
{
	if ( !m_pFindListener )
		return;

    QRegExp regExp;
    QTextDocument::FindFlags flags;
    
    if ( !getSearchParameters(true,regExp,flags) )
        return;

    m_pFindListener->goToStart(flags);

    while ( find(true,false) )
        m_pFindListener->replace(m_pReplaceEdit->text(),regExp,flags);
}

void ReplaceDialog::textToFindChanged() 
{
	bool bEnable = m_pFindEdit->text().size() > 0;
    m_pFindNextButton->setEnabled(bEnable);
    m_pFindPreviousButton->setEnabled(bEnable);
	m_pReplaceButton->setEnabled(bEnable);
	m_pReplaceAllButton->setEnabled(bEnable);
}

bool ReplaceDialog::isSelectionValid()
{
	QString selectedText = m_pFindListener->getSelectedText();
	if ( !selectedText.isEmpty() )
	{
		QString toSearch = m_pFindEdit->text();
		if ( !toSearch.isEmpty() )
		{
			QRegExp reg(toSearch);
			if ( !m_pRegExCheckBox->isChecked() )
				reg.setPatternSyntax(QRegExp::FixedString);

			reg.setCaseSensitivity(m_pCaseCheckBox->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

			//selection and replacement string should be matching
			if ( selectedText.indexOf(reg) == 0 )
				return true;
		}
	}

	return false;
}

FindReplaceProvider& FindReplaceProvider::instance()
{
	static FindReplaceProvider inst;
	return inst;
}

FindReplaceProvider::FindReplaceProvider()
	: m_pEmbeddedFindReplaceWidget(NULL)
	, m_pFindDialog(NULL)
	, m_pReplaceDialog(NULL)
	, m_pFindAllDialog(NULL)
	, m_pGotoLineDialog(NULL)
{
	if (FFlag::StudioEmbeddedFindDialogEnabled)
	{
		m_pEmbeddedFindReplaceWidget = new EmbeddedFindReplaceWidget(&UpdateUIManager::Instance().getMainWindow());
		m_pEmbeddedFindReplaceWidget->hide();
	}
	else
	{
		m_pFindDialog = new FindDialog(&UpdateUIManager::Instance().getMainWindow());
		m_pFindDialog->hide();

		m_pReplaceDialog = new ReplaceDialog(&UpdateUIManager::Instance().getMainWindow());		
		m_pReplaceDialog->hide();
	}

	m_pFindAllDialog = new FindAllDialog(&UpdateUIManager::Instance().getMainWindow());
	m_pFindAllDialog->hide();

	m_pGotoLineDialog = new GoToLineDialog(&UpdateUIManager::Instance().getMainWindow());
	m_pGotoLineDialog->hide();
}

FindReplaceProvider::~FindReplaceProvider()
{
	m_pFindDialog = NULL;       // Will be deleted by parent i.e. main window
	m_pReplaceDialog = NULL;    // Will be deleted by parent i.e. main window
	m_pFindAllDialog = NULL;
	m_pGotoLineDialog = NULL;
	m_pEmbeddedFindReplaceWidget = NULL;
}

void FindReplaceProvider::setFindListener(IFindListener* listener)
{
	if (m_pFindDialog)
		m_pFindDialog->setFindListener(listener);

	if (m_pReplaceDialog)
		m_pReplaceDialog->setFindListener(listener);

	if (m_pFindAllDialog)
		m_pFindAllDialog->setFindListener(listener);

	if (m_pGotoLineDialog)
		m_pGotoLineDialog->setFindListener(listener);

	if (FFlag::StudioEmbeddedFindDialogEnabled)
		m_pEmbeddedFindReplaceWidget->setFindListener(listener);
}

bool FindReplaceProvider::isFindNextEnabled()
{
	return (m_pFindDialog->isFindNextEnabled() || m_pReplaceDialog->isFindNextEnabled());
}

void FindReplaceProvider::find(bool forward)
{
	if(m_pReplaceDialog->isVisible())
		m_pReplaceDialog->find(forward,true);
	else
		m_pFindDialog->find(forward,true);
}


GoToLineDialog::GoToLineDialog(QWidget *parent)
	: QDialog(parent)
{
	initDialog();
}

void GoToLineDialog::initDialog()
{
	setModal(true);
	setWindowFlags(windowFlags() ^ Qt::WindowContextHelpButtonHint);
	setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
	
	m_pLineNumberLabel = new QLabel("Line number:");

	m_pNumberValidator = new QIntValidator(this);
	m_pNumberValidator->setBottom(0);

	m_pLineNumberEdit = new QLineEdit;
	m_pLineNumberEdit->setValidator(m_pNumberValidator);

	m_pOkButton = new QPushButton("OK");
	m_pCancelButton = new QPushButton("Cancel");

	QHBoxLayout *pushButtons = new QHBoxLayout();
	pushButtons->addStretch();
	pushButtons->addWidget(m_pOkButton);
	pushButtons->addWidget(m_pCancelButton);

	QVBoxLayout *mainLayout = new QVBoxLayout();
	mainLayout->addWidget(m_pLineNumberLabel);
	mainLayout->addWidget(m_pLineNumberEdit);
	mainLayout->addLayout(pushButtons);

	setLayout(mainLayout);

	setWindowTitle("Go to line");

	connect(m_pLineNumberEdit, SIGNAL(returnPressed()), this, SLOT(goToLine()));
	connect(m_pOkButton, SIGNAL(clicked()), this, SLOT(goToLine()));
	connect(m_pCancelButton, SIGNAL(clicked()), this, SLOT(close()));
}

void GoToLineDialog::activate(int currentLine, int totalLines)
{
	lineCount = totalLines;
	m_pLineNumberLabel->setText(QString("Line number (1-%1):").arg(lineCount));

	m_pLineNumberEdit->setText(QString::number(currentLine));

	show();
	activateWindow();
	m_pLineNumberEdit->setFocus();
	m_pLineNumberEdit->selectAll();
}

void GoToLineDialog::goToLine()
{
	if (m_pFindListener)
		m_pFindListener->moveCursorToLine(qMin(m_pLineNumberEdit->text().toInt() - 1, lineCount - 1));
	close();
}


EmbeddedFindReplaceWidget::EmbeddedFindReplaceWidget(QWidget *parent)
	: QDialog(parent)
	, m_replaceVisible(false)
{
	initDialog();
}

void EmbeddedFindReplaceWidget::activate(bool showReplace)
{
	if (showReplace != m_replaceVisible)
		toggleReplaceVisible();

	show();
	activateWindow();
	setFocus();

	m_pFindComboBox->lineEdit()->selectAll();
}


void EmbeddedFindReplaceWidget::initDialog()
{
	setWindowFlags(Qt::Window | Qt::Dialog | Qt::FramelessWindowHint);

	setStyleSheet("background-color: #f3f3f3");

	setMinimumWidth(280);
	setMaximumHeight(0);

	QString pushButtonStyleSheet = STRINGIFY(

	QPushButton 
	{
		border: 0px;
	}

	QPushButton:hover 
	{
		background: #E4EEFE;
	}

	QPushButton:checked
	{
		border: 1px solid #B8B8B8;
		background: #DBDBDB;
	}
	);

	QString comboBoxStyleSheet = STRINGIFY(
	QComboBox 
	{
		border: 0px;
	}
	);

	m_pFindComboBox = new QComboBox();
	m_pFindComboBox->setEditable(true);
	m_pFindComboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_pFindComboBox->lineEdit()->setPlaceholderText("Find...");
	m_pFindComboBox->setAutoCompletion(false);

	setFocusProxy(m_pFindComboBox);

	m_pReplaceComboBox = new QComboBox();
	m_pReplaceComboBox->setEditable(true);
	m_pReplaceComboBox->hide();
	m_pReplaceComboBox->lineEdit()->setPlaceholderText("Replace...");
	m_pReplaceComboBox->setAutoCompletion(false);
	
	m_pToggleReplaceButton = new QPushButton(QIcon(":/16x16/images/Studio 2.0 icons/16x16/expand_gray_16.png"), "");
	m_pToggleReplaceButton->setFixedSize(18, 18);
	m_pToggleReplaceButton->setStyleSheet(pushButtonStyleSheet);
	m_pToggleReplaceButton->setAutoDefault(false);

	m_pFindNextButton = new QPushButton(QIcon(":/16x16/images/Studio 2.0 icons/16x16/arrowright_black_16.png"), "");
	m_pFindNextButton->setFixedSize(18, 18);
	m_pFindNextButton->setStyleSheet(pushButtonStyleSheet);
	m_pFindNextButton->setAutoDefault(true);
	m_pFindNextButton->setToolTip("Find Next");

	m_pFindPreviousButton = new QPushButton(QIcon(":/16x16/images/Studio 2.0 icons/16x16/arrowleft_black_16.png"), "");
	m_pFindPreviousButton->setFixedSize(18, 18);
	m_pFindPreviousButton->setStyleSheet(pushButtonStyleSheet);
	m_pFindPreviousButton->setAutoDefault(false);
	m_pFindPreviousButton->setToolTip("Find Previous");

	m_pCloseDialogButton = new QPushButton(QIcon(":/16x16/images/Studio 2.0 icons/16x16/delete_x_16_b.png"), "");
	m_pCloseDialogButton->setFixedSize(18, 18);
	m_pCloseDialogButton->setStyleSheet(pushButtonStyleSheet);
	m_pCloseDialogButton->setAutoDefault(false);
	m_pCloseDialogButton->setToolTip("Close");

	m_pReplaceNextButton = new QPushButton(QIcon(":/16x16/images/Studio 2.0 icons/16x16/replace.png"), "");
	m_pReplaceNextButton->setFixedSize(18, 18);
	m_pReplaceNextButton->setStyleSheet(pushButtonStyleSheet);
	m_pReplaceNextButton->hide();
	m_pReplaceNextButton->setAutoDefault(false);
	m_pReplaceNextButton->setToolTip("Replace Next");

	m_pReplaceAllButton = new QPushButton(QIcon(":/16x16/images/Studio 2.0 icons/16x16/replace_all.png"), "");
	m_pReplaceAllButton->setFixedSize(18, 18);
	m_pReplaceAllButton->setStyleSheet(pushButtonStyleSheet);
	m_pReplaceAllButton->hide();
	m_pReplaceAllButton->setAutoDefault(false);
	m_pReplaceAllButton->setToolTip("Replace All");

	m_pMatchCaseButton = new QPushButton(QIcon(":/16x16/images/Studio 2.0 icons/16x16/match_case.png"), "");
	m_pMatchCaseButton->setFixedSize(18, 18);
	m_pMatchCaseButton->setStyleSheet(pushButtonStyleSheet);
	m_pMatchCaseButton->setCheckable(true);
	m_pMatchCaseButton->setAutoDefault(false);
	m_pMatchCaseButton->setToolTip("Match Case");
	
	m_pMatchWholeWordButton = new QPushButton(QIcon(":/16x16/images/Studio 2.0 icons/16x16/match_whole_word.png"), "");
	m_pMatchWholeWordButton->setFixedSize(18, 18);
	m_pMatchWholeWordButton->setStyleSheet(pushButtonStyleSheet);
	m_pMatchWholeWordButton->setCheckable(true);
	m_pMatchWholeWordButton->setAutoDefault(false);
	m_pMatchWholeWordButton->setToolTip("Match Whole Word");
	
	m_pRegularExpressionButton = new QPushButton(QIcon(":/16x16/images/Studio 2.0 icons/16x16/regexp.png"), "");
	m_pRegularExpressionButton->setFixedSize(18, 18);
	m_pRegularExpressionButton->setStyleSheet(pushButtonStyleSheet);
	m_pRegularExpressionButton->setCheckable(true);
	m_pRegularExpressionButton->setAutoDefault(false);
	m_pRegularExpressionButton->setToolTip("Regular Expression");

	m_pNumberOfResultsFound = new QLabel();
	m_pNumberOfResultsFound->setText("");

	QBrush colorBrush(QColor(128, 128, 128, 255));
	QPalette palette;
	palette.setBrush(QPalette::Inactive, QPalette::WindowText, colorBrush);
	palette.setBrush(QPalette::Active, QPalette::WindowText, colorBrush);

	m_pNumberOfResultsFound->setPalette(palette);
	
	QFrame* bottomFrame = new QFrame();
	bottomFrame->setStyleSheet("background-color: #c4c4c4");
	bottomFrame->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
	bottomFrame->setFixedHeight(4);

	QSpacerItem* spacer1 = new QSpacerItem(2, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);
	QSpacerItem* spacer2 = new QSpacerItem(2, 10, QSizePolicy::Fixed, QSizePolicy::Fixed);

	QGridLayout* mainLayout = new QGridLayout();

	mainLayout->setSpacing(2);
	mainLayout->setContentsMargins(QMargins(0, 4, 0, 0));

	mainLayout->addItem(spacer1, 1, 0);
	mainLayout->addWidget(m_pToggleReplaceButton, 1, 1);
	mainLayout->addWidget(m_pFindComboBox, 1, 2, 1, 4);
	mainLayout->addWidget(m_pFindPreviousButton, 1, 6);
	mainLayout->addWidget(m_pFindNextButton, 1, 7);
	mainLayout->addWidget(m_pCloseDialogButton, 1, 8);
	mainLayout->addItem(spacer2, 1, 9);

	mainLayout->addWidget(m_pReplaceComboBox, 2, 2, 1, 4);
	mainLayout->addWidget(m_pReplaceNextButton, 2, 6);
	mainLayout->addWidget(m_pReplaceAllButton, 2, 7);

	mainLayout->addWidget(m_pMatchCaseButton, 3, 2);
	mainLayout->addWidget(m_pMatchWholeWordButton, 3, 3);
	mainLayout->addWidget(m_pRegularExpressionButton, 3, 4);
	mainLayout->addWidget(m_pNumberOfResultsFound, 3, 5);

	mainLayout->addWidget(bottomFrame, 4, 0, 1, 10);

	setLayout(mainLayout);
	
	connect(m_pCloseDialogButton, SIGNAL(pressed()), this, SLOT(onCloseDialog()));
	connect(m_pFindComboBox->lineEdit(), SIGNAL(textChanged(QString)), this, SLOT(find()));

	connect(m_pMatchCaseButton, SIGNAL(clicked()), this, SLOT(find()));
	connect(m_pMatchWholeWordButton, SIGNAL(clicked()), this, SLOT(find()));
	connect(m_pRegularExpressionButton, SIGNAL(clicked()), this, SLOT(find()));
	connect(m_pToggleReplaceButton, SIGNAL(pressed()), this, SLOT(toggleReplaceVisible()));
	
	connect(m_pFindPreviousButton, SIGNAL(pressed()), this, SLOT(findPrevious()));
	connect(m_pFindNextButton, SIGNAL(pressed()), this, SLOT(findNext()));
	connect(m_pReplaceNextButton, SIGNAL(pressed()), this, SLOT(replaceNext()));
	connect(m_pReplaceAllButton, SIGNAL(pressed()), this, SLOT(replaceAll()));
}

void EmbeddedFindReplaceWidget::keyPressEvent(QKeyEvent* evt)
{
	QKeySequence key = QKeySequence(evt->modifiers()|evt->key());

	QAction* previous = UpdateUIManager::Instance().getMainWindow().findPreviousAction;
	if (previous && key == previous->shortcut())
	{
		previous->trigger();
		evt->accept();
		return;
	}

	QAction* next = UpdateUIManager::Instance().getMainWindow().findNextAction;
	if (next && key == next->shortcut())
	{
		next->trigger();
		evt->accept();
		return;
	}

	if (evt->key() == Qt::Key_Enter ||
		evt->key() == Qt::Key_Return)
	{
		if (m_pFindComboBox->hasFocus())
			findNext();
		else if (m_pReplaceComboBox->hasFocus())
			replaceNext();

		evt->accept();
		return;
	}

	if (evt->key() == Qt::Key_Escape)
	{
		onCloseDialog();
		evt->accept();
		return;
	}

	QWidget::keyPressEvent(evt);
}

void EmbeddedFindReplaceWidget::setNumberOfFoundResults(int number)
{
	if (number == -1)
	{
		m_pNumberOfResultsFound->setText("");
		return;
	}

	m_pNumberOfResultsFound->setText(QString("  %1 occurrences found").arg(number));
}

QRegExp EmbeddedFindReplaceWidget::getSearchString()
{
	QRegExp rx(m_pFindComboBox->lineEdit()->text());

	if (!m_pRegularExpressionButton->isChecked())
		rx.setPatternSyntax(QRegExp::FixedString);

	rx.setCaseSensitivity(m_pMatchCaseButton->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);

	return rx;
}

void EmbeddedFindReplaceWidget::toggleReplaceVisible()
{
	m_replaceVisible = !m_replaceVisible;

	QString toggleButtonIconLocation = m_replaceVisible ? "collapse_gray_16.png" : "expand_gray_16.png";
	m_pToggleReplaceButton->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/" + toggleButtonIconLocation));

	m_pReplaceComboBox->setVisible(m_replaceVisible);
	m_pReplaceNextButton->setVisible(m_replaceVisible);
	m_pReplaceAllButton->setVisible(m_replaceVisible);

 	adjustSize();
}

void EmbeddedFindReplaceWidget::updateFindPosition(QPoint position, QSize size)
{
	this->setGeometry(position.x() + size.width() - geometry().width(), position.y(), geometry().width(), geometry().height());
}

void EmbeddedFindReplaceWidget::find()
{	
    QTextDocument::FindFlags flags = m_pMatchWholeWordButton->isChecked() ?
                                            QTextDocument::FindWholeWords :
                                            (QTextDocument::FindFlags)0;

	m_pFindListener->find(false, getSearchString(), flags);
}

void EmbeddedFindReplaceWidget::findNext()
{
	m_pFindListener->moveNext(true);
}

void EmbeddedFindReplaceWidget::findPrevious()
{
	m_pFindListener->moveNext(false);
}

void EmbeddedFindReplaceWidget::replaceNext()
{
	m_pFindListener->replaceNext(getSearchString(), m_pReplaceComboBox->lineEdit()->text());
}

void EmbeddedFindReplaceWidget::replaceAll()
{
	m_pFindListener->replaceAll(getSearchString(), m_pReplaceComboBox->lineEdit()->text());
}

void EmbeddedFindReplaceWidget::onCloseDialog()
{
	m_pFindComboBox->lineEdit()->setText("");
	close();
}
