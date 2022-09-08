//
//  RBAccountManagerEmailViewController.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 9/26/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBAccountManagerEmailViewController.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "RobloxHUD.h"
#import "UserInfo.h"
#import "LoginManager.h"
#import "RobloxData.h"
#import "UIView+Position.h"
#import "SignUpVerifier.h"
#import "RBXFunctions.h"

#define DEFAULT_VIEW_SIZE CGRectMake(0, 0, 540, 344)
@interface RBAccountManagerEmailViewController ()
    @property (nonatomic, strong) NSString* hiddenEmail;
@end

//----------------CHANGE EMAIL SCREEN----------------
@implementation RBAccountManagerEmailViewController
{
    UITapGestureRecognizer* _touches;
}

#pragma mark Lifecycle Functions
- (void)viewDidLoad
{
    //configure the modal popup superclass
    [self shouldAddCloseButton:NO];
    [self shouldApplyModalTheme:NO];
    [super viewDidLoad];
    
    _touches = [[UITapGestureRecognizer alloc] init];
    [_touches setNumberOfTouchesRequired:1];
    [_touches setNumberOfTapsRequired:1];
    [_touches setDelegate:self];
    [_touches setEnabled:YES];
    [self.view addGestureRecognizer:_touches];
    
    
    //figure out what to ask the user for
    NSString* titleText;
    if ([UserInfo CurrentPlayer].userEmail != nil)
    {
        _hiddenEmail = [[SignupVerifier sharedInstance] obfuscateEmail:[UserInfo CurrentPlayer].userEmail];
        [_txtEmail setText:_hiddenEmail];
        [_btnSave setTitle:NSLocalizedString(@"SaveWord",nil) forState:UIControlStateNormal ];
        titleText = NSLocalizedString(@"ChangeEmailWord", nil);
    }
    else
    {
        [_btnSave setTitle:NSLocalizedString(@"AddWord", nil) forState:UIControlStateNormal];
        titleText = NSLocalizedString(@"AddEmailWord", nil);
    }
    
    [_lblTitle setText:titleText];
    [_lblAlertReason setText:NSLocalizedString(@"AddEmailSuggestion", nil)];
    [_lblAlertReason setFont:[RobloxTheme fontBody]];
    [_lblAlertReason setTextColor:[RobloxTheme colorRed3]];
    
    if (![RobloxInfo thisDeviceIsATablet])
    {
        [_whiteView.layer setShadowColor:[UIColor blackColor].CGColor];
        [_whiteView.layer setShadowOpacity:0.4];
        [_whiteView.layer setShadowRadius:2.0];
        [_whiteView.layer setShadowOffset:CGSizeMake(0.0, 0.5)];
        //[_lblTitle setText:titleText];
    }
    //else
    //{
    //    self.navigationItem.title = titleText;
    //}
    
    [_btnCancel setTitle:NSLocalizedString(@"CancelWord",nil) forState:UIControlStateNormal ];
    [RobloxTheme applyToModalCancelButton:_btnCancel];
    [RobloxTheme applyToModalSubmitButton:_btnSave];
    
    __weak RBAccountManagerEmailViewController* weakSelf = self;
    
    
    
    //[_txtEmail setText:[UserInfo CurrentPlayer].userEmail ? [UserInfo CurrentPlayer].userEmail : nil];
    [_txtEmail setTitle:[UserInfo CurrentPlayer].userOver13 ? NSLocalizedString(@"EmailWord", nil) : NSLocalizedString(@"EmailUnder13Word", nil)];
    [_txtEmail setHint:NSLocalizedString(@"ChangeEmailExample", nil)];
    [_txtEmail setKeyboardType:UIKeyboardTypeEmailAddress];
    [_txtEmail setNextResponder:_txtPassword];
    [_txtEmail setValidationBlock:^{
        if ([weakSelf matchesOriginalEmail:weakSelf.txtEmail.text])
        {
            [weakSelf.txtEmail markAsInvalid];
            [weakSelf.txtEmail showError:NSLocalizedString(@"ErrorSameEmail", nil)];
            return;
        }
        
        [[SignupVerifier sharedInstance] checkIfValidEmail:weakSelf.txtEmail.text completion:^(BOOL success, NSString *message)
        {
            if (success)
                [weakSelf.txtEmail markAsValid];
            else
            {
                [weakSelf.txtEmail markAsInvalid];
                [weakSelf.txtEmail showError:message];
            }
        }];
    }];
    
    [_txtPassword setTitle:NSLocalizedString(@"PasswordCurrentWord", nil)];
    [_txtPassword setHint:NSLocalizedString(@"PasswordCurrentPhrase", nil)];
    [_txtPassword setProtectedTextEntry:YES];
    [_txtPassword setValidationBlock:^{
        if (weakSelf.txtPassword.text && weakSelf.txtPassword.text.length > 0)
            [weakSelf.txtPassword markAsValid];
        else
            [weakSelf.txtPassword markAsNormal];
    }];
    
    
}
-(void) viewWillLayoutSubviews
{
    [super viewWillLayoutSubviews];
    
    
    if ([UserInfo CurrentPlayer].userEmail == nil)
    {
        CGSize textSize = [_lblAlertReason.text sizeWithAttributes:@{NSFontAttributeName:_lblAlertReason.font}];
        int numRows = ceilf(textSize.width / _lblAlertReason.width);
        
        _imgAlert.hidden = NO;
        _lblAlertReason.hidden = NO;
        [_lblAlertReason setY:_lblTitle.bottom];
        [_lblAlertReason setHeight:MAX(20.0, MIN(textSize.height * numRows, 60.0))]; //clamp the height
        [_lblAlertReason setNumberOfLines:numRows];
        [_lblAlertReason sizeToFit];
        [_imgAlert setY:_lblAlertReason.center.y - (_imgAlert.height * 0.5) ];
        
        int contentHeight = _lblTitle.height + _lblAlertReason.height + _txtPassword.height + _txtPassword.height + _btnCancel.height;
        int margin = (([RobloxInfo thisDeviceIsATablet] ? self.view.height : _whiteView.height) - contentHeight) / 5 ;
        
        //move the other text fields down to compensate
        [_txtEmail setY:_lblAlertReason.bottom + margin];
        [_txtPassword setY:_txtEmail.bottom + margin];
        
        if ([RobloxInfo thisDeviceIsATablet])
        {
            float lowest = _txtPassword.bottom + margin;
            [_btnCancel setY:lowest];
            [_btnSave setY:lowest];
        }
    }
    else
    {
        _imgAlert.hidden = YES;
        _lblAlertReason.hidden = YES;
        
        int contentHeight = _lblTitle.height + _txtPassword.height + _txtPassword.height + _btnCancel.height;
        int margin = (([RobloxInfo thisDeviceIsATablet] ? self.view.height : _whiteView.height) - contentHeight) / 4 ;
        
        //move the other text fields down to compensate
        [_txtEmail setY:_lblTitle.bottom + (margin * 0.5)];
        [_txtPassword setY:_txtEmail.bottom + margin];
        
        if ([RobloxInfo thisDeviceIsATablet])
        {
            float lowest = self.view.height - (_btnCancel.height + 10);
            [_btnCancel setY:lowest];
            [_btnSave setY:lowest];
        }
    }
    
    
}

