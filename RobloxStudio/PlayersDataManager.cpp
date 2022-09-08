#include "stdafx.h" 
#include "PlayersDataManager.h"

#include <QPointer>
#include <QUrl>
#include <QPainter>

#include "V8DataModel/DataModel.h"
#include "Network/Players.h"
#include "Network/Player.h"
#include "V8Xml/WebParser.h"
#include "util/BrickColor.h"

#include "RobloxSettings.h"
#include "QtUtilities.h"
#include "RobloxGameExplorer.h"

FASTFLAGVARIABLE(TeamCreateReduceCopiesSelectionSet, true)

static const int kAvatarImageSize = 32;
static const int kMaxColors = 128;

static const QString kPlayersColorSettingKey = "Players Color";
static const QString kPlayerIdKey = "PlayerId";
static const QString kColorValueKey = "ColorVal";

//--------------------------------------------------------------------------------------------
// PlayersDataManager
//--------------------------------------------------------------------------------------------
PlayersDataManager::PlayersDataManager(QObject* parent, boost::shared_ptr<RBX::DataModel> dataModel)
: QObject(parent)
, m_pDataModel(dataModel)
, m_CurrentColorCount(0)
{
	qRegisterMetaType<PlayersColorCollection >();
	qRegisterMetaTypeStreamOperators<PlayersColorCollection >("PlayersColorCollection");

	loadPlayersColorSettings();

	RBXASSERT(m_pDataModel->currentThreadHasWriteLock());
	if (RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>())
	{
		for (auto& playerInstance : *(players->getPlayers()))
		{
			onPlayerAdded(playerInstance);
		}
		players->playerAddedSignal.connect(boost::bind(&PlayersDataManager::onPlayerAdded, this, _1));
		players->playerRemovingSignal.connect(boost::bind(&PlayersDataManager::onPlayerRemoving, this, _1));
	}
}

PlayersDataManager::~PlayersDataManager()
{
	savePlayersColorSettings();
}

void PlayersDataManager::getPlayerAvatar(int playerId, QObject* resultListener, const char* callbackName)
{
	QSharedPointer<AvatarLoader> avatarLoader = getAvatarLoader(playerId);
	if (avatarLoader && avatarLoader->hasFinishedLoading())
	{
		QMetaObject::invokeMethod(resultListener, callbackName, Qt::AutoConnection, Q_ARG(QImage, avatarLoader->getAvatar()));
	}
	else
	{
		if (!avatarLoader)
		{
			avatarLoader =  QSharedPointer<AvatarLoader>(new AvatarLoader(playerId, getPlayerColor(playerId)), &QObject::deleteLater);
			m_AvatarLoaders.push_back(avatarLoader);
		}

		// HACK: convert callback name to SLOT
		QString slot = QString("1%1(const QImage&)").arg(callbackName);
		connect(avatarLoader.data(), SIGNAL(avatarLoaded(const QImage&)), resultListener, qPrintable(slot));
	}
}

static void invokePlayerDataLoaded(QPointer<PlayersDataManager> playersDataManager, RBX::HttpFuture future)
{
	if (playersDataManager.isNull())
		return;
	QMetaObject::invokeMethod(playersDataManager.data(), "onPlayerNameLoaded", Qt::QueuedConnection, Q_ARG(RBX::HttpFuture, future));

}
void PlayersDataManager::getPlayerName(int playerId, QObject* resultListener, const char* callbackName)
{
	if (m_PlayersInfo.contains(playerId))
	{
		QMetaObject::invokeMethod(resultListener, callbackName, Qt::AutoConnection, Q_ARG(int, playerId), Q_ARG(QString, m_PlayersInfo.value(playerId)));
	}
	else
	{
		QString str = QString("%1/Users/%2").arg(RobloxSettings::getApiBaseURL()).arg(playerId);
		RBX::HttpAsync::get(str.toStdString()).then(boost::bind(&invokePlayerDataLoaded, QPointer<PlayersDataManager>(this), _1));

		// HACK: convert callback name to SLOT
		QString slot = QString("1%1(int, const QString&)").arg(callbackName);
		connect(this, SIGNAL(playerNameLoaded(int, const QString&)), resultListener, qPrintable(slot));
	}
}

int PlayersDataManager::getPlayerColor(int playerId)
{
	PlayersColorCollection::const_iterator iter = m_PlayersColor.find(playerId);
	if (iter != m_PlayersColor.end())
		return iter.value();

	m_PlayersColor[playerId] = getNextUnassignedColor();
	return m_PlayersColor[playerId];
}

