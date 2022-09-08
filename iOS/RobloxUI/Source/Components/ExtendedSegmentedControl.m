//
//  ExtendedSegmentedControl.m
//  RobloxMobile
//
//  Created by Ariel Lichtin on 6/5/14.
//  Copyright (c) 2014 ROBLOX. All rights reserved.
//

#import "ExtendedSegmentedControl.h"
#import <UIKit/UIPopoverController.h>

#define CELL_SIZE CGSizeMake(320.0f, 44.0f)

//-----------------------------------------------------------------------------------------------------------------------------
@interface ExtendedSegmentedControlCell : UITableViewCell
@property (weak, nonatomic) IBOutlet UILabel *title;
@property (weak, nonatomic) IBOutlet UIImageView *checkImage;
@end

@implementation ExtendedSegmentedControlCell

@end
//-----------------------------------------------------------------------------------------------------------------------------

@interface ExtendedSegmentedControl () <UITableViewDelegate, UITableViewDataSource>

@end

@implementation ExtendedSegmentedControl
{
    NSMutableArray* _elements;
    NSInteger _extendedSelectedIndex;
    
    UIViewController* _tableController;
    UITableView* _tableView;
    UIPopoverController* _popOver;
}

- (void)awakeFromNib
{
    [super awakeFromNib];
    
    _elements = [NSMutableArray array];
    
    _tableView = [[UITableView alloc] init];
    _tableView.delegate = self;
    _tableView.dataSource = self;
    _tableView.scrollEnabled = NO;
    _tableView.tableFooterView = [[UIView alloc] initWithFrame:CGRectZero];
    [_tableView registerNib:[UINib nibWithNibName:@"ExtendedSegmentedControlCell" bundle:nil] forCellReuseIdentifier:@"ReuseCell"];
    
    _tableController = [[UIViewController alloc] init];
    [_tableController.view addSubview:_tableView];
}

-(void) showPopOver
{
    if(!_popOver.isPopoverVisible)
    {
        [_tableView reloadData];
        
        CGRect fromRect = self.bounds;
        float itemWidth = fromRect.size.width / (float)self.numberOfSegments;
        fromRect.origin.x = fromRect.size.width - itemWidth;
        fromRect.size.width = itemWidth;

        CGRect tableRect;
        tableRect.origin = CGPointZero;
        tableRect.size = CELL_SIZE;
        tableRect.size.height *= [_tableView numberOfRowsInSection:0];

        _tableView.frame = tableRect;
        _tableController.preferredContentSize = tableRect.size;
        //_tableController.contentSizeForViewInPopover = tableRect.size;
        
        _popOver = [[UIPopoverController alloc] initWithContentViewController:_tableController];
        [_popOver presentPopoverFromRect:fromRect inView:self permittedArrowDirections:UIPopoverArrowDirectionUp animated:YES];
    }
}

- (void)insertSegmentWithTitle:(NSString *)title atIndex:(NSUInteger)segment animated:(BOOL)animated
{
    [_elements addObject:title];
    
    if(self.maxVisibleItems > 0 && _elements.count > self.maxVisibleItems)
    {
        [self setTitle:NSLocalizedString(@"MoreWord", nil) forSegmentAtIndex:(self.maxVisibleItems - 1)];
    }
    else
    {
        [super insertSegmentWithTitle:title atIndex:segment animated:animated];
    }
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
    NSInteger previousSelectedSegmentIndex = [super selectedSegmentIndex];
    [super touchesEnded:touches withEvent:event];

    if(_elements.count > self.maxVisibleItems && [super selectedSegmentIndex] == self.maxVisibleItems-1)
    {
        if(previousSelectedSegmentIndex == [super selectedSegmentIndex])
            [self showPopOver];
        else
            [super setSelectedSegmentIndex:previousSelectedSegmentIndex];
    }
}

- (NSInteger)selectedSegmentIndex
{
    return _extendedSelectedIndex;
}

- (void)setSelectedSegmentIndex:(NSInteger)selectedSegmentIndex
{
    _extendedSelectedIndex = selectedSegmentIndex;
    
    if(_elements.count > self.maxVisibleItems && selectedSegmentIndex >= self.maxVisibleItems)
    {
        [super setSelectedSegmentIndex:self.maxVisibleItems-1];
        
        [_tableView reloadData];
    }
    else
        [super setSelectedSegmentIndex:selectedSegmentIndex];
}

- (void)sendActionsForControlEvents:(UIControlEvents)controlEvents
{
    if(controlEvents == UIControlEventValueChanged)
    {
        if(_elements.count > self.maxVisibleItems && [super selectedSegmentIndex] == self.maxVisibleItems-1)
        {
            [self showPopOver];
        }
        else
        {
            _extendedSelectedIndex = [super selectedSegmentIndex];
        }
    }
    
    [super sendActionsForControlEvents:controlEvents];
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    return MAX(0, (_elements.count - self.maxVisibleItems));
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    ExtendedSegmentedControlCell* cell = [tableView dequeueReusableCellWithIdentifier:@"ReuseCell" forIndexPath:indexPath];
    
    cell.title.text = _elements[self.maxVisibleItems + indexPath.row];
    cell.checkImage.hidden = (_extendedSelectedIndex - self.maxVisibleItems) != indexPath.row;
    
    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    _extendedSelectedIndex = indexPath.row + self.maxVisibleItems;
    [super setSelectedSegmentIndex:(self.maxVisibleItems - 1)];
    
    [self sendActionsForControlEvents:UIControlEventValueChanged];
    
    [_popOver dismissPopoverAnimated:YES];
}

@end
