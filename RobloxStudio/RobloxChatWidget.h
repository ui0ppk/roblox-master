/**
 * RobloxChatWidget.h
 * Copyright (c) 2015 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
#define QT_NO_KEYWORDS
#endif

#include <QListWidget>
#include <QItemDelegate>
#include <QLineEdit>
#include <QPair>
#include <QQueue>

#include "rbx/signal.h"
#include "Util/HttpAsync.h"

namespace RBX {
	class DataModel;
	namespace Network {
		class ChatMessage;
	}
}

class RobloxChatWidget;
class PlayersDataManager;

namespace StudioChat {
	enum ChatType
	{
		All,
		Whisper,
		Game
	};

	struct ChatMessageData 
	{
		ChatType                         chatType;
		QString                          message;
		QString                          playerName;
		int                              playerId;
		bool                             isLocalPlayer;

		ChatMessageData()
		{;}

		ChatMessageData(ChatType chatType_, QString message_)
		: chatType(chatType_), message(message_), playerId(-1), isLocalPlayer(false)
		{;}
	};
}

class ChatMessageItem: public QObject, public QListWidgetItem
{
	Q_OBJECT
public:
	ChatMessageItem(QObject* parent);

	QSize sizeHint(const QStyleOptionViewItem& option, const QSize& viewportSize) const;
	void paint(QPainter* painter, const QStyleOptionViewItem& option);

private Q_SLOTS:
	void updatePlayerAvatar(const QImage& image);

private:
	static QPolygon getMessageBoxPolygon(QRect& boundingRect, bool pointingRight);
};

class ChatMessageDelegate: public QItemDelegate
{
public:
	ChatMessageDelegate(QObject* parent);

	enum Role
	{
		SubLabelRole = Qt::UserRole,
		SubLabelDecorationPath,
		SubLabelDecorationRole
	};

private:
	/*override*/ QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const;
	/*override*/ void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const;
};

class ChatMessageListWidget: public QListWidget
{
public:
	ChatMessageListWidget(QWidget* parent, boost::shared_ptr<PlayersDataManager> playerDataManager);

	QListWidgetItem* addMessageItem(StudioChat::ChatMessageData messageData);
	QListWidgetItem* addMessageStatusItem(const QString& statusMessage);
	QListWidgetItem* addBroadcastItem(const QString& message, bool hasError = false);

	using QListWidget::itemFromIndex;

private:
	void insertAndScrollToItem(QListWidgetItem* pItem);

	boost::shared_ptr<PlayersDataManager>   m_pPlayersDataManager;
};

class ChatMessageLineEdit: public QLineEdit
{
	Q_OBJECT
public:
	ChatMessageLineEdit(QWidget* parent, boost::shared_ptr<PlayersDataManager> playersDataManager);

	QString getText();
	StudioChat::ChatType getChatType() { return m_chatType; }

Q_SIGNALS:
	void returnPressed(int chatType, const QString& message, const QString& targetPlayerName);

private Q_SLOTS:
	void onTextEdited(const QString& message);
	void onReturnPressed();

private:
	boost::shared_ptr<PlayersDataManager>   m_pPlayersDataManager;
	StudioChat::ChatType                    m_chatType;
	QString                                 m_prefixString;
};

class RobloxChatWidget: public QFrame
{
	Q_OBJECT
public:
	RobloxChatWidget(QWidget* parent, boost::shared_ptr<RBX::DataModel> dataModel, boost::shared_ptr<PlayersDataManager> playerDataManager);

private Q_SLOTS:
	void onReturnPressed(int chatMode, const QString& message, const QString& targetPlayerName);

private:
	Q_INVOKABLE void addMessage(StudioChat::ChatMessageData messageData);

	void onPlayerChatMessage(const RBX::Network::ChatMessage& chatMessage);

	void sendMessage(const StudioChat::ChatMessageData& messageData, QString& errorMessage);
	void updateSentMessageStatus(const QString& message);

	void showEvent(QShowEvent* evt);

	void setToggleChatActionPixmap(const QPixmap& pixmap);
	void updateToggleChatActionPixmap();

	QQueue<QPair<QListWidgetItem*, QListWidgetItem*> > m_sentMessages;

	ChatMessageListWidget*                 m_pChatListWidget;
	ChatMessageLineEdit*                   m_pMessageBox;

	boost::shared_ptr<RBX::DataModel>      m_pDataModel;
	boost::shared_ptr<PlayersDataManager>  m_pPlayersDataManager;

	rbx::signals::scoped_connection        m_cPlayerChatMessage;

	QPixmap                                m_OriginalChatActionPixmap;
};
