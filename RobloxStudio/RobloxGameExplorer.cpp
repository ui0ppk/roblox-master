#include "stdafx.h"

#include "RobloxGameExplorer.h"

#include "QtUtilities.h"
#include "RobloxDocManager.h"
#include "RobloxIDEDoc.h"
#include "RobloxMainWindow.h"
#include "RobloxScriptDoc.h"
#include "RobloxSettings.h"
#include "RobloxStudioVerbs.h"
#include "RobloxWebDoc.h"
#include "UpdateUIManager.h"
#include "RobloxServicesTools.h"

#include "Util/ContentId.h"
#include "Util/LuaWebService.h"
#include "Util/RobloxGoogleAnalytics.h"
#include "V8DataModel/ContentProvider.h"
#include "V8DataModel/DataModel.h"
#include "V8DataModel/Decal.h"
#include "V8DataModel/PartOperationAsset.h"
#include "V8Xml/WebParser.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPoint>
#include <QPushButton>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTextstream>
#include <QTreeView>
#include <QWidget>
#include <QClipboard>

#include <boost/algorithm/string.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>

FASTINTVARIABLE(GameViewItemCap, 200)
FASTINTVARIABLE(NewGameTemplateId, 95206881)
FASTFLAG(StudioCSGAssets)
FASTFLAGVARIABLE(GameExplorerCopyPath, false)
FASTFLAGVARIABLE(GameExplorerUseV2AliasEndpoint, false)
FASTFLAGVARIABLE(GameExplorerCopyId, false)
FASTFLAGVARIABLE(UseNewBadgesCreatePage, false)

Q_DECLARE_METATYPE(boost::function<void()>);
Q_DECLARE_METATYPE(boost::function<void(const QPoint&)>);
Q_DECLARE_METATYPE(EntityProperties*);
Q_DECLARE_METATYPE(RBX::HttpFuture);

using namespace RBX;
using namespace Reflection;

namespace
{

const int kNoGameLoaded = -1;
const QString kImageAssetNamePrefix = "Images/";
const QString kErrorTextStyleSheet = "color: #d86868";
const QString kErrorTextInputStyleSheet = "border: 2px solid #d86868";
const char* kLastImageDirectoryKey = "image_last_directory";

enum GameExplorerDataRoles
{
	ROLE_CategoryIfGroupRoot = Qt::UserRole + 1,
	ROLE_EntityProperties,
	ROLE_DoubleClickCallback,
	ROLE_ContextMenuCallback,
	ROLE_NameChangedCallback,
};

void placesPropertiesFixer(int universeId, EntityProperties* properties);
void badgesPropertiesFixer(int universeId, EntityProperties* properties);
void namedAssetsPropertiesFixer(int universeId, EntityProperties* properties);

const CategoryWebSettings kWebSettings[ENTITY_CATEGORY_MAX] =
{
	// Places
	{
		"Places",
		"PlaceId",
		"ID",
		"%API_PROXY%/universes/get-universe-places?universeId=%UNIVERSE_ID%&page=%ITEM_ID%",
		"%WEB_BASE%/ide/places/create?universeId=%UNIVERSE_ID%",
		"%WEB_BASE%/ide/places/%ITEM_ID%/updatesettings",
		"%WEB_BASE%/universes/removeplace?universeId=%UNIVERSE_ID%&placeId=%ITEM_ID%",
		"%WEB_BASE%/asset/?id=%ITEM_ID%",
		"%DATA_BASE%/Data/Upload.ashx?assetid=%ITEM_ID%",
		&placesPropertiesFixer
	},
	// Developer products
	{
		"DeveloperProducts",
		"ProductId",
		"ProductId",
		"%API_PROXY%/developerproducts/list?universeId=%UNIVERSE_ID%&page=%ITEM_ID%",
		"%API_PROXY%/developerproducts/add?universeId=%UNIVERSE_ID%",
		"%API_PROXY%/developerproducts/update?universeId=%UNIVERSE_ID%",
		"", // remove not supported
		"", // dev products don't have content to get/set
		"", // no content, so no content write url
		NULL // no properties fixer
	},
	{
		"Places",
		"PlaceId",
		"BadgeId",
		"%API_PROXY%/universes/get-universe-places?universeId=%UNIVERSE_ID%&page=%ITEM_ID%",
		"",
		"",
		"",
		"",
		"",
		&badgesPropertiesFixer
	},
	// Named assets
	{
		"Aliases",
		"Name",
		"Name",
		"%API_PROXY%/universes/get-aliases?universeId=%UNIVERSE_ID%&page=%ITEM_ID%",
		"%API_PROXY%/universes/create-alias-v2?universeId=%UNIVERSE_ID%",
		"%API_PROXY%/universes/update-alias-v2?universeId=%UNIVERSE_ID%&oldName=%ITEM_ID%",
		"%API_PROXY%/universes/delete-alias?universeId=%UNIVERSE_ID%&name=%ITEM_ID%",
		"", // Images do not need to have content get/set on publish
		"", // See above
		&namedAssetsPropertiesFixer
	},
};

const CategoryWebSettings badgesWebSettings = {"GameBadges",
											   "PlaceId",
											   "BadgeId",
											   "%WEB_BASE%/badges/list-badges-for-place/json?placeId=%ITEM_ID%",
											   "",
											   "",
											   "",
											   "",
											   "",
											   NULL};

template<class IdType>
IdType extractPropertyHelper(boost::shared_future<std::string> future, const std::string fieldName)
{
	Variant v;
	WebParser::parseJSONObject(future.get(), v);
	return v.cast<shared_ptr<const ValueTable> >()->at(fieldName).cast<IdType>();
}

template<class IdType>
static boost::function<IdType()> extractProperty(boost::shared_future<std::string> future, const std::string& fieldName)
{
	return boost::bind(&extractPropertyHelper<IdType>, future, fieldName);
}

QVariant getEntityId(EntityProperties* settings, EntityCategory category) {
	QVariant id;
	if (settings)
	{
		const std::string& fieldName = kWebSettings[category].perItemIdFieldName;
	
		boost::optional<int> intId = settings->get<int>(fieldName);
		boost::optional<std::string> stringId = settings->get<std::string>(fieldName);
		RBXASSERT(intId.is_initialized() != stringId.is_initialized());
		if (intId)
		{
			id = intId.get();
		}
		else if (stringId)
		{
			id = QString::fromStdString(stringId.get());
		}
	}
	return id;
}

QVariant getEntityId(QStandardItem* item, EntityCategory category)
{
	return getEntityId(item->data(ROLE_EntityProperties).value<EntityProperties*>(), category);
}

QVariant qvar(boost::function<void()> val)
{
	QVariant result;
	result.setValue<boost::function<void()> >(val);
	return result;
}

QVariant qvar(EntityProperties* val)
{
	QVariant result;
	result.setValue<EntityProperties*>(val);
	return result;
}

QVariant qvar2(boost::function<void(const QPoint&)> val)
{
	QVariant result;
	result.setValue<boost::function<void(const QPoint&)> >(val);
	return result;
}

template<class T>
T identity(T x) { return x; }

std::string formatUrl(const std::string& formatString, int universeId, QVariant itemId = -1)
{
	const QString baseUrl = RobloxSettings::getBaseURL();
	const std::string stdBaseUrl = baseUrl.toStdString();
	const QString apiBaseUrl = QString::fromStdString(ContentProvider::getUnsecureApiBaseUrl(stdBaseUrl));
	const QString dataBaseUrl = QString::fromStdString(ReplaceTopSubdomain(stdBaseUrl, "data"));

	return QString::fromStdString(formatString)
		.replace("%WEB_BASE%", baseUrl)
		.replace("%DATA_BASE%", dataBaseUrl)
		.replace("%API_PROXY%", apiBaseUrl)
		.replace("%UNIVERSE_ID%", QString("%1").arg(universeId))
		.replace("%ITEM_ID%", QString::fromStdString(Http::urlEncode(itemId.toString().toStdString())))
		.toStdString();
}

std::string formatUrl(const std::string& formatString, int universeId, std::string itemId)
{
	return formatUrl(formatString, universeId, QString::fromStdString(itemId));
}

Variant fixNumbersForWeb(const Variant& v)
{
	// MVC framework does not parse numbers for optional arguments well. It
	// can succeed if the optional numbers are sent in strings; to be careful, wrap
	// all numbers as strings.
	// http://stackoverflow.com/questions/8964646/javascriptserializer-and-asp-net-mvc-model-binding-yield-different-results/8968207#8968207

	if (v.isFloat())
	{
		return format("%.30g", v.cast<double>());
	}
	else if (v.isType<int>())
	{
		return format("%d", v.cast<int>());
	}
	else if (v.isType<shared_ptr<const ValueTable> >())
	{
		shared_ptr<ValueTable> newTable(new ValueTable);
		shared_ptr<const ValueTable> ctbl = v.cast<shared_ptr<const ValueTable> >();
		for (ValueTable::const_iterator itr = ctbl->begin(); itr != ctbl->end(); ++itr)
		{
			(*newTable)[itr->first] = fixNumbersForWeb(itr->second);
		}
		return shared_ptr<const ValueTable>(newTable);
	}
	else if (v.isType<shared_ptr<const ValueArray> >())
	{
		shared_ptr<const ValueArray> carr = v.cast<shared_ptr<const ValueArray> >();
		shared_ptr<ValueArray> newArray(new ValueArray(carr->size()));
		for (size_t i = 0; i < carr->size(); ++i)
		{
			(*newArray)[i] = fixNumbersForWeb((*carr)[i]);
		}
		return shared_ptr<const ValueArray>(newArray);
	}
	else
	{
		return v;
	}
}

void blockingMakeRoot(int gameId, int placeId, bool* success)
{
	try
	{
		HttpPostData d("", Http::kContentTypeDefaultUnspecified, false);
		HttpAsync::post(formatUrl("%WEB_BASE%/universes/setrootplace?universeid=%UNIVERSE_ID%&placeid=%ITEM_ID%",
			gameId, placeId), d).get();
	}
	catch (const RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while setting start place: %s", e.what());
		*success = false;
		return;
	}
	*success = true;
}

void blockingUnMakeRoot(int gameId, int placeId, bool* success)
{
	try
	{
		HttpPostData d("", Http::kContentTypeDefaultUnspecified, false);
		HttpAsync::post(formatUrl("%WEB_BASE%/universes/unrootplace?universeid=%UNIVERSE_ID%&placeid=%ITEM_ID%",
			gameId, placeId), d).get();
	}
	catch (const RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while un-setting start place: %s", e.what());
		*success = false;
		return;
	}
	*success = true;
}

void placesPropertiesFixer(int universeId, EntityProperties* properties)
{
	const QVariant id = properties->get<int>(kWebSettings[ENTITY_CATEGORY_Places].jsonItemIdFieldName).get();
	HttpOptions options;
	options.setDoNotUseCachedResponse();
	properties->setFromJsonFuture(HttpAsync::get(
		formatUrl("%API_PROXY%/Marketplace/ProductInfo?assetId=%ITEM_ID%", universeId, id), options));		

	try
	{
		// TODO: On FFlag removal, replace ID with AssetId in CategoryWebSettings for place
		properties->set("ID", properties->getVariant("AssetId").get());

		// replace creator information for the place (available only if place owner is logged in)	
		boost::function<shared_ptr<const ValueTable>()> f = extractProperty<shared_ptr<const ValueTable> >(HttpAsync::get(
			formatUrl("%WEB_BASE%/places/%ITEM_ID%/settings", universeId, id), options), "Creator");

		shared_ptr<const ValueTable> jsonList = f();
		if (jsonList && jsonList->size() > 0)
			properties->set("Creator", jsonList);
	}
	catch (...)
	{
	}
}

void badgesPropertiesFixer(int universeId, EntityProperties* properties)
{
	// get place id
	const QVariant id = properties->get<int>(kWebSettings[ENTITY_CATEGORY_Badges].jsonItemIdFieldName).get();

	// load badges information for the place
	HttpOptions options;
	options.setDoNotUseCachedResponse();
	boost::function<shared_ptr<const ValueArray>()> f = extractProperty<shared_ptr<const ValueArray> >(HttpAsync::get(
		formatUrl(badgesWebSettings.fetchPageUrlFormatString, universeId, id), options), badgesWebSettings.jsonItemGroupFieldName);

	// set badges list
	shared_ptr<const ValueArray> jsonList = f();
	for (size_t i = 0; i < jsonList->size(); ++i)
	{
		EntityProperties newProp;
		newProp.setFromValueTable(jsonList->at(i).cast<shared_ptr<const ValueTable> >());
		properties->addChild(newProp);
	}
}

void namedAssetsPropertiesFixer(int universeId, EntityProperties* properties)
{
	if (boost::optional<shared_ptr<const ValueTable> > optAssetInfo =
		properties->get<shared_ptr<const ValueTable> >("Asset"))
	{
		shared_ptr<const ValueTable> assetInfo = optAssetInfo.get();
		properties->set("AssetId", assetInfo->find("Id")->second);
		properties->set("AssetTypeId", assetInfo->find("TypeId")->second);
	}
}

QString generalDisplayNameGetter(EntityProperties* settings)
{
	return QString::fromStdString(settings->get<std::string>("Name").get_value_or(""));
}

QString namedImagesDisplayNameGetter(EntityProperties* settings)
{
	QString name = generalDisplayNameGetter(settings);
	RBXASSERT(name.startsWith(kImageAssetNamePrefix));
	if (name.startsWith(kImageAssetNamePrefix))
	{
		name = name.right(name.size() - kImageAssetNamePrefix.size());
	}
	return name;
}

bool namedAssetsShouldBuildExplorerItemCallback(EntityProperties* properties)
{
	if (boost::optional<int> nameType = properties->get<int>("Type"))
	{
		return nameType == (int)ALIAS_TYPE_Asset || nameType == (int)ALIAS_TYPE_AssetVersion;
	}
	return true;
}

bool validateImageName(const QString& name, const std::vector<QString>* usedNames, QString* errorMessage)
{
	bool valid = true;
	if (name.trimmed().isEmpty())
	{
		valid = false;
		*errorMessage = "Name cannot be empty";
	}

	if (usedNames)
	{
		QString compareName = QString(kImageAssetNamePrefix + name);
		for (std::vector<QString>::const_iterator itr = usedNames->begin();
			itr != usedNames->end(); ++itr)
		{
			if (compareName.compare(*itr, Qt::CaseInsensitive) == 0)
			{
				valid = false;
				*errorMessage = "Name already taken (not case sensitive)";
				break;
			}
		}
	}

	if (name.contains(QChar('\\')) || name.contains(QChar('/')))
	{
		valid = false;
		*errorMessage = "Name cannot include '\\' or '/'";
	}

	return valid;
}

void findAvailableImageNameFromFilename(const QString& fileName, const std::vector<QString>* usedNames, QString* availableImageName)
{
	QString completedBase = QFileInfo(fileName).completeBaseName();
	*availableImageName = completedBase;
	QString unusedErrorMessage;

	// Make up to #usedNames+1 attempts to find a free name: worst case, all used names
	// conflict, so need to try one more than that to get a free name.
	for (size_t attempt = 0; attempt <= usedNames->size() &&
		!validateImageName(*availableImageName, usedNames, &unusedErrorMessage);
		++attempt)
	{
		*availableImageName = QString("%1 (%2)").arg(completedBase).arg(attempt + 1);
	}
}

void createImageAssetAndNameThread(const QString& name, const QString& imageFileName, int currentGameId,
	boost::optional<int> currentGameGroupId, bool* created, QString* errorMessage)
{
	EntityProperties createImageAssetResponse;
	HttpPostData postData(
		shared_ptr<std::istream>(new std::ifstream(imageFileName.toStdString().c_str(), std::ios::binary)),
		Http::kContentTypeDefaultUnspecified, false);
	std::string createImageUrl = formatUrl("%DATA_BASE%/data/upload/json?assetTypeId=13&name=%ITEM_ID%&description=madeinstudio", -1, name);
	if (currentGameGroupId)
	{
		createImageUrl += format("&groupId=%d", currentGameGroupId.get());
	}
	createImageAssetResponse.setFromJsonFuture(HttpAsync::post(createImageUrl, postData));
	
	bool succeededInCreatingAsset = createImageAssetResponse.get<bool>("Success").get_value_or(false);
	boost::optional<int> optionalAssetId = createImageAssetResponse.get<int>("BackingAssetId");
	if (!succeededInCreatingAsset || !optionalAssetId.is_initialized())
	{
		*created = false;
		std::string returnedMessage = createImageAssetResponse.get<std::string>("Message").get_value_or("Try again later.");
		*errorMessage =QString("Failed to create image: %1").arg(QString::fromStdString(returnedMessage));
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
			GA_CATEGORY_ACTION, "Game Explorer", "Failed to create image asset", 0, false), flag);
		return;
	}
	int assetId = optionalAssetId.get();
	
	EntityProperties createNameRequest;
	createNameRequest.set("Name", name.toStdString());
	if (FFlag::GameExplorerUseV2AliasEndpoint)
	{
		createNameRequest.set("Type", (int)ALIAS_TYPE_Asset);
		createNameRequest.set("TargetId", assetId);
	}
	else
	{
		createNameRequest.set("AssetId", assetId);
	}
	
	Http http(formatUrl(kWebSettings[ENTITY_CATEGORY_NamedAssets].createFormatString,
		currentGameId, name));
	std::string propertiesJson = createNameRequest.asJson();
	std::istringstream propertiesStream(propertiesJson);
	std::string postResponse;
	try
	{
		http.post(propertiesStream, Http::kContentTypeApplicationJson, false, postResponse);
	}
	catch (const RBX::base_exception& e)
	{
		*created = false;
		*errorMessage = "An error occurred. See Output Window for details.";
		std::string gaMessage = format("Error while creating named asset: %s", e.what());
		StandardOut::singleton()->print(MESSAGE_ERROR, gaMessage.c_str());
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
			GA_CATEGORY_ACTION, "Game Explorer", gaMessage.c_str(), 0, false), flag);
		return;
	}

	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "Game Explorer", "Created named image", 0);

	*created = true;
}

