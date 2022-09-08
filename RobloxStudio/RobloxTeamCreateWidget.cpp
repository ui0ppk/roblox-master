/**
 * RobloxTeamCreateWidget.cpp
 * Copyright (c) 2015 ROBLOX Corp. All Rights Reserved.
 */

#include "stdafx.h"
#include "RobloxTeamCreateWidget.h"

#include <QMouseEvent>
#include <QFont>
#include <QCoreApplication>
#include <QStyledItemDelegate>
#include <QPainter>
#include <QCompleter>
#include <QStringListModel>
#include <QApplication>

#include "V8DataModel/DataModel.h"
#include "Network/Players.h"
#include "Network/Player.h"
#include "V8Xml/WebParser.h"
#include "util/RobloxGoogleAnalytics.h"

#include "PlayersDataManager.h"
#include "AuthenticationHelper.h"
#include "RobloxUser.h"
#include "RobloxIDEDoc.h"
#include "UpdateUIManager.h"
#include "RobloxMainWindow.h"
#include "RobloxStudioVerbs.h"
#include "RobloxSettings.h"
#include "RobloxGameExplorer.h"
#include "QtUtilities.h"
#include "RobloxContextualHelp.h"

static const int kPlayerAvatarSize   = 32;
static const int kVerticalMargin     = 4;
static const int kHorizontalMargin   = 8;

FASTFLAGVARIABLE(KickUsersWhenTheyLoseAccessToCloudEdit, true)
FASTFLAGVARIABLE(DoNotCheckCanCloudEditWhenNotLoggedIn, true)
FASTFLAGVARIABLE(TeamCreateEnableContextHelp, true)
FASTFLAGVARIABLE(TeamCreateEnableCompleterFix, true)
FASTFLAGVARIABLE(TeamCreateEnablePublishOnLogin, true)
FASTFLAGVARIABLE(TeamCreateCheckInvalidUniverse, true)

//--------------------------------------------------------------------------------------------
// PlayersWidgetDelegate
//--------------------------------------------------------------------------------------------
class CreatorsWidgetDelegate: public QStyledItemDelegate
{
public:
	CreatorsWidgetDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
	{

	}

	enum Role
	{
		DecorationStyle = Qt::UserRole + 1,
		OwnershipRole,
		SubLabelRole
	};

private:
	QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
	{
		QSize size = QStyledItemDelegate::sizeHint(option, index);
		return QSize(size.width(), kPlayerAvatarSize + 2*kVerticalMargin); 
	}

	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		painter->save();

		QStyleOptionViewItemV4 optionV4 = option;
		initStyleOption(&optionV4, index);

		// DecorationStyle - 0 (circular avatar, selectable)
		//                 - 1 (square avatar, non-selectable)
		if (!index.data(DecorationStyle).toInt())
		{
			optionV4.text = "";
			optionV4.icon = QIcon();
			CreatorsListWidget* pListWidget = static_cast<CreatorsListWidget*>(parent());
			pListWidget->style()->drawControl(QStyle::CE_ItemViewItem, &optionV4, painter, pListWidget);
		}

		QRect adjustedRect = option.rect.adjusted(kHorizontalMargin, kVerticalMargin, -kHorizontalMargin, -kVerticalMargin);

		QImage icon(qvariant_cast<QImage>(index.data(Qt::DecorationRole)));
		if (!icon.isNull())
		{
			painter->save();
			painter->setRenderHints(QPainter::Antialiasing);
			if (!index.data(DecorationStyle).toInt())
			{
				QPainterPath path;
				path.addEllipse(adjustedRect.topLeft().x(), adjustedRect.topLeft().y(), kPlayerAvatarSize, kPlayerAvatarSize);
				painter->setClipPath(path);
			}
			painter->drawImage(QPoint(adjustedRect.topLeft().x(), adjustedRect.topLeft().y()), icon);
			adjustedRect.setLeft(adjustedRect.left() + kPlayerAvatarSize + kHorizontalMargin);
			painter->restore();
		}

		if (index.data(OwnershipRole).toInt())
		{
			painter->setPen(QColor(Qt::gray));
			painter->drawLine(option.rect.bottomLeft(), option.rect.bottomRight() + QPoint(1, 0));
		}

		QString subLabel = index.data(SubLabelRole).toString();
		if (!subLabel.isEmpty())
		{
			painter->setPen(QColor(Qt::black));
			painter->drawText(adjustedRect, Qt::AlignLeft|Qt::AlignTop|Qt::ElideRight, index.data(Qt::DisplayRole).toString());

			QFont updatedFont(option.font);
			updatedFont.setPointSize(option.font.pointSize() - 2);
			painter->setFont(updatedFont);
			painter->setPen(QColor(Qt::gray));
			painter->drawText(adjustedRect, Qt::AlignLeft|Qt::AlignBottom|Qt::ElideRight, subLabel);
		}
		else
		{
			painter->setPen(QColor(Qt::black));
			painter->drawText(adjustedRect, Qt::AlignLeft|Qt::AlignVCenter|Qt::ElideRight, index.data(Qt::DisplayRole).toString());
		}

		painter->restore();
	}
};

//--------------------------------------------------------------------------------------------
// PlayersWidgetDelegate
//--------------------------------------------------------------------------------------------
class CompleterPopupDelegate: public QStyledItemDelegate
{
public:
	CompleterPopupDelegate(QObject* parent)
		: QStyledItemDelegate(parent)
	{

	}

private:
	
	void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		QListView* pListWidget(static_cast<QListView*>(parent()));

		QStyleOptionViewItemV4 optionV4 = option;
		initStyleOption(&optionV4, index);
		optionV4.text = "";
		pListWidget->style()->drawControl(QStyle::CE_ItemViewItem, &option, painter, pListWidget);

		QRect adjustedRect(optionV4.rect);
		adjustedRect.setRight(adjustedRect.right() - 20);

		painter->setPen(QColor(Qt::black));
		QString userName(index.data(Qt::DisplayRole).toString());
		painter->drawText(optionV4.rect, Qt::AlignLeft|Qt::AlignVCenter|Qt::ElideRight, userName.left(userName.indexOf(':')));

		static QImage addImage(":/16x16/images/Studio 2.0 icons/16x16/team_create_add.png");
		static QImage addOnImage(":/16x16/images/Studio 2.0 icons/16x16/team_create_add_ON.png");

		painter->drawImage(QPoint(adjustedRect.topRight().x(), adjustedRect.topRight().y() + 2), 
			               (option.state & QStyle::State_MouseOver) ? addOnImage : addImage);
	}
};
//--------------------------------------------------------------------------------------------
// GroupItem
//--------------------------------------------------------------------------------------------
static void invokeGroupDataLoaded(QPointer<GroupItem> pGroupItem, RBX::HttpFuture future)
{
	if (pGroupItem.isNull())
		return;
	QMetaObject::invokeMethod(pGroupItem.data(), "groupDataLoaded", Qt::QueuedConnection, Q_ARG(RBX::HttpFuture, future));
}

static void invokeGroupThumbnailLoaded(QPointer<GroupItem> pGroupItem, RBX::HttpFuture future)
{
	if (pGroupItem.isNull())
		return;
	QMetaObject::invokeMethod(pGroupItem.data(), "groupThumbnailLoaded", Qt::QueuedConnection, Q_ARG(RBX::HttpFuture, future));
}

