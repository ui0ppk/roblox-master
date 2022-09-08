//
//  StoreManager.m
//  newapp
//
//  Created by Dongwei Liao on 9/5/12.
//  Copyright (c) 2012 roblox. All rights reserved.
//

#import <Foundation/NSJSONSerialization.h>
#import "AppDelegate.h"
#import "StoreManager.h"
#import "RobloxAlert.h"
#import "UserInfo.h"
#import "RobloxInfo.h"
#import "RobloxGoogleAnalytics.h"
#import "iOSSettingsService.h"
#import "Reachability.h"
#import "RobloxWebUtility.h"
#import "RobloxNotifications.h"
#import "PlaceLauncher.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/MarketplaceService.h"
#include "Network/Players.h"

#define NUM_RETRIES_BEFORE_INFORM_USER 10
#define TOTAL_NUM_RETRIES 200

@implementation StoreManager


- (id) init
{
    if(self = [super init])
    {
        // set these to some default value (should get them from app settings pretty quick though)
        timeIntervalBetweenBCPurchaseInMinutes = 5;
        timeIntervalBetweenCatalogPurchaseInMinutes = 5;
        timeLimitForRetryOfBillingServiceBeforeGivingUp = 20;
        allowAppleInAppPurchase = true;
        readInAppPurchaseSettingsBeforeEveryPurchase = false;
        
#ifdef NATIVE_IOS
        timeIntervalBetweenRobuxPurchaseInMinutes = 0;
#else
        timeIntervalBetweenRobuxPurchaseInMinutes = 5;
#endif
        
        
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
            iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
#ifdef NATIVE_IOS
            timeIntervalBetweenRobuxPurchaseInMinutes = iosSettings->GetValueRobloxHDTimeIntervalBetweenRobuxPurchaseInMinutes();
#else
            timeIntervalBetweenRobuxPurchaseInMinutes = iosSettings->GetValueTimeIntervalBetweenRobuxPurchaseInMinutes();
#endif
            
            timeIntervalBetweenBCPurchaseInMinutes = iosSettings->GetValueTimeIntervalBetweenBCPurchaseInMinutes();
            timeIntervalBetweenCatalogPurchaseInMinutes = iosSettings->GetValueTimeIntervalBetweenCatalogPurchaseInMinutes();
            timeLimitForRetryOfBillingServiceBeforeGivingUp = iosSettings->GetValueTimeLimitForBillingServiceRetriesBeforeGivingUp();
            allowAppleInAppPurchase = iosSettings->GetValueAllowAppleInAppPurchase();
            readInAppPurchaseSettingsBeforeEveryPurchase = iosSettings->GetValueReadInAppPurchaseSettingsBeforeEveryPurchase();
        });
        
        retries = 0;
        
        [self resetPlayerPurchasing];
        
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(gotDidLeaveGameNotification:) name:RBX_NOTIFY_GAME_DID_LEAVE object:nil ];
        
        [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
    }
    return self;
}

// Initialize this after Login, do not initialize on Application Start up, the purchase should go to Roblox Authenticated User
+ (StoreManager*)getStoreMgr
{
    static dispatch_once_t storePred = 0;
    __strong static StoreManager* _sharedStoreManager = nil;
    dispatch_once(&storePred, ^{ // Need to use GCD for thread-safe allocation of singleton
        _sharedStoreManager = [[self alloc] init];
    });
    return _sharedStoreManager;
}

-(void) signalMarketplacePurchaseFinished:(bool) purchased
{
    if (playerPurchasing.get() && !productIdPurchasing.empty())
    {
        if (RBX::MarketplaceService* marketService = RBX::ServiceProvider::find<RBX::MarketplaceService>(playerPurchasing.get()))
        {
            marketService->signalPromptNativePurchaseFinished(playerPurchasing, productIdPurchasing, purchased);
            [self resetPlayerPurchasing];
        }
    }
}

- (BOOL) canMakePurchase
{
    [[UserInfo CurrentPlayer] UpdatePlayerInfo];
    return [SKPaymentQueue canMakePayments];
}

