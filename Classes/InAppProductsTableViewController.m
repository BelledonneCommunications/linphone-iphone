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
#import "DTAlertView.h"

@implementation InAppProductsTableViewController {
	InAppProductsManager *iapm;
	NSInteger currentExpanded;
}

- (void)viewWillAppear:(BOOL)animated {
	iapm = [[LinphoneManager instance] iapManager];
	currentExpanded = -1;
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
		cell = [[[InAppProductsCell alloc] initWithIdentifier:kCellId maximized:(currentExpanded == indexPath.row)] autorelease];
	}
	SKProduct *prod = [[[[LinphoneManager instance] iapManager] inAppProducts] objectAtIndex:indexPath.row];
	[cell.ptitle setText: [prod localizedTitle]];
	[cell.pdescription setText: [prod localizedDescription]];
	[cell.pprice setText: [NSString stringWithFormat:@"%@", [prod price]]];
	[cell.ppurchased setOn: [iapm isPurchased:prod]];
	cell.isMaximized = (currentExpanded == indexPath.row);
	cell.productID = prod.productIdentifier;
	
	LOGI(@"One more: %@", cell);
	return cell;
}

//- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
//	if(currentExpanded == indexPath.row) {
//		currentExpanded = -1;
//		[tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
//		return;
//	} else if(currentExpanded >= 0) {
//		NSIndexPath *previousPath = [NSIndexPath indexPathForRow:currentExpanded inSection:0];
//		currentExpanded = indexPath.row;
//		[tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:previousPath] withRowAnimation:UITableViewRowAnimationFade];
//	}
//	currentExpanded = indexPath.row;
//	[tableView reloadRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
//}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	InAppProductsCell *cell = (InAppProductsCell*)[tableView cellForRowAtIndexPath:indexPath];
	if (cell.ppurchased.isOn) {
		DTAlertView* alert = [[DTAlertView alloc] initWithTitle:NSLocalizedString(@"Already purchased", nil) message: [NSString stringWithFormat:NSLocalizedString(@"You already bought %@.",nil), cell.ptitle.text]];

		[alert addCancelButtonWithTitle:NSLocalizedString(@"OK", nil) block:nil];
		[alert show];
		[alert release];
	} else {
		//try to purchase item, and if successfull change the switch
		[[[LinphoneManager instance] iapManager] purchaseWithID: cell.productID];
	}
}

-(CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	return [InAppProductsCell getHeight:(currentExpanded == indexPath.row)];
}

@end
