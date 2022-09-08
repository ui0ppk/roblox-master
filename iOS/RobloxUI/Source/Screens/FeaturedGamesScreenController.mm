//
//  FeaturedGamesScreenController.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 5/23/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "AFHTTPRequestOperation.h"
#import "FeaturedGamesScreenController.h"
#import "GameSortCarouselViewController.h"
#import "GameSortHorizontalViewController.h"
#import "GameSortResultsScreenController.h"
#import "GameSearchResultsScreenController.h"
#import "RBWebGamePreviewScreenController.h"
#import "iOSSettingsService.h"
#import "MBProgressHUD.h"
#import "RBXFunctions.h"
#import "RBXUIUtil.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "RobloxData.h"
#import "RobloxNotifications.h"
#import "RobloxWebUtility.h"
#import "Flurry.h"
#import "UIPopOverController+Helpers.h"
#import "UIViewController+Helpers.h"
#import "UIScrollView+Auto.h"
#import "FastLog.h"
#import "UserInfo.h"
#import "LoginManager.h"
#import "UIView+Position.h"

#define VERTICAL_SPACING 20.0f
#define BUTTON_WIDTH 170.0f
#define BUTTON_HEIGHT 30.0f
#define GAME_SORT_CONTROLLER_VIEW_TAG 921

DYNAMIC_FASTINTVARIABLE(ROMAItemsInCarousel, 0)

//---METRICS---
#define GS_openRobux                @"GAMES SCREEN - Open Robux"
#define GS_openBuildersClub         @"GAMES SCREEN - Open Builders Club"
#define GS_openSettings             @"GAMES SCREEN - Open Settings"
#define GS_openLogout               @"GAMES SCREEN - Open Logout"
#define GS_openGameDetail           @"GAMES SCREEN - Open Game Detail"
#define GS_launchGame               @"GAMES SCREEN - Launch Game"

#define GS_searchGames              @"GAMES SCREEN - Search Games"
#define GS_gameCategoryRecommended  @"GAMES SCREEN - See All Recommended"
#define GS_gameCategoryPopular      @"GAMES SCREEN - See All Popular"
#define GS_gameCategoryTopEarning   @"GAMES SCREEN - See All Top Earning"
#define GS_gameCategoryTopPaid      @"GAMES SCREEN - See All Top Paid"
#define GS_gameCategoryTopRated     @"GAMES SCREEN - See All Top Rated"
#define GS_gameCategoryBuildersClub @"GAMES SCREEN - See All Builders Club"

#pragma mark - OptionListTableViewController class

@protocol OptionListTableViewControllerDelegate <NSObject>
@required
- (void) selectedOption:(id)option;
@end

@interface OptionListTableViewController : UITableViewController

@property (nonatomic, strong) NSMutableArray *options;
@property (nonatomic, weak) id<OptionListTableViewControllerDelegate> delegate;

@end

@implementation OptionListTableViewController

-(id)initWithStyle:(UITableViewStyle)style
{
    if ([super initWithStyle:style] != nil) {
        
        //Make row selections persist.
        self.clearsSelectionOnViewWillAppear = NO;
    }
    
    return self;
}

- (void) setOptions:(NSMutableArray *)options {
    _options = options;
    
    //Calculate how tall the view should be by multiplying
    //the individual row height by the total number of rows.
    NSInteger rowsCount = [self.options count];
    NSInteger singleRowHeight = [self.tableView.delegate tableView:self.tableView
                                           heightForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]];
    NSInteger totalRowsHeight = rowsCount * singleRowHeight;
    
    //Calculate how wide the view should be by finding how
    //wide each string is expected to be
    CGFloat largestLabelWidth = 0;
    for (id option in self.options) {
        
        if ([option isKindOfClass:[RBXGameAttribute class]]) {
            RBXGameAttribute *attribute = (RBXGameAttribute *)option;
            
            //Checks size of text using the default font for UITableViewCell's textLabel.
            NSLog(@"Option (Attribute Title): %@", attribute.title);
            
            //        CGSize labelSize = CGSizeMake(100, 30);
            CGSize labelSize = [attribute.title sizeWithAttributes:
                                @{NSFontAttributeName:
                                      [UIFont systemFontOfSize:14.0f]}];
            if (labelSize.width > largestLabelWidth) {
                largestLabelWidth = labelSize.width;
            }
        }
    }
    
    //Add a little padding to the width
    CGFloat popoverWidth = largestLabelWidth + 100;
    
    //Set the property to tell the popover container how big this view will be.
    self.preferredContentSize = CGSizeMake(popoverWidth, totalRowsHeight);
}