-(void) resetPlayerPurchasing
{
    playerPurchasing = shared_ptr<RBX::Instance>();
    productIdPurchasing = "";
}

- (void) request:(SKRequest*)request didFailWithError:(NSError *)error
{
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"StoreManager: Could not contact App Store properly because %s", [[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding]);
}


- (void) requestDidFinish:(SKRequest*)request
{
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"StoreManager: Request finished");
}

// Restrict Time Bound Purchase only for Robux & BC Purchases, all catalog purchases should go through without restrictions.
-(BOOL) restrictTimeBoundPurchase:(NSString *)productId
{
    if (readInAppPurchaseSettingsBeforeEveryPurchase)
    {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0), ^{
            iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
#ifdef NATIVE_IOS
            timeIntervalBetweenRobuxPurchaseInMinutes = iosSettings->GetValueRobloxHDTimeIntervalBetweenRobuxPurchaseInMinutes();
#else
            timeIntervalBetweenRobuxPurchaseInMinutes = iosSettings->GetValueTimeIntervalBetweenRobuxPurchaseInMinutes();
#endif
            
            timeIntervalBetweenBCPurchaseInMinutes = iosSettings->GetValueTimeIntervalBetweenBCPurchaseInMinutes();
            timeIntervalBetweenCatalogPurchaseInMinutes = iosSettings->GetValueTimeIntervalBetweenCatalogPurchaseInMinutes();
            timeLimitForRetryOfBillingServiceBeforeGivingUp = iosSettings->GetValueTimeLimitForBillingServiceRetriesBeforeGivingUp();
            allowAppleInAppPurchase = iosSettings->GetValueAllowAppleInAppPurchase();
            readInAppPurchaseSettingsBeforeEveryPurchase = iosSettings->GetValueReadInAppPurchaseSettingsBeforeEveryPurchase();
        });
    }
    
    if(!allowAppleInAppPurchase)
        return NO;
    
    BOOL retVal = YES;
    NSTimeInterval currentTime  = [[NSDate date] timeIntervalSince1970];
    
    if([productId hasSuffix:@"Robux"])
    {
        NSTimeInterval lastPurchaseTimeRobux = [[NSUserDefaults standardUserDefaults] doubleForKey:@"LastPurchaseTimeRobux"];
        // Block Robux Purchase for 10 minutes between purchases
        if(lastPurchaseTimeRobux != 0 && currentTime  < (lastPurchaseTimeRobux + 60 * timeIntervalBetweenRobuxPurchaseInMinutes))
            retVal = NO;
    }
    else  if([productId hasSuffix:@"monthBC"] || [productId hasSuffix:@"monthOBC"] || [productId hasSuffix:@"monthTBC"])
    {
        NSTimeInterval lastPurchaseTimeBC = [[NSUserDefaults standardUserDefaults] doubleForKey:@"LastPurchaseTimeBC"];
        
        // Block BC Purchases for 24 hours between purchases
        if(lastPurchaseTimeBC != 0 && currentTime  < (lastPurchaseTimeBC + 60 * timeIntervalBetweenBCPurchaseInMinutes))
            retVal = NO;
    }
    else // This is a Catalog purchase
    {
        NSTimeInterval lastPurchaseTimeCatalog = [[NSUserDefaults standardUserDefaults] doubleForKey:productId];
        
        // Block Individual Catalog Purchases for 10 Minutes between purchases
        if(lastPurchaseTimeCatalog != 0 && currentTime  < (lastPurchaseTimeCatalog + 60 * timeIntervalBetweenCatalogPurchaseInMinutes))
            retVal = NO;
    }
    
    return retVal;
}

-(void) reset
{
    retries = 0;
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"PendingTransactionUserName"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"BillingServiceRetries"];
    [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RecordedTimeForBillingServiceFirstFailure"];
    [[NSUserDefaults standardUserDefaults] synchronize];
    
}

