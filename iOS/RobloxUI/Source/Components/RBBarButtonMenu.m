//
//  RBBarButtonMenu.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 9/4/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "RBBarButtonMenu.h"

#define CELL_SIZE CGSizeMake(320.0f, 44.0f)

//-----------------------------------------------------------------------------------------------------------------------------
@interface RBBarButtonMenuItemData: NSObject
@property(strong, nonatomic) NSString* title;
@property(nonatomic)         SEL action;
@property(nonatomic,assign)  id  target;
@end

@implementation RBBarButtonMenuItemData
@end
//-----------------------------------------------------------------------------------------------------------------------------
@interface RBBarButtonMenuCell : UITableViewCell
@property (weak, nonatomic) IBOutlet UILabel *title;
@property (weak, nonatomic) IBOutlet UIImageView *checkImage;
@end

@implementation RBBarButtonMenuCell

@end
//-----------------------------------------------------------------------------------------------------------------------------

@interface RBBarButtonMenu () <UITableViewDelegate, UITableViewDataSource>

@end

@implementation RBBarButtonMenu
{
    NSMutableArray* _elements;
    
    UIViewController* _tableController;
    UITableView* _tableView;
    UIPopoverController* _popOver;
}

- (void)initButton
{
    self.target = self;
    self.action = @selector(mainButtonTouched);
    
    _elements = [NSMutableArray array];
    
    _tableView = [[UITableView alloc] init];
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.scrollEnabled = NO;
    _tableView.tableFooterView = [[UIView alloc] initWithFrame:CGRectZero];
    [_tableView registerNib:[UINib nibWithNibName:@"RBBarButtonMenuCell" bundle:nil] forCellReuseIdentifier:@"ReuseCell"];
    
    _tableController = [[UIViewController alloc] init];
    [_tableController.view addSubview:_tableView];
}

- (instancetype)initWithTitle:(NSString*)title style:(UIBarButtonItemStyle)style
{
    self = [super initWithTitle:title style:UIBarButtonItemStyleDone target:nil action:nil];
    if(self)
    {
        [self initButton];
    }
    return self;
}

- (instancetype)initWithImage:(UIImage *)image style:(UIBarButtonItemStyle)style
{
    self = [super initWithImage:image style:style target:nil action:nil];
    if(self)
    {
        [self initButton];
    }
    return self;
}

-(void) showPopOver
{
    if(_popOver == nil || !_popOver.isPopoverVisible)
    {
        [_tableView reloadData];
        
        CGRect tableRect;
        tableRect.origin = CGPointZero;
        tableRect.size = CELL_SIZE;
        tableRect.size.height *= [_tableView numberOfRowsInSection:0];
        
        _tableView.frame = tableRect;
        _tableController.preferredContentSize = tableRect.size;
        
        _popOver = [[UIPopoverController alloc] initWithContentViewController:_tableController];
        [_popOver presentPopoverFromBarButtonItem:self permittedArrowDirections:UIPopoverArrowDirectionUp animated:YES];
    }
}

- (void)addItemWithTitle:(NSString *)title target:(id)target action:(SEL)action
{
    RBBarButtonMenuItemData* item = [[RBBarButtonMenuItemData alloc] init];
    item.title = title;
    item.target = target;
    item.action = action;
    [_elements addObject:item];
}

- (void) mainButtonTouched
{
    [self showPopOver];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return _elements.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    RBBarButtonMenuCell* cell = [tableView dequeueReusableCellWithIdentifier:@"ReuseCell" forIndexPath:indexPath];
    
    RBBarButtonMenuItemData* itemData = _elements[indexPath.row];
    cell.title.text = itemData.title;
    
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [_popOver dismissPopoverAnimated:YES];
    
    RBBarButtonMenuItemData* itemData = _elements[indexPath.row];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Warc-performSelector-leaks"
    [itemData.target performSelector:itemData.action];
#pragma clang diagnostic pop
}


@end
