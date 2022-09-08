//
//  GamePreviewScreenController.m
//  RobloxMobile
//
//  Created by alichtin on 5/30/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GamePreviewScreenController.h"
#import "GamePreviewDescriptionViewController.h"
#import "RobloxImageView.h"
#import "RobloxHUD.h"
#import "RobloxTheme.h"
#import "RobloxNotifications.h"
#import "Flurry.h"
#import "UIViewController+Helpers.h"
#import "UIScrollView+Auto.h"
#import "UIView+Position.h"
#import "GameConsumableCollectionViewController.h"
#import "LeaderboardsViewController.h"
#import "RBVotesView.h"
#import "RBFavoritesView.h"
#import "RBGameViewController.h"
#import "RBBadgesViewController.h"
#import "RBProfileViewController.h"
#import "RBConfirmPurchaseViewController.h"
#import "RBActivityIndicatorView.h"
#import "RobloxImageView.h"
#import "YTPlayerView.h"

#define THUMBNAIL_SIZE RBX_SCALED_DEVICE_SIZE(455, 256)
#define AVATAR_SIZE CGSizeMake(100, 100)

#define DETAILS_Y_OFFSET 22.0f

#define SEGMENT_DETAIL_INDEX 0
#define SEGMENT_LEADERBOARD_INDEX 1

//---METRICS---
#define GPSC_closeButtonTouched    @"GAME PREVIEW SCREEN - Close Button Touched"
#define GPSC_playButtonTouched     @"GAME PREVIEW SCREEN - Play Button Touched"

@interface GamePreviewScreenController ()
- (void)setUpMaskLayer;
@end

@implementation GamePreviewScreenController
{
    NSNumber* _creatorID;
    
    UISegmentedControl* _segmentedControl;
    
    IBOutlet UIScrollView* _detailsScrollView;
    IBOutlet UIView* _leaderboardView;
    IBOutlet UIView* _moreDetailsView;
    
    // Details view items
    IBOutlet UIPageControl* _pcItems;
    IBOutlet UIScrollView* _svThumbnails;
    NSArray* thumbnails;
    IBOutlet UILabel* _gameTitle;
    IBOutlet UILabel *_builderTitle;
    IBOutlet UIButton *_builderButton;
    IBOutlet UITextView *_description;
    IBOutlet UIButton *_readMoreButton;
    IBOutlet UIButton *_playButton;
    IBOutlet RobloxImageView* _builderAvatar;
    IBOutlet RBFavoritesView* _favoritesView;
    
    IBOutlet UILabel* _moreTitle;
    IBOutlet UILabel* _visitsTitle;
    IBOutlet UILabel* _createdTitle;
    IBOutlet UILabel* _updatedTitle;
    IBOutlet UILabel* _maxPlayersTitle;
    IBOutlet UILabel* _genreTitle;
    IBOutlet UILabel* _visitsValue;
    IBOutlet UILabel* _createdValue;
    IBOutlet UILabel* _updatedValue;
    IBOutlet UILabel* _maxPlayersValue;
    IBOutlet UILabel* _genreValue;
    
    IBOutlet RBVotesView* _votesView;
    
    // Leaderboard view items
    IBOutlet RobloxImageView* _leaderboardGameThumbnail;
    IBOutlet UILabel* _leaderboardGameTitle;
    
    GameConsumableCollectionViewController* _gearCollectionViewController;
    GameConsumableCollectionViewController* _passesCollectionViewController;
    RBBadgesViewController* _badgesViewController;
    
    CAGradientLayer* maskLayer;
    
    RBXGameData* _firstGameData;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    _firstGameData = self.gameData;
    
    self.view.backgroundColor = [RobloxTheme lightBackground];
    
    self.edgesForExtendedLayout = UIRectEdgeNone;
    
    // Save this value, since its not provided in the full game details function
    _creatorID = _gameData.creatorID;
    
    _segmentedControl = [[UISegmentedControl alloc] init];
    [_segmentedControl insertSegmentWithTitle:NSLocalizedString(@"DetailsWord", nil) atIndex:0 animated:NO];
    [_segmentedControl insertSegmentWithTitle:NSLocalizedString(@"LeaderboardsWord", nil) atIndex:1 animated:NO];
    [_segmentedControl addTarget:self action:@selector(onSegmentedControlValueChanged) forControlEvents:UIControlEventValueChanged];
    [_segmentedControl setSelectedSegmentIndex:SEGMENT_DETAIL_INDEX];
    [_segmentedControl setEnabled:NO forSegmentAtIndex:SEGMENT_LEADERBOARD_INDEX];
    [_segmentedControl sizeToFit];
    self.navigationItem.titleView = _segmentedControl;
    [self onSegmentedControlValueChanged];
    
    [self initializeGameDetails];
    
    [self layoutDetailsElements];
}

