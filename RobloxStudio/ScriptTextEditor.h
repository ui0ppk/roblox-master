/**
 * ScriptTextEditor.h
 * Copyright (c) 2013 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
    #define QT_NO_KEYWORDS
#endif

// Standard C/C++ Headers
#include <map>

// Qt Headers
#include <QPlainTextEdit>
#include <QThread>
#include <QToolTip>
#include <QMutex>
#include <QWaitCondition>
#include <QListWidgetItem>
#include <QClipboard>

// Roblox Headers
#include "rbx/BaldPtr.h"
#include "rbx/signal.h"
#include "reflection/Property.h"

#include "script/ScriptAnalyzer.h"

// Roblox Studio Headers
#include "FindDialog.h"

#include "RobloxContextualHelp.h"
#include "rbx/DenseHash.h"
#include "boost/optional/optional.hpp"

class RobloxScriptDoc;
class RobloxMainWindow;
class ScriptSyntaxHighlighter;
class ScriptSideWidget;
class ScriptTextEditor;

/*****************************************************************************/
// ScriptEditorUtils
/*****************************************************************************/

class ScriptEditorUtils
{
public:

	static QTextBlock findFoldBoundary(QTextBlock endBlock, bool bUp);
	static int noOfStartingTabs(QTextBlock block);
};

/*****************************************************************************/
// ScriptTraversalThread
/*****************************************************************************/

class ScriptTraversalThread : public QThread
{
	Q_OBJECT

public:
	ScriptTraversalThread(ScriptTextEditor * parent, RobloxScriptDoc * document);
	virtual ~ScriptTraversalThread();

	void requestRun();

private:
	void run();

protected:

	virtual void doWork() = 0;

	QMutex				mutex;
	QWaitCondition		condition;
	ScriptTextEditor*	m_pScriptEditor;
	RobloxScriptDoc*	m_pDocument;
	bool                restart;
	bool                abort;
};

/*****************************************************************************/
// CheckSyntaxThread
/*****************************************************************************/

class CheckSyntaxThread : public ScriptTraversalThread
{
    Q_OBJECT

public:
    CheckSyntaxThread(ScriptTextEditor *parent, RobloxScriptDoc *document)
		: ScriptTraversalThread(parent, document)
	{}

private:
    void doWork();
};

/*****************************************************************************/
// FindThread
/*****************************************************************************/

class FindThread : public ScriptTraversalThread
{
	Q_OBJECT

public:

	FindThread(ScriptTextEditor *parent, RobloxScriptDoc *document)
		: ScriptTraversalThread(parent, document)
		, m_goToNextItem(true)
		, m_redrawRequested(false)
	{}
		
	void setData(const QRegExp& regExp,QTextDocument::FindFlags flags);

	void setAutoFindNext(bool value) { m_goToNextItem = value; }

private:

	QRegExp toSearch;
	QTextDocument::FindFlags searchFlags;

	void doWork();

	bool findInBlockNoCursor(
		const QString&				text, 
		int							startOfBlock,
		const QRegExp&              expression, 
		int                         offset,
		QTextDocument::FindFlags    options, 
		int&						start,
		int&						end);
	
	int					m_searchStartBlockLine;
	int					m_searchLastBlockLine;

	bool				m_goToNextItem;

	bool				m_redrawRequested;
};

/*****************************************************************************/
// ScriptTextEditor
/*****************************************************************************/

class ScriptTextEditor : public QPlainTextEdit, public IFindListener
{
	Q_OBJECT

public:

	ScriptTextEditor(RobloxScriptDoc *pScriptDoc,RobloxMainWindow *pParentWidget);
	~ScriptTextEditor();

	Q_INVOKABLE void setErrorLine(int errorLine, QString errorMessage);
	Q_INVOKABLE void setAnalysisResult(const RBX::ScriptAnalyzer::Result& analysisResults);
	Q_INVOKABLE void startFind();

	void clearFoundItems();
	void updateFindSelection();

	virtual void focusOutEvent(QFocusEvent * evt);
    virtual void focusInEvent(QFocusEvent * evt);

	void setViewportMargins(int left, int top, int right, int bottom);
	QTextBlock firstVisibleBlock() const;
	QPointF	contentOffset() const;
	QRectF blockBoundingGeometry(const QTextBlock &block) const;
    void ensureVisible(const QTextBlock& block);
    void updateFolds();

    // action handling
    bool doHandleAction(const QString& actionID);
    bool actionState(const QString& actionID,bool& enableState,bool& checkedState);

	QMenu* getContextualMenu() { return m_ContextMenu; }

	void moveCursorToLine(int line);

	void selectText(int startPosition, int endPosition);
	void selectText(const RBX::ScriptAnalyzer::Location& location, bool trySettingBlocksVisible = true);

	bool addToExtraSelections(const QTextEdit::ExtraSelection& extraSelection, bool prepend = false);
	void removeFromExtraSelections(QList<QTextEdit::ExtraSelection> extraSelections);

	virtual void keyPressEvent(QKeyEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);

	void updateEmbeddedFindPosition();
	
	void addFoundItem(int start, int end);
	
	// Wiki Search
	static void setupWikiLookup();
	static void cleanupWikiLookup()
	{
		wikiLookup.clear();
	}

	static bool isWordCharacter(const QChar& c);

	int numberOfFoundItems() { return m_foundItems.size(); }

Q_SIGNALS:
	void toggleBreakpoint(int lineNumber);
	bool showToolTip(const QPoint& pos, const QString& wordUnderCursor);
	void updateContextualMenu(QMenu* pMenu, QPoint pos);
	void helpTopicChanged(const QString& topic);

protected:

