//
//  GameSortCarouselViewController
//
//  Created by Huy Le on 4/29/14.
//  Copyright (c) 2014 2359Media. All rights reserved.
//

#import <UIKit/UIKit.h>
#import "RobloxData.h"
#import "iCarousel/iCarousel.h"

@interface GameSortCarouselViewController : UIViewController

@property(strong, nonatomic) RBXGameSort* gameSort;
@property(nonatomic) NSUInteger maxNumItems;
@property(nonatomic, strong) iCarousel *carousel;

@property(copy) void (^gameSelectedHandler)(RBXGameData* gameData);

-(id) initWithFrame:(CGRect)frame;

@end
