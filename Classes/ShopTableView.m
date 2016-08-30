//
//  ShopTableView.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 30/08/16.
//
//

#import "ShopTableView.h"

@implementation ShopTableView

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return LinphoneManager.instance.iapManager.productsAvailable.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell = [[UITableViewCell alloc] init];

	cell.textLabel.text = LinphoneManager.instance.iapManager.productsAvailable[indexPath.row];

	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:NO];

	UITableViewCell *cell = [self tableView:tableView cellForRowAtIndexPath:indexPath];
	[LinphoneManager.instance.iapManager purchaseWithID:cell.textLabel.text];
}

@end
