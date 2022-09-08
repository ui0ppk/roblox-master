//
//  StandaloneAppStore.m
//  RobloxMobile
//
//  Created by Ben Tkacheff on 5/2/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "StandaloneAppStore.h"
#import "UserInfo.h"

#include "v8datamodel/DataModel.h"
#include "v8datamodel/MarketplaceService.h"
#include "Network/Players.h"

@implementation StandaloneAppStore

- (id) init
{
    if(self = [super init])
    {
        [[SKPaymentQueue defaultQueue] addTransactionObserver:self];
    }
    return self;
}

+(StandaloneAppStore*) getStandaloneAppStore
{
    static dispatch_once_t standaloneStorePred = 0;
    __strong static StandaloneAppStore* _sharedAppStore = nil;
    dispatch_once(&standaloneStorePred, ^{ // Need to use GCD for thread-safe allocation of singleton
        _sharedAppStore = [[self alloc] init];
    });
    return _sharedAppStore;
}

-(void) resetCurrentInfo
{
    currentProductId = @"";
    currentPlayerPurchasing.reset();
}

-(NSString*)base64forData:(NSData*)theData
{
    const uint8_t* input = (const uint8_t*)[theData bytes];
    NSInteger length = [theData length];
    
    static char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
    
    NSMutableData* data = [NSMutableData dataWithLength:((length + 2) / 3) * 4];
    uint8_t* output = (uint8_t*)data.mutableBytes;
    
    NSInteger i;
    for (i=0; i < length; i += 3) {
        NSInteger value = 0;
        NSInteger j;
        for (j = i; j < (i + 3); j++) {
            value <<= 8;
            
            if (j < length) {
                value |= (0xFF & input[j]);
            }
        }
        
        NSInteger theIndex = (i / 3) * 4;
        output[theIndex + 0] =                    table[(value >> 18) & 0x3F];
        output[theIndex + 1] =                    table[(value >> 12) & 0x3F];
        output[theIndex + 2] = (i + 1) < length ? table[(value >> 6)  & 0x3F] : '=';
        output[theIndex + 3] = (i + 2) < length ? table[(value >> 0)  & 0x3F] : '=';
    }
    
    return [[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding];
}

-(NSString*) getReceiptiOS7AndAbove
{
    //Get the receipt URL
    NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
    
    //Check if it exists
    if ([[NSFileManager defaultManager] fileExistsAtPath:receiptURL.path])
    {
        //Encapsulate the base64 encoded receipt on NSData
        NSData *receiptData = [NSData dataWithContentsOfFile:receiptURL.path];
        NSString *base64Receipt = [self base64forData:receiptData];
        
        return base64Receipt;
    }
    
    return @"";
}

// SKPaymentTransactionObserver
- (void) paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
    for (SKPaymentTransaction *transaction in transactions)
    {
        switch (transaction.transactionState)
        {
            case SKPaymentTransactionStatePurchased:
            {
                NSString* receiptString = @"";
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                if (floor(NSFoundationVersionNumber) <= NSFoundationVersionNumber_iOS_6_1)
                {
                    NSData *receiptData = [NSData dataWithData:transaction.transactionReceipt];
                    receiptString = [self base64forData:receiptData];
                }
#pragma GCC diagnostic pop
                else
                {
                    receiptString = [self getReceiptiOS7AndAbove];
                }
                
                [queue finishTransaction: transaction];
                [self signalMarketplacePurchaseFinished:currentPlayerPurchasing.lock() productId:[currentProductId UTF8String] receipt:[receiptString UTF8String] purchased:true];
                [self resetCurrentInfo];
                break;
            }
            case SKPaymentTransactionStateFailed:
            {
                [queue finishTransaction: transaction];
                [self signalMarketplacePurchaseFinished:currentPlayerPurchasing.lock() productId:[currentProductId UTF8String] receipt:"" purchased:false];
                [self resetCurrentInfo];
                break;
            }
            case SKPaymentTransactionStateRestored:
            default:
            {
                break;
            }
        }
    }
}

