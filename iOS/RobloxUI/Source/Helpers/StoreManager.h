//
//  StoreManager.h
//  newapp
//
//  Created by Dongwei Liao on 9/5/12.
//  Copyright (c) 2012 roblox. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>
#import "StandaloneAppStore.h"

// this is used for getting the current store, as we only want
// to use one store for each app, either StoreManager
// which is the RobloxMobile store (does receipt verifying)
// or StandaloneAppStore which is used by standalone apps (no receipt verifying, apps do that)
#ifdef STANDALONE_PURCHASING
#define GetStoreMgr [StandaloneAppStore getStandaloneAppStore]
#else
#define GetStoreMgr [StoreManager getStoreMgr]
#endif

@class UpgradesViewController;
@interface StoreManager : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver>
{
    int retries;
    int timeIntervalBetweenRobuxPurchaseInMinutes;
    int timeIntervalBetweenBCPurchaseInMinutes;
    int timeIntervalBetweenCatalogPurchaseInMinutes;
    int timeLimitForRetryOfBillingServiceBeforeGivingUp;
    bool allowAppleInAppPurchase;
    bool readInAppPurchaseSettingsBeforeEveryPurchase;
    
    shared_ptr<RBX::Instance> playerPurchasing;
    std::string productIdPurchasing;
    enum ReceiptStatus { ok, error, bogus };
}

-(void) signalMarketplacePurchaseFinished:(bool) purchased;
-(void) promptNativePurchase:(shared_ptr<RBX::Instance>) player productId:(std::string) productId;

-(BOOL) checkForInAppPurchases:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType;

-(void) purchaseProduct:(NSString*)productId;
-(BOOL) restrictTimeBoundPurchase:(NSString*)productId;
-(void) recordPurchaseTime:(NSString*)productId;
// WARNING: DO NOT access this directly!!!!! please use the macro 'GetStoreMgr' instead
+(StoreManager*) getStoreMgr;


@end
