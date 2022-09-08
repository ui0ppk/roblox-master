//
//  GamePreviewScreenController.h
//  RobloxMobile
//
//  Created by alichtin on 5/30/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"

@interface GamePreviewScreenController : UIViewController<UITextViewDelegate, UIScrollViewDelegate>

@property (strong, nonatomic) RBXGameData* gameData;

@end