void bulkAddNewImageNamesThread(std::vector<std::pair<QString, QString> >* fileAndImageNames, int currentGameId,
	boost::optional<int> currentGameGroupId)
{
	std::vector<char> results(fileAndImageNames->size(), 0);
	std::vector<QString> errorMessages(fileAndImageNames->size());
	boost::thread_group threadGroup;
	for (size_t i = 0; i < fileAndImageNames->size(); ++i)
	{
		threadGroup.create_thread(boost::bind(&createImageAssetAndNameThread,
			kImageAssetNamePrefix + (*fileAndImageNames)[i].second, (*fileAndImageNames)[i].first, currentGameId,
			currentGameGroupId, (bool*)(&results[i]), &errorMessages[i]));
	}
	threadGroup.join_all();

	// TODO: show modal with failed file names
	int failed = 0;
	for (size_t i = 0; i < fileAndImageNames->size(); ++i)
	{
		if (!results[i])
		{
			failed++;
		}
	}
	if (failed > 0)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Failed to upload %d images", failed);
	}
}

void updateNameThread(const std::string& properties, const std::string& updateUrl, bool* success,
	std::string* responseValue)
{
	Http updater(updateUrl);
	std::istringstream serializedSettings(properties);
	std::string postResponse;

	try
	{
		updater.post(serializedSettings, Http::kContentTypeApplicationJson, false, postResponse);
	}
	catch (const RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while updating name: %s", e.what());
		*success = false;
		return;
	}

	*success = true;

	if (responseValue)
	{
		*responseValue = postResponse;
	}
}

QString getNameOrEmptyIfNotAssetType(int assetType, EntityProperties* settings)
{
	if (boost::optional<int> assetId = settings->get<int>("AssetTypeId"))
	{
		if (assetId.get() == assetType)
		{
			return generalDisplayNameGetter(settings);
		}
	}
	return QString();
}

std::string getPropertiesAsJason(int assetType, EntityProperties* settings)
{
	if (boost::optional<int> assetId = settings->get<int>("AssetTypeId"))
	{
		if (assetId.get() == assetType)
		{
			return settings->asJson();
		}
	}
	return std::string();
}

static EntityProperties* getPropertiesIfNameAndAssetTypeMatch(const std::string& name, int assetType, EntityProperties* properties)
{
	if (name == properties->get<std::string>("Name").get() &&
		assetType == properties->get<int>("AssetTypeId").get())
	{
		return properties;
	}
	else
	{
		return NULL;
	}
}

QString getUnpublishedChangesDirectory(int universeId)
{
	return QDir(AppSettings::instance().tempLocation() + QString("/UnpublishedChanges/%1/").arg(universeId))
		.absolutePath() + "/";
}

QString getUnpublishedAssetFilePath(const std::string& assetName, int universeId)
{
	return getUnpublishedChangesDirectory(universeId) + QString::fromStdString(assetName);
}

bool anyFilesRecursive(QDir root)
{
	QStringList files = root.entryList(QDir::Files);
	if (!files.empty())
	{
		return true;
	}

	QStringList dirs = root.entryList(QDir::NoDotAndDotDot | QDir::Dirs);
	for (QStringList::iterator itr = dirs.begin(); itr != dirs.end(); ++itr)
	{
		if (anyFilesRecursive(QDir(root.filePath(*itr))))
		{
			return true;
		}
	}
	return false;
}

void removeFile(QString path, HttpFuture future)
{
	try
	{
		future.get();
		QFile(path).remove();
	}
	catch (const RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while publishing script: %s",
			e.what());
	}
}

void deleteFilesAndFoldersRecursive(QDir root)
{
	if (!root.exists())
	{
		return;
	}

	QFileInfoList list = root.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
	for (QFileInfoList::iterator itr = list.begin(); itr != list.end(); ++itr)
	{
		if (itr->isFile())
		{
			QFile(itr->absoluteFilePath()).remove();
		}
		else if (itr->isDir())
		{
			deleteFilesAndFoldersRecursive(QDir(itr->absoluteFilePath()));
		}
	}
	QDir().rmdir(root.absolutePath());
}

EntityProperties* getGroupIdFromPlaceEntityPropertyHelper(boost::optional<int>* groupId, EntityProperties* placeProperties)
{
	typedef Reflection::ValueTable JsonTable;
	typedef shared_ptr<const JsonTable> JsonTableRef;
	try
	{
		JsonTableRef r = placeProperties->get<JsonTableRef>("Creator").get();
		// CreatorType 0 = User 1 = Group
		JsonTable::const_iterator iter = r->find("CreatorType");
		if (iter != r->end() && iter->second.isNumber() && iter->second.cast<int>() == 1)
		{
			*groupId = r->find("CreatorTargetId")->second.cast<int>();
		}
	}
	catch (const RBX::base_exception&)
	{
	}
	return placeProperties;
}

boost::optional<int> getGroupIdFromPlaceEntityProperties(EntityPropertiesForCategory::ref categoryProperties)
{
	std::vector<EntityProperties*> unused;
	boost::optional<int> groupId;
	categoryProperties->transform<EntityProperties*>(boost::bind(&getGroupIdFromPlaceEntityPropertyHelper, &groupId, _1),
		&unused);
	return groupId;
}

} // end anonymous namespace

EntityProperties::EntityProperties()
	: hasPendingFuture(false)
	, ableToGetFuture(true)
	, ableToParseJson(true)
{
}

void EntityProperties::setFromJsonFuture(const boost::shared_future<std::string>& jsonFuture)
{
	hasPendingFuture = true;
	this->jsonFuture = jsonFuture;
}

void EntityProperties::setFromValueTable(const boost::shared_ptr<const ValueTable>& valueTable)
{
	hasPendingFuture = false;
	ableToGetFuture = true;
	ableToParseJson = true;

	data.clear();
	data.insert(valueTable->begin(), valueTable->end());
}

void EntityProperties::setFromEntitySettings(EntityProperties& other)
{
	waitUntilLoaded();
	other.waitUntilLoaded();
	RBXASSERT(!other.hasPendingFuture);
	RBXASSERT(other.ableToGetFuture);
	RBXASSERT(other.ableToParseJson);

	hasPendingFuture = false;
	ableToGetFuture = true;
	ableToParseJson = true;

	data.clear();
	data.insert(other.data.begin(), other.data.end());
}

std::string EntityProperties::asJson()
{
	waitUntilLoaded();

	std::string result;
	shared_ptr<ValueTable> val(new ValueTable);
	for (ValueTable::iterator itr = data.begin(); itr != data.end(); ++itr)
	{
		(*val)[itr->first] = fixNumbersForWeb(itr->second);
	}
	WebParser::writeJSON(shared_ptr<const ValueTable>(val), result);
	return result;
}

void EntityProperties::waitUntilLoaded()
{
	if (!hasPendingFuture)
	{
		return;
	}

	hasPendingFuture = false;
	std::string json;
	try
	{
		json = jsonFuture.get();
	}
	catch (const RBX::base_exception&)
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
			GA_CATEGORY_ACTION, "Game Explorer", "load entity settings http failure", 0, false), flag);
		ableToGetFuture = false;
		ableToParseJson = false;
		return;
	}

	ableToGetFuture = true;

	shared_ptr<const ValueTable> table;
	ableToParseJson = WebParser::parseJSONTable(json, table);

	if (!ableToParseJson) 
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
			GA_CATEGORY_ACTION, "Game Explorer", "load entity settings json parse failure", 0, false), flag);
		return;
	}

	data.clear();
	data.insert(table->begin(), table->end());
}

bool EntityProperties::isLoadedAndParsed() const
{
	if (hasPendingFuture) return false;
	for (EntityCollection::const_iterator itr = children.begin(); itr != children.end(); ++itr)
	{
		if (!itr->isLoadedAndParsed())
		{
			return false;
		}
	}
	return ableToGetFuture && ableToParseJson;
}

boost::optional<Variant> EntityProperties::getVariant(const std::string& fieldName)
{
	waitUntilLoaded();
	boost::optional<Variant> result;
	RBX::Reflection::ValueTable::iterator itr = data.find(fieldName);
	if (itr != data.end())
	{
		result = itr->second;
	}
	return result;
}

void UniverseSettings::setFromJsonFuture(const boost::shared_future<std::string>& jsonFuture)
{
	properties.setFromJsonFuture(jsonFuture);
}

void UniverseSettings::waitUntilLoaded()
{
	properties.waitUntilLoaded();
}

bool UniverseSettings::isLoadedAndParsed() const
{
	return properties.isLoadedAndParsed();
}

bool UniverseSettings::hasRootPlace()
{
	return properties.get<int>("RootPlace").get();
}

int UniverseSettings::rootPlaceId()
{
	boost::optional<int> rootPlaceId = properties.get<int>("RootPlace");
	RBXASSERT(rootPlaceId);
	if (rootPlaceId)
	{
		return rootPlaceId.get();
	}
	return -1;
}

std::string UniverseSettings::getName()
{
	return properties.get<std::string>("Name").get_value_or("");
}

void UniverseSettings::setName(const std::string& newName)
{
	properties.set("Name", newName);
}

bool UniverseSettings::currentUserHasAccess()
{
	return properties.get<bool>("CurrentUserHasEditPermissions").get_value_or(false);
}

std::string UniverseSettings::asJson()
{
	return properties.asJson();
}

EntityPropertiesForCategory::ref EntityPropertiesForCategory::loadFromWeb(
	const CategoryWebSettings& webSettings, int universeId, boost::function<void()> doneLoadingCallback)
{
	ref newSettings(new EntityPropertiesForCategory(webSettings, universeId));

	newSettings->doneLoadingCallback = doneLoadingCallback;
	newSettings->requestPage(1);

	return newSettings;
}

bool EntityPropertiesForCategory::isLoadedAndParsed()
{
	waitUntilLoaded();
	for (EntityCollection::const_iterator itr = data.begin(); itr != data.end(); ++itr)
	{
		if (!itr->isLoadedAndParsed())
		{
			return false;
		}
	}
	return true;
}

