//
//  RobloxJailbreakDetector.h
//  RobloxMobile
//
//  Created by Martin Robaszewski on 7/16/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#define JAILBREAK_IOS_CYDIA     0x00000001
#define JAILBREAK_IOS_BASH      0x00000002
#define JAILBREAK_IOS_VARLIBAPT 0x00000004

void jbDecodeString(const char * input, char * output);

int jbCheckAll();
int jbCheckCydia();
int jbCheckBash();
int jbCheckVarLibApt();