- (CGFloat) tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    return 50;
}

#pragma mark - Table view data source
- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    // Return the number of sections.
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    // Return the number of rows in the section.
    return [self.options count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    static NSString *CellIdentifier = @"Cell";
    UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:CellIdentifier];
    if (cell == nil) {
        cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault reuseIdentifier:CellIdentifier];
    }
    
    id data = [self.options objectAtIndex:indexPath.row];
    
    // Configure the cell...
    if ([data isKindOfClass:[RBXGameAttribute class]]) {
        RBXGameAttribute *attribute = (RBXGameAttribute *)data;
        cell.textLabel.text = attribute.title;
    }
    
    return cell;
}

#pragma mark - Table view delegate
- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:YES];
    
    id data = [self.options objectAtIndex:indexPath.row];
    
    // Configure the cell...
    if ([data isKindOfClass:[RBXGameAttribute class]]) {
        //Notify the delegate if it exists.
        if (_delegate != nil) {
            [_delegate selectedOption:data];
        }
    }
}

@end

@interface iPadDropdownButtonView : UIView

- (void) deselectedState;
- (void) selectedState;

@end

@interface iPadDropdownButtonView ()

@property (nonatomic, strong) UIButton *button;
@property (nonatomic, strong) UIImageView *imageView;

@property (nonatomic, strong) UIImage *arrowDark;
@property (nonatomic, strong) UIImage *arrowLite;

@end

@implementation iPadDropdownButtonView

- (instancetype) initWithFrame:(CGRect)frame title:(NSString *)title {
    NSLog(@"frame: %@", NSStringFromCGRect(frame));
    
    if (self == [super initWithFrame:frame]) {
        // init here
    }

    // Round the edges of the view
    self.layer.cornerRadius = 5.0;
    
    CGFloat w = frame.size.width;
    CGFloat h = frame.size.height;
    
    // Create the button
    // Be careful when adjusting the width of the imageview and the button.
    self.button = [[UIButton alloc] initWithFrame:CGRectMake(0, 0, w * .87, h)];
    self.button.clipsToBounds = YES;
    [self.button setTitle:title forState:UIControlStateNormal];
    [self addSubview:self.button];
    
    // Create the drop down indicator arrow
    // Be careful when adjusting the width of the imageview and the button.
    self.imageView = [[UIImageView alloc] initWithFrame:CGRectMake(CGRectGetMaxX(self.button.frame), 10, w * .07, 10)];
    [self addSubview:self.imageView];

    // Enable user interaction on the view so that the button is able to receive tap events
    [self setUserInteractionEnabled:YES];
    
    // Further setup
    [self setupView];
    
    return self;
}

- (void) setupView {
    // Left align the text within the button
    self.button.contentHorizontalAlignment = UIControlContentHorizontalAlignmentLeft;
    
    // Adjust the left content margin within the button
    self.button.contentEdgeInsets = UIEdgeInsetsMake(0, 10, 0, 0);
    
    // Round the edges of the button
    self.button.layer.cornerRadius = 5.0;
    
    [RBXUIUtil increaseTappableAreaOfButton:self.button];
    
    // Setup arrow images
    self.arrowDark = [UIImage imageNamed:@"down_arrow_dark.png"];
    self.arrowLite = [UIImage imageNamed:@"down_arrow_light.png"];
    
    // Put the button in the default deselected state
    [self deselectedState];
}

- (void) deselectedState {
    
    [self.imageView setImage:self.arrowDark];
    [self.imageView setContentMode:UIViewContentModeScaleAspectFit];
    
    [self.button setBackgroundColor:[UIColor whiteColor]];
    [self.button setTitleColor:[RobloxTheme colorGray2] forState:UIControlStateNormal];
    
    [self setBackgroundColor:[UIColor whiteColor]];
}

- (void) selectedState {
    
    [self.imageView setImage:self.arrowLite];
    [self.imageView setContentMode:UIViewContentModeScaleAspectFit];
    
    [self.button setBackgroundColor:[RobloxTheme colorBlue2]];
    [self.button setTitleColor:[UIColor whiteColor] forState:UIControlStateNormal];
    
    [self setBackgroundColor:[RobloxTheme colorBlue2]];
}

@end

#pragma mark - FeaturedGamesScreenController class

@interface FeaturedGamesScreenController () <UISearchBarDelegate, UISearchDisplayDelegate, OptionListTableViewControllerDelegate, UIPopoverControllerDelegate>

