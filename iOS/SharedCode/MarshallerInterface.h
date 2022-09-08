//
//  Roblox.h
//  RobloxMobile
//
//  Created by Ben Tkacheff on 1/30/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#include <Foundation/Foundation.h>

@interface FunctionMarshaller : NSObject
{
    @public void* pClosure;
}

@property (nonatomic,assign) void* pClosure;

- (void)marshallFunction;

@end

