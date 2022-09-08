//
//  GameBadgeViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 10/16/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GameBadgeViewController.h"
#import "RBActivityIndicatorView.h"
#import "UIView+Position.h"
#import "RobloxImageView.h"

#define DEFAULT_VIEW_SIZE CGRectMake(0, 0, 600, 390)

@interface GameBadgeViewController ()

@end

@implementation GameBadgeViewController
{
    IBOutlet UINavigationItem* _navBar;
    
    IBOutlet RobloxImageView* _assetThumbnail;
    IBOutlet RobloxImageView* _avatarThumbnail;
    
    IBOutlet UILabel* _lblCreatorWord;
    IBOutlet UILabel* _lblCreatorName;
    IBOutlet UILabel* _lblCreatedWord;
    IBOutlet UILabel* _lblCreatedDate;
    IBOutlet UILabel* _lblUpdatedWord;
    IBOutlet UILabel* _lblUpdatedDate;
    
    IBOutlet UILabel* _lblRarityWord;
    IBOutlet UILabel* _lblRarityDescription;
    IBOutlet UILabel* _lblWonYesterdayWord;
    IBOutlet UILabel* _lblWonYesterdayCount;
    IBOutlet UILabel* _lblWonEverWord;
    IBOutlet UILabel* _lblWonEverCount;
    
    IBOutlet UITextView* _lblDescription;
    
    RBXBadgeInfo* _badgeInfo;
}

-(id) initWithBadgeInfo:(RBXBadgeInfo*)aBadge
{
    self = [super initWithNibName:@"GameBadgeViewController" bundle:nil];
    self.modalPresentationStyle = UIModalPresentationFormSheet;
    
    _badgeInfo = aBadge;
    
    return self;
}

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    //localized the strings
    [_lblCreatorWord setText:NSLocalizedString(@"BadgeCreatorWord", nil)];
    [_lblCreatedWord setText:NSLocalizedString(@"BadgeCreatedWord", nil)];
    [_lblUpdatedWord setText:NSLocalizedString(@"BadgeUpdatedWord", nil)];
    [_lblRarityWord  setText:NSLocalizedString(@"BadgeRarityWord",  nil)];
    [_lblWonYesterdayWord setText:NSLocalizedString(@"BadgeWonYesterdayWord", nil)];
    [_lblWonEverWord setText:NSLocalizedString(@"BadgeWonEverWord", nil)];
    
    
    [_navBar setTitle:_badgeInfo.name != nil ? _badgeInfo.name : @"Badge"];
    
    //populate the labels and images with the badge information
    _lblCreatorName.hidden = YES;
    _lblCreatedDate.hidden = YES;
    _lblCreatedWord.hidden = YES;
    _lblUpdatedDate.hidden = YES;
    _lblUpdatedWord.hidden = YES;
    //[self showLabel:_lblCreatedDate andTitle:_lblCreatedWord ifStringExists:_badgeInfo.badgeCreatedDate];
    //[self showLabel:_lblUpdatedDate andTitle:_lblUpdatedWord ifStringExists:_badgeInfo.badgeUpdatedDate];
    [self showLabel:_lblWonEverCount andTitle:_lblWonEverWord ifStringExists:_badgeInfo.badgeTotalAwardedEver];
    [self showLabel:_lblWonYesterdayCount andTitle:_lblWonYesterdayWord ifStringExists:_badgeInfo.badgeTotalAwardedYesterday];
    
    //handle special cases
    if (_badgeInfo.badgeDescription)
    {
        [_lblDescription setText:_badgeInfo.badgeDescription];
        //[RobloxTheme applyToGamePreviewDescription:_lblDescription];
        _lblDescription.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:20];
    }
    else
        _lblDescription.hidden = YES;

    
    
    if (_badgeInfo.badgeRarity && _badgeInfo.badgeRarityName)
        [_lblRarityDescription setText:[NSString stringWithFormat:@"%.2f : %@", _badgeInfo.badgeRarity, _badgeInfo.badgeRarityName]];
    else
    {
        _lblRarityDescription.hidden = YES;
        _lblRarityWord.hidden = YES;
    }

    
    [self loadBadgeImage];
    [self loadCreatorImage];
}
- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = DEFAULT_VIEW_SIZE;
}

-(void) loadBadgeImage
{
    //load the badge image
    RBActivityIndicatorView* loadingSpinner1 = [[RBActivityIndicatorView alloc] initWithFrame:CGRectMake(0, 0, 32, 32)];
    [self.view addSubview:loadingSpinner1];
    [loadingSpinner1 startAnimating];
    [loadingSpinner1 centerInFrame:_assetThumbnail.frame];
    
    [_assetThumbnail loadBadgeWithURL:_badgeInfo.imageURL withSize:_assetThumbnail.frame.size completion:^
    {
        [loadingSpinner1 stopAnimating];
        [loadingSpinner1 removeFromSuperview];
    }];
    
    //poll the server for a higher resolution badge image
    /*[RobloxData fetchURLForAssetID:_badgeInfo.badgeAssetId withSize:CGSizeMake(230, 230) completion:^(NSString *imageURL)
    {
        //any image request for badges outside the fixed 75x75 throws a db error
        NSLog(@"found better resolution @ %@ vs original @ %@", imageURL, _badgeInfo.imageURL);
        if (imageURL)
            [_assetThumbnail loadBadgeWithURL:imageURL withSize:_assetThumbnail.frame.size completion:nil];
    }];*/
}

-(void) loadCreatorImage
{
    NSNumber* creatorID = [NSNumber numberWithInteger:_badgeInfo.badgeCreatorId.integerValue];
    creatorID = creatorID.integerValue == 0 ? [NSNumber numberWithInt:1] : creatorID;
    
    RBActivityIndicatorView* loadingSpinner2 = [[RBActivityIndicatorView alloc] initWithFrame:CGRectMake(0, 0, 16, 16)];
    [self.view addSubview:loadingSpinner2];
    [loadingSpinner2 startAnimating];
    [loadingSpinner2 centerInFrame:_avatarThumbnail.frame];
    
    //load the image of the creator
    [_avatarThumbnail loadAvatarForUserID:creatorID.intValue withSize:[RobloxTheme sizeProfilePictureMedium] completion:^
    {
        [loadingSpinner2 stopAnimating];
        [loadingSpinner2 removeFromSuperview];
    }];
    
    [RobloxData fetchUserProfile:creatorID
                      avatarSize:[RobloxTheme sizeProfilePictureMedium]
                      completion:^(RBXUserProfileInfo *profile)
    {
        dispatch_async(dispatch_get_main_queue(), ^
        {
            [self showLabel:_lblCreatorName andTitle:_lblCreatorWord ifStringExists:profile.username];
        });
        
    }];
    
}

- (IBAction)tabCloseButton:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}
    
    
-(void) showLabel:(UILabel*)aLabel
         andTitle:(UILabel*)titleLabel
   ifStringExists:(NSString*)someText
{
    bool exists = (someText != nil) && (someText.length > 0);
    aLabel.hidden = !exists;
    titleLabel.hidden = !exists;
    
    if (exists)
        [aLabel setText:someText];
}
  
@end
