//
//  InAppProductsTableViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 16/04/15.
//
//

#import "InAppProductsTableViewController.h"
#import "InAppProductsCell.h"
#import "InAppProductsManager.h"
#import "LinphoneManager.h"

@implementation InAppProductsTableViewController {
	InAppProductsManager *iapm;
	NSInteger currentExpanded;
}

- (void)viewWillAppear:(BOOL)animated {
	iapm = [[LinphoneManager instance] iapManager];

	[iapm loadProducts];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [iapm inAppProducts].count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"InAppProductsCell";
	InAppProductsCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[[InAppProductsCell alloc] initWithIdentifier:kCellId] autorelease];
	}
	SKProduct *prod = [[[[LinphoneManager instance] iapManager] inAppProducts] objectAtIndex:indexPath.row];
	[cell.ptitle setText: [prod localizedTitle]];
	[cell.pdescription setText: [prod localizedDescription]];
	[cell.pprice setText: [NSString stringWithFormat:@"%@", [prod price]]];
	[cell.ppurchased setEnabled: [iapm isPurchased:prod]];
	cell.isMaximized = (currentExpanded == indexPath.row);

	LOGI(@"One more: %@", cell);
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	if(currentExpanded == indexPath.row) {
		currentExpanded = -1;
		[tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
		return;
	} else if(currentExpanded >= 0) {
		NSIndexPath *previousPath = [NSIndexPath indexPathForRow:currentExpanded inSection:0];
		currentExpanded = indexPath.row;
		[tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:previousPath] withRowAnimation:UITableViewRowAnimationFade];
	}
	currentExpanded = indexPath.row;
	[tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
}

-(CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	return [InAppProductsCell getHeight:(currentExpanded == indexPath.row)];
}

@end