@property (nonatomic, strong) OptionListTableViewController *optionListTableViewController;
@property (nonatomic, strong) UIPopoverController *optionListPopoverController;
@property (nonatomic, strong) UIScrollView *scrollView;

@property (nonatomic, strong) RBXGameSort *selectedGameSort;
@property (nonatomic, strong) RBXGameGenre *selectedGameGenre;

@property (nonatomic, strong) RBXGameSort *lastSelectedSort;
@property (nonatomic, strong) RBXGameGenre *lastSelectedGenre;

@property (nonatomic, strong) NSMutableArray *allSorts;
@property (nonatomic, strong) NSArray *allGenres;

@property (nonatomic, strong) NSArray *sortsData;
@property (nonatomic) CGFloat bottom;

@property (nonatomic, strong) iPadDropdownButtonView *sortsView;
@property (nonatomic, strong) iPadDropdownButtonView *genresView;

@property (nonatomic, strong) MBProgressHUD *activityIndicator;

@end

@implementation FeaturedGamesScreenController
{
    //Views and Controllers
    UIWebView* _siteAlertWebView;
    GameSortCarouselViewController* _featuredGamesController;
    NSMutableArray* _gamesCategoriesControllers;
}


#pragma mark - View Functions

- (void) viewDidLoad {
    [super viewDidLoad];
    
    //Initialize and style the view
    [self setViewTheme:RBXThemeGame];
    [self addSearchIconWithSearchType:SearchResultGames andFlurryEvent:nil];
    self.view.backgroundColor = [RobloxTheme lightBackground];
    self.navigationItem.title = NSLocalizedString(@"GameWord", nil);
    self.edgesForExtendedLayout = UIRectEdgeNone;
    
    //initialize some variables
    _gamesCategoriesControllers = [NSMutableArray array];
    
    //check if the user is logged in to determine if we should add icons or observers
    if ([UserInfo CurrentPlayer].userLoggedIn)
        [self addRobuxIconWithFlurryEvent:GS_openRobux
                 andBCIconWithFlurryEvent:GS_openBuildersClub];
    else
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(addIcons:) name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(refreshViews) name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
    
    // Site alert banner webview
    iOSSettingsService* iOSSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    if (iOSSettings->GetValueEnableSiteAlertBanner() )
    {
        _siteAlertWebView = [[UIWebView alloc]initWithFrame:CGRectMake(0.0f, 10.0f, 1024.0f, 30.0f)];
        _siteAlertWebView.hidden = true;
        _siteAlertWebView.delegate = self;
        [self fetchSiteAlertBanner:_siteAlertWebView];
        
        [self.scrollView addSubview:_siteAlertWebView];
    }
    
    // Scroll view container
    // 2015.0902 - Adjust the Y position of the scroll view to acommodate a possible siteAlertWebView and the sort/genre buttons
    float bannerBottom = (_siteAlertWebView.hidden == NO) ? _siteAlertWebView.bottom + BUTTON_HEIGHT + VERTICAL_SPACING : 0;
    _scrollView = [[UIScrollView alloc] initWithFrame:CGRectMake(0, bannerBottom, self.view.width, self.view.height)];
    [_scrollView setScrollEnabled:YES];
    _scrollView.clipsToBounds = NO; // <--- ????
    [self.view addSubview:_scrollView];
    
    [self refreshViews];
}

- (void) showActivityIndicator {
    
    if ([RBXFunctions isEmpty:self.activityIndicator]) {
        self.activityIndicator = [[MBProgressHUD alloc] initWithView:self.view];
        [self.view addSubview:self.activityIndicator];
    }
    
    FeaturedGamesScreenController __weak *weakSelf = self;
    
    [RBXFunctions dispatchOnMainThread:^{
        [weakSelf.activityIndicator show:YES];
    }];
}

- (void) hideActivityIndicator {
    FeaturedGamesScreenController __weak *weakSelf = self;

    [RBXFunctions dispatchOnMainThread:^{
        [weakSelf.activityIndicator hide:YES];
    }];
}

- (void) viewDidLayoutSubviews {
    [super viewDidLayoutSubviews];
    
    _scrollView.frame = self.view.bounds;
    
    
    [_scrollView setContentSizeForDirection:UIScrollViewDirectionVertical];
}

- (void) viewWillAppear:(BOOL)animated {
    [[RBXEventReporter sharedInstance] reportScreenLoaded:RBXAContextGames];
    
    [self setViewTheme:RBXThemeGame];
    [super viewWillAppear:animated];
}