shared_ptr<const RBX::Instances> PlayersDataManager::getLastKnownSelection(int playerId)
{
	PlayerSelectionMap::iterator iter = m_lastKnownSelectionByPlayerId.find(playerId);
	if (iter != m_lastKnownSelectionByPlayerId.end())
		return iter->second;

	// return empty value if the key doesn't exist
	static shared_ptr<const RBX::Instances> emptyInstances(new RBX::Instances());
	return emptyInstances;
}

bool PlayersDataManager::isLocalPlayer(boost::shared_ptr<RBX::Network::Player> player)
{
	RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>();
	return (players && players->getLocalPlayer() == player.get());
}

boost::shared_ptr<RBX::Network::Player> PlayersDataManager::getPlayer(const QString& playerName, Qt::CaseSensitivity cs)
{
	if (!playerName.isEmpty())
	{
		RBX::Network::Players* players = m_pDataModel->find<RBX::Network::Players>();
		if (players)
		{
			boost::shared_ptr<const RBX::Instances> playersList = players->getPlayers();
			for (RBX::Instances::const_iterator iter = playersList->begin(); iter != playersList->end(); ++iter)
			{
				RBX::Network::Player* player = boost::polymorphic_downcast<RBX::Network::Player*>(iter->get());
				if (QString::compare(player->getName().c_str(), playerName, cs) == 0)
					return shared_from(player);
			}
		}
	}

	return shared_ptr<RBX::Network::Player>();
}

QSharedPointer<AvatarLoader> PlayersDataManager::getAvatarLoader(int playerId)
{
	QListIterator<QSharedPointer<AvatarLoader> > iter(m_AvatarLoaders);
	for (int ii = 0; ii < m_AvatarLoaders.size(); ++ii)
	{
		if (m_AvatarLoaders.at(ii)->getPlayerId() == playerId)
			return m_AvatarLoaders.at(ii);
	}

	return QSharedPointer<AvatarLoader>();
}

void PlayersDataManager::onPlayerAdded(shared_ptr<RBX::Instance> playerInstance)
{
	if (shared_ptr<RBX::Network::Player> player = RBX::Instance::fastSharedDynamicCast<RBX::Network::Player>(playerInstance))
	{
		player->cloudEditSelectionChanged.connect(boost::bind(&PlayersDataManager::onCloudEditSelectionChanged,
			this, player->getUserID(), _1));
	}
}

void PlayersDataManager::onPlayerRemoving(shared_ptr<RBX::Instance> playerInstance)
{
	if (shared_ptr<RBX::Network::Player> player = RBX::Instance::fastSharedDynamicCast<RBX::Network::Player>(playerInstance))
	{
		m_lastKnownSelectionByPlayerId.erase(player->getUserID());
		// remove selections for the removing player
		Q_EMIT cloudEditSelectionChanged(player->getUserID());
	}
}

void PlayersDataManager::onCloudEditSelectionChanged(int playerId, shared_ptr<const RBX::Reflection::ValueArray>& newSelection)
{
	shared_ptr<RBX::Instances> inst(new RBX::Instances());
	inst->reserve(newSelection->size());
	for (auto& var : *newSelection)
	{
		if (var.isType<shared_ptr<RBX::Instance> >())
		{
			if (shared_ptr<RBX::Instance> i = var.cast<shared_ptr<RBX::Instance> >())
			{
				inst->push_back(i);
			}
		}
	}

	m_lastKnownSelectionByPlayerId[playerId] = inst;
	Q_EMIT cloudEditSelectionChanged(playerId);
}

/*
  Players color data present in registry:

  RobloxStudio
  |__
	  Players Color
	  |__ gametest2.robloxlabs.com
	  |   |_ PlayersColorCollection (will be having player id and color assigned)
	  |
	  |__ roblox.com
	     |_ PlayersColorCollection
*/

static QString getHostName()
{
	QUrl url(RobloxSettings::getBaseURL());
	return url.host().mid(url.host().indexOf(QString(".")) + 1);
}

void PlayersDataManager::loadPlayersColorSettings()
{
	RobloxSettings settings;
	settings.beginGroup(kPlayersColorSettingKey);
	m_PlayersColor = settings.value(getHostName()).value<PlayersColorCollection >();
	settings.endGroup();
}

