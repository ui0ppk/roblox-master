#pragma once

#include "reflection/Type.h"
#include "rbx/CEvent.h"
#include "Script/LuaSourceContainer.h"
#include "Util/ContentId.h"
#include "Util/HttpAsync.h"
#include "V8DataModel/ContentProvider.h"

#include <QIcon>
#include <QPointer>
#include <QString>
#include <QKeyEvent>
#include <QLineEdit>
#include <QTreeView>

#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/future.hpp>

class QLabel;
class QPushButton;
class QStandardItem;
class QStandardItemModel;


enum EntityCategory
{
    ENTITY_CATEGORY_Places, // the node that places are inserted under
    ENTITY_CATEGORY_DeveloperProducts, // The node that dev products are inserted under
	ENTITY_CATEGORY_Badges,
	ENTITY_CATEGORY_NamedAssets,
    ENTITY_CATEGORY_MAX
};

enum AliasType
{
	ALIAS_TYPE_Asset = 1,
	ALIAS_TYPE_DeveloperProduct = 2,
	ALIAS_TYPE_AssetVersion = 3,
};

enum AssetTypeId
{
	ASSET_TYPE_ID_Image = 1,
	ASSET_TYPE_ID_Script = 5,
	ASSET_TYPE_ID_Animation = 24
};

// Caller is responsible for thread safety when using setter methods
class EntityProperties
{
public:
	EntityProperties();

	void setFromJsonFuture(const boost::shared_future<std::string>& jsonFuture);
	void setFromValueTable(const boost::shared_ptr<const RBX::Reflection::ValueTable>& valueTable);
	void setFromEntitySettings(EntityProperties& other);
	std::string asJson();
	void waitUntilLoaded();
	bool isLoadedAndParsed() const;
	template<class T>
	boost::optional<T> get(const std::string& fieldName)
	{
		boost::optional<RBX::Reflection::Variant> var = getVariant(fieldName);
		if (var.is_initialized() && var->isType<T>())
		{
			return var->cast<T>();
		}
		return boost::optional<T>();
	}
	boost::optional<RBX::Reflection::Variant> getVariant(const std::string& fieldName);
	// Caller is responsible for ensuring the thread safety of this call
	template<class T>
	void set(const std::string& fieldName, const T& value)
	{
		waitUntilLoaded();
		data[fieldName] = value;
	}

	typedef std::vector<EntityProperties> EntityCollection;
	void addChild(const EntityProperties& child) 
	{
		children.push_back(child);
	}
	EntityCollection& getChildren() 
	{
		return children;
	}

private:
	bool hasPendingFuture;
	boost::shared_future<std::string> jsonFuture;
	bool ableToGetFuture;
	bool ableToParseJson;
	RBX::Reflection::ValueTable data;
	EntityCollection children;
};

class UniverseSettings
{
public:
	void setFromJsonFuture(const boost::shared_future<std::string>& jsonFuture);
	void waitUntilLoaded();
	bool isLoadedAndParsed() const;
	bool hasRootPlace();
	int rootPlaceId();
	std::string getName();
	void setName(const std::string& newName);
	bool currentUserHasAccess();
	std::string asJson();

private:
	EntityProperties properties;
};

struct CategoryWebSettings
{
	std::string jsonItemGroupFieldName;
	std::string jsonItemIdFieldName;
	std::string perItemIdFieldName;
	std::string fetchPageUrlFormatString;
	std::string createFormatString;
	std::string updateFormatString;
	std::string removeFormatString;
	std::string getContentFormatString;
	std::string writeContentFormatString;
	boost::function<void(int, EntityProperties*)> propertiesFixer;
};

class EntityPropertiesForCategory : public boost::enable_shared_from_this<EntityPropertiesForCategory>
{
public:
	typedef boost::shared_ptr<EntityPropertiesForCategory> ref;
	static ref loadFromWeb(const CategoryWebSettings& webSettings, int universeId,
		boost::function<void()> doneLoadingCallback);
	bool isLoadedAndParsed();

	template<class IdType>
	void publishTo(ref target, const std::vector<std::string>& ignoredFields,
		boost::unordered_map<IdType, boost::function<IdType()> >* idRemaps);
	template<class T>
	void transform(boost::function<T(EntityProperties*)> transform, std::vector<T>* out)
	{
		waitUntilLoaded();
		for (EntityCollection::iterator itr = data.begin(); itr != data.end(); ++itr)
		{
			out->push_back(transform(&(*itr)));
		}
	}

private:
	EntityPropertiesForCategory(const CategoryWebSettings& webSettings, const int universeId);

