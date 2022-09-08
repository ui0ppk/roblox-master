#include "StudioMacUtilities.h"
#import <Cocoa/Cocoa.h>

namespace StudioMacUtilities {
    
static NSObject* processActivity = nil;
void disableAppNap(const char* reason)
{
    if (processActivity)
        return;
    
    @autoreleasepool
    {
        if ([[NSProcessInfo processInfo] respondsToSelector:@selector(beginActivityWithOptions:reason:)])
        {
            processActivity = [[NSProcessInfo processInfo] beginActivityWithOptions:NSActivityIdleSystemSleepDisabled | NSActivityIdleDisplaySleepDisabled reason:@(reason)];
            [processActivity retain];
        }
    }
}

void enableAppNap()
{
    if (!processActivity)
        return;
    
    @autoreleasepool
    {
        if ([[NSProcessInfo processInfo] respondsToSelector:@selector(endActivity:)])
        {
            [[NSProcessInfo processInfo] endActivity:processActivity];
            [processActivity release];
            processActivity = nil;
        }
    }
}
    
}