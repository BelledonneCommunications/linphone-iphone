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
	_callLogs = [[NSMutableArray alloc] init];
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

- (void)loadData {
	[_callLogs removeAllObjects];
	const MSList *logs = linphone_core_get_call_logs([LinphoneManager getLc]);
	while (logs != NULL) {
		LinphoneCallLog *log = (LinphoneCallLog *)logs->data;
		if (missedFilter) {
			if (linphone_call_log_get_status(log) == LinphoneCallMissed) {
				[_callLogs addObject:[NSValue valueWithPointer:log]];
			}
		} else {
			[_callLogs addObject:[NSValue valueWithPointer:log]];
		}
		logs = ms_list_next(logs);
	}
	[super loadData];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [_callLogs count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"UIHistoryCell";
	UIHistoryCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIHistoryCell alloc] initWithIdentifier:kCellId];
	}

	id logId = [_callLogs objectAtIndex:indexPath.row];
	LinphoneCallLog *log = [logId pointerValue];
	[cell setCallLog:log];
	[super accessoryForCell:cell atPath:indexPath];
	return cell;
}

#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[super tableView:tableView didSelectRowAtIndexPath:indexPath];
	if ([self isEditing]) {
		LinphoneCallLog *callLog = [[_callLogs objectAtIndex:[indexPath row]] pointerValue];
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

- (UITableViewCellEditingStyle)tableView:(UITableView *)aTableView
		   editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	// Detemine if it's in editing mode
	if (self.editing) {
		return UITableViewCellEditingStyleDelete;
	}
	return UITableViewCellEditingStyleNone;
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	if (editingStyle == UITableViewCellEditingStyleDelete) {
		[tableView beginUpdates];
		LinphoneCallLog *callLog = [[_callLogs objectAtIndex:[indexPath row]] pointerValue];
		linphone_core_remove_call_log([LinphoneManager getLc], callLog);
		[_callLogs removeObjectAtIndex:[indexPath row]];
		[tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath]
						 withRowAnimation:UITableViewRowAnimationFade];
		[tableView endUpdates];
	}
}

@end
