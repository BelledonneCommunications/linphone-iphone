/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
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

#import "CallConferenceTableView.h"
#import "UICallConferenceCell.h"
#import "LinphoneManager.h"
#import "Utils.h"

@implementation CallConferenceTableView

#pragma mark - UI change

- (void)update {
	[self.tableView reloadData];
}

#pragma mark - UITableViewDataSource Functions
- (LinphoneCall *)conferenceCallForRow:(NSInteger)row {
	LinphoneConference *c = linphone_core_get_conference(LC);
	return linphone_core_get_call_by_remote_address2(LC, bctbx_list_nth_data(linphone_conference_get_participants(c),(int)row));
}

#pragma mark - UITableViewDataSource Functions

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = NSStringFromClass(UICallConferenceCell.class);
	UICallConferenceCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UICallConferenceCell alloc] initWithIdentifier:kCellId];
	}
	[cell setCall:[self conferenceCallForRow:indexPath.row]];
	return cell;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	LinphoneConference *c = linphone_core_get_conference(LC);
	return c == nil ? 0 : bctbx_list_size(linphone_conference_get_participants(c));
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
	return 1e-5;
}
- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	return 1e-5;
}

@end
