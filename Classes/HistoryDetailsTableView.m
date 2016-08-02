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

	if (peer) {
		const bctbx_list_t *logs = linphone_core_get_call_history_for_address(LC, peer);
		while (logs != NULL) {
			LinphoneCallLog *log = (LinphoneCallLog *)logs->data;
			if (linphone_address_weak_equal(linphone_call_log_get_remote_address(log), peer)) {
				[callLogs addObject:[NSValue valueWithPointer:log]];
			}
			logs = bctbx_list_next(logs);
		}
	}
	[[self tableView] reloadData];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [callLogs count];
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	CGRect frame = CGRectMake(0, 0, tableView.frame.size.width, 44);
	UIView *tempView = [[UIView alloc] initWithFrame:frame];
	tempView.backgroundColor = [UIColor whiteColor];

	UILabel *tempLabel = [[UILabel alloc] initWithFrame:frame];
	tempLabel.backgroundColor = [UIColor clearColor];
	tempLabel.textColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_E.png"]];
	tempLabel.text = NSLocalizedString(@"Calls", nil);
	tempLabel.textAlignment = NSTextAlignmentCenter;
	tempLabel.font = [UIFont boldSystemFontOfSize:17];
	tempLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
	[tempView addSubview:tempLabel];

	return tempView;
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
		setText:[NSString stringWithFormat:@"%@ - %@",
										   [LinphoneUtils timeToString:callTime withFormat:LinphoneDateHistoryDetails],
										   [LinphoneUtils durationToString:duration]]];
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
