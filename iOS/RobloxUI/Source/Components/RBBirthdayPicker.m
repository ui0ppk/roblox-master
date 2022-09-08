//
//  RBBirthdayPicker.m
//  RobloxMobile
//
//  Created by Kyler Mulherin on 7/15/15.
//  Copyright (c) 2015 ROBLOX. All rights reserved.
//

#import "RBBirthdayPicker.h"
#import "RobloxTheme.h"
#import "RobloxInfo.h"
#import "UIView+Position.h"


@implementation RBBirthdayPicker
{
    NSString* _birthdayString;
    UILabel* _birthdayLabel;
    UILabel* _birthdayMonth;
    UILabel* _birthdayDay;
    UILabel* _birthdayYear;

    UIButton* _dateButton;
    UIView* _dimView;
    UIView* _datePickerContainer;
    UINavigationBar* _datePickerNavBar;
    UIButton* _datePickerAcceptButton;
    UIButton* _datePickerCancelButton;
    UIPickerView* _dateStringPicker;
    
    CGRect _datePosOffScreen;
    CGRect _datePosOnScreen;
    
    bool isValid;
    void (^validationBlock)();
    
    NSMutableArray* _dataMonths;
    NSMutableArray* _dataDays;
    NSMutableArray* _dataYears;
    NSMutableArray* _lastDateIndexes;
}