- (void) initializeGameDetails
{
    [RobloxTheme applyToGamePreviewTitle:_gameTitle];
    [RobloxTheme applyToGamePreviewDetailTitle:_builderTitle];
    [RobloxTheme applyToGamePreviewDescription:_description];
    [RobloxTheme applyToGamePreviewBuilderButton:_builderButton];
    
    [RobloxTheme applyToGameSortTitle:_moreTitle];
    [RobloxTheme applyToGamePreviewMoreInfoTitle:_visitsTitle];
    [RobloxTheme applyToGamePreviewMoreInfoTitle:_createdTitle];
    [RobloxTheme applyToGamePreviewMoreInfoTitle:_updatedTitle];
    [RobloxTheme applyToGamePreviewMoreInfoTitle:_maxPlayersTitle];
    [RobloxTheme applyToGamePreviewMoreInfoTitle:_genreTitle];
    [RobloxTheme applyToGamePreviewMoreInfoValue:_visitsValue];
    [RobloxTheme applyToGamePreviewMoreInfoValue:_createdValue];
    [RobloxTheme applyToGamePreviewMoreInfoValue:_updatedValue];
    [RobloxTheme applyToGamePreviewMoreInfoValue:_maxPlayersValue];
    [RobloxTheme applyToGamePreviewMoreInfoValue:_genreValue];
    
    _builderTitle.text = [NSString stringWithFormat:@"%@:", NSLocalizedString(@"BuilderWord", nil)];
    [_readMoreButton setTitle:NSLocalizedString(@"ReadMoreWord", nil) forState:UIControlStateNormal];
    
    if( _firstGameData.userOwns == NO && _firstGameData.price > 0 )
    {
        NSString* buyAccessButtonTitle = NSLocalizedString(@"BuyAccessPhrase", nil);
        [_playButton setTitle:buyAccessButtonTitle forState:UIControlStateNormal];
    }
    else
    {
        [_playButton setTitle:NSLocalizedString(@"PlayButtonLabel", nil) forState:UIControlStateNormal];
    }
    
    _moreTitle.text = [NSLocalizedString(@"MoreDetailsPhrase", nil) uppercaseString];
    _visitsTitle.text = NSLocalizedString(@"VisitsWord", nil);
    _createdTitle.text = NSLocalizedString(@"CreatedWord", nil);
    _updatedTitle.text = NSLocalizedString(@"UpdatedWord", nil);
    _maxPlayersTitle.text = NSLocalizedString(@"MaxPlayersPhrase", nil);
    _genreTitle.text = NSLocalizedString(@"GenreWord", nil);
    
    _visitsValue.text = @"";
    _createdValue.text = @"";
    _updatedValue.text = @"";
    _maxPlayersValue.text = @"";
    _genreValue.text = @"";
    
    [_votesView setVotesForGame:_gameData];
    
    _gameTitle.text = _gameData.title;
    
    [_builderButton setEnabled:NO];
    [_builderButton setTitle:@"" forState:UIControlStateNormal];
    _description.text = @"";
    
    _description.delegate = self;
    
    [_builderAvatar loadAvatarForUserID:[_gameData.creatorID integerValue] withSize:AVATAR_SIZE completion:nil];
    
    __weak GamePreviewScreenController* weakSelf = self;
    
    [RobloxData fetchGameDetails:_gameData.placeID completion:^(RBXGameData *game)
     {
         if(game != nil)
         {
             _gameData = game;
             
             dispatch_async(dispatch_get_main_queue(), ^
             {
                 [_favoritesView setFavoritesForGame:game];
                
                 [_builderButton setEnabled:YES];
                 [_builderButton setTitle:[NSString stringWithFormat:@"%@  >", game.creatorName] forState:UIControlStateNormal];
                
                 _description.text = game.gameDescription;
                 bool isTextLongerThanThreeLines = [[game.gameDescription componentsSeparatedByString:@"\n"] count] >= 3;
                 bool isTextTooLong = [game.gameDescription length] > 90;
                 _readMoreButton.hidden = !(isTextLongerThanThreeLines || isTextTooLong);
                
                 NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
                 [numberFormatter setNumberStyle:NSNumberFormatterDecimalStyle];
                 [numberFormatter setMaximumFractionDigits:0];
                
                 _visitsValue.text = [numberFormatter stringFromNumber:[NSNumber numberWithUnsignedInteger:_gameData.visited]];
                 _createdValue.text = _gameData.dateCreated;
                 _updatedValue.text = _gameData.dateUpdated;
                 _maxPlayersValue.text = [NSString stringWithFormat:@"%lu", (unsigned long)_gameData.maxPlayers];
                 _genreValue.text = _gameData.assetGenre;
                
                 [_votesView setVotesForGame:_gameData];
                 [_favoritesView setFavoritesForGame:_gameData];
                
                 [weakSelf initializeGameLeaderboards];
             });
         }
     }];
    
    _gearCollectionViewController = [[GameConsumableCollectionViewController alloc] initWithNibName:@"GameConsumableCollectionViewController" bundle:nil];
    _gearCollectionViewController.view.frame = CGRectMake(0, 0, 1024, 245);
    [_gearCollectionViewController fetchGearForPlaceID:_gameData.placeID gameTitle:_gameData.title completion:^{
        [weakSelf layoutDetailsElements];
    }];
    _gearCollectionViewController.view.hidden = YES;
    [_detailsScrollView addSubview:_gearCollectionViewController.view];
    [self addChildViewController:_gearCollectionViewController];
    
    _passesCollectionViewController = [[GameConsumableCollectionViewController alloc] initWithNibName:@"GameConsumableCollectionViewController" bundle:nil];
    _passesCollectionViewController.view.frame = CGRectMake(0, 0, 1024, 245);
    [_passesCollectionViewController fetchPassesForPlaceID:_gameData.placeID gameTitle:_gameData.title completion:^{
        [weakSelf layoutDetailsElements];
    }];
    _passesCollectionViewController.view.hidden = YES;
    [_detailsScrollView addSubview:_passesCollectionViewController.view];
    [self addChildViewController:_passesCollectionViewController];
    
    _badgesViewController = [[RBBadgesViewController alloc] initWithNibName:@"RBBadgesViewController" bundle:nil];
    _badgesViewController.gameID = _gameData.placeID;
    _badgesViewController.completionHandler = ^{
        [weakSelf layoutDetailsElements];
    };
    _badgesViewController.view.hidden = YES;
    [_detailsScrollView addSubview:_badgesViewController.view];
    [self addChildViewController:_badgesViewController];
    
    
    //add in the thumbnails for the game
    [RobloxData fetchThumbnailsForPlace:_gameData.placeID
                               withSize:THUMBNAIL_SIZE
                             completion:^(NSArray *gameThumbnails)
     {
         //loop through all the thumbnails and parse them out
         for (RBXThumbnail* rbxThumb in gameThumbnails)
         {
             if (rbxThumb.assetTypeId == RBXThumbnailImage)
             {
                 //add the image to the scroll
                 dispatch_async(dispatch_get_main_queue(), ^
                 {
                     CGRect framePosition = CGRectMake(_svThumbnails.subviews.count * _svThumbnails.frame.size.width,
                                                       0,
                                                       _svThumbnails.frame.size.width,
                                                       _svThumbnails.frame.size.height);
                      RobloxImageView* rbxImage = [[RobloxImageView alloc] initWithFrame:framePosition];
                     
                     //load the image with the thumbnail's asset ID
                     [rbxImage loadWithAssetID:rbxThumb.assetId
                             withPrefetchedURL:rbxThumb.assetURL
                                  withFinalURL:rbxThumb.assetIsFinal
                                      withSize:framePosition.size
                                    completion:nil];
                     [_svThumbnails addSubview:rbxImage];
                 });
             }
             else if (rbxThumb.assetTypeId == RBXThumbnailVideo)
             {
                 //we have a video thumbnail
                 dispatch_async(dispatch_get_main_queue(), ^
                 {
                     CGRect framePosition = CGRectMake(_svThumbnails.subviews.count * _svThumbnails.frame.size.width,
                                                       0,
                                                       _svThumbnails.frame.size.width,
                                                       _svThumbnails.frame.size.height);
                     YTPlayerView* ytThumbnail = [[YTPlayerView alloc] initWithFrame:framePosition];
                     NSDictionary* playerSettings = [NSDictionary dictionaryWithObjects:@[[NSNumber numberWithInt:0], [NSNumber numberWithInt:1], [NSNumber numberWithInt:0]]
                                                                                forKeys:@[@"autoplay", @"controls", @"rel"]];
                     
                     //load the video thumbnail
                     [ytThumbnail loadWithVideoId:rbxThumb.assetHash playerVars:playerSettings];
                     [_svThumbnails addSubview:ytThumbnail];
                 });
             }
         }
         dispatch_async(dispatch_get_main_queue(), ^
         {
             [_pcItems setNumberOfPages:_svThumbnails.subviews.count];
             [_svThumbnails setContentSize:CGSizeMake(_svThumbnails.frame.size.width * [_svThumbnails.subviews count], _svThumbnails.frame.size.height)];
         });
     }];
}

