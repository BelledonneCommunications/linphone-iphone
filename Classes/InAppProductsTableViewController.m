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
	NSInteger currentExpanded;
	InAppProductsManager *iapm;
}

- (void)viewWillAppear:(BOOL)animated {
	currentExpanded = -1;
	iapm = [[LinphoneManager instance] iapManager];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(onIAPPurchaseNotification:)
												 name:IAPAvailableSucceeded
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(onIAPPurchaseNotification:)
												 name:IAPRestoreSucceeded
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(onIAPPurchaseNotification:)
												 name:IAPPurchaseSucceeded
											   object:nil];
	[[self tableView] reloadData];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];

	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:IAPAvailableSucceeded
												  object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:IAPRestoreSucceeded
												  object:nil];
	[[NSNotificationCenter defaultCenter] removeObserver:self
													name:IAPPurchaseSucceeded
												  object:nil];
}

- (void)onIAPPurchaseNotification:(NSNotification*)notif {
	[[self tableView] reloadData];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [iapm productsAvailable].count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"InAppProductsCell";
	InAppProductsCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[[InAppProductsCell alloc] initWithIdentifier:kCellId maximized:(currentExpanded == indexPath.row)] autorelease];
	}
	SKProduct *prod = [[[[LinphoneManager instance] iapManager] productsAvailable] objectAtIndex:indexPath.row];
	[cell fillFromProduct:prod];
	cell.isMaximized = (currentExpanded == indexPath.row);
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
//		[[[LinphoneManager instance] iapManager] purchaseWithID: cell.productID];
	}
}

-(CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	return [InAppProductsCell getHeight:(currentExpanded == indexPath.row)];
}

@end