	const CategoryWebSettings& webSettings;
	const int universeId;
	RBX::CEvent doneEvent;
	boost::function<void()> doneLoadingCallback;
	typedef std::vector<EntityProperties> EntityCollection;
	EntityCollection data;

	void requestPage(int page);
	void handlePage(int page, std::string* json, std::exception* error);
	void waitUntilLoaded();
};

class AddImageDialog : public QObject
{
	Q_OBJECT
public:
	void runModal(QWidget* parent, bool* created, QString* newName);

private:
	QDialog* dialog;

	QLineEdit* nameEdit;
	QLabel* nameErrorMessage;

	QPushButton* fileNameEdit;
	QLabel* fileNameLabel;
	QLabel* fileNameErrorMessage;

	QLabel* generalErrorMessage;
	
	int currentGameId;
	boost::optional<int> currentGameGroupId;
	bool* created;
	QString* newName;
	std::vector<QString> usedNames;

	void createImageAndNameThread();

private Q_SLOTS:
	bool validateName();
	bool validateImageFile();
	void openFileSelector();
	void createButtonClicked();
};

class AbortableLineEdit : public QLineEdit
{
	Q_OBJECT
public:
	AbortableLineEdit(boost::function<std::string()> getLastValue)
		: QLineEdit("")
		, getLastValue(getLastValue)
	{
	}

protected:
	virtual void keyReleaseEvent(QKeyEvent* keyEvent)
	{
		if (keyEvent->key() == Qt::Key_Escape)
		{
			setText(QString::fromStdString(getLastValue()));
			clearFocus();
		}
		if (keyEvent->key() == Qt::Key_Enter ||
			keyEvent->key() == Qt::Key_Return)
		{
			clearFocus();
		}
		QLineEdit::keyReleaseEvent(keyEvent);
	}

private:
	boost::function<std::string()> getLastValue;
};

class RobloxGameExplorer : public QTreeView, public RBX::AssetFetchMediator
{
    Q_OBJECT
public:
	RobloxGameExplorer(QWidget* parent);

// Asset management functions
	virtual boost::optional<std::string> findCachedAssetOrEmpty(const RBX::ContentId& contentId, int universeId);
	void updateAsset(const RBX::ContentId& contentId, const std::string& newContents);
	void notifyIfUnpublishedChanges(int universeId);
	void publishNamedAssetsToCurrentSlot();
	static RBX::HttpFuture publishScriptAsset(const std::string& scriptText,
		boost::optional<int> optionalAssetId, boost::optional<int> optionalGroupId);

// These methods must be run from main thread for concurrency protection:
	void getListOfImages(std::vector<QString>* out);
	void getListOfScripts(std::vector<QString>* out);
	void getListOfAnimations(std::vector<QString>* out);
	int getCurrentGameId() const { return currentGameId; }
	boost::optional<int> getCurrentGameGroupId() { return currentGameGroupId; }

	QString getAnimationsDataJson();

Q_SIGNALS:
	void namedAssetsLoaded(int gameId);
	void namedAssetModified(int gameId, const QString& assetName);
	
public Q_SLOTS:
	void nonGameLoaded();
    void openGameFromGameId(int gameId);
    void openGameFromPlaceId(int placeId);
	void onCloseIdeDoc();
	void publishGameToNewSlot();
	void publishGameToNewGroupSlot(int groupId);
	void publishGameToExistingSlot(int gameId);
	void reloadDataFromWeb();

private:
	struct CategoryUiSettings
	{
		enum NameMode 
		{
			FlatNames,
			NameAsFolderPath
		};

		QString groupDisplayName;
		QIcon icon;
		boost::function<QString(EntityProperties*)> displayNameGetter;
		boost::function<bool(EntityProperties*)> shouldBuildExplorerItemCallback;
		boost::function<void(EntityProperties*)> doubleClickCallback;
		// The QPoint provided will be global (ready to use for showing context menu)
		boost::function<void(const QPoint&, EntityProperties*)> contextMenuCallback;
		boost::function<void(const QPoint&)> groupContextMenuCallback;
		boost::function<void(QStandardItem*)> nameChangedCallback;
		NameMode nameMode;
	};

	struct NamedAssetCopyInfo
	{
		std::string assetName;
		int aliasType;
		bool needsTargetIdUpdate;
		EntityProperties newAssetIdProperties;
		boost::function<int()> newTargetIdGetter;
	};