template<class IdType>
void EntityPropertiesForCategory::publishTo(ref target,
	const std::vector<std::string>& ignoreFieldsWhenCopying,
	boost::unordered_map<IdType, boost::function<IdType()> >* outputRemaps)
{
	waitUntilLoaded();
	target->waitUntilLoaded();
	RBXASSERT(&webSettings == &target->webSettings);

	boost::unordered_map<IdType, boost::function<IdType()> > idRemaps;

	std::vector<HttpFuture> danglingFutures;

	// create & update slots, marking used ids
	boost::unordered_set<IdType> usedTargetIds;
	for (EntityCollection::iterator itr = data.begin(); itr != data.end(); ++itr)
	{
		const boost::optional<std::string> name = itr->get<std::string>("Name");
		const boost::optional<IdType> id = itr->get<IdType>(webSettings.perItemIdFieldName);
		RBXASSERT(name);
		RBXASSERT(id);

		EntityProperties copyOfSettings;
		copyOfSettings.setFromEntitySettings(*itr);

		bool found = false;
		for (EntityCollection::iterator targetItr = target->data.begin();
			!found && targetItr != target->data.end(); ++targetItr)
		{
			const boost::optional<IdType> targetId = targetItr->get<IdType>(webSettings.perItemIdFieldName);
			RBXASSERT(targetId);
			if (usedTargetIds.count(targetId.get()) == 0 &&
				targetItr->get<std::string>("Name").get() == name.get())
			{
				found = true;
				usedTargetIds.insert(targetId.get());
				idRemaps[id.get()] = boost::bind(&identity<IdType>, targetId.get());
				copyOfSettings.set(webSettings.perItemIdFieldName, targetId.get());
				for (size_t i = 0; i < ignoreFieldsWhenCopying.size(); ++i)
				{
					const std::string& fieldName = ignoreFieldsWhenCopying[i];
					boost::optional<Variant> valueToKeep = targetItr->getVariant(fieldName);
					RBXASSERT(valueToKeep.is_initialized());
					if (valueToKeep.is_initialized())
					{
						copyOfSettings.set(fieldName, valueToKeep.get());
					}
				}
				HttpPostData postData(copyOfSettings.asJson(), Http::kContentTypeApplicationJson, false);
				danglingFutures.push_back(HttpAsync::post(
					formatUrl(webSettings.updateFormatString, target->universeId, targetId.get()),
					postData));
			}
		}
		if (!found)
		{
			HttpPostData postData(copyOfSettings.asJson(), Http::kContentTypeApplicationJson, false);
			idRemaps[id.get()] = extractProperty<IdType>(
				HttpAsync::post(formatUrl(webSettings.createFormatString, target->universeId), postData),
				webSettings.jsonItemIdFieldName);
		}
	}

	// clean up unused ids
	if (!webSettings.removeFormatString.empty())
	{
		for (EntityCollection::iterator itr = target->data.begin(); itr != target->data.end(); ++itr)
		{
			boost::optional<IdType> targetId = itr->get<IdType>(webSettings.perItemIdFieldName);
			RBXASSERT(targetId);
			if (usedTargetIds.count(targetId.get()) == 0)
			{
				HttpPostData postData("", Http::kContentTypeDefaultUnspecified, false);
				danglingFutures.push_back(HttpAsync::post(
					formatUrl(webSettings.removeFormatString, target->universeId, targetId.get()), postData));
			}
		}
	}

	// complete pending requests
	for (size_t i = 0; i < danglingFutures.size(); ++i)
	{
		danglingFutures[i].get();
	}

	if (outputRemaps)
	{
		outputRemaps->clear();
		outputRemaps->insert(idRemaps.begin(), idRemaps.end());
	}
}

EntityPropertiesForCategory::EntityPropertiesForCategory(const CategoryWebSettings& webSettings, int universeId)
	: webSettings(webSettings)
	, universeId(universeId)
	, doneEvent(true /*manual reset*/)
{
}

void EntityPropertiesForCategory::requestPage(int page)
{
	Http fetcher(formatUrl(webSettings.fetchPageUrlFormatString, universeId, page));
	fetcher.doNotUseCachedResponse = true;
	fetcher.get(boost::bind(&EntityPropertiesForCategory::handlePage, shared_from_this(), page, _1, _2));
}

void EntityPropertiesForCategory::handlePage(int loadedPage, std::string* json, std::exception* error)
{
	std::string errorStatus;
	shared_ptr<const ValueTable> topLevel;
	if (!LuaWebService::parseWebJSONResponseHelper(json, error, topLevel, errorStatus))
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while loading entities for universe %d: %s",
			universeId, errorStatus.c_str());
		static boost::once_flag flag = BOOST_ONCE_INIT;
		std::string label = format("load entity page failed: %s", errorStatus.c_str());
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
			GA_CATEGORY_ACTION, "Game Explorer", label.c_str(), 0, false), flag);
		return;
	}

	bool needMoreData = true;
	bool finalPage, exceededLimit;

	try
	{
		shared_ptr<const ValueArray> jsonList = topLevel->at(webSettings.jsonItemGroupFieldName)
			.cast<shared_ptr<const ValueArray> >();

		finalPage = topLevel->at("FinalPage").cast<bool>();
		exceededLimit = FInt::GameViewItemCap < (int)(data.size() + jsonList->size());
		needMoreData = !(finalPage || exceededLimit);

		for (size_t i = 0; i < jsonList->size(); ++i)
		{
			shared_ptr<const ValueTable> itemInfo = jsonList->at(i).cast<shared_ptr<const ValueTable> >();
			data.push_back(EntityProperties());
			data.back().setFromValueTable(itemInfo);
			if (webSettings.propertiesFixer)
			{
				webSettings.propertiesFixer(universeId, &data.back());
			}
		}

		// request more after adding
		if (needMoreData)
		{
			// kick off next page load
			requestPage(loadedPage + 1);
		}
	}
	catch (const std::exception& e)
	{
		std::string label = format("load entity page exception: %s", e.what());
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
			GA_CATEGORY_ACTION, "Game Explorer", label.c_str(), 0, false), flag);
		StandardOut::singleton()->print(MESSAGE_WARNING, "Error while loading data for game. Try again later.");

		doneEvent.Set();
	}

	if (!needMoreData)
	{
		if (exceededLimit && !finalPage)
		{
			StandardOut::singleton()->printf(MESSAGE_WARNING,
				"Warning: studio can currently only load %d items per category at a time. "
				"Some of your items may not have been loaded.",
				FInt::GameViewItemCap);
			static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_ACTION,
				"Game Explorer", "over item limit", 0, false), flag);
		}

		// wait for actual data
		for (EntityCollection::iterator itr = data.begin(); itr != data.end(); ++itr)
		{
			itr->waitUntilLoaded();
		}

		doneEvent.Set();
		if (doneLoadingCallback)
		{
			doneLoadingCallback();
			doneLoadingCallback = NULL;
		}
	}
}

void EntityPropertiesForCategory::waitUntilLoaded()
{
	doneEvent.Wait();
}

void AddImageDialog::runModal(QWidget* parent, bool* created, QString* newName)
{
	this->created = created;
	this->newName = newName;

	// initialize usedNames
	usedNames.clear();
	RobloxGameExplorer& rge = UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER);
	rge.getListOfImages(&usedNames);
	this->currentGameId = rge.getCurrentGameId();
	this->currentGameGroupId = rge.getCurrentGameGroupId();

	*created = false;
	
	dialog = new QDialog(parent);
	QGridLayout* layout = new QGridLayout();

	QGridLayout* inputLayout = new QGridLayout();

	// Image file
	QLabel* fileLabel = new QLabel("Image:", dialog);
	inputLayout->addWidget(fileLabel, 0, 0, Qt::AlignRight);
	fileNameLabel = new QLabel(" ", dialog);
	inputLayout->addWidget(fileNameLabel, 0, 1, Qt::AlignLeft);
	fileNameEdit = new QPushButton("Choose File", dialog);
	connect(fileNameEdit, SIGNAL(clicked()), this, SLOT(openFileSelector()));
	inputLayout->addWidget(fileNameEdit, 1, 1, Qt::AlignLeft);
	fileNameErrorMessage = new QLabel(" ", dialog);
	fileNameErrorMessage->setStyleSheet(kErrorTextStyleSheet);
	inputLayout->addWidget(fileNameErrorMessage, 2, 1, Qt::AlignLeft);

	// Name
	QLabel* nameLabel = new QLabel("Name:", dialog);
	inputLayout->addWidget(nameLabel, 3, 0, Qt::AlignRight);
	nameEdit = new QLineEdit(dialog);
	nameEdit->setFixedWidth(250);
	connect(nameEdit, SIGNAL(textEdited(const QString&)), this, SLOT(validateName()));
	inputLayout->addWidget(nameEdit, 3, 1, Qt::AlignLeft);
	nameErrorMessage = new QLabel(" ", dialog);
	nameErrorMessage->setStyleSheet(kErrorTextStyleSheet);
	inputLayout->addWidget(nameErrorMessage, 4, 1, Qt::AlignLeft);

	layout->addLayout(inputLayout, 0, 0, 1, 2);

	// General error message
	generalErrorMessage = new QLabel("", dialog);
	generalErrorMessage->setStyleSheet(kErrorTextStyleSheet);
	layout->addWidget(generalErrorMessage, 1, 0, 1, 2, Qt::AlignCenter);
	
	QPushButton* createButton = new QPushButton("Create", dialog);
	connect(createButton, SIGNAL(clicked()), this, SLOT(createButtonClicked()));
	layout->addWidget(createButton, 2, 0);

	QPushButton* cancelButton = new QPushButton("Cancel", dialog);
	connect(cancelButton, SIGNAL(clicked()), dialog, SLOT(reject()));
	layout->addWidget(cancelButton, 2, 1);

	dialog->setLayout(layout);
	dialog->exec();
}

void AddImageDialog::createImageAndNameThread()
{
	QString name = kImageAssetNamePrefix + nameEdit->text();
	QString errorMessage;
	createImageAssetAndNameThread(name, fileNameLabel->text(), currentGameId, currentGameGroupId,
		created, &errorMessage);
	if (*created)
	{
		*newName = name;
	}
	else
	{
		generalErrorMessage->setText(errorMessage);
	}
}

bool AddImageDialog::validateName()
{
	QString errorMessage;
	bool valid = validateImageName(nameEdit->text(), &usedNames, &errorMessage);

	if (valid)
	{
		nameEdit->setStyleSheet("");
		nameErrorMessage->setText(" ");
	}
	else
	{
		nameEdit->setStyleSheet(kErrorTextInputStyleSheet);
		nameErrorMessage->setText(errorMessage);
	}
	return valid;
}

bool AddImageDialog::validateImageFile()
{
	bool valid = true;
	QString errorMessage;

	QString fileName = fileNameLabel->text();
	if (fileName.trimmed().isEmpty())
	{
		valid = false;
		fileNameErrorMessage->setText("Image file is required");
	}
	else if (!QFile(fileName).exists())
	{
		valid = false;
		fileNameErrorMessage->setText("File does not exist");
	}

	if (valid)
	{
		fileNameErrorMessage->setText(" ");
	}

	return valid;
}

void AddImageDialog::openFileSelector()
{
	RobloxSettings settings;
	QString imageLastDir = settings.value(kLastImageDirectoryKey).toString();
	if (imageLastDir.isEmpty())
        imageLastDir = RobloxMainWindow::getDefaultSavePath();

	QString fileName = QDir::toNativeSeparators(
		QFileDialog::getOpenFileName(dialog, "Choose image to upload", imageLastDir,
			"Images (*.jpg *.jpeg *.png *.gif *.bmp);;All Files (*.*)"));

	if (!fileName.isEmpty())
	{
		fileNameLabel->setText(fileName);
		settings.setValue(kLastImageDirectoryKey, QFileInfo(fileName).absolutePath());

		QString availableName;
		findAvailableImageNameFromFilename(fileName, &usedNames, &availableName);
		nameEdit->setText(availableName);
	}

	validateImageFile();
}

void AddImageDialog::createButtonClicked()
{
	bool nameValid = validateName();
	bool fileValid = validateImageFile();

	if (nameValid && fileValid)
	{
		UpdateUIManager::Instance().waitForLongProcess("Creating asset name...",
			boost::bind(&AddImageDialog::createImageAndNameThread, this));

		if (*created)
		{
			UpdateUIManager::Instance().getViewWidget<RobloxGameExplorer>(eDW_GAME_EXPLORER)
				.reloadDataFromWeb();

			dialog->accept();
		}
	}
	else
	{
		// refocus the first field that is not valid
		if (!nameValid)
		{
			nameEdit->setFocus();
		}
		else
		{
			fileNameEdit->setFocus();
		}
	}
}


RobloxGameExplorer::RobloxGameExplorer(QWidget* parent)
    : QTreeView(parent)
    , currentGameId(kNoGameLoaded)
    , currentPlaceId(kNoGameLoaded)
    , currentSessionId(0)
	, enoughDataLoadedToBeAbleToOpenStartPlace(false)
	, startPlaceOpenRequested(false)
{
	if (!FFlag::GameExplorerUseV2AliasEndpoint)
	{
		CategoryWebSettings* namedAssetSettings = const_cast<CategoryWebSettings*>(&kWebSettings[ENTITY_CATEGORY_NamedAssets]);
		namedAssetSettings->createFormatString =
			"%API_PROXY%/universes/create-alias?universeId=%UNIVERSE_ID%";
		namedAssetSettings->updateFormatString =
			"%API_PROXY%/universes/update-alias?universeId=%UNIVERSE_ID%&oldName=%ITEM_ID%";
	}

	CategoryUiSettings placeSettings =
	{
		"Places",
		QtUtilities::getPixmap(":/images/ClassImages.PNG", 19),
		&generalDisplayNameGetter,
		NULL, // shouldBuildExplorerItemCallback
		boost::bind(&RobloxGameExplorer::placeDoubleClickCallback, this, _1),
		boost::bind(&RobloxGameExplorer::placeContextMenuHandler, this, _1, _2),
		boost::bind(&RobloxGameExplorer::placeGroupContextMenuHandler, this, _1),
		boost::bind(&RobloxGameExplorer::checkRowForNameUpdate, this, ENTITY_CATEGORY_Places, _1),
		CategoryUiSettings::FlatNames
	};
	uiSettings[ENTITY_CATEGORY_Places] = placeSettings;

	CategoryUiSettings devProductSettings =
	{
		"Developer Products",
		QPixmap(":/images/DeveloperProduct.png"),
		&generalDisplayNameGetter,
		NULL, // shouldBuildExplorerItemCallback
		NULL, // double click callback
		boost::bind(&RobloxGameExplorer::developerProductContextMenuHandler, this, _1, _2),
		boost::bind(&RobloxGameExplorer::developerProductGroupContextMenuHandler, this, _1),
		boost::bind(&RobloxGameExplorer::checkRowForNameUpdate, this, ENTITY_CATEGORY_DeveloperProducts, _1),
		CategoryUiSettings::FlatNames
	};
	uiSettings[ENTITY_CATEGORY_DeveloperProducts] = devProductSettings;

	CategoryUiSettings badgeSettings =
	{
		"Badges",
		QPixmap(":/16x16/images/Studio 2.0 icons/16x16/badges.png"),
		&generalDisplayNameGetter,
		NULL, // shouldBuildExplorerItemCallback
		NULL, // double click callback
		boost::bind(&RobloxGameExplorer::badgesContextMenuHandler, this, _1, _2),
		NULL,
		NULL,
		CategoryUiSettings::FlatNames
	};
	uiSettings[ENTITY_CATEGORY_Badges] = badgeSettings;

	CategoryUiSettings namedAssetSettings =
	{
		"Assets",
		QtUtilities::getPixmap(":/images/ClassImages.PNG", 52),
		&generalDisplayNameGetter,
		&namedAssetsShouldBuildExplorerItemCallback,
		boost::bind(&RobloxGameExplorer::namedAssetsDoubleClickCallback, this, _1),
		boost::bind(&RobloxGameExplorer::namedAssetsContextMenuHandler, this, _1, _2),
		boost::bind(&RobloxGameExplorer::namedAssetsGroupContextMenuHandler, this, _1),
		boost::bind(&RobloxGameExplorer::checkRowForNameUpdate, this, ENTITY_CATEGORY_NamedAssets, _1),
		CategoryUiSettings::NameAsFolderPath
	};
	uiSettings[ENTITY_CATEGORY_NamedAssets] = namedAssetSettings;

	setModel(new QStandardItemModel);
	setExpandsOnDoubleClick(false);
	setEditTriggers(QAbstractItemView::SelectedClicked);
	setContextMenuPolicy(Qt::CustomContextMenu);
    
	QStringList lst;
	lst.push_back("");
	getModel()->setHorizontalHeaderLabels(lst);
	
	gameNameEdit = new AbortableLineEdit(boost::bind(&UniverseSettings::getName, &gameSettings));
	gameNameEdit->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum));
	gameNameEdit->setEnabled(false);
	idLabel = new QLabel("");
	idLabel->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum));
	idLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
	reloadButton = new QPushButton("");
	reloadButton->setIcon(QPixmap(":/images/advT_rotateGrid_old.png"));
	reloadButton->setEnabled(false);
	QGridLayout* layout = new QGridLayout(header());
	int left, unused;
	layout->getContentsMargins(&left, &unused, &unused, &unused);
	layout->setContentsMargins(left, 0, 0, 0);
	layout->addWidget(gameNameEdit, 0, 0);
	layout->addWidget(idLabel, 0, 1);
	layout->addWidget(reloadButton, 0, 2, Qt::AlignRight);
	layout->setColumnStretch(0, 1);

	header()->hide();

	connect(gameNameEdit, SIGNAL(editingFinished        ()),
			this,      SLOT(  doneEditingUniverseName()));
	connect(reloadButton, SIGNAL(clicked()), this, SLOT(reloadDataFromWeb()));
    connect(this,         SIGNAL(doubleClicked      (QModelIndex)),
	        this,         SLOT(  doubleClickHandler (QModelIndex)));
	connect(this,         SIGNAL(customContextMenuRequested (const QPoint &)),
			this,         SLOT(  contextMenuHandler         (const QPoint &)));
	connect(getModel(),   SIGNAL(itemChanged     (QStandardItem*)),
		    this,         SLOT(itemChangedHandler(QStandardItem*)));
}