- (void) initializeGameLeaderboards
{
    [RobloxTheme applyToGamePreviewTitle:_leaderboardGameTitle];
    
    _leaderboardGameTitle.text = _gameData.title;
    
    [_leaderboardGameThumbnail loadThumbnailForGame:_gameData withSize:THUMBNAIL_SIZE completion:nil];
    
    LeaderboardsViewController* leaderboardController = [self.storyboard instantiateViewControllerWithIdentifier:@"LeaderboardViewController"];
    leaderboardController.timeFilter = RBXLeaderboardsTimeFilterDay;
    leaderboardController.distributorID = _gameData.universeID;
    leaderboardController.onLeaderboardsLoaded = ^(BOOL hasLeaderboards)
    {
        // Only enable leaderboards when the leaderboard are fully loaded
        if(hasLeaderboards)
        {
            [_segmentedControl setEnabled:YES forSegmentAtIndex:SEGMENT_LEADERBOARD_INDEX];
        }
    };
    [self addChildViewController:leaderboardController];
    [_leaderboardView addSubview:leaderboardController.view];
    [leaderboardController updateLeaderboards];
    leaderboardController.view.position = CGPointMake(28, 132);
}

- (void) onSegmentedControlValueChanged
{
    _detailsScrollView.hidden = _segmentedControl.selectedSegmentIndex != 0;
    _leaderboardView.hidden = _segmentedControl.selectedSegmentIndex != 1;
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    if ([scrollView isEqual:_svThumbnails])
    {
        //update the page control item
        CGFloat pageWidth = _svThumbnails.frame.size.width;
        int page = floor((_svThumbnails.contentOffset.x - pageWidth / 2) / pageWidth) + 1;
        _pcItems.currentPage = page;
    }
    else
    {
        [CATransaction begin];
        [CATransaction setDisableActions:YES];
        maskLayer.position = CGPointMake(0, scrollView.contentOffset.y);
        [CATransaction commit];
    }
}

