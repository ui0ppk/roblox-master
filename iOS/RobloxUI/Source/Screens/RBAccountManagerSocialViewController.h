//
//  RBAccountManagerSocialViewController.h
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/14/15.
//  Copyright © 2015 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface RBSocialLinkView : UIView

-(void) initWithSocialName:(NSString*)socialName
                  iconName:(NSString*)iconName
              providerName:(NSString*)providerName
        andRefToController:(UIViewController*)controllerRef;

-(void) setIsConnected:(BOOL)connectedToIdentity;

@end



@interface RBAccountManagerSocialViewController : UIViewController

@property IBOutlet UILabel* lblTitle;
@property IBOutlet UILabel* lblWarning;
@property IBOutlet UIView*  whiteView;
@property IBOutlet UIButton* btnCancel;
@property IBOutlet UIButton* btnUpdate;

@property IBOutlet RBSocialLinkView* rbsFacebook;
@property IBOutlet RBSocialLinkView* rbsTwitter;
@property IBOutlet RBSocialLinkView* rbsGPlus;

- (IBAction)updateInfo:(id)sender;
- (IBAction)closeController:(id)sender;

@end