// Record the Purchase Time,s o that we can restrict Time Bound Purchase with ios Client Settings adjustments.
-(void) recordPurchaseTime:(NSString *)productId
{
    NSTimeInterval currentTime  = [[NSDate date] timeIntervalSince1970];
    
    if([productId hasSuffix:@"Robux"])
        [[NSUserDefaults standardUserDefaults] setDouble:currentTime forKey:@"LastPurchaseTimeRobux"];
    else  if([productId hasSuffix:@"monthBC"] || [productId hasSuffix:@"monthOBC"] || [productId hasSuffix:@"monthTBC"])
        [[NSUserDefaults standardUserDefaults] setDouble:currentTime forKey:@"LastPurchaseTimeBC"];
    else // This is a Catalog Purchase
        [[NSUserDefaults standardUserDefaults] setDouble:currentTime forKey:productId];
        
    [[NSUserDefaults standardUserDefaults] synchronize];
    
}



-(void) productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    
    if ([self canMakePurchase])
    {
        NSInteger numProduct = [response.products count];
        SKProduct* product_ = [response.products lastObject];
        
        if (!product_)
        {
            [self signalMarketplacePurchaseFinished:false];
            
            [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"Attempt" withLabel:@"NoProduct" withValue:0];
            [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureAppStore" withLabel:@"NoProduct" withValue:0];
            NSString *message = [NSString stringWithFormat:@"Num products: %ld, no products found", (long)numProduct];
            RBX::StandardOut::singleton()->printf(RBX::MESSAGE_WARNING,"StoreManager: %s",[message cStringUsingEncoding:NSUTF8StringEncoding]);
            [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"CannotMakePurchase", nil)];
        }
        else
        {
            NSInteger numTransaction = [[[SKPaymentQueue defaultQueue] transactions] count];
            
            if(numTransaction > 0)
            {
                SKPaymentTransaction *transaction = [[[SKPaymentQueue defaultQueue] transactions] objectAtIndex:0];
                [self completeTransaction:transaction];
                return;
            }
            
            [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"Attempt" withLabel:product_.productIdentifier withValue:0];
            NSString* pendingTransUserName = [UserInfo CurrentPlayer].username;
            if([pendingTransUserName length] != 0)
            {
                RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"StoreManager: Product ID: %s", [product_.productIdentifier cStringUsingEncoding:NSUTF8StringEncoding]);
                BOOL proceed = [self restrictTimeBoundPurchase:product_.productIdentifier];
                
                if(proceed == YES)
                {
                    SKPayment* payment = [SKPayment paymentWithProduct:product_];
                    [[SKPaymentQueue defaultQueue] addPayment:payment];
                    
                    pendingTransUserName = [UserInfo CurrentPlayer].username;
                    [[NSUserDefaults standardUserDefaults] setObject:pendingTransUserName forKey:@"PendingTransactionUserName"];
                    [[NSUserDefaults standardUserDefaults] synchronize];
                }
                else
                {
                    [self signalMarketplacePurchaseFinished:false];
                    
                    if([product_.productIdentifier hasSuffix:@"Robux"])
                        [RobloxAlert RobloxAlertWithMessage:[NSString stringWithFormat:NSLocalizedString(@"WaitForAnotherRobuxPurchase", nil)]];
                    else  if([product_.productIdentifier hasSuffix:@"monthBC"] || [product_.productIdentifier hasSuffix:@"monthOBC"] || [product_.productIdentifier hasSuffix:@"monthTBC"])
                        [RobloxAlert RobloxAlertWithMessage:[NSString stringWithFormat:NSLocalizedString(@"WaitForAnotherBCPurchase", nil)]];
                    else // Catalog Purchase
                        [RobloxAlert RobloxAlertWithMessage:[NSString stringWithFormat:NSLocalizedString(@"WaitForAnotherCatalogPurchase", nil)]];
                    
                    [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureAppStore" withLabel:@"WaitForAnotherPurchaseTimeDelay" withValue:0];
                }
            }
            else
            {
                [self signalMarketplacePurchaseFinished:false];
                
                NSString *message = [NSString stringWithFormat:NSLocalizedString(@"SessionExpired", nil)];
                [RobloxAlert RobloxAlertWithMessage:message];
                [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureAppStore" withLabel:product_.productIdentifier withValue:0];
            }
        }
    }
    else
    {
        [self signalMarketplacePurchaseFinished:false];
        
        [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"CannotPurchaseBecauseParentalControl", nil)];
    }
    
}


