//
//  RBConfirmPurchaseViewController.m
//  RobloxMobile
//
//  Created by alichtin on 10/1/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBConfirmPurchaseViewController.h"
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

#define DEFAULT_VIEW_SIZE CGSizeMake(600, 254)
#define ASSET_THUMBNAIL_SIZE CGSizeMake(420, 230)
#define SELLER_THUMBNAIL_SIZE CGSizeMake(110, 110)

@interface RBConfirmPurchaseViewController ()

@end

@implementation RBConfirmPurchaseViewController
{
    IBOutlet UINavigationBar* _navBar;
    
    IBOutlet RobloxImageView* _assetThumbnail;
    IBOutlet UIView* _confirmPurchaseView;
    IBOutlet UITextView* _confirmPurchaseTitle;
    IBOutlet UITextView* _confirmPurchaseSubtitle;
    IBOutlet UIButton* _confirmPurchaseButton;
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    self.preferredContentSize = DEFAULT_VIEW_SIZE;
    
    [RobloxTheme applyToModalPopupNavBar:_navBar];
    [RobloxTheme applyToBuyWithRobuxButton:_confirmPurchaseButton];
    
    [self presentConfirmPurchaseScreen];
}

- (void) presentConfirmPurchaseScreen
{
    _navBar.topItem.title = self.productName;
    
    [_assetThumbnail loadWithAssetID:self.thumbnailAssetID withSize:ASSET_THUMBNAIL_SIZE completion:nil];
    
    NSString* priceText = [NSString stringWithFormat:@"\nR$%lu", (unsigned long)self.price];
    NSString* confirmTitleText = [NSString stringWithFormat:NSLocalizedString(@"ConfirmPurchaseTitle", nil), priceText];
    
    // Decorate title
    {
        NSMutableAttributedString* confirmTitle = [[NSMutableAttributedString alloc] initWithString:confirmTitleText
                                                                                         attributes:[NSDictionary dictionaryWithObjectsAndKeys:
                                                                                                     [UIColor colorWithWhite:(0x80/255.0f) alpha:1.0f],
                                                                                                     NSForegroundColorAttributeName,
                                                                                                     [UIFont fontWithName:@"SourceSansPro-Regular" size:18],
                                                                                                     NSFontAttributeName,
                                                                                                     nil]];

        NSRange priceRange = [confirmTitleText rangeOfString:priceText];
        [confirmTitle beginEditing];
        [confirmTitle addAttribute:NSForegroundColorAttributeName value:[UIColor colorWithRed:(0x18/255.0) green:(0xB6/255.0) blue:(0x5B/255.0) alpha:1.0f] range:priceRange];
        [confirmTitle endEditing];
        
        _confirmPurchaseTitle.attributedText = confirmTitle;
    }
    
    // Decorate subtitle
    {
        UserInfo* currentPlayer = [UserInfo CurrentPlayer];
        NSInteger currentBalance = [currentPlayer.rbxBal integerValue];
        NSInteger remainingBalance = currentBalance > self.price ? (currentBalance - self.price) : 0;
        
        NSString* remainingBalanceText = [NSString stringWithFormat:@"R$%ld", (long)remainingBalance];
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
    
    [_confirmPurchaseButton setTitle:NSLocalizedString(@"BuyWord", nil) forState:UIControlStateNormal];
}

- (IBAction)confirmPurchaseTouchUpInside:(id)sender
{
    [RobloxHUD showSpinnerWithLabel:@"" dimBackground:YES];
    
    [RobloxData
        purchaseProduct:self.productID
        currencyType:RBXCurrencyTypeRobux
        purchasePrice:self.price
        completion:^(BOOL success, NSString *errorMessage)
        {
            dispatch_async(dispatch_get_main_queue(), ^
            {
                [RobloxHUD hideSpinner:YES];
                if(success)
                {
                    [self dismissViewControllerAnimated:YES completion:nil];
                    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_ROBUX_UPDATED object:nil];
                    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_GAME_ITEMS_UPDATED object:nil];
                    [[NSNotificationCenter defaultCenter] postNotificationName:RBX_NOTIFY_GAME_PURCHASED object:nil];
                }
                else if(errorMessage)
                {
                    [RobloxHUD showMessage:errorMessage];
                }
                else
                {
                    [RobloxHUD showMessage:NSLocalizedString(@"UnknownError", nil)];
                }
            });
        }];
}

- (IBAction)closeButtonTouchUpInside:(id)sender
{
    [self dismissViewControllerAnimated:YES completion:nil];
}

@end
