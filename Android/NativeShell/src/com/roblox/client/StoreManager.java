package com.roblox.client;

import java.util.List;
import java.util.ListIterator;
import java.util.Map;

import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.os.AsyncTask;
import android.util.Log;

import com.amazon.device.iap.PurchasingListener;
import com.amazon.device.iap.PurchasingService;
import com.amazon.device.iap.model.FulfillmentResult;
import com.amazon.device.iap.model.Product;
import com.amazon.device.iap.model.ProductDataResponse;
import com.amazon.device.iap.model.PurchaseResponse;
import com.amazon.device.iap.model.PurchaseUpdatesResponse;
import com.amazon.device.iap.model.Receipt;
import com.amazon.device.iap.model.UserDataResponse;
import com.roblox.client.http.HttpAgent;
import com.roblox.client.http.HttpResponse;
import com.roblox.client.http.OnRbxHttpRequestFinished;
import com.roblox.client.http.RbxHttpPostRequest;
import com.roblox.iab.IabHelper;
import com.roblox.iab.IabHelper.OnConsumeFinishedListener;
import com.roblox.iab.IabResult;
import com.roblox.iab.Inventory;
import com.roblox.iab.Purchase;



public class StoreManager {
	protected static final String TAG = "StoreManager";
    protected static final String PENDING_PURCHASE_ROBLOX_USER_NAME = "RobloxUserNameForPendingAmazonPurchase";
    protected static final String PENDING_PURCHASE_AMAZON_USER_ID = "AmazonUserIDForPendingAmazonPurchase";
    protected static final String PENDING_PURCHASE_AMAZON_RECEIPT_ID = "AmazonReceiptIDForPendingAmazonPurchase";
	private IabHelper mGoogleIABHelper;
	private Context mContext;
	private RobloxActivity mActivity;
	private String mProductId = null;
	private String mUserName = "";
	private long mPlayerPtr = 0;
    private SharedPreferences mKeyValues;
    private boolean bDebugPurchasing = false;
    private enum StoreManagerType
    {
        IAB_GOOGLE,
        IAB_AMAZON,
        IAB_NONE,
        IAB_UNKNOWN
    }
    public StoreManagerType iabStoreType = StoreManagerType.IAB_UNKNOWN;

    void printLogMessage(String message)
    {
        if (bDebugPurchasing)
            Log.d(TAG, message);
    }
	
	private static StoreManager storeMgr = null;
	protected StoreManager(Context ctx) {
        mContext = ctx;
        mKeyValues = RobloxSettings.mKeyValues;
        if(BuildConfig.USE_AMAZON_PURCHASING)
        {
            PurchasingService.registerListener(mContext, new AmazonPurchasingListener());
            printLogMessage("onCreate: sandbox mode is:" + PurchasingService.IS_SANDBOX_MODE);
            iabStoreType = StoreManagerType.IAB_AMAZON;
        }
        else // Google Play Store
        {
            // Break down this key later and combine with concatenation
            String base64EncodedPublicKey = "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0gQsTOERl7cAXlms9RMRN+XhTyE9h1oX/Wubr0x6FthR6gqktjdHOJ7ge6RR5Tbdpnv9/uhBMjk2hZOG/UktG8gxHbC0FZYdgm2T56tZrkpmodVk3+jN4gPBDIDhSPoKPIbu0dCbiTNOudL68nJVj+jRZI3nk4UDATTktWL7mzHkkt2B9BvKoA7MLJdGVhOSQzMgcTycI14em75apxWliIUDPz11L2USvIddTT+oPvRqLGe+BmIIvS5rCqdEqpLN4G0Qn6ioCw5R6I+kBy0cDY1604Vs5/FEPI+fSk48Kme+peiX+hGVlPF6ZZ9jF6yT/vAjpvn/DEEwkMMa7JnmhwIDAQAB";
            mGoogleIABHelper = new IabHelper(mContext, base64EncodedPublicKey);

            // enable debug logging (for a production application, you should set this to false).
            mGoogleIABHelper.enableDebugLogging(bDebugPurchasing);

            mGoogleIABHelper.startSetup(new IabHelper.OnIabSetupFinishedListener()
            {
                @Override
                public void onIabSetupFinished(IabResult result)
                {
                    if(!result.isSuccess())
                    {
                        printLogMessage("Google IAB is not setup");
                        mGoogleIABHelper = null;
                        iabStoreType = StoreManagerType.IAB_NONE;
                    }
                    else
                    {
                        printLogMessage("Google IAB is setup");
                        iabStoreType = StoreManagerType.IAB_GOOGLE;
                    }
                }
            });
        }
	}
	public static StoreManager getStoreManager(Context ctx)
	{
		if(storeMgr == null)
			storeMgr = new StoreManager(ctx);
		
		return storeMgr;
	}
	