	virtual void resizeEvent(QResizeEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);
	virtual bool event(QEvent *e);
    virtual void wheelEvent(QWheelEvent* event);

    /**
     * IFindListener
     */
    virtual bool find(bool loop,const QRegExp& regExp,QTextDocument::FindFlags flags);
    virtual void replace(const QString& text,const QRegExp& regExp,QTextDocument::FindFlags flags);
    virtual QString getSelectedText();
    virtual bool hasSelection();
    virtual void goToStart(QTextDocument::FindFlags flags);

	virtual void moveNext(bool foward);

	virtual void replaceNext(const QRegExp& rx, const QString& text);
	virtual void replaceAll(const QRegExp& rx, const QString& text);

	void homeEvent(QKeyEvent* e);
	void endEvent(QKeyEvent* e);

public Q_SLOTS:
	
	void onCheckSyntax();

private Q_SLOTS:
	void checkSyntax();

    void intellesenseDoubleClick(QListWidgetItem* listItem);
	void updateCursorVisibility();

	void selectionChangedSearch();
	void onSearchTimeout();
    
    void onCheckSyntaxTimerTimeOut();

	void find();

	void onFind();

	void findNext(bool moveCursorToBeginningOfSelection = false);

private:

    // actions
	void commentSelection();    
	void uncommentSelection();
    void toggleCommentSelection();
	void updateSelection(bool* isBlockUnfolded = NULL);
    void goToError();
    void expandAllFolds();
    void collapseAllFolds();
	void goToLine();
    void findPrevious();
    void replace();
    void zoom(int delta, bool setDeltaAsValue = false);

    void replaceText(QString text, int cursorPos);

	void processTooltip(QHelpEvent* evt);
	bool tabSelection(bool bPrepend);
	void autoIndent();
    void replaceSelection(int startPos,int endPos,const QString& text);
    void expandAllFolds(bool expand);

    void onPropertyChanged(const RBX::Reflection::PropertyDescriptor* pDescriptor);

    bool isPasteEnabled();

    int getDocFoldState(int blockNumber = 0);
    void autoCompleteBlock(const QString& text, QString& indent);

    bool findInBlock(
        const QTextBlock&           block, 
        const QRegExp&              expression, 
        int                         offset,
        QTextDocument::FindFlags    options, 
        QTextCursor&                cursor );

    QTextCursor findInDoc(
        const QTextDocument&        doc,
        const QRegExp&              expr,
        const QTextCursor&          from,
        QTextDocument::FindFlags    options );

	QString constructStyleSheet();

	QString getVariableAtPos(const QPoint& location);
	int variableStartPos(const QString &text, int startPos);
	int variableEndPos(const QString &text, int endPos);

	void updateViewAndCursor(bool resizeRequired);

	bool appendExtraSelection(const QTextCharFormat& format, int startPos, int endPos, bool atEof, QList<QTextEdit::ExtraSelection>& appendList);
	void populateValuesFromLocation(const RBX::ScriptAnalyzer::Location& location, int& startPos, int& endPos, bool& atEof);

	int getStartPosition(const RBX::ScriptAnalyzer::Location& location);
	int getEndPosition(const RBX::ScriptAnalyzer::Location& location);

	int						                m_visibleErrorLine;     //!< line that the error is being displayed on
    int                                     m_errorLine;            //!< actual error line
	QString					                m_errorMessage;

    struct ScriptMessage
	{
        int code;
        QString text;
		int begin;
		int end;
	};

	RBX::BaldPtr<RobloxScriptDoc>           m_pScriptDoc;
	RBX::BaldPtr<ScriptSyntaxHighlighter>   m_pSyntaxHighlighter;
	RBX::BaldPtr<ScriptSideWidget>          m_pSideWidget;
	RBX::BaldPtr<CheckSyntaxThread>         m_pCheckSyntaxThread;
	FindThread*								m_pFindThread;
    RBX::BaldPtr<RobloxMainWindow>          m_pMainWindow;
    RBX::BaldPtr<QMenu>                     m_ContextMenu;
    rbx::signals::scoped_connection         m_PropertyChangedConnection;
    QTextCharFormat                         m_ErrorFormat;
    QTextCharFormat                         m_WarningFormat;
    
    QTimer*                                 m_pCheckSyntaxTimer;

	int										m_originalCursorPosition;

	QTimer*									m_pWikiSearchTimer;
	QString									currentWikiSearchSelection;

	QList<QTextEdit::ExtraSelection>        m_AnalysisSelections;

	QMutex									m_selectionMutex;

	QTextCharFormat							m_findSelectionFormat;
	

	std::map<int, int>								m_extraSelectionsToAdd;
	std::multimap<int, QTextEdit::ExtraSelection>	m_extraSelectionsToRemove;
	QList<QTextEdit::ExtraSelection>				m_currentSelections;
	QList<QTextEdit::ExtraSelection>				m_currentFoundItems;

	boost::optional<std::vector<RBX::ScriptAnalyzer::IntellesenseResult> >			m_analysisResult;

	std::map<int, int>	m_foundItems;

	typedef std::pair<int, int> Range;
	struct RangeCompare
	{
		bool operator()(const Range& lRange, const Range& rRange) const
		{   
			return lRange.second < rRange.first;
		} 
	};
	typedef std::multimap<Range, ScriptMessage, ScriptTextEditor::RangeCompare> MessageCollection;
    MessageCollection    m_scriptMessageForRange;

	static void updateWikiLookup(const char*, std::string);

	typedef RBX::DenseHashMap<const char*, std::pair<int, std::string>, RBX::Reflection::StringHashPredicate, RBX::Reflection::StringEqualPredicate> WikiLookup;
	static WikiLookup						wikiLookup;
};