void PlayersDataManager::savePlayersColorSettings()
{
	RobloxSettings settings;
	settings.beginGroup(kPlayersColorSettingKey);
	settings.setValue(getHostName(), QVariant::fromValue<PlayersColorCollection >(m_PlayersColor));
	settings.endGroup();
}

int PlayersDataManager::getNextUnassignedColor()
{
	using namespace RBX;
	static int colorsIndex[kMaxColors] = {  BrickColor::brick_45, BrickColor::brick_331,  BrickColor::roblox_1020, BrickColor::brick_334, 
											BrickColor::brick_335, BrickColor::brick_21, BrickColor::roblox_1006, BrickColor::roblox_1019, 
											BrickColor::brick_28, BrickColor::brick_337, BrickColor::roblox_1014, BrickColor::brick_307, 
											BrickColor::roblox_1030, BrickColor::roblox_1009, BrickColor::brick_352, BrickColor::roblox_1016, 
											BrickColor::roblox_1029, BrickColor::brick_119, BrickColor::brick_359, BrickColor::brick_361, 
											BrickColor::brick_194, BrickColor::brick_365, BrickColor::brick_328, BrickColor::roblox_1017,
											BrickColor::roblox_1032, BrickColor::brick_11, BrickColor::brick_324, BrickColor::brick_322,
											BrickColor::brick_321, BrickColor::brick_314, BrickColor::brick_318, BrickColor::brick_316,
											BrickColor::brick_315, BrickColor::brick_332, BrickColor::brick_313, BrickColor::brick_23, 
											BrickColor::brick_340, BrickColor::roblox_1021, BrickColor::brick_306, BrickColor::roblox_1018,
											BrickColor::roblox_1011, BrickColor::brick_26, BrickColor::brick_141, BrickColor::brick_329,
											BrickColor::roblox_1004, BrickColor::brick_333, BrickColor::brick_226, BrickColor::brick_336, 
											BrickColor::brick_338, BrickColor::brick_133, BrickColor::brick_341, BrickColor::brick_9,
											BrickColor::brick_344, BrickColor::brick_105, BrickColor::brick_348, BrickColor::brick_125,
											BrickColor::brick_192, BrickColor::brick_353, BrickColor::brick_5, BrickColor::brick_355,
											BrickColor::brick_357, BrickColor::brick_360, BrickColor::brick_362, BrickColor::brick_363,
											BrickColor::roblox_1003, BrickColor::roblox_1028, BrickColor::brick_350, BrickColor::roblox_1015,
											BrickColor::roblox_1026, BrickColor::brick_325, BrickColor::brick_153, BrickColor::roblox_1008,
											BrickColor::roblox_1013, BrickColor::brick_319, BrickColor::brick_151, BrickColor::roblox_1023,
											BrickColor::roblox_1027, BrickColor::brick_37, BrickColor::roblox_1010, BrickColor::brick_135,
											BrickColor::brick_309, BrickColor::brick_347, BrickColor::brick_302, BrickColor::brick_304,
											BrickColor::roblox_1012, BrickColor::brick_301, BrickColor::brick_330, BrickColor::brick_343,
											BrickColor::brick_24, BrickColor::brick_217, BrickColor::brick_342, BrickColor::roblox_1007,
											BrickColor::brick_106, BrickColor::roblox_1001, BrickColor::roblox_1025, BrickColor::brick_345,
											BrickColor::brick_346, BrickColor::brick_349, BrickColor::brick_101, BrickColor::brick_351,
											BrickColor::brick_354, BrickColor::brick_18, BrickColor::brick_356, BrickColor::brick_358,
											BrickColor::brick_38, BrickColor::brick_1, BrickColor::brick_364, BrickColor::brick_208,
											BrickColor::brick_29, BrickColor::brick_327, BrickColor::roblox_1002, BrickColor::brick_320,
											BrickColor::brick_323, BrickColor::brick_339, BrickColor::brick_104, BrickColor::roblox_1024,
											BrickColor::brick_317, BrickColor::roblox_1031, BrickColor::brick_311, BrickColor::roblox_1022,
											BrickColor::brick_312, BrickColor::brick_102, BrickColor::brick_310, BrickColor::brick_308,
											BrickColor::brick_305, BrickColor::brick_199, BrickColor::brick_303, BrickColor::brick_107 };

	return colorsIndex[m_PlayersColor.size() % kMaxColors];
}