	private boolean internalDoInAppPurchase()
	{
        try
        {
            if(mGoogleIABHelper != null && iabStoreType == StoreManagerType.IAB_GOOGLE)
            {
                printLogMessage("Do Google Purchase Initiate");
                mGoogleIABHelper.launchPurchaseFlow(mActivity, mProductId.toString(), 10001, mPurchaseFinishedListener, mUserName);
            }
            else if(iabStoreType == StoreManagerType.IAB_AMAZON)
            {
                String pendingAmazonPurchaseUserName = mKeyValues.getString(PENDING_PURCHASE_ROBLOX_USER_NAME, "");
                if (pendingAmazonPurchaseUserName.isEmpty()) {
                    printLogMessage("Do Amazon Purchase Initiate");
                    PurchasingService.purchase(mProductId.toString());
                }
                else
                {
                    Utils.alertExclusively("There is a pending Purchase with ROBLOX User Name. Please login with User Name: " + pendingAmazonPurchaseUserName);
                    grantPendingPurchases();
                }
            }
        }
        catch( IllegalStateException e )
        {
            // This was a very rare crash in Google Analytics.  No idea why.
            Utils.alertUnexpectedError("StoreManager IllegalStateException");
            return false;
        }
        return true;
	}
	
	
	private void resetPurchaseData()
	{
		mUserName = "";
		mActivity = null;
		mProductId = "";
		mPlayerPtr = 0;
        SharedPreferences.Editor editor = mKeyValues.edit();
        editor.remove(PENDING_PURCHASE_ROBLOX_USER_NAME);
        editor.remove(PENDING_PURCHASE_AMAZON_USER_ID);
        editor.remove(PENDING_PURCHASE_AMAZON_RECEIPT_ID);
        editor.apply();
	}
	
	// for testing purchasing use "android.test.purchased" as mProductId
	// regular product ids should look like "com.roblox.client.robux90"
	
	public boolean doInAppPurchaseForProduct(RobloxActivity activity, String productId, String userName, long playerPtr)
	{
		mUserName = userName;
		mActivity = activity;
		mProductId = productId;
		mPlayerPtr = playerPtr;

        // New pre-purchase validate check
        if(purchasingEnabled()) {
            String params = RobloxSettings.validatePurchaseParams(mProductId);
            doCommonValidationCheck(params);

            return true;
        }
        else
            return false;
	}
	
	public void grantPendingPurchases()
	{
		if(mGoogleIABHelper != null && iabStoreType == StoreManagerType.IAB_GOOGLE) {
            printLogMessage("Do Amazon Purchase GrantPending");
            mGoogleIABHelper.queryInventoryAsync(mGotInventoryListener);
        }
		else if(iabStoreType == StoreManagerType.IAB_AMAZON)
        {
            printLogMessage("Do Amazon Purchase GrantPending");
            PurchasingService.getPurchaseUpdates(true);
        }
	}
	
	
	// for testing purchasing use "android.test.purchased" as mProductId
	// regular product ids should look like "com.roblox.client.robux90"
	public boolean doInAppPurchaseForUrl(RobloxActivity activity, String urlString, String userName)
	{
		mUserName = userName;
		mActivity = activity;
		Uri uriObject = Uri.parse(urlString);
		mProductId = uriObject.getQueryParameter("id");
		//mProductId = "android.test.purchased";

        // New pre-purchase validate check
        if(purchasingEnabled()) {
            String params = RobloxSettings.validatePurchaseParams(mProductId);
            doCommonValidationCheck(params);

            return true;
        }
        else
            return false;
	}

