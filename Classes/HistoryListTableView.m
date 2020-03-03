/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import "HistoryListTableView.h"
#import "UIHistoryCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
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
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(loadData)
											   name:kLinphoneAddressBookUpdate
											 object:nil];

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(loadData)
											   name:kLinphoneCallUpdate
											 object:nil];

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(coreUpdateEvent:)
											   name:kLinphoneCoreUpdate
											 object:nil];
	[self loadData];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

	[NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneAddressBookUpdate object:nil];
	[NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneCoreUpdate object:nil];
	[NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneCallUpdate object:nil];
}

#pragma mark - Event Functions

- (void)coreUpdateEvent:(NSNotification *)notif {
	@try {
		// Invalid all pointers
		[self loadData];
	}
	@catch (NSException *exception) {
		if ([exception.name isEqualToString:@"LinphoneCoreException"]) {
			LOGE(@"Core already destroyed");
			return;
		}
		LOGE(@"Uncaught exception : %@", exception.description);
		abort();
	}
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
		[calendar components:(NSYearCalendarUnit | NSMonthCalendarUnit | NSDayCalendarUnit) fromDate:inputDate];

	dateComps.hour = dateComps.minute = dateComps.second = 0;
	return [calendar dateFromComponents:dateComps];
}

- (void)loadData {
	for (id day in self.sections.allKeys) {
		for (id log in self.sections[day]) {
			linphone_call_log_unref([log pointerValue]);
		}
	}

	const bctbx_list_t *logs = linphone_core_get_call_logs(LC);
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

			linphone_call_log_set_user_data(log, NULL);

			// if this contact was already the previous entry, do not add it twice
			LinphoneCallLog *prev = [eventsOnThisDay lastObject] ? [[eventsOnThisDay lastObject] pointerValue] : NULL;
			if (prev && linphone_address_weak_equal(linphone_call_log_get_remote_address(prev),
													linphone_call_log_get_remote_address(log))) {
				bctbx_list_t *list = linphone_call_log_get_user_data(prev);
				list = bctbx_list_append(list, log);
				linphone_call_log_set_user_data(prev, list);
			} else {
				[eventsOnThisDay addObject:[NSValue valueWithPointer:linphone_call_log_ref(log)]];
			}
		}
		logs = bctbx_list_next(logs);
	}

	[self computeSections];

	[super loadData];
    
	if (IPAD) {
		if (![self selectFirstRow]) {
			HistoryDetailsView *view = VIEW(HistoryDetailsView);
			[view setCallLogId:nil];
		}
	}
}

+ (void) saveDataToUserDefaults {
	// As extensions is disabled by default, this function takes too much CPU.
#if 0
    const bctbx_list_t *logs = linphone_core_get_call_logs(LC);
    NSUserDefaults *mySharedDefaults = [[NSUserDefaults alloc] initWithSuiteName: @"group.belledonne-communications.linphone.widget"];
    NSMutableArray *logsShare = [NSMutableArray array];
    NSMutableDictionary *tmpStoreDict = [NSMutableDictionary dictionary];
    NSMutableArray *addedContacts = [NSMutableArray array];
    while (logs) {
        LinphoneCallLog *log = (LinphoneCallLog *)logs->data;
        const LinphoneAddress *address = linphone_call_log_get_remote_address(log);
        
        // if contact is already to be display, skip
        if ([addedContacts containsObject:[NSString stringWithUTF8String:linphone_address_as_string_uri_only(address)]]) {
            logs = bctbx_list_next(logs);
            continue;
        }
        // if null log id, skip
        if (!linphone_call_log_get_call_id(log)) {
            logs = bctbx_list_next(logs);
            continue;
        }
        
        NSMutableDictionary *dict = [NSMutableDictionary dictionary];
        
        [dict setObject:[NSString stringWithUTF8String:linphone_call_log_get_call_id(log)]
                 forKey:@"id"];
        [dict setObject:[NSString stringWithUTF8String:linphone_address_get_display_name(address)?:(linphone_address_get_username(address)?:"unknown")]
                 forKey:@"display"];
        UIImage *avatar = [FastAddressBook imageForAddress:address];
        if (avatar) {
            UIImage *image = [UIImage resizeImage:avatar
                                     withMaxWidth:200
                                     andMaxHeight:200];
            NSData *imageData = UIImageJPEGRepresentation(image, 1);
            [dict setObject:imageData
                     forKey:@"img"];
        }
        [tmpStoreDict setObject:dict
                         forKey:[NSDate dateWithTimeIntervalSince1970:linphone_call_log_get_start_date(log)]];
        [addedContacts addObject:[NSString stringWithUTF8String:linphone_address_as_string_uri_only(address)]];
        
        logs = bctbx_list_next(logs);
    }
    
    NSArray *sortedDates = [[NSMutableArray alloc]
                   initWithArray:[tmpStoreDict.allKeys sortedArrayUsingComparator:^NSComparisonResult(NSDate *d1, NSDate *d2) {
        return [d2 compare:d1];
    }]];
    
    // sort logs array on date
    for (NSDate *date in sortedDates) {
        [logsShare addObject:[tmpStoreDict objectForKey:date]];
        if (logsShare.count >= 4) //send no more data than needed
            break;
    }
    
    [mySharedDefaults setObject:logsShare forKey:@"logs"];
#endif
}

