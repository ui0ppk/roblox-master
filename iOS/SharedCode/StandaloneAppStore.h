//
//  StandaloneAppStore.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 5/2/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

#include "v8tree/Instance.h"

@interface StandaloneAppStore : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver>
{
    weak_ptr<RBX::Instance> currentPlayerPurchasing;
    NSString* currentProductId;
    SKProductsRequest *request;
}
-(id) init;
// WARNING: DO NOT access this directly!!!!! please use the macro 'GetStoreMgr' instead
+(StandaloneAppStore*) getStandaloneAppStore;

-(void) purchaseProduct:(NSString*)productId player:(shared_ptr<RBX::Instance>) player;
-(void) promptThirdPartyPurchase:(shared_ptr<RBX::Instance>) player productId:(std::string) productId;

-(void) signalMarketplacePurchaseFinished:(shared_ptr<RBX::Instance>) player productId:(std::string) productId receipt:(std::string) receipt purchased:(bool) purchased;

- (void)requestDidFinish:(SKRequest *)finishedRequest;
@end
