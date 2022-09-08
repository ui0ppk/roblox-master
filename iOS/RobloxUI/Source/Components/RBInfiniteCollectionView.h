//
//  RBInfiniteCollectionView.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/14/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

// Forward declarations
@class RBInfiniteCollectionView;


// Infinite scroll view delagate
@protocol RBInfiniteCollectionViewDelegate <UICollectionViewDelegate>

@required
//
- (void) asyncRequestItemsForCollectionView:(RBInfiniteCollectionView*)collectionView numItemsToRequest:(NSUInteger)itemsToRequest completionHandler:(void(^)())completionHandler;

- (NSUInteger)numItemsInInfiniteCollectionView:(RBInfiniteCollectionView*)collectionView;
- (UICollectionViewCell*) infiniteCollectionView:(RBInfiniteCollectionView*)collectionView cellForItemAtIndexPath:(NSIndexPath*)indexPath;

@optional
- (void) infiniteCollectionView:(RBInfiniteCollectionView*)collectionView didSelectItemAtIndexPath:(NSIndexPath*)indexPath;

@end


// Infinite Collection View
@interface RBInfiniteCollectionView : UICollectionView

// Retrieves the total number of items in the collection view (real cells + placeholders)
@property(nonatomic, readonly) NSUInteger numItems;

// Delegate
@property(weak, nonatomic) id<RBInfiniteCollectionViewDelegate> infiniteDelegate;

// Start
- (void) loadElementsAsync;

@end
