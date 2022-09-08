/**
 * RobloxTeamCreateWidget.h
 * Copyright (c) 2015 ROBLOX Corp. All Rights Reserved.
 */

#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
#define QT_NO_KEYWORDS
#endif

#include <QTreeWidget>
#include "ui_TeamCreate.h"

#include "rbx/signal.h"
#include "Util/HttpAsync.h"

#include "PlayersDataManager.h"

namespace RBX {
	class Instance;
	class DataModel;
}
namespace QtUtilities {
	class StackedWidgetAnimator;
}
class PlayersDataManager;
class RobloxTeamCreateWidget;

typedef QMap<int, bool> CreatorsCollection;

enum eTeamType
{
	TEAM_USERS = 0,
	TEAM_GROUP
};

struct PlaceCreatorDetails
{
	QString ownerName;
	int ownerId;
	eTeamType teamType;

	PlaceCreatorDetails()
	: ownerId(-1), teamType(TEAM_USERS)
	{

	}
};

class GroupItem: public QObject, public QTreeWidgetItem
{
	Q_OBJECT
public:
	GroupItem(int groupId);
	int getGroupId() {  return m_GroupId;}

private Q_SLOTS:
	void groupDataLoaded(RBX::HttpFuture future);
	void groupThumbnailLoaded(RBX::HttpFuture future);

private:
	int   m_GroupId;
};

class CreatorItem: public QObject, public QTreeWidgetItem
{
	Q_OBJECT
public:
	CreatorItem(int playerId, bool isAdmin = false) 
	: m_PlayerId(playerId)
	, m_IsAdmin(isAdmin)
	{;}

	int getPlayerId() {  return m_PlayerId;}

private Q_SLOTS:
	void updatePlayerAvatar(const QImage& playerImage);
	void updatePlayerName(int playerId, const QString& userName);

private:
	int   m_PlayerId;
	bool  m_IsAdmin;
};

class CreatorsListLoader : public QObject
{
	Q_OBJECT
public:
	CreatorsListLoader(QObject* parent, int universeId);

	bool hasFinishedLoading() { return m_bFinishedLoading; }
	CreatorsCollection& getCreators() { return m_Creators; }

	void reload();

Q_SIGNALS:
	void finishedLoading();

private:
	void loadPage(int pageToLoad);
	void doneLoading();
	Q_INVOKABLE void creatorsListLoaded(int pageLoaded, RBX::HttpFuture future);

	CreatorsCollection          m_Creators;

	int                         m_UniverseId;
	bool                        m_bFinishedLoading;
};

class FriendsInfoLoader : public QObject
{
	Q_OBJECT
public:
	FriendsInfoLoader(QObject* parent, int playerId);

	bool hasFinishedLoading() { return m_bFinishedLoading; }
	PlayersInfoCollection& getFriendsInfo() { return m_FriendsInfo; }

Q_SIGNALS:
	void finishedLoading();

private:
	void loadPage(int pageToLoad);
	void doneLoading();

	Q_INVOKABLE void friendsDataLoaded(int pageLoaded, RBX::HttpFuture future);

	PlayersInfoCollection       m_FriendsInfo;
	int                         m_PlayerId;
	bool                        m_bFinishedLoading;
};

class CreatorsListWidget : public QTreeWidget
{
	Q_OBJECT
public:
	CreatorsListWidget(QWidget* parent, shared_ptr<RBX::DataModel> dataModel, shared_ptr<PlayersDataManager> dataManager);
	using QTreeWidget::itemFromIndex;

	virtual void addOwnerDetails(const PlaceCreatorDetails& creatorDetails) = 0;
	virtual bool canDeleteSelectedCreator() = 0;
	
	void addCreatorItem(int playerId, const QString& playerName, const QString& subLabel);

	CreatorItem* getCreatorItem(int playerId);
	CreatorItem* getCreatorItem(const QString& playerName);

Q_SIGNALS:
	void removeCloudEditorRequested(int playerId);
	void finishedLoading();

private Q_SLOTS:
	virtual void onPlayerAddRequsted(int playerId) = 0;
	virtual void onPlayerRemoveRequested(int playerId) = 0;

protected:	
	void onPlayerAdded(boost::shared_ptr<RBX::Instance> player);
	void onPlayerRemoving(boost::shared_ptr<RBX::Instance> player);	

	bool isPlayerPresent(int playerId);
	QString getPlayerName(int playerId);	

	void mousePressEvent(QMouseEvent *evt) override;	

	rbx::signals::scoped_connection       m_cPlayerAdded;
	rbx::signals::scoped_connection       m_cPlayerRemoving;

	boost::shared_ptr<RBX::DataModel>     m_pDataModel;
	boost::shared_ptr<PlayersDataManager> m_pPlayersDataManager;
};

