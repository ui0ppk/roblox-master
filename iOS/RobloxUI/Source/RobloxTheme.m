//
//  RobloxTheme.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/29/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RobloxTheme.h"
#import "RBRoundedeBorder.h"

@implementation RobloxTheme

// Style Guide
//Header Typography
+ (NSString*) fontNameSourceSansPro         { return @"SourceSansPro-Regular";  }
+ (NSString*) fontNameSourceSansProSemiBold { return @"SourceSansPro-Semibold"; }
+ (NSString*) fontNameSourceSansProLight    { return @"SourceSansPro-Light";    }

+ (UIFont*) fontH1  { return [UIFont fontWithName:[RobloxTheme fontNameSourceSansPro] size:48]; } // page headers
+ (UIFont*) fontH2  { return [UIFont fontWithName:[RobloxTheme fontNameSourceSansPro] size:36]; } // page footer
+ (UIFont*) fontH3  { return [UIFont fontWithName:[RobloxTheme fontNameSourceSansPro] size:24]; } // ALL CAPS - SECTION AND FORM HEADER
+ (UIFont*) fontH4  { return [UIFont fontWithName:[RobloxTheme fontNameSourceSansProSemiBold] size:21]; } // secondary section and form header

//Body Typography
+ (UIFont*) fontBody            { return [UIFont fontWithName:[RobloxTheme fontNameSourceSansPro] size:20]; }           // body, validation error, success, item info, feed text, description
+ (UIFont*) fontBodyBold        { return [UIFont fontWithName:[RobloxTheme fontNameSourceSansProSemiBold] size:20]; }   // highlighted text, game title, username on feed
+ (UIFont*) fontBodyLarge       { return [UIFont fontWithName:[RobloxTheme fontNameSourceSansPro] size:22]; }           // horizontal tab, table header
+ (UIFont*) fontBodyLargeBold   { return [UIFont fontWithName:[RobloxTheme fontNameSourceSansProSemiBold] size:24]; }   // article title
+ (UIFont*) fontBodySmall       { return [UIFont fontWithName:[RobloxTheme fontNameSourceSansProLight] size:16]; }      // footnote, hint