boost::optional<std::string> RobloxGameExplorer::findCachedAssetOrEmpty(const RBX::ContentId& contentId, int universeId)
{
	boost::optional<std::string> assetName;
	if (contentId.isNamedAsset())
	{
		assetName = contentId.getAssetName();
	}
	if (contentId.isConvertedNamedAsset())
	{
		assetName = contentId.getUnConvertedAssetName();
	}
	if (assetName)
	{
		QFile file(getUnpublishedAssetFilePath(assetName.get(), universeId));

		if (file.exists())
		{
			file.open(QIODevice::ReadOnly);
			QString content(file.readAll());
			return content.toStdString();
		}
	}
	
	return boost::optional<std::string>();
}

void RobloxGameExplorer::updateAsset(const RBX::ContentId& contentId, const std::string& newContents)
{
	RBXASSERT(contentId.isNamedAsset());
	RBXASSERT(currentGameId != kNoGameLoaded);
	if (contentId.isNamedAsset() && currentGameId != kNoGameLoaded)
	{
		QString filePath = getUnpublishedAssetFilePath(contentId.getAssetName(), currentGameId);
		
		QDir dir(filePath + "/..");
		QDir().mkpath(dir.absolutePath());

		QFile file(filePath);
		file.open(QIODevice::WriteOnly | QIODevice::Truncate);
		QTextStream outStream(&file);
		outStream << QString::fromStdString(newContents);
		outStream.flush();

		refreshNamedScriptIcons(currentSessionId);
	}
}

void RobloxGameExplorer::notifyIfUnpublishedChanges(int universeId)
{
	QDir unpublishedDir(getUnpublishedChangesDirectory(universeId));
	if (!unpublishedDir.exists())
	{
		return;
	}

	if (anyFilesRecursive(unpublishedDir))
	{
		QMessageBox mb;
		mb.setText("RobloxStudio is using unpublished changes for this universe. You can revert "
			"these changes to the last published version using the Game Explorer.");
		mb.setStandardButtons(QMessageBox::Ok);
		mb.exec();
	}
}

void RobloxGameExplorer::publishNamedAssetsToCurrentSlot()
{
	RBXASSERT(currentGameId != kNoGameLoaded);
	if (currentGameId == kNoGameLoaded)
	{
		return;
	}

	std::vector<boost::shared_future<void> > publishFutures;
	entitySettings[ENTITY_CATEGORY_NamedAssets]->transform<boost::shared_future<void> >(
		boost::bind(&RobloxGameExplorer::publishIfThereAreLocalModifications, this, _1), &publishFutures);

	for (auto& f : publishFutures)
	{
		try
		{
			f.get();
		}
		catch (const RBX::base_exception&)
		{
		}
	}

	QMetaObject::invokeMethod(this, "refreshNamedScriptIcons", Qt::QueuedConnection,
		Q_ARG(int, currentSessionId));
}

HttpFuture RobloxGameExplorer::publishScriptAsset(const std::string& scriptText,
	boost::optional<int> optionalAssetId, boost::optional<int> optionalGroupId)
{
	int assetId = 0;
	if (optionalAssetId)
	{
		assetId = optionalAssetId.get();
	}
	// if assetId url parameter is 0 it will create a new asset
	QString uploadUrl;
	uploadUrl = QString::fromStdString(formatUrl(
		"%DATA_BASE%/Data/Upload.ashx?type=Lua&name=Script&description=Script&ispublic=false&json=1&assetId=%ITEM_ID%",
		-1, assetId));

	// Group id url parameter is ignored when updating an existing asset, so only append url parameter
	// when creating a new asset.
	if (optionalGroupId && !optionalAssetId)
	{
		uploadUrl += QString("&groupId=%1").arg(optionalGroupId.get());
	}
	
	HttpPostData postData(scriptText, Http::kContentTypeTextPlain, false);
	return HttpAsync::post(uploadUrl.toStdString(), postData);
}

void RobloxGameExplorer::getListOfImages(std::vector<QString>* out)
{
	if (currentGameId == kNoGameLoaded)
	{
		return;
	}
	
	entitySettings[ENTITY_CATEGORY_NamedAssets]->transform<QString>(
		boost::bind(&getNameOrEmptyIfNotAssetType, ASSET_TYPE_ID_Image, _1), out);
	out->erase(std::remove_if(out->begin(), out->end(), boost::bind(&QString::isEmpty, _1)),
		out->end());
}

void RobloxGameExplorer::getListOfScripts(std::vector<QString>* out)
{
	if (currentGameId == kNoGameLoaded)
	{
		return;
	}
	entitySettings[ENTITY_CATEGORY_NamedAssets]->transform<QString>(
		boost::bind(&getNameOrEmptyIfNotAssetType, ASSET_TYPE_ID_Script, _1), out);
	out->erase(std::remove_if(out->begin(), out->end(), boost::bind(&QString::isEmpty, _1)),
		out->end());
}

void RobloxGameExplorer::getListOfAnimations(std::vector<QString>* out)
{	
	if (currentGameId == kNoGameLoaded)
		return;

	entitySettings[ENTITY_CATEGORY_NamedAssets]->transform<QString>(
		boost::bind(&getNameOrEmptyIfNotAssetType, ASSET_TYPE_ID_Animation, _1), out);
	out->erase(std::remove_if(out->begin(), out->end(), boost::bind(&QString::isEmpty, _1)),
		out->end());
}

QString RobloxGameExplorer::getAnimationsDataJson()
{
	if (currentGameId == kNoGameLoaded)
		return QString();

	std::vector<std::string> animationsData;
	entitySettings[ENTITY_CATEGORY_NamedAssets]->transform<std::string>(
		boost::bind(&getPropertiesAsJason, ASSET_TYPE_ID_Animation, _1), &animationsData);
	animationsData.erase(std::remove_if(animationsData.begin(), animationsData.end(), boost::bind(&std::string::empty, _1)), animationsData.end());

	if (animationsData.size() < 1)
		return QString();

	std::stringstream outputBuffer;
	outputBuffer << "[";
	outputBuffer << boost::algorithm::join(animationsData, ",");
	outputBuffer << "]";

	return QString(outputBuffer.str().c_str());
}

void RobloxGameExplorer::openGameFromGameId(int gameId)
{
	// close the current play doc, if there is one
	RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc();
	if (gameId != currentGameId && ideDoc)
	{
		// If we have an ide doc either it is 1) a place file / template where
		// _neither_ game id nor place id are set, or 2) an open game where both 
		// game id and place id are _both_ set.
		RBXASSERT((currentPlaceId == kNoGameLoaded) == (currentGameId == kNoGameLoaded));
		
		QMessageBox dialog(this);
		dialog.setText("Opening a new game will close the currently opened Place. Continue?");
		dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		dialog.setDefaultButton(QMessageBox::No);
		int state = dialog.exec();
		if (state == QMessageBox::No)
		{
			return;
		}
		RBXASSERT(state == QMessageBox::Yes);

		// close the current play doc
		if (!UpdateUIManager::Instance().getMainWindow().requestDocClose(ideDoc))
		{
			return;
		}
	}

	startPlaceOpenRequested = true;
	if (currentGameId == gameId)
	{
		openStartPlaceIfRequestedAndNoPlaceOpenedAndDataReady();
	}

	doOpenGameFromGameId(gameId);
}

void RobloxGameExplorer::doOpenGameFromGameId(int gameId)
{
    // Check if we are requesting to open the current game
    if (gameId == currentGameId) 
	{
		Q_EMIT namedAssetsLoaded(gameId);
		return;
	}

	notifyIfUnpublishedChanges(gameId);

	// If we are opening a different game:
	enoughDataLoadedToBeAbleToOpenStartPlace = false;
	UpdateUIManager::Instance().getMainWindow().closeConfigureWebDoc();

	if (currentGameId == kNoGameLoaded)
	{
		// clear "this place not associated with game" message
		getModel()->setRowCount(0);
	}

    // initialize view for a new universe
    currentGameId = gameId;

	UpdateUIManager::Instance().getMainWindow().publishGameAction->setEnabled(!RobloxIDEDoc::getIsCloudEditSession());

	// initialize groups if they're missing
	for (unsigned int i = 0; i < ENTITY_CATEGORY_MAX; ++i)
	{
		EntityCategory category = (EntityCategory)i;
		if (!findGroup(category))
		{
			QStandardItem* group = makeGroup(category);
			if (uiSettings[category].groupContextMenuCallback)
			{
				group->setData(qvar2(uiSettings[category].groupContextMenuCallback), ROLE_ContextMenuCallback);
			}
			if (i == ENTITY_CATEGORY_Places)
			{
				setExpanded(group->index(), true);
			}
		}
	}

	reloadDataFromWeb();
	header()->show();
	gameNameEdit->setText("Loading...");
	gameNameEdit->setEnabled(false);
	idLabel->setText(QString("Id: %1").arg(currentGameId));
	reloadButton->setEnabled(true);
}

void RobloxGameExplorer::openGameFromPlaceId(int placeId)
{
    if (placeId == currentPlaceId) return;
	if (placeId == 0)
	{
		nonGameLoaded();
		return;
	}

	currentPlaceId = placeId;
	refreshOpenedPlaceIndicator();

    Http getUniverse(formatUrl("%API_PROXY%/universes/get-universe-containing-place?placeid=%ITEM_ID%",
		-1, placeId));
	getUniverse.doNotUseCachedResponse = true;
    getUniverse.get(boost::bind(&RobloxGameExplorer::handleUniverseForPlaceResponse, QPointer<RobloxGameExplorer>(this),
        currentSessionId, placeId, _1, _2));
}

void RobloxGameExplorer::onCloseIdeDoc()
{
	currentPlaceId = kNoGameLoaded;
	if (currentGameId == kNoGameLoaded)
	{
		getModel()->setRowCount(0);
	}
	else
	{
		refreshOpenedPlaceIndicator();
	}
}

static void kickOffPublish()
{
	RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc();
	RBXASSERT(ideDoc);
	if (!ideDoc) return;

	Verb* v = ideDoc->getVerb("PublishToRobloxAsVerb");
	RBXASSERT(v);
	if (!v) return;

	v->doIt(NULL);
}

void RobloxGameExplorer::nonGameLoaded()
{
    currentGameId = kNoGameLoaded;
    currentPlaceId = kNoGameLoaded;
	// change session to invalidate any pending http requests
	currentSessionId++;

    getModel()->setRowCount(0);
	gameNameEdit->setText("");
	idLabel->setText("");
	header()->hide();
	reloadButton->setEnabled(false);

	UpdateUIManager::Instance().getMainWindow().publishGameAction->setEnabled(false);

	QStandardItem* noGameItem = new QStandardItem("Publish this place to load game data (double click here)");
	noGameItem->setData(qvar(boost::bind(&kickOffPublish)), ROLE_DoubleClickCallback);
	noGameItem->setEditable(false);
	QFont f = noGameItem->font();
	f.setItalic(true);
	noGameItem->setFont(f);
	getModel()->appendRow(noGameItem);

	Q_EMIT namedAssetsLoaded(currentGameId);
}

std::pair<int, boost::function<std::string()> > RobloxGameExplorer::buildPlaceContentPair(EntityProperties* properties)
{
	std::pair<int, boost::function<std::string()> > result;
	const CategoryWebSettings& placeWebSettings = kWebSettings[ENTITY_CATEGORY_Places];
	result.first = properties->get<int>(placeWebSettings.perItemIdFieldName).get();

	bool useLocalContent = false;
	if (result.first == currentPlaceId)
	{
		if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
		{
			if (shared_ptr<DataModel> dm = ideDoc->getEditDataModel())
			{
				// This step is best-effort. The asset publishes help, but are
				// not strictly required for publish
				try
				{
					if (FFlag::StudioCSGAssets)
						PartOperationAsset::publishAll(dm.get());
				}
				catch (const std::exception&)
				{
				}

				try
				{
					shared_ptr<std::stringstream> serialized = dm->serializeDataModel();
					result.second = boost::bind(&std::stringstream::str, serialized);
					useLocalContent = true;
				}
				catch(const std::exception& e)
				{
					StandardOut::singleton()->printf(MESSAGE_ERROR,
						"Error when saving current place: %s. Continuing to publish with last known copy of this place.",
						e.what());
				}
			}
		}
	}
	if (!useLocalContent)
	{
		HttpOptions options;
		options.setDoNotUseCachedResponse();
		result.second = boost::bind(&HttpFuture::get,
			HttpAsync::get(formatUrl(placeWebSettings.getContentFormatString, currentGameId, result.first)));
	}

	return result;
}

