#pragma once

//this will make sure that we can use qt with boost (else moc compiler errors)
#ifndef QT_NO_KEYWORDS
#define QT_NO_KEYWORDS
#endif

#include <QObject>
#include <QList>
#include <QSharedPointer>
#include <QImage>
#include <QMap>
#include <QMetaType>

#include "rbx/signal.h"
#include "V8Tree/Instance.h"
#include "Util/HttpAsync.h"

namespace RBX {
	class DataModel;
	class BrickColor;
	namespace Network {
		class Player;
	}
}

typedef QMap<int, int>  PlayersColorCollection;
typedef QMap<int, QString> PlayersInfoCollection;
typedef boost::unordered_map<int, shared_ptr<const RBX::Instances> > PlayerSelectionMap;

Q_DECLARE_METATYPE(PlayersColorCollection)

class AvatarLoader: public QObject
{
	Q_OBJECT
public:
	AvatarLoader(int playerId, int backgroundColor);

	int getPlayerId() { return m_PlayerId; }
	bool hasFinishedLoading() { return m_bFinishedLoading; }
	QImage& getAvatar() { return m_PlayerAvatar; }

Q_SIGNALS:
	void avatarLoaded(const QImage& image);

private:
	Q_INVOKABLE void headshotThumbnailDataLoaded(int playerId, RBX::HttpFuture future);
	Q_INVOKABLE void headshotThumbnailLoaded(int playerId, RBX::HttpFuture future);

	QImage    m_PlayerAvatar;
	int       m_PlayerId;
	int       m_BackgroundColor;
	bool      m_bFinishedLoading;
};

class PlayersDataManager: public QObject
{
	Q_OBJECT
public:
	PlayersDataManager(QObject* parent, boost::shared_ptr<RBX::DataModel> dataModel);
	~PlayersDataManager();

	void getPlayerAvatar(int playerId, QObject* resultListener, const char* callbackName);
	void getPlayerName(int playerId, QObject* resultListener, const char* callbackName);

	int getPlayerColor(int playerId);

	// must be called with a datamodel lock
	shared_ptr<const RBX::Instances> getLastKnownSelection(int playerId);

	bool isLocalPlayer(boost::shared_ptr<RBX::Network::Player> player);
	boost::shared_ptr<RBX::Network::Player> getPlayer(const QString& playerName, Qt::CaseSensitivity cs = Qt::CaseInsensitive);

Q_SIGNALS:
	void cloudEditSelectionChanged(int playerId);
	void playerNameLoaded(int playerId, const QString& playerName);

private:
	QSharedPointer<AvatarLoader> getAvatarLoader(int playerId);

	void onPlayerAdded(shared_ptr<RBX::Instance> playerInstance);
	void onPlayerRemoving(shared_ptr<RBX::Instance> playerInstance);
	void onCloudEditSelectionChanged(int playerId, shared_ptr<const RBX::Reflection::ValueArray>& newSelection);

	Q_INVOKABLE void onPlayerNameLoaded(RBX::HttpFuture future);

	void loadPlayersColorSettings();
	void savePlayersColorSettings();

	int getNextUnassignedColor();

	PlayerSelectionMap                   m_lastKnownSelectionByPlayerId;

	boost::shared_ptr<RBX::DataModel>    m_pDataModel;
	QList<QSharedPointer<AvatarLoader> > m_AvatarLoaders;

	PlayersColorCollection               m_PlayersColor;
	PlayersInfoCollection                m_PlayersInfo;

	int                                  m_CurrentColorCount;
};