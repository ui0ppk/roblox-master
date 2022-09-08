//
//  RobloxMemoryManager.m
//  RobloxMobile
//
//  Created by Martin Robaszewski on 5/15/13.
//  Copyright (c) 2013 ROBLOX. All rights reserved.
//

#import "RobloxMemoryManager.h"
#import "RobloxGoogleAnalytics.h"
#import "PlaceLauncher.h"
#import "RobloxInfo.h"

#include "ObjectiveCUtilities.h"

#include <time.h>
#include <sys/time.h>

#ifdef __MACH__
#include <mach/clock.h>
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

// memory functions from stackoverflow.com

vm_size_t usedMemory(void)
{
    struct task_basic_info info;
    mach_msg_type_number_t size = sizeof(info);
    kern_return_t kerr = task_info(mach_task_self(), TASK_BASIC_INFO, (task_info_t)&info, &size);
    return (kerr == KERN_SUCCESS) ? info.resident_size : 0; // size in bytes
}

vm_size_t freeMemory(void)
{
    mach_port_t host_port = mach_host_self();
    mach_msg_type_number_t host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
    vm_size_t pagesize;
    vm_statistics_data_t vm_stat;
    
    host_page_size(host_port, &pagesize);
    (void) host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size);
    return vm_stat.free_count * pagesize;
}

void print_free_memory()
{
    mach_port_t host_port;
    mach_msg_type_number_t host_size;
    vm_size_t pagesize;
    
    host_port = mach_host_self();
    host_size = sizeof(vm_statistics_data_t) / sizeof(integer_t);
    host_page_size(host_port, &pagesize);
    
    vm_statistics_data_t vm_stat;
    
    if (host_statistics(host_port, HOST_VM_INFO, (host_info_t)&vm_stat, &host_size) != KERN_SUCCESS)
        NSLog(@"Failed to fetch vm statistics");
    
    /* Stats in bytes */
    natural_t mem_used = (vm_stat.active_count +
                          vm_stat.inactive_count +
                          vm_stat.wire_count) * pagesize;
    natural_t mem_free = vm_stat.free_count * pagesize;
    natural_t mem_total = mem_used + mem_free;
    NSLog(@"used: %u free: %u total: %u", mem_used, mem_free, mem_total);
}

static std::vector<void *> debug_memBlocks;

void debug_stealMemory(size_t bytes)
{
    NSLog(@"stealMemory(%ld)", bytes);
    void * memBlock = 0;
    print_free_memory();
    memBlock = malloc(bytes);
    if (memBlock != 0)
    {
        // write to the block to make it real otherwise it wont really be allocated
        int value = 0;
        for (long a=0; a<bytes; a += 1024)
        {
            ((unsigned char *)memBlock)[a] = value;
            value++;
        }
        
        debug_memBlocks.push_back(memBlock);
    }
    else
    {
        NSLog(@"failed!");
    }
    print_free_memory();
}

@implementation RobloxMemoryManager

+ (id)sharedInstance
{
    static dispatch_once_t robloxMemoryManagerPred = 0;
    __strong static id _sharedObject = nil;
    dispatch_once(&robloxMemoryManagerPred, ^{ // Need to use GCD for thread-safe allocation
        _sharedObject = [[self alloc] init];
    });
    return _sharedObject;
}

//
// MEMORY BOUNCER FUNCTIONS
//