//Constructors and Initializers
-(void) initialize {
    isValid = NO;
    validationBlock = nil;
    
    //initialize the data for some date pickers
    //Days
    NSString* placeHolder = @"----";
    _dataDays = [NSMutableArray arrayWithCapacity:32];
    [_dataDays addObject:placeHolder];
    for (NSInteger i = 1; i <= 31; i++)
        [_dataDays addObject:[[NSNumber numberWithInteger:i] stringValue]];
    
    //Months
    _dataMonths = [NSMutableArray arrayWithArray:@[placeHolder,
                                                   NSLocalizedString(@"MonthJanuary", nil),
                                                   NSLocalizedString(@"MonthFebruary", nil),
                                                   NSLocalizedString(@"MonthMarch", nil),
                                                   NSLocalizedString(@"MonthApril", nil),
                                                   NSLocalizedString(@"MonthMay", nil),
                                                   NSLocalizedString(@"MonthJune", nil),
                                                   NSLocalizedString(@"MonthJuly", nil),
                                                   NSLocalizedString(@"MonthAugust", nil),
                                                   NSLocalizedString(@"MonthSeptember", nil),
                                                   NSLocalizedString(@"MonthOctober", nil),
                                                   NSLocalizedString(@"MonthNovember", nil),
                                                   NSLocalizedString(@"MonthDecember", nil)]];
    
    //Years
    NSDateComponents* dateComps = [[NSCalendar currentCalendar] components:(NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit) fromDate:[NSDate date]];
    _dataYears = [NSMutableArray arrayWithCapacity:((dateComps.year - 1900) + 1)];
    int currentYear = 1900;
    while (currentYear <= dateComps.year)
        [_dataYears addObject:[NSNumber numberWithInt:(currentYear++)].stringValue];
    [_dataYears addObject:placeHolder];
    
    _lastDateIndexes = [NSMutableArray arrayWithCapacity:3];
    _lastDateIndexes[0] = [NSNumber numberWithInt:_dataMonths.count * 250];
    _lastDateIndexes[1] = [NSNumber numberWithInt:_dataDays.count * 250];
    _lastDateIndexes[2] = [NSNumber numberWithInt:_dataYears.count - 1];
    
    _dateStringPicker = [[UIPickerView alloc] init];
    [_dateStringPicker setDelegate:self];
    [_dateStringPicker setDataSource:self];
    [_dateStringPicker selectRow:[_lastDateIndexes[0] integerValue] inComponent:0 animated:NO];
    [_dateStringPicker selectRow:[_lastDateIndexes[1] integerValue] inComponent:1 animated:NO];
    [_dateStringPicker selectRow:[_lastDateIndexes[2] integerValue] inComponent:2 animated:NO];
    
    _birthdayString = @"";
    
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //   UI ELEMENTS
    
    [self.layer setBorderWidth:1];
    [self.layer setBorderColor:[RobloxTheme colorGray4].CGColor];
    [self.layer setCornerRadius:5.0];
    [self setClipsToBounds:YES];
    
    
    _birthdayLabel = [[UILabel alloc] init];
    [_birthdayLabel setText:NSLocalizedString(@"BirthdayWord", nil)];
    [_birthdayLabel setFont:[RobloxTheme fontBodyBold]];
    [_birthdayLabel setTextColor:[RobloxTheme colorGray1]];
    
    _birthdayMonth = [[UILabel alloc] init];
    [_birthdayMonth setTextAlignment:NSTextAlignmentCenter];
    [_birthdayMonth setText: placeHolder];
    [_birthdayMonth.layer setBorderWidth:1];
    [_birthdayMonth.layer setBorderColor:[RobloxTheme colorGray4].CGColor];
    [RobloxTheme applyToModalLoginTitleLabel:_birthdayMonth];
    
    _birthdayDay = [[UILabel alloc] init];
    [_birthdayDay setTextAlignment:NSTextAlignmentCenter];
    [_birthdayDay setText: placeHolder];
    [_birthdayDay.layer setBorderWidth:1];
    [_birthdayDay.layer setBorderColor:[RobloxTheme colorGray4].CGColor];
    [RobloxTheme applyToModalLoginTitleLabel:_birthdayDay];
    
    _birthdayYear = [[UILabel alloc] init];
    [_birthdayYear setTextAlignment:NSTextAlignmentCenter];
    [_birthdayYear setText: placeHolder];
    [_birthdayYear.layer setBorderWidth:1];
    [_birthdayYear.layer setBorderColor:[RobloxTheme colorGray4].CGColor];
    [RobloxTheme applyToModalLoginTitleLabel:_birthdayYear];
    
    _dateButton = [[UIButton alloc] init];
    [_dateButton addTarget:self action:@selector(openBirthday:) forControlEvents:UIControlEventTouchUpInside];
    
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //   DATE PICKER
    if (NSClassFromString(@"UIVisualEffectView") && NSClassFromString(@"UIBlurEffect"))
    {
        _dimView = [[UIVisualEffectView alloc] initWithEffect:[UIBlurEffect effectWithStyle:UIBlurEffectStyleDark]];
        [_dimView setAlpha:0.0];
    }
    else
    {
        _dimView = [[UIView alloc] init];
        [_dimView setBackgroundColor:[UIColor colorWithRed:160.0/255.0 green:160.0/255.0 blue:160.0/255.0 alpha:0.85]];
        [_dimView setAlpha:0.0];
    }
        
    _datePickerContainer = [[UIView alloc] init];
    [_datePickerContainer setOpaque:YES];
    [_datePickerContainer setBackgroundColor:[UIColor whiteColor]];
    [_datePickerContainer setClipsToBounds:YES];
    [_datePickerContainer.layer setBorderColor:[[RobloxTheme colorGray4] CGColor]];
    [_datePickerContainer.layer setBorderWidth:1];
    [_datePickerContainer.layer setCornerRadius:5.0];
    [_datePickerContainer.layer setShadowColor:[UIColor blackColor].CGColor];
    [_datePickerContainer.layer setShadowOpacity:0.6];
    [_datePickerContainer.layer setShadowOffset:CGSizeMake(0, 2)];
    [_datePickerContainer.layer setShadowRadius:4];
    [_datePickerContainer setClipsToBounds:YES];
    
    _datePickerNavBar = [[UINavigationBar alloc] init];
    [_datePickerNavBar pushNavigationItem:[[UINavigationItem alloc] init] animated:NO];
    [_datePickerNavBar.topItem setTitle:NSLocalizedString(@"SelectBirthday", nil)];
    [RobloxTheme applyToModalPopupNavBar:_datePickerNavBar];
    
    
    _datePickerAcceptButton = [[UIButton alloc] init];
    [_datePickerAcceptButton setTitle:NSLocalizedString(@"AcceptWord", nil) forState:UIControlStateNormal];
    [_datePickerAcceptButton addTarget:self action:@selector(closeBirthdayAccept:) forControlEvents:UIControlEventTouchUpInside];
    [_datePickerAcceptButton setEnabled:NO];
    [RobloxTheme applyToDisabledModalSubmitButton:_datePickerAcceptButton];
    
    _datePickerCancelButton = [[UIButton alloc] init];
    [_datePickerCancelButton setTitle:NSLocalizedString(@"CancelWord", nil) forState:UIControlStateNormal];
    [_datePickerCancelButton addTarget:self action:@selector(closeBirthday:) forControlEvents:UIControlEventTouchUpInside];
    [RobloxTheme applyToModalCancelButton:_datePickerCancelButton];
    
    if ([RobloxInfo thisDeviceIsATablet])
    {
        //insert the buttons into the navigation bar
        [_datePickerNavBar.topItem setRightBarButtonItem:[[UIBarButtonItem alloc] initWithCustomView:_datePickerAcceptButton]];
        [_datePickerNavBar.topItem setLeftBarButtonItem:[[UIBarButtonItem alloc] initWithCustomView:_datePickerCancelButton] ];
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    //   ADD THE SUBVIEWS
    [self addSubview:_birthdayLabel];
    [self addSubview:_birthdayMonth];
    [self addSubview:_birthdayDay];
    [self addSubview:_birthdayYear];
    [self addSubview:_dateButton];
    [_datePickerContainer addSubview:_dateStringPicker];
    [_datePickerContainer addSubview:_datePickerNavBar];
    if (![RobloxInfo thisDeviceIsATablet])
    {
        [_datePickerContainer addSubview:_datePickerAcceptButton];
        [_datePickerContainer addSubview:_datePickerCancelButton];
    }
}
-(void) layoutSubviews {
    [_dateButton setFrame:CGRectMake(0, 0, self.width, self.height)];
    
    int margin = 12; //self.width * 0.05;
    [_birthdayLabel setFrame:CGRectMake(margin, 0, (self.width / 3) - margin,                           self.height)];
    [_birthdayMonth setFrame:CGRectMake(_birthdayLabel.right,   -1, self.width * 0.33,                  self.height+2)];
    [_birthdayDay   setFrame:CGRectMake(_birthdayMonth.right-1, -1, self.width * 0.15,                  self.height+2)];
    [_birthdayYear  setFrame:CGRectMake(_birthdayDay.right-1,   -1, (self.width - _birthdayDay.right)+2,self.height+2)];
    
    
    CGFloat datePickerNavHeight = 40;
    CGFloat datePickerHeight =  220;
    CGFloat dateButtonWidth = [RobloxInfo thisDeviceIsATablet] ? 80 : 100;
    CGFloat containerHeight = datePickerHeight + datePickerNavHeight + ([RobloxInfo thisDeviceIsATablet] ? 0 : datePickerNavHeight + 16);
    _datePosOffScreen = CGRectMake(0, self.superview.height + 300, self.superview.width, containerHeight);
    _datePosOnScreen  = CGRectMake(0, MIN((self.superview.height * 0.5) - (containerHeight * 0.5), self.superview.height - containerHeight), self.superview.width, containerHeight);
    [_datePickerContainer setFrame:CGRectMake(0, _datePosOffScreen.origin.y, self.superview.width, containerHeight)];
    [_datePickerNavBar setFrame:CGRectMake(0, 6, _datePickerContainer.width, datePickerNavHeight)];
    [_dateStringPicker setFrame:CGRectMake(0, datePickerNavHeight, _datePickerContainer.width, datePickerHeight)];
    [_dateStringPicker setNeedsLayout];
    
    if ([RobloxInfo thisDeviceIsATablet])
    {
        [_datePickerNavBar.topItem.rightBarButtonItem.customView setFrame:CGRectMake(0, 0, dateButtonWidth, datePickerNavHeight)];
        [_datePickerNavBar.topItem.leftBarButtonItem.customView  setFrame:_datePickerNavBar.topItem.rightBarButtonItem.customView.frame];
    }
    else
    {
        float containerMargin = 10;
        float center = _datePickerContainer.center.x;
        [_datePickerAcceptButton setFrame:CGRectMake(center + (containerMargin * 0.5), _dateStringPicker.bottom, dateButtonWidth, datePickerNavHeight)];
        [_datePickerCancelButton setFrame:CGRectMake(_datePickerAcceptButton.x - _datePickerAcceptButton.width - containerMargin,
                                                     _datePickerAcceptButton.y,
                                                     _datePickerAcceptButton.width,
                                                     _datePickerAcceptButton.height)];
    }
    
    int edgeOffset = 400;
    [_dimView setFrame:CGRectMake(-edgeOffset, -edgeOffset, self.superview.width + (edgeOffset * 2), self.superview.height + (edgeOffset * 2))];
}
-(id) init {
    self = [super init];
    if (self)
        [self initialize];
    
    return self;
}
-(id) initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if (self)
        [self initialize];
    
    return self;
}
-(id) initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if (self)
        [self initialize];
    
    return self;
}


