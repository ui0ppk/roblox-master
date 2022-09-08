//
//  GameThumbnailCell.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/27/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//
#import <QuartzCore/QuartzCore.h>
#import "GameThumbnailCell.h"
#import "RobloxImageView.h"
#import "RobloxData.h"
#import "RobloxTheme.h"
#import "RBActivityIndicatorView.h"
#import "RobloxInfo.h"

#define NUM_CELLS 5

@interface GameVotesView : UIView

-(void) setLikes:(uint)numLikes andDislikes:(uint)numDislikes;

@end
@implementation GameVotesView
{
    uint likes;
    uint dislikes;
    
    CALayer* likesLayer;
    CALayer* dislikesLayer;
    NSMutableArray* margins;
}

-(void) initialize
{
    likes = 1;
    dislikes = 1;
    

    //draw the likes
    likesLayer = [CALayer layer];
    [likesLayer setBackgroundColor:[RobloxTheme colorGray2].CGColor];
    [likesLayer setFrame:CGRectZero];
    [self.layer addSublayer:likesLayer];

    //draw the dislikes
    dislikesLayer = [CALayer layer];
    [dislikesLayer setBackgroundColor:[RobloxTheme colorGray3].CGColor];
    [dislikesLayer setFrame:CGRectZero];
    [self.layer addSublayer:dislikesLayer];

    //create the margin layers
    margins = [NSMutableArray arrayWithCapacity:NUM_CELLS-1];
    for (int i = 1; i < NUM_CELLS; i++)
    {
        CALayer* aMargin = [CALayer layer];
        [aMargin setBackgroundColor:[UIColor whiteColor].CGColor];
        [aMargin setFrame:CGRectZero];
        [self.layer addSublayer:aMargin];
        [margins addObject:aMargin];
    }
    
    self.backgroundColor = [UIColor clearColor];
}

-(id) init
{
    self = [super init];
    if (self)
        [self initialize];
    
    return self;
}
-(id) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if (self)
        [self initialize];
    
    return self;
}
-(id) initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
        [self initialize];
    
    return self;
}

-(void) setLikes:(uint)numLikes andDislikes:(uint)numDislikes
{
    likes = numLikes;
    dislikes = numDislikes;
    
    [self drawAllLayers];
}
-(void) drawAllLayers
{
    //draw the likes and dislikes
    int totalVotes = likes + dislikes;
    float percentLikes = (float) likes / (float) MAX(totalVotes, 1);
    
    [likesLayer setFrame:CGRectMake(0, 0, self.frame.size.width * percentLikes, self.frame.size.height)];
    [dislikesLayer setFrame:CGRectMake(likesLayer.frame.size.width, 0, self.frame.size.width - likesLayer.frame.size.width, self.frame.size.height)];
}
-(void) layoutSubviews
{
    //draw the margin dividers
    float margin = 2;
    float cellWidth = (self.frame.size.width - (margin * margins.count)) / NUM_CELLS;
    
    for (int i = 1; i <= margins.count; i++)
    {
        CGRect marginDivider = CGRectMake((cellWidth + margin) * i, 0, margin, self.frame.size.height);
        [(CALayer*)margins[i-1] setFrame:marginDivider];
    }
}

@end



@interface GameThumbnailCell()
@end

@implementation GameThumbnailCell
{
	IBOutlet RobloxImageView *_image;
	IBOutlet UILabel *_title;
    IBOutlet UILabel *_numPlayers;
    IBOutlet GameVotesView  *_votes;
    IBOutlet UIImageView *_playerVote;
	IBOutlet RBActivityIndicatorView *_activityIndicator;
    
    NSString* _placeID;
}

+ (CGSize) getCellSize
{
    return [RobloxInfo thisDeviceIsATablet] ? CGSizeMake(166, 220) : CGSizeMake(150, 220);
}
+ (NSString*) getNibName
{
    return [RobloxInfo thisDeviceIsATablet] ? @"GameThumbnailCell" : @"GameThumbnailCell_iPhone";
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if(self)
    {
        [RobloxTheme applyRoundedBorderToView:self];
        _title.hidden = YES;
        _image.hidden = YES;
        _numPlayers.hidden = YES;
        _votes.hidden = YES;
        _playerVote.hidden = YES;
        
        [RobloxTheme applyToGameThumbnailTitle:_title];
        _image.animateInOptions = RBXImageViewAnimateInIfNotCached;
    }
    return self;
}

-(void)setGameData:(RBXGameData*)data
{
    if (data) // if(![_placeID isEqualToString:data.placeID])
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            //save the placeID
            _placeID = data.placeID;
            
            //update the title
            _title.text = data.title;
            _title.hidden = NO;
            
            //set the number of players currently playing
            _numPlayers.text = [NSString stringWithFormat:@"%@ %@", data.players, NSLocalizedString(@"NumberPlayersPhrase", nil)];
            _numPlayers.hidden = NO;
            
            
            //update the player voted image
            switch (data.userVote)
            {
                case RBXUserVotePositive: { [_playerVote setImage:[UIImage imageNamed:@"Thumbs Down Filled"]]; } break;
                case RBXUserVoteNegative: { [_playerVote setImage:[UIImage imageNamed:@"Thumbs Up Filled"]]; } break;
                default: { [_playerVote setImage:[UIImage imageNamed:@"Thumbs Up Greyed"]]; } break;
            }
            _playerVote.hidden = NO;
            
            
            //draw the total player votes
            //[self drawVotes:data.upVotes withNumberOfLikes:(data.upVotes + data.downVotes)];
            [_votes setLikes:data.upVotes andDislikes:data.downVotes];
            _votes.hidden = NO;
            
            
            //load the image icon
            [_activityIndicator startAnimating];
            [_image loadThumbnailForGame:data withSize:[RobloxTheme sizeProfilePictureMedium] completion:^
             {
                dispatch_async(dispatch_get_main_queue(), ^
                {
                    [_activityIndicator stopAnimating];
                    _image.hidden = NO;
                });
             }];
        });
        
        
        
    }
    else
    {
        _placeID = nil;
        
        _image.hidden = YES;
        _title.hidden = YES;
        _votes.hidden = YES;
        _playerVote.hidden = YES;
        _numPlayers.hidden = YES;
    }
}

@end