#pragma mark Helper Functions

-(void) disableUI
{
    [_btnCancel setEnabled:NO];
    [_btnSave setEnabled:NO];
    [RobloxHUD showSpinnerWithLabel:NSLocalizedString(@"ChangeEmailVerb", nil) dimBackground:YES];
}
-(void) enableUI
{
    [_btnCancel setEnabled:YES];
    [_btnSave setEnabled:YES];
    [RobloxHUD hideSpinner:YES];
    
}
-(bool) matchesOriginalEmail:(NSString*)email
{
    if (![UserInfo CurrentPlayer].userEmail)
        return NO;
    
    bool matchesHidden = [email isEqualToString:_hiddenEmail];
    bool matchesExisting = [email isEqualToString:[UserInfo CurrentPlayer].userEmail];
    return (matchesHidden || matchesExisting);
}


#pragma mark UI Actions
- (void)returnToPrevious:(id)sender { NSLog(@"Returning to previous"); [self.navigationController popViewControllerAnimated:YES]; }
- (IBAction)saveEmailChange:(id)sender
{
    //[_txtEmail resignFirstResponder];
    //[_txtPassword resignFirstResponder];
    [RBXFunctions dispatchOnMainThread:^{ [self disableUI]; }];
    
    [RBXFunctions dispatchAfter:1.0 onMainThread:^
    {
        ////////////////////////////////////////////////////////////////////////////////////////////////
        ///CHECKS ROUND 1 - check if any required fields are missing
        
        //check if the required field is missing
        bool hasEmail = _txtEmail.text && _txtEmail.text.length > 0;
        bool hasPassword = _txtPassword.text && _txtPassword.text.length > 0;
        bool invalidEmail = NO;
        
        if (!hasEmail)
        {
            //[RobloxHUD showMessage:NSLocalizedString(@"FieldsIncomplete", nil)];
            [_txtEmail showError:NSLocalizedString(@"MissingEmailWord", nil)];
            [_txtEmail markAsInvalid];
            
            [_txtEmail becomeFirstResponder];
        }
        else
        {
            ////////////////////////////////////////////////////////////////////////////////////////////////
            ///CHECKS ROUND 1.5 - check that the email is valid
            if ([self matchesOriginalEmail:_txtEmail.text])
            {
                [_txtEmail markAsInvalid];
                [_txtEmail showError:NSLocalizedString(@"ErrorSameEmail", nil)];
                
                [_txtEmail becomeFirstResponder];
            }
        }
        
        if (!hasPassword)
        {
            //[RobloxHUD showMessage:NSLocalizedString(@"FieldsIncomplete", nil)];
            [_txtPassword showError:NSLocalizedString(@"PasswordCurrentPhrase", nil)];
            [_txtPassword markAsInvalid];
            
            //goto the missing field
            if (![_txtEmail isFirstResponder])
                [_txtPassword becomeFirstResponder];
        }
        
        
        //do not proceed until checks 1 and 1.5 succeed
        if (!hasEmail || !hasPassword || invalidEmail)
        {
            [self enableUI];
            return;
        }
        
        ////////////////////////////////////////////////////////////////////////////////////////////////
        ///CHECKS ROUND2 - validation checks
        if (!_txtEmail.isValidated)
        {
            [_txtEmail becomeFirstResponder];
            [self enableUI];
            return;
        }
        
        
        
        //display the loading spinner
        //[RobloxHUD showSpinnerWithLabel:NSLocalizedString(@"ChangeEmailVerb", nil) dimBackground:YES];
        
        //send the request and alert the player with the response
        [RobloxData changeUserEmail:_txtEmail.text
                       withPassword:_txtPassword.text
                      andCompletion:^(BOOL success, NSString *message)
         {
             NSString* title = NSLocalizedString(@"ErrorWord", nil);
             if (success)
             {
                 //clear the UI
                 [_txtPassword setText:@""];
                 title = NSLocalizedString(@"SuccessWord", nil);
                 
                 [UserInfo CurrentPlayer].userEmail = _txtEmail.text;
                 [[UserInfo CurrentPlayer] UpdateAccountInfo];
                 
                 [RBXFunctions dispatchOnMainThread:^{
                     _hiddenEmail = [[SignupVerifier sharedInstance] obfuscateEmail:_txtEmail.text];
                     [_txtEmail setText:_hiddenEmail];
                 }];
                 
                 //hide the warning and resize the view
                 [UIView animateWithDuration:1.0
                                  animations:^
                 {
                     _imgAlert.alpha = 0.0;
                     _lblAlertReason.alpha = 0.0;
                 }
                                  completion:^(BOOL finished)
                 {
                     [RBXFunctions dispatchOnMainThread:^{
                         [_lblTitle setText:NSLocalizedString(@"ChangeEmailWord", nil)];
                         [self viewWillLayoutSubviews];
                     }];
                 }];
             }
             else
             {
                 [RBXFunctions dispatchOnMainThread:^{
                     [_txtEmail markAsInvalid];
                     [_txtEmail showError:message];
                     [_txtEmail becomeFirstResponder];
                 }];
             }
             
             //tell the user what's going on
             //[RobloxHUD hideSpinner:YES];
             [self enableUI];
             [RobloxHUD prompt:message withTitle:title];
         }];
    }];
}
- (IBAction)closeController:(id)sender {
    //[[RBXEventReporter sharedInstance] reportButtonClick:RBXAButtonClose withContext:RBXAContextLogin];
    [self.navigationController popViewControllerAnimated:YES];
}

- (void) resignAllResponders
{
    [self.view endEditing:YES];
    //[_txtEmail resignFirstResponder];
    //[_txtPassword resignFirstResponder];
}

//Delegate functions
- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
    //NSLog(@"GestureRecognizer : %@", touch);
    UIView* touchedView = touch.view;
    if (touchedView == self.view  || touchedView == _whiteView)
    {
        [self resignAllResponders];
        return  YES;
    }
    return NO;
}


@end
