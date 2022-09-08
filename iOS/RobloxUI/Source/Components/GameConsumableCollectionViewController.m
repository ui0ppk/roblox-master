//
//  GameSortHorizontalViewController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/28/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GameConsumableCollectionViewController.h"
#import "GameConsumableCollectionViewCell.h"
#import "RobloxData.h"
#import "RobloxTheme.h"
#import "RobloxNotifications.h"
#import "RBConsumablePurchaseViewController.h"

#define NUM_GAMES_IN_CONTROLLER 10
#define ITEM_SIZE CGSizeMake(150, 207)
#define THUMBNAIL_SIZE CGSizeMake(420, 420)

@interface GameConsumableCollectionViewController ()

@end

@implementation GameConsumableCollectionViewController
{
    IBOutlet UILabel *_title;
    IBOutlet UICollectionView* _collectionView;
    
    NSString* _gameTitle;
    NSArray* _passes;
    NSArray* _gear;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(onGameItemsUpdated:) name:RBX_NOTIFY_GAME_ITEMS_UPDATED object:nil];
    if(self)
    {
        
    }
    return self;
}

- (void)onGameItemsUpdated:(NSNotification*)notification
{
    [_collectionView reloadData];
}


- (void)viewDidLoad
{
    [super viewDidLoad];
	
	[RobloxTheme applyToGameSortTitle:_title];
	
    // Setup the collection view
    [_collectionView registerNib:[UINib nibWithNibName:@"GameConsumableCollectionViewCell" bundle: nil] forCellWithReuseIdentifier:@"ReuseCell"];
    [_collectionView setBackgroundView:nil];
    [_collectionView setBackgroundColor:[UIColor clearColor]];
    
    UICollectionViewFlowLayout *flowLayout = [[UICollectionViewFlowLayout alloc] init];
    [flowLayout setItemSize:ITEM_SIZE];
    [flowLayout setScrollDirection:UICollectionViewScrollDirectionHorizontal];
    [flowLayout setMinimumLineSpacing:14.0f];
    [flowLayout setSectionInset:UIEdgeInsetsMake(0.0, 29.0, 0.0, 29.0)];
    [_collectionView setCollectionViewLayout:flowLayout];
}

- (NSUInteger)numItems
{
    if(_passes != nil)
        return _passes.count;
    else if(_gear != nil)
        return _gear.count;
    else
        return 0;
}

-(void) fetchGearForPlaceID:(NSString*)placeID gameTitle:(NSString*)gameTitle completion:(void(^)())completionHandler
{
    _title.text = [NSLocalizedString(@"GearTitlePhrase", nil) uppercaseString];
    _gear = nil;
    _passes = nil;
    _gameTitle = gameTitle;
    
    [_collectionView reloadData];
    
    [RobloxData fetchGameGear:placeID startIndex:0 maxRows:10 completion:^(NSArray *gear)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            _gear = gear;
            [_collectionView reloadData];
            
            if(completionHandler != nil)
                completionHandler();
        });
    }];
}

-(void) fetchPassesForPlaceID:(NSString*)placeID gameTitle:(NSString*)gameTitle completion:(void(^)())completionHandler
{
    _title.text = [NSLocalizedString(@"PassesTitlePhrase", nil) uppercaseString];
    _gear = nil;
    _passes = nil;
    _gameTitle = gameTitle;
    
    [_collectionView reloadData];
    
    [RobloxData fetchGamePasses:placeID startIndex:0 maxRows:10 completion:^(NSArray *passes)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            _passes = passes;
            [_collectionView reloadData];
            
            if(completionHandler != nil)
                completionHandler();
        });
    }];
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    if(_passes != nil)
        return _passes.count;
    else if(_gear != nil)
        return _gear.count;
    else
        return 0;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    GameConsumableCollectionViewCell *cell = [_collectionView dequeueReusableCellWithReuseIdentifier:@"ReuseCell" forIndexPath:indexPath];
    cell.layer.cornerRadius = 2;
    
    if(_passes != nil)
    {
        RBXGamePass* pass = _passes[indexPath.row];
        [cell setGamePassData:pass];
    }
    else if(_gear != nil)
    {
        RBXGameGear* gear = _gear[indexPath.row];
        [cell setGameGearData:gear];
    }
    else
    {
        [cell setGameGearData:nil];
    }
    
    return cell;
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    RBConsumablePurchaseViewController* controller = [[RBConsumablePurchaseViewController alloc] initWithNibName:@"RBConsumablePurchaseViewController" bundle:nil];

    controller.modalPresentationStyle = UIModalPresentationFormSheet;
    controller.gameTitle = _gameTitle;
    
    if(_passes != nil)
    {
        controller.passData = _passes[indexPath.row];
    }
    else if(_gear != nil)
    {
        controller.gearData = _gear[indexPath.row];
    }
    
    [self.navigationController presentViewController:controller animated:YES completion:nil];
}

@end