GroupItem::GroupItem(int groupId) 
: m_GroupId(groupId)
{
	// request player item update
	QString str = QString("%1/groups/%2").arg(RobloxSettings::getApiBaseURL()).arg(groupId);
	RBX::HttpAsync::get(str.toStdString()).then(boost::bind(&invokeGroupDataLoaded, QPointer<GroupItem>(this), _1));
}

void GroupItem::groupDataLoaded(RBX::HttpFuture future)
{
	EntityProperties thumbnailProp;
	thumbnailProp.setFromJsonFuture(future);

	boost::optional<std::string> thumnailUri = thumbnailProp.get<std::string>("EmblemUrl");
	if (thumnailUri.is_initialized() && !thumnailUri.get().empty())
		RBX::HttpAsync::get(thumnailUri.get()).then(boost::bind(&invokeGroupThumbnailLoaded, QPointer<GroupItem>(this), _1));
}

void GroupItem::groupThumbnailLoaded(RBX::HttpFuture future)
{
	std::string imageContent;
	try
	{
		imageContent = future.get();
	}
	catch (const RBX::base_exception&)
	{

	}

	if (!imageContent.empty())
	{
		QImage image;
		if (image.loadFromData((uchar*)imageContent.c_str(), imageContent.size()))
		{
			QImage resultingImage(QSize(32, 32), image.format());
			resultingImage.fill(Qt::lightGray);
			QPainter painter(&resultingImage);
			painter.drawImage(0, 0, image.scaled(QSize(32, 32), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
			setData(0, Qt::DecorationRole, resultingImage);
		}
	}
}

//--------------------------------------------------------------------------------------------
// CreatorItem
//--------------------------------------------------------------------------------------------
void CreatorItem::updatePlayerAvatar(const QImage& playerImage)
{
	setData(0, Qt::DecorationRole, playerImage);
}

void CreatorItem::updatePlayerName(int playerId, const QString& userName)
{
	if (playerId == m_PlayerId)
		setData(0, Qt::DisplayRole, userName);
}

//--------------------------------------------------------------------------------------------
// RobloxPlayersListWidget
//--------------------------------------------------------------------------------------------
CreatorsListWidget::CreatorsListWidget(QWidget* parent, shared_ptr<RBX::DataModel> dataModel, shared_ptr<PlayersDataManager> dataManager)
: QTreeWidget(parent)
, m_pDataModel(dataModel)
, m_pPlayersDataManager(dataManager)
{
	RBXASSERT(m_pDataModel);
	RBXASSERT(m_pPlayersDataManager);

	setHeaderHidden(true);
	setRootIsDecorated(false);
	setUniformRowHeights(true);
	setAlternatingRowColors(true);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setFocusPolicy(Qt::NoFocus);
	// add for custom drawing of the items
	setItemDelegate(new CreatorsWidgetDelegate(this));
}

void CreatorsListWidget::addCreatorItem(int playerId, const QString& playerName, const QString& subLabel)
{
	CreatorItem* pCreatorItem = new CreatorItem(playerId);
	pCreatorItem->setText(0, playerName);
	if (!subLabel.isEmpty())
		pCreatorItem->setData(0, CreatorsWidgetDelegate::SubLabelRole, subLabel);

	addTopLevelItem(pCreatorItem);

	// request player item update
	m_pPlayersDataManager->getPlayerAvatar(playerId, pCreatorItem, "updatePlayerAvatar");
}

void CreatorsListWidget::onPlayerAdded(boost::shared_ptr<RBX::Instance> player)
{
	int playerId = RBX::Instance::fastDynamicCast<RBX::Network::Player>(player.get())->getUserID();
	if (playerId <= 0)
		return;

	QMetaObject::invokeMethod(this, "onPlayerAddRequsted", Qt::QueuedConnection, Q_ARG(int, playerId));
}

void CreatorsListWidget::onPlayerRemoving(boost::shared_ptr<RBX::Instance> player)
{
	int playerId = RBX::Instance::fastDynamicCast<RBX::Network::Player>(player.get())->getUserID();
	if (playerId <= 0)
		return;

	QMetaObject::invokeMethod(this, "onPlayerRemoveRequested", Qt::QueuedConnection, Q_ARG(int, playerId));
}

bool CreatorsListWidget::isPlayerPresent(int playerId)
{
	if (!m_pDataModel)
		return false;

	RBX::DataModel::LegacyLock lock(m_pDataModel.get(), RBX::DataModelJob::Write);
	RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>();
	return (players && players->getPlayerByID(playerId));
}

QString CreatorsListWidget::getPlayerName(int playerId)
{
	if (!m_pDataModel)
		return false;

	RBX::DataModel::LegacyLock lock(m_pDataModel.get(), RBX::DataModelJob::Write);
	RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>();
	if (players)
	{
		boost::shared_ptr<RBX::Network::Player> player = players->getPlayerByID(playerId);
		if (player)
			return QString::fromStdString(player->getName());
	}

	return QString();
}

CreatorItem* CreatorsListWidget::getCreatorItem(int playerId)
{
	int currentCount = 0;
	CreatorItem* pCreatorItem = NULL;

	while (currentCount < topLevelItemCount())
	{
		pCreatorItem = dynamic_cast<CreatorItem*>(topLevelItem(currentCount));
		if (pCreatorItem && (pCreatorItem->getPlayerId() == playerId))
			return pCreatorItem;
		currentCount++;
	}

	return NULL;
}

CreatorItem* CreatorsListWidget::getCreatorItem(const QString& playerName)
{
	int currentCount = 0;
	CreatorItem* pCreatorItem = NULL;

	while (currentCount < topLevelItemCount())
	{
		pCreatorItem = dynamic_cast<CreatorItem*>(topLevelItem(currentCount));
		if (pCreatorItem && (QString::compare(pCreatorItem->text(0), playerName, Qt::CaseInsensitive) == 0))
			return pCreatorItem;
		currentCount++;
	}

	return NULL;
}

void CreatorsListWidget::mousePressEvent(QMouseEvent *evt)
{
	if (!itemAt(evt->pos()))
		setCurrentItem(NULL);
	else
		QTreeWidget::mousePressEvent(evt);
}

//--------------------------------------------------------------------------------------------
// UserPlaceCreatorsListWidget
//--------------------------------------------------------------------------------------------
UserPlaceCreatorsListWidget::UserPlaceCreatorsListWidget(QWidget* parent, shared_ptr<RBX::DataModel> dataModel, shared_ptr<PlayersDataManager> dataManager)
: CreatorsListWidget(parent, dataModel, dataManager)
, m_PlaceOwnerId(-1)
{
	RBXASSERT(dataModel->getUniverseId());
	m_pCreatorsListLoader = new CreatorsListLoader(this, dataModel->getUniverseId());
	connect(m_pCreatorsListLoader, SIGNAL(finishedLoading()), this, SLOT(onCreatorsListLoaded()));

	// connect to data model
	RBX::DataModel::LegacyLock lock(m_pDataModel.get(), RBX::DataModelJob::Write);
	RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>();
	if (players)
	{
		m_cPlayerAdded = players->playerAddedSignal.connect(boost::bind(&UserPlaceCreatorsListWidget::onPlayerAdded, this, _1));
		m_cPlayerRemoving = players->playerRemovingSignal.connect(boost::bind(&UserPlaceCreatorsListWidget::onPlayerRemoving, this, _1));
		players->visitChildren(boost::bind(&UserPlaceCreatorsListWidget::onPlayerAdded, this, _1));
	}
}

void UserPlaceCreatorsListWidget::addOwnerDetails(const PlaceCreatorDetails& creatorDetails)
{
	CreatorItem* pCreatorItem = new CreatorItem(creatorDetails.ownerId);
	pCreatorItem->setText(0, creatorDetails.ownerName);
	pCreatorItem->setData(0, CreatorsWidgetDelegate::SubLabelRole, "Place Owner (Offline)");
	pCreatorItem->setData(0, CreatorsWidgetDelegate::OwnershipRole, 1);
	insertTopLevelItem(0, pCreatorItem);	

	m_pPlayersDataManager->getPlayerAvatar(creatorDetails.ownerId, pCreatorItem, "updatePlayerAvatar");
	m_PlaceOwnerId = creatorDetails.ownerId;
}

void UserPlaceCreatorsListWidget::onPlayerAddRequsted(int playerId)
{
	CreatorItem* pCreatorItem = getCreatorItem(playerId);
	if (pCreatorItem)
	{
		QString subLabel = pCreatorItem->data(0, CreatorsWidgetDelegate::SubLabelRole).toString();
		if (subLabel == "Offline")
		{
			subLabel = "";
		}
		else
		{
			subLabel.replace(" (Offline)", "");
		}

		pCreatorItem->setData(0, CreatorsWidgetDelegate::SubLabelRole, subLabel);
	}
	else
	{		
		// add item
		addCreatorItem(playerId, getPlayerName(playerId), "");
		// request creators list reload, just in case if there are other creators added
		m_pCreatorsListLoader->reload();
	}
}

void UserPlaceCreatorsListWidget::onPlayerRemoveRequested(int playerId)
{
	CreatorItem* pCreatorItem = getCreatorItem(playerId);
	if (pCreatorItem)
	{
		QString subLabel = pCreatorItem->data(0, CreatorsWidgetDelegate::SubLabelRole).toString();
		if (subLabel.isEmpty())
		{
			subLabel = "Offline";
		}
		else
		{
			subLabel.append(" (Offline)");
		}

		pCreatorItem->setData(0, CreatorsWidgetDelegate::SubLabelRole, subLabel);
	}
}

void UserPlaceCreatorsListWidget::onCreatorsListLoaded()
{
	// first remove the removed creators
	removeRemovedCreators();
	// now added the added creators
	addAddedCreators();

	Q_EMIT finishedLoading();
}

void UserPlaceCreatorsListWidget::contextMenuEvent(QContextMenuEvent * evt)
{
	if (!canDeleteSelectedCreator())
		return;

	QMenu menu;

	QAction* deleteAction = menu.addAction("Delete");
	deleteAction->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/delete_x_16_h.png"));

	QAction* result = menu.exec(evt->globalPos());
	if (result && (result == deleteAction))
    {
		QList<QTreeWidgetItem*> items = selectedItems();
		if (items.size() > 0)
		{
			RBXASSERT(items.size() == 1);
			CreatorItem* pCreatorItem = dynamic_cast<CreatorItem*>(items.at(0));
			if (pCreatorItem)
				Q_EMIT removeCloudEditorRequested(pCreatorItem->getPlayerId());
		}
	}
}

bool UserPlaceCreatorsListWidget::canDeleteSelectedCreator()
{
	// do not show context menu for non owner
	if (m_PlaceOwnerId > 0 && (RobloxUser::singleton().getUserId() == m_PlaceOwnerId))
	{
		QList<QTreeWidgetItem*> items = selectedItems();
		if (items.size() == 1)
		{
			CreatorItem* pCreatorItem = dynamic_cast<CreatorItem*>(items.at(0));
			if (pCreatorItem && pCreatorItem->getPlayerId() != m_PlaceOwnerId)
				return true;
		}
	}

	return false;
}

void UserPlaceCreatorsListWidget::removeRemovedCreators()
{
	CreatorItem* pCreatorItem = NULL;
	QList<QTreeWidgetItem*> itemsToDelete;
	const CreatorsCollection& creators = m_pCreatorsListLoader->getCreators();	

	for (int currentCount = 0; currentCount < topLevelItemCount(); ++currentCount)
	{
		pCreatorItem = dynamic_cast<CreatorItem*>(topLevelItem(currentCount));
		if (pCreatorItem && (pCreatorItem->getPlayerId() != m_PlaceOwnerId) && !creators.contains(pCreatorItem->getPlayerId()))
			itemsToDelete.append(pCreatorItem);
	}

	while (!itemsToDelete.empty())
		delete itemsToDelete.takeFirst();
}

void UserPlaceCreatorsListWidget::addAddedCreators()
{
	int playerId = 0;
	bool playerPresent = false;
	const CreatorsCollection& creators = m_pCreatorsListLoader->getCreators();

	for (CreatorsCollection::const_iterator iter = creators.constBegin(); iter != creators.constEnd(); ++iter)
	{
		playerId = iter.key();
		if (playerId > 0 && !getCreatorItem(playerId))
		{
			playerPresent = isPlayerPresent(playerId);
			addCreatorItem(playerId, playerPresent ? getPlayerName(playerId) : "", playerPresent ? "" : "Offline");

			if (!playerPresent)
			{
				CreatorItem* pCreatorItem = getCreatorItem(playerId);
				if (pCreatorItem)
					m_pPlayersDataManager->getPlayerName(playerId, pCreatorItem, "updatePlayerName");
			}
		}
	}
}

//--------------------------------------------------------------------------------------------
// GroupPlaceCreatorsListWidget
//--------------------------------------------------------------------------------------------
GroupPlaceCreatorsListWidget::GroupPlaceCreatorsListWidget(QWidget* parent, shared_ptr<RBX::DataModel> dataModel, shared_ptr<PlayersDataManager> dataManager)
: CreatorsListWidget(parent, dataModel, dataManager)
{
	RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>();
	if (players)
	{
		m_cPlayerAdded = players->playerAddedSignal.connect(boost::bind(&GroupPlaceCreatorsListWidget::onPlayerAdded, this, _1));
		m_cPlayerRemoving = players->playerRemovingSignal.connect(boost::bind(&GroupPlaceCreatorsListWidget::onPlayerRemoving, this, _1));
		players->visitChildren(boost::bind(&GroupPlaceCreatorsListWidget::onPlayerAdded, this, _1));
	}

	Q_EMIT finishedLoading();
}

void GroupPlaceCreatorsListWidget::addOwnerDetails(const PlaceCreatorDetails& creatorDetails)
{
	GroupItem* pGroupItem = new GroupItem(creatorDetails.ownerId);
	pGroupItem->setText(0, creatorDetails.ownerName);
	pGroupItem->setData(0, CreatorsWidgetDelegate::SubLabelRole, "Group owned place");
	pGroupItem->setData(0, CreatorsWidgetDelegate::OwnershipRole, 1);
	pGroupItem->setData(0, CreatorsWidgetDelegate::DecorationStyle, 1);

	insertTopLevelItem(0, pGroupItem);
}

void GroupPlaceCreatorsListWidget::onPlayerAddRequsted(int playerId)
{
	// if name is empty meaning player is not present
	QString playerName = getPlayerName(playerId);
	if (playerName.isEmpty())
		return;

	CreatorItem* pCreatorItem = new CreatorItem(playerId);
	pCreatorItem->setText(0, playerName);
	
	addTopLevelItem(pCreatorItem);

	// request player item update
	m_pPlayersDataManager->getPlayerAvatar(playerId, pCreatorItem, "updatePlayerAvatar");
}

void GroupPlaceCreatorsListWidget::onPlayerRemoveRequested(int playerId)
{
	CreatorItem* pCreatorItem = getCreatorItem(playerId);
	if (pCreatorItem)
		delete pCreatorItem;
}

//--------------------------------------------------------------------------------------------
// RobloxTeamCreateWidget
//--------------------------------------------------------------------------------------------
RobloxTeamCreateWidget::RobloxTeamCreateWidget(QWidget* parent)
: QWidget(parent)
, m_pCreatorsListWidget(NULL)
, m_pMainStackedWidgetAnimator(NULL)
, m_pFriendsInfoLoader(NULL)
, m_bCanManageCurrentPlace(false)
, m_bLoginButtonClicked(false)
{
	setupUi(this);

	connect(loginButton, SIGNAL(clicked()), this, SLOT(onLoginButtonClicked()));
	connect(publishButton, SIGNAL(clicked()), this, SLOT(onPublishButtonClicked()));

	connect(groupPlaceTurnOnButton, SIGNAL(clicked()), this, SLOT(onTurnOnButtonClicked()));
	connect(userPlaceTurnOnButton, SIGNAL(clicked()), this, SLOT(onTurnOnButtonClicked()));

	connect(bottomMoreButton, SIGNAL(clicked()), this, SLOT(onBottomMoreButtonClicked()));
	bottomMoreButton->setToolTip(tr("Team Create Options"));

	connect(userRemoveButton, SIGNAL(clicked()), this, SLOT(onRemoveCloudEditConfirm()));
	connect(userRemoveCancelButton, SIGNAL(clicked()), this, SLOT(onRemoveCloudEditCancel()));

	connect(friendsInviteEdit, SIGNAL(returnPressed()), this, SLOT(onFriendsInviteEditReturnPressed()));

	m_pMainStackedWidgetAnimator = new QtUtilities::StackedWidgetAnimator(mainStackedWidget);

	updateCurrentWidgetInStack();
}

void RobloxTeamCreateWidget::setDataModel(boost::shared_ptr<RBX::DataModel> dataModel, boost::shared_ptr<PlayersDataManager> pPlayersDataManager)
{
	if (m_pDataModel == dataModel)
		return;

	m_bLoginButtonClicked = false;
	disconnect(&AuthenticationHelper::Instance(), SIGNAL(authenticationChanged(bool)), this, SLOT(onAuthenticationChanged(bool)));
	m_PlaceCreatorDetails = PlaceCreatorDetails();
	m_PlaceDetailsQuery = boost::shared_future<void>();
	m_PlaceManageStatusQuery = boost::shared_future<void>();
	m_cPlaceIdChange.disconnect();
	if (m_pFriendsInfoLoader) delete m_pFriendsInfoLoader; m_pFriendsInfoLoader = NULL;

	if (m_pCreatorsListWidget)
	{
		delete m_pCreatorsListWidget;
		m_pCreatorsListWidget = NULL;
	}

	m_pDataModel = dataModel;
	m_pPlayersDataManager = pPlayersDataManager;
	
	if (m_pDataModel)
	{
		connect(&AuthenticationHelper::Instance(), SIGNAL(authenticationChanged(bool)), this, SLOT(onAuthenticationChanged(bool)));
		m_cPlaceIdChange = m_pDataModel->propertyChangedSignal.connect(boost::bind(&RobloxTeamCreateWidget::onPlaceIdChanged, this, _1) );
	}

	QMetaObject::invokeMethod(this, "updateCurrentWidgetInStack", Qt::QueuedConnection);
}

PlaceCreatorDetails RobloxTeamCreateWidget::getPlaceCreatorDetails()
{
	canManageCurrentPlace();
	if (m_PlaceDetailsQuery.future_ && !m_PlaceDetailsQuery.is_ready())
		m_PlaceDetailsQuery.get();
	return m_PlaceCreatorDetails;
}

void RobloxTeamCreateWidget::updateCurrentWidgetInStack()
{
	m_bLoginButtonClicked = false;

	// default state
	if (!m_pDataModel)
	{
		m_pMainStackedWidgetAnimator->setCurrentIndex(EMPTY_PAGE);
	}
	// if user is not logged in
	else if (RobloxUser::singleton().getUserId() <= 0)
	{		
		optionsStackedWidget->setCurrentIndex(LOGIN_PAGE);
		m_pMainStackedWidgetAnimator->setCurrentIndex(OPTIONS_PAGE);
	}
	// if we are launching cloud edit
	else if (RobloxIDEDoc::getIsCloudEditSession())
	{
		if (m_PlaceCreatorDetails.ownerId < 0)
			return;

		bool canManage = canManageCurrentPlace();
		eTeamType creatorType = getCreatorType();

		if (!m_pCreatorsListWidget)
			addCreatorsListWidget(creatorType);

		if (creatorType == TEAM_USERS)
		{
			optionsStackedWidget->setCurrentIndex(USER_PLACE_OPTIONS_PAGE);
			userPlaceStackedWidget->setCurrentIndex(USER_PLACE_CREATORS_LIST_PAGE);
			enableFriendsInviteEdit(canManage);			
		}
		else
		{
			optionsStackedWidget->setCurrentIndex(GROUP_PLACE_OPTIONS_PAGE);
			groupPlaceStackedWidget->setCurrentIndex(GROUP_PLACE_CREATORS_LIST_PAGE);
		}

		m_pMainStackedWidgetAnimator->setCurrentIndex(OPTIONS_PAGE);
	}
	// if we have a published place
	else if (m_pDataModel && m_pDataModel->getPlaceID() > 0)
	{
		bool canManage = canManageCurrentPlace();
		eTeamType creatorType = getCreatorType();

		if (creatorType == TEAM_GROUP)
		{
			optionsStackedWidget->setCurrentIndex(GROUP_PLACE_OPTIONS_PAGE);
			if (canManage)
			{
				groupPlaceStackedWidget->setCurrentIndex(GROUP_PLACE_TURN_ON_PAGE);
			}
			else
			{			
				groupPlaceStackedWidget->setCurrentIndex(GROUP_PLACE_NON_ADMIN_MESSAGE_PAGE);
			}
		}
		else if (canManage)
		{
			optionsStackedWidget->setCurrentIndex(USER_PLACE_OPTIONS_PAGE);
			userPlaceStackedWidget->setCurrentIndex(USER_PLACE_TURN_ON_PAGE);
			enableFriendsInviteEdit(true);			
		}
		else
		{
			m_pMainStackedWidgetAnimator->setCurrentIndex(PUBLISH_PAGE);
			return;
		}

		m_pMainStackedWidgetAnimator->setCurrentIndex(OPTIONS_PAGE);
	}
	// if user is logged in but having an unpublished place
	else
	{
		m_pMainStackedWidgetAnimator->setCurrentIndex(PUBLISH_PAGE);
	}
}

void RobloxTeamCreateWidget::addCreatorsListWidget(eTeamType creatorType)
{
	QWidget* pParentWidget = creatorType == TEAM_GROUP ? groupPlaceCreatorsListPage : userPlaceCreatorsListPage;

	QLayout* pExistingLayout = pParentWidget->layout();
	if (pExistingLayout) delete pExistingLayout;

	QGridLayout* pLayout = new QGridLayout;
	pLayout->setMargin(0);
	pLayout->setSpacing(0);

	if (creatorType == TEAM_GROUP)
	{
		m_pCreatorsListWidget = new GroupPlaceCreatorsListWidget(this, m_pDataModel, m_pPlayersDataManager);
	}
	else
	{
		m_pCreatorsListWidget = new UserPlaceCreatorsListWidget(this, m_pDataModel, m_pPlayersDataManager);

		if (canManageCurrentPlace())
		{
			connect(m_pCreatorsListWidget, SIGNAL(removeCloudEditorRequested(int)), this, SLOT(onRemoveCloudEditorRequested(int)));
			connect(m_pCreatorsListWidget, SIGNAL(finishedLoading()), this, SLOT(updatePlayerFriendsInfo()));
		}
	}

	m_pCreatorsListWidget->addOwnerDetails(getPlaceCreatorDetails());

	pLayout->addWidget(m_pCreatorsListWidget, 0, 0, 1, 1);
	pParentWidget->setLayout(pLayout);
}

void RobloxTeamCreateWidget::onAuthenticationChanged(bool)
{
	// TODO: shall we reset the widget if user changes?

	IRobloxDoc* pDoc = RobloxDocManager::Instance().getPlayDoc();
	if (FFlag::TeamCreateEnablePublishOnLogin && m_bLoginButtonClicked && pDoc)
	{
		// bring play doc to the front
		RobloxDocManager::Instance().setCurrentDoc(pDoc);
		// invoke publish slot, must be done queued as user might not have been authenticated by now
		QMetaObject::invokeMethod(this, "onPublishButtonClicked", Qt::QueuedConnection);
	}
	else
	{
		// this has to be delayed as user RBX::User authentication SLOT might not have been called by now
		QMetaObject::invokeMethod(this, "updateCurrentWidgetInStack", Qt::QueuedConnection);
	}
}

void RobloxTeamCreateWidget::onLoginButtonClicked()
{
	UpdateUIManager::Instance().getMainWindow().openStartPage(true, "showlogin=True");
	UpdateUIManager::Instance().getMainWindow().setFocus();

	if (FFlag::TeamCreateEnablePublishOnLogin)
		m_bLoginButtonClicked = true;

	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create", "User login");
}

void RobloxTeamCreateWidget::onPublishButtonClicked()
{
	bool loginButtonClicked = m_bLoginButtonClicked;
	m_bLoginButtonClicked = false;

	if (FFlag::TeamCreateEnablePublishOnLogin && loginButtonClicked)
	{		
		// if there's no user, then just update the widget stack
		if (RobloxUser::singleton().getUserId() <= 0)
		{
			QMetaObject::invokeMethod(this, "updateCurrentWidgetInStack", Qt::QueuedConnection);
			return;
		}
	}

	// get dialog and re-enable the previous page
	PublishToRobloxAsVerb* publishToRobloxVerb = dynamic_cast<PublishToRobloxAsVerb*>(m_pDataModel->getVerb("PublishToRobloxAsVerb"));
	if(publishToRobloxVerb)
	{
		publishToRobloxVerb->doIt(m_pDataModel.get());
		QDialog* pDialog = publishToRobloxVerb->getPublishDialog();
		if (pDialog)
		{
			m_pMainStackedWidgetAnimator->setCurrentIndex(PUBLISHING_MESSAGE_PAGE);
			connect(pDialog, SIGNAL(finished(int)), this, SLOT(onPublishDialogFinished(int)));
		}
	}

	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create", "Publish invoked");
}

void RobloxTeamCreateWidget::onTurnOnButtonClicked()
{
	RBXASSERT(m_pDataModel->getUniverseId() > 0);

	eTeamType creatorType = getCreatorType();
	if (creatorType == TEAM_USERS)
	{
		optionsStackedWidget->setCurrentIndex(USER_PLACE_OPTIONS_PAGE);
		userPlaceStackedWidget->setCurrentIndex(USER_PLACE_SAVE_MESSAGE_PAGE);			
	}
	else
	{
		optionsStackedWidget->setCurrentIndex(GROUP_PLACE_OPTIONS_PAGE);
		groupPlaceStackedWidget->setCurrentIndex(GROUP_PLACE_SAVE_MESSAGE_PAGE);
	}

	m_pMainStackedWidgetAnimator->setCurrentIndex(OPTIONS_PAGE);

	// publish place
	UpdateUIManager::Instance().getMainWindow().publishToRobloxAction->trigger();

	// turn on cloud edit
	QString postUrl = QString("%1/universes/%2/enablecloudedit").arg(RobloxSettings::getApiBaseURL()).arg(m_pDataModel->getUniverseId());
	std::string result = doPost(postUrl.toStdString());
	if (result != "{}")
	{
		QString label = QString("Cloud Edit enable failed for %1").arg(creatorType == TEAM_USERS ? "userPlace" : "groupPlace");
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create",  qPrintable(label));

		QMetaObject::invokeMethod(this, "updateCurrentWidgetInStack", Qt::QueuedConnection);
		return;
	}	

	// launch cloud edit
	QMetaObject::invokeMethod(&(UpdateUIManager::Instance().getMainWindow()), 
		                      "cloudEditStatusChanged", Qt::QueuedConnection, Q_ARG(int, m_pDataModel->getPlaceID()), Q_ARG(int, m_pDataModel->getUniverseId()));

	QString label = QString("Cloud Edit turned on for %1").arg(creatorType == TEAM_USERS ? "userPlace" : "groupPlace");
	RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create",  qPrintable(label));
}

void RobloxTeamCreateWidget::onBottomMoreButtonClicked()
{
	QMenu menu(this);
	QAction* cloudEditTurnOffAction = NULL;
	QAction* deleteUserAction = NULL;
	if (RobloxIDEDoc::getIsCloudEditSession() && canManageCurrentPlace())
	{
		cloudEditTurnOffAction = menu.addAction(tr("Turn OFF Team Create"));
		if (getCreatorType() == TEAM_USERS)
		{
			deleteUserAction = menu.addAction(tr("Delete User"));
			deleteUserAction->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/delete_x_16_h.png"));
			deleteUserAction->setEnabled(m_pCreatorsListWidget->canDeleteSelectedCreator());
		}
	}
	QAction* helpAction = menu.addAction(tr("Help"));

	const int kButtonHeight = 20;
	const int kMenuMargin = 2;

	QPoint bottomRight = mapToGlobal(rect().bottomRight());
	QSize     menuSize = menu.sizeHint();
	QAction* resultAction = menu.exec(QPoint(bottomRight.x() - (menuSize.width()+kMenuMargin), bottomRight.y() - (menuSize.height()+kButtonHeight+kMenuMargin)));
	if (!resultAction)
		return;

	if (resultAction == cloudEditTurnOffAction)
	{
		QtUtilities::RBXMessageBox msgBox;
		msgBox.setIcon(QMessageBox::Question);
		msgBox.setText(tr("Please confirm you want to turn Team Create off."));
		msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
		msgBox.button(QMessageBox::Yes)->setText(tr("Confirm"));
		msgBox.setDefaultButton(QMessageBox::No);

		//user has pressed NO or close button, do not close the document
		if ( msgBox.exec() != QMessageBox::Yes )
			return;	

		RBXASSERT(m_pDataModel->getUniverseId());
		QString postUrl = QString("%1/universes/%2/disablecloudedit").arg(RobloxSettings::getApiBaseURL()).arg(m_pDataModel->getUniverseId());
		std::string result = doPost(postUrl.toStdString());
		if (result == "{}")
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create",  "Cloud Edit turned off for userPlace");
			if (FFlag::KickUsersWhenTheyLoseAccessToCloudEdit)
			{
				UpdateUIManager::Instance().getMainWindow().shutDownCloudEditServer();
			}
			else
			{
				QMetaObject::invokeMethod(&(UpdateUIManager::Instance().getMainWindow()), 
					"cloudEditStatusChanged", Qt::QueuedConnection, Q_ARG(int, m_pDataModel->getPlaceID()), Q_ARG(int, m_pDataModel->getUniverseId()));
			}
		}
		else
		{
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create",  "Cloud Edit turned off failed for userPlace");
		}
	}
	else if (resultAction == helpAction)
	{
		if (FFlag::TeamCreateEnableContextHelp)
		{
			UpdateUIManager::Instance().setDockVisibility(eDW_CONTEXTUAL_HELP, true);
			QMetaObject::invokeMethod(&RobloxContextualHelpService::singleton(), "onHelpTopicChanged", Q_ARG(QString, QString("Team_Create")));
		}
		else
		{
			UpdateUIManager::Instance().getMainWindow().onlineHelpAction->trigger();
		}

		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create",  "Help invoked");
	}
	else if (resultAction == deleteUserAction)
	{
		QList<QTreeWidgetItem*> items = m_pCreatorsListWidget->selectedItems();
		if (items.size() > 0)
		{
			RBXASSERT(items.size() == 1);
			CreatorItem* pCreatorItem = dynamic_cast<CreatorItem*>(items.at(0));
			if (pCreatorItem)
				onRemoveCloudEditorRequested(pCreatorItem->getPlayerId());
		}
	}
}

void RobloxTeamCreateWidget::onPublishDialogFinished(int status)
{
	disconnect(qobject_cast<QDialog*>(sender()), SIGNAL(finished(int)), this, SLOT(onPublishDialogFinished(int)));
	QMetaObject::invokeMethod(this, "updateCurrentWidgetInStack", Qt::QueuedConnection);
}

void RobloxTeamCreateWidget::onPlaceIdChanged(const RBX::Reflection::PropertyDescriptor* desc)
{
	if (!desc || desc->name != "PlaceId")
		return;

	QString canManageUrl = QString("%1/users/%2/canmanage/%3").arg(RobloxSettings::getApiBaseURL()).arg(RobloxUser::singleton().getUserId()).arg(m_pDataModel->getPlaceID());
	m_PlaceManageStatusQuery = RBX::HttpAsync::get(canManageUrl.toStdString()).then(boost::bind(&RobloxTeamCreateWidget::updatePlaceManageStatus, this, _1));
}

eTeamType RobloxTeamCreateWidget::getCreatorType()
{
	if (m_PlaceDetailsQuery.future_ && !m_PlaceDetailsQuery.is_ready())
		m_PlaceDetailsQuery.get();

	return m_PlaceCreatorDetails.teamType;
}

void RobloxTeamCreateWidget::updatePlaceManageStatus(RBX::HttpFuture future)
{
	EntityProperties placeManageProp;
	placeManageProp.setFromJsonFuture(future);

	m_bCanManageCurrentPlace = placeManageProp.get<bool>("CanManage").get_value_or(false);

	// if user can manage place and cloud edit is not running currently then launch cloud edit
	int universeId = -1;
	if (m_bCanManageCurrentPlace && !RobloxIDEDoc::getIsCloudEditSession() && canLaunchCloudEdit(universeId))
	{
		// if user publishes on existing cloud edit enabled place, then launch cloud edit
		QMetaObject::invokeMethod(&(UpdateUIManager::Instance().getMainWindow()),
			"cloudEditStatusChanged", Qt::QueuedConnection, Q_ARG(int, m_pDataModel->getPlaceID()), Q_ARG(int, universeId));
		return;
	}

	if (FFlag::TeamCreateCheckInvalidUniverse && universeId <= 0)
	{
		// reset data model, we will not be handling any updates for this place
		QMetaObject::invokeMethod(this, "resetDataModel", Qt::QueuedConnection);
		return;
	}

	int placeId = m_pDataModel->getPlaceID();
	if (placeId <= 0)
	{
		// ideally flow must not come here...
		RBXASSERT(placeId);
		QMetaObject::invokeMethod(this, "updateCurrentWidgetInStack", Qt::QueuedConnection);
		return;
	}

	QString url = m_bCanManageCurrentPlace ? QString("%1/places/%2/settings").arg(RobloxSettings::getBaseURL()).arg(placeId)
		                                   : QString("%1/Marketplace/ProductInfo?assetId=%2").arg(RobloxSettings::getApiBaseURL()).arg(placeId);
	m_PlaceDetailsQuery = RBX::HttpAsync::get(url.toStdString()).then(boost::bind(&RobloxTeamCreateWidget::updatePlaceDetails, this, _1));	
}

void RobloxTeamCreateWidget::updatePlaceDetails(RBX::HttpFuture future)
{
	typedef RBX::Reflection::ValueTable JsonTable;
	typedef shared_ptr<const JsonTable> JsonTableRef;

	EntityProperties placeSettingsProp;
	placeSettingsProp.setFromJsonFuture(future);

	JsonTableRef placeCreatorTable = placeSettingsProp.get<JsonTableRef >("Creator").get_value_or(JsonTableRef());
	if (placeCreatorTable)
	{
		EntityProperties placeCreatorProp;
		placeCreatorProp.setFromValueTable(placeCreatorTable);

		if (canManageCurrentPlace())
		{
			int creatorType = placeCreatorProp.get<int>("CreatorType").get_value_or(0);
			m_PlaceCreatorDetails.teamType = creatorType ? TEAM_GROUP : TEAM_USERS;

			m_PlaceCreatorDetails.ownerId = placeCreatorProp.get<int>("CreatorTargetId").get_value_or(-1);
		}
		else
		{
			m_PlaceCreatorDetails.teamType = TEAM_USERS;
			m_PlaceCreatorDetails.ownerId = placeCreatorProp.get<int>("Id").get_value_or(-1);				
		}

		std::string ownerName = placeCreatorProp.get<std::string>("Name").get_value_or("");
		if (!ownerName.empty())			
			m_PlaceCreatorDetails.ownerName = QString::fromStdString(ownerName);
	}

	QMetaObject::invokeMethod(this, "updateCurrentWidgetInStack", Qt::QueuedConnection);
}

bool RobloxTeamCreateWidget::canManageCurrentPlace()
{
	if (m_PlaceManageStatusQuery.future_ && !m_PlaceManageStatusQuery.is_ready())
		m_PlaceManageStatusQuery.get();
	return m_bCanManageCurrentPlace;
}

void RobloxTeamCreateWidget::enableFriendsInviteEdit(bool state)
{
	friendsInviteEdit->setVisible(state);
	if (state)
	{
		loadFriendsList();		

		QCompleter* pCompleter = friendsInviteEdit->completer();
		if (!pCompleter)
		{
			pCompleter = new QCompleter(friendsInviteEdit);		
			pCompleter->setCaseSensitivity(Qt::CaseInsensitive);
			friendsInviteEdit->setCompleter(pCompleter);
			if (FFlag::TeamCreateEnableCompleterFix)
				connect(pCompleter->popup(), SIGNAL(clicked(const QModelIndex &)), this, SLOT(onCompleterPopupMenuItemClicked(const QModelIndex&)));
		}
	}
}

void RobloxTeamCreateWidget::loadFriendsList()
{
	if (m_pFriendsInfoLoader)
		return;

	m_pFriendsInfoLoader = new FriendsInfoLoader(this, RobloxUser::singleton().getUserId());
	connect(m_pFriendsInfoLoader, SIGNAL(finishedLoading()), this, SLOT(updatePlayerFriendsInfo()));
}

void RobloxTeamCreateWidget::updatePlayerFriendsInfo()
{
	if (!canManageCurrentPlace())
		return;

	QCompleter* pCompleter = friendsInviteEdit->completer();
	RBXASSERT(pCompleter);

	PlayersInfoCollection friendsInfo = m_pFriendsInfoLoader->getFriendsInfo();
	QStringList friendsList;

	QMapIterator<int, QString> iter(friendsInfo);
	while (iter.hasNext())
	{
		iter.next();
		if (!m_pCreatorsListWidget || !m_pCreatorsListWidget->getCreatorItem(iter.key()))
			friendsList.append(iter.value());
	}

	// update friends list
	pCompleter->setModel(new QStringListModel(friendsList));
	// setting model destroys popup, so need to reinitialize delegate and connection
	pCompleter->popup()->setItemDelegate(new CompleterPopupDelegate(pCompleter->popup()));
	if (!FFlag::TeamCreateEnableCompleterFix)
		connect(pCompleter->popup(), SIGNAL(clicked(const QModelIndex &)), this, SLOT(onCompleterPopupMenuItemClicked(const QModelIndex&)));
}

void RobloxTeamCreateWidget::onFriendsInviteEditReturnPressed()
{
	QString playerName = friendsInviteEdit->text();
	if (playerName.isEmpty())
		return;
		
	if (m_pCreatorsListWidget && m_pCreatorsListWidget->getCreatorItem(playerName))
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Requested creator '%s' already exists.", qPrintable(playerName));
		return;	
	}

	QApplication::setOverrideCursor(Qt::BusyCursor);

	EntityProperties prop;
	prop.setFromJsonFuture(RBX::HttpAsync::get(QString("%1/users/get-by-username?username=%2").arg(RobloxSettings::getApiBaseURL()).arg(playerName).toStdString()));

	if (boost::optional<int> userIdOpt = prop.get<int>("Id"))
	{
		int playerId = userIdOpt.get();
		if (playerId > 0 && addCloudEditor(playerId))
		{
			if (m_pCreatorsListWidget)
				m_pCreatorsListWidget->addCreatorItem(playerId, playerName, "Offline");

			if (FFlag::TeamCreateEnableCompleterFix)
				QMetaObject::invokeMethod(friendsInviteEdit, "clear", Qt::QueuedConnection);
			else
				friendsInviteEdit->clear();

			updatePlayerFriendsInfo();
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create",  "Creator added from editor");
		}
	}
	else
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Requested creator '%s' does not exist.", qPrintable(playerName));
	}

	QApplication::restoreOverrideCursor();
}