- (void)setUpMaskLayer
{
    maskLayer = [CAGradientLayer layer];
    maskLayer.frame = _readMoreButton.bounds;
    maskLayer.colors = @[(id)[UIColor colorWithWhite:1.0 alpha:1.0].CGColor,
                         (id)[UIColor colorWithWhite:1.0 alpha:1.0].CGColor,
                         (id)[UIColor colorWithWhite:1.0 alpha:0.0].CGColor];
    maskLayer.startPoint = CGPointMake(1.0, 0.5);
    maskLayer.endPoint = CGPointMake(0, 0.5);

    [_readMoreButton.layer insertSublayer:maskLayer atIndex:0];
}

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    
    [self setUpMaskLayer];
}

- (IBAction)playButtonTouched:(id)sender
{
    [Flurry logEvent:GPSC_playButtonTouched];
    
    // Exit if we are in background or transitioning
    if( ![RBGameViewController isAppRunning] )
    {
        return;
    }
    
    int typeId = [_gameData.placeID intValue];
    if (typeId > 0)
    {
        if(_firstGameData.userOwns == NO && _firstGameData.price > 0)
        {
            RBConfirmPurchaseViewController* controller = [[RBConfirmPurchaseViewController alloc] initWithNibName:@"RBConfirmPurchaseViewController" bundle:nil];
            controller.modalPresentationStyle = UIModalPresentationFormSheet;
            controller.thumbnailAssetID = _firstGameData.placeID;
            controller.productName = _firstGameData.title;
            controller.productID = _firstGameData.productID;
            controller.price = _firstGameData.price;
            [self presentViewController:controller animated:YES completion:nil];
        }
        else
        {
            RBGameViewController* controller = [[RBGameViewController alloc] initWithLaunchParams:[RBXGameLaunchParams InitParamsForJoinPlace:[_firstGameData.placeID intValue]]];
            [self presentViewController:controller animated:NO completion:nil];
        }
    }
    else
    {
        [RobloxHUD showMessage:NSLocalizedString(@"GeneralGameStartError", nil)];
    }
}

