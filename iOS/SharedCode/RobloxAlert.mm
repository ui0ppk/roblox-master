#import "RobloxAlert.h"
#import <UIKit/UIKit.h>

@implementation RobloxAlert
+(void) RobloxAlertWithMessage:(NSString*) message
{
    dispatch_async(dispatch_get_main_queue(),^{
        /* open an alert with an OK button */
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"RobloxWord", @"")
                                                        message:message
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"OkWord", nil)
                                              otherButtonTitles: nil];
        [alert show];
    });
}
+(void) RobloxAlertWithMessageAndDelegate:(NSString*) message Delegate:(id) delegate
{
     dispatch_async(dispatch_get_main_queue(),^{
        /* open an alert with OK and Cancel buttons */
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"RobloxWord", @"")
                                                        message:message
                                                       delegate:delegate
                                              cancelButtonTitle:NSLocalizedString(@"CancelWord", nil)
                                              otherButtonTitles:NSLocalizedString(@"OkWord", nil), nil];
        [alert show];
     });
}
+(void) RobloxOKAlertWithMessageAndDelegate:(NSString*) message Delegate:(id) delegate
{
    dispatch_async(dispatch_get_main_queue(),^{
        /* open an alert with an OK button */
        UIAlertView *alert = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"RobloxWord", @"")
                                                        message:message
                                                       delegate:delegate
                                              cancelButtonTitle:NSLocalizedString(@"OkWord", nil)
                                              otherButtonTitles: nil];
        [alert show];
    });
}
@end
