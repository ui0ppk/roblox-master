//
//  GameSearchResultCell.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/10/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxImageView.h"
#import "RobloxData.h"
#import "RBActivityIndicatorView.h"

@interface UserSearchResultCell : UICollectionViewCell


//cell visual elements
@property IBOutlet UILabel* lblName;
@property IBOutlet RobloxImageView* imgAvatar;
@property IBOutlet UIImageView* imgIsOnline;
@property IBOutlet RBActivityIndicatorView* imgLoadingSpinner;

//data values
@property (strong, nonatomic) RBXUserSearchInfo* searchInfo;

+(CGSize) getCellSize;
+(NSString*) getNibName;

-(void) setInfo:(RBXUserSearchInfo *)searchInfo;
@end
