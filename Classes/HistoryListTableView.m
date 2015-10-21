/* HistoryTableViewController.m
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "HistoryListTableView.h"
#import "UIHistoryCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "UACellBackgroundView.h"
#import "Utils.h"

@implementation HistoryListTableView

@synthesize missedFilter;

#pragma mark - Lifecycle Functions

- (void)initHistoryTableViewController {
	missedFilter = false;
}

- (id)init {
	self = [super init];
	if (self) {
		[self initHistoryTableViewController];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initHistoryTableViewController];
	}
	return self;
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(loadData)
												 name:kLinphoneAddressBookUpdate
											   object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(coreUpdateEvent:)
												 name:kLinphoneCoreUpdate
											   object:nil];
	[self loadData];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

	[[NSNotificationCenter defaultCenter] removeObserver:self name:kLinphoneAddressBookUpdate object:nil];

	[[NSNotificationCenter defaultCenter] removeObserver:self name:kLinphoneCoreUpdate object:nil];
}

#pragma mark - Event Functions

- (void)coreUpdateEvent:(NSNotification *)notif {
	// Invalid all pointers
	[self loadData];
}

#pragma mark - Property Functions

- (void)setMissedFilter:(BOOL)amissedFilter {
	if (missedFilter == amissedFilter) {
		return;
	}
	missedFilter = amissedFilter;
	[self loadData];
}

#pragma mark - UITableViewDataSource Functions

- (NSDate *)dateAtBeginningOfDayForDate:(NSDate *)inputDate {
	NSCalendar *calendar = [NSCalendar currentCalendar];
	NSTimeZone *timeZone = [NSTimeZone systemTimeZone];
	[calendar setTimeZone:timeZone];
	NSDateComponents *dateComps =
		[calendar components:NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit fromDate:inputDate];
	dateComps.hour = dateComps.minute = dateComps.second = 0;
	return [calendar dateFromComponents:dateComps];
}

- (void)loadData {
	const MSList *logs = linphone_core_get_call_logs([LinphoneManager getLc]);
	self.sections = [NSMutableDictionary dictionary];
	while (logs != NULL) {
		LinphoneCallLog *log = (LinphoneCallLog *)logs->data;
		if (!missedFilter || linphone_call_log_get_status(log) == LinphoneCallMissed) {
			NSDate *startDate = [self
				dateAtBeginningOfDayForDate:[NSDate
												dateWithTimeIntervalSince1970:linphone_call_log_get_start_date(log)]];
			NSMutableArray *eventsOnThisDay = [self.sections objectForKey:startDate];
			if (eventsOnThisDay == nil) {
				eventsOnThisDay = [NSMutableArray array];
				[self.sections setObject:eventsOnThisDay forKey:startDate];
			}

			[eventsOnThisDay addObject:[NSValue valueWithPointer:log]];
		}
		logs = ms_list_next(logs);
	}

	NSArray *unsortedDays = [self.sections allKeys];
	self.sortedDays = [unsortedDays sortedArrayUsingComparator:^NSComparisonResult(NSDate *d1, NSDate *d2) {
	  return ![d1 compare:d2]; // reverse order
	}];

	[super loadData];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return _sortedDays.count;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	NSArray *logs = [_sections objectForKey:_sortedDays[section]];
	return logs.count;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	UIView *tempView = [[UIView alloc] initWithFrame:CGRectMake(0, 200, 300, 244)];
	tempView.backgroundColor = [UIColor whiteColor];

	UILabel *tempLabel = [[UILabel alloc] initWithFrame:CGRectMake(15, 0, 300, 44)];
	tempLabel.backgroundColor = [UIColor clearColor];
	tempLabel.textColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A"]];
	NSDate *eventDate = _sortedDays[section];
	NSDate *currentDate = [self dateAtBeginningOfDayForDate:[NSDate date]];
	if ([eventDate isEqualToDate:currentDate]) {
		tempLabel.text = NSLocalizedString(@"TODAY", nil);
	} else if ([eventDate isEqualToDate:[currentDate dateByAddingTimeInterval:-3600 * 24]]) {
		tempLabel.text = NSLocalizedString(@"YESTERDAY", nil);
	} else {
		tempLabel.text =
			[LinphoneUtils timeToString:eventDate.timeIntervalSince1970 withStyle:NSDateFormatterMediumStyle];
	}
	tempLabel.textAlignment = NSTextAlignmentCenter;
	tempLabel.font = [UIFont boldSystemFontOfSize:17];
	[tempView addSubview:tempLabel];

	return tempView;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"UIHistoryCell";
	UIHistoryCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIHistoryCell alloc] initWithIdentifier:kCellId];
	}

	id logId = [_sections objectForKey:_sortedDays[indexPath.section]][indexPath.row];
	LinphoneCallLog *log = [logId pointerValue];
	[cell setCallLog:log];
	[super accessoryForCell:cell atPath:indexPath];
	return cell;
}

#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[super tableView:tableView didSelectRowAtIndexPath:indexPath];
	if (![self isEditing]) {
		id log = [_sections objectForKey:_sortedDays[indexPath.section]][indexPath.row];
		LinphoneCallLog *callLog = [log pointerValue];
		if (callLog != NULL && linphone_call_log_get_call_id(callLog) != NULL) {
			LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
			char *uri = linphone_address_as_string(addr);
			DialerView *view = VIEW(DialerView);
			[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
			[view call:[NSString stringWithUTF8String:uri] displayName:[FastAddressBook displayNameForAddress:addr]];
			ms_free(uri);
		}
	}
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		[tableView beginUpdates];
		id log = [_sections objectForKey:_sortedDays[indexPath.section]][indexPath.row];
		LinphoneCallLog *callLog = [log pointerValue];
		linphone_core_remove_call_log([LinphoneManager getLc], callLog);
		[[_sections objectForKey:_sortedDays[indexPath.section]] removeObject:log];
		if (((NSArray *)[_sections objectForKey:_sortedDays[indexPath.section]]).count == 0) {
			[_sections removeObjectForKey:_sortedDays[indexPath.section]];
		}

		[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath]
						 withRowAnimation:UITableViewRowAnimationFade];
		[tableView endUpdates];
	}
}

@end