-(void) requestProductData:(NSString* )productId
{
    NSSet* set = [NSSet setWithObject:productId];
    SKProductsRequest* request = [[SKProductsRequest alloc] initWithProductIdentifiers:set];
    request.delegate = self;
    [request start];
}


-(void) purchaseProduct:(NSString*)productId
{
    if (![self canMakePurchase])
    {
        [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"CannotPurchaseBecauseParentalControl", nil)];
    }
    else
    {
        [self requestProductData:productId];
    }
}

enum
{
    Transaction_PendingMatchesCurrentUser = 0,
    Transaction_PendingNotMatchesCurrentUser,
    Transaction_PendingUsernameLost,
};

-(int) verifyIfCorrectUser
{
    if (![[NSUserDefaults standardUserDefaults] objectForKey:@"PendingTransactionUserName"])
    {
        return Transaction_PendingUsernameLost;
    }
    else if ([[[NSUserDefaults standardUserDefaults] stringForKey:@"PendingTransactionUserName"] isEqualToString:@""])
    {
        return Transaction_PendingUsernameLost;
    }
    
    NSString *userNameOfPendingTransaction = [[NSUserDefaults standardUserDefaults] stringForKey:@"PendingTransactionUserName"];
    
    UserInfo* user = [UserInfo CurrentPlayer];
    NSString *userNameOfCurrentLoggedInUser = user.username;
    
    if(![user userLoggedIn])// It means the user is logged out.
        userNameOfCurrentLoggedInUser = @"";
    
    if ([userNameOfCurrentLoggedInUser isEqualToString:userNameOfPendingTransaction])
        return Transaction_PendingMatchesCurrentUser;
    
    return Transaction_PendingNotMatchesCurrentUser;
    
}

// This is called whenevr we have unfinished transaction in the Apple Store Queue
// Also called when we do retries in case of Roblox Billing Service failure.
-(void) completeTransaction:(SKPaymentTransaction*)transaction
{
    if (transaction == nil)
    {
        return;
    }
    
    Reachability* reachability = [Reachability reachabilityForInternetConnection];
    NetworkStatus remoteStatus = [reachability currentReachabilityStatus];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    NSData* receipt = transaction.transactionReceipt;
#pragma clang diagnostic pop

    if (receipt == nil) // Apple Bug: iOS sometimes will give you the nil receipt even though Queue is empty, exit gracefully
        return;

    if (remoteStatus == NotReachable)
    {
        [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"ConnectionError", nil)];
        [self performSelector:@selector(completeTransaction:) withObject:transaction afterDelay:60*10];
        return;
    }
    
    
    int transactionUserState = [self verifyIfCorrectUser];
    if (transactionUserState == Transaction_PendingMatchesCurrentUser)
    {
        [self verifyReceipt:receipt forProductId:transaction.payment.productIdentifier paymentTransaction:transaction paymentQueue:[SKPaymentQueue defaultQueue]];
    }
    else if (transactionUserState == Transaction_PendingNotMatchesCurrentUser)
    {
        NSTimeInterval currentTime  = [[NSDate date] timeIntervalSince1970];
        NSTimeInterval timeofFirstFailure = [[NSUserDefaults standardUserDefaults] doubleForKey:@"RecordedTimeForBillingServiceFirstFailure"];
        
        // If Time elapsed is more than than the retries limit set with billing service, grant the current user the item.
        if(timeofFirstFailure != 0 && currentTime  > (timeofFirstFailure + 60 * 60 * timeLimitForRetryOfBillingServiceBeforeGivingUp))
        {
            [self verifyReceipt:receipt forProductId:transaction.payment.productIdentifier paymentTransaction:transaction paymentQueue:[SKPaymentQueue defaultQueue]];
        }
        else
        {
            [self performSelector:@selector(completeTransaction:) withObject:transaction afterDelay:60*10];
        }
    }
    else if (transactionUserState == Transaction_PendingUsernameLost)
    {
        if ([[UserInfo CurrentPlayer] userLoggedIn])
        {
            [self verifyReceipt:receipt forProductId:transaction.payment.productIdentifier paymentTransaction:transaction paymentQueue:[SKPaymentQueue defaultQueue]];
        }
        else
        {
            [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"AnyUserNeedsToSignIn", nil)];
            [self performSelector:@selector(completeTransaction:) withObject:transaction afterDelay:60*10];
        }
    }
}

