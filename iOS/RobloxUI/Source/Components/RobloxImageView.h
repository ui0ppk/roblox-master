//
//  RobloxImageView.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/27/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RobloxData.h"
#import <UIKit/UIKit.h>

// Animate in options
typedef NS_ENUM(NSUInteger, RBXImageViewAnimateInOptions)
{
    RBXImageViewAnimateInNever = 0,
    RBXImageViewAnimateInIfNotCached = 1,
    RBXImageViewAnimateInAlways = 3
};

// Completion block type
typedef void (^completion_block_t)();

@interface RobloxImageView : UIImageView

// If true, animate a fade in once the image is loaded.
@property (nonatomic) RBXImageViewAnimateInOptions animateInOptions;

/**
    Load the image for the game.
    If the image is already cached, the completion block will execute immediately (cached=YES)
    Otherwise, it will start a asynchronus request and execute "block(cached=NO)" when the image is final.
 */
-(void)loadThumbnailForGame:(RBXGameData*)game withSize:(CGSize)size completion:(completion_block_t)block;

/**
 Load the avatar for users/clans.
 */
-(void)loadAvatarForUserID:(NSInteger)userID withSize:(CGSize)size completion:(completion_block_t)block;

/**
 Load the avatar for users/clans, using the prefetched url if is final
 */
-(void)loadAvatarForUserID:(NSInteger)userID prefetchedURL:(NSString*)url urlIsFinal:(BOOL)urlIsFinal withSize:(CGSize)size completion:(completion_block_t)block;

/**
 Load a badge.
 */
-(void)loadBadgeWithURL:(NSString*)badgeUrl withSize:(CGSize)size completion:(completion_block_t)block;

/**
 Load any ROBLOX asset.
 */
-(void)loadWithAssetID:(NSString*)assetID withSize:(CGSize)size completion:(completion_block_t)block;

/**
 Load any ROBLOX asset when we already have a prefetched URL for it.
 */
-(void)loadWithAssetID:(NSString*)assetID
     withPrefetchedURL:(NSString*)assetURL
          withFinalURL:(BOOL)assetIsFinal
              withSize:(CGSize)size
            completion:(completion_block_t)block;

@end
