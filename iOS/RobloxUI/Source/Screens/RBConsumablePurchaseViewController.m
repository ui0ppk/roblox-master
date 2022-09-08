//
//  RBPurchaseConsumableViewController.m
//  RobloxMobile
//
//  Created by alichtin on 10/1/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBConsumablePurchaseViewController.h"
#import "RobloxTheme.h"
#import "RobloxImageView.h"
#import "UIView+Position.h"
#import "RobloxData.h"
#import "RobloxHUD.h"
#import "RBVotesView.h"
#import "UserInfo.h"
#import "RBFavoritesView.h"
#import "RBProfileViewController.h"
#import "RobloxNotifications.h"

#define DEFAULT_VIEW_SIZE CGRectMake(0, 0, 600, 512)
#define ASSET_THUMBNAIL_SIZE CGSizeMake(420, 230)
#define SELLER_THUMBNAIL_SIZE CGSizeMake(110, 110)

@interface RBConsumablePurchaseViewController ()

@end

@implementation RBConsumablePurchaseViewController
{
    IBOutlet UINavigationBar* _navBar;
    IBOutlet UIView* _detailView;
    IBOutlet UIView* _descriptionView;
    
    IBOutlet RobloxImageView* _assetThumbnail;
    IBOutlet RobloxImageView* _sellerThumbnail;
    IBOutlet UILabel* _sellerLabel;
    IBOutlet UIButton* _sellerButton;
    IBOutlet UIButton* _chevronButton;
    IBOutlet UIButton* _buyWithRobuxButton;
    IBOutlet UIButton* _buyWithTicketsButton;
    IBOutlet UILabel* _descriptionTitleLabel;
    IBOutlet UITextView* _descriptionTextView;
    IBOutlet RBVotesView* _passVotesView;
    IBOutlet RBFavoritesView* _favoritesView;
    
    IBOutlet UIView* _confirmPurchaseView;
    IBOutlet UITextView* _confirmPurchaseTitle;
    IBOutlet UITextView* _confirmPurchaseSubtitle;
    IBOutlet UIButton* _confirmPurchaseButton;
    
    RBXCurrencyType _purchaseCurrency;
    NSUInteger _purchasePrice;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    [RobloxTheme applyToModalPopupNavBar:_navBar];
    [RobloxTheme applyToConsumableSellerTitle:_sellerLabel];
    [RobloxTheme applyToConsumableSellerButton:_sellerButton];
    [RobloxTheme applyToBuyWithRobuxButton:_buyWithRobuxButton];
    [RobloxTheme applyToBuyWithTixButton:_buyWithTicketsButton];
    [RobloxTheme applyToConsumableDescriptionTitle:_descriptionTitleLabel];
    [RobloxTheme applyToConsumableDescriptionTextView:_descriptionTextView];
    [RobloxTheme applyToBuyWithRobuxButton:_confirmPurchaseButton];
    
    if(_gearData != nil)
    {
        _navBar.topItem.title = _gearData.name;
        
        [_assetThumbnail loadWithAssetID:[_gearData.assetID stringValue] withSize:ASSET_THUMBNAIL_SIZE completion:nil];
        [_sellerThumbnail loadAvatarForUserID:[_gearData.sellerID intValue] withSize:SELLER_THUMBNAIL_SIZE completion:nil];
        
        _sellerLabel.text = [NSLocalizedString(@"ByWord", nil) stringByAppendingString:@": "];
        [_sellerButton setTitle:_gearData.sellerName forState:UIControlStateNormal];
        
        _buyWithRobuxButton.hidden = _gearData.priceInRobux == 0 || _gearData.userOwns;
        _buyWithTicketsButton.hidden = _gearData.priceInTickets == 0 || _gearData.userOwns;

        if(_buyWithRobuxButton.hidden == NO)
        {
            [_buyWithRobuxButton setTitle:[NSString stringWithFormat:@" %lu", (unsigned long)_gearData.priceInRobux] forState:UIControlStateNormal];
        }
        
        if(_buyWithTicketsButton.hidden == NO)
        {
            [_buyWithTicketsButton setTitle:[NSString stringWithFormat:@" %lu", (unsigned long)_gearData.priceInTickets] forState:UIControlStateNormal];
        }
        
        [_favoritesView setFavoritesForGear:_gearData];
        
        _descriptionTitleLabel.text = NSLocalizedString(@"DescriptionWord", nil);
        _descriptionTextView.text = _gearData.gearDescription;
        
        _passVotesView.hidden = YES;
    }
    else if(_passData != nil)
    {
        _navBar.topItem.title = _passData.passName;
        
        [_assetThumbnail loadWithAssetID:[_passData.passID stringValue] withSize:ASSET_THUMBNAIL_SIZE completion:nil];
        _sellerThumbnail.hidden = YES;
        
        _sellerLabel.text = [NSString stringWithFormat:@"%@: %@", NSLocalizedString(@"GamePassForPhrase", nil), _gameTitle];
        _sellerLabel.width = _sellerLabel.width + 52;
        _sellerLabel.x = 252;
        _sellerButton.hidden = YES;
        _chevronButton.hidden = YES;
        
        [_passVotesView setVotesForPass:_passData];
        
        _buyWithRobuxButton.hidden = _passData.priceInRobux == 0 || _passData.userOwns;
        _buyWithTicketsButton.hidden = _passData.priceInTickets == 0  || _passData.userOwns;
        
        if(_buyWithRobuxButton.hidden == NO)
        {
            [_buyWithRobuxButton setTitle:[NSString stringWithFormat:@" %lu", (unsigned long)_passData.priceInRobux] forState:UIControlStateNormal];
        }
        
        if(_buyWithTicketsButton.hidden == NO)
        {
            [_buyWithTicketsButton setTitle:[NSString stringWithFormat:@" %lu", (unsigned long)_passData.priceInTickets] forState:UIControlStateNormal];
        }
        
        [_favoritesView setFavoritesForPass:_passData];
        
        _descriptionTitleLabel.text = NSLocalizedString(@"DescriptionWord", nil);
        _descriptionTextView.text = _passData.passDescription;
    }
    
    _confirmPurchaseView.hidden = YES;
    [_confirmPurchaseButton setTitle:NSLocalizedString(@"BuyWord", nil) forState:UIControlStateNormal];
}