RobloxGameExplorer::NamedAssetCopyInfo RobloxGameExplorer::buildNamedContentInfo(
	EntityPropertiesForCategory::ref targetProperties, int sourceUniverseId,
	boost::optional<int> targetUniverseGroupId, boost::unordered_map<int, boost::function<int()> >* devProductIdRemap,
	EntityProperties* properties)
{
	NamedAssetCopyInfo result;
	result.assetName = properties->get<std::string>("Name").get();

	if (FFlag::GameExplorerUseV2AliasEndpoint)
	{
		result.aliasType = properties->get<int>("Type").get();

		if ((AliasType)result.aliasType == ALIAS_TYPE_DeveloperProduct)
		{
			result.needsTargetIdUpdate = true;
			result.newTargetIdGetter = (*devProductIdRemap)[properties->get<int>("TargetId").get()];
			return result;
		}
	}

	const int sourceAssetTypeId = properties->get<int>("AssetTypeId").get();

	// for immutable asset types just keep the source asset id
	if (sourceAssetTypeId == (int)ASSET_TYPE_ID_Image)
	{
		result.needsTargetIdUpdate = false;
		return result;
	}
	RBXASSERT(sourceAssetTypeId == (int)ASSET_TYPE_ID_Script);

	// If the asset type is mutable we always want to update asset id because
	// when the asset name properties were copied over the asset id binding gets clobbered.
	// (so either we need to restore the asset id after creating a new version, or we
	// will need to create a completely new asset for the asset name).
	result.needsTargetIdUpdate = true;

	// for mutable asset types, figure out if there is an existing asset name pointing to a
	// mutable type
	std::vector<EntityProperties*> foundProperty;

	targetProperties->transform<EntityProperties*>(
		boost::bind(&getPropertiesIfNameAndAssetTypeMatch, boost::ref(result.assetName), sourceAssetTypeId, _1),
		&foundProperty);

	boost::optional<int> foundAssetId;
	for (size_t i = 0; i < foundProperty.size(); ++i)
	{
		if (foundProperty[i])
		{
			foundAssetId = foundProperty[i]->get<int>("AssetId");
			break;
		}
	}

	std::string scriptText;
	const ContentId contentId = ContentId::fromGameAssetName(result.assetName);
	try
	{
		scriptText = LuaSourceBuffer::getScriptAssetSource(contentId, sourceUniverseId).get();
		HttpFuture scriptPublishFuture = publishScriptAsset(scriptText, foundAssetId, targetUniverseGroupId);
		if (FFlag::GameExplorerUseV2AliasEndpoint)
		{
			result.newTargetIdGetter = extractProperty<int>(scriptPublishFuture, "AssetId");
		}
		else
		{
			result.newAssetIdProperties.setFromJsonFuture(scriptPublishFuture);
		}
	}
	catch (const RBX::base_exception&)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Failed to update asset %s",
			result.assetName.c_str());
		// mark the result as not-update
		result.needsTargetIdUpdate = false;
	}

	return result;
}

void RobloxGameExplorer::publishGameThread(boost::function<int()> newUniverseFuture, bool* succeeded, int* targetUniverseId)
{
	*targetUniverseId = -1;
	*succeeded = false;
	try
	{
		*targetUniverseId = newUniverseFuture();

		UniverseSettings targetUniverseSettings;
		{
			// todo: use HttpRbxApiService
			HttpOptions getOptions;
			getOptions.setDoNotUseCachedResponse();
			targetUniverseSettings.setFromJsonFuture(HttpAsync::get(
				formatUrl("%API_PROXY%/universes/get-info?universeId=%UNIVERSE_ID%", *targetUniverseId),
				getOptions));
		}

		std::vector<std::pair<int, boost::function<std::string()> > > placeContentFutures;
		// start place content requests
		entitySettings[ENTITY_CATEGORY_Places]->transform<std::pair<int, boost::function<std::string()> > >(
			boost::bind(&RobloxGameExplorer::buildPlaceContentPair, this, _1), &placeContentFutures);

		CEvent targetPlacesLoadedEvent(true);
		EntityPropertiesForCategory::ref targetPlaces = EntityPropertiesForCategory::loadFromWeb(
			kWebSettings[ENTITY_CATEGORY_Places], *targetUniverseId,
			boost::bind(&CEvent::Set, &targetPlacesLoadedEvent));

		CEvent devProductsLoadedEvent(true);
		EntityPropertiesForCategory::ref targetDevProducts = EntityPropertiesForCategory::loadFromWeb(
			kWebSettings[ENTITY_CATEGORY_DeveloperProducts], *targetUniverseId,
			boost::bind(&CEvent::Set, &devProductsLoadedEvent));

		CEvent namedAssetsLoadedEvent(true);
		EntityPropertiesForCategory::ref targetNamedAssets;
		if (!FFlag::GameExplorerUseV2AliasEndpoint)
		{
			targetNamedAssets = EntityPropertiesForCategory::loadFromWeb(
				kWebSettings[ENTITY_CATEGORY_NamedAssets], *targetUniverseId,
				boost::bind(&CEvent::Set, &namedAssetsLoadedEvent));
		}

		devProductsLoadedEvent.Wait();
		boost::unordered_map<int, boost::function<int()> > devProductIdRemap;
		std::vector<std::string> ignoreFields;
		ignoreFields.push_back("PriceInRobux");
		ignoreFields.push_back("PriceInTickets");
		entitySettings[ENTITY_CATEGORY_DeveloperProducts]->publishTo<int>(targetDevProducts, ignoreFields, &devProductIdRemap);

		if (FFlag::GameExplorerUseV2AliasEndpoint)
		{
			// load aliases for the target universe after publishing dev products because
			// dev products modifications can change aliases
			targetNamedAssets = EntityPropertiesForCategory::loadFromWeb(
				kWebSettings[ENTITY_CATEGORY_NamedAssets], *targetUniverseId,
				boost::bind(&CEvent::Set, &namedAssetsLoadedEvent));
		}

		namedAssetsLoadedEvent.Wait();
		entitySettings[ENTITY_CATEGORY_NamedAssets]->publishTo<std::string>(
			targetNamedAssets, std::vector<std::string>(), NULL);

		targetPlacesLoadedEvent.Wait();
		boost::unordered_map<int, boost::function<int()> > placeIdRemap;
		entitySettings[ENTITY_CATEGORY_Places]->publishTo<int>(targetPlaces, std::vector<std::string>(), &placeIdRemap);
	
		std::vector<HttpFuture> danglingFutures;
		// publish place contents
		for (size_t i = 0; i < placeContentFutures.size(); ++i)
		{
			HttpPostData postData(placeContentFutures[i].second(), Http::kContentTypeDefaultUnspecified, true);
			danglingFutures.push_back(HttpAsync::post(
				formatUrl(kWebSettings[ENTITY_CATEGORY_Places].writeContentFormatString, *targetUniverseId,
					placeIdRemap[placeContentFutures[i].first]()),
				postData));
		}

		std::vector<NamedAssetCopyInfo> namedAssetCopyInfos;
		boost::optional<int> targetGroupId = getGroupIdFromPlaceEntityProperties(targetPlaces);

		entitySettings[ENTITY_CATEGORY_NamedAssets]->transform<NamedAssetCopyInfo>(
			boost::bind(&RobloxGameExplorer::buildNamedContentInfo, this,
			targetNamedAssets, currentGameId, targetGroupId, &devProductIdRemap, _1), &namedAssetCopyInfos);

		for (size_t i = 0; i < namedAssetCopyInfos.size(); ++i)
		{
			NamedAssetCopyInfo& info = namedAssetCopyInfos[i];
			if (info.needsTargetIdUpdate)
			{
				if (!FFlag::GameExplorerUseV2AliasEndpoint)
				{
					info.newAssetIdProperties.waitUntilLoaded();
					if (!info.newAssetIdProperties.isLoadedAndParsed())
					{
						throw std::runtime_error(format("Unable to update asset \"%s\", aborting publish",
							info.assetName.c_str()));
					}
				}
				EntityProperties propertiesForUpdate;
				propertiesForUpdate.set("Name", info.assetName);
				if (FFlag::GameExplorerUseV2AliasEndpoint)
				{
					propertiesForUpdate.set("Type", info.aliasType);
					propertiesForUpdate.set("TargetId", info.newTargetIdGetter());
				}
				else
				{
					propertiesForUpdate.set("AssetId", info.newAssetIdProperties.get<int>("AssetId").get());
				}

				HttpPostData postData(propertiesForUpdate.asJson(), Http::kContentTypeApplicationJson, false);
				danglingFutures.push_back(HttpAsync::post(
					formatUrl(kWebSettings[ENTITY_CATEGORY_NamedAssets].updateFormatString,
					*targetUniverseId, info.assetName),
					postData));
			}
		}

		// Apply game level setting changes, (e.g. root place)
		if (targetUniverseSettings.hasRootPlace())
		{
			bool rootSuccess;
			blockingUnMakeRoot(*targetUniverseId, targetUniverseSettings.rootPlaceId(), &rootSuccess);
		}
		if (gameSettings.hasRootPlace())
		{
			bool rootSuccess;
			blockingMakeRoot(*targetUniverseId, placeIdRemap[gameSettings.rootPlaceId()](), &rootSuccess);
		}

		for (size_t i = 0; i < danglingFutures.size(); ++i)
		{
			danglingFutures[i].get();
		}
		
		// cleanup assets in target folder
		QString unpublishedChangesDir = getUnpublishedChangesDirectory(*targetUniverseId);
		deleteFilesAndFoldersRecursive(QDir(unpublishedChangesDir));

		*succeeded = true;	
	}
	catch (const RBX::base_exception& e)
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
			GA_CATEGORY_ACTION, "Game Explorer", format("PublishTo exception: %s", e.what()).c_str(),
			0, false), flag);
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while publishing game: %s", e.what());
		*succeeded = false;
	}
}

void RobloxGameExplorer::publishGameToNewSlot()
{
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "Game Explorer", "Publish Game To New Slot");

	EntityProperties newUniverseProperties;
	newUniverseProperties.set("Name", gameSettings.getName() + " (Copy)");
	HttpFuture future = HttpAsync::post(formatUrl("%API_PROXY%/universes/create-universe", -1),
		HttpPostData(newUniverseProperties.asJson(), Http::kContentTypeApplicationJson, false));

	publishInternal(extractProperty<int>(future, "UniverseId"));
}

void RobloxGameExplorer::publishGameToNewGroupSlot(int groupId)
{
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "Game Explorer", "Publish Game To New Group Slot");

	EntityProperties newUniverseProperties;
	newUniverseProperties.set("Name", gameSettings.getName() + " (Copy)");
	HttpFuture future = HttpAsync::post(formatUrl("%API_PROXY%/universes/create-universe?groupId=%ITEM_ID%", -1, groupId),
		HttpPostData(newUniverseProperties.asJson(), Http::kContentTypeApplicationJson, false));

	publishInternal(extractProperty<int>(future, "UniverseId"));
}

void RobloxGameExplorer::publishGameToExistingSlot(int gameId)
{
	RobloxGoogleAnalytics::trackEvent(GA_CATEGORY_ACTION, "Game Explorer",
		gameId == currentGameId ? "Publish Game To Current Slot" : "Publish Game To Other Existing Slot");

	publishInternal(boost::bind(&identity<int>, gameId));
}

void RobloxGameExplorer::handleUniverseForPlaceResponse(QPointer<RobloxGameExplorer> explorer,
    int originatingSessionId, int placeId, std::string* result, std::exception* exception)
{
    if (explorer.isNull()) return;
    if (explorer->currentSessionId != originatingSessionId) return;

    Variant var;
	if (result && WebParser::parseJSONObject(*result, var) &&
		var.isType<shared_ptr<const ValueTable> >())
    {
        shared_ptr<const ValueTable> table = var.cast<shared_ptr<const ValueTable> >();
		ValueTable::const_iterator itr = table->find("UniverseId");
		if (itr != table->end() && !itr->second.isVoid() && itr->second.isNumber())
        {
			int gameId = itr->second.cast<int>();
            QMetaObject::invokeMethod(explorer, "doOpenGameFromGameId", Qt::QueuedConnection,
                Q_ARG(int, gameId));
        }
    }
}

void RobloxGameExplorer::doneLoadingDataForCategory(QPointer<RobloxGameExplorer> explorer, int originatingSessionId, EntityCategory category)
{
	if (explorer.isNull()) return;
	if (explorer->currentSessionId != originatingSessionId) return;
	
	// wait for settings to load in this thread (the http thread), not the main thread!
	explorer->gameSettings.waitUntilLoaded();
	if (!explorer->entitySettings[category]->isLoadedAndParsed() ||
		!explorer->gameSettings.isLoadedAndParsed())
	{
		QMetaObject::invokeMethod(explorer, "failedToLoadSettings",
			Qt::QueuedConnection, Q_ARG(int, originatingSessionId));
		return;
	}

	QMetaObject::invokeMethod(explorer, "populateWithLoadedData",
		Qt::QueuedConnection, Q_ARG(int, originatingSessionId), Q_ARG(int, category));

	if (category == ENTITY_CATEGORY_Places)
	{
		QMetaObject::invokeMethod(explorer, "afterPlacesLoadedFinished",
			Qt::QueuedConnection, Q_ARG(int, originatingSessionId));
	}

	if (category == ENTITY_CATEGORY_NamedAssets)
	{
		QMetaObject::invokeMethod(explorer, "afterNamedAssetsFinished",
			Qt::QueuedConnection, Q_ARG(int, originatingSessionId));
	}

	if (category == ENTITY_CATEGORY_Badges)
	{
		QMetaObject::invokeMethod(explorer, "afterBadgesFinished",
			Qt::QueuedConnection, Q_ARG(int, originatingSessionId));
	}
}

QStandardItemModel* RobloxGameExplorer::getModel()
{
    return dynamic_cast<QStandardItemModel*>(model());
}

QStandardItem* RobloxGameExplorer::findGroup(EntityCategory category)
{
    QModelIndexList resultList = getModel()->match(getModel()->index(0,0), ROLE_CategoryIfGroupRoot,
		category, 1, Qt::MatchExactly);
    RBXASSERT(resultList.size() <= 1);
    return resultList.empty() ? NULL : getModel()->itemFromIndex(*resultList.begin());
}

QStandardItem* RobloxGameExplorer::makeGroup(EntityCategory category)
{
	CategoryUiSettings& settings = uiSettings[category];
	QStandardItem* group = new QStandardItem(settings.groupDisplayName);
	group->setEditable(false);
	group->setData(settings.icon, Qt::DecorationRole);
	group->setData(category, ROLE_CategoryIfGroupRoot);
    getModel()->appendRow(group);
	return group;
}

