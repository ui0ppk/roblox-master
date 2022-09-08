/**
 * FindWidget.h
 * Copyright (c) 2014 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

#include <QObject>
#include <QToolButton>
#include <QListView>
#include <QMutex>
#include <QTextCursor>
#include <QTextDocument>

#include "LuaSourceBuffer.h"
#include "Script/Script.h"

class FindData
{
public:
	FindData()
		: m_line(0)
		, m_lineText("")
	{}
	FindData(QString text)
		: m_line(0)
		, m_lineText(text)
	{}
	FindData(LuaSourceBuffer instance,
			 int lineNumber,
			 QString text,
			 QString hierarchy = QString())
		: m_instance(instance)
		, m_line(lineNumber)
		, m_lineText(text)
		, m_hierarchy(hierarchy)
	{}

	void setPlaceData(int position, int length) { m_position = position; m_length = length; }

	int getPosition() { return m_position; }
	int getLength() { return m_length; }

	Qt::ItemFlags flags() const;

	QString toString() const;

	LuaSourceBuffer getInstance() { return m_instance; }

	int getLine() { return m_line; }

private:
	LuaSourceBuffer         m_instance;
	int						m_line;
	QString					m_lineText;
	QString					m_hierarchy;

	int						m_position;
	int						m_length;

};

Q_DECLARE_METATYPE(FindData)

class FindListModel : public QAbstractListModel
{
	Q_OBJECT
public:
	FindListModel();

	void addItem(FindData item);
	void addItems(QList<FindData> items) { m_FindList.append(items); }

	void updateEntireView();

	void clear();

	virtual int rowCount( const QModelIndex& parent = QModelIndex() ) const;

	virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const;
	Qt::ItemFlags flags(const QModelIndex& index) const;

public Q_SLOTS:
	void updateView();

	int openScript(int row);

private:

	mutable QMutex				m_listMutex;

	bool						m_stopUpdater;
	bool						m_runningUpdater;
	int							m_loadedIndex;

	int							m_previousCount;

	QList<FindData>				m_FindList;
};

class RobloxFindWidget : public QWidget
{
	Q_OBJECT
public:
	RobloxFindWidget(QWidget *pParent = 0);
	~RobloxFindWidget();

	static RobloxFindWidget& singleton();

	enum eFindFlags
	{
		NO_FLAGS   = 1 << 0,
		MATCH_CASE = 1 << 1,
		MATCH_WORD = 1 << 2,
		REG_EXP    = 1 << 3,
	};
	inline friend eFindFlags operator|(eFindFlags a, eFindFlags b) { return static_cast<eFindFlags>(static_cast<int>(a) | static_cast<int>(b)); }

	void findAll(const QString& findStr, bool allScripts, eFindFlags flags);
	void dataModelClosing();

protected:
	virtual bool event(QEvent* e);

Q_SIGNALS:
	void populatingWidgetFinished();

protected Q_SLOTS:
	void openItem(const QModelIndex & index);

	void openCurrentItem();
	void previousItem();
	void nextItem();
	void clearItems();

	void resetItems();
	void stopPopulatingItems();

	void addItem(FindData item);

private:
	void initWidget();

	void populateWidget(shared_ptr<RBX::DataModel> dm, const QString& findStr, eFindFlags flags, bool allScripts);

	void populateScriptList(bool allScripts);

	QTextCursor findNextInScript(const QTextDocument& doc, const QString& findStr, const QTextCursor& cursor, QTextDocument::FindFlags flags, bool regExp);

	void getNextScript(int index, shared_ptr<RBX::DataModel> dm, const QString& findStr, eFindFlags flags);
	void searchScriptChain(int index, const QString& sourceText, shared_ptr<RBX::DataModel> dm, const QString& findStr, eFindFlags flags, const QString& heirarchy);

	QString getInstanceHierarchy(shared_ptr<RBX::Instance> script);
	
	enum eDataRole
	{
		SCRIPT_ROLE	= Qt::UserRole + 1,
		POS_ROLE	= Qt::UserRole + 2,
		LENGTH_ROLE = Qt::UserRole + 3
	};

	QToolButton *m_pFindInAllScriptsButton;
	QToolButton *m_pGotoLocationButton;
	QToolButton *m_pPreviousButton;
	QToolButton *m_pNextButton;
	QToolButton *m_pClearButton;
	QToolButton *m_pStopButton;

	int								m_findListIndex;

	QListView*						m_pListView;

	QList<LuaSourceBuffer>          m_scriptList;

	int								m_matchesFound;
	int								m_matchingScripts;
	int								m_searchedScripts;

	int								m_totalAttempted;

	bool							m_stopPopulating;

	FindListModel*					m_pFindListModel;
};
