//
//  RBPurchaseConsumableViewController.h
//  RobloxMobile
//
//  Created by alichtin on 10/1/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"
#include "RBModalPopUpViewController.h"

@interface RBConfirmPurchaseViewController : RBModalPopUpViewController

@property(strong, nonatomic) NSString* thumbnailAssetID;
@property(strong, nonatomic) NSString* productName;
@property(nonatomic) NSUInteger productID;
@property(nonatomic) NSUInteger price;

@end