    private void doCommonValidationCheck(String params) {
        RbxHttpPostRequest req = new RbxHttpPostRequest(RobloxSettings.validatePurchaseUrl(), params, null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                try {
                    String responseStr = response.responseBodyAsString();
                    if (!responseStr.isEmpty()) {
                        if (responseStr.equals("OK")) {
                            internalDoInAppPurchase();
                        } else if (responseStr.equals("Error")) {
                            Utils.alert(R.string.PurchaseValidateError);
                        } else if (response.equals("Retry")) {
                            Utils.alert(responseStr);
                        } else if (responseStr.equals("Limit")) {
                            Utils.alert(R.string.PurchaseValidateLimit);
                        }
                    }
                } catch (Exception e) {
                    e.printStackTrace();
                }
            }
        });
        req.execute();
    }
	
	public boolean handleActivityResult(int requestCode, int resultCode, Intent data) {
        printLogMessage("onActivityResult(" + requestCode + "," + resultCode + "," + data);
        if (iabStoreType != StoreManagerType.IAB_GOOGLE || mGoogleIABHelper == null)
            return false;

        // Pass on the activity result to the helper for handling, only Google needs it.
        if(mGoogleIABHelper.handleActivityResult(requestCode, resultCode, data))
            return true;

        return false;
    }
	
	public void handleActivityResume()
	{
        // Only Amazon needs it to get Pending Purchases.
        if (iabStoreType == StoreManagerType.IAB_AMAZON )
        {
            printLogMessage("Handle Activity Resume");
            PurchasingService.getUserData();
            grantPendingPurchases();
        }
	}
	
	
	// Google IAB Listener that's called when we finish querying the items and subscriptions we own
    IabHelper.QueryInventoryFinishedListener mGotInventoryListener = new IabHelper.QueryInventoryFinishedListener() {
        public void onQueryInventoryFinished(IabResult result, Inventory inventory) {
            printLogMessage("Query inventory finished.");

            // Have we been disposed of in the meantime? If so, quit.
            if (mGoogleIABHelper == null) return;

            // Is it a failure?
            if (result.isFailure()) {
                printLogMessage("Failed to query inventory: " + result);
                return;
            }

            printLogMessage("Query inventory was successful.");

            List<Purchase> purchases = inventory.getAllPurchases();
            for(ListIterator<Purchase> it = purchases.listIterator(purchases.size()); it.hasPrevious();)
            {
            	Purchase purchase = it.previous();
            	if(mUserName.equals(purchase.getDeveloperPayload()))
            		verifyDeveloperPayload(purchase, true); 
            }

            if (bDebugPurchasing)
                mGoogleIABHelper.consumeAsync(purchases, null);
        }
    };
    
    boolean purchasingEnabled()
    {
    	boolean retVal = false;
    	if(!mUserName.isEmpty() && (iabStoreType == StoreManagerType.IAB_AMAZON || iabStoreType == StoreManagerType.IAB_GOOGLE))
    	{
    		retVal = true;
    	}
    	
    	return retVal;
    }
    
    /** Google IAB Verifies the developer payload of a purchase. */
    void verifyDeveloperPayload(Purchase p, boolean isRetry) {
        /*
         * TODO: verify that the developer payload of the purchase is correct. It will be
         * the same one that you sent when initiating the purchase.
         *
         * WARNING: Locally generating a random string when starting a purchase and
         * verifying it here might seem like a good approach, but this will fail in the
         * case where the user purchases an item on one device and then uses your app on
         * a different device, because on the other device you will not have access to the
         * random string you originally generated.
         *
         * So a good developer payload has these characteristics:
         *
         * 1. If two different users purchase an item, the payload is different between them,
         *    so that one user's purchase can't be replayed to another user.
         *
         * 2. The payload must be such that you can verify it even when the app wasn't the
         *    one who initiated the purchase flow (so that items purchased by the user on
         *    one device work on other devices owned by the user).
         *
         * Using your own server to store and verify developer payloads across app
         * installations is recommended.
         */
        	// Do website billing service verification if the username matches
        	// Only return true if Billing service passes verification

        launchVerifyPurchaseReceipt(mActivity, this, p, mGoogleIABHelper, isRetry, isRetry ? null : mConsumeFinishedListener, "", "", null);
    }

 // Google Callback for when a purchase is finished
    IabHelper.OnIabPurchaseFinishedListener mPurchaseFinishedListener = new IabHelper.OnIabPurchaseFinishedListener() {
        public void onIabPurchaseFinished(IabResult result, Purchase purchase) {
            printLogMessage("Purchase finished: " + result + ", purchase: " + purchase);
            // if we were disposed of in the meantime, quit.
            if (mGoogleIABHelper == null) return;
            
       
            if (result.isFailure()) 
            {
            	Utils.alertExclusively("Purchasing cancelled: " + result);
            	grantPendingPurchases();
            	ActivityGlView.inGamePurchaseFinished(false,mPlayerPtr,mProductId);
                return;
            }
            verifyDeveloperPayload(purchase, false);
        }
    };
    
    // Google IAB callback when consumption is complete
    IabHelper.OnConsumeFinishedListener mConsumeFinishedListener = new IabHelper.OnConsumeFinishedListener() {
        public void onConsumeFinished(Purchase purchase, IabResult result) {
            printLogMessage("Consumption finished. Purchase: " + purchase + ", result: " + result);

            // if we were disposed of in the meantime, quit.
            if (mGoogleIABHelper == null) return;

            if (result.isSuccess()) {
            	Utils.alertExclusively("Purchase successful, your product will be delivered shortly.");
            	ActivityGlView.inGamePurchaseFinished(true,mPlayerPtr,mProductId);
            }
            else {
            	Utils.alertExclusively("Error while purchasing. Receipt verification failed: " + result);
            	ActivityGlView.inGamePurchaseFinished(false,mPlayerPtr,mProductId);
            }
            
            resetPurchaseData();

            printLogMessage("End consumption flow.");
        }
    };

    private void launchVerifyPurchaseReceipt(final RobloxActivity activity,
                                             final StoreManager storeManager,
                                             final Purchase purchase,
                                             final IabHelper iabHelper,
                                             final boolean isRetry,
                                             final OnConsumeFinishedListener consumeFinishedListener,
                                             final String amazonReceiptId,
                                             final String amazonUserId,
                                             final AmazonPurchasingListener amazonListener) {
//        (new VerifyPurchaseReceipt(activity, mgr, p, h, isRetry, listener, amazonReceiptId, amazonUserId, amazonListener)).execute();
        String url = "";

        if(storeManager.iabStoreType == StoreManagerType.IAB_GOOGLE)
            url = RobloxSettings.verifyPurchaseReceiptUrlForGoogle(purchase, isRetry);
        else if(storeManager.iabStoreType == StoreManagerType.IAB_AMAZON)
            url = RobloxSettings.verifyPurchaseReceiptUrlForAmazon(amazonReceiptId, amazonUserId, isRetry);

        RbxHttpPostRequest verifyPurchaseReq = new RbxHttpPostRequest(url, "", null, new OnRbxHttpRequestFinished() {
            @Override
            public void onFinished(HttpResponse response) {
                if( response.responseBodyAsString().isEmpty() )
                {
                    Utils.alertIfNetworkNotConnected();
                    Log.d(TAG, "Response NULL");
                }
                else try
                {
                    if( response.responseBodyAsString().equals("OK") )
                    {
                        if (verifySuccessResponse(storeManager, iabHelper, purchase, consumeFinishedListener, amazonListener, amazonReceiptId, isRetry))
                            return;
                    }
                    else if ( response.responseBodyAsString().equals("error") || response.responseBodyAsString().equals("Error") )
                    {
                        verifyErrorResponse();
                    }
                    else if ( response.responseBodyAsString().equals("Bogus") || response.responseBodyAsString().equals("bogus") )
                    {
                        verifyBogusResponse(storeManager, iabHelper, purchase, consumeFinishedListener, amazonListener, isRetry);
                    }
                }
                catch (Exception e)
                {
                    // swallow all, not a show stopper
                }
            }
        });
        verifyPurchaseReq.execute();
    }

    private boolean verifySuccessResponse(StoreManager storeManager, IabHelper iabHelper, Purchase purchase, OnConsumeFinishedListener consumeFinishedListener, AmazonPurchasingListener amazonListener, String amazonReceiptId, boolean isRetry) {
        // Move this to inside receipt verification
        Utils.sendAnalytics("StoreManager", "PurchaseOK");
        Log.d(TAG, "Receipt Verification Successful");
        if (storeManager.iabStoreType == StoreManagerType.IAB_GOOGLE)
            iabHelper.consumeAsync(purchase, consumeFinishedListener);
        else if (storeManager.iabStoreType == StoreManagerType.IAB_AMAZON)
            amazonListener.consumePurchase(amazonReceiptId, isRetry);
        else
            return true;
        return false;
    }

    private void verifyErrorResponse() {
        Log.e(TAG, "ROBLOX Billing service is down");
        Utils.sendAnalytics("StoreManager", "PurchaseFailedDueToBillingServiceFailed");
        // Should we consume??
        // We should handle this as a separate case, if billing service is down then we should not consume the purchase
        // But if the Receipt is bad then we should consume
        // Web should give us four states, Billing service down, Bogus Receipt, Purchase Already Consumed and OK Receipt.
        // Currently they give us only OK & Error.

        //mInAppBillingHelper.consumeAsync(mPurchase, null);
        //mAmazonListener.consumePurchase(mAmazonReceiptId);
        //mStoreMgr.resetPurchaseData();
    }

    private static void verifyBogusResponse(StoreManager storeManager, IabHelper iabHelper, Purchase purchase, OnConsumeFinishedListener consumeFinishedListener, AmazonPurchasingListener amazonListener, boolean isRetry) {
        Log.e(TAG, "Bogus Amazon Receipt");
        Utils.sendAnalytics("StoreManager", "BogusAmazonReceiptDetected");
        if (storeManager.iabStoreType == StoreManagerType.IAB_GOOGLE)
            iabHelper.consumeAsync(purchase, consumeFinishedListener);
        else if (storeManager.iabStoreType == StoreManagerType.IAB_AMAZON)
            amazonListener.consumePurchase("", isRetry);
    }
    
    public void destroy()
    {
    	if (mGoogleIABHelper != null) mGoogleIABHelper.dispose();
		mGoogleIABHelper = null;
		storeMgr = null;
    }

    public class AmazonPurchasingListener implements PurchasingListener {
        private static final String TAG = "AmazonPurchasingListener";

        private String currentUserId = null;
        private String currentMarketplace = null;
        private boolean rvsProductionMode = false;

        public AmazonPurchasingListener() {
            rvsProductionMode = !PurchasingService.IS_SANDBOX_MODE;
            iabStoreType = StoreManagerType.IAB_AMAZON;
        }

        public void onUserDataResponse(final UserDataResponse response)
        {
            final UserDataResponse.RequestStatus status = response.getRequestStatus();

            switch(status)
            {
                case SUCCESSFUL:
                    currentUserId = response.getUserData().getUserId();
                    currentMarketplace = response.getUserData().getMarketplace();
                    iabStoreType = StoreManagerType.IAB_AMAZON;

                    break;

                case FAILED:
                case NOT_SUPPORTED:
                    iabStoreType = StoreManagerType.IAB_NONE;
                    // Fail gracefully.
                    break;
            }
        }

        public void onProductDataResponse(final ProductDataResponse response)
        {
            switch (response.getRequestStatus())
            {
                case SUCCESSFUL:
                    for (final String s : response.getUnavailableSkus())
                    {
                        printLogMessage("Unavailable SKU:" + s);
                    }

                    final Map<String, Product> products = response.getProductData();
                    for (final String key : products.keySet())
                    {
                        Product product = products.get(key);
                        printLogMessage(String.format("Product: %s\n Type: %s\n SKU: %s\n Price: %s\n Description: %s\n", product.getTitle(), product.getProductType(), product.getSku(), product.getPrice(), product.getDescription()));
                    }
                    break;

                case FAILED:
                    printLogMessage("ProductDataRequestStatus: FAILED");
                    break;
            }
        }

        public void onPurchaseUpdatesResponse(final PurchaseUpdatesResponse response)
        {
            printLogMessage("Purchase Update Response");
            String pendingAmazonPurchaseRobloxUserName = mKeyValues.getString(PENDING_PURCHASE_ROBLOX_USER_NAME, "");
            switch (response.getRequestStatus())
            {
                case SUCCESSFUL:
                {
                    for (final Receipt receipt : response.getReceipts()) {
                        // ...
                        String receiptId = receipt.getReceiptId();
                        String userId = response.getUserData().getUserId();
                        printLogMessage("Amazon: receiptID: " + receiptId + "userID: " + userId);

                        // Grant only if the current logged in Roblox User Name is not empty.
                        // The Earlier Purchase initiated Roblox User Name matches the current logged in user Name
                        // Or we lost the earlier purchase initiated Roblox User Name and then teh current logged in Roblox User Name is not empty
                        if (!mUserName.isEmpty() && (pendingAmazonPurchaseRobloxUserName.equals(mUserName) || pendingAmazonPurchaseRobloxUserName.isEmpty())) {
                            printLogMessage("Sending Retry Receipt for verification: receiptID: " + receiptId + "userID: " + userId);
                            launchVerifyPurchaseReceipt(mActivity, storeMgr, null, null, true, null, receiptId, userId, this);
                        } else {
                            // Store the values if the Roblox User is not logged in, there can be any scenario
                            SharedPreferences.Editor editor = mKeyValues.edit();
                            editor.putString(PENDING_PURCHASE_AMAZON_USER_ID, userId);
                            editor.putString(PENDING_PURCHASE_AMAZON_RECEIPT_ID, receiptId);
                            editor.apply();
                        }
                    }
                    return;
                }

                case FAILED:
                {
                    printLogMessage("On Purchase Update Response Status Failed");
                    return;
                }
            }

            // If amazon fails to give us the receipt see if we have any receipt stored with us which have not been fulfilled
            String pendingAmazonPurchaseUserID = mKeyValues.getString(PENDING_PURCHASE_AMAZON_USER_ID, "");
            String pendingAmazonPurchaseReceiptID = mKeyValues.getString(PENDING_PURCHASE_AMAZON_RECEIPT_ID, "");
            // Grant only if the current logged in Roblox User Nam, Amazon Receipt ID & Amazon User ID is not empty.
            // The Earlier Purchase initiated Roblox User Name matches the current logged in user Name
            // Or we lost the earlier purchase initiated Roblox User Name and then the current logged in Roblox User Name is not empty
            if (!mUserName.isEmpty() && !pendingAmazonPurchaseReceiptID.isEmpty() && !pendingAmazonPurchaseUserID.isEmpty()
                    && (pendingAmazonPurchaseRobloxUserName.equals(mUserName) || pendingAmazonPurchaseRobloxUserName.isEmpty()))
            {
                printLogMessage("Sending Retry Receipt for verification: receiptID: " + pendingAmazonPurchaseReceiptID + "userID: " + pendingAmazonPurchaseUserID);
                launchVerifyPurchaseReceipt(mActivity, storeMgr, null, null, true, null, pendingAmazonPurchaseReceiptID, pendingAmazonPurchaseUserID, this);
            }
        }

        public void onPurchaseResponse(final PurchaseResponse response)
        {
            final PurchaseResponse.RequestStatus status = response.getRequestStatus();

            if (status == PurchaseResponse.RequestStatus.SUCCESSFUL)
            {
                Receipt receipt = response.getReceipt();
                String receiptId = receipt.getReceiptId();
                String userId = response.getUserData().getUserId();


                SharedPreferences.Editor editor = mKeyValues.edit();
                editor.putString(PENDING_PURCHASE_ROBLOX_USER_NAME, mUserName);
                editor.apply();

                if (!mUserName.isEmpty()) {
                    printLogMessage("Sending Fresh Receipt for verification: receiptID: " + receiptId + "userID: " + userId);
                    launchVerifyPurchaseReceipt(mActivity, storeMgr, null, null, false, null, receiptId, userId, this);
                }

            }
            else
                ActivityGlView.inGamePurchaseFinished(false,mPlayerPtr,mProductId);
        }

        public void consumePurchase(String receiptId, boolean
                isRetry)
        {
            printLogMessage("Consuming Receipt: receiptID: " + receiptId);
            if (!receiptId.isEmpty()) {
                printLogMessage("Fulfilled: receiptID: " + receiptId);
                PurchasingService.notifyFulfillment(receiptId, FulfillmentResult.FULFILLED);
                if (!isRetry) Utils.alertExclusively("Purchase successful, your product will be delivered shortly.");
                ActivityGlView.inGamePurchaseFinished(true,mPlayerPtr,mProductId);
            }
            else
                ActivityGlView.inGamePurchaseFinished(false,mPlayerPtr,mProductId);

            resetPurchaseData();
            printLogMessage("End consumption flow.");
        }
    } //AmazonPurchasingListener

}
