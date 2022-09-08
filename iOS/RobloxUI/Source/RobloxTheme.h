//
//  RobloxTheme.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/29/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

// Move these to General Utils
#define SYSTEM_VERSION_EQUAL_TO(v)                  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedSame)
#define SYSTEM_VERSION_GREATER_THAN(v)              ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedDescending)
#define SYSTEM_VERSION_GREATER_THAN_OR_EQUAL_TO(v)  ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN(v)                 ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] == NSOrderedAscending)
#define SYSTEM_VERSION_LESS_THAN_OR_EQUAL_TO(v)     ([[[UIDevice currentDevice] systemVersion] compare:v options:NSNumericSearch] != NSOrderedDescending)

#define RBX_SCALED_DEVICE_SIZE(width,height) CGSizeMake(width * [RobloxTheme scaleFactor], height * [RobloxTheme scaleFactor])

@interface RobloxTheme : NSObject

typedef NS_ENUM(NSInteger, RBXTheme)
{
    RBXThemeGame,
    RBXThemeSocial,
    RBXThemeCreative,
    RBXThemeGeneric
};

// Style Guide
//Header Typography
+ (UIFont*) fontH1; // page headers
+ (UIFont*) fontH2; // page footer
+ (UIFont*) fontH3; // ALL CAPS - SECTION AND FORM HEADER
+ (UIFont*) fontH4; // secondary section and form header

//Body Typography
+ (UIFont*) fontBody;       // body, validation error, success, item info, feed text, description
+ (UIFont*) fontBodyBold;   // highlighted text, game title, username on feed
+ (UIFont*) fontBodyLarge;  // horizontal tab, table header
+ (UIFont*) fontBodyLargeBold; // article title
+ (UIFont*) fontBodySmall;  //footnote, hint

//Colors
+ (UIColor*) colorRed1;
+ (UIColor*) colorRed2;
+ (UIColor*) colorRed3;
+ (UIColor*) colorOrange1;
+ (UIColor*) colorOrange2;
+ (UIColor*) colorGreen1;
+ (UIColor*) colorGreen2;
+ (UIColor*) colorGreen3;
+ (UIColor*) colorBlue1;
+ (UIColor*) colorBlue2;
+ (UIColor*) colorBlue3;
+ (UIColor*) colorBlue4;
+ (UIColor*) colorGray1;
+ (UIColor*) colorGray2;
+ (UIColor*) colorGray3;
+ (UIColor*) colorGray4;
+ (UIColor*) colorGray5;

//Image and Thumbnail Sizes and Ratios
+ (CGSize) sizeGameCoverSquare;
+ (CGSize) sizeGameCoverRectangle;
+ (CGSize) sizeGameCoverRectangleLarge;
+ (CGSize) sizeProfilePictureExtraSmall;
+ (CGSize) sizeProfilePictureSmall;
+ (CGSize) sizeProfilePictureMedium;
+ (CGSize) sizeProfilePictureLarge;
+ (CGSize) sizeProfilePictureExtraLarge;
+ (CGSize) sizeItemThumbnail;
+ (CGSize) sizeItemThumbnailLarge;
+ (CGSize) sizeItemDetail;

+ (CGSize) ratioGameCoverRectangle;
+ (CGSize) ratioGameCoverSquare;
+ (CGSize) ratioProfilePicture;
+ (CGSize) ratioItemPicture;


//Themes
+ (void) applyToHeaderText:(UILabel*)header;
+ (void) applyToBodyText:(UILabel*)body;
+ (void) applyToFootNote:(UILabel*)footnote;
+ (void) applyToButtonText:(UIButton*)button;

+ (void) applyTheme:(RBXTheme)theme toViewController:(UIViewController*)vc quickly:(BOOL)isFast;
+ (void) applyShadowToView:(UIView*)view;

// Generic theme settings
+ (void) configureDefaultAppearance;

+ (UIColor*) lightBackground;
+ (void) applyToModalPopupNavBar:(UINavigationBar*)navBar;
+ (void) applyToModalPopupNavBarButton:(UIBarButtonItem*)button;
+ (UIButton*)applyCloseButtonToUINavigationItem:(UINavigationItem*)item;
+ (UIButton*)applyCloseButtonToWebUINavigationItem:(UINavigationItem*)item;

+ (void) applyRoundedBorderToView:(UIView*)view;

+ (CGFloat) scaleFactor;

+ (UIImageRenderingMode) iconColoringMode;

// Login
+ (void) applyToWelcomeLoginButton:(UIButton*)button;
+ (void) applyToFacebookButton:(UIButton*)button;
+ (void) applyToModalLoginButton:(UIButton*)button;
+ (void) applyToModalLoginTextField:(UITextField*)textfield;
+ (void) applyToModalLoginTitleLabel:(UILabel*)label;
+ (void) applyToModalLoginHintLabel:(UILabel*)label;
+ (void) applyToModalLoginSelectableButton:(UIButton*)button;
+ (void) applyToModalCancelButton:(UIButton*)button;
+ (void) applyToModalSubmitButton:(UIButton*)button;
+ (void) applyToDisabledModalSubmitButton:(UIButton *)button;

// Games
+ (void) applyToGamesNavBar:(UINavigationBar*)navBar;
+ (void) applyToSearchNavItem:(UISearchBar*)searchBar;
+ (void) applyToGameSearchToolbarCell:(UILabel*)label;
+ (void) applyToGameSortSeeAllButton:(UIButton*)button;
+ (void) applyToGameSortTitle:(UILabel*)label;
+ (void) applyToGameThumbnailTitle:(UILabel*)label;
+ (void) applyToCarouselThumbnailTitle:(UILabel*)label;

// Consumable
+ (void) applyToConsumableSellerTitle:(UILabel*)label;
+ (void) applyToConsumableSellerButton:(UIButton*)button;
+ (void) applyToBuyWithRobuxButton:(UIButton*)button;
+ (void) applyToBuyWithTixButton:(UIButton*)button;
+ (void) applyToFavoriteLabel:(UILabel*)label;
+ (void) applyToConsumableDescriptionTitle:(UILabel*)label;
+ (void) applyToConsumableDescriptionTextView:(UITextView*)textView;

// Game Preview
+ (void) applyToGamePreviewTitle:(UILabel*)title;
+ (void) applyToGamePreviewDetailTitle:(UILabel*)label;
+ (void) applyToGamePreviewDetailText:(UILabel*)label;
+ (void) applyToGamePreviewBuilderButton:(UIButton*)button;
+ (void) applyToGamePreviewDescription:(UITextView*)label;
+ (void) applyToConsumablePriceLabel:(UILabel*)label;
+ (void) applyToConsumableTitleLabel:(UILabel*)label;
+ (void) applyToGamePreviewMoreInfoTitle:(UILabel*)label;
+ (void) applyToGamePreviewMoreInfoValue:(UILabel*)label;

// Leaderboards
+ (void) applyToTableHeaderTitle:(UILabel*)label;

// Profile
+ (void) applyToProfileHeaderSubtitle:(UILabel*)subtitle;
+ (void) applyToProfileHeaderValue:(UILabel*)subtitle;
+ (void) applyToProfileActionButton:(UIButton*)button;
+ (void) applyToFriendCellLabel:(UILabel*)label;

@end