void RobloxTeamCreateWidget::onCompleterPopupMenuItemClicked(const QModelIndex& clickedIndex)
{
	QString playerName = clickedIndex.data(Qt::DisplayRole).toString();
	int playerId = m_pFriendsInfoLoader ? m_pFriendsInfoLoader->getFriendsInfo().key(playerName, 0) : 0;

	QApplication::setOverrideCursor(Qt::BusyCursor);
	if (playerId > 0 && addCloudEditor(playerId))
	{
		if (m_pCreatorsListWidget)
			m_pCreatorsListWidget->addCreatorItem(playerId, playerName, "Offline");

		updatePlayerFriendsInfo();
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create",  "Creator added from popup");
	}
	QApplication::restoreOverrideCursor();

	friendsInviteEdit->clear();
}

void RobloxTeamCreateWidget::onRemoveCloudEditorRequested(int playerId)
{
	userRemoveButton->setProperty("userIdToDelete", playerId);
	m_pMainStackedWidgetAnimator->setCurrentIndex(USER_REMOVE_CONFIRM_PAGE);
}

void RobloxTeamCreateWidget::onRemoveCloudEditConfirm()
{
	QApplication::setOverrideCursor(Qt::BusyCursor);
	int playerId = userRemoveButton->property("userIdToDelete").toInt();
	if (playerId > 0 && removeCloudEditor(playerId))
	{
		CreatorItem* pCreatorItem = m_pCreatorsListWidget->getCreatorItem(playerId);
		if (pCreatorItem)
		{
			delete pCreatorItem;

			updatePlayerFriendsInfo();
			RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create",  "Creator removed");
		}
		if (FFlag::KickUsersWhenTheyLoseAccessToCloudEdit)
		{
			RBXASSERT(m_pDataModel);
			if (m_pDataModel)
			{
				RBX::DataModel::LegacyLock lock(m_pDataModel.get(), RBX::DataModelJob::Write);
				if (RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>())
				{
					RBX::Network::Players::event_requestCloudEditKick.replicateEvent(players, playerId);
				}
			}
		}
	}
	QApplication::restoreOverrideCursor();
	userRemoveButton->setProperty("userIdToDelete", QVariant());
	QMetaObject::invokeMethod(this, "updateCurrentWidgetInStack", Qt::QueuedConnection);
}