// Transaction probably verified with Roblox Billing Service
// Finish the Transaction from Apple Store Kit if successful verification
// Otherwise keep retrying with Roblox billing service calls
-(void) endTransaction: (ReceiptStatus) verified paymentTransaction:(SKPaymentTransaction*) transaction paymentQueue:(SKPaymentQueue*) paymentQueue
{
    if (verified == ReceiptStatus::ok)
    {
        [paymentQueue finishTransaction: transaction];
        [self reset];
        [self signalMarketplacePurchaseFinished:true];
        
        // Record the purchase time of the successful transaction
        [self recordPurchaseTime:transaction.payment.productIdentifier];
    
        [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"Success" withLabel:transaction.payment.productIdentifier withValue:0];
    
        if (retries)
            [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"SuccessBillingServiceAfterRetry" withLabel:transaction.payment.productIdentifier withValue:retries];
        else
        {
            NSInteger retriesRecorded = [[NSUserDefaults standardUserDefaults] integerForKey:@"BillingServiceRetries"];
            if (retriesRecorded)
                [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"SuccessBillingServiceAfterRelaunch" withLabel:transaction.payment.productIdentifier withValue:1];
        }
    }
    else if(verified == ReceiptStatus::bogus)
    {
        [paymentQueue finishTransaction: transaction];
        [self reset];
        [self signalMarketplacePurchaseFinished:false];
        [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"Bogus" withLabel:transaction.payment.productIdentifier withValue:0];
    }
    else
    {
        retries++;
        if(retries == 1)
        {
            
            NSInteger retriesRecorded = [[NSUserDefaults standardUserDefaults] integerForKey:@"BillingServiceRetries"];
            if (retriesRecorded)
            {
                [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureBillingServiceAfterRelaunch" withLabel:transaction.payment.productIdentifier withValue:1];
            }
            else
            {
                [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureBillingService1stRetry" withLabel:transaction.payment.productIdentifier withValue:1];
                
                NSTimeInterval currentTime  = [[NSDate date] timeIntervalSince1970];
                [[NSUserDefaults standardUserDefaults] setDouble:currentTime forKey:@"RecordedTimeForBillingServiceFirstFailure"];
            }
        }
        else
            [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureBillingServiceMultipleRetry" withLabel:transaction.payment.productIdentifier withValue:retries];

        [[NSUserDefaults standardUserDefaults] setInteger:retries forKey:@"BillingServiceRetries"];
        
        // Retry reached 10 attempts, inform user that his transaction is not working, and keep trying the transaction.
        if (retries == NUM_RETRIES_BEFORE_INFORM_USER)
        {
            dispatch_sync(dispatch_get_main_queue(), ^{
                [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"TransactionDelayedBody", nil)];
                [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureBillingServiceTooManyRetries" withLabel:transaction.payment.productIdentifier withValue:1];
                
            });
        }
        
        
        // We will keep retrying for 200 times with increamenting the interval by 10 seconds on each failure
        // Will give up retrying in 48 hours as recorded with 1st retry
        if(retries <= TOTAL_NUM_RETRIES)
        {
            
            NSTimeInterval currentTime  = [[NSDate date] timeIntervalSince1970];
            NSTimeInterval timeofFirstFailure = [[NSUserDefaults standardUserDefaults] doubleForKey:@"RecordedTimeForBillingServiceFirstFailure"];
            
            // Try to make the billing service attempt for 48 hours, can be changed with client settings
            if(timeofFirstFailure != 0 && currentTime  < (timeofFirstFailure + 60 * 60 * timeLimitForRetryOfBillingServiceBeforeGivingUp))
            {
                
#ifdef _DEBUG
                // Do not keep retrying in DEBUG build, just clear out the transaction if it failed on the 1st attempt.
                [paymentQueue finishTransaction: transaction];
                [self reset];
                return;
#endif
                
                dispatch_sync(dispatch_get_main_queue(), ^{
                    [self performSelector:@selector(completeTransaction:) withObject:transaction afterDelay:10*retries];
                });
            }
            else// If this is a bogus receipt, we want to remove the transaction after 48 hours
            {
                
                dispatch_sync(dispatch_get_main_queue(), ^{
        
                    [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"TransactionFailedBody", nil)];
                    [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureBillingServiceTooManyRetries" withLabel:transaction.payment.productIdentifier withValue:1];
                    
                });
                
                [self signalMarketplacePurchaseFinished:false];
                [paymentQueue finishTransaction: transaction];
                [self reset];
                                
                [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureDueToBogusReceipt" withLabel:transaction.payment.productIdentifier withValue:0];
            }
        }
    }
    
}

// Transaction not yet initiated, it failed before being charged, it can be user clicked Cancel, or incorect password, etc
-(void) failedTransaction:(SKPaymentTransaction*)transaction
{
#ifndef NDEBUG
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"StoreManager: User cancelled transaction: %s", [[transaction description] cStringUsingEncoding:NSUTF8StringEncoding]);
#endif
    
    [[SKPaymentQueue defaultQueue] finishTransaction: transaction];
    [self reset];
    [self signalMarketplacePurchaseFinished:false];
    
    if (transaction.transactionState == SKPaymentTransactionStateFailed)
    {
        NSError *err = transaction.error;
        NSString *desc = [err description];
        NSLog(desc, nil);
        
        if ([err code] == 2)
            [RobloxGoogleAnalytics setEventTracking:@"Purchase" withAction:@"FailureAppStoreDueToCancelOrUserAuthentication" withLabel:transaction.payment.productIdentifier withValue:0];
    }
}


-(void) restoreTransaction:(SKPaymentTransaction*)transaction
{
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"StoreManager: restore transaction ...");
    
    [[SKPaymentQueue defaultQueue] finishTransaction: transaction];
}


// SKObserver
-(void) paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
    for (SKPaymentTransaction *transaction in transactions)
    {
        switch (transaction.transactionState)
        {
            case SKPaymentTransactionStatePurchased:
                [self completeTransaction:transaction];
                break;
            case SKPaymentTransactionStateFailed:
                [self failedTransaction:transaction];
                break;
            case SKPaymentTransactionStateRestored:
                [self restoreTransaction:transaction];
            default:
                break;
        }
    }
}


- (NSString *)encode:(const uint8_t *)input length:(NSInteger)length
{
    static char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    
    NSMutableData *data = [NSMutableData dataWithLength:((length + 2) / 3) * 4];
    uint8_t *output = (uint8_t *)data.mutableBytes;
    
    for (NSInteger i = 0; i < length; i += 3) {
        NSInteger value = 0;
        for (NSInteger j = i; j < (i + 3); j++) {
            value <<= 8;
            
			if (j < length) {
                value |= (0xFF & input[j]);
			}
        }
        
        NSInteger index = (i / 3) * 4;
        output[index + 0] =                    table[(value >> 18) & 0x3F];
        output[index + 1] =                    table[(value >> 12) & 0x3F];
        output[index + 2] = (i + 1) < length ? table[(value >> 6)  & 0x3F] : '=';
        output[index + 3] = (i + 2) < length ? table[(value >> 0)  & 0x3F] : '=';
    }
    
    NSString* s = [[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding];
    NSLog(@"This is the current iTunes purchase receipt:\n%@\n", s);
    return s;
}

// Verify the Apple Transaction receit with the Roblox Billing Service
-(void) verifyReceipt:(NSData*)receiptData forProductId:(NSString*)productId paymentTransaction:(SKPaymentTransaction*) transaction paymentQueue:(SKPaymentQueue*) paymentQueue
{
    
    NSString* urlString = [RobloxInfo getWWWBaseUrl];
    
    NSObject *testObject = [[NSUserDefaults standardUserDefaults] objectForKey:@"RecordedTimeForBillingServiceFirstFailure"];
    if (testObject)
        urlString = [urlString stringByAppendingString:@"mobileapi/apple-purchase?isRetry=true"];
    else
        urlString = [urlString stringByAppendingString:@"mobileapi/apple-purchase"];
    
    urlString = [urlString stringByReplacingOccurrencesOfString:@"http:" withString:@"https:"];
    
    
    NSURL *url = [NSURL URLWithString: urlString];
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:url
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60];
    
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    [theRequest setHTTPMethod:@"POST"];
    [theRequest setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    
    NSString *receiptDataString = [self encode:(uint8_t *)receiptData.bytes length:receiptData.length];
    receiptDataString = [receiptDataString stringByReplacingOccurrencesOfString:@"+" withString: @"%2B"];
    NSString *postData = [NSString stringWithFormat:@"receipt=%@&productId=%@", receiptDataString, productId];
    RBX::StandardOut::singleton()->printf(RBX::MESSAGE_INFO,"StoreManager: Receipt Data String: %s", [receiptDataString cStringUsingEncoding:NSUTF8StringEncoding]);
    
    NSString *length = [NSString stringWithFormat:@"%lu", (unsigned long)[postData length]];
    [theRequest setValue:length forHTTPHeaderField:@"Content-Length"];
    
    [theRequest setHTTPBody:[postData dataUsingEncoding:NSASCIIStringEncoding]];
    
    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
    [NSURLConnection sendAsynchronousRequest:theRequest queue:queue completionHandler:
     ^(NSURLResponse *response, NSData *receiptResponseData, NSError *error)
     {
         ReceiptStatus retVal = ReceiptStatus::error;
         NSHTTPURLResponse* urlResponse = ( NSHTTPURLResponse*) response;
         
         if(urlResponse == nil)
             RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"StoreManager: Login failed due to improper cast");
         
         long responseStatusCode = [urlResponse statusCode];
         
         NSString *responseString = [[NSString alloc] initWithData:receiptResponseData encoding:NSASCIIStringEncoding];
         
         if(responseStatusCode == 200 && [responseString isEqualToString: @"OK"])
         {
             retVal = ReceiptStatus::ok;
             // bring up home view or error alert in main synchronous thread (ui stuff must be loaded in main thread)
             dispatch_sync(dispatch_get_main_queue(), ^{
                 [RobloxAlert RobloxAlertWithMessage:NSLocalizedString(@"PurchaseSuccessful", nil)];
#ifdef NATIVE_IOS
                 // this is a bit of a hack for now.
                 //we don't know _exactly_ how long it will take for the server to respond, so we err on the side of caution with 10sec.
                 dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(10 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
                     [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_ROBUX_UPDATED object:nil];
                 });
#endif
             });
         }
         else if(responseStatusCode == 200 && [responseString isEqualToString: @"Bogus"])
             retVal = ReceiptStatus::bogus;
         
        
         [self endTransaction:retVal paymentTransaction:transaction paymentQueue:paymentQueue];
         
     }];
}