//Accessors
-(NSDate*) getDate       {
    int selectedDay = [_dateStringPicker selectedRowInComponent:1] % _dataDays.count;
    int selectedMonth = [_dateStringPicker selectedRowInComponent:0] % _dataMonths.count;
    int selectedYear= [_dataYears[[_dateStringPicker selectedRowInComponent:2]] integerValue];
    
    NSDateComponents* dateConstructor = [[NSDateComponents alloc] init];
    [dateConstructor setDay:selectedDay];
    [dateConstructor setMonth:selectedMonth];
    [dateConstructor setYear:selectedYear];
    
    NSDate* validDate = [[NSCalendar currentCalendar] dateFromComponents:dateConstructor];
    if (validDate)
    {
        //Verify the validity of the created date,
        //If the components of the created date match those of the inputted date, it must be correct
        //Ex:   Feb 31st, 2000 as input yields 2/31/2000
        //      The created date would result in something like 3/3/2000
        //      Thus, 2/31/2000 is not a valid date
        NSDateComponents* dateComps = [[NSCalendar currentCalendar] components:(NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit) fromDate:validDate];
        bool matchesDay = (selectedDay == [dateComps day]);
        bool matchesMonth = (selectedMonth == [dateComps month]);
        bool matchesYear = (selectedYear == [dateComps year]);
        
        if (!matchesDay || !matchesMonth || !matchesYear)
            return nil;
    }
    
    //check to see if the created date is in the future
    NSTimeInterval time = [validDate timeIntervalSinceDate:[NSDate date]];
    if (time > 0)
    {
        //The date is in the future
        return nil;
    }
    
    return validDate;
}
-(NSString*) getBirthday { return _birthdayString; }
-(BOOL) getWasBornToday  {
    NSDate* pickedDate = [self getDate];
    if (!pickedDate)
        return NO;
    
    return [pickedDate isEqualToDate:[NSDate date]];
}
-(BOOL) userUnder13      {
    //getDate is guaranteed to return a valid date, as it is only called after a successful validation
    NSDate* dateToday = [NSDate date];
    NSTimeInterval secondsSince = [dateToday timeIntervalSinceDate:[self getDate]];
    float numberOfYears = secondsSince / (60.0f * 60.0f * 24.0f * 365.25f);
    
    return (numberOfYears < 13.0f);
}
-(BOOL) isValid          { return isValid; }