#pragma mark - Initialization Functions

- (void) fetchSiteAlertBanner:(UIWebView*)webView {
    NSString* urlAsString = [NSString stringWithFormat:@"%@/alerts/alert-info",
                             [RobloxInfo getApiBaseUrl]];
    NSLog(@"%@", urlAsString);
    NSURL *url = [[NSURL alloc] initWithString:urlAsString];
    NSMutableURLRequest *request = [NSMutableURLRequest requestWithURL:url];
    [request setValue:@"application/json" forHTTPHeaderField:@"Accept"];
    
    AFHTTPRequestOperation *operation = [[AFHTTPRequestOperation alloc] initWithRequest:request];
    operation.responseSerializer = [AFJSONResponseSerializer serializer];
    
    FeaturedGamesScreenController __weak *weakSelf = self;
    [operation setCompletionBlockWithSuccess:^(AFHTTPRequestOperation *operation, id responseObject)
     {
         NSDictionary* jsonResults = (NSDictionary*) responseObject;
         if (jsonResults != nil)
         {
             bool visible = [[jsonResults valueForKey:@"IsVisible"] intValue] > 0;
             
             if (visible)
             {
                 NSString* html = [jsonResults valueForKey:@"Text"];
                 
                 // If the string starts with [alertColor, we know it is not an Announcement, and will have its own bg color
                 if ([html rangeOfString:@"[alertColor"].location == 0 )
                 {
                     NSError *error = nil;
                     NSRegularExpression *regex = [NSRegularExpression regularExpressionWithPattern:@"\\Q[alertColor=\\E(.*?)\\Q]\\E" options:NSRegularExpressionCaseInsensitive error:&error];
                     html = [regex stringByReplacingMatchesInString:html options:0 range:NSMakeRange(0, [html length]) withTemplate:@"<body style='font-family: sans-serif; font-style: bold; color: #FFFFFF; border-bottom: 2px solid #333333; text-align:center; background-color: $1;'>"];
                     html = [NSString stringWithFormat:@"%@%s", html, "</body>"];
                 }
                 else
                     html = [NSString stringWithFormat:@"%s%@%s", "<body style='font-family: sans-serif; font-style: bold; color: #FFFFFF; border-bottom: 2px solid #333333; text-align:center; background-color: #FF3030;'>", html, "</body>"];
                 
                 webView.hidden = false;
                 [webView loadHTMLString:html baseURL:nil];
             }
             else
             {
                 webView.hidden = true;
             }
             
             [weakSelf refreshViews];
         }
     }
                                     failure:^(AFHTTPRequestOperation *operation, NSError *error)
     {
         NSLog(@"SiteAlert request failed.");
         webView.hidden = true;
     }];
    [operation start];
}

- (void) loadGames {
    // 2015.0902 ajain - If the selected sort is the Default sort, then we use the loadGames method to show the original arrangement of games,
    // which is five rows of games of different sort types based on genre -- Popular, Top Earning, Top Rated, Recommended, and Featured.

    __weak FeaturedGamesScreenController* weakSelf = self;
    
    // Update the "bottom" variable which is used as a Y-axis reference point to layout content in the scrollview
    self.bottom = CGRectGetMaxY(self.sortsView.frame) + VERTICAL_SPACING;
    
    //initialize the carousel
    if( DFInt::ROMAItemsInCarousel > 0 )
    {
        if (!_featuredGamesController)
        {
            _featuredGamesController = [[GameSortCarouselViewController alloc] initWithFrame:CGRectMake(0.0f, CGRectGetMaxY(self.sortsView.frame) + 50 + VERTICAL_SPACING, _scrollView.width, 180.0f)];
            _featuredGamesController.gameSelectedHandler = ^(RBXGameData* gameData) { [weakSelf showGameDetails:gameData]; };
        }
        
        if ([RBXFunctions isEmpty:_featuredGamesController.view.superview])
        {
            [_scrollView addSubview:_featuredGamesController.view];
        }
        
        self.bottom = CGRectGetMaxY(_featuredGamesController.carousel.frame) + 50 + VERTICAL_SPACING;
    }
    
    [self showActivityIndicator];
    
    //loop through all the controllers and reload the data
    [RobloxData fetchDefaultSorts:0 completion:^(NSArray *sorts)
     {
         [weakSelf hideActivityIndicator];
         
         _sortsData = sorts;
         
         // 2015.0902 ajain - We call reloadGameSortsCollectionController to layout the five rows of sorts based on genre.
         [weakSelf reloadGameSortsCollectionController];
     }];
    
}