-(void) purchasePrecheck:(NSString*)productId completion:(void(^)(BOOL success, NSString* error))handler
{
    NSString* urlString = [NSString stringWithFormat:@"%@mobileapi/validate-mobile-purchase", [RobloxInfo getWWWBaseUrl]];
    
    NSURL *url = [NSURL URLWithString: urlString];
    NSMutableURLRequest *theRequest = [NSMutableURLRequest requestWithURL:url
                                                              cachePolicy:NSURLRequestReloadIgnoringCacheData
                                                          timeoutInterval:60];
    
    [RobloxInfo setDefaultHTTPHeadersForRequest:theRequest];
    [theRequest setHTTPMethod:@"POST"];
    [theRequest setValue:@"application/x-www-form-urlencoded" forHTTPHeaderField:@"Content-Type"];
    
    NSString *postData = [NSString stringWithFormat:@"productId=%@", productId];
    NSString *length = [NSString stringWithFormat:@"%lu", (unsigned long)[postData length]];
    [theRequest setValue:length forHTTPHeaderField:@"Content-Length"];
    [theRequest setHTTPBody:[postData dataUsingEncoding:NSASCIIStringEncoding]];
    
    NSOperationQueue *queue = [[NSOperationQueue alloc] init];
    [NSURLConnection sendAsynchronousRequest:theRequest queue:queue completionHandler:
     ^(NSURLResponse *response, NSData *responseData, NSError *error)
     {
         NSHTTPURLResponse* urlResponse = ( NSHTTPURLResponse*) response;
         
         if(urlResponse == nil)
             RBX::StandardOut::singleton()->printf(RBX::MESSAGE_ERROR,"StoreManager: Purchase precheck failed due to missing response.");
         
         long responseStatusCode = [urlResponse statusCode];
         NSString *responseString = [[NSString alloc] initWithData:responseData encoding:NSASCIIStringEncoding];
         
         if(responseStatusCode == 200)
         {
             if ([responseString isEqualToString: @"OK"])
                 handler(YES, nil);
             
             else if ([responseString isEqualToString: @"Error"])
                 handler(NO, NSLocalizedString(@"PurchasingInvalidPurchase", nil));
             
             else if ([responseString isEqualToString:@"Retry"])
                 handler(NO, NSLocalizedString(@"PurchasingBillingDown", nil));
             
             else if ([responseString isEqualToString:@"Limit"])
                 handler(NO, NSLocalizedString(@"PurchasingLimit", nil));
             
             else
                 handler(NO, NSLocalizedString(@"PurchasingUnknownError", nil));
         }
         else
             handler(NO, NSLocalizedString(@"PurchasingUnknownError", nil));
         
     }];
}
-(BOOL) checkForInAppPurchases:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
    // handle in-app purchases
    if (navigationType == UIWebViewNavigationTypeFormSubmitted)
    {
        [request mainDocumentURL];
    }
    else
    {
        NSURL *url = request.URL;
        NSString *urlLoadStr = url.absoluteString;
        
        NSString* urlBuy = [[RobloxInfo getWWWBaseUrl] stringByAppendingString:@"mobile-app-upgrades/buy"];
        
        if ([urlLoadStr hasPrefix:urlBuy])
        {
            NSArray* tokens = [urlLoadStr componentsSeparatedByCharactersInSet:[NSCharacterSet characterSetWithCharactersInString:@"="]];
            if (tokens.count == 2)
            {
                NSString* id = (NSString*)[tokens objectAtIndex:1];
                [self purchasePrecheck:id completion:^(BOOL success, NSString *errorMessage) {
                    if (success)
                        dispatch_async(dispatch_get_main_queue(), ^{
                            [[StoreManager getStoreMgr] purchaseProduct:id];
                        });
                    else
                        [RobloxAlert RobloxAlertWithMessage:errorMessage];
                }];
                
            }
            return YES;
        }
    }
    return NO;
}