//Colors
+ (UIColor*) colorRed1      { return [UIColor colorWithRed:(226.0 / 255.0) green:( 35.0 / 255.0) blue:( 26.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorRed2      { return [UIColor colorWithRed:(253.0 / 255.0) green:( 40.0 / 255.0) blue:( 67.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorRed3      { return [UIColor colorWithRed:(216.0 / 255.0) green:(107.0 / 255.0) blue:(104.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorOrange1   { return [UIColor colorWithRed:(246.0 / 255.0) green:(136.0 / 255.0) blue:(  2.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorOrange2   { return [UIColor colorWithRed:(246.0 / 255.0) green:(183.0 / 255.0) blue:(  2.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorGreen1    { return [UIColor colorWithRed:(  2.0 / 255.0) green:(183.0 / 255.0) blue:( 87.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorGreen2    { return [UIColor colorWithRed:( 63.0 / 255.0) green:(198.0 / 255.0) blue:(121.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorGreen3    { return [UIColor colorWithRed:(163.0 / 255.0) green:(226.0 / 255.0) blue:(189.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorBlue1     { return [UIColor colorWithRed:(  0.0 / 255.0) green:(162.0 / 255.0) blue:(255.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorBlue2     { return [UIColor colorWithRed:( 50.0 / 255.0) green:(181.0 / 255.0) blue:(255.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorBlue3     { return [UIColor colorWithRed:(  0.0 / 255.0) green:(116.0 / 255.0) blue:(189.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorBlue4     { return [UIColor colorWithRed:(153.0 / 255.0) green:(218.0 / 255.0) blue:(255.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorGray1     { return [UIColor colorWithRed:( 25.0 / 255.0) green:( 25.0 / 255.0) blue:( 25.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorGray2     { return [UIColor colorWithRed:(117.0 / 255.0) green:(117.0 / 255.0) blue:(117.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorGray3     { return [UIColor colorWithRed:(184.0 / 255.0) green:(184.0 / 255.0) blue:(184.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorGray4     { return [UIColor colorWithRed:(204.0 / 255.0) green:(204.0 / 255.0) blue:(204.0 / 255.0) alpha:1.0]; }
+ (UIColor*) colorGray5     { return [UIColor colorWithRed:(227.0 / 255.0) green:(227.0 / 255.0) blue:(227.0 / 255.0) alpha:1.0]; }

//Image and Thumbnail Sizes and Ratios
+ (CGSize) sizeGameCoverSquare              { return CGSizeMake(120, 120); } //CGSizeMake(128, 128); } //changed on 10/13/2015
+ (CGSize) sizeGameCoverRectangle           { return CGSizeMake(192, 108); }
+ (CGSize) sizeGameCoverRectangleLarge      { return CGSizeMake(576, 324); }
+ (CGSize) sizeProfilePictureExtraSmall     { return CGSizeMake( 48,  48); }
+ (CGSize) sizeProfilePictureSmall          { return CGSizeMake( 60,  60); }
+ (CGSize) sizeProfilePictureMedium         { return CGSizeMake(110, 110); } //CGSizeMake( 90,  90); }
+ (CGSize) sizeProfilePictureLarge          { return CGSizeMake(110, 110); } //CGSizeMake(120, 120); }
+ (CGSize) sizeProfilePictureExtraLarge     { return CGSizeMake(360, 360); }
+ (CGSize) sizeItemThumbnail                { return CGSizeMake(176, 176); }
+ (CGSize) sizeItemThumbnailLarge           { return CGSizeMake(180, 180); }
+ (CGSize) sizeItemDetail                   { return CGSizeMake(360, 360); }
+ (CGSize) sizeSponsoredEvent               { return CGSizeMake(117,  30);  }

+ (CGSize) ratioGameCoverRectangle          { return CGSizeMake(16,  9); }
+ (CGSize) ratioGameCoverSquare             { return CGSizeMake( 1,  1); }
+ (CGSize) ratioProfilePicture              { return CGSizeMake( 1,  1); }
+ (CGSize) ratioItemPicture                 { return CGSizeMake( 1,  1); }
+ (CGSize) ratioSponsoredEvent              { return CGSizeMake(39, 10); }


//Themes
+ (void) applyToHeaderText:(UILabel*)header
{
    [header setFont:[RobloxTheme fontH1]];
    [header setTextColor:[RobloxTheme colorGray1]];
}
+ (void) applyToBodyText:(UILabel*)body
{
    [body setFont:[RobloxTheme fontBody]];
    [body setTextColor:[RobloxTheme colorGray1]];
}
+ (void) applyToFootNote:(UILabel*)footnote
{
    [footnote setFont:[RobloxTheme fontBodySmall]];
    [footnote setTextColor:[RobloxTheme colorGray4]];
}
+ (void) applyToButtonText:(UIButton*)button
{
    [button.titleLabel setFont:[RobloxTheme fontBody]];
    [button.titleLabel setTextColor:[RobloxTheme colorGray4]];
}
//add more of these as necessary

+ (void) animateNavBar:(UINavigationBar*)navBar
               toColor:(UIColor*)targetColor
    withLighterColor:(UIColor*)lighterColor
          withInterval:(NSTimeInterval)interval
{
    if (interval <= 0.0)
    {
        [navBar setBarTintColor:targetColor];
    }
    else
    {
        //check if we have already animated there
        if (CGColorEqualToColor(navBar.barTintColor.CGColor, targetColor.CGColor))
        {
            //flash a brighter color instead, and then loop back
            [UIView animateWithDuration:interval * 0.5
                             animations:^{ navBar.barTintColor = lighterColor; }
                             completion:^(BOOL finished)
            {
                if (finished)
                {
                    [UIView animateWithDuration:interval * 0.5
                                     animations:^{ navBar.barTintColor = targetColor; }];
                }
            }];
        }
        else
        {
            //simply animate to the target color
            [UIView animateWithDuration:interval
                             animations:^{ navBar.barTintColor = targetColor; }];
        }
    }
}
+ (void) applyTheme:(RBXTheme)theme toViewController:(UIViewController*)vc quickly:(BOOL)isFast
{
    //This should be an RBXTheme instead of an NSInteger
    UIColor* color1;
    UIColor* color2;
    
    switch (theme)
    {
        case RBXThemeGame:
        {
            color1 = [RobloxTheme colorGreen1];
            color2 = [RobloxTheme colorGreen2];
        }break;
        case RBXThemeSocial:
        {
            color1 = [RobloxTheme colorBlue3];
            color2 = [RobloxTheme colorBlue2];
        }break;
        case RBXThemeCreative:
        {
            color1 = [RobloxTheme colorOrange1];
            color2 = [RobloxTheme colorOrange2];
        }break;
        default: //case RBXThemeGeneric:
        {
            color1 = [RobloxTheme colorGray1];
            color2 = [RobloxTheme colorGray2];
        }break;
    }
    
    if (vc.navigationController && vc.navigationController.navigationBar)
        [RobloxTheme animateNavBar:vc.navigationController.navigationBar
                           toColor:color1
                  withLighterColor:color2
                      withInterval:isFast ? 0.0 : 0.7];
    
    if (vc.tabBarController && vc.tabBarController.tabBar)
        [vc.tabBarController.tabBar setTintColor:color1];
}


+ (void) configureDefaultAppearance
{
    [[UINavigationBar appearance] setBarStyle:UIBarStyleBlack];
    [[UINavigationBar appearance] setTintColor:[UIColor whiteColor]];
    //[[UINavigationBar appearance] setTranslucent:NO]; <-- this crashes the app. Don't do this.
    //[[UINavigationBar appearance] setBarTintColor:[UIColor colorWithRed:0.2549f green:0.3882f blue:0.6f alpha:1.0f]];
    
    [[UITabBarItem appearance] setTitleTextAttributes:@{ NSFontAttributeName:[UIFont fontWithName:@"SourceSansPro-Regular" size:10.0] } forState:UIControlStateNormal];
    [[UITabBarItem appearance] setTitleTextAttributes:@{ NSFontAttributeName:[UIFont fontWithName:@"SourceSansPro-Regular" size:10.0] } forState:UIControlStateSelected];
}

+ (CGFloat) scaleFactor
{
    return [UIScreen mainScreen].scale;
}

+ (UIColor*) lightBackground
{
    return [UIColor colorWithRed:(242/255.0f) green:(243/255.f) blue:(247/255.f) alpha:1.0f];
}

+ (UIImageRenderingMode) iconColoringMode
{
    return UIImageRenderingModeAlwaysTemplate;
}


+ (UIButton*)applyCloseButtonToUINavigationItem:(UINavigationItem*)item {
    UIButton* close = [UIButton buttonWithType:UIButtonTypeCustom];
    close.frame = CGRectMake(0.0, 0.0, 16.0, 22.0);
    [close setImage:[UIImage imageNamed:@"Close Button"] forState:UIControlStateNormal];
    item.leftBarButtonItem = [[UIBarButtonItem alloc] initWithCustomView:close];
    
    return close;
}
+ (UIButton*)applyCloseButtonToWebUINavigationItem:(UINavigationItem*)item {
    UIButton* close = [UIButton buttonWithType:UIButtonTypeCustom];
    close.frame = CGRectMake(0.0, 0.0, 16.0, 22.0);
    [close setImage:[UIImage imageNamed:@"Close Button"] forState:UIControlStateNormal];
    item.rightBarButtonItem = [[UIBarButtonItem alloc] initWithCustomView:close];
    
    return close;
}

+ (void) applyShadowToView:(UIView*)view
{
    [view.layer setShadowColor:[UIColor blackColor].CGColor];
    [view.layer setShadowOpacity:0.4];
    [view.layer setShadowRadius:2.0];
    [view.layer setShadowOffset:CGSizeMake(0.0, 0.5)];
}

+ (void) applyRoundedBorderToView:(UIView*)view
{
    RBRoundedeBorder* roundedBorder = [[RBRoundedeBorder alloc] init];
    [view addSubview:roundedBorder];
}

+ (void) applyToWelcomeLoginButton:(UIButton*)button
{
    [button setTitleColor:[UIColor colorWithRed:(0x6F/255.0f) green:(0x6F/255.0f) blue:(0x6F/255.0f) alpha:1.0f] forState:UIControlStateNormal];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:24]];
}

+ (void) applyToFacebookButton:(UIButton*)button
{
    //add the FB logo to the button
    UIImageView *fbLogo = [[UIImageView alloc] initWithImage:[UIImage imageNamed:@"FB-Icon-White-Flat"]];
    CGRect frame = fbLogo.frame;
    frame.origin = CGPointMake(20, button.frame.size.height * 0.25);
    frame.size = CGSizeMake(button.frame.size.height - (frame.origin.y * 2), button.frame.size.height - (frame.origin.y * 2));
    fbLogo.frame = frame;
    fbLogo.contentMode = UIViewContentModeScaleAspectFit;
    
    if ([button subviews].count > 1)
    {
        NSArray* views = [button subviews];
        for (UIView* aView in views)
            if ([aView.class isSubclassOfClass:fbLogo.class])
                [aView removeFromSuperview];
        
    }
    [button addSubview:fbLogo];
    
    //style the button
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [button.titleLabel setTextColor:[UIColor whiteColor]];
    [button.titleLabel setFont:[RobloxTheme fontBody]];
    [button setTitleEdgeInsets:UIEdgeInsetsMake(0, CGRectGetMaxX(fbLogo.frame), 0, 0)];
    [button setBackgroundImage:nil forState:UIControlStateNormal];
    [button setBackgroundColor:[UIColor colorWithRed:(59.0/255.0) green:(89.0/255.0) blue:(152.0/255.0) alpha:1.0]];
    [button.layer setCornerRadius:5.0f];
    
    [RobloxTheme applyShadowToView:button];
}

+ (void) applyToModalPopupNavBar:(UINavigationBar*)navBar
{
    [navBar setBackgroundColor:[UIColor whiteColor]];
    [navBar setTintColor:[UIColor colorWithRed:(0x42/255.f) green:(0x52/255.f) blue:(0x8F/255.f) alpha:1.0f]];
    [navBar setBarTintColor:[UIColor whiteColor]];
	
    UIGraphicsBeginImageContextWithOptions(CGSizeMake(1, 1), NO, 0.0);
    UIImage *blank = UIGraphicsGetImageFromCurrentImageContext();
    UIGraphicsEndImageContext();
    
    [navBar setBackgroundImage:blank forBarMetrics:UIBarMetricsDefault];
    [navBar setShadowImage:[UIImage imageNamed:@"Header Line"]];
    [navBar setTitleTextAttributes:
        [NSDictionary dictionaryWithObjectsAndKeys:
            [UIColor colorWithRed:(0x42/255.f) green:(0x52/255.f) blue:(0x8F/255.f) alpha:1.0f], NSForegroundColorAttributeName,
            [UIFont fontWithName:@"SourceSansPro-Regular" size:18], NSFontAttributeName,
            nil]];
}

+ (void) applyToModalPopupNavBarButton:(UIBarButtonItem*)button
{
    [button setTitleTextAttributes:
     [NSDictionary dictionaryWithObjectsAndKeys:
      [UIColor colorWithRed:(0x42/255.f) green:(0x52/255.f) blue:(0x8F/255.f) alpha:1.0f], NSForegroundColorAttributeName,
      [UIFont fontWithName:@"SourceSansPro-Semibold" size:18], NSFontAttributeName,
      nil]
    forState:UIControlStateNormal];
}

+ (void) applyToModalLoginButton:(UIButton*)button
{
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:24]];
}

+ (void) applyToModalLoginTextField:(UITextField*)textfield
{
    [textfield setBorderStyle:UITextBorderStyleNone];
    [textfield setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:18]];
    [textfield setTextColor:[UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f]];
    [textfield setContentVerticalAlignment:UIControlContentVerticalAlignmentBottom];
}

+ (void) applyToModalLoginTitleLabel:(UILabel*)label
{
    label.font = [RobloxTheme fontBody]; //[UIFont fontWithName:@"SourceSansPro-Regular" size:18];
    label.textColor = [UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f];
}

+ (void) applyToModalLoginHintLabel:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Light" size:12];
    label.textColor = [UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f];
}

+ (void) applyToModalLoginSelectableButton:(UIButton*)button
{
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:18]];
    [button setTitleColor:[UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f] forState:UIControlStateNormal];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateHighlighted];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected];
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateSelected | UIControlStateHighlighted];
    
    [button setBackgroundImage:[UIImage imageNamed:@"Gender Button"] forState:UIControlStateNormal];
    [button setBackgroundImage:[UIImage imageNamed:@"Gender Button Down"] forState:UIControlStateSelected];
    [button setBackgroundImage:[UIImage imageNamed:@"Gender Button Down"] forState:UIControlStateSelected | UIControlStateHighlighted];
}

+ (void) applyToModalCancelButton:(UIButton *)button
{
    [button setTitleColor:[RobloxTheme colorGray3] forState:UIControlStateNormal];
    [button.layer setCornerRadius:5];
    [button.layer setBorderColor:[RobloxTheme colorGray4].CGColor];
    [button.layer setBorderWidth:1.0];
    [button.titleLabel setFont:[self fontBody]]; //[UIFont systemFontOfSize:15]];
}

+ (void) applyToModalSubmitButton:(UIButton *)button
{
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [button setBackgroundColor:[RobloxTheme colorBlue1]];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:18]];
    [button.layer setCornerRadius:5];
    [button.titleLabel setFont:[self fontBody]]; //[UIFont systemFontOfSize:15]];
}
+ (void) applyToDisabledModalSubmitButton:(UIButton *)button
{
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [button setBackgroundColor:[RobloxTheme colorGray5]];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:18]];
    [button.layer setCornerRadius:5];
    [button.titleLabel setFont:[self fontBody]]; //[UIFont systemFontOfSize:15]];
}
+ (void) applyToGamesNavBar:(UINavigationBar*)navBar
{
	[navBar setBarStyle:UIBarStyleBlack];
	[navBar setTintColor:[UIColor whiteColor]];

    [navBar setBarTintColor:[UIColor colorWithRed:0.2549f green:0.3882f blue:0.6f alpha:1.0f]];
}

+ (void) applyToSearchNavItem:(UISearchBar*)searchBar
{
	// Find the internal textfield in the search bar
	UITextField* searchBarTextField = nil;
	
    for (UIView *subView in searchBar.subviews)
    {
        subView.clipsToBounds = NO;
        for (UIView *secondLevelSubview in subView.subviews)
        {
            secondLevelSubview.clipsToBounds = NO;
            if ([secondLevelSubview isKindOfClass:[UITextField class]])
            {
                searchBarTextField = (UITextField *)secondLevelSubview;
                break;
            }
        }
    }
	
	searchBar.tintColor = [UIColor whiteColor];
	searchBar.backgroundColor = [UIColor clearColor];
	
	if(searchBarTextField != nil)
	{
		UIImageView *iconView = (id)searchBarTextField.leftView;
		iconView.image = [iconView.image imageWithRenderingMode:UIImageRenderingModeAlwaysTemplate];
		iconView.tintColor = [UIColor whiteColor];

		searchBarTextField.textColor = [UIColor whiteColor];
		searchBarTextField.tintColor = [UIColor whiteColor];
		searchBarTextField.clipsToBounds = NO;
		
		searchBarTextField.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14];
		
		[searchBarTextField.subviews[0] removeFromSuperview];
		
		searchBarTextField.layer.cornerRadius=5.0f;
		searchBarTextField.layer.masksToBounds=NO;
		searchBarTextField.layer.borderColor=[[UIColor whiteColor]CGColor];
		searchBarTextField.layer.borderWidth= 1.0f;
		
		searchBarTextField.attributedPlaceholder = [[NSAttributedString alloc] initWithString:searchBarTextField.placeholder attributes:@{NSForegroundColorAttributeName: [UIColor whiteColor]}];
	}
}

+ (void) applyToGameSearchToolbarCell:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:20];
    label.textColor = [UIColor whiteColor];
}

+ (void) applyToGameSortSeeAllButton:(UIButton*)button
{
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:17]];
    [button setBackgroundColor:[RobloxTheme colorBlue1]];
    [button.layer setCornerRadius:5.0];
}

+ (void) applyToGameSortTitle:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Light" size:20];
    label.textColor = [UIColor colorWithRed:(0x47/255.0f) green:(0x47/255.0f) blue:(0x47/255.0f) alpha:1.0f];
}

+ (void) applyToCarouselThumbnailTitle:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:28];
    label.textColor = [UIColor whiteColor];
}

+ (void) applyToConsumableSellerTitle:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:18];
    label.textColor = [UIColor colorWithWhite:(0x80/255.0f) alpha:1.0f];
}

+ (void) applyToConsumableSellerButton:(UIButton*)button
{
    [button setTitleColor:[UIColor colorWithRed:(0x00/255.0f) green:(0xA2/255.0f) blue:(0xFF/255.0f) alpha:1.0f] forState:UIControlStateNormal];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Light" size:18]];
}

+ (void) applyToBuyWithRobuxButton:(UIButton*)button
{
    [button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Semibold" size:18]];
}

+ (void) applyToBuyWithTixButton:(UIButton*)button
{
    [button setTitleColor:[UIColor colorWithWhite:(0x80/255.0f) alpha:1.0f] forState:UIControlStateNormal];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:18]];
}

+ (void) applyToFavoriteLabel:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:18];
    label.textColor = [UIColor colorWithWhite:(0x80/255.f) alpha:1.0f];
}

+ (void) applyToConsumableDescriptionTitle:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Light" size:18];
    label.textColor = [UIColor colorWithWhite:(0x60/255.f) alpha:1.0f];
}

+ (void) applyToConsumableDescriptionTextView:(UITextView*)textView
{
    textView.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14];
    textView.textColor = [UIColor colorWithWhite:(0x80/255.f) alpha:1.0f];
}

+ (void) applyToGameThumbnailTitle:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:14];
    label.textColor = [UIColor colorWithWhite:(0x60/255.f) alpha:1.0f];
}

+ (void) applyToGamePreviewTitle:(UILabel*)title
{
    title.font = [UIFont fontWithName:@"SourceSansPro-Light" size:32];
    title.textColor = [UIColor colorWithWhite:(0x19/255.f) alpha:1.0f];
}

+ (void) applyToGamePreviewDetailTitle:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:18];
    label.textColor = [UIColor colorWithWhite:(0x80/255.f) alpha:1.0f];
}

+ (void) applyToGamePreviewDetailText:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:14];
    label.textColor = [UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f];
}

+ (void) applyToGamePreviewBuilderButton:(UIButton*)button
{
    [button setTitleColor:[UIColor colorWithRed:(0x00/255.0f) green:(0xA2/255.0f) blue:(0xFF/255.0f) alpha:1.0f] forState:UIControlStateNormal];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Light" size:18]];
}

+ (void) applyToGamePreviewDescription:(UITextView*)textview
{
    NSMutableParagraphStyle *paragraphStyle = [[NSMutableParagraphStyle alloc] init];
    paragraphStyle.lineHeightMultiple = 50.0f;
    paragraphStyle.maximumLineHeight = 50.0f;
    paragraphStyle.minimumLineHeight= 50.0f;
    
	textview.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:14];
    textview.textColor = [UIColor colorWithWhite:(0x80/255.f) alpha:1.0f];
}

+ (void) applyToConsumablePriceLabel:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:18];
    label.textColor = [UIColor colorWithRed:(0x18/255.f) green:(0xB6/255.f) blue:(0x5B/255.f) alpha:1.0f];
}

+ (void) applyToConsumableTitleLabel:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Semibold" size:14];
    label.textColor = [UIColor colorWithRed:(0x18/255.f) green:(0xB6/255.f) blue:(0x5B/255.f) alpha:1.0f];
}

+ (void) applyToGamePreviewMoreInfoTitle:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:13];
    label.textColor = [UIColor colorWithWhite:(0x80/255.f) alpha:1.0f];
}

+ (void) applyToGamePreviewMoreInfoValue:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Light" size:20];
    label.textColor = [UIColor colorWithWhite:(0x47/255.f) alpha:1.0f];
}

+ (void) applyToTableHeaderTitle:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Light" size:20];
    label.textColor = [UIColor colorWithRed:(0x47/255.0f) green:(0x47/255.0f) blue:(0x47/255.0f) alpha:1.0f];
}

+ (void) applyToProfileHeaderSubtitle:(UILabel*)subtitle
{
    subtitle.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:13];
    subtitle.textColor = [UIColor colorWithWhite:(0x80/255.0f) alpha:1.0f];
}

+ (void) applyToProfileHeaderValue:(UILabel*)header
{
    header.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:32];
    header.textColor = [UIColor colorWithRed:(0x41/255.0f) green:(0x63/255.0f) blue:(0x99/255.0f) alpha:1.0f];
}

+ (void) applyToProfileActionButton:(UIButton*)button
{
    [button setTitleColor:[UIColor colorWithRed:(0x41/255.0f) green:(0x63/255.0f) blue:(0x99/255.0f) alpha:1.0f] forState:UIControlStateNormal];
    [button.titleLabel setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:18]];
}

+ (void) applyToFriendCellLabel:(UILabel*)label
{
    label.font = [UIFont fontWithName:@"SourceSansPro-Regular" size:11];
    label.textColor = [UIColor colorWithWhite:(0x47/255.0f) alpha:1.0f];
}

@end