- (void) reloadGameSortsCollectionController {

    [RBXFunctions dispatchOnMainThread:^{
        
        [self resetViews];
        
        //reference frame is out of scope and read only, copy it locally and continue
        CGFloat localBottom = self.bottom;
        
        int i = 0;
        while (i < _sortsData.count || i < _gamesCategoriesControllers.count)
        {
            if (i < _sortsData.count && i < _gamesCategoriesControllers.count)
            {
                //if we have a sort AND a category, update the view with the new sort data
                GameSortHorizontalViewController* gshvc = _gamesCategoriesControllers[i];
                [gshvc setAnalyticsLocation:RBXALocationGames andContext:RBXAContextMain];
                [gshvc setSort:_sortsData[i] andGenre:self.selectedGameGenre];
            }
            else if (i < _sortsData.count)
            {
                FeaturedGamesScreenController __weak *weakSelf = self;
                
                //we have more data than sorts, add a new game category controller
                GameSortHorizontalViewController* controller = [[GameSortHorizontalViewController alloc] initWithNibName:@"GameSortHorizontalViewController" bundle:nil];
                controller.startIndex = (i == 0) ? DFInt::ROMAItemsInCarousel : 0;
                [controller setAnalyticsLocation:RBXALocationGames andContext:RBXAContextMain];
                controller.gameSelectedHandler = ^(RBXGameData* gameData) {
                    [weakSelf showGameDetails:gameData];
                };
                controller.seeAllHandler = ^(NSNumber* sortID) {
                    [weakSelf showFullListForCategory:sortID];
                };
                [controller.view setFrame:CGRectMake(0, localBottom, _scrollView.width, controller.view.height)];
                [controller setSort:self.sortsData[i] andGenre:self.selectedGameGenre];
                
                //update the bottom of the reference frame
                localBottom = controller.view.bottom + VERTICAL_SPACING;
                
                // set a tag so we can identify these views for removal when we re-load sorts or filters
                controller.view.tag = GAME_SORT_CONTROLLER_VIEW_TAG;
                
                //keep a reference to the controller and add it to the screen
                [_gamesCategoriesControllers addObject:controller];
                [_scrollView addSubview:controller.view];
                
            }
            else// if (i < _gamesCategoriesControllers.count)
            {
                //we have more game category controllers than data, remove the category
                GameSortHorizontalViewController* controller = _gamesCategoriesControllers[i];
                [controller removeFromParentViewController];
                [_gamesCategoriesControllers removeObjectAtIndex:i];
            }
            
            //increment the counter and loop
            i++;
        }
        
        [_featuredGamesController setGameSort:self.selectedGameSort];
        
        [self updateDropDownButtonTitles];
        
        //update the scrollview size, just to be safe
        [_scrollView setContentSizeForDirection:UIScrollViewDirectionVertical];
    }];
    
}

#pragma mark - Delegate and Notification Functions

- (void) addIcons:(NSNotification*) notification {
    [RBXFunctions dispatchOnMainThread:^{
        [self addRobuxIconWithFlurryEvent:GS_openRobux
                 andBCIconWithFlurryEvent:GS_openBuildersClub];
        [self loadGames];
        
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(removeIcons:) name:RBX_NOTIFY_LOGGED_OUT object:nil];
    }];
}
-(void) removeIcons:(NSNotification*) notification {
    [RBXFunctions dispatchOnMainThread:^{
        [self removeRobuxAndBCIcons];
        
        [[NSNotificationCenter defaultCenter] removeObserver:self name:RBX_NOTIFY_LOGGED_OUT object:nil];
        [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(addIcons:) name:RBX_NOTIFY_LOGIN_SUCCEEDED object:nil];
    }];
}
- (BOOL) webView:(UIWebView *)inWeb shouldStartLoadWithRequest:(NSURLRequest *)inRequest navigationType:(UIWebViewNavigationType)inType {
    //In case someone clicks on the button in the site banner, open up the result in the browser
    if ( inType == UIWebViewNavigationTypeLinkClicked ) {
        [[UIApplication sharedApplication] openURL:[inRequest URL]];
        return NO;
    }
    
    return YES;
}

#pragma mark - Navigation Functions

