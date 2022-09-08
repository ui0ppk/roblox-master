//
//  RBInfiniteTableView.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/14/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBInfiniteTableView.h"

#define NUM_ITEMS_PER_REQUEST 20
#define START_REQUEST_THRESHOLD 10 // The next request will start when there are 10 elements left

@interface RBInfiniteTableView () <UITableViewDelegate, UITableViewDataSource>

@end

@implementation RBInfiniteTableView
{
    NSUInteger _numItemsInView;
    BOOL _requestInProgress;
    BOOL _listComplete;
}

- (id)initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    if(self)
    {
        [self initTableView];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame
{
    self = [super initWithFrame:frame];
    if(self)
    {
        [self initTableView];
    }
    return self;
}

- (instancetype)initWithFrame:(CGRect)frame style:(UITableViewStyle)style
{
    self = [super initWithFrame:frame style:style];
    if(self)
    {
        [self initTableView];
    }
    return self;
}

- (void)initTableView
{
    _numItemsInView = NUM_ITEMS_PER_REQUEST;
    _listComplete = NO;
    _requestInProgress = NO;
    
    __weak RBInfiniteTableView* weakSelf = self;
    self.delegate = weakSelf;
    self.dataSource = weakSelf;
    
    [self reloadData];
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    // Get the index of the last visible item
    NSArray* indexes = [self indexPathsForVisibleRows];
    NSIndexPath* maxIndex = nil;
    for(NSIndexPath* index in indexes)
    {
        if(maxIndex == nil || maxIndex.row < index.row)
        {
            maxIndex = index;
        }
    }
    
    if(_listComplete == NO && maxIndex.row + START_REQUEST_THRESHOLD > _numItemsInView)
    {
        _numItemsInView += NUM_ITEMS_PER_REQUEST;
        
        NSMutableArray* indexes = [NSMutableArray array];
        for(NSUInteger i = _numItemsInView - NUM_ITEMS_PER_REQUEST; i < _numItemsInView; ++i)
        {
            [indexes addObject:[NSIndexPath indexPathForRow:i inSection:0]];
        }

        [self beginUpdates];
        [self insertRowsAtIndexPaths:indexes withRowAnimation:UITableViewRowAnimationAutomatic];
        [self endUpdates];
        
        [self loadElementsAsync];
    }
}

- (void)loadElementsAsync
{
    if(_infiniteDelegate == nil)
        return;
    
    NSUInteger itemCount = [_infiniteDelegate numItemsInInfiniteTableView:self];
    if(!_listComplete && !_requestInProgress && _numItemsInView > itemCount)
    {
        _requestInProgress = YES;

        NSUInteger itemsToRequest = _numItemsInView - itemCount;

        void(^block)() = ^()
        {
            NSUInteger newItemCount = [_infiniteDelegate numItemsInInfiniteTableView:self];
            
            NSInteger rangeFrom = itemCount;
            NSInteger rangeTo = newItemCount;
            
            // Update visible elements
            NSArray* visibleCells = [self indexPathsForVisibleRows];
            if(visibleCells.count > 0)
            {
                NSMutableArray* cellsToUpdate = [NSMutableArray array];
                for(NSIndexPath* indexPath in visibleCells)
                {
                    BOOL inRange = indexPath.row >= rangeFrom && indexPath.row < rangeTo;
                    if(inRange)
                        [cellsToUpdate addObject:indexPath];
                }
                [self reloadRowsAtIndexPaths:cellsToUpdate withRowAnimation:UITableViewRowAnimationAutomatic];
            }
            
            NSUInteger numItemsRetrieved = newItemCount - itemCount;
            _listComplete = numItemsRetrieved < itemsToRequest;
            if(_listComplete)
            {
                // If the list is already complete,
                // remove the remaining placeholder empty cells
                NSUInteger totalElements = _numItemsInView;
                
                _numItemsInView = newItemCount;
                
                NSMutableArray* indexes = [NSMutableArray array];
                for(NSUInteger i = newItemCount; i < totalElements; ++i)
                {
                    [indexes addObject:[NSIndexPath indexPathForRow:i inSection:0]];
                }
                
                [self beginUpdates];
                [self deleteRowsAtIndexPaths:indexes withRowAnimation:UITableViewRowAnimationAutomatic];
                [self endUpdates];
            }
            
            _requestInProgress = NO;
            
            [self loadElementsAsync];
        };
        
        [_infiniteDelegate asyncRequestItemsForTableView:self numItemsToRequest:itemsToRequest completionHandler:block];
    }
}

- (NSUInteger)numItems
{
    return _numItemsInView;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView
{
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return _numItemsInView;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    if(_infiniteDelegate)
        return [_infiniteDelegate infiniteTableView:self cellForItemAtIndexPath:indexPath];
    else
        return nil;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    if(_infiniteDelegate)
    {
        [_infiniteDelegate infiniteTableView:self didSelectItemAtIndexPath:indexPath];
    }
}

@end
