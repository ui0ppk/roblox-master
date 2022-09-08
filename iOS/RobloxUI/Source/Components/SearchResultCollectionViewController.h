//
//  
//  RobloxMobile
//
//  Created by Kyler Mulherin on 1/16/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBBaseViewController.h"
#import "RBInfiniteCollectionView.h"
#import "RobloxData.h"

@interface SearchResultCollectionViewController : RBBaseViewController <RBInfiniteCollectionViewDelegate>

@property NSUInteger resultsPerSearch;
@property NSUInteger resultBufferZone;

@property (readonly) NSMutableArray* searchResults;
@property (readonly) NSString* searchKeyword;
@property (readonly) SearchResultType searchResultType;

@property (readonly) RBInfiniteCollectionView* collectionView;
@property (readonly) NSString* collectionViewCellXibName;



-(id) initWithKeyword:(NSString*)keyword andSearchType:(SearchResultType)searchResultType;


@end