- (void)viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    
    self.view.superview.bounds = DEFAULT_VIEW_SIZE;
}

- (void) goToConfirmPurchase
{
    NSString* itemName;
    NSString* priceText = [NSString stringWithFormat:(_purchaseCurrency == RBXCurrencyTypeRobux ? @"R$%lu" : @"TX$%lu"), (unsigned long)_purchasePrice];
    NSString* confirmTitleText;
    
    if(_gearData != nil)
    {
        itemName = _gearData.name;
        
        confirmTitleText = [NSString stringWithFormat:NSLocalizedString(@"ConfirmGearPurchaseTitle", nil),
            _gearData.name,
            _gearData.sellerName,
            priceText];
    }
    else // if(_passData != nil)
    {
        itemName = _passData.passName;
        
        confirmTitleText = [NSString stringWithFormat:NSLocalizedString(@"ConfirmPassPurchaseTitle", nil),
            _passData.passName,
            priceText];
    }

    // Decorate title
    {
        NSMutableAttributedString* confirmTitle = [[NSMutableAttributedString alloc] initWithString:confirmTitleText
                                                                                         attributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                                                                     [UIColor colorWithWhite:(0x80/255.0f) alpha:1.0f],
                                                                                                     NSForegroundColorAttributeName,
                                                                                                     [UIFont fontWithName:@"SourceSansPro-Regular" size:18],
                                                                                                     NSFontAttributeName,
                                                                                                     nil]];

        NSRange nameRange = [confirmTitleText rangeOfString:itemName];
        NSRange priceRange = [confirmTitleText rangeOfString:priceText];
        [confirmTitle beginEditing];
        [confirmTitle addAttribute:NSFontAttributeName value:[UIFont fontWithName:@"SourceSansPro-Semibold" size:18] range:nameRange];
        [confirmTitle addAttribute:NSForegroundColorAttributeName value:[UIColor colorWithRed:(0x18/255.0) green:(0xB6/255.0) blue:(0x5B/255.0) alpha:1.0f] range:priceRange];
        [confirmTitle endEditing];
        
        _confirmPurchaseTitle.attributedText = confirmTitle;
    }
    
    // Decorate subtitle
    {
        UserInfo* currentPlayer = [UserInfo CurrentPlayer];
        NSInteger currentBalance = [(_purchaseCurrency == RBXCurrencyTypeRobux ? currentPlayer.rbxBal : currentPlayer.tikBal) integerValue];
        NSInteger remainingBalance = currentBalance > _purchasePrice ? (currentBalance - _purchasePrice) : 0;
        
        NSString* remainingBalanceText = [NSString stringWithFormat:(_purchaseCurrency == RBXCurrencyTypeRobux ? @"R$%lu" : @"TX$%lu"), (long)remainingBalance];
        NSString* confirmSubtitleText = [NSString stringWithFormat:NSLocalizedString(@"BalanceSubtitlePhrase", nil), remainingBalanceText];
        
        NSMutableAttributedString* confirmSubtitle = [[NSMutableAttributedString alloc] initWithString:confirmSubtitleText
                                                                                         attributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                                                                     [UIColor colorWithWhite:(0x80/255.0f) alpha:1.0f],
                                                                                                     NSForegroundColorAttributeName,
                                                                                                     [UIFont fontWithName:@"SourceSansPro-Regular" size:14],
                                                                                                     NSFontAttributeName,
                                                                                                     nil]];
        
        NSRange priceRange = [confirmSubtitleText rangeOfString:remainingBalanceText];
        [confirmSubtitle beginEditing];
        [confirmSubtitle addAttribute:NSForegroundColorAttributeName value:[UIColor colorWithRed:(0x18/255.0) green:(0xB6/255.0) blue:(0x5B/255.0) alpha:1.0f] range:priceRange];
        [confirmSubtitle endEditing];
        
        _confirmPurchaseSubtitle.attributedText = confirmSubtitle;
    }
    
    // Animate in
    [UIView animateWithDuration:0.2 animations:^
    {
        _detailView.alpha = 0.0f;
    }
    completion:^(BOOL finished)
    {
        _detailView.hidden = YES;
        
        _confirmPurchaseView.hidden = NO;
        _confirmPurchaseView.alpha = 0;
        
        CGFloat sourcePosY = _confirmPurchaseView.y;
        _confirmPurchaseView.y = sourcePosY + 20;
        
        [UIView animateWithDuration:0.4 animations:^
        {
            _confirmPurchaseView.alpha = 1;
            _confirmPurchaseView.y = sourcePosY;
        }
        completion:nil];
    }];
}