-(void) promptNativePurchase:(shared_ptr<RBX::Instance>) player productId:(std::string) productId
{
    if (!player)
    {
        [self signalMarketplacePurchaseFinished:false];
        return;
    }
    if (productId.empty())
    {
        [self signalMarketplacePurchaseFinished:false];
        return;
    }
    if (productIdPurchasing.compare("") != 0)
    {        [self signalMarketplacePurchaseFinished:false];
        return;
    }
    if (playerPurchasing != shared_ptr<RBX::Instance>())
    {
        [self signalMarketplacePurchaseFinished:false];
        return;
    }
    
    if (RBX::Network::Player* thePlayer = RBX::Instance::fastDynamicCast<RBX::Network::Player>(player.get()))
    {
        RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(RBX::DataModel::get(player.get()));
        if (localPlayer == thePlayer)
        {
            productIdPurchasing = productId;
            playerPurchasing = player;
            
            [[StoreManager getStoreMgr] purchaseProduct:[NSString stringWithUTF8String:productId.c_str()]];
        }
        else
        {
            [self signalMarketplacePurchaseFinished:false];
        }
    }
    else
    {
        [self signalMarketplacePurchaseFinished:false];
    }
}

- (void) gotDidLeaveGameNotification:(NSNotification *)aNotification
{
    [self resetPlayerPurchasing];
}

@end