- (void) showGameDetails:(RBXGameData*) gameData {
    [Flurry logEvent:GS_openGameDetail];
    
    [self pushGameDetailWithGameData:gameData];
}
- (void) showFullListForCategory:(NSNumber*)selectedSort {
    [self performSegueWithIdentifier:@"sortResults" sender:selectedSort];
    
    //Report the flurry event
    NSInteger sortID = selectedSort.integerValue;
    switch (sortID)
    {
        case (RBXGameSortTypeRecommended):  { [Flurry logEvent:GS_gameCategoryRecommended]; }   break;
        case (RBXGameSortTypePopular):      { [Flurry logEvent:GS_gameCategoryPopular]; }       break;
        case (RBXGameSortTypeTopEarning):   { [Flurry logEvent:GS_gameCategoryTopEarning]; }    break;
        case (RBXGameSortTypeTopPaid):      { [Flurry logEvent:GS_gameCategoryTopRated]; }      break;
        case (RBXGameSortTypeBuildersClub): { [Flurry logEvent:GS_gameCategoryBuildersClub];}   break;
    }
    
}
- (void) prepareForSegue:(UIStoryboardSegue *)segue sender:(id)sender {
	if( [segue.identifier isEqualToString:@"sortResults"] )
	{
		GameSortResultsScreenController* controller = segue.destinationViewController;
		controller.selectedSort = (NSNumber*) sender;
	}
	else if( [segue.identifier isEqualToString:@"searchResults"] )
	{
		GameSearchResultsScreenController* controller = segue.destinationViewController;
		controller.keywords = (NSString*) sender;
	}
}

#pragma mark -  OptionListTableViewControllerDelegate method

- (void) selectedOption:(id)option {
    NSLog(@"the selected option is %@", option);
    
    if ([option isKindOfClass:[RBXGameSort class]]) {
        self.selectedGameSort = (RBXGameSort *)option;
    } else if ([option isKindOfClass:[RBXGameGenre class]]) {
        self.selectedGameGenre = (RBXGameGenre *)option;
    }
    
    NSLog(@"Selected Sort: %@", self.selectedGameSort.title);
    NSLog(@"Selected Genre: %@", self.selectedGameGenre.title);
    
    [self updateGames];
    
    if (self.optionListPopoverController) {
        [self.optionListPopoverController dismissPopoverAnimated:YES];
        self.optionListPopoverController = nil;
    }
    
    [self updateDropDownButtonTitles];
}

#pragma mark - Drop Down List Methods

- (void) createDropDownMenuButtons {
    CGFloat yAdj = (_siteAlertWebView.hidden == NO) ? CGRectGetMaxY(_siteAlertWebView.frame) : 0;
    
    if ([RBXFunctions isEmpty:self.sortsView]) {
        self.sortsView = [[iPadDropdownButtonView alloc] initWithFrame:CGRectMake(20, yAdj + 10, BUTTON_WIDTH, BUTTON_HEIGHT) title:@"Default"];
        [self.scrollView addSubview:self.sortsView];
    }
    [self.sortsView.button addTarget:self action:@selector(dropDownListButtonTapped:) forControlEvents:UIControlEventTouchUpInside];
    self.sortsView.tag = 111;
    self.sortsView.button.showsTouchWhenHighlighted = YES;
    
    if ([RBXFunctions isEmpty:self.genresView]) {
        self.genresView = [[iPadDropdownButtonView alloc] initWithFrame:CGRectMake(CGRectGetMaxX(self.sortsView.frame) + 10, yAdj + 10, BUTTON_WIDTH, BUTTON_HEIGHT) title:@"All"];
        [self.scrollView addSubview:self.genresView];
    }
    [self.genresView.button addTarget:self action:@selector(dropDownListButtonTapped:) forControlEvents:UIControlEventTouchUpInside];
    self.genresView.tag = 112;
    self.genresView.button.showsTouchWhenHighlighted = YES;
    
    // 2015.0902 ajain - Initialize the selected game attribute values
    self.selectedGameSort = [RBXGameSort DefaultSort];
    self.selectedGameGenre = self.allGenres.firstObject;
    
    [self updateDropDownButtonTitles];
    
    // Update the "bottom" variable which is used as a Y-axis reference point to layout content in the scrollview
    self.bottom = CGRectGetMaxY(self.sortsView.frame) + VERTICAL_SPACING;
}

- (void) updateDropDownButtonTitles {
    [self.sortsView.button setTitle:self.selectedGameSort.title forState:UIControlStateNormal];
    [self.genresView.button setTitle:self.selectedGameGenre.title forState:UIControlStateNormal];
    
    if (self.selectedGameSort.sortID.integerValue == RBXGameSortTypeDefault) {
        [self.sortsView deselectedState];
    }
    
    RBXGameGenre *allGenre = self.allGenres.firstObject;
    if (self.selectedGameGenre.genreID.integerValue == allGenre.genreID.integerValue) {
        [self.genresView deselectedState];
    }
}

