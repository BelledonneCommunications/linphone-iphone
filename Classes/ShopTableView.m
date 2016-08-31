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
	UITableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:NSStringFromClass(self.class)];
	if (!cell) {
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleSubtitle
									  reuseIdentifier:NSStringFromClass(self.class)];
	}
	SKProduct *product = LinphoneManager.instance.iapManager.productsAvailable[indexPath.row];

	NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
	[numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
	[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
	[numberFormatter setLocale:product.priceLocale];
	NSString *price = [numberFormatter stringFromNumber:product.price];

	cell.textLabel.text = [NSString stringWithFormat:@"%@ (%@)", product.localizedTitle, price];
	cell.detailTextLabel.text = product.localizedDescription;
	cell.detailTextLabel.numberOfLines = 2;
	cell.detailTextLabel.minimumScaleFactor = .5;
	cell.detailTextLabel.adjustsFontSizeToFitWidth = cell.detailTextLabel.adjustsLetterSpacingToFitWidth = YES;
	cell.accessoryType = UITableViewCellAccessoryDetailButton;
	[cell setImage:[UIImage imageNamed:@"linphone_logo"]];

	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:NO];

	SKProduct *product = LinphoneManager.instance.iapManager.productsAvailable[indexPath.row];
	[LinphoneManager.instance.iapManager purchaseWithID:product.productIdentifier];
}

@end