void RobloxTeamCreateWidget::onRemoveCloudEditCancel()
{
	userRemoveButton->setProperty("userIdToDelete", QVariant());
	QMetaObject::invokeMethod(this, "updateCurrentWidgetInStack", Qt::QueuedConnection);
}

bool RobloxTeamCreateWidget::addCloudEditor(int playerId)
{
	// if player id is owner, then do not add as editor
	if (playerId == m_PlaceCreatorDetails.ownerId)
		return false;

	int universeId = m_pDataModel->getUniverseId();

	RBXASSERT(universeId);
	RBXASSERT(playerId);

	bool canAddEditor = true;
	if (!RobloxIDEDoc::getIsCloudEditSession())
	{		
		QString postUrl = QString("%1/universes/%2/enablecloudedit").arg(RobloxSettings::getApiBaseURL()).arg(universeId);
		if (doPost(postUrl.toStdString()) == "{}")
		{
			// launch cloud edit
			QMetaObject::invokeMethod(&(UpdateUIManager::Instance().getMainWindow()), 
				"cloudEditStatusChanged", Qt::QueuedConnection, Q_ARG(int, m_pDataModel->getPlaceID()), Q_ARG(int, m_pDataModel->getUniverseId()));
		}
		else
		{
			canAddEditor = false;
			RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error while turning on cloud edit");
		}
	}

	if (canAddEditor)
	{
		QString postUrl = QString("%1/universes/%2/addcloudeditor?userId=%3").arg(RobloxSettings::getApiBaseURL()).arg(universeId).arg(playerId);
		std::string result = doPost(postUrl.toStdString());	
		return result == "{}";
	}

	return false;
}

