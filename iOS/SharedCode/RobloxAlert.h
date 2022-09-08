
#import <Foundation/Foundation.h>

@interface RobloxAlert : NSObject

+(void) RobloxAlertWithMessage:(NSString*) message;
+(void) RobloxOKAlertWithMessageAndDelegate:(NSString*) message Delegate:(id) delegate;
+(void) RobloxAlertWithMessageAndDelegate:(NSString*) message Delegate:(id) delegate;
@end

