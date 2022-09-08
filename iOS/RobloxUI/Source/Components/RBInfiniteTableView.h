//
//  RBInfiniteTableView.h
//  RobloxMobile
//
//  Created by Ariel Lichtin on 10/14/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import <UIKit/UIKit.h>

// Forward declarations
@class RBInfiniteTableView;


// Infinite scroll view delagate
@protocol RBInfiniteTableViewDelegate <UICollectionViewDelegate>

@required
//
- (void) asyncRequestItemsForTableView:(RBInfiniteTableView*)tableView numItemsToRequest:(NSUInteger)itemsToRequest completionHandler:(void(^)())completionHandler;

- (NSUInteger)numItemsInInfiniteTableView:(RBInfiniteTableView*)tableView;
- (UITableViewCell*) infiniteTableView:(RBInfiniteTableView*)tableView cellForItemAtIndexPath:(NSIndexPath*)indexPath;

@optional
- (void) infiniteTableView:(RBInfiniteTableView*)tableView didSelectItemAtIndexPath:(NSIndexPath*)indexPath;

@end


// Infinite Collection View
@interface RBInfiniteTableView : UITableView

// Retrieves the total number of items in the collection view (real cells + placeholders)
@property(nonatomic, readonly) NSUInteger numItems;

// Delegate
@property(weak, nonatomic) id<RBInfiniteTableViewDelegate> infiniteDelegate;

// Start
- (void) loadElementsAsync;

@end