- (void) dropDownListButtonTapped:(id)sender {
    if ([sender isKindOfClass:[UIButton class]]) {
        
        UIView *containerView = ((UIView *)sender).superview;
        
        if (containerView == self.sortsView || containerView == self.genresView) {
            if ([RBXFunctions isEmpty:self.optionListTableViewController]) {
                self.optionListTableViewController = [[OptionListTableViewController alloc] initWithStyle:UITableViewStylePlain];
                self.optionListTableViewController.delegate = self;
            }
            
            if (containerView == self.sortsView) {
                [self.sortsView selectedState];
                
                NSMutableArray *updatedOptions = [NSMutableArray arrayWithArray:self.allSorts];
                if (![RBXFunctions isEmpty:self.selectedGameSort]) {
                    [updatedOptions insertObject:[RBXGameSort DefaultSort] atIndex:0];
                }
                
                self.optionListTableViewController.options = updatedOptions;
            } else if (containerView == self.genresView) {
                [self.genresView selectedState];

                self.optionListTableViewController.options = [self.allGenres mutableCopy];
            }
            
            if ([RBXFunctions isEmpty:self.optionListPopoverController]) {
                [RBXFunctions dispatchOnMainThread:^{
                    // Setup the UIPopoverController
                    self.optionListPopoverController = [[UIPopoverController alloc] initWithContentViewController:self.optionListTableViewController];
                    [self.optionListPopoverController presentPopoverFromRect:containerView.frame inView:self.view permittedArrowDirections:UIPopoverArrowDirectionUp animated:YES];
                    self.optionListPopoverController.delegate = self;
                    
                    // Assign the passthrough views - the views that can receive touches even when a popover is displayed
                    [self.optionListPopoverController setPassthroughViews:@[self.sortsView, self.genresView]];
                    
                    // Refresh the options available
                    [self.optionListTableViewController.tableView reloadData];
                }];
            } else {
                [RBXFunctions dispatchOnMainThread:^{
                    [self.optionListPopoverController dismissPopoverAnimated:NO completionHandler:^{
                        [self updateDropDownButtonTitles];
                        
                        // Recursively call the method when a dropdown button is pressed while a popover is being displayed
                        [self dropDownListButtonTapped:sender];
                    }];
                    
                    self.optionListPopoverController = nil;
                    
                    // Important to nil out the ListTableViewController otherwise the tableviewcontroller will be reused and you will get
                    // weird refresh issues when toggling between two different drop down menus.
                    self.optionListTableViewController = nil;

                }];
            }
        }
    }
}



// 2015.0902 ajain - Restore the previous game attribute selections
- (void) revertSelectedAttributes {
    self.selectedGameSort = self.lastSelectedSort;
    self.selectedGameGenre = self.lastSelectedGenre;
}

// 2015.0902 ajain - Update the game attribute's with the latest valid selections
- (void) updateLastSelectedAttributes {
    self.lastSelectedSort = self.selectedGameSort;
    self.lastSelectedGenre = self.selectedGameGenre;
}

- (void) updateGames
{
    NSUInteger maxNumItems = 40;
    
    if (self.selectedGameSort.sortID.integerValue == @(RBXGameSortTypeDefault).integerValue) {
        [self loadGames];
        return;
    }
    
    [self showActivityIndicator];
    
    FeaturedGamesScreenController __weak *weakSelf = self;
    
    [RobloxData fetchGameListWithSortID:self.selectedGameSort.sortID
                                genreID:self.selectedGameGenre.genreID
                               playerID:[UserInfo CurrentPlayer].userId
                              fromIndex:0
                               numGames:maxNumItems
                              thumbSize:RBX_SCALED_DEVICE_SIZE(455, 256)
                             completion:^(NSArray *games)
     {
         
         [RBXFunctions dispatchOnMainThread:^{
             [weakSelf hideActivityIndicator];
             
             NSLog(@"gamesArray count for sort:%@ : %ld", self.selectedGameSort.title, (unsigned long)games.count);
             
             if ([RBXFunctions isEmpty:games]) {
                 
                 NSMutableString *message = [NSMutableString stringWithFormat:@"There are no games"];
                 if (![RBXFunctions isEmpty:weakSelf.selectedGameSort]) {
                     NSString *sortDesc = [NSString stringWithFormat:@"\nin Sort: %@", weakSelf.selectedGameSort.title];
                     [message appendString:sortDesc];
                 }
                 
                 if (![RBXFunctions isEmpty:weakSelf.selectedGameGenre]) {
                     NSString *genreDesc = [NSString stringWithFormat:@" in Genre: %@", weakSelf.selectedGameGenre.title];
                     [message appendString:genreDesc];
                 }
                 
                 UIAlertView *alert = [[UIAlertView alloc] initWithTitle:@"Ooops"
                                                                 message:message
                                                                delegate:nil
                                                       cancelButtonTitle:@"OK"
                                                       otherButtonTitles:nil];
                 [alert show];
                 
                 // 2015.0902 ajain - Revert the newly selected attributes to what they were before because the new selections are not valid.
                 // Must be done AFTER showing the alert view, so the alert view displays the incorrect values.
                 [weakSelf revertSelectedAttributes];
                 
                 return;
                 
             } else {
                 
                 // 2015.0902 ajain - Update the last selections to what the previous selections were
                 [weakSelf updateLastSelectedAttributes];
                 [weakSelf updateDropDownButtonTitles];
             }
             
             [weakSelf reloadGameCollectionFullScreenWithGames:games];
         }];
         
     }];
}