// SKPaymentTransactionObserver
- (void) paymentQueue:(SKPaymentQueue *)queue updatedDownloads:(NSArray *)downloads
{
    // don't need anything here yet
}

// SKProductsRequestDelegate
- (void) productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
    if ([self canMakePurchase])
    {
        NSInteger numProduct = [response.products count];
        SKProduct* product = [response.products lastObject];
        if (numProduct > 0 && product) // we can purchase
        {
            SKPayment* payment = [SKPayment paymentWithProduct:product];
            [[SKPaymentQueue defaultQueue] addPayment:payment];
        }
        else
        {
            [self signalMarketplacePurchaseFinished:currentPlayerPurchasing.lock() productId:[currentProductId UTF8String] receipt:"" purchased:false];
            [self resetCurrentInfo];
        }
    }
    else
    {
        [self signalMarketplacePurchaseFinished:currentPlayerPurchasing.lock() productId:[currentProductId UTF8String] receipt:"" purchased:false];
        [self resetCurrentInfo];
    }
}
// SKProductsRequestDelegate
- (void)request:(SKRequest *)errorRequest didFailWithError:(NSError *)error
{
    NSLog(@"we have request error with %@",[error localizedDescription]);
    [self signalMarketplacePurchaseFinished:currentPlayerPurchasing.lock() productId:[currentProductId UTF8String] receipt:"" purchased:false];
    if (request && errorRequest == request)
    {
        request = nil;
    }
    [self resetCurrentInfo];
}
// SKProductsRequestDelegate
- (void)requestDidFinish:(SKRequest *)finishedRequest
{
    if (request && finishedRequest == request)
    {
        request = nil;
    }
}

-(void) requestProductData:(NSString* )productId
{
    NSSet* set = [NSSet setWithObject:productId];
    request = [[SKProductsRequest alloc] initWithProductIdentifiers:set];
    request.delegate = self;
    [request start];
}

- (BOOL) canMakePurchase
{
    [[UserInfo CurrentPlayer] UpdatePlayerInfo];
    return [SKPaymentQueue canMakePayments];
}

-(void) purchaseProduct:(NSString*)productId player:(shared_ptr<RBX::Instance>) player
{
    if (![self canMakePurchase])
    {
        NSLog(@"Account not allowed to make purchases");
        [self signalMarketplacePurchaseFinished:player productId:[productId UTF8String] receipt:"" purchased:false];
    }
    else if (shared_ptr<RBX::Instance> sharedPlayer = currentPlayerPurchasing.lock())
    {
        NSLog(@"Currently trying to purchase something, then asked to purchase another thing in the middle of it");
        [self signalMarketplacePurchaseFinished:player productId:[productId UTF8String] receipt:"" purchased:false];
    }
    else
    {
        currentPlayerPurchasing = player;
        currentProductId = productId;
        [self requestProductData:productId];
    }
    
}

-(void) signalMarketplacePurchaseFinished:(shared_ptr<RBX::Instance>) player productId:(std::string) productId receipt:(std::string) receipt purchased:(bool) purchased
{
    if (RBX::MarketplaceService* marketService = RBX::ServiceProvider::find<RBX::MarketplaceService>(player.get()))
    {
        marketService->signalPromptThirdPartyPurchaseFinished(player, productId, receipt, purchased);
    }
}

-(void) promptThirdPartyPurchase:(shared_ptr<RBX::Instance>) player productId:(std::string) productId
{
    if (!player)
        return;
    
    if (RBX::Network::Player* thePlayer = RBX::Instance::fastDynamicCast<RBX::Network::Player>(player.get()))
    {
        RBX::Network::Player* localPlayer = RBX::Network::Players::findLocalPlayer(RBX::DataModel::get(player.get()));
        if (localPlayer == thePlayer)
        {
            [self purchaseProduct:[NSString stringWithUTF8String:productId.c_str()] player:player];
        }
        else
        {
            [self signalMarketplacePurchaseFinished:player productId:productId receipt:"" purchased:false];
        }
    }
    else
    {
        [self signalMarketplacePurchaseFinished:player productId:productId receipt:"" purchased:false];
    }
}

@end