void RobloxGameExplorer::openAndFocusConfigureDoc(const std::string& url)
{
	RobloxWebDoc* webDoc = UpdateUIManager::Instance().getMainWindow().getConfigureWebDoc();
	webDoc->open(&UpdateUIManager::Instance().getMainWindow(), QString::fromStdString(url));
}

void RobloxGameExplorer::setGroupLoadingStatus(EntityCategory category)
{
	QStandardItem* group = findGroup(category);
	group->setRowCount(0);
	group->setText(uiSettings[category].groupDisplayName + " (Loading...)");
}

void RobloxGameExplorer::placeDoubleClickCallback(EntityProperties* placeInfo)
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
	boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent, GA_CATEGORY_ACTION,
		"Game Explorer", "Open Place", 0, false), flag);
	openPlace(getEntityId(placeInfo, ENTITY_CATEGORY_Places).toInt());
}

void RobloxGameExplorer::namedAssetsDoubleClickCallback(EntityProperties* properties)
{
	boost::optional<int> assetTypeId = properties->get<int>("AssetTypeId");
	if (!assetTypeId)
	{
		return;
	}

	if (assetTypeId == (int)ASSET_TYPE_ID_Image)
	{
		insertNamedImage(properties);
	}
	else if (assetTypeId == (int)ASSET_TYPE_ID_Script)
	{
		RobloxDocManager::Instance().openDoc(LuaSourceBuffer::fromContentId(
			ContentId::fromGameAssetName(properties->get<std::string>("Name").get())));
	}
}

void RobloxGameExplorer::openPlace(int placeId)
{
	if (placeId != currentPlaceId)
	{
		UpdateUIManager::Instance().getMainWindow().handleFileOpen("", IRobloxDoc::IDE, QString::fromStdString(
			formatUrl(
				"%WEB_BASE%/Game/edit.ashx?PlaceID=%ITEM_ID%&upload=%ITEM_ID%&testmode=false&universeId=%UNIVERSE_ID%",
				currentGameId, placeId)));
	}
	else
	{
		// double-clicking currently opened place; bring ide doc to focus
		if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
		{
			RobloxDocManager::Instance().setCurrentDoc(ideDoc);
		}
	}
}

void RobloxGameExplorer::refreshOpenedPlaceIndicator()
{
	if (QStandardItem* placeGroup = findGroup(ENTITY_CATEGORY_Places))
    {
        for (int i = 0; i < placeGroup->rowCount(); ++i)
        {
            updateItemForOpenedPlaceState(placeGroup->child(i));
        }
    }
}

void RobloxGameExplorer::refreshNamedScriptIcons(int originatingSessionId)
{
	if (originatingSessionId != currentSessionId)
	{
		return;
	}

	QPixmap scriptPixmap(":/RibbonBar/images/RibbonIcons/Insert/Script.png");
	QIcon scriptIcon(scriptPixmap.scaled(QSize(16, 16)));
	QIcon modifiedScriptIcon(":/images/script_source_setting.png");

	std::vector<QStandardItem*> itemsToExplore;
	itemsToExplore.push_back(findGroup(ENTITY_CATEGORY_NamedAssets));
	while (!itemsToExplore.empty())
	{
		QStandardItem* item = itemsToExplore.back();
		itemsToExplore.pop_back();
		if (!item) continue;

		if (EntityProperties* properties = item->data(ROLE_EntityProperties).value<EntityProperties*>())
		{
			if (boost::optional<int> assetTypeId = properties->get<int>("AssetTypeId"))
			{
				if (assetTypeId == (int)ASSET_TYPE_ID_Script)
				{
					item->setIcon(scriptIcon);
					if (namedAssetHasUnpublishedChanges(properties->get<std::string>("Name").get()))
					{
						item->setIcon(modifiedScriptIcon);
					}
				}
			}
		}

		for (int i = 0; i < item->rowCount(); ++i)
		{
			itemsToExplore.push_back(item->child(i));
		}
	}
}

void RobloxGameExplorer::openStartPlaceIfRequestedAndNoPlaceOpenedAndDataReady()
{
	if (startPlaceOpenRequested && enoughDataLoadedToBeAbleToOpenStartPlace)
	{
		// mark request fulfilled if we met the criteria
		startPlaceOpenRequested = false;
		// but only open an ide doc with start place if no ide doc currently open
		if (currentPlaceId == kNoGameLoaded && gameSettings.hasRootPlace())
		{
			openPlace(gameSettings.rootPlaceId());
		}
	}
}

void RobloxGameExplorer::updateItemForOpenedPlaceState(QStandardItem* placeItem)
{
	if (getEntityId(placeItem, ENTITY_CATEGORY_Places) == currentPlaceId)
    {
        QFont f = placeItem->font();
        f.setFamily("Helvetica");
        f.setBold(true);
        placeItem->setFont(f);
    }
    else
    {
        QFont f = placeItem->font();
        f.setFamily("Source Sans Pro");
        f.setBold(false);
        placeItem->setFont(f);
    }
}

QStandardItem* RobloxGameExplorer::buildItem(const CategoryUiSettings& uiSettings,
	const CategoryWebSettings& webSettings, QStandardItem* root, EntityProperties* settings)
{
	if (uiSettings.shouldBuildExplorerItemCallback && !uiSettings.shouldBuildExplorerItemCallback(settings))
	{
		return NULL;
	}

	QString name = uiSettings.displayNameGetter(settings);
	QStandardItem* itemParent = root;

	if (uiSettings.nameMode == CategoryUiSettings::NameAsFolderPath)
	{
		// break on path separators
		QStringList path = name.split(QChar('/'));
		for (int pathPieceIndex = 0; pathPieceIndex < path.size() - 1; ++pathPieceIndex)
		{
			QString& pathPiece = path[pathPieceIndex];
			// try to find path item as an item under current parent
			bool found = false;
			for (int parentRow = 0; parentRow < itemParent->rowCount() && !found; ++parentRow)
			{
				QStandardItem* potentialParent = itemParent->child(parentRow);
				if (potentialParent->text() == pathPiece)
				{
					itemParent = potentialParent;
					found = true;
				}
			}
			if (!found)
			{
				QStandardItem* newParent = new QStandardItem(pathPiece);
				newParent->setEditable(false);
				newParent->setData(QtUtilities::getPixmap(":/images/ClassImages.PNG", 77), Qt::DecorationRole);
				itemParent->appendRow(newParent);
				itemParent = newParent;
			}
		}

		name = path.back();
	}

	QStandardItem* item = new QStandardItem(name);
	if (uiSettings.doubleClickCallback)
	{
		item->setData(qvar(boost::bind(uiSettings.doubleClickCallback, settings)), ROLE_DoubleClickCallback);
	}
	item->setData(uiSettings.icon, Qt::DecorationRole);
	item->setData(qvar2(boost::bind(uiSettings.contextMenuCallback, _1, settings)), ROLE_ContextMenuCallback);
	item->setData(qvar(settings), ROLE_EntityProperties);
	if (uiSettings.nameChangedCallback)
	{
		item->setData(qvar(boost::bind(uiSettings.nameChangedCallback, item)), ROLE_NameChangedCallback);
	}
	item->setEditable(true);
	
	itemParent->appendRow(item);

	return item;
}

void RobloxGameExplorer::publishInternal(boost::function<int()> newUniverseFuture)
{
	UpdateUIManager::Instance().getMainWindow().closePublishGameWindow();
	RBXASSERT(currentGameId != kNoGameLoaded);
	if (currentGameId == kNoGameLoaded) return;

	reloadDataFromWeb();

	bool publishSucceeded;
	int targetGameId;
	UpdateUIManager::Instance().waitForLongProcess("Publishing Game...",
		boost::bind(&RobloxGameExplorer::publishGameThread, this,
		newUniverseFuture, &publishSucceeded, &targetGameId));

	if (publishSucceeded)
	{
		refreshNamedScriptIcons(currentSessionId);

		if (targetGameId != currentGameId)
		{
			QMessageBox dialog(this);
			dialog.setText("Successfully published game. Do you want to open the published game now?");
			dialog.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
			dialog.setDefaultButton(QMessageBox::No);
			int state = dialog.exec();
			if (state == QMessageBox::Yes)
			{
				openGameFromGameId(targetGameId);
			}
		}
		else
		{
			QMessageBox messageBox(this);
			messageBox.setText("Successfully published game.");
			messageBox.setStandardButtons(QMessageBox::Ok);
			messageBox.exec();
		}
	}
	else
	{
		QMessageBox messageBox(this);
		messageBox.setText("Error while publishing. See Output window for details.");
		messageBox.setStandardButtons(QMessageBox::Ok);
		messageBox.exec();
	}
}

void RobloxGameExplorer::failedToLoadSettings(int originatingSessionId)
{
	if (originatingSessionId != currentSessionId) return;

	nonGameLoaded();
	QMessageBox warning(this);
	warning.setText("Failed to load game settings, some studio features have been disabled.");
	warning.setStandardButtons(QMessageBox::Ok);
	warning.exec();
}

void RobloxGameExplorer::populateWithLoadedData(int originatingSessionId, int cat)
{
	EntityCategory category = (EntityCategory)cat;
	QStandardItem* group = findGroup(category);
	if (currentSessionId != originatingSessionId || group == NULL) return;

	const CategoryUiSettings& settings = uiSettings[category];

	std::vector<QStandardItem*> items;
	entitySettings[category]->transform(boost::function<QStandardItem*(EntityProperties*)>(
		boost::bind(&RobloxGameExplorer::buildItem, boost::ref(settings), boost::ref(kWebSettings[cat]), group, _1)),
		&items);

	// get rid of "Loading..."
	group->setText(settings.groupDisplayName);
}

void RobloxGameExplorer::afterPlacesLoadedFinished(int originatingSessionId)
{
	if (originatingSessionId != currentSessionId) return;
	
	refreshOpenedPlaceIndicator();
	enoughDataLoadedToBeAbleToOpenStartPlace = true;

	if (gameSettings.hasRootPlace())
	{
		if (QStandardItem* placeGroup = findGroup(ENTITY_CATEGORY_Places))
		{
			for (int i = 0; i < placeGroup->rowCount(); ++i)
			{
				QStandardItem* placeItem = placeGroup->child(i);
				if (getEntityId(placeItem, ENTITY_CATEGORY_Places) == gameSettings.rootPlaceId())
				{
					placeItem->setData(QtUtilities::getPixmap(":/images/ClassImages.PNG", 25), Qt::DecorationRole);
				}
			}
		}
	}

	QString baseName = QString::fromStdString(gameSettings.getName());
	gameNameEdit->setText(baseName);
	gameNameEdit->setEnabled(true);

	currentGameGroupId = getGroupIdFromPlaceEntityProperties(entitySettings[ENTITY_CATEGORY_Places]);

	openStartPlaceIfRequestedAndNoPlaceOpenedAndDataReady();
}

static void invokeThumbnailLoadedForImage(RobloxGameExplorer* explorer, int originatingSessionId, QModelIndex item, HttpFuture future)
{
	QVariant var;
	var.setValue<HttpFuture>(future);
	QMetaObject::invokeMethod(explorer, "thumbnailLoadedForImage", Qt::QueuedConnection,
		Q_ARG(int, originatingSessionId), Q_ARG(QModelIndex, item), Q_ARG(QVariant, var));
}

void RobloxGameExplorer::afterBadgesFinished(int originatingSessionId)
{
	if (originatingSessionId != currentSessionId) return;

	if (QStandardItem* badgesGroup = findGroup(ENTITY_CATEGORY_Badges))
	{
		for (int i = 0; i < badgesGroup->rowCount(); ++i)
		{
			const CategoryUiSettings& badgesUiSettings = uiSettings[ENTITY_CATEGORY_Badges];
			QStandardItem* placesItem = badgesGroup->child(i);
			placesItem->setEditable(false);

			if (EntityProperties* properties = placesItem->data(ROLE_EntityProperties).value<EntityProperties*>())
			{
				// update icon and context menu handler for placeItem under Badge
				placesItem->setData(uiSettings[ENTITY_CATEGORY_Places].icon, Qt::DecorationRole);
				placesItem->setData(qvar2(boost::bind(&RobloxGameExplorer::badgesPlaceContextMenuHandler, this, _1, properties)), ROLE_ContextMenuCallback);

				// create badge items for the place
				EntityProperties::EntityCollection& children = properties->getChildren();
				for (size_t i = 0; i < children.size(); ++i)
				{
					EntityProperties& childProp = children[i];
					QStandardItem* newItem = buildItem(badgesUiSettings, badgesWebSettings, placesItem, &childProp);
					if (!newItem)
						continue;

					newItem->setEditable(false);
					// update badge icon with the image url for the badge
					boost::optional<shared_ptr<const Reflection::ValueTable> > thumbnailDetails = childProp.get<shared_ptr<const Reflection::ValueTable> >("Thumbnail");
					if (thumbnailDetails.is_initialized() && thumbnailDetails.get())
					{
						Reflection::ValueTable::const_iterator iter = thumbnailDetails.get()->find("Url");
						if (iter != thumbnailDetails.get()->end())
						{
							std::string thumbnailUrl = iter->second.get<std::string>();
							if (!thumbnailUrl.empty())
							{
								HttpOptions options;
								options.setExternal(true);
								HttpAsync::get(thumbnailUrl, options).then(boost::bind(&invokeThumbnailLoadedForImage, this,
											                                    currentSessionId, newItem->index(), _1));
							}
						}
					}
				}
			}
		}
	}
}

void RobloxGameExplorer::afterNamedAssetsFinished(int originatingSessionId)
{
	if (originatingSessionId != currentSessionId) return;

	if (QStandardItem* namedAssetsGroup = findGroup(ENTITY_CATEGORY_NamedAssets))
	{
		QStringList imagesToReload;
		afterNamedAssetsFinishedRecursive(namedAssetsGroup, imagesToReload);
		if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
		{
			ideDoc->forceReloadImages(imagesToReload);
		}
		refreshNamedScriptIcons(currentSessionId);

		Q_EMIT namedAssetsLoaded(currentGameId);
	}
}

void RobloxGameExplorer::afterNamedAssetsFinishedRecursive(QStandardItem* root, QStringList& imagesToReload)
{
	for (int i = 0; i < root->rowCount(); ++i)
	{
		QStandardItem* namedImageItem = root->child(i);
		if (EntityProperties* properties = namedImageItem->data(ROLE_EntityProperties).value<EntityProperties*>())
		{
			boost::optional<int> assetTypeId = properties->get<int>("AssetTypeId");
			if (!assetTypeId.is_initialized() || assetTypeId.get() != (int)ASSET_TYPE_ID_Image)
			{
				continue;
			}

			boost::optional<int> assetId = properties->get<int>("AssetId");
			if (assetId.is_initialized())
			{
				HttpAsync::get(formatUrl("%WEB_BASE%/asset/?id=%ITEM_ID%", -1, assetId.get()))
					.then(boost::bind(&invokeThumbnailLoadedForImage, this,
						currentSessionId, namedImageItem->index(), _1));
			}

			std::string name = properties->get<std::string>("Name").get();
			imagesToReload.push_back(QString::fromStdString(ContentId::fromGameAssetName(name).toString()));
		}

		afterNamedAssetsFinishedRecursive(namedImageItem, imagesToReload);
	}
}