-(void) startMemoryBouncer
{
    if (isMemoryBouncerActive)
    {
        return;
    }
    
    stopCount = 0;
    
    NSLog(@"RMM::startMB");
    
    // memory bouncer
    iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    isMemoryBouncerActive = iosSettings->GetValueMemoryBouncerActive();
    memoryBouncerEnforceRate = iosSettings->GetValueMemoryBouncerEnforceRateMilliSeconds();
    memoryBouncerThresholdKB = iosSettings->GetValueMemoryBouncerThresholdKiloBytes();
    memoryBouncerLimitMB = iosSettings->GetValueMemoryBouncerLimitMegaBytes();
    memoryBouncerBlockSizeKB = iosSettings->GetValueMemoryBouncerBlockSizeKB();
    
    int memBouncerDelayCountNew = iosSettings->GetValueMemoryBouncerDelayCount();
    if (bMemBouncerCrashed)
    {
        // set delay counter...
        [[NSUserDefaults standardUserDefaults] setInteger:memBouncerDelayCountNew forKey:@"MemBouncerDelayCount"];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
    
    // check if there is a delay
    int memBouncerDelayCount = [[NSUserDefaults standardUserDefaults] integerForKey:@"MemBouncerDelayCount"];
    if (memBouncerDelayCount > 0)
    {
        NSLog(@"MB - memBouncerDelayCount %d", memBouncerDelayCount);
        isMemoryBouncerActive  = false;
        memBouncerDelayCount--;
        [[NSUserDefaults standardUserDefaults] setInteger:memBouncerDelayCount forKey:@"MemBouncerDelayCount"];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
    
    NSString *platform = [RobloxInfo deviceType];
    
    // use MemoryBouncerLimitMegaBytesLow value for older devices
    if ([platform hasPrefix:@"iPod4"] ||
        [platform hasPrefix:@"iPad1"] ||
        [platform hasPrefix:@"iPhone2"] // iPhone 3GS
        )
    {
        memoryBouncerLimitMB = iosSettings->GetValueMemoryBouncerLimitMegaBytesForLowMemDevices();
    }
    
    // logging
    long freeMem = freeMemory();
    long usedMem = usedMemory();
    long totalMemory = freeMem + usedMem;
    
    NSLog(@"MB isMBActive:%d MBEnforceRate:%d MBThresholdKB:%d MBLimitMB:%d totalM:%ld platform:%@", isMemoryBouncerActive, memoryBouncerEnforceRate, memoryBouncerThresholdKB, memoryBouncerLimitMB, totalMemory, platform );
    
    NSLog(@"MB free mem: %ld", freeMem);
    NSLog(@"MB used mem: %ld", usedMem);
    NSLog(@"MB total   : %ld", totalMemory);
    print_free_memory();
    
    if (isMemoryBouncerActive)
    {
        [[NSUserDefaults standardUserDefaults] setObject:@"Bouncer" forKey:@"RobloxMemMgrState"];
        [[NSUserDefaults standardUserDefaults] synchronize];
        
        double checkTime = (double)memoryBouncerEnforceRate/1000.0;
        memoryBouncerTimer = [NSTimer scheduledTimerWithTimeInterval:checkTime
                                                          target:self
                                                        selector:@selector(bounceFreeMemory:)
                                                        userInfo:nil
                                                         repeats:YES];
        
        [self balloonMemory];
    }
}

-(BOOL) stopMemoryBouncer:(BOOL)bForce
{
    BOOL bStopped = FALSE;
    
    stopCount++;
    
    if (memoryBouncerTimer != nil)
    {
        if (bForce) // forcing by bouncer
        {
            [memoryBouncerTimer invalidate];
            memoryBouncerTimer = nil;
            bStopped = TRUE;
            
            [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxMemMgrState"];
            [[NSUserDefaults standardUserDefaults] synchronize];
        }
    }
    
    [self popBalloon];
    
    NSLog(@"RMM::stopMB stopCount:%d stopped:%d", stopCount, bStopped);
    
    return bStopped;
}

// use the memory to force it into reality
void makeMemoryBlockDirty(void * memBlock, long memSize)
{
    int value = 0;
    for (long a=0; a<memSize; a += 1024)
    {
        ((unsigned char *)memBlock)[a] = value;
        value++;
    }
}

-(void) balloonMemory
{
    [self popBalloon];
    
    long memorySize = freeMemory();
    long mbSize = (memorySize/(1024*1024));
    NSLog(@"RMM::balloonMB %ld %ldMB mbbsKB %d", memorySize, mbSize, memoryBouncerBlockSizeKB);
    
    if (mbSize >= memoryBouncerLimitMB) // we are done
    {
        NSLog(@"mbSize >= MBLimitMB --- STOPPING");
        [self stopMemoryBouncer:TRUE];
        return;
    }
    
    if (memoryBouncerBlockSizeKB == 0)
    {
        balloonMemoryBlock = malloc(memorySize);
        
        if (balloonMemoryBlock)
        {
            makeMemoryBlockDirty(balloonMemoryBlock, memorySize);
        }
        else
        {
            [self stopMemoryBouncer:TRUE];
        }
    }
    else
    {
        bool bRun = true;
        long totalUsedSize = 0;
        do
        {
            long memBlockSize = memoryBouncerBlockSizeKB * 1024;
            long usedSize = 0;
            if (memorySize > memBlockSize)
            {
                usedSize = memBlockSize;
            }
            else
            {
                usedSize = memorySize;
                bRun = false;
            }
            
            memorySize -= usedSize;
            balloonMemoryBlock = malloc(usedSize);
            
            //NSLog(@"usedSize: %ld totalUsedSize %ld remaining: %ld", usedSize, totalUsedSize, memorySize);
            
            if (balloonMemoryBlock)
            {
                makeMemoryBlockDirty(balloonMemoryBlock, usedSize);
                memBlocks.push_back(balloonMemoryBlock);
                totalUsedSize += usedSize;
            }
            else
            {
                //NSLog(@"--- malloc() failed");
                [self stopMemoryBouncer:TRUE];
                bRun = false;
            }
        }
        while (bRun);
        
        balloonMemoryBlock = 0; // clear it
        
    }
}

-(void) popBalloon
{
    if (memoryBouncerBlockSizeKB == 0)
    {
        if (balloonMemoryBlock)
        {
            free(balloonMemoryBlock);
            balloonMemoryBlock = 0;
        }
    }
    else
    {
        while (int size = memBlocks.size())
        {
            //NSLog(@"size %d - freeing index %d", size, (size-1));
            free(memBlocks[size-1]);
            memBlocks.pop_back();
        }
    }
}

-(void) bounceFreeMemory:(NSTimer *)timer;
{
    int kiloBytesFree = freeMemory()/1024.0f;
    NSLog(@"RMM::bounceFM - %d", kiloBytesFree);
    
    if (kiloBytesFree  > memoryBouncerThresholdKB)
    {
        [self balloonMemory];
    }
}

-(void) memBouncerCrashed
{
    NSLog(@"RMM::bouncerCrashed");
    bMemBouncerCrashed = true;
}

//
// FREE MEMORY CHECKER FUNCTIONS
//

-(void) startFreeMemoryChecker
{
    NSLog(@"RMM::startFMC");
    print_free_memory();
    
    [self stopMemoryBouncer:TRUE]; // force stop bouncer
    
    // start timer (if enabled) to detect low memory
    iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    isFreeMemoryCheckerActive = iosSettings->GetValueFreeMemoryCheckerActive();
    freeMemoryCheckerRate = iosSettings->GetValueFreeMemoryCheckerRateMilliSeconds();
    freeMemoryCheckerTheshold = iosSettings->GetValueFreeMemoryCheckerThresholdKiloBytes();
    //freeMemoryCheckerTheshold = 20000; // override test
    
    if (isFreeMemoryCheckerActive)
    {
        NSLog(@"STARTING FMC - rate %dsec threshold %dkB", freeMemoryCheckerRate, freeMemoryCheckerTheshold);
        
        [[NSUserDefaults standardUserDefaults] setObject:@"Checker" forKey:@"RobloxMemMgrState"];
        [[NSUserDefaults standardUserDefaults] synchronize];
        
        double checkTime = (double)freeMemoryCheckerRate/1000.0;
        freeMemoryCheckerTimer = [NSTimer scheduledTimerWithTimeInterval:checkTime
                                                          target:self
                                                        selector:@selector(checkFreeMemory:)
                                                        userInfo:nil
                                                         repeats:YES];
    }
    else
    {
        NSLog(@"NOT STARTING FMC - rate %dsec threshold %dkB", freeMemoryCheckerRate, freeMemoryCheckerTheshold);
    }
}

-(void) stopFreeMemoryChecker
{
    NSLog(@"RMM::stopFMC");
    
    if (freeMemoryCheckerTimer != nil)
    {
        [freeMemoryCheckerTimer invalidate];
        freeMemoryCheckerTimer = nil;
        
        [[NSUserDefaults standardUserDefaults] removeObjectForKey:@"RobloxMemMgrState"];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
}

-(void) checkFreeMemory:(NSTimer *)timer;
{
    int kiloBytesFree = freeMemory()/1024.0f;
    
    if (kiloBytesFree < freeMemoryCheckerTheshold)
    {
        NSLog(@"RMM::checkFreeMemory %dkB passed threshold %dkB", kiloBytesFree, freeMemoryCheckerTheshold);
        
        NSString* freeMemoryType = [NSString stringWithFormat:@"%d_%d", freeMemoryCheckerRate, freeMemoryCheckerTheshold];
        [RobloxGoogleAnalytics setEventTracking:@"PlayErrors" withAction:@"OutOfMemory_EarlyExit" withLabel:freeMemoryType withValue:kiloBytesFree];
        
        [[PlaceLauncher sharedInstance] applicationDidReceiveMemoryWarning];
    }
    else
    {
        [self logMemUsage:timer];
    }
}

-(void) logMemUsage:(NSTimer *)timer;
{
    // compute memory usage and log if different by >= 100k
    static long prevMemUsage = 0;
    long curMemUsage = usedMemory();
    long memUsageDiff = curMemUsage - prevMemUsage;
    
    if (memUsageDiff > 100000 || memUsageDiff < -100000)
    {
        prevMemUsage = curMemUsage;
        
        if (timer != nil)
        {
            NSLog(@"RMM::logMemUsage BEAT M_used %7.1f (%+5.0f), free %7.1f kb", curMemUsage/1000.0f, memUsageDiff/1000.0f, freeMemory()/1000.0f);
        }
        else
        {
            NSLog(@"RMM::logMemUsage **** M_used %7.1f (%+5.0f), free %7.1f kb", curMemUsage/1000.0f, memUsageDiff/1000.0f, freeMemory()/1000.0f);
        }
    }
}

//
// MAX MEMORY TRACKER
//

-(void) startMaxMemoryTracker
{
    NSLog(@"RMM::startMMT");
    
    iOSSettingsService * iosSettings = [[RobloxWebUtility sharedInstance] getCachediOSSettings];
    maxMemoryTrackerRate = iosSettings->GetValueMaxMemoryReporterRateMilliSeconds();
    
    maxMemoryUsed = usedMemory();
    
    if (maxMemoryTrackerRate > 0)
    {
        NSLog(@"STARTING MMT - rate %d", maxMemoryTrackerRate);
        
        [[NSUserDefaults standardUserDefaults] setInteger:maxMemoryUsed forKey:@"MaxMemoryUsed"];
        [[NSUserDefaults standardUserDefaults] synchronize];
        
        double checkTime = (double)maxMemoryTrackerRate/1000.0;
        maxMemoryTrackerTimer = [NSTimer scheduledTimerWithTimeInterval:checkTime
                                                                  target:self
                                                                selector:@selector(checkMaxMemory:)
                                                                userInfo:nil
                                                                 repeats:YES];
    }
    else
    {
        NSLog(@"NOT STARTING MMT - rate %d", maxMemoryTrackerRate);
    }
}

-(void) checkMaxMemory:(NSTimer *)timer
{
    long currMemUsage = usedMemory();
    
    if (currMemUsage > maxMemoryUsed)
    {
        maxMemoryUsed = currMemUsage;

        [[NSUserDefaults standardUserDefaults] setInteger:maxMemoryUsed forKey:@"MaxMemoryUsed"];
        [[NSUserDefaults standardUserDefaults] synchronize];
    }
}

-(void) stopMaxMemoryTracker
{
    NSLog(@"RMM::stopMMT");
    
    if (maxMemoryTrackerTimer != nil)
    {
        [maxMemoryTrackerTimer invalidate];
        maxMemoryTrackerTimer = nil;
    }
}

@end
