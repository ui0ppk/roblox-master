/**
 * FindDialog.h
 * Copyright (c) 2012 ROBLOX Corp. All Rights Reserved.
 */

#pragma once 

// Qt Headers
#include <QDialog>
#include <QTextDocument>
#include <QRegExp>
#include <QComboBox>

#include "RobloxFindWidget.h"

class QLineEdit;
class QPushButton;
class QRadioButton;
class QCheckBox;
class QGroupBox;
class QLabel;
class QGridLayout;

class IFindListener
{
public:

    /**
     * Finds the item after the current cursor location.
     *   The cursor will be moved to the new item and the item selected.
     */
    virtual bool find(bool loop,const QRegExp& regExp,QTextDocument::FindFlags flags) = 0;

	virtual void moveNext(bool forward) = 0;
    /**
     * Replaces the text for the current item.
     */
    virtual void replace(const QString& text,const QRegExp& regExp,QTextDocument::FindFlags flags) = 0;

	virtual void replaceNext(const QRegExp& rx, const QString& text) = 0;
	virtual void replaceAll(const QRegExp& rx, const QString& text) = 0;

    /**
     * Get the text at the current cursor location.
     *  The current text will be used to validate the selection and for populating the
     *  find dialog when shown.
     */
    virtual QString getSelectedText() = 0;

    /**
     * Checks if there is a current selected item.
     */
    virtual bool hasSelection() = 0;

    /**
     * Move the current item to the beginning.
     */
    virtual void goToStart(QTextDocument::FindFlags flags) = 0;

	/**
	 * Go to line
	 */
	virtual void moveCursorToLine(int line) = 0;
};

class BasicFindDialog : public QDialog 
{
    Q_OBJECT

public:
	virtual ~BasicFindDialog();

	void setFindListener(IFindListener* listener);
	bool isFindNextEnabled();

    bool find(bool forward,bool loop);

    bool getSearchParameters(bool forward,QRegExp& outRegExp,QTextDocument::FindFlags& outflags);

public Q_SLOTS:

    virtual void setFocus();

protected Q_SLOTS:
	
    void findNext() { find(true,true); }
    void findPrevious() { find(false,true); }
	
protected:
    BasicFindDialog(QWidget *parent = 0);    

	virtual void keyPressEvent(QKeyEvent* e);

protected Q_SLOTS:
    virtual void textToFindChanged();
	virtual void setVisible(bool);

protected:
    IFindListener   *m_pFindListener;
	QLineEdit		*m_pFindEdit;
	QPushButton		*m_pFindButton;
	QPushButton		*m_pFindNextButton;
    QPushButton		*m_pFindPreviousButton;
	QPushButton		*m_pCloseButton;
	QRadioButton	*m_pDownRadioButton;
	QCheckBox		*m_pCaseCheckBox;
	QCheckBox		*m_pWholeCheckBox;
	QCheckBox		*m_pRegExCheckBox;
};

class FindAllDialog: public BasicFindDialog
{
	Q_OBJECT
public:
	FindAllDialog(QWidget *parent);

	void showError(const QString& error);

	void setFindAllRunning(bool value);

private Q_SLOTS:
	void findAll();
	void onFindOptionsToggled(bool on);

	virtual void textToFindChanged();
	virtual void adjustSize() { QWidget::adjustSize(); }

private:

	RobloxFindWidget::eFindFlags getCurrentFlags();

	void initDialog();

	QComboBox		*m_pLookInComboBox;
	QPushButton		*m_pFindAllButton;
	QGroupBox		*m_pFindOptions;
    QGridLayout     *m_pFindLayout;

protected:
    void showEvent(QShowEvent * event);
};

class FindDialog : public BasicFindDialog
{
	Q_OBJECT

public:
	FindDialog(QWidget *parent);
	void initDialog();
};

class ReplaceDialog : public BasicFindDialog
{
	Q_OBJECT

public:
	ReplaceDialog(QWidget *parent);

