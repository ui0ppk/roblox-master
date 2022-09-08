//
//  RBBadgesViewController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/7/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBBadgesViewController.h"
#import "RobloxImageView.h"
#import "RobloxData.h"
#import "UIView+Position.h"
#import "RobloxTheme.h"
#import "GameBadgeViewController.h"

#define REUSE_IDENTIFIER @"ReuseCell"
#define BADGE_SIZE CGSizeMake(110, 110)
#define ITEM_SIZE CGSizeMake(75, 75)

//--------------------------------------------

@interface RBBadgeCollectionViewCell : UICollectionViewCell

@property(strong, nonatomic) RBXBadgeInfo* badge;

@end

@implementation RBBadgeCollectionViewCell
{
    RobloxImageView* _badgeImage;
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

- (void) initElements
{
    _badgeImage = [[RobloxImageView alloc] initWithFrame:self.bounds];
    [self addSubview:_badgeImage];
}

- (void)setBadge:(RBXBadgeInfo *)badge
{
    [_badgeImage loadBadgeWithURL:badge.imageURL withSize:BADGE_SIZE completion:nil];
}

@end

//--------------------------------------------

@interface RBBadgesViewController () <UICollectionViewDelegate, UICollectionViewDataSource>

@end

@implementation RBBadgesViewController
{
    IBOutlet UILabel* _title;
    IBOutlet UICollectionView* _collectionView;
    
    NSArray* _badges;
}

- (instancetype)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if(self)
    {
        
    }
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.view.backgroundColor = [UIColor clearColor];
    
    _title.text = [NSLocalizedString(@"GameBadgesPhrase", nil) uppercaseString];
    [RobloxTheme applyToGameSortTitle:_title];
    
    [_collectionView registerClass:[RBBadgeCollectionViewCell class] forCellWithReuseIdentifier:REUSE_IDENTIFIER];
    _collectionView.backgroundView = nil;
    _collectionView.backgroundColor = [UIColor clearColor];
    _collectionView.delegate = self;
    _collectionView.dataSource = self;
    
    UICollectionViewFlowLayout *flowLayout = [[UICollectionViewFlowLayout alloc] init];
    [flowLayout setItemSize:ITEM_SIZE];
    [flowLayout setScrollDirection:UICollectionViewScrollDirectionVertical];
    [flowLayout setMinimumInteritemSpacing:36.0f];
    [flowLayout setMinimumLineSpacing:25.0f];
    [_collectionView setCollectionViewLayout:flowLayout];
    
    [RobloxData fetchGameBadges:_gameID completion:^(NSArray *badges)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            _badges = badges;
            [_collectionView reloadData];
            
            // Adjust size
            _collectionView.size = _collectionView.collectionViewLayout.collectionViewContentSize;
            _collectionView.height = _collectionView.height;
            self.view.height = CGRectGetMaxY(_collectionView.frame);
            
            if(self.completionHandler != nil)
            {
                self.completionHandler();
            }
        });
    }];
}

- (NSUInteger)numItems
{
    return _badges != nil ? _badges.count : 0;
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return self.numItems;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    RBBadgeCollectionViewCell* cell = [_collectionView dequeueReusableCellWithReuseIdentifier:REUSE_IDENTIFIER forIndexPath:indexPath];
    
    cell.badge = _badges[indexPath.row];
    
    return cell;
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    RBXBadgeInfo* badge = _badges[indexPath.row];
    GameBadgeViewController* badgePopup = [[GameBadgeViewController alloc] initWithBadgeInfo:badge];
    [self.navigationController presentViewController:badgePopup animated:YES completion:nil];
}
@end