bool RobloxTeamCreateWidget::removeCloudEditor(int playerId)
{
	int universeId = m_pDataModel->getUniverseId();

	RBXASSERT(universeId);
	RBXASSERT(playerId);

	QString postUrl = QString("%1/universes/%2/removecloudeditor?userId=%3").arg(RobloxSettings::getApiBaseURL()).arg(universeId).arg(playerId);
	std::string result = doPost(postUrl.toStdString());
	return result == "{}";
}

bool RobloxTeamCreateWidget::canLaunchCloudEdit(int &universeId)
{
	if (FFlag::DoNotCheckCanCloudEditWhenNotLoggedIn && RobloxUser::singleton().getUserId() <= 0)
	{
		return false;
	}

	universeId = m_pDataModel->getUniverseId();

	// in case universe id hasn't been set in datamodel, query for universe id
	if (universeId <= 0)
	{
		int placeId = m_pDataModel->getPlaceID();
		RBXASSERT(placeId > 0);

		EntityProperties universeIdProp;
		QString getUniverseUrl = QString("%1//universes/get-universe-containing-place?placeId=%2").arg(RobloxSettings::getApiBaseURL()).arg(placeId);
		universeIdProp.setFromJsonFuture(RBX::HttpAsync::get(getUniverseUrl.toStdString()));
		universeId = universeIdProp.get<int>("UniverseId").get_value_or(-1);
	}
	
	if (universeId > 0)
	{
		EntityProperties cloudEditEnabledProp;
		QString cloudEditEnabledUrl = QString("%1/universes/%2/cloudeditenabled").arg(RobloxSettings::getApiBaseURL()).arg(universeId);
		cloudEditEnabledProp.setFromJsonFuture(RBX::HttpAsync::get(cloudEditEnabledUrl.toStdString()));
		return cloudEditEnabledProp.get<bool>("enabled").get_value_or(false);
	}

	return false;
}