void RobloxGameExplorer::thumbnailLoadedForImage(int originatingSessionId, QModelIndex index, QVariant future)
{
	if (originatingSessionId != currentSessionId) return;

	QStandardItem* item = getModel()->itemFromIndex(index);
	RBXASSERT(item);
	if (!item) return;

	std::string imageContent;
	try
	{
		imageContent = future.value<HttpFuture>().get();
	}
	catch (const RBX::base_exception&)
	{
		// This should indicate to the user that there was an issue with loading the
		// thumbnail instead of silently returning.
		return;
	}

	QImage image;
	if (image.loadFromData((uchar*)imageContent.c_str(), imageContent.size()))
	{
		image = image.scaled(QSize(16,16));
		item->setData(image, Qt::DecorationRole);
	}
}

static void addNewPlaceHelperThread(int universeId, bool* succeeded)
{
	// get default settings
	std::string defaultSettingsUrl = formatUrl(
		"%WEB_BASE%/ide/places/defaultsettings?universeId=%UNIVERSE_ID%&templateId=%ITEM_ID%",
		universeId, FInt::NewGameTemplateId);
	Http getDefaults(defaultSettingsUrl);
	getDefaults.doNotUseCachedResponse = true;
	std::string defaultSettings;
	try
	{
		getDefaults.get(defaultSettings);
	}
	catch (const RBX::base_exception& e)
	{
		*succeeded = false;
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while adding: %s", e.what());
		std::string gaMessage = format("Error while getting default place value for add: %s", e.what());
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
			GA_CATEGORY_ACTION, "Game Explorer", gaMessage.c_str(), 0, false), flag);
		return;
	}

	// create from default settings
	std::string createUrl = formatUrl(kWebSettings[ENTITY_CATEGORY_Places].createFormatString, universeId);
	Http create(createUrl);
	std::istringstream defaultSettingsStream(defaultSettings);
	std::string postResponse;
	try
	{
		create.post(defaultSettingsStream, Http::kContentTypeApplicationJson, false, postResponse);
	}
	catch (const RBX::base_exception& e)
	{
		*succeeded = false;
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while adding: %s", e.what());
		std::string gaMessage = format("Error while creating place from getting default value: %s", e.what());
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
			GA_CATEGORY_ACTION, "Game Explorer", gaMessage.c_str(), 0, false), flag);
		return;
	}

	*succeeded = true;
}

void RobloxGameExplorer::addNewPlace()
{
	bool succeeded = false;
	UpdateUIManager::Instance().waitForLongProcess(QString("Adding Place..."), boost::bind(&addNewPlaceHelperThread,
		currentGameId, &succeeded));

	reloadDataFromWeb();

	if (!succeeded)
	{
		QMessageBox messageBox(this);
		messageBox.setText("Error while adding place. See Output window for details.");
		messageBox.setStandardButtons(QMessageBox::Ok);
		messageBox.exec();
	}
}

static void blockingRemovePlace(int gameId, int placeId, bool isRoot, bool* reload)
{
	if (isRoot)
	{
		blockingUnMakeRoot(gameId, placeId, reload);
		if (!*reload)
		{
			return;
		}
	}

	try
	{
        HttpPostData d("", Http::kContentTypeDefaultUnspecified, false);
        HttpAsync::post(formatUrl(kWebSettings[ENTITY_CATEGORY_Places].removeFormatString,
			gameId, placeId), d).get();
	}
	catch (const RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while removing place: %s", e.what());
	}
	*reload = true;
}

void RobloxGameExplorer::placeContextMenuHandler(const QPoint& point, EntityProperties* properties)
{
	QMenu menu(this);

	int placeId = getEntityId(properties, ENTITY_CATEGORY_Places).toInt();

	QAction* remove = menu.addAction("Remove From Game");
	remove->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/delete_x_16_h.png"));

	QAction* rename = menu.addAction("Rename");

	bool isCurrentRoot = gameSettings.hasRootPlace() && gameSettings.rootPlaceId() == placeId;
	QAction* makeRoot = menu.addAction("Mark as Start Place");
	makeRoot->setIcon(QtUtilities::getPixmap(":/images/ClassImages.PNG", 25));
	makeRoot->setEnabled(!isCurrentRoot);

	QAction* copyId = NULL;
	if (FFlag::GameExplorerCopyId)
	{
		copyId = menu.addAction("Copy ID to Clipboard");
	}

	QAction* result = menu.exec(point);
	bool reload = false;
	if (result == remove)
	{
		static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
		GA_CATEGORY_ACTION, "Game Explorer", "Removing place from universe", 0, false), flag);

		bool placeNotCurrentlyOpen = true;
		if (currentPlaceId == placeId)
		{
			// try to close the current play doc
			if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
			{
				placeNotCurrentlyOpen = UpdateUIManager::Instance().getMainWindow().requestDocClose(ideDoc);
			}
		}

		if (placeNotCurrentlyOpen)
		{
			UpdateUIManager::Instance().waitForLongProcess(
				QString("Removing place \"%1\" from game").arg(QString::fromStdString(properties->get<std::string>("Name").get())),
				boost::bind(&blockingRemovePlace, currentGameId, placeId, isCurrentRoot, &reload));
		}
	}
	else if (result == rename)
	{
		reload = false;
		handleRename(point);
	}
	else if (result == makeRoot)
	{	
		static boost::once_flag flag = BOOST_ONCE_INIT;
			boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
		GA_CATEGORY_ACTION, "Game Explorer", "Setting root place", 0, false), flag);

		UpdateUIManager::Instance().waitForLongProcess(
			QString("Changing start place for game"),
			boost::bind(&blockingMakeRoot, currentGameId, placeId, &reload));
	}
	else if (FFlag::GameExplorerCopyId && copyId && result == copyId)
	{
		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText(QString::number(placeId));
	}

	if (reload)
	{
		reloadDataFromWeb();
	}
}

void RobloxGameExplorer::developerProductContextMenuHandler(const QPoint& point, EntityProperties* properties)
{
	QMenu menu(this);

	QAction* rename = menu.addAction("Rename");

	QAction* copyId = NULL;
	if (FFlag::GameExplorerCopyId)
	{
		copyId = menu.addAction("Copy ID to Clipboard");
	}

	QAction* result = menu.exec(point);
	if (result == rename)
	{
		handleRename(point);
	}
	else if (FFlag::GameExplorerCopyId && copyId && result == copyId)
	{
		QString productId = getEntityId(properties, ENTITY_CATEGORY_DeveloperProducts).toString();
		
		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText(productId);
	}
}

void RobloxGameExplorer::badgesContextMenuHandler(const QPoint& point, EntityProperties* properties)
{
	boost::optional<int> var = properties->get<int>("BadgeAssetId").get();
	if (!var.is_initialized())
		return;

	int badgeId = var.get();
	RBXASSERT(badgeId > 0);

	QMenu menu(this);
	QAction* configureAction = menu.addAction(tr("Configure"));
	QAction* copyIdAction = NULL;
	if (FFlag::GameExplorerCopyId)
		copyIdAction = menu.addAction("Copy ID to Clipboard");

	QAction* result = menu.exec(point);
	if (result == configureAction)
	{
		openAndFocusConfigureDoc(
			formatUrl("%WEB_BASE%/My/Item.aspx?ID=%ITEM_ID%", currentGameId, badgeId));
	}
	else if (FFlag::GameExplorerCopyId && copyIdAction && result == copyIdAction)
	{
		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText(QString::number(badgeId));
	}
}

void RobloxGameExplorer::badgesPlaceContextMenuHandler(const QPoint& point, EntityProperties* properties)
{
	boost::optional<int> var = properties->get<int>("PlaceId");
	if (!var.is_initialized())
		return;
		
	int placeId = var.get();
	RBXASSERT(placeId > 0);

	QMenu menu(this);
	QAction* createBadgeAction = menu.addAction(tr("Create Badge"));

	QAction* result = menu.exec(point);
	if (result == createBadgeAction)
	{
		if (FFlag::UseNewBadgesCreatePage)
		{
			std::string url = formatUrl("%WEB_BASE%/build/upload?AssetTypeId=21&TargetPlaceId=%ITEM_ID%", currentGameId, placeId);
			if (currentGameGroupId)
			{
				url += format("&groupId=%d", currentGameGroupId.get());
			}
			openAndFocusConfigureDoc(url);
		}
		else
		{
			openAndFocusConfigureDoc(
				formatUrl("%WEB_BASE%/My/NewBadge.aspx?targetID=%ITEM_ID%", currentGameId, placeId));
		}
	}
}

static void blockingRemoveNamedAsset(int universeId, const std::string& name, bool* success)
{
	try
	{
		HttpPostData d("", Http::kContentTypeDefaultUnspecified, false);
		HttpAsync::post(formatUrl(kWebSettings[ENTITY_CATEGORY_NamedAssets].removeFormatString,
			universeId, name), d).get();
	}
	catch (const RBX::base_exception& e)
	{
		StandardOut::singleton()->printf(MESSAGE_ERROR, "Error while removing name: %s",
			e.what());
		*success = false;
		return;
	}

	QFile(getUnpublishedAssetFilePath(name, universeId)).remove();

	*success = true;
}

void RobloxGameExplorer::namedAssetsContextMenuHandler(const QPoint& point, EntityProperties* properties)
{
	QMenu menu(this);

	QAction* remove = menu.addAction("Remove From Game");
	remove->setIcon(QIcon(":/16x16/images/Studio 2.0 icons/16x16/delete_x_16_h.png"));

	QAction* rename = menu.addAction("Rename");

	QAction* copyPath = NULL;

	QAction* insert = NULL;
	QAction* insertAsScript = NULL;
	QAction* insertAsLocalScript = NULL;
	QAction* insertAsModuleScript = NULL;
	QAction* publishScript = NULL;
	QAction* revertToLastPublished = NULL;
	bool isImage = false;
	bool isScript = false;
	bool hasOpenDataModel = RobloxDocManager::Instance().getPlayDoc() != NULL; 
	isImage = properties->get<int>("AssetTypeId") == (int)ASSET_TYPE_ID_Image;
	isScript = properties->get<int>("AssetTypeId") == (int)ASSET_TYPE_ID_Script;
	if (isImage)
	{
		insert = menu.addAction("Insert");
		insert->setEnabled(hasOpenDataModel);
		if (FFlag::GameExplorerCopyPath)
		{
			copyPath = menu.addAction("Copy Path to Clipboard");
		}
	}
	else if (isScript)
	{
		insertAsScript = menu.addAction("Insert as Script");
		insertAsScript->setEnabled(hasOpenDataModel);

		insertAsLocalScript = menu.addAction("Insert as LocalScript");
		insertAsLocalScript->setEnabled(hasOpenDataModel);

		insertAsModuleScript = menu.addAction("Insert as ModuleScript");
		insertAsModuleScript->setEnabled(hasOpenDataModel);

		menu.addSeparator();

		std::string name = properties->get<std::string>("Name").get();
		bool hasUnpublishedChange = namedAssetHasUnpublishedChanges(name);
		revertToLastPublished = menu.addAction("Revert to Last Published Version");
		revertToLastPublished->setEnabled(hasUnpublishedChange);

		publishScript = menu.addAction("Publish This LinkedSource");
		publishScript->setEnabled(hasUnpublishedChange);
	}

	bool needReload = false;
	QAction* result = menu.exec(point);
	if (result == remove)
	{
		QMessageBox mb;
		mb.setText("Assets are referenced by name, so any references to this name will fail to "
					"load. Are you sure you want to remove this asset?");
		mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		if (mb.exec() != QMessageBox::Yes)
		{
			return;
		}
	
		static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
		GA_CATEGORY_ACTION, "Game Explorer", "Removing name from universe", 0, false), flag);

		std::string name = properties->get<std::string>("Name").get();
		UpdateUIManager::Instance().waitForLongProcess(QString("Removing name"),
			boost::bind(&blockingRemoveNamedAsset, currentGameId, name, &needReload));

		if (properties->get<int>("AssetTypeId") == (int)ASSET_TYPE_ID_Script)
		{
			RobloxDocManager::Instance().closeDoc(
				LuaSourceBuffer::fromContentId(ContentId::fromGameAssetName(name)));
		}
	}
	else if (result == rename)
	{
		handleRename(point);
	}
	else if (insert && result == insert)
	{
		insertNamedImage(properties);
	}
	else if (insertAsScript && result == insertAsScript)
	{
		insertNamedScript(properties, Creatable<Instance>::create<Script>());
	}
	else if (insertAsLocalScript && result == insertAsLocalScript)
	{
		insertNamedScript(properties, Creatable<Instance>::create<LocalScript>());
	}
	else if (insertAsModuleScript && result == insertAsModuleScript)
	{
		insertNamedScript(properties, Creatable<Instance>::create<ModuleScript>());
	}
	else if (revertToLastPublished && result == revertToLastPublished)
	{
		QMessageBox mb;
		mb.setText("This will destroy all local changes you have made. Are you sure?");
		mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		if (mb.exec() == QMessageBox::Yes)
		{
			std::string name = properties->get<std::string>("Name").get();
			QFile(getUnpublishedAssetFilePath(name, currentGameId)).remove();
			refreshNamedScriptIcons(currentSessionId);
			if (RobloxScriptDoc* scriptDoc = RobloxDocManager::Instance().findOpenScriptDoc(
				LuaSourceBuffer::fromContentId(ContentId::fromGameAssetName(name))))
			{
				scriptDoc->refreshEditorFromSourceBuffer();
			}

			Q_EMIT namedAssetModified(currentGameId, QString::fromStdString(name));
		}
	}
	else if (publishScript && result == publishScript)
	{
		QMessageBox mb;
		mb.setText("Are you sure you want to publish your changes?");
		mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
		if (mb.exec() == QMessageBox::Yes)
		{
			boost::shared_future<void> publishFuture = publishIfThereAreLocalModifications(properties);
			UpdateUIManager::Instance().waitForLongProcess("Publishing LinkedSource...",
				boost::bind(&boost::shared_future<void>::get, &publishFuture));
			refreshNamedScriptIcons(currentSessionId);
		}
	}
	else if (FFlag::GameExplorerCopyPath && copyPath && result == copyPath)
	{
		std::string path = ContentId::fromGameAssetName(properties->get<std::string>("Name").get()).toString();

		QClipboard *clipboard = QApplication::clipboard();
		clipboard->setText(QString(path.c_str()));
	}

	if (needReload)
	{
		reloadDataFromWeb();
	}
}

