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
	const MSList *calls = linphone_core_get_calls(LC);
	int i = -1;
	while (calls) {
		if (linphone_call_params_get_local_conference_mode(linphone_call_get_current_params(calls->data))) {
			i++;
			if (i == row)
				break;
		}
		calls = calls->next;
	}
	return (calls ? calls->data : NULL);
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
	const MSList *calls = linphone_core_get_calls(LC);
	int count = 0;
	while (calls) {
		if (linphone_call_params_get_local_conference_mode(linphone_call_get_current_params(calls->data))) {
			count++;
		}
		calls = calls->next;
	}
	return count;
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