void RobloxTeamCreateWidget::resetDataModel()
{	setDataModel(shared_ptr<RBX::DataModel>(), shared_ptr<PlayersDataManager>()); }

std::string RobloxTeamCreateWidget::doPost(const std::string& postUrl)
{
	std::string result;
	try
	{
		RBX::HttpPostData d("", RBX::Http::kContentTypeDefaultUnspecified, false);
		result = RBX::HttpAsync::post(postUrl, d).get();
	}
	catch (const RBX::base_exception& e)
	{
		RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR, "Error: %s", e.what());
		RBX::RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_STUDIO, "Team Create",  "Post error");
		result = e.what();
	}

	return result;
}

//--------------------------------------------------------------------------------------------
// FriendsInfoLoader
//--------------------------------------------------------------------------------------------
static void invokeFriendsDataLoaded(QPointer<FriendsInfoLoader> friendsInfoLoader, int pageLoaded, RBX::HttpFuture future)
{
	if (friendsInfoLoader.isNull())
		return;
	QMetaObject::invokeMethod(friendsInfoLoader.data(), "friendsDataLoaded", Qt::QueuedConnection, 
		Q_ARG(int, pageLoaded), Q_ARG(RBX::HttpFuture, future));

}

FriendsInfoLoader::FriendsInfoLoader(QObject* parent, int playerId)
: QObject(parent)
, m_PlayerId(playerId)
, m_bFinishedLoading(false)
{
	loadPage(1);
}

