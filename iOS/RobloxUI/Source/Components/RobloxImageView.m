//
//  RobloxImageView.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/27/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RobloxImageView.h"
#import "RobloxData.h"
#import "UIImage+Utils.h"
#import "RobloxNotifications.h"
#import "RobloxImageCache.h"

@implementation RobloxImageView
{
	NSString* _imageID;
}

+ (NSString*) fullImageID:(NSString*)baseID forSize:(CGSize)size
{
    return [NSString stringWithFormat:@"%@_%d_%d", baseID, (int)size.width, (int)size.height];
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
	{
        [self setupDefaultParams];
    }
    return self;
}

- (id)initWithImage:(UIImage *)image
{
    self = [super initWithImage:image];
    if (self)
	{
        [self setupDefaultParams];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
	{
        [self setupDefaultParams];
    }
    return self;
}

- (void)setupDefaultParams
{
    _imageID = @"";
    self.animateInOptions = RBXImageViewAnimateInAlways;
}

/**
 Load any ROBLOX asset when we already have a prefetched URL for it.
 */
-(void)loadWithAssetID:(NSString*)assetID
     withPrefetchedURL:(NSString*)assetURL
          withFinalURL:(BOOL)assetIsFinal
              withSize:(CGSize)size
            completion:(completion_block_t)block
{
    //if the url is not final, revert to the basic call
    if (!assetIsFinal)
    {
        [self loadWithAssetID:assetID withSize:size completion:block];
        return;
    }
    
    // If the image is already loaded, return immediately
    NSString* fullImageID = [RobloxImageView fullImageID:assetID forSize:size];
    if(_imageID && [_imageID isEqualToString:fullImageID])
    {
        if(block) block();
        return;
    }
    
    _imageID = fullImageID;
    
    self.image = nil;
    
    // check the cache to see if we have already loaded this image
    UIImage* cachedImage = [[RobloxImageCache sharedInstance] getImage:_imageID size:size];
    if(cachedImage != nil)
    {
        [self displayImage:cachedImage wasCached:YES completion:block];
    }
    else
    {
        [self loadImageWithURL:assetURL imageID:_imageID withSize:size completion:block];
    }
}


/**
 Fallback for more specific loadWithAssetId function. This loads any asset.
 */
-(void)loadWithAssetID:(NSString*)assetID withSize:(CGSize)size completion:(completion_block_t)block
{
    // If the image is already loaded, return immediately
    NSString* fullImageID = [RobloxImageView fullImageID:assetID forSize:size];
    if(_imageID && [_imageID isEqualToString:fullImageID])
    {
        if(block) block();
        return;
    }
    
    _imageID = fullImageID;
    
    self.image = nil;
    
    UIImage* cachedImage = [[RobloxImageCache sharedInstance] getImage:_imageID size:size];
    if(cachedImage != nil)
    {
        [self displayImage:cachedImage wasCached:YES completion:block];
    }
    else
    {
        [RobloxData fetchURLForAssetID:assetID withSize:size completion:^(NSString *imageURL)
         {
             if(imageURL)
             {
                 if([_imageID isEqualToString:fullImageID])
                 {
                     [self loadImageWithURL:imageURL imageID:_imageID withSize:size completion:block];
                 }
             }
             else
             {
                 // TODO: Failed get the image url.
                 //       Add a placeholder image for this case.
                 if(block)
                     block();
             }
         }];
    }
}


- (void)loadThumbnailForGame:(RBXGameData *)game withSize:(CGSize)size completion:(completion_block_t)block
{
	// If the image is already loaded, return immediately
    NSString* fullImageID = [RobloxImageView fullImageID:game.placeID forSize:size];
	if(_imageID && [_imageID isEqualToString:fullImageID] )
	{
		if(block) block();
		return;
	}
	
	_imageID = fullImageID;
	
    self.image = nil;
    
    UIImage* cachedImage = [[RobloxImageCache sharedInstance] getImage:_imageID size:size];
    if(cachedImage != nil)
    {
        [self displayImage:cachedImage wasCached:YES completion:block];
    }
    else
    {
        if(game.thumbnailIsFinal && game.thumbnailURL != nil)
        {
            [self loadImageWithURL:game.thumbnailURL imageID:_imageID withSize:size completion:block];
        }
        else
        {
            [RobloxData fetchThumbnailURLForGameID:game.placeID withSize:size completion:^(NSString *imageURL)
            {
                if(imageURL)
                {
                    if([fullImageID isEqualToString:_imageID])
                    {
                        [self loadImageWithURL:imageURL imageID:_imageID withSize:size completion:block];
                    }
                }
                else
                {
                    // TODO: Failed get the image url.
                    //       Add a placeholder image for this case.
                    if(block)
                        block();
                }
            }];
        }
    }
}

- (void)loadAvatarForUserID:(NSInteger)userID prefetchedURL:(NSString *)url urlIsFinal:(BOOL)urlIsFinal withSize:(CGSize)size completion:(completion_block_t)block
{
    if(urlIsFinal == NO)
    {
        [self loadAvatarForUserID:userID withSize:size completion:block];
        return;
    }
    
    // If the image is already loaded, return immediately
    NSString* imageID = [RobloxImageView fullImageID:[NSString stringWithFormat:@"%ld", (long)userID] forSize:size];
    if(_imageID && [_imageID isEqualToString:imageID])
    {
        if(block) block();
        return;
    }
    
    self.image = nil;
    _imageID = imageID;
    
    UIImage* cachedImage = [[RobloxImageCache sharedInstance] getImage:imageID size:size];
    if(cachedImage != nil)
    {
        [self displayImage:cachedImage wasCached:YES completion:block];
    }
    else
    {
        if(urlIsFinal && url != nil)
        {
            [self loadImageWithURL:url imageID:_imageID withSize:size completion:block];
        }
        else
        {
            [self loadAvatarForUserID:userID withSize:size completion:block];
        }
    }
}

- (void)loadAvatarForUserID:(NSInteger)userID withSize:(CGSize)size completion:(completion_block_t)block
{
    // If the image is already loaded, return immediately
    NSString* imageID = [RobloxImageView fullImageID:[NSString stringWithFormat:@"%ld", (long)userID] forSize:size];
    if(_imageID && [_imageID isEqualToString:imageID])
    {
        if(block) block();
        return;
    }
    
    self.image = nil;
    _imageID = imageID;
    
    UIImage* cachedImage = [[RobloxImageCache sharedInstance] getImage:imageID size:size];
    if(cachedImage != nil)
    {
        [self displayImage:cachedImage wasCached:YES completion:block];
    }
    else
    {
        [RobloxData fetchAvatarURLForUserID:userID withSize:size completion:^(NSString *imageURL)
        {
            if(imageURL)
            {
                if([_imageID isEqualToString:imageID])
                {
                    [self loadImageWithURL:imageURL imageID:_imageID withSize:size completion:block];
                }
            }
            else
            {
                // TODO: Failed get the image url.
                //       Add a placeholder image for this case.
                if(block)
                    block();
            }
        }];
    }
}

- (void)loadBadgeWithURL:(NSString *)badgeUrl withSize:(CGSize)size completion:(completion_block_t)block
{
	// If the image is already loaded, return immediately
	if(_imageID && [_imageID isEqualToString:badgeUrl])
	{
		if(block) block();
		return;
	}
	
    _imageID = badgeUrl;
    
    self.image = nil;
    [self loadImageWithURL:badgeUrl imageID:_imageID withSize:size completion:block];
}

/**
    Internal function to actually load the image from a URL
 */
-(void)loadImageWithURL:(NSString*)imageUrl imageID:(NSString*)imageID withSize:(CGSize)size completion:(completion_block_t)block
{
    NSURL* url = [NSURL URLWithString:imageUrl];
    [NSURLConnection sendAsynchronousRequest:[NSURLRequest requestWithURL:url]
                                       queue:[[NSOperationQueue alloc] init]
                           completionHandler:^(NSURLResponse *response, NSData *data, NSError *connectionError)
                           {
                               if(data)
                               {
                                   dispatch_async(dispatch_get_main_queue(), ^
                                   {
                                       // Early check to see if we still have to load the image, or maybe theuser is scrolling too fast
                                       if([imageID isEqualToString:_imageID])
                                       {
                                           dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^(void)
                                           {
                                               UIImage* image = [UIImage imageWithData:data];
                                               dispatch_async(dispatch_get_main_queue(), ^
                                               {
                                                   NSString* fullImageID = [RobloxImageView fullImageID:imageID forSize:size];
                                                   [[RobloxImageCache sharedInstance] setImage:image key:fullImageID size:size];
                                                   if([imageID isEqualToString:_imageID])
                                                   {
                                                       [self displayImage:image wasCached:NO completion:block];
                                                   }
                                               });
                                           });
                                       }
                                   });
                               }
                               else
                               {
                                   if(block)
                                       block(NO);
                               }
                           }];
}

- (void) displayImage:(UIImage*)image wasCached:(BOOL)wasCached completion:(completion_block_t)block
{
    self.image = image;
    
    [self.layer removeAllAnimations];
    
    BOOL animateIn = self.animateInOptions == RBXImageViewAnimateInAlways
                 || (!wasCached && self.animateInOptions == RBXImageViewAnimateInIfNotCached);
    if(animateIn)
    {
        CABasicAnimation *animation = [CABasicAnimation animationWithKeyPath:@"opacity"];
        animation.beginTime = 0.0f;
        animation.duration = 0.2f;
        animation.fromValue = [NSNumber numberWithFloat:0.0f];
        animation.toValue = [NSNumber numberWithFloat:1.0f];
        animation.removedOnCompletion = YES;
        animation.fillMode = kCAFillModeBoth;
        animation.additive = NO;
        [self.layer addAnimation:animation forKey:@"opacityIN"];
    }
    else
    {
        self.layer.opacity = 1.0f;
    }
    
    // Call the completion block
    if(block)
    {
        block();
    }
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