- (void)computeSections {
	NSArray *unsortedDays = [self.sections allKeys];
	_sortedDays = [[NSMutableArray alloc]
		initWithArray:[unsortedDays sortedArrayUsingComparator:^NSComparisonResult(NSDate *d1, NSDate *d2) {
		  return [d2 compare:d1]; // reverse order
		}]];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return _sortedDays.count;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	NSArray *logs = [_sections objectForKey:_sortedDays[section]];
	return logs.count;
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	CGRect frame = CGRectMake(0, 0, tableView.frame.size.width, 44);
	UIView *tempView = [[UIView alloc] initWithFrame:frame];
	if (@available(iOS 13, *)) {
		tempView.backgroundColor = [UIColor systemBackgroundColor];
	} else {
		tempView.backgroundColor = [UIColor whiteColor];
	}

	UILabel *tempLabel = [[UILabel alloc] initWithFrame:frame];
	tempLabel.backgroundColor = [UIColor clearColor];
	tempLabel.textColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A.png"]];
	NSDate *eventDate = _sortedDays[section];
	NSDate *currentDate = [self dateAtBeginningOfDayForDate:[NSDate date]];
	if ([eventDate isEqualToDate:currentDate]) {
		tempLabel.text = NSLocalizedString(@"TODAY", nil);
	} else if ([eventDate isEqualToDate:[currentDate dateByAddingTimeInterval:-3600 * 24]]) {
		tempLabel.text = NSLocalizedString(@"YESTERDAY", nil);
	} else {
		tempLabel.text = [LinphoneUtils timeToString:eventDate.timeIntervalSince1970 withFormat:LinphoneDateHistoryList]
							 .uppercaseString;
	}
	tempLabel.textAlignment = NSTextAlignmentCenter;
	tempLabel.font = [UIFont boldSystemFontOfSize:17];
	tempLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
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
		if (callLog != NULL) {
			if (IPAD) {
				UIHistoryCell *cell = (UIHistoryCell *)[self tableView:tableView cellForRowAtIndexPath:indexPath];
				[cell onDetails:self];
			} else {
				const LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
				[LinphoneManager.instance call:addr];
			}
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
		MSList *count = linphone_call_log_get_user_data(callLog);
		while (count) {
			linphone_core_remove_call_log(LC, count->data);
			count = count->next;
		}
		linphone_core_remove_call_log(LC, callLog);
		linphone_call_log_unref(callLog);
		[[_sections objectForKey:_sortedDays[indexPath.section]] removeObject:log];
		if (((NSArray *)[_sections objectForKey:_sortedDays[indexPath.section]]).count == 0) {
			[_sections removeObjectForKey:_sortedDays[indexPath.section]];
			[_sortedDays removeObjectAtIndex:indexPath.section];
			[tableView deleteSections:[NSIndexSet indexSetWithIndex:indexPath.section]
					 withRowAnimation:UITableViewRowAnimationFade];
		}

		[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath]
						 withRowAnimation:UITableViewRowAnimationFade];
		[tableView endUpdates];
	}
}

- (void)removeSelectionUsing:(void (^)(NSIndexPath *))remover {
	[super removeSelectionUsing:^(NSIndexPath *indexPath) {
	  id log = [_sections objectForKey:_sortedDays[indexPath.section]][indexPath.row];
	  LinphoneCallLog *callLog = [log pointerValue];
	  MSList *count = linphone_call_log_get_user_data(callLog);
	  while (count) {
		  linphone_core_remove_call_log(LC, count->data);
		  count = count->next;
	  }
	  linphone_core_remove_call_log(LC, callLog);
	  linphone_call_log_unref(callLog);
	  [[_sections objectForKey:_sortedDays[indexPath.section]] removeObject:log];
	  if (((NSArray *)[_sections objectForKey:_sortedDays[indexPath.section]]).count == 0) {
		  [_sections removeObjectForKey:_sortedDays[indexPath.section]];
		  [_sortedDays removeObjectAtIndex:indexPath.section];
	  }
	}];
}

@end