void FriendsInfoLoader::loadPage(int pageToLoad)
{
	QString str = QString("%1/users/%2/friends?page=%3").arg(RobloxSettings::getApiBaseURL()).arg(m_PlayerId).arg(pageToLoad);
	RBX::HttpAsync::get(str.toStdString()).then(boost::bind(&invokeFriendsDataLoaded, QPointer<FriendsInfoLoader>(this), pageToLoad, _1));
}

void FriendsInfoLoader::friendsDataLoaded(int pageLoaded, RBX::HttpFuture future)
{
	try 
	{
		boost::shared_ptr<const RBX::Reflection::ValueArray> table;
		if (!RBX::WebParser::parseJSONArray(future.get(), table) || table->size() <= 0)
		{
			// we get empty table once the friends list is complete
			doneLoading();			
			return;
		}

		boost::optional<std::string> friendName;
		boost::optional<int> friendId;

		EntityProperties friendsProp;
		for (RBX::Reflection::ValueArray::const_iterator iter = table->begin(); iter != table->end(); ++iter)
		{
			if (iter->isType<shared_ptr<const RBX::Reflection::ValueTable> >())
			{
				friendsProp.setFromValueTable(iter->cast<shared_ptr<const RBX::Reflection::ValueTable> >());

				friendName = friendsProp.get<std::string>("Username");
				friendId   = friendsProp.get<int>("Id");

				if (friendName.is_initialized() && !friendName.get().empty() && friendId.is_initialized())
					m_FriendsInfo.insert(friendId.get(), friendName.get().c_str());
			}
		}
	}
	catch (const RBX::base_exception& )
	{
		doneLoading();
		return;
	}

	loadPage(pageLoaded+1);
}

