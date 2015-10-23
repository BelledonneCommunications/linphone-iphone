//
//  HistoryDetailsTableViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 27/07/15.
//
//

#import "HistoryDetailsTableView.h"
#import "LinphoneManager.h"
#import "Utils.h"

@implementation HistoryDetailsTableView

- (void)loadDataForAddress:(const LinphoneAddress *)peer {
	if (callLogs == nil) {
		callLogs = [[NSMutableArray alloc] init];
	} else {
		[callLogs removeAllObjects];
	}
	const MSList *logs = linphone_core_get_call_history_for_address([LinphoneManager getLc], peer);
	while (logs != NULL) {
		LinphoneCallLog *log = (LinphoneCallLog *)logs->data;
		[callLogs addObject:[NSValue valueWithPointer:log]];
		logs = ms_list_next(logs);
	}
	[[self tableView] reloadData];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [callLogs count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"UITableViewCell";
	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UITableViewCell alloc] init];
	}

	LinphoneCallLog *log = [[callLogs objectAtIndex:[indexPath row]] pointerValue];
	int duration = linphone_call_log_get_duration(log);
	time_t callTime = linphone_call_log_get_start_date(log);
	cell.textLabel.textAlignment = NSTextAlignmentCenter;
	[cell.textLabel
		setText:[NSString stringWithFormat:@"%@ - %d sec",
										   [LinphoneUtils timeToString:callTime withStyle:NSDateFormatterMediumStyle],
										   duration]];
	BOOL outgoing = (linphone_call_log_get_dir(log) == LinphoneCallOutgoing);

	if (linphone_call_log_get_status(log) == LinphoneCallMissed) {
		cell.imageView.image = [UIImage imageNamed:@"call_missed.png"];
	} else if (outgoing) {
		cell.imageView.image = [UIImage imageNamed:@"call_outgoing.png"];
	} else {
		cell.imageView.image = [UIImage imageNamed:@"call_incoming.png"];
	}
	return cell;
}

@end
