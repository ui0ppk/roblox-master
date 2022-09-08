//
//  RBVotesView.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/1/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBVotesView.h"
#import "UIView+Position.h"
#import "RobloxData.h"
#import "RobloxHUD.h"

#define BAR_HEIGHT 4.0f
#define HORIZONTAL_MARGIN 33.0f
#define RADIUS 2.0f

#define THUMBS_UP @"Thumbs Up"
#define THUMBS_UP_FILLED @"Thumbs Up Filled"
#define THUMBS_DOWN @"Thumbs Down"
#define THUMBS_DOWN_FILLED @"Thumbs Down Filled"

@implementation RBVotesView
{
    NSString* _assetID;
    NSUInteger _positiveVotes;
    NSUInteger _negativeVotes;
    
    UIView* _positiveVotesBar;
    UIView* _negativeVotesBar;
    UIView* _disabledVotesBar;
    
    UILabel* _positiveLabel;
    UILabel* _negativeLabel;
    
    UIButton* _upVoteButton;
    UIButton* _downVoteButton;
    
    RBXUserVote _userVote;
    
    BOOL _requestInProgress;
}

- (instancetype)init
{
    self = [super init];
    if(self)
    {
        [self initElements];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if(self)
    {
        [self initElements];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if(self)
    {
        [self initElements];
    }
    return self;
}

- (void) setVotesForGame:(RBXGameData*)gameData
{
    _assetID = gameData.placeID;
    _positiveVotes = gameData.upVotes;
    _negativeVotes = gameData.downVotes;
    _userVote = gameData.userVote;
    
    [self updateVotes];
}

- (void) setVotesForPass:(RBXGamePass*)gamePass
{
    _assetID = [gamePass.passID stringValue];
    _positiveVotes = gamePass.upVotes;
    _negativeVotes = gamePass.downVotes;
    _userVote = gamePass.userVote;
    
    [self updateVotes];
}

- (void) initElements
{
    _requestInProgress = NO;
    
    self.backgroundColor = [UIColor clearColor];
    
    UIImage* upVoteImage = [UIImage imageNamed:THUMBS_UP];
    _upVoteButton = [UIButton buttonWithType:UIButtonTypeCustom];
    _upVoteButton.size = upVoteImage.size;
    _upVoteButton.position = CGPointMake(0, self.height - upVoteImage.size.height);
    [_upVoteButton setImage:upVoteImage forState:UIControlStateNormal];
    [_upVoteButton addTarget:self action:@selector(upVoteTouched) forControlEvents:UIControlEventTouchUpInside];
    [_upVoteButton setImageEdgeInsets:UIEdgeInsetsMake(-20, -20, -20, -20)];
    [self addSubview:_upVoteButton];

    UIImage* downVoteImage = [UIImage imageNamed:THUMBS_DOWN];
    _downVoteButton = [UIButton buttonWithType:UIButtonTypeCustom];
    _downVoteButton.size = downVoteImage.size;
    _downVoteButton.position = CGPointMake(self.width - downVoteImage.size.width, self.height - downVoteImage.size.height);
    [_downVoteButton setImage:downVoteImage forState:UIControlStateNormal];
    [_downVoteButton addTarget:self action:@selector(downVoteTouched) forControlEvents:UIControlEventTouchUpInside];
    [_downVoteButton setImageEdgeInsets:UIEdgeInsetsMake(-20, -20, -20, -20)];
    [self addSubview:_downVoteButton];
    
    CGRect positiveRect;
    positiveRect.origin = CGPointMake(HORIZONTAL_MARGIN, self.height - BAR_HEIGHT);
    positiveRect.size = CGSizeMake(self.width - HORIZONTAL_MARGIN - HORIZONTAL_MARGIN, BAR_HEIGHT);
    _positiveVotesBar = [[UIView alloc] initWithFrame:positiveRect];
    _positiveVotesBar.backgroundColor = [UIColor colorWithRed:(0x00/255.0f) green:(0xB2/255.0f) blue:(0x59/255.0f) alpha:1.0f];
    _positiveVotesBar.layer.cornerRadius = 2.0f;
    _positiveVotesBar.hidden = YES;
    [self addSubview:_positiveVotesBar];
    
    CGRect negativeRect;
    negativeRect.origin = CGPointMake(HORIZONTAL_MARGIN, self.height - BAR_HEIGHT);
    negativeRect.size = CGSizeMake(self.width - HORIZONTAL_MARGIN * 2, BAR_HEIGHT);
    _negativeVotesBar = [[UIView alloc] initWithFrame:negativeRect];
    _negativeVotesBar.backgroundColor = [UIColor colorWithRed:(0xD8/255.0f) green:(0x68/255.0f) blue:(0x68/255.0f) alpha:1.0f];
    _negativeVotesBar.layer.cornerRadius = 2.0f;
    _negativeVotesBar.hidden = YES;
    [self addSubview:_negativeVotesBar];
    
    CGRect disabledRect;
    disabledRect.origin = CGPointMake(HORIZONTAL_MARGIN, self.height - BAR_HEIGHT);
    disabledRect.size = CGSizeMake(self.width - HORIZONTAL_MARGIN * 2, BAR_HEIGHT);
    _disabledVotesBar = [[UIView alloc] initWithFrame:disabledRect];
    _disabledVotesBar.backgroundColor = [UIColor colorWithWhite:(0x80/255.0f) alpha:1.0f];
    _disabledVotesBar.layer.cornerRadius = 2.0f;
    [self addSubview:_disabledVotesBar];
    
    CGRect positiveLabelRect;
    positiveLabelRect.size = CGSizeMake(70.0, 23.0);
    positiveLabelRect.origin = CGPointMake(disabledRect.origin.x, disabledRect.origin.y - positiveLabelRect.size.height - 2.0);
    _positiveLabel = [[UILabel alloc] initWithFrame:positiveLabelRect];
    _positiveLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:18];
    _positiveLabel.textColor = [UIColor colorWithWhite:(0x80/255.f) alpha:1.0f];
    [self addSubview:_positiveLabel];
    
    CGRect negativeLabelRect;
    negativeLabelRect.size = CGSizeMake(70.0, 23.0);
    negativeLabelRect.origin.x = CGRectGetMaxX(_disabledVotesBar.frame) - negativeLabelRect.size.width;
    negativeLabelRect.origin.y = positiveLabelRect.origin.y;
    _negativeLabel = [[UILabel alloc] initWithFrame:negativeLabelRect];
    _negativeLabel.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:18];
    _negativeLabel.textColor = [UIColor colorWithWhite:(0x80/255.f) alpha:1.0f];
    _negativeLabel.textAlignment = NSTextAlignmentRight;
    [self addSubview:_negativeLabel];
}