void FriendsInfoLoader::doneLoading()
{
	m_bFinishedLoading = true;
	Q_EMIT finishedLoading();
}

//--------------------------------------------------------------------------------------------
// CreatorsListLoader
//--------------------------------------------------------------------------------------------
static void invokeCreatorsListLoaded(QPointer<CreatorsListLoader> creatorsListLoader, int pageLoaded, RBX::HttpFuture future)
{
	if (creatorsListLoader.isNull())
		return;
	QMetaObject::invokeMethod(creatorsListLoader.data(), "creatorsListLoaded", Qt::QueuedConnection, 
		Q_ARG(int, pageLoaded), Q_ARG(RBX::HttpFuture, future));

}

CreatorsListLoader::CreatorsListLoader(QObject* parent, int universeId)
	: QObject(parent)
	, m_UniverseId(universeId)
	, m_bFinishedLoading(false)
{
	loadPage(1);
}

void CreatorsListLoader::reload()
{
	if (!m_bFinishedLoading)
		return;

	m_Creators.clear();
	loadPage(1);
}

void CreatorsListLoader::loadPage(int pageToLoad)
{
	QString str = QString("%1/universes/%2/listcloudeditors?page=%3").arg(RobloxSettings::getApiBaseURL()).arg(m_UniverseId).arg(pageToLoad);
	RBX::HttpAsync::get(str.toStdString()).then(boost::bind(&invokeCreatorsListLoaded, QPointer<CreatorsListLoader>(this), pageToLoad, _1));
}

void CreatorsListLoader::creatorsListLoaded(int pageLoaded, RBX::HttpFuture future)
{	
	typedef RBX::Reflection::ValueArray JsonArray;
	typedef shared_ptr<const JsonArray> JsonArryRef;
	
	EntityProperties creatorsListProp;
	creatorsListProp.setFromJsonFuture(future);

	bool finalPage = creatorsListProp.get<bool>("finalPage").get_value_or(true);
	JsonArryRef creatorsList = creatorsListProp.get<JsonArryRef >("users").get_value_or(JsonArryRef());
	if (creatorsList)
	{
		int userId = -1;
		bool isAdmin = false;

		EntityProperties creatorsDetails;

		for (JsonArray::const_iterator iter = creatorsList->begin(); iter != creatorsList->end(); ++iter)
		{
			if (iter->isType<shared_ptr<const RBX::Reflection::ValueTable> >())
			{
				creatorsDetails.setFromValueTable(iter->cast<shared_ptr<const RBX::Reflection::ValueTable> >());

				userId = creatorsDetails.get<int>("userId").get_value_or(-1);
				isAdmin = creatorsDetails.get<bool>("isAdmin").get_value_or(false);

				if (userId > 0)
					m_Creators.insert(userId, isAdmin);
			}
		}	
	}
	
	if (!finalPage)
		loadPage(pageLoaded+1);
	else 
		doneLoading();
}

void CreatorsListLoader::doneLoading()
{
	m_bFinishedLoading = true;
	Q_EMIT finishedLoading();
}