- (IBAction)buyWithRobuxTouchUpInside:(id)sender
{
    _purchaseCurrency = RBXCurrencyTypeRobux;
    _purchasePrice = (_gearData != nil) ? _gearData.priceInRobux : _passData.priceInRobux;
    [self goToConfirmPurchase];
}

- (IBAction)buyWithTixTouchUpInside:(id)sender
{
    _purchaseCurrency = RBXCurrencyTypeTickets;
    _purchasePrice = (_gearData != nil) ? _gearData.priceInTickets : _passData.priceInTickets;
    [self goToConfirmPurchase];
}

- (IBAction)confirmPurchaseTouchUpInside:(id)sender
{
    [RobloxHUD showSpinnerWithLabel:@"" dimBackground:YES];
    
    NSUInteger productID = _gearData != nil ? [_gearData.productID unsignedIntegerValue]
                                            : [_passData.productID unsignedIntegerValue];

    [RobloxData
        purchaseProduct:productID
        currencyType:_purchaseCurrency
        purchasePrice:_purchasePrice
        completion:^(BOOL success, NSString *errorMessage)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                [RobloxHUD hideSpinner:YES];
                if(success)
                {
                    if (_gearData != nil)
                        _gearData.userOwns = YES;
                    else
                        _passData.userOwns = YES;

                    [self dismissViewControllerAnimated:YES completion:nil];
                    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_ROBUX_UPDATED object:nil];
                    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_GAME_ITEMS_UPDATED object:nil];
                    
                }
                else if(errorMessage)
                {
                    [RobloxHUD showMessage:errorMessage];
                }
            });
        }];
}

- (IBAction)closeButtonTouchUpInside:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

- (IBAction)sellerButtonTouchUpInside:(id)sender
{
    if(_gearData != nil)
    {
        NSString *storyboardName = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"UIMainStoryboardFile"];
        UIStoryboard* mainStoryboard = [UIStoryboard storyboardWithName:storyboardName bundle:[NSBundle mainBundle]];
        
        RBProfileViewController *viewController = (RBProfileViewController*) [mainStoryboard instantiateViewControllerWithIdentifier:@"RBOthersProfileViewController"];
        
        viewController.userId = _gearData.sellerID;
        
        [self.navigationController pushViewController:viewController animated:YES];
    }
}
@end