//Mutators
-(void) setValidationBlock:(void(^)())callback { validationBlock = callback; }

//Delegate Functions
-(void)pickerView:(UIPickerView *)pickerView didSelectRow:(NSInteger)row inComponent:(NSInteger)component{
    NSDate* currentDate = [self getDate];
    [_datePickerAcceptButton setEnabled:(currentDate != nil)];
    
    if (_datePickerAcceptButton.enabled)
        [RobloxTheme applyToModalSubmitButton:_datePickerAcceptButton];
    else
        [RobloxTheme applyToDisabledModalSubmitButton:_datePickerAcceptButton];
}
-(NSString *)pickerView:(UIPickerView *)pickerView titleForRow:(NSInteger)row forComponent:(NSInteger)component {
    NSString* title = @"";
    switch (component)
    {
        case (0): title = _dataMonths[row % _dataMonths.count]; break;
        case (1): title = _dataDays[row % _dataDays.count];     break;
        case (2): title = _dataYears[row % _dataYears.count];   break;
    }
    return title;
}
-(NSInteger)numberOfComponentsInPickerView:(UIPickerView *)pickerView {
    return 3;
}
-(NSInteger)pickerView:(UIPickerView *)pickerView numberOfRowsInComponent:(NSInteger)component {
    NSInteger numRows = 0;
    switch (component)
    {
        case 0: numRows = _dataMonths.count * 500;  break;
        case 1: numRows = _dataDays.count * 500;    break;
        case 2: numRows = _dataYears.count;         break;
    }
    
    return numRows;
}
-(CGFloat)pickerView:(UIPickerView *)pickerView widthForComponent:(NSInteger)component {
    CGFloat compWidth = pickerView.width;
    switch (component)
    {
        case (0) : compWidth *= 0.40f; break;
        case (1) : compWidth *= 0.20f; break;
        case (2) : compWidth *= 0.40f; break;
    }
    return compWidth;
}