void RobloxGameExplorer::placeGroupContextMenuHandler(const QPoint& point)
{
	QMenu menu(this);

	QAction* add = menu.addAction("Add Empty Place");

	QAction* configure = menu.addAction("Change Place List");

	QAction* result = menu.exec(point);
	if (result == add)
	{
		addNewPlace();
	}
	else if (result == configure)
	{
		openAndFocusConfigureDoc(
			formatUrl("%WEB_BASE%/universes/configure?id=%UNIVERSE_ID%#/#places", currentGameId));
	}
}

void RobloxGameExplorer::developerProductGroupContextMenuHandler(const QPoint& point)
{
	QMenu menu(this);

	QAction* configure = menu.addAction("Configure Products");

	QAction* result = menu.exec(point);
	if (result == configure)
	{
		openAndFocusConfigureDoc(
			formatUrl("%WEB_BASE%/universes/configure?id=%UNIVERSE_ID%#/#developerProducts", currentGameId));
	}
}

void RobloxGameExplorer::namedAssetsGroupContextMenuHandler(const QPoint& point)
{
	QMenu menu(this);

	QAction* add = menu.addAction("Add Image");

	QAction* bulkAdd = menu.addAction("Add Multiple Images");

	QAction* result = menu.exec(point);
	if (result == add)
	{
		bool created = false;
		QString newName;
		AddImageDialog dialog;
		dialog.runModal(this, &created, &newName);
	}
	else if (result == bulkAdd)
	{
		bulkAddNewImageNames();
	}
}

void RobloxGameExplorer::handleRename(const QPoint& globalPoint)
{
	static boost::once_flag flag = BOOST_ONCE_INIT;
		boost::call_once(boost::bind(&RobloxGoogleAnalytics::trackEvent,
	GA_CATEGORY_ACTION, "Game Explorer", "Context menu rename", 0, false), flag);

	QModelIndex index = indexAt(viewport()->mapFromGlobal(globalPoint));
	RBXASSERT(index.isValid());
	if (index.isValid())
	{
		edit(index);
	}
}

void RobloxGameExplorer::bulkAddNewImageNames()
{
	RobloxSettings settings;
	QString imageLastDir = settings.value(kLastImageDirectoryKey).toString();
	if (imageLastDir.isEmpty())
        imageLastDir = RobloxMainWindow::getDefaultSavePath();

	QStringList files = 
		QFileDialog::getOpenFileNames(this, "Choose images to upload", imageLastDir,
			"Images (*.jpg *.jpeg *.png *.gif *.bmp);;All Files (*.*)");

	if (!files.empty())
	{
		std::vector<QString> usedNames;
		getListOfImages(&usedNames);

		// documentation suggests copying file list before processing
		QStringList copy = files;
		std::vector<std::pair<QString, QString> > fileAndImageNames(copy.size());
		size_t index = 0;
		for (QStringList::iterator itr = copy.begin(); itr != copy.end(); ++itr)
		{
			std::pair<QString, QString>& fileAndImageName = fileAndImageNames[index++];
			fileAndImageName.first = QDir::toNativeSeparators(*itr);
			findAvailableImageNameFromFilename(*itr, &usedNames, &fileAndImageName.second);
			usedNames.push_back(fileAndImageName.second);
		}
		RBXASSERT(((int)index) == copy.size());

		UpdateUIManager::Instance().waitForLongProcess("Uploading Images",
			boost::bind(&bulkAddNewImageNamesThread, &fileAndImageNames, currentGameId, currentGameGroupId));
		reloadDataFromWeb();
	}
}

static void doInsert(shared_ptr<Instance> instance)
{
	if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
	{
		if (DataModel* dm = ideDoc->getDataModel())
		{
			RBX::DataModel::LegacyLock lock(dm, RBX::DataModelJob::Write);

			Selection* selection = dm->create<Selection>();
			Instance* parent;
			if (selection->size() == 1)
			{
				parent = selection->front().get();
			}
			else
			{
				parent = dm->getWorkspace();
			}
			
			instance->setParent(parent);
		}
	}
}

void RobloxGameExplorer::insertNamedImage(EntityProperties* imageInfo)
{
	shared_ptr<Decal> decal = Creatable<Instance>::create<Decal>();
	decal->setTexture(ContentId::fromGameAssetName(imageInfo->get<std::string>("Name").get()));
	doInsert(decal);
}

void RobloxGameExplorer::insertNamedScript(EntityProperties* scriptInfo, shared_ptr<LuaSourceContainer> container)
{
	const std::string assetName = scriptInfo->get<std::string>("Name").get();
	container->setScriptId(ContentId::fromGameAssetName(assetName));
	std::string scriptName = assetName;
	if (boost::starts_with(scriptName, "Scripts/"))
	{
		scriptName = scriptName.substr(8);
	}
	container->setName(scriptName);
	doInsert(container);
}

void RobloxGameExplorer::doneEditingUniverseName()
{
	std::string oldName = gameSettings.getName();
	std::string newName = gameNameEdit->text().toStdString();
	if (oldName == newName) return;

	gameSettings.setName(newName);

	bool success;
	std::string newSettings;
	UpdateUIManager::Instance().waitForLongProcess("Updating Game Name...",
		boost::bind(&updateNameThread, gameSettings.asJson(),
			formatUrl("%API_PROXY%/universes/update-universe?universeid=%UNIVERSE_ID%", currentGameId),
			&success, &newSettings));

	if (!success)
	{
		gameSettings.setName(oldName);
	}
	else
	{
		boost::promise<std::string> jsonPromise;
		jsonPromise.set_value(newSettings);
		gameSettings.setFromJsonFuture(HttpFuture(jsonPromise.get_future()));
	}
	gameNameEdit->setText(QString::fromStdString(gameSettings.getName()));
}

void RobloxGameExplorer::reloadDataFromWeb()
{
	currentSessionId++;

	// kick off request to load settings
	// todo: use HttpRbxApiService
	HttpOptions getOptions;
	getOptions.setDoNotUseCachedResponse();
	gameSettings.setFromJsonFuture(HttpAsync::get(
		formatUrl("%API_PROXY%/universes/get-info?universeId=%UNIVERSE_ID%", currentGameId),
		getOptions));

	if (!gameSettings.currentUserHasAccess() && !RobloxIDEDoc::getIsCloudEditSession())
	{
		nonGameLoaded();
		return;
	}

	for (unsigned int i = 0; i < ENTITY_CATEGORY_MAX; ++i)
	{
		setGroupLoadingStatus((EntityCategory)i);
		entitySettings[(EntityCategory)i] = EntityPropertiesForCategory::loadFromWeb(
			kWebSettings[i], currentGameId,
			boost::bind(&doneLoadingDataForCategory, QPointer<RobloxGameExplorer>(this),
			currentSessionId, (EntityCategory)i));
	}
}

void RobloxGameExplorer::doubleClickHandler(const QModelIndex& modelIndex)
{
    QStandardItem* item = getModel()->itemFromIndex(modelIndex);
    if (!item) return;

	QVariant callback = item->data(ROLE_DoubleClickCallback);
	if (!callback.isNull())
	{
		callback.value<boost::function<void()> >()();
	}
}

void RobloxGameExplorer::contextMenuHandler(const QPoint& point)
{
	QModelIndex index = indexAt(point);
    if (index.isValid())
	{
		if (QStandardItem* item = getModel()->itemFromIndex(index))
		{
			QVariant callback = item->data(ROLE_ContextMenuCallback);
			if (!callback.isNull())
			{
				callback.value<boost::function<void(const QPoint&)> >()(viewport()->mapToGlobal(point));
			}
		}
	}
}

void RobloxGameExplorer::itemChangedHandler(QStandardItem* updatedItem)
{
	QVariant callback = updatedItem->data(ROLE_NameChangedCallback);
	if (!callback.isNull())
	{
		callback.value<boost::function<void()> >()();
	}
}

void RobloxGameExplorer::handleNameUpdates()
{
	// find incorrect names
	for (int i = 0; i < ENTITY_CATEGORY_MAX; ++i)
	{
		EntityCategory category = (EntityCategory)i;
		QStandardItem* group = findGroup(category);
		EntityPropertiesForCategory::ref categoryProperties = entitySettings[category];
		RBXASSERT(group);
		if (!group) continue;

		for (int row = 0; row < group->rowCount(); ++row)
		{
			checkRowForNameUpdate(category, group->child(row));
		}
	}
}

void RobloxGameExplorer::checkRowForNameUpdate(EntityCategory category, QStandardItem* entityRow)
{
	QVariant rowId = getEntityId(entityRow, category);
	if (!rowId.isValid()) return;

	EntityProperties* props = entityRow->data(ROLE_EntityProperties).value<EntityProperties*>();
	RBXASSERT(props);
	if (!props) return;
			
	boost::optional<std::string> oldName = props->get<std::string>("Name");
	QString newName = entityRow->text();
	bool validated = true;

	if (category == ENTITY_CATEGORY_NamedAssets)
	{
		QString errorMessage;
		validated = validateImageName(newName, NULL, &errorMessage);
		if (!validated)
		{
			std::string stdErrorMessage = errorMessage.toStdString();
			StandardOut::singleton()->printf(MESSAGE_ERROR, "%s", stdErrorMessage.c_str());
		}

		for (QStandardItem* stepParent = entityRow->parent();
			stepParent != NULL && stepParent != findGroup(category);
			stepParent = stepParent->parent())
		{
			newName = stepParent->text() + "/" + newName;
		}
	}

	if (oldName.is_initialized() && oldName.get() != newName.toStdString())
	{
		bool success = false;
				
		if (validated)
		{
			if (category == ENTITY_CATEGORY_NamedAssets)
			{
				QMessageBox mb(this);
				mb.setText("Assets are referenced by name, so any references to this name will fail to "
					"load. Are you sure you want to rename this asset?");
				mb.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
				if (mb.exec() != QMessageBox::Yes)
				{
					if (uiSettings[category].nameMode == CategoryUiSettings::NameAsFolderPath)
					{
						entityRow->setText((uiSettings[category].displayNameGetter(props)).split('/').back());
					}
					else
					{
						entityRow->setText(uiSettings[category].displayNameGetter(props));
					}
					return;
				}
			}

			props->set("Name", newName.toStdString());
			UpdateUIManager::Instance().waitForLongProcess(
				QString("Updating name of %1 to %2").arg(QString::fromStdString(oldName.get())).arg(newName),
				boost::bind(&updateNameThread, props->asJson(),
					formatUrl(kWebSettings[category].updateFormatString, currentGameId, rowId),
					&success, (std::string*)NULL));
		}

		if (!success)
		{
			props->set("Name", oldName.get());
			if (uiSettings[category].nameMode == CategoryUiSettings::NameAsFolderPath)
			{
				entityRow->setText((uiSettings[category].displayNameGetter(props)).split('/').back());
			}
			else
			{
				entityRow->setText(uiSettings[category].displayNameGetter(props));
			}
			QMessageBox messageBox(this);
			messageBox.setText("Error while updating name. See Output window for details.");
			messageBox.setStandardButtons(QMessageBox::Ok);
			messageBox.exec();
		}
		else
		{
			// special name update logic for places
			if (category == ENTITY_CATEGORY_Places)
			{
				// if it is the currently opened place update tab name
				if (rowId.toInt() == currentPlaceId)
				{
					if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
					{
						ideDoc->updateDisplayName(newName);
					}
				}
			}
			if (category == ENTITY_CATEGORY_NamedAssets)
			{
				ContentId newContentId = ContentId::fromGameAssetName(newName.toStdString());
				ContentId oldContentId = ContentId::fromGameAssetName(oldName.get());

				// update temp files
				if (boost::optional<std::string> oldContentOverride =
						findCachedAssetOrEmpty(oldContentId, currentGameId))
				{
					updateAsset(newContentId, oldContentOverride.get());
				}
				else
				{
					// if there is a lingering unpublished change for the target name, remove it
					QFile(getUnpublishedAssetFilePath(newName.toStdString(), currentGameId)).remove();
				}

				// remove unpublished changes for old name, that name does not exist anymore
				QFile(getUnpublishedAssetFilePath(oldName.get(), currentGameId)).remove();

				int assetTypeId = props->get<int>("AssetTypeId").get();
				if (assetTypeId == (int)ASSET_TYPE_ID_Script)
				{
					if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
					{
						if (DataModel* dm = ideDoc->getDataModel())
						{
							DataModel::LegacyLock l(dm, DataModelJob::Write);
							ContentProvider* cp = dm->create<ContentProvider>();
							
							cp->invalidateCache(oldContentId);
							cp->invalidateCache(newContentId);
						}
					}

					LuaSourceBuffer oldLsb = LuaSourceBuffer::fromContentId(oldContentId);
					LuaSourceBuffer newLsb = LuaSourceBuffer::fromContentId(newContentId);
					if (RobloxScriptDoc* scriptDoc = RobloxDocManager::Instance().findOpenScriptDoc(oldLsb))
					{
						scriptDoc->setScript(scriptDoc->getDataModel(), newLsb);
					}
				}
				else if (assetTypeId == (int)ASSET_TYPE_ID_Image)
				{
					if (RobloxIDEDoc* ideDoc = RobloxDocManager::Instance().getPlayDoc())
					{
						QStringList list;
						list.push_back(QString("rbxgameasset://%1").arg(QString::fromStdString(oldName.get())));
						list.push_back(QString("rbxgameasset://%1").arg(newName));
						ideDoc->forceReloadImages(list);
					}
				}
			}
		}
	}
}

bool RobloxGameExplorer::namedAssetHasUnpublishedChanges(const std::string& name)
{
	return QFile(getUnpublishedAssetFilePath(name, currentGameId)).exists();
}

boost::shared_future<void> RobloxGameExplorer::publishIfThereAreLocalModifications(EntityProperties* settings)
{
	std::string name = settings->get<std::string>("Name").get();
	ContentId contentId = ContentId::fromGameAssetName(name);
	boost::optional<std::string> localVersion = findCachedAssetOrEmpty(contentId, currentGameId);
	
	if (localVersion && settings->get<int>("AssetTypeId") == (int)ASSET_TYPE_ID_Script)
	{
		return boost::shared_future<void>(
			publishScriptAsset(localVersion.get(), settings->get<int>("AssetId").get(),
				currentGameGroupId)
			.then(boost::bind(&removeFile, getUnpublishedAssetFilePath(name, currentGameId), _1)));
	}
	else
	{
		boost::promise<void> prom;
		prom.set_value();
		return boost::shared_future<void>(prom.get_future());
	}
}
