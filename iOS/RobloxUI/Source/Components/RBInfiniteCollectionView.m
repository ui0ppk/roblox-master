//
//  RBInfiniteCollectionView.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/14/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBInfiniteCollectionView.h"

#define NUM_ITEMS_PER_REQUEST 40
#define START_REQUEST_THRESHOLD 10 // The next request will start when there are 10 elements left

@interface RBInfiniteCollectionView () <UICollectionViewDelegate, UICollectionViewDataSource>

@end

@implementation RBInfiniteCollectionView
{
    NSUInteger _numItemsInCollectionView;
    BOOL _requestInProgress;
    BOOL _listComplete;
}

- (instancetype)initWithFrame:(CGRect)frame collectionViewLayout:(UICollectionViewLayout *)layout
{
    self = [super initWithFrame:frame collectionViewLayout:layout];
    if(self)
    {
        _numItemsInCollectionView = NUM_ITEMS_PER_REQUEST;
        _listComplete = NO;
        _requestInProgress = NO;
        
        __weak RBInfiniteCollectionView* weakSelf = self;
        self.delegate = weakSelf;
        self.dataSource = weakSelf;
        
        [self reloadData];
    }
    return self;
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView
{
    // Get the index of the last visible item
    NSArray* indexes = [self indexPathsForVisibleItems];
    NSIndexPath* maxIndex = nil;
    for(NSIndexPath* index in indexes)
    {
        if(maxIndex == nil || maxIndex.row < index.row)
        {
            maxIndex = index;
        }
    }
    
    if(_listComplete == NO && maxIndex.row + START_REQUEST_THRESHOLD > _numItemsInCollectionView)
    {
        _numItemsInCollectionView += NUM_ITEMS_PER_REQUEST;
        
        [self performBatchUpdates:^
        {
            NSMutableArray* indexes = [NSMutableArray array];
            for(NSUInteger i = _numItemsInCollectionView - NUM_ITEMS_PER_REQUEST; i < _numItemsInCollectionView; ++i)
            {
                [indexes addObject:[NSIndexPath indexPathForRow:i inSection:0]];
            }
            [self insertItemsAtIndexPaths:indexes];
        }
        completion:nil];
        
        [self loadElementsAsync];
    }
}

- (void)loadElementsAsync
{
    if(_infiniteDelegate == nil)
        return;
    
    NSUInteger itemCount = [_infiniteDelegate numItemsInInfiniteCollectionView:self];
    if(!_listComplete && !_requestInProgress && _numItemsInCollectionView > itemCount)
    {
        _requestInProgress = YES;

        NSUInteger itemsToRequest = _numItemsInCollectionView - itemCount;

        void(^block)() = ^()
        {
            NSUInteger newItemCount = [_infiniteDelegate numItemsInInfiniteCollectionView:self];
            
            NSInteger rangeFrom = itemCount;
            NSInteger rangeTo = newItemCount;
            
            // Update visible elements
            NSArray* visibleCells = [self indexPathsForVisibleItems];
            NSMutableArray* cellsToUpdate = [NSMutableArray array];
            for(NSIndexPath* indexPath in visibleCells)
            {
                BOOL inRange = indexPath.row >= rangeFrom && indexPath.row < rangeTo;
                if(inRange)
                    [cellsToUpdate addObject:indexPath];
            }
            [self reloadItemsAtIndexPaths:cellsToUpdate];
            
            NSUInteger numItemsRetrieved = newItemCount - itemCount;
            _listComplete = numItemsRetrieved < itemsToRequest;
            if(_listComplete)
            {
                // If the list is already complete,
                // remove the remaining placeholder empty cells
                NSUInteger totalElements = _numItemsInCollectionView;
                
                _numItemsInCollectionView = newItemCount;
                
                [self performBatchUpdates:^
                {
                    NSMutableArray* indexes = [NSMutableArray array];
                    for(NSUInteger i = newItemCount; i < totalElements; ++i)
                    {
                        [indexes addObject:[NSIndexPath indexPathForRow:i inSection:0]];
                    }
                    [self deleteItemsAtIndexPaths:indexes];
                }
                completion:nil];
            }
            
            _requestInProgress = NO;
            
            [self loadElementsAsync];
        };
        
        [_infiniteDelegate asyncRequestItemsForCollectionView:self numItemsToRequest:itemsToRequest completionHandler:block];
    }
}

- (NSUInteger)numItems
{
    return _numItemsInCollectionView;
}

- (NSInteger)numberOfSectionsInCollectionView:(UICollectionView *)collectionView
{
    return 1;
}

- (NSInteger)collectionView:(UICollectionView *)collectionView numberOfItemsInSection:(NSInteger)section
{
    return _numItemsInCollectionView;
}

- (UICollectionViewCell *)collectionView:(UICollectionView *)collectionView cellForItemAtIndexPath:(NSIndexPath *)indexPath
{
    if(_infiniteDelegate)
        return [_infiniteDelegate infiniteCollectionView:self cellForItemAtIndexPath:indexPath];
    else
        return nil;
}

- (void)collectionView:(UICollectionView *)collectionView didSelectItemAtIndexPath:(NSIndexPath *)indexPath
{
    if(_infiniteDelegate)
    {
        [_infiniteDelegate infiniteCollectionView:self didSelectItemAtIndexPath:indexPath];
    }
}

@end
