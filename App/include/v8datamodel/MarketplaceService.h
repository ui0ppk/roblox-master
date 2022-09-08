// This service is used for in-game purchases.  Most of the actual work occurs in a lua script on player's clients, 
// but this service is responsible for sending the appropriate signals around, and exposing a lua call to our users.
// There is also a nifty function exposed called "GetProductInfo" that will return a table from the web with properties about an assetId
#pragma once 
#include "V8Tree/Service.h"
#include "V8Tree/Instance.h"
#include "V8DataModel/Remote.h"

namespace RBX {

	extern const char* const sMarketplaceService;
	class MarketplaceService
		: public DescribedCreatable<MarketplaceService, Instance, sMarketplaceService, Reflection::ClassDescriptor::INTERNAL>
		, public Service
	{
		std::string devProductInfoUrl;
		std::string productInfoUrl;
		std::string playerOwnsAssetUrl;

		rbx::signals::scoped_connection playerRemovingConnection;
		boost::unordered_map<shared_ptr<Instance>, boost::function<void(bool, shared_ptr<Instance>)> > callbackFunctionMap;

		bool receiptProcessingEnabledByUser;
		rbx::signals::scoped_connection playerAddedReceiptConnection;
		rbx::signals::scoped_connection playerPurchaseReceiptConnection;
		rbx::signals::scoped_connection verifyPurchaseConnection;

		// Cache the MarketPlace Product Info Response
		struct ResponseCache
		{
			RBX::Time lastFetch;
			shared_ptr<const Reflection::ValueTable> values;
		};
		typedef boost::unordered_map<std::string, ResponseCache> UrlToResponseMap;
		UrlToResponseMap mMap;


		template<typename ResultType>
		void dispatchRequest(const std::string& url, boost::function<void(ResultType)> resumeFunction, boost::function<void(std::string)> errorFunction);

		bool isValidPlayer(const shared_ptr<Instance> player, const std::string& funcName, boost::function<void(std::string)> errorFunction = 0);

		void processPlayerOwnsAssetResponseSuccess(std::string response, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void processPlayerOwnsAssetResponseError(std::string error, boost::function<void(std::string)> errorFunction);

		void verifyPurchaseResponseNoDMLock(std::string response, int userId, int productId);
		void verifyPurchaseErrorNoDMLock(std::string error, int userId, int productId);

		void setupBackendOnlyReceiptHandling();
		void getReceiptsForJoiningPlayer(shared_ptr<Instance> newPlayer);
		void getReceiptsAfterPurchase(std::string response, int userId, int productId);
		void fetchAndProcessTransactions(int userId);

		static void handleReceiptQueryResponseSuccess(weak_ptr<MarketplaceService> weakMk, std::string response);
		static void handleReceiptQueryResponseFailure(weak_ptr<MarketplaceService> weakMk, std::string error);

		static void processReceipt(weak_ptr<MarketplaceService> weakMk, shared_ptr<const Reflection::ValueTable> receiptInfo);
		static void processReceiptSuccessContinuation(shared_ptr<MarketplaceService> mk,
			std::string receiptId, int currencyType, int currencySpent, shared_ptr<const Reflection::Tuple> result);

	private:
		typedef DescribedCreatable<MarketplaceService, Instance, sMarketplaceService, Reflection::ClassDescriptor::INTERNAL> Super;

	protected:
		void onServiceProvider(ServiceProvider* oldSp, ServiceProvider* newSp) override;

	public:
		MarketplaceService();

		enum CurrencyType
		{
			CURRENCY_DEFAULT = 0,
			CURRENCY_ROBUX   = 1,
			CURRENCY_TIX	= 2,
		};

		enum InfoType
		{
			INFO_ASSET = 0,
			INFO_PRODUCT = 1,
			INFO_NONE = 2
		};

		enum ProductPurchaseDecision
		{
			DECISION_NOT_PROCESSED_YET = 0,
			DECISION_PURCHASE_GRANTED = 1
		};

        static const char* urlApiPath() { return "marketplace"; }

		void verifyPurchaseTicket(std::string ticket, int userId, int productId);
        
        rbx::remote_signal<void(shared_ptr<Instance> player, std::string productId, bool isPurchased)> nativePurchaseFinished;
		rbx::remote_signal<void(shared_ptr<Instance> player, std::string productId)> promptNativePurchaseRequested;

		rbx::remote_signal<void(shared_ptr<Instance> player, std::string productId, std::string receipt, bool isPurchased)> thirdPartyPurchaseFinished;
		rbx::remote_signal<void(shared_ptr<Instance> player, std::string productId)> promptThirdPartyPurchaseRequested;

		rbx::remote_signal<void(shared_ptr<Instance> player, int assetId, bool equipIfPurchased, CurrencyType currencyType)> promptProductPurchaseRequested;
		rbx::remote_signal<void(int userId, int assetId, bool isPurchased)> promptProductPurchaseFinished;
				
		rbx::remote_signal<void(shared_ptr<Instance> player, int assetId, bool isPurchased)> promptPurchaseFinished;
		rbx::remote_signal<void(shared_ptr<Instance> player, int assetId, bool equipIfPurchased, CurrencyType currencyType)> promptPurchaseRequested;

		rbx::remote_signal<void(std::string response, int userId, int productId)> clientPurchaseSuccess;
		rbx::remote_signal<void(shared_ptr<const Reflection::ValueTable> responseTable)> serverPurchaseVerification;

        rbx::remote_signal<void(shared_ptr<const Reflection::Tuple>)> clientLuaDialogRequested;
		rbx::remote_signal<void(bool, shared_ptr<Instance>)> luaDialogCallbackSignal;

		typedef boost::function<void(shared_ptr<const Reflection::ValueTable>,
			Reflection::AsyncCallbackDescriptor::ResumeFunction,
			Reflection::AsyncCallbackDescriptor::ErrorFunction)> ReceiptCallback;
		ReceiptCallback receiptCallback;
		void processReceiptsCallbackChanged(const ReceiptCallback& oldValue);

		void signalPromptProductPurchaseFinished(int userId, const int productId, const bool success);
		void promptProductPurchase(const shared_ptr<Instance> player, const int productId, const bool equipIfPurchased, const CurrencyType currencyType);

		void signalPromptPurchaseFinished(const shared_ptr<Instance> player, const int assetId, const bool isPurchase);
		void promptPurchase(const shared_ptr<Instance> player, const int assetId, const bool equipIfPurchased, const CurrencyType currencyType);

		void signalClientPurchaseSuccess(std::string ticket, int userId, int productId);

		void getProductInfo(const int assetId, const MarketplaceService::InfoType infoType, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction);

		void onReceivedRawProductInfoSuccess(std::string urlPath, std::string response, boost::weak_ptr<RBX::DataModel> weakDM, boost::function<void(shared_ptr<const Reflection::ValueTable>)> resumeFunction, boost::function<void(std::string)> errorFunction);
		void onReceivedRawProductInfoError(std::string error, boost::weak_ptr<RBX::DataModel> weakDM, boost::function<void(std::string)> errorFunction);

		void playerOwnsAsset(const shared_ptr<Instance> player, const int assetId, boost::function<void(bool)> resumeFunction, boost::function<void(std::string)> errorFunction);

		bool launchClientLuaDialog(std::string message, std::string accept, std::string decline, shared_ptr<Instance> player, boost::function<void(bool, shared_ptr<Instance>)> callback);
		void signalServerLuaDialogClosed(bool value);
		void executeClientDialogCallback(bool value, shared_ptr<Instance> player);

		void promptThirdPartyPurchase(const shared_ptr<Instance> player, std::string thirdPartyProductId);
		void signalPromptThirdPartyPurchaseFinished(const shared_ptr<Instance> player, std::string productId, std::string receipt, const bool success);
        
        void getDeveloperProductsAsync(boost::function<void(shared_ptr<Instance>) > resumeFunction,
            boost::function<void(std::string)> errorFunction);
        
        void promptNativePurchase(const shared_ptr<Instance> player, std::string nativeProductId);
        void signalPromptNativePurchaseFinished(const shared_ptr<Instance> player, std::string productId, const bool success);

        void processRemoteEvent(const Reflection::EventDescriptor& descriptor, const Reflection::EventArguments& args, const SystemAddress& source) override;
        static void processPurchaseFinished( weak_ptr<DataModel> weakDm, Reflection::EventArguments args, SystemAddress source, const bool isOwner);
        static void processPurchaseError( weak_ptr<DataModel> weakDm, Reflection::EventArguments args, SystemAddress source, const std::string msg);
        static void processPurchaseFinishedCallback(DataModel* dm, Reflection::EventArguments args, SystemAddress source);


	};
}
