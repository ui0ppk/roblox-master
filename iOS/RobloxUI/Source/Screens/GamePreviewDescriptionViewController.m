//
//  GamePreviewDescriptionViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 10/6/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "GamePreviewDescriptionViewController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "RobloxHUD.h"

#define CONTENT_VIEW_SIZE CGRectMake(0, 0, 540, 320)
#define CONTENT_MARGIN_SIZE CGRectMake(16, 16, 508, 288)

@implementation GamePreviewDescriptionViewController

-(id) initWithDescription:(NSString *)aDescription
{
    self = [super init];
    
    if (self)
    {
        _gameDescription = aDescription;
    }
    
    return self;
}

//view delegate functions
-(void) viewDidLoad
{
    [super viewDidLoad];
    self.view.backgroundColor = [RobloxTheme lightBackground];
    
    UIButton* close = [RobloxTheme applyCloseButtonToUINavigationItem:self.navigationItem];
    [close addTarget:self action:@selector(didPressClose:) forControlEvents:UIControlEventTouchUpInside];
    
    [self.navigationItem setTitle:NSLocalizedString(@"DescriptionWord", nil)];
    [RobloxTheme applyToModalPopupNavBar:self.navigationController.navigationBar];
    
    //initialize the label
    _lblDescription = [[UILabel alloc] initWithFrame:CONTENT_MARGIN_SIZE];
    [_lblDescription setText:_gameDescription];
    [_lblDescription setFont:[UIFont fontWithName:@"SourceSansPro-Regular" size:16]];
    _lblDescription.numberOfLines = 0;
    [_lblDescription sizeToFit];
    
    //initialize the scrollview
    CGRect contentSize = _lblDescription.frame;
    contentSize.size.height += 20;
    _svScroll = [[UIScrollView alloc] initWithFrame:CONTENT_VIEW_SIZE];
    [_svScroll addSubview:_lblDescription];
    [_svScroll setContentSize:contentSize.size];
    _svScroll.delegate = self;
    [self.view addSubview:_svScroll];
}
-(void) viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    //self.navigationController.view.bounds = CONTENT_VIEW_SIZE;
    _svScroll.frame = self.view.bounds;
    _svScroll.clipsToBounds = NO;
}

- (void) didPressClose:(id)sender { [self dismissViewControllerAnimated:YES completion:nil]; }
@end