- (IBAction)readMoreButtonTouched:(id)sender
{
    GamePreviewDescriptionViewController* gpdvcPopUp = [[GamePreviewDescriptionViewController alloc] initWithDescription:_gameData.gameDescription];
    UINavigationController* navigation = [[UINavigationController alloc] initWithRootViewController:gpdvcPopUp];
    navigation.modalPresentationStyle = UIModalPresentationFormSheet;
    [self.navigationController presentViewController:navigation animated:YES completion:nil];
}

- (IBAction)didTapPageDot:(id)sender
{
    [_svThumbnails setContentOffset:CGPointMake(_svThumbnails.frame.size.width * _pcItems.currentPage, 0) animated:YES];
}

-(void) handleStartGameFailure
{
    [RobloxHUD hideSpinner:YES];
}

-(void) handleStartGameSuccess
{
    [RobloxHUD hideSpinner:YES];
}

- (void) viewWillAppear:(BOOL)animated
{
    [super viewWillAppear:animated];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onGamePurchased:) name:RBX_NOTIFY_GAME_PURCHASED object:nil];
}

- (void) onGamePurchased:(NSNotification*)notification
{
    _firstGameData.userOwns = YES;
    _gameData.userOwns = YES;
    
    [_playButton setTitle:NSLocalizedString(@"PlayButtonLabel", nil) forState:UIControlStateNormal];
}

- (IBAction)builderButtonTouchUpInside:(id)sender
{
    RBProfileViewController *viewController = [self.storyboard instantiateViewControllerWithIdentifier:@"RBOthersProfileViewController"];
    viewController.userId = _creatorID;
    [self.navigationController pushViewController:viewController animated:YES];
}

-(void)processTap:(UIGestureRecognizer*)gestureRecognizer
{
	if(gestureRecognizer.state == UIGestureRecognizerStateEnded)
	{
		CGPoint location = [gestureRecognizer locationInView:nil]; //Passing nil gives us coordinates in the window

		//Convert tap location into the local view's coordinate system. If outside, dismiss the view.
		if (![self.view pointInside:[self.view convertPoint:location fromView:self.view.window] withEvent:nil])
		{
			[self dismissViewControllerAnimated:YES completion:nil];
		}
	}
}

- (void) layoutDetailsElements
{
    CGFloat referenceY = 405.0f;
    
    if(_gearCollectionViewController.numItems > 0)
    {
        _gearCollectionViewController.view.position = CGPointMake(0, referenceY);
        referenceY += _gearCollectionViewController.view.frame.size.height + DETAILS_Y_OFFSET;
        _gearCollectionViewController.view.hidden = NO;
    }
    else
        _gearCollectionViewController.view.hidden = YES;
    
    _moreDetailsView.y = referenceY;
    referenceY += _moreDetailsView.frame.size.height + DETAILS_Y_OFFSET;
    
    if(_passesCollectionViewController.numItems > 0)
    {
        _passesCollectionViewController.view.position = CGPointMake(0, referenceY);
        referenceY += _passesCollectionViewController.view.frame.size.height + DETAILS_Y_OFFSET;
        _passesCollectionViewController.view.hidden = NO;
    }
    else
        _passesCollectionViewController.view.hidden = YES;
    
    if( _badgesViewController != nil && _badgesViewController.numItems > 0 )
    {
        _badgesViewController.view.position = CGPointMake(28, referenceY);
        _badgesViewController.view.hidden = NO;
    }
    else
        _badgesViewController.view.hidden = YES;
    
    [_detailsScrollView setContentSizeForDirection:UIScrollViewDirectionVertical];
    _detailsScrollView.contentSize = CGSizeMake(_detailsScrollView.contentSize.width, _detailsScrollView.contentSize.height + DETAILS_Y_OFFSET);
}

- (void)dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver:self];
}

@end
