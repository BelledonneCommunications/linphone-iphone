/* InCallTableViewController.h
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import "CallPausedTableView.h"
#import "UICallPausedCell.h"
#import "UIConferenceHeader.h"
#import "LinphoneManager.h"
#import "Utils.h"

@implementation CallPausedTableView

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	updateTime =
		[NSTimer scheduledTimerWithTimeInterval:1 target:self selector:@selector(update) userInfo:nil repeats:YES];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	if (updateTime != nil) {
		[updateTime invalidate];
		updateTime = nil;
	}
}

#pragma mark - UI change

- (void)update {
	[self.tableView reloadData];
	CGRect newOrigin = self.tableView.frame;
	newOrigin.size.height =
		[self tableView:self.tableView heightForRowAtIndexPath:[NSIndexPath indexPathForRow:0 inSection:0]] *
		[self tableView:self.tableView numberOfRowsInSection:0];
	newOrigin.origin.y = self.tableView.frame.origin.y + self.tableView.frame.size.height - newOrigin.size.height;
	self.tableView.frame = newOrigin;
}

#pragma mark - UITableViewDataSource Functions
- (LinphoneCall *)pausedCallForRow:(NSInteger)row {
	const MSList *calls = linphone_core_get_calls([LinphoneManager getLc]);
	return (row < ms_list_size(calls) - 1) ? ms_list_nth_data(calls, (int)row) : NULL;
}

#pragma mark - UITableViewDataSource Functions

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = NSStringFromClass(UICallPausedCell.class);
	UICallPausedCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UICallPausedCell alloc] initWithIdentifier:kCellId];
	}
	[cell setCall:[self pausedCallForRow:indexPath.row]];
	return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return ms_list_size(linphone_core_get_calls([LinphoneManager getLc])) - 1;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	return [self tableView:tableView cellForRowAtIndexPath:indexPath].frame.size.height;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
	return 1e-5;
}
- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	return 1e-5;
}

@end