- (void) updateVotes
{
    NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
    [numberFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
    [numberFormatter setMaximumFractionDigits:0];
    
    _positiveLabel.text = [numberFormatter stringFromNumber:[NSNumber numberWithUnsignedInteger:_positiveVotes]];
    _negativeLabel.text = [numberFormatter stringFromNumber:[NSNumber numberWithUnsignedInteger:_negativeVotes]];
    
    if(_positiveVotes == 0 && _negativeVotes == 0)
    {
        _positiveVotesBar.hidden = YES;
        _negativeVotesBar.hidden = YES;
        _disabledVotesBar.hidden = NO;
    }
    else
    {
        _positiveVotesBar.hidden = NO;
        _negativeVotesBar.hidden = NO;
        _disabledVotesBar.hidden = YES;
        
        float totalVotes = _positiveVotes + _negativeVotes;
        
        _positiveVotesBar.width = _disabledVotesBar.width * (_positiveVotes / totalVotes);
        
        _negativeVotesBar.x = _positiveVotesBar.x + _positiveVotesBar.width - 2.0;
        _negativeVotesBar.width = _disabledVotesBar.width - _positiveVotesBar.width;
    }
    
    if(_userVote == RBXUserVotePositive)
    {
        UIImage* upImageFill = [UIImage imageNamed:THUMBS_UP_FILLED];
        [_upVoteButton setImage:upImageFill forState:UIControlStateNormal];
    }
    else
    {
        UIImage* upImageFill = [UIImage imageNamed:THUMBS_UP];
        [_upVoteButton setImage:upImageFill forState:UIControlStateNormal];
    }
    
    if(_userVote == RBXUserVoteNegative)
    {
        UIImage* downImageFill = [UIImage imageNamed:THUMBS_DOWN_FILLED];
        [_downVoteButton setImage:downImageFill forState:UIControlStateNormal];
    }
    else
    {
        UIImage* downImageFill = [UIImage imageNamed:THUMBS_DOWN];
        [_downVoteButton setImage:downImageFill forState:UIControlStateNormal];
    }
}

- (void) upVoteTouched
{
    if(_requestInProgress || _userVote == RBXUserVotePositive)
        return;
    
    [self emitVote:YES];
}

- (void) downVoteTouched
{
    if(_requestInProgress || _userVote == RBXUserVoteNegative)
        return;
    
    [self emitVote:NO];
}

- (void) emitVote:(BOOL)positive
{
    _requestInProgress = YES;
    
    UIView* animatable = positive ? _upVoteButton : _downVoteButton;
    
    CGRect sourceRect = animatable.frame;
    [UIView animateWithDuration:0.2
        delay:0
        options:UIViewAnimationOptionCurveEaseInOut
        animations:^
        {
            animatable.frame = CGRectOffset(sourceRect, 0, +15);
        }
        completion:^(BOOL finished)
        {
            [UIView animateWithDuration:0.2
                delay:0
                options:UIViewAnimationOptionCurveEaseInOut
                animations:^
                {
                    animatable.frame = sourceRect;
                }
                completion:nil];
        }];

    [RobloxData voteForAssetID:_assetID
        votePositive:positive
        completion:^(BOOL success, NSString *message, NSUInteger upVotes, NSUInteger downVotes, RBXUserVote userVote)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                _requestInProgress = NO;
                if(success)
                {
                    _positiveVotes = upVotes;
                    _negativeVotes = downVotes;
                    _userVote = userVote;
                    
                    [self updateVotes];
                }
                else if(message != nil)
                {
                    [RobloxHUD showMessage:message];
                }
            });
        }];
}

@end
