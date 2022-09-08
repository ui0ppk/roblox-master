/**
 * RobloxChatWidget.cpp
 * Copyright (c) 2015 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxChatWidget.h"

#include <QFont>
#include <QPainter>
#include <QScrollBar>

#include "V8DataModel/DataModel.h"
#include "Network/Players.h"
#include "Network/Player.h"

#include "UpdateUIManager.h"
#include "RobloxMainWindow.h"
#include "PlayersDataManager.h"

static const int kPlayerIconSize  = 32;
static const int kMessageBoxHMargin = 8;
static const int kMessageBoxVMargin = 6;
static const int kMessageBoxVOuterMargin = 3;
static const int kIconMessageBoxDistance = 4;
static const int kPointingTriSize = 5;

static const QString kLockImagePath(":/16x16/images/Studio 2.0 icons/16x16/lock.png");
static const QSize   kMinChatWidgetSize(200, 150);

//--------------------------------------------------------------------------------------------
// ChatMessageItem
//--------------------------------------------------------------------------------------------
ChatMessageItem::ChatMessageItem(QObject* parent)
: QObject(parent)
{}

QSize ChatMessageItem::sizeHint(const QStyleOptionViewItem& option, const QSize& viewportSize) const
{
	QFontMetrics fontMetrics(option.font);

	QString headerText(data(Qt::DisplayRole).toString());
	QString sublabelText(data(ChatMessageDelegate::SubLabelRole).toString());
	bool pointingRight = data(Qt::TextAlignmentRole).toInt() == Qt::AlignRight;

	int avatarRelatedWidth = (pointingRight || sublabelText.isEmpty()) ? 0 : (kPlayerIconSize + kIconMessageBoxDistance + kMessageBoxHMargin);
	int availableWidth = viewportSize.width() -  avatarRelatedWidth - kMessageBoxHMargin - 2*kMessageBoxHMargin - kPointingTriSize;
	QRectF bRect = fontMetrics.boundingRect(kMessageBoxHMargin, 0, availableWidth, 0, Qt::AlignLeft | Qt::TextWordWrap, headerText);

	return QSize(viewportSize.width(), bRect.height() + (sublabelText.isEmpty() ? 0 : fontMetrics.height()) + 2*kMessageBoxVOuterMargin + 2*kMessageBoxVMargin);
}

void ChatMessageItem::paint(QPainter* painter, const QStyleOptionViewItem& option)
{
	// adjust rectangle for text drawing
	QRect adjustedRect = option.rect.adjusted(kMessageBoxHMargin, kMessageBoxVOuterMargin, -kMessageBoxHMargin, -kMessageBoxVOuterMargin);

	// draw player thumbnail (assuming we show icon only for the left aligned items)
	QImage icon(qvariant_cast<QImage>(data(Qt::DecorationRole)));
	if (!icon.isNull())
	{
		painter->save();
		QPainterPath path;
		path.addEllipse(adjustedRect.topLeft().x(), adjustedRect.topLeft().y(), kPlayerIconSize, kPlayerIconSize);
		painter->setRenderHints(QPainter::Antialiasing);
		painter->setClipPath(path);
		painter->drawImage(QPoint(adjustedRect.topLeft().x(), adjustedRect.topLeft().y()), icon);
		adjustedRect.setLeft(adjustedRect.left() + kPlayerIconSize + kIconMessageBoxDistance);
		painter->restore();
	}

	painter->save();
	// construct text rectangle
	QString messageText(data(Qt::DisplayRole).toString());
	QString sublabelText(data(ChatMessageDelegate::SubLabelRole).toString());
	QString subLabelImagePath(data(ChatMessageDelegate::SubLabelDecorationPath).toString());

	QFont subLabelFont = option.font;
	subLabelFont.setPointSize(subLabelFont.pointSize() - 3);
	subLabelFont.setBold(true);

	int subLabelImageHeight = QFontMetrics(subLabelFont).height();
	int messageTextWidth = QFontMetrics(option.font).width(messageText) + 2;
	int subLabelTextWidth = (!sublabelText.isEmpty() ? QFontMetrics(subLabelFont).width(sublabelText) : 0) + 
			                (!subLabelImagePath.isEmpty() ? (subLabelImageHeight + kMessageBoxHMargin) : 0);
	int textRectWidth = qMax(messageTextWidth, subLabelTextWidth);
	int adjustedWidth = textRectWidth + (2*kMessageBoxHMargin + kPointingTriSize);

	bool pointingRight = data(Qt::TextAlignmentRole).toInt() == Qt::AlignRight;
	if (pointingRight)
	{
		if (adjustedRect.width() > adjustedWidth)
			adjustedRect.setLeft(adjustedRect.right() - textRectWidth - 2*kMessageBoxHMargin);
	}
	else
	{
		adjustedRect.setWidth(adjustedWidth > adjustedRect.width() ? adjustedRect.width() : 
				                                                        textRectWidth +  2*kMessageBoxHMargin + kPointingTriSize);
	}

	// draw shadow rect
	QRect shadowRect(adjustedRect);
	pointingRight ? shadowRect.setRight(shadowRect.right() - 6) : shadowRect.setLeft(shadowRect.left() + 5);
	QColor gray(Qt::gray);
	painter->fillRect(pointingRight ? shadowRect.translated(-1, 1) : shadowRect.translated(1, 1), QBrush(gray.lighter(120)));

	// draw message polygon
	QPainterPath path;
	path.addPolygon(getMessageBoxPolygon(adjustedRect, pointingRight));
	painter->fillPath(path, pointingRight ? QBrush(QColor("#99daff")) : QBrush(QColor("#e3e3e3")));

	// draw message text	
	if (!sublabelText.isEmpty())
	{
		adjustedRect.adjust(kMessageBoxHMargin, kMessageBoxVMargin, -kMessageBoxHMargin, -kMessageBoxVMargin);
		QRect subLabelRect(adjustedRect);

		if (!subLabelImagePath.isEmpty())
		{
			// if image is not set already, load image
			QImage image = qvariant_cast<QImage>(data(ChatMessageDelegate::SubLabelDecorationRole));
			if (image.isNull())
			{
				image = QImage(subLabelImagePath).scaled(QSize(subLabelImageHeight, subLabelImageHeight), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
				setData(ChatMessageDelegate::SubLabelDecorationRole, image);
			}

			if (pointingRight)
			{
				painter->drawImage(QPoint(subLabelRect.topRight().x() - subLabelTextWidth - 6, subLabelRect.topLeft().y()), image);
				subLabelRect.setLeft(subLabelRect.topRight().x() - subLabelTextWidth + subLabelImageHeight);
			}
			else
			{
				painter->drawImage(QPoint(subLabelRect.topLeft().x(), subLabelRect.topLeft().y()), image);
				subLabelRect.setLeft(subLabelRect.topLeft().x() + subLabelImageHeight + 6);
			}
		}

		painter->setPen(QPen(QColor(Qt::black)));
		painter->setFont(subLabelFont);
		painter->drawText(subLabelRect, Qt::AlignLeft | Qt::TextWordWrap, sublabelText);

		adjustedRect.adjust(0, 3*kMessageBoxVMargin, 0, 0);
		// right align message text if sublabel message is longer than the message
		if (pointingRight && (subLabelTextWidth > messageTextWidth))
			adjustedRect.setLeft(subLabelRect.topRight().x() - messageTextWidth - kMessageBoxHMargin);
	}
	else
	{
		adjustedRect.adjust(kMessageBoxHMargin, kMessageBoxVMargin, -kMessageBoxHMargin, -kMessageBoxVMargin);
	}

	painter->setFont(option.font);
	painter->setPen(QColor(Qt::black));
	painter->drawText(adjustedRect, Qt::AlignLeft | Qt::TextWordWrap, messageText);

	painter->restore();
}

void ChatMessageItem::updatePlayerAvatar(const QImage& playerImage)
{	setData(Qt::DecorationRole, playerImage); }

QPolygon ChatMessageItem::getMessageBoxPolygon(QRect& boundingRect, bool pointingRight)
{
	QPolygon polygon;
	const int pointingTriOffet = boundingRect.top() + 2*kMessageBoxVMargin;

	if (pointingRight)
	{
		polygon << QPoint(boundingRect.right(), pointingTriOffet) << QPoint(boundingRect.right() - kPointingTriSize, pointingTriOffet + kPointingTriSize) 
				<< QPoint(boundingRect.right() - kPointingTriSize, boundingRect.bottom()) << boundingRect.bottomLeft() << boundingRect.topLeft() 
				<< QPoint(boundingRect.right() - kPointingTriSize, boundingRect.top()) << QPoint(boundingRect.right() - kPointingTriSize, pointingTriOffet - kPointingTriSize);
		boundingRect.adjust(-kPointingTriSize, 0, 0, 0);
	}
	else
	{
		polygon << QPoint(boundingRect.left(), pointingTriOffet) << QPoint(boundingRect.left() + kPointingTriSize, pointingTriOffet + kPointingTriSize) 
				<< QPoint(boundingRect.left() + kPointingTriSize, boundingRect.bottom()) << boundingRect.bottomRight() << boundingRect.topRight() 
				<< QPoint(boundingRect.left() + kPointingTriSize, boundingRect.top()) << QPoint(boundingRect.left() + kPointingTriSize, pointingTriOffet - kPointingTriSize);
		boundingRect.adjust(kPointingTriSize, 0, 0, 0);
	}

	return polygon;
}

//--------------------------------------------------------------------------------------------
// ChatMessageStatusItem
//--------------------------------------------------------------------------------------------
class ChatMessageStatusItem: public QListWidgetItem
{
public:
	ChatMessageStatusItem(const QString& messageStatus)
	{
		setText(messageStatus);
	}

	QSize sizeHint(const QStyleOptionViewItem& option, const QSize& viewportSize) const
	{
		QFont updatedFont(option.font);
		updatedFont.setPointSize(option.font.pointSize() - 2);

		QFontMetrics fontMetrics(updatedFont);
		return QSize(viewportSize.width(), fontMetrics.height() + 2*kMessageBoxVMargin);
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option)
	{
		painter->save();
		QFont updatedFont(option.font);
		updatedFont.setPointSize(option.font.pointSize() - 2);
		painter->setFont(updatedFont);

		QRect adjustedRect = option.rect.adjusted(kMessageBoxHMargin, kMessageBoxVOuterMargin, -kMessageBoxHMargin, -kMessageBoxVOuterMargin);
		painter->drawText(adjustedRect, Qt::AlignRight | Qt::TextWordWrap, data(Qt::DisplayRole).toString());
		painter->restore();
	}
};

//--------------------------------------------------------------------------------------------
// ChatBroadcastItem
//--------------------------------------------------------------------------------------------
class ChatBroadcastItem: public QListWidgetItem
{
public:
	ChatBroadcastItem(const QString& message, bool hasError = false)
		: m_bHasError(hasError)
	{
		setText(message);
	}

	QSize sizeHint(const QStyleOptionViewItem& option, const QSize& viewportSize) const
	{
		QFont updatedFont(option.font);
		updatedFont.setPointSize(option.font.pointSize() - 1);
		QFontMetrics fontMetrics(updatedFont);

		int availableWidth = viewportSize.width() - 2*kMessageBoxHMargin - 2*kMessageBoxHMargin - kPointingTriSize;
		QRectF bRect = fontMetrics.boundingRect(kMessageBoxHMargin, 0, availableWidth, 0, Qt::AlignLeft | Qt::TextWordWrap, data(Qt::DisplayRole).toString());

		return QSize(viewportSize.width(), bRect.height() + 2*kMessageBoxVMargin + 2*kMessageBoxVOuterMargin);
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option)
	{
		painter->save();
		QRect adjustedRect = option.rect.adjusted(kMessageBoxHMargin, kMessageBoxVOuterMargin, -kMessageBoxHMargin, -kMessageBoxVOuterMargin);

		// paint shadow rect
		QColor gray(Qt::gray);
		painter->fillRect(adjustedRect.translated(2, 1), QBrush(gray.lighter(120)));

		// paint enclosing rect (TODO: check the colors)
		painter->fillRect(adjustedRect, QBrush(m_bHasError ? QColor(255, 5, 100, 127) : QColor(85, 255, 127, 127)));

		QFont updatedFont(option.font);
		updatedFont.setPointSize(option.font.pointSize() - 1);
		painter->setFont(updatedFont);

		painter->setPen(QColor(Qt::black));
		adjustedRect = adjustedRect.adjusted(kMessageBoxHMargin, kMessageBoxVMargin, -kMessageBoxHMargin, -kMessageBoxVMargin);
		painter->drawText(adjustedRect, Qt::AlignLeft|Qt::TextWordWrap, data(Qt::DisplayRole).toString());
		painter->restore();
	}

private:
	bool             m_bHasError;
};

//--------------------------------------------------------------------------------------------
// ChatMessageDelegate
//--------------------------------------------------------------------------------------------
ChatMessageDelegate::ChatMessageDelegate(QObject* parent)
	: QItemDelegate(parent)
{
}

QSize ChatMessageDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{	
	ChatMessageListWidget* pListWidget(static_cast<ChatMessageListWidget*>(parent()));
	QListWidgetItem* pItem = pListWidget->itemFromIndex(index);

	if (ChatMessageItem* pMessageItem = dynamic_cast<ChatMessageItem*>(pItem))
		return pMessageItem->sizeHint(option, pListWidget->viewport()->size());

	if (ChatMessageStatusItem* pStatusItem = dynamic_cast<ChatMessageStatusItem*>(pItem))
		return pStatusItem->sizeHint(option, pListWidget->viewport()->size());

	if (ChatBroadcastItem* pBroadcastItem = dynamic_cast<ChatBroadcastItem*>(pItem))
		return pBroadcastItem->sizeHint(option, pListWidget->viewport()->size());

	return QItemDelegate::sizeHint(option, index);
}

void ChatMessageDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
	ChatMessageListWidget* pListWidget(static_cast<ChatMessageListWidget*>(parent()));
	QListWidgetItem* pItem = pListWidget->itemFromIndex(index);

	if (ChatMessageItem* pMessageItem = dynamic_cast<ChatMessageItem*>(pItem))
	{
		pMessageItem->paint(painter, option);
	}
	else if (ChatMessageStatusItem* pStatusItem = dynamic_cast<ChatMessageStatusItem*>(pItem))
	{
		pStatusItem->paint(painter, option);
	}
	if (ChatBroadcastItem* pBroadcastItem = dynamic_cast<ChatBroadcastItem*>(pItem))
	{
		return pBroadcastItem->paint(painter, option);
	}
}

//--------------------------------------------------------------------------------------------
// ChatMessageListWidget
//--------------------------------------------------------------------------------------------
ChatMessageListWidget::ChatMessageListWidget(QWidget* parent, boost::shared_ptr<PlayersDataManager> m_pPlayersDataManager)
: QListWidget(parent)
, m_pPlayersDataManager(m_pPlayersDataManager)
{
	setUniformItemSizes(false);
	setResizeMode(QListView::Adjust);
	setWordWrap(true);

	setItemDelegate(new ChatMessageDelegate(this));
}

QListWidgetItem* ChatMessageListWidget::addMessageItem(StudioChat::ChatMessageData messageData)
{
	ChatMessageItem* pItem = new ChatMessageItem(this);
	pItem->setText(messageData.message);

	if (messageData.isLocalPlayer)
	{
		if (messageData.chatType == StudioChat::Whisper)
		{
			pItem->setData(ChatMessageDelegate::SubLabelRole, QString("Me %1 %2").arg(QChar(0x2192)).arg(messageData.playerName));
			pItem->setData(ChatMessageDelegate::SubLabelDecorationPath, kLockImagePath);
		}

		pItem->setData(Qt::TextAlignmentRole, Qt::AlignRight);
	}
	else
	{
		if (messageData.chatType == StudioChat::Whisper)
		{
			pItem->setData(ChatMessageDelegate::SubLabelRole, QString("%1 %2 Me").arg(messageData.playerName).arg(QChar(0x2192)));
			pItem->setData(ChatMessageDelegate::SubLabelDecorationPath, kLockImagePath);
		}
		else
		{
			pItem->setData(ChatMessageDelegate::SubLabelRole, messageData.playerName);
		}

		// update of avatar is required only for remote player i.e. not for local player
		m_pPlayersDataManager->getPlayerAvatar(messageData.playerId, pItem, "updatePlayerAvatar");
	}

	addItem(pItem);

	// scroll if local player or if user at the end of the widget
	if (messageData.isLocalPlayer || (verticalScrollBar() && verticalScrollBar()->value() == verticalScrollBar()->maximum()))
		scrollToItem(pItem);

	return pItem;
}

QListWidgetItem* ChatMessageListWidget::addMessageStatusItem(const QString& statusMessage)
{
	QListWidgetItem* pStatusItem = new ChatMessageStatusItem(statusMessage);
	insertAndScrollToItem(pStatusItem);
	return pStatusItem;
}

QListWidgetItem* ChatMessageListWidget::addBroadcastItem(const QString& message, bool hasError)
{
	QListWidgetItem* pItem = new ChatBroadcastItem(message, hasError);
	insertAndScrollToItem(pItem);
	return pItem;
}

void ChatMessageListWidget::insertAndScrollToItem(QListWidgetItem* pItem)
{
	addItem(pItem);
	scrollToItem(pItem);
}

//--------------------------------------------------------------------------------------------
// ChatMessageLineEdit
//--------------------------------------------------------------------------------------------
ChatMessageLineEdit::ChatMessageLineEdit(QWidget* parent, boost::shared_ptr<PlayersDataManager> playersDataManager)
: QLineEdit(parent)
, m_pPlayersDataManager(playersDataManager)
, m_chatType(StudioChat::All)
{
	connect(this, SIGNAL(textEdited(const QString&)), this, SLOT(onTextEdited(const QString&)));
	connect(this, SIGNAL(returnPressed()), this, SLOT(onReturnPressed()));	
}

QString ChatMessageLineEdit::getText()
{	return text().mid(m_prefixString.length()); }

void ChatMessageLineEdit::onTextEdited(const QString& message)
{
	if (message.isEmpty() || (!m_prefixString.isEmpty() && (message.length() < m_prefixString.length())))
	{
		m_chatType = StudioChat::All;
		m_prefixString = "";
		setText("");
		return;
	}

	QString messageText = message.mid(m_prefixString.length());
	QString firstWord = messageText.left(messageText.indexOf(' ') + 1);

	if (firstWord.startsWith("/a ", Qt::CaseInsensitive) || firstWord.startsWith("/all ", Qt::CaseInsensitive))
	{
		m_chatType = StudioChat::All;
		m_prefixString = "";
		setText(messageText.mid(firstWord.length()));
	}
	else if (messageText.startsWith("/w ", Qt::CaseInsensitive) || messageText.startsWith("/whisper ", Qt::CaseInsensitive))
	{
		messageText = messageText.mid(firstWord.length());
		int index = messageText.indexOf(' ');
		if (index < 0)
			return;

		QString playerName;
		boost::shared_ptr<RBX::Network::Player> player = m_pPlayersDataManager->getPlayer(messageText.mid(0, index));
		if (player)
			playerName = player->getName().c_str();

		if (!playerName.isEmpty())
		{
			m_chatType = StudioChat::Whisper;
			m_prefixString = QString("[%1] ").arg(playerName);

			setText(m_prefixString);
		}
	}
}

void ChatMessageLineEdit::onReturnPressed()
{
	Q_EMIT returnPressed(m_chatType, getText(), m_prefixString.mid(1, m_prefixString.length() - 3));
	setText(m_prefixString);
}

//--------------------------------------------------------------------------------------------
// RobloxChatWidget
//--------------------------------------------------------------------------------------------
RobloxChatWidget::RobloxChatWidget(QWidget* parent, boost::shared_ptr<RBX::DataModel> dataModel, boost::shared_ptr<PlayersDataManager> playerDataManager)
: QFrame(parent)
, m_pDataModel(dataModel)
, m_pPlayersDataManager(playerDataManager)
{
	static bool registerMetaData(true);
	if (registerMetaData)
	{
		qRegisterMetaType<StudioChat::ChatMessageData>("StudioChat::ChatMessageData");
		registerMetaData = false;
	}

	m_pChatListWidget = new ChatMessageListWidget(this, m_pPlayersDataManager);

	m_pMessageBox     = new ChatMessageLineEdit(this, m_pPlayersDataManager);
	m_pMessageBox->setPlaceholderText(tr("Send a message"));

	QGridLayout* pLayout = new QGridLayout;
	pLayout->setMargin(0);
	pLayout->setSpacing(0);
	pLayout->addWidget(m_pChatListWidget, 0, 0, 1, 1);
	pLayout->addWidget(m_pMessageBox, 1, 0, 1, 1);
	setLayout(pLayout);
	setMinimumSize(kMinChatWidgetSize);

	connect(m_pMessageBox, SIGNAL(returnPressed(int, const QString&, const QString&)), this, SLOT(onReturnPressed(int, const QString&, const QString&)));

	// datamodel lock must be set in the function caller
	RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>();
	RBXASSERT(players);
	m_cPlayerChatMessage = players->chatMessageSignal.connect(boost::bind(&RobloxChatWidget::onPlayerChatMessage, this, _1));

	// save pixmap for chat action
	QDockWidget* chatDockWidget = UpdateUIManager::Instance().getMainWindow().findChild<QDockWidget*>("chatDockWidget");
	RBXASSERT(chatDockWidget && chatDockWidget->toggleViewAction());
	QIcon icon = chatDockWidget->toggleViewAction()->icon();
	m_OriginalChatActionPixmap = icon.pixmap(icon.availableSizes()[0]);
}

void RobloxChatWidget::onPlayerChatMessage(const RBX::Network::ChatMessage& chatMessage)
{
	if (chatMessage.message.empty())
		return;

	StudioChat::ChatType chatType;
	switch (chatMessage.chatType)
	{
	case RBX::Network::ChatMessage::CHAT_TYPE_ALL:
		chatType = StudioChat::All;
		break;
	case RBX::Network::ChatMessage::CHAT_TYPE_WHISPER:
		chatType = StudioChat::Whisper;
		break;
	case RBX::Network::ChatMessage::CHAT_TYPE_GAME:
		chatType = StudioChat::Game;
		break;
	default:
		return;
	}

	StudioChat::ChatMessageData messageData(chatType, QString::fromStdString(chatMessage.message));
	if (chatMessage.source)
	{
		messageData.isLocalPlayer = m_pPlayersDataManager->isLocalPlayer(chatMessage.source);
		messageData.playerName = QString::fromStdString(chatMessage.source->getName());
		messageData.playerId = chatMessage.source->getUserID();
	}

	// message must be added in main thread
	QMetaObject::invokeMethod(this, "addMessage", Qt::QueuedConnection, Q_ARG(StudioChat::ChatMessageData, messageData));
}

void RobloxChatWidget::addMessage(StudioChat::ChatMessageData messageData)
{
	// must be called from Main Thread
	if (messageData.isLocalPlayer)
	{
		updateSentMessageStatus(messageData.message);
	}
	else
	{
		if (messageData.playerId > 0 && (messageData.chatType == StudioChat::All || messageData.chatType == StudioChat::Whisper))
		{
			m_pChatListWidget->addMessageItem(messageData);
			// if widget is not visible then update action pixmap to convey new message received
			if (!isVisible())
				updateToggleChatActionPixmap();
		}
		else if (messageData.chatType == StudioChat::Game)
		{
			m_pChatListWidget->addBroadcastItem(messageData.message);
		}
	}
}

void RobloxChatWidget::onReturnPressed(int chatType, const QString& message, const QString& targetPlayerName)
{
	// do not send empty message
	if (message.isEmpty() || (chatType == (int)StudioChat::Whisper && targetPlayerName.isEmpty()))
		return;

	// check if help is required
	if (message == "/?")
	{
		// TODO: check if we need to change color for the item?
		QString helpMessage("Chat Commands:\n/w [PlayerName] or /whisper [PlayerName] - Whisper Chat\n/a or /all - All Chat");
		m_pChatListWidget->addBroadcastItem(helpMessage);
		return;
	}

	// if the player name is invalid then the chat type will be all
	if ((chatType == (int)StudioChat::All) && (message.startsWith("/w ", Qt::CaseInsensitive) || message.startsWith("/whisper ", Qt::CaseInsensitive)))
	{
		QStringList words = message.split(' ');
		QString playerName = words.count() > 1 ? words[1] : message.mid(words[0].length() + 1);
		m_pChatListWidget->addBroadcastItem(QString("Cannot send message to Player '%1'").arg(playerName), true);
		return;
	}

	// do not send invalid message
	if (message.startsWith("/"))
	{
		m_pChatListWidget->addBroadcastItem("Please make sure message is in correct format. Type \'/?\' for help.", true);
		return;
	}

	QString errorMessage;
	StudioChat::ChatMessageData messageData((StudioChat::ChatType)chatType, message);
	messageData.playerName = targetPlayerName;
	messageData.isLocalPlayer = true;

	// send message 
	sendMessage(messageData, errorMessage);

	// if we do not have any errors, then directly add the message to listwidget or else add error
	if (errorMessage.isEmpty())
	{
		QListWidgetItem* pMessageItem = m_pChatListWidget->addMessageItem(messageData);
		QListWidgetItem* pStatusMessageItem = m_pChatListWidget->addMessageStatusItem("Sending...");
		m_sentMessages.enqueue(qMakePair(pMessageItem, pStatusMessageItem));
	}
	else
	{
		m_pChatListWidget->addBroadcastItem(errorMessage, true);
	}
}

void RobloxChatWidget::sendMessage(const StudioChat::ChatMessageData& messageData, QString& errorMessage)
{
	RBX::DataModel::LegacyLock lock(m_pDataModel.get(), RBX::DataModelJob::Write);
	RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>();
	if (players)
	{
		try
		{
			if (messageData.chatType == StudioChat::All)
			{
				players->chat(messageData.message.toStdString());
			}
			else if (messageData.chatType == StudioChat::Whisper)
			{
				if (boost::shared_ptr<RBX::Network::Player> player = m_pPlayersDataManager->getPlayer(messageData.playerName))
				{
					if (m_pPlayersDataManager->isLocalPlayer(player))
					{
						errorMessage = "You cannot send a whisper to yourself.";
					}
					else
					{
						players->whisperChat(messageData.message.toStdString(), player);
					}
				}
				else
				{
					errorMessage = QString("Cannot send message to Player '%1'").arg(messageData.playerName);
				}
			}
		}

		catch (const RBX::base_exception& e)
		{
			errorMessage = e.what();
		}
	}
}

void RobloxChatWidget::updateSentMessageStatus(const QString& message)
{
	if (m_sentMessages.isEmpty())
		return;

	QPair<QListWidgetItem*, QListWidgetItem*> sentMessagePair = m_sentMessages.dequeue();
	RBXASSERT(sentMessagePair.first);
	RBXASSERT(sentMessagePair.second);
	if (message != sentMessagePair.first->text())
		sentMessagePair.first->setText(message);
	delete sentMessagePair.second;
}

void RobloxChatWidget::showEvent(QShowEvent* evt)
{
	QWidget::showEvent(evt);
	// reset the original pixmap
	setToggleChatActionPixmap(m_OriginalChatActionPixmap);
}

void RobloxChatWidget::setToggleChatActionPixmap(const QPixmap& pixmap)
{
	QDockWidget* chatDockWidget = UpdateUIManager::Instance().getMainWindow().findChild<QDockWidget*>("chatDockWidget");
	RBXASSERT(chatDockWidget && chatDockWidget->toggleViewAction());
	chatDockWidget->toggleViewAction()->setIcon(QIcon(pixmap));
}

void RobloxChatWidget::updateToggleChatActionPixmap()
{
	QPixmap updatedPixmap(m_OriginalChatActionPixmap.rect().width(), m_OriginalChatActionPixmap.rect().height());
	updatedPixmap.fill(Qt::transparent);

	QPainter painter;
	painter.begin(&updatedPixmap);
	painter.drawPixmap(0, 0, m_OriginalChatActionPixmap);

	painter.setRenderHints(QPainter::Antialiasing);
	QPainterPath circlePath;
	circlePath.addEllipse(QPoint(26, 6), 6, 6);
	painter.fillPath(circlePath, QColor("#e31132"));
	
	setToggleChatActionPixmap(updatedPixmap);
}