//Button Actions
- (void)openBirthday:(id)sender {
    [self.superview addSubview:_dimView];
    [self.superview addSubview:_datePickerContainer];
    
    //set the date of the last date that was accepted
    [_dateStringPicker selectRow:[_lastDateIndexes[0] integerValue] inComponent:0 animated:NO];
    [_dateStringPicker selectRow:[_lastDateIndexes[1] integerValue] inComponent:1 animated:NO];
    [_dateStringPicker selectRow:[_lastDateIndexes[2] integerValue] inComponent:2 animated:NO];
    
    [_dateStringPicker becomeFirstResponder];
    [UIView animateWithDuration:0.3
                     animations:^
    {
        _datePickerContainer.frame = _datePosOnScreen;
        _dimView.alpha = 1.0;
    }
                     completion:nil];
}
- (void)closeBirthday:(id)sender {
    [_dateStringPicker resignFirstResponder];
    [UIView animateWithDuration:0.3
                     animations:^
     {
         _datePickerContainer.frame = _datePosOffScreen;
         _dimView.alpha = 0.0;
     }
                     completion:^(BOOL finished)
     {
         [_datePickerContainer removeFromSuperview];
         [_dimView removeFromSuperview];
     }];
}
- (void)closeBirthdayAccept:(id)sender {
    isValid = YES;
    [self updateBirthdate];
    [self closeBirthday:sender];
    [self markAsValid];
}
- (void)updateBirthdate {
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setTimeStyle:NSDateFormatterNoStyle];
    [dateFormatter setLocale:[[NSLocale alloc] initWithLocaleIdentifier:@"en_US_POSIX"]];
    [dateFormatter setDateStyle:NSDateFormatterShortStyle];
    
    //save the string for the signup string
    NSDate* acceptedDate = [self getDate];
    _birthdayString = [dateFormatter stringFromDate:acceptedDate];
    
    //update the indexes of the last dates
    _lastDateIndexes[0] = [NSNumber numberWithInteger:[_dateStringPicker selectedRowInComponent:0]];
    _lastDateIndexes[1] = [NSNumber numberWithInteger:[_dateStringPicker selectedRowInComponent:1]];
    _lastDateIndexes[2] = [NSNumber numberWithInteger:[_dateStringPicker selectedRowInComponent:2]];
    
    //update the UI
    NSDateComponents* dateComps = [[NSCalendar currentCalendar] components:(NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit) fromDate:acceptedDate];
    [_birthdayMonth setText:[dateFormatter.monthSymbols objectAtIndex:(dateComps.month - 1)]];
    [_birthdayDay   setText:[NSString stringWithFormat:@"%li", (long)dateComps.day]];
    [_birthdayYear  setText:[NSString stringWithFormat:@"%li", (long)dateComps.year]];

    //run some code for some other controllers
    if (validationBlock != nil)
        validationBlock();
}


//Miscellaneous functions
- (BOOL)becomeFirstResponder {
    [super becomeFirstResponder];
    return [_dateStringPicker becomeFirstResponder];
}
- (BOOL)resignFirstResponder {
    [super resignFirstResponder];
    if (_datePickerContainer.superview)
        [self closeBirthday:nil];
    return [_dateStringPicker resignFirstResponder];
}


//Colors and crap
-(void) changeBorderColor:(UIColor*)color {
    dispatch_async(dispatch_get_main_queue(), ^
    {
        self.layer.borderColor = [color CGColor];
        _birthdayMonth.layer.borderColor = [color CGColor];
        _birthdayDay.layer.borderColor = [color CGColor];
        _birthdayYear.layer.borderColor = [color CGColor];
    });
}
-(void) markAsValid     {
    [self changeBorderColor:[RobloxTheme colorGreen1]];
    [_birthdayLabel setTextColor:[RobloxTheme colorGray1]];
    [_birthdayMonth setTextColor:[RobloxTheme colorGray1]];
    [_birthdayDay   setTextColor:[RobloxTheme colorGray1]];
    [_birthdayYear  setTextColor:[RobloxTheme colorGray1]];
}
-(void) markAsNormal    {
    [self changeBorderColor:[RobloxTheme colorGray4]];
    [_birthdayLabel setTextColor:[UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f]];
    [_birthdayMonth setTextColor:[UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f]];
    [_birthdayDay   setTextColor:[UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f]];
    [_birthdayYear  setTextColor:[UIColor colorWithWhite:(0xA9/255.f) alpha:1.0f]];
}
-(void) markAsInvalid   {
    [self changeBorderColor:[RobloxTheme colorRed1]];
    //[_birthdayLabel setTextColor:[RobloxTheme colorGray1]];
    //[_birthdayMonth setTextColor:[RobloxTheme colorGray1]];
    //[_birthdayDay   setTextColor:[RobloxTheme colorGray1]];
    //[_birthdayYear  setTextColor:[RobloxTheme colorGray1]];
}
@end