- (void) reloadGameCollectionFullScreenWithGames:(NSArray *)games {
    [RBXFunctions dispatchOnMainThread:^{
        //reference frame is out of scope and read only, copy it locally and continue
        CGFloat localBottom = self.bottom;
        
        // 2015.0902 ajain - repurposed the reloadGameSortsCollectionController method
        
        [self resetViews];
        
        NSNumber *playerID = [UserInfo CurrentPlayer].userId;
        GameSortResultsScreenController *controller = [GameSortResultsScreenController gameSortResultsScreenControllerWithSort:self.selectedGameSort genre:self.selectedGameGenre playerID:playerID];
        
        //we have more data than sorts, add a new game category controller
        [controller.view setFrame:CGRectMake(0, localBottom, _scrollView.width, controller.view.height)];
        
        //update the bottom of the reference frame
        localBottom = controller.view.bottom + VERTICAL_SPACING;
        
        // set a tag so we can identify these views for removal when we re-load sorts or filters
        controller.view.tag = GAME_SORT_CONTROLLER_VIEW_TAG;
        
        //keep a reference to the controller and add it to the screen
        [_gamesCategoriesControllers addObject:controller];
        [_scrollView addSubview:controller.view];
        
        //update the scrollview size, just to be safe
        [_scrollView setContentSizeForDirection:UIScrollViewDirectionVertical];
    }];
}

- (void) loadGameAttributeLists {
    [RobloxData fetchAllSorts:^(NSArray *sorts) {
        self.allSorts = [NSMutableArray arrayWithArray:sorts];
    }];
    
    // 2015.0904 ajain - The implementation of this method [RBXGameGenre allGenres] is a stop gap feature until the
    // web team is able to make an endpoint that returns the current genres.  At that time, we will create a
    // proper [RobloxData fetchAllGenres:] method.
    self.allGenres = [RBXGameGenre allGenres];
}

- (void) refreshViews {
    // 2015.0902 - Adjust the Y position of the scroll view to acommodate a possible siteAlertWebView and the sort/genre buttons
    float bannerBottom = (_siteAlertWebView.hidden == NO) ? _siteAlertWebView.bottom + BUTTON_HEIGHT + VERTICAL_SPACING : 0;
    
    // Update scrollview frame to adjust for siteAlertView
    [RBXUIUtil view:_scrollView setOrigin:CGPointMake(0, bannerBottom)];
    [RBXUIUtil view:_scrollView setSize:CGSizeMake(self.view.width, self.view.width)];
    
    [self loadGameAttributeLists];
    [self createDropDownMenuButtons];
    [self loadGames];
}

- (void) resetViews {
    
    [_gamesCategoriesControllers removeAllObjects];
    for (UIView *subview in _scrollView.subviews) {
        if (subview.tag == GAME_SORT_CONTROLLER_VIEW_TAG) {
            [subview removeFromSuperview];
        }
    }
    
}

#pragma mark - OptionListPopoverDelegate method

- (BOOL)popoverControllerShouldDismissPopover:(UIPopoverController *)popoverController {
    [self updateDropDownButtonTitles];
    return YES;
}

- (void) popoverControllerDidDismissPopover:(UIPopoverController *)popoverController {
    [self updateDropDownButtonTitles];
}

@end