class UserPlaceCreatorsListWidget: public CreatorsListWidget
{
	Q_OBJECT
public:
	UserPlaceCreatorsListWidget(QWidget* parent, shared_ptr<RBX::DataModel> dataModel, shared_ptr<PlayersDataManager> dataManager);
	void addOwnerDetails(const PlaceCreatorDetails& creatorDetails) override;

private Q_SLOTS:
	void onPlayerAddRequsted(int playerId) override;
	void onPlayerRemoveRequested(int playerId) override;
	void onCreatorsListLoaded();

private:
	void contextMenuEvent(QContextMenuEvent * evt) override;
	bool canDeleteSelectedCreator() override;

	void removeRemovedCreators();
	void addAddedCreators();

	CreatorsListLoader*    m_pCreatorsListLoader;
	int                    m_PlaceOwnerId;
};

class GroupPlaceCreatorsListWidget: public CreatorsListWidget
{
	Q_OBJECT
public:
	GroupPlaceCreatorsListWidget(QWidget* parent, shared_ptr<RBX::DataModel> dataModel, shared_ptr<PlayersDataManager> dataManager);
	void addOwnerDetails(const PlaceCreatorDetails& creatorDetails) override;

private Q_SLOTS:
	void onPlayerAddRequsted(int playerId) override;
	void onPlayerRemoveRequested(int playerId) override;

private:
	bool canDeleteSelectedCreator() override { return false; }
};

class RobloxTeamCreateWidget : public QWidget, public Ui::TeamCreate
{
	Q_OBJECT
public:
	RobloxTeamCreateWidget(QWidget* parent);
	void setDataModel(boost::shared_ptr<RBX::DataModel> dataModel, boost::shared_ptr<PlayersDataManager> pPlayersDataManager);

	PlaceCreatorDetails getPlaceCreatorDetails();

private Q_SLOTS:
	void onAuthenticationChanged(bool);
	void onLoginButtonClicked();
	void onPublishButtonClicked();
	void onPublishDialogFinished(int status);

	void onTurnOnButtonClicked();
	void onBottomMoreButtonClicked();

	void onFriendsInviteEditReturnPressed();

	void onCompleterPopupMenuItemClicked(const QModelIndex& clickedIndex);
	void onRemoveCloudEditorRequested(int userId);

	void onRemoveCloudEditConfirm();
	void onRemoveCloudEditCancel();

	void updatePlayerFriendsInfo();

private:
	enum eMainStackedWidgetPage
	{
		EMPTY_PAGE = 0,		
		PUBLISH_PAGE,
		PUBLISHING_MESSAGE_PAGE,
		OPTIONS_PAGE,
		USER_REMOVE_CONFIRM_PAGE
	};

	enum eOptionsStackedWidgetPage
	{
		LOGIN_PAGE = 0,
		USER_PLACE_OPTIONS_PAGE,
		GROUP_PLACE_OPTIONS_PAGE,
	};

	enum eUserPlaceOptionsStackedWidgetPage
	{
		USER_PLACE_TURN_ON_PAGE = 0,
		USER_PLACE_SAVE_MESSAGE_PAGE,
		USER_PLACE_CREATORS_LIST_PAGE
	};

	enum eGroupPlaceOptionsStackedWidgetPage
	{
		GROUP_PLACE_TURN_ON_PAGE = 0,
		GROUP_PLACE_NON_ADMIN_MESSAGE_PAGE,
		GROUP_PLACE_SAVE_MESSAGE_PAGE,
		GROUP_PLACE_CREATORS_LIST_PAGE
	};

	void addCreatorsListWidget(eTeamType creatorType);

	Q_INVOKABLE void updateCurrentWidgetInStack();
	void onPlaceIdChanged(const RBX::Reflection::PropertyDescriptor* desc);
	
	void enableFriendsInviteEdit(bool state);
	void loadFriendsList();

	void updatePlaceDetails(RBX::HttpFuture future);
	eTeamType getCreatorType();

	void updatePlaceManageStatus(RBX::HttpFuture future);
	bool canManageCurrentPlace();

	bool addCloudEditor(int playerId);
	bool removeCloudEditor(int playerId);
	
	bool canLaunchCloudEdit(int &universeId);
	Q_INVOKABLE void resetDataModel();

	std::string doPost(const std::string& postUrl);

	boost::shared_future<void>                     m_PlaceDetailsQuery;
	boost::shared_future<void>                     m_PlaceManageStatusQuery;

	boost::shared_ptr<RBX::DataModel>              m_pDataModel;
	boost::shared_ptr<PlayersDataManager>          m_pPlayersDataManager;
	rbx::signals::scoped_connection                m_cPlaceIdChange;

	CreatorsListWidget*                            m_pCreatorsListWidget;
	QtUtilities::StackedWidgetAnimator*            m_pMainStackedWidgetAnimator;
	FriendsInfoLoader*                             m_pFriendsInfoLoader;

	PlaceCreatorDetails                            m_PlaceCreatorDetails;
	bool                                           m_bCanManageCurrentPlace;
	bool                                           m_bLoginButtonClicked;
};