	void initDialog();

private Q_SLOTS:
    void replace();
    void replaceAll();
	virtual void textToFindChanged();

private:
	bool isSelectionValid();

	QLineEdit		*m_pReplaceEdit;
	QPushButton		*m_pReplaceButton;
	QPushButton		*m_pReplaceAllButton;
};

class EmbeddedFindReplaceWidget : public QDialog
{
	Q_OBJECT

public:
	EmbeddedFindReplaceWidget(QWidget *parent);

	void initDialog();

	void setFindListener(IFindListener* listener) { m_pFindListener = listener; }

	void updateFindPosition(QPoint position, QSize size);
	
	void activate(bool showReplace = false);

	QRegExp getSearchString();

	Q_INVOKABLE void setNumberOfFoundResults(int number);

	virtual void keyPressEvent(QKeyEvent* evt);

private Q_SLOTS:
	void find();
	void findNext();
	void findPrevious();
	void replaceNext();
	void replaceAll();

	void onCloseDialog();

	void toggleReplaceVisible();

private:
	bool			m_replaceVisible;

	IFindListener	*m_pFindListener;

	QComboBox		*m_pFindComboBox;
	QComboBox		*m_pReplaceComboBox;

	QPushButton		*m_pToggleReplaceButton;
	QPushButton		*m_pFindNextButton;
	QPushButton		*m_pFindPreviousButton;
	QPushButton		*m_pCloseDialogButton;
	QPushButton		*m_pReplaceNextButton;
	QPushButton		*m_pReplaceAllButton;
	QPushButton		*m_pMatchCaseButton;
	QPushButton		*m_pMatchWholeWordButton;
	QPushButton		*m_pRegularExpressionButton;

	QLabel			*m_pNumberOfResultsFound;
};

class GoToLineDialog : public QDialog
{
	Q_OBJECT

public:
	GoToLineDialog(QWidget *parent);

	void initDialog();

	void activate(int currentLine, int totalLines);

	void setFindListener(IFindListener* listener) { m_pFindListener = listener; }

private Q_SLOTS:
	void goToLine();

private:

	IFindListener	*m_pFindListener;

	QLabel			*m_pLineNumberLabel;
	QLineEdit		*m_pLineNumberEdit;
	QPushButton		*m_pOkButton;
	QPushButton		*m_pCancelButton;

	QIntValidator	*m_pNumberValidator;

	int				lineCount;
};

class FindReplaceProvider
{	
public:
	static FindReplaceProvider& instance();

	FindDialog* getFindDialog() { RBXASSERT(m_pFindDialog); return m_pFindDialog; }
	ReplaceDialog* getReplaceDialog() { RBXASSERT(m_pReplaceDialog); return m_pReplaceDialog; }
	FindAllDialog* getFindAllDialog() { RBXASSERT(m_pFindAllDialog); return m_pFindAllDialog; }
	GoToLineDialog* getGotoLineDialog() { RBXASSERT(m_pGotoLineDialog); return m_pGotoLineDialog; }
	EmbeddedFindReplaceWidget* getEmbeddedFindDialog() { RBXASSERT(m_pEmbeddedFindReplaceWidget); return m_pEmbeddedFindReplaceWidget; }

	void setFindListener(IFindListener* listener);
	bool isFindNextEnabled();

	void find(bool forward);

	void setSearchStr(const QString& str) { m_searchStr = str; }
	const QString& getSearchStr() { return m_searchStr; }

private:
	FindReplaceProvider();
	~FindReplaceProvider();

	FindDialog*					m_pFindDialog;
	ReplaceDialog*				m_pReplaceDialog;
	FindAllDialog*				m_pFindAllDialog;
	GoToLineDialog*				m_pGotoLineDialog;
	EmbeddedFindReplaceWidget*	m_pEmbeddedFindReplaceWidget;
	QString						m_searchStr;
};