void PlayersDataManager::onPlayerNameLoaded(RBX::HttpFuture future)
{
	EntityProperties playerProp;
	playerProp.setFromJsonFuture(future);

	boost::optional<std::string> playerName = playerProp.get<std::string>("Username");
	boost::optional<int> playerId = playerProp.get<int>("Id");

	if (playerName.is_initialized() && playerId.is_initialized() && !playerName.get().empty())
	{
		m_PlayersInfo.insert(playerId.get(), QString::fromStdString(playerName.get()));
		Q_EMIT playerNameLoaded(playerId.get(), QString::fromStdString(playerName.get()));
	}
}

//--------------------------------------------------------------------------------------------
// AvatarLoader
//--------------------------------------------------------------------------------------------
static void invokeHeadshotThumbnailDataLoaded(QPointer<AvatarLoader> avatarLoader, int playerId, RBX::HttpFuture future)
{
	if (avatarLoader.isNull())
		return;
	QMetaObject::invokeMethod(avatarLoader.data(), "headshotThumbnailDataLoaded", Qt::QueuedConnection, 
		Q_ARG(int, playerId), Q_ARG(RBX::HttpFuture, future));
}

static void invokeHeadshotThumbnailLoaded(QPointer<AvatarLoader> avatarLoader, int playerId, RBX::HttpFuture future)
{
	if (avatarLoader.isNull())
		return;
	QMetaObject::invokeMethod(avatarLoader.data(), "headshotThumbnailLoaded", Qt::QueuedConnection, 
		Q_ARG(int, playerId), Q_ARG(RBX::HttpFuture, future));
}

AvatarLoader::AvatarLoader(int playerId, int backgroundColor)
: m_PlayerId(playerId)
, m_BackgroundColor(backgroundColor)
, m_bFinishedLoading(false)
{
	// request player item update
	QString str = QString("%1/thumbnail/avatar-headshot?userid=%2").arg(RobloxSettings::getBaseURL()).arg(playerId);
	RBX::HttpAsync::get(str.toStdString()).then(boost::bind(&invokeHeadshotThumbnailDataLoaded, QPointer<AvatarLoader>(this), playerId, _1));
}

void AvatarLoader::headshotThumbnailDataLoaded(int playerId, RBX::HttpFuture future)
{
	boost::optional<std::string> thumnailUri;

	try
	{
		std::string result = future.get();
		boost::shared_ptr<const RBX::Reflection::ValueTable> table;
		if (RBX::WebParser::parseJSONTable(future.get(), table) && table->size() > 0)
		{
			RBX::Reflection::ValueTable::const_iterator finalStrIter = table->find("Final");
			if (finalStrIter != table->end() && finalStrIter->second.isType<bool>() && finalStrIter->second.get<bool>())
			{
				RBX::Reflection::ValueTable::const_iterator thumnailUrlIter = table->find("Url");
				if (thumnailUrlIter != table->end() && thumnailUrlIter->second.isString())
					thumnailUri = thumnailUrlIter->second.get<std::string>();
			}
			else
			{
				RBX::Reflection::ValueTable::const_iterator RetryUrlIter = table->find("RetryUrl");
				if (RetryUrlIter != table->end() && RetryUrlIter->second.isString())
					thumnailUri = RetryUrlIter->second.get<std::string>();
			}
		}
	}
	catch (const RBX::base_exception& )
	{

	}

	// TODO: in case of error shall we again try to reload avatar?
	if (thumnailUri.is_initialized() && !thumnailUri.get().empty())
	{
		RBX::HttpOptions options;
		options.setExternal(true);
		RBX::HttpAsync::get(thumnailUri.get(), options).then(boost::bind(&invokeHeadshotThumbnailLoaded, QPointer<AvatarLoader>(this), playerId, _1));
	}
}

void AvatarLoader::headshotThumbnailLoaded(int playerId, RBX::HttpFuture future)
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
			// set the image background here itself, so the same image can be used in both Creators panel and Chat widget
			QImage resultingImage(QSize(kAvatarImageSize,kAvatarImageSize), image.format());
			resultingImage.fill(QtUtilities::toQColor(RBX::BrickColor(m_BackgroundColor).color3()));
			QPainter painter(&resultingImage);
			painter.drawImage(0, 0, image.scaled(QSize(kAvatarImageSize,kAvatarImageSize), Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
			// we are done with image drawing, assign the resulting image
			m_PlayerAvatar = resultingImage;
		}
	}

	m_bFinishedLoading = true;
	// let connected slots to update image
	Q_EMIT avatarLoaded(m_PlayerAvatar);
	// remove connection
	disconnect(SIGNAL(avatarLoaded(const QImage&)));
}