	int currentGameId;
    int currentPlaceId;
    // This widget is stateful, and does a lot of async requests for data. Use
    // a session identifier to throw out async requests that are stale (e.g.
    // request places for a universe, then the widget is pointed to a different
    // universe before the request returns).
    int currentSessionId;
	boost::optional<int> currentGameGroupId;
	bool enoughDataLoadedToBeAbleToOpenStartPlace;
	bool startPlaceOpenRequested;

	boost::unordered_map<EntityCategory, CategoryUiSettings> uiSettings;
	boost::unordered_map<EntityCategory, EntityPropertiesForCategory::ref > entitySettings;
	UniverseSettings gameSettings;

	QLabel* nameLabel;
	AbortableLineEdit* gameNameEdit;
	QLabel* idLabel;
	QPushButton* reloadButton;

    static void handleUniverseForPlaceResponse(QPointer<RobloxGameExplorer> explorer,
        int originatingSessionId, int placeId, std::string* data, std::exception* error);
	static void doneLoadingDataForCategory(QPointer<RobloxGameExplorer> explorer,
		int originatingSessionId, EntityCategory category);
	static QStandardItem* buildItem(const CategoryUiSettings& uiSettings,
		const CategoryWebSettings& webSettings, QStandardItem* root, EntityProperties* settings);
	
    QStandardItemModel* getModel();
    QStandardItem* findGroup(EntityCategory type);
	QStandardItem* makeGroup(EntityCategory type);
	void openAndFocusConfigureDoc(const std::string& url);
	void setGroupLoadingStatus(EntityCategory category);
	void placeDoubleClickCallback(EntityProperties* placeInfo);
	void namedAssetsDoubleClickCallback(EntityProperties* assetInfo);
	void openPlace(int placeId);
	void updateItemForOpenedPlaceState(QStandardItem* placeItem);
	void refreshOpenedPlaceIndicator();
	void openStartPlaceIfRequestedAndNoPlaceOpenedAndDataReady();
	void publishInternal(boost::function<int()> newUniverseFuture);
	std::pair<int, boost::function<std::string()> > buildPlaceContentPair(EntityProperties* properties);
	NamedAssetCopyInfo buildNamedContentInfo(EntityPropertiesForCategory::ref targetProperties, int sourceUnviverseId,
		boost::optional<int> targetUniverseGroupId, boost::unordered_map<int, boost::function<int()> >* devProductIdRemap,
		EntityProperties* properties);
	void publishGameThread(boost::function<int()> newUniverseFuture, bool* publishSucceeded, int* targetGameId);
	void addNewPlace();
	void placeContextMenuHandler(const QPoint& point, EntityProperties* properties);
	void developerProductContextMenuHandler(const QPoint& point, EntityProperties* properties);
	void namedAssetsContextMenuHandler(const QPoint& point, EntityProperties* properties);
	void badgesContextMenuHandler(const QPoint& point, EntityProperties* properties);
	void badgesPlaceContextMenuHandler(const QPoint& point, EntityProperties* properties);
	void placeGroupContextMenuHandler(const QPoint& point);
	void developerProductGroupContextMenuHandler(const QPoint& point);
	void namedAssetsGroupContextMenuHandler(const QPoint& point);
	void handleRename(const QPoint& globalPoint);
	void handleRemoveAssetName(const std::string& name, bool* needReload);
	void bulkAddNewImageNames();
	void insertNamedImage(EntityProperties* imageInfo);
	void insertNamedScript(EntityProperties* scriptInfo, shared_ptr<RBX::LuaSourceContainer> container);
	void checkRowForNameUpdate(EntityCategory category, QStandardItem* entityRow);
	void afterNamedAssetsFinishedRecursive(QStandardItem* root, QStringList& imagesToReload);
	bool namedAssetHasUnpublishedChanges(const std::string& name);
	boost::shared_future<void> publishIfThereAreLocalModifications(EntityProperties* settings);
    
private Q_SLOTS:
	void doOpenGameFromGameId(int gameId);
	void failedToLoadSettings(int originatingSessionId);
	void populateWithLoadedData(int originatingSessionId, int category);
	void afterPlacesLoadedFinished(int originatingSessionId);
	void afterNamedAssetsFinished(int originatingSessionId);
	void refreshNamedScriptIcons(int originatingSessionId);
	void afterBadgesFinished(int originatingSessionId);
	void thumbnailLoadedForImage(int originatingSessionId, QModelIndex item, QVariant future);

    void doubleClickHandler(const QModelIndex& modelIndex);
	// The QPoint provided will be relative to the viewport
	void contextMenuHandler(const QPoint& clickPoint);
	void itemChangedHandler(QStandardItem* updatedItem);
	void handleNameUpdates();
	void doneEditingUniverseName();
};
