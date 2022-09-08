//
//  GameSortCarouselViewController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/28/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GameSortCarouselViewController.h"
#import "CarouselThumbnailCell.h"

#import "RobloxTheme.h"
#import "RobloxData.h"

#import "NSTimer+Blocks.h"

#define THUMBNAIL_SIZE RBX_SCALED_DEVICE_SIZE(455, 256)
#define ITEM_FRAME CGRectMake(0,0,455,256)

@interface GameSortCarouselViewController () <iCarouselDataSource, iCarouselDelegate>

@end

@implementation GameSortCarouselViewController
{
    CGRect _customFrame;
//    iCarousel* _carousel;
    NSTimer* _autoScrollTimer;
    
    NSArray* _games;
}

- (id)initWithFrame:(CGRect)frame
{
    self = [super init];
    if(self)
    {
        _games = nil;
        _customFrame = frame;
        _maxNumItems = 10;
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.view.frame = CGRectZero;
    
    _carousel = [[iCarousel alloc] initWithFrame:_customFrame];
    _carousel.autoresizingMask = UIViewAutoresizingNone;
    _carousel.type = iCarouselTypeCoverFlow;
    _carousel.delegate = self;
    _carousel.dataSource = self;
    _carousel.decelerationRate = 0.7f;
    
    [self.view addSubview:_carousel];
    
    [self initAutoScrollTimer];
}

- (void)viewWillUnload
{
    [self killAutoScrollTimer];
    
    [super viewWillUnload];
    _carousel = nil;
}

- (void)setGameSort:(RBXGameSort *)gameSort
{
    _gameSort = gameSort;
    
    [RobloxData fetchGameListWithSortID:_gameSort.sortID genreID:nil fromIndex:0 numGames:self.maxNumItems thumbSize:THUMBNAIL_SIZE completion:^(NSArray *games)
    {
        _games = games;

        dispatch_async(dispatch_get_main_queue(), ^
        {
            [_carousel reloadData];
        });
    }];
}

-(void) initAutoScrollTimer
{
    _autoScrollTimer = [NSTimer scheduledTimerWithTimeInterval:7.0f block:^
    {
        [_carousel scrollByNumberOfItems:1 duration:1.0f];
    }
    repeats:YES];
}

-(void) killAutoScrollTimer
{
    if(_autoScrollTimer)
    {
        [_autoScrollTimer invalidate];
        _autoScrollTimer = nil;
    }
}

#pragma mark -
#pragma mark iCarousel methods

- (NSUInteger)numberOfItemsInCarousel:(iCarousel *)carousel
{
    return _games != nil ? _games.count : 0;
}

- (UIView *)carousel:(iCarousel *)carousel viewForItemAtIndex:(NSUInteger)index reusingView:(UIView *)view
{
    // Create new view if no view is available for recycling
    if (view == nil)
    {
		view = [[[NSBundle mainBundle] loadNibNamed:@"CarouselThumbnailCell" owner:self options:nil] firstObject];
		//[view setFrame:ITEM_FRAME];
    }
    
    CarouselThumbnailCell* cell = (CarouselThumbnailCell*) view;
    cell.layer.cornerRadius = 5;
    [cell setGameData:_games[index]];
    
    return view;
}

- (CGFloat)carousel:(iCarousel *)carousel valueForOption:(iCarouselOption)option withDefault:(CGFloat)value
{
    //customize carousel display
    switch (option)
    {
        case iCarouselOptionWrap:
        {
            return YES;
        }
        case iCarouselOptionSpacing:
        {
            return 0.7f;
        }
        case iCarouselOptionTilt:
        {
            return 0.5f;
        }
        default:
        {
            return value;
        }
    }
}

- (void)carousel:(iCarousel *)carousel didSelectItemAtIndex:(NSInteger)index
{
    if(_gameSelectedHandler != nil)
    {
        RBXGameData* gameData = _games[index];
        _gameSelectedHandler(gameData);
    }
}

- (void)carouselWillBeginDragging:(__unused iCarousel *)carousel
{
    [self killAutoScrollTimer];
}
- (void)carouselDidEndDragging:(__unused iCarousel *)carousel willDecelerate:(__unused BOOL)decelerate
{
    [self initAutoScrollTimer];
}

@end
