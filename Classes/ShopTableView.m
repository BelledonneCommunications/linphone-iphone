//
//  ShopTableView.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 30/08/16.
//
//

#import "ShopTableView.h"
#import "ShopView.h"
#import "PhoneMainView.h"
#import "LinphoneUI/UIShopTableCell.h"

@implementation ShopTableView

- (void)viewDidLoad {
	[super viewDidLoad];

	// remove separators between empty items, cf
	self.tableView.tableFooterView = [[UIView alloc] initWithFrame:CGRectZero];
}

#pragma mark - Table view data source

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return LinphoneManager.instance.iapManager.productsAvailable.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {

	static NSString *kCellId = @"UIShopTableCell";
	UIShopTableCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIShopTableCell alloc] initWithIdentifier:kCellId];
	}

	SKProduct *product = LinphoneManager.instance.iapManager.productsAvailable[indexPath.row];

	NSNumberFormatter *numberFormatter = [[NSNumberFormatter alloc] init];
	[numberFormatter setFormatterBehavior:NSNumberFormatterBehavior10_4];
	[numberFormatter setNumberStyle:NSNumberFormatterCurrencyStyle];
	[numberFormatter setLocale:product.priceLocale];
	NSString *price = [numberFormatter stringFromNumber:product.price];

	cell.nameLabel.text = [NSString stringWithFormat:@"%@ (%@)", product.localizedTitle, price];
	cell.descriptionLabel.numberOfLines = 2;
	cell.descriptionLabel.minimumScaleFactor = .5;
	cell.descriptionLabel.adjustsFontSizeToFitWidth = cell.detailTextLabel.adjustsLetterSpacingToFitWidth = YES;
	cell.descriptionLabel.text = [NSString stringWithFormat:@"%@", product.localizedDescription];
	// LOGE(@"ShopTableView : name = %@ - descr = %@",
	//	 [NSString stringWithFormat:@"%@ (%@)", product.localizedTitle, price], product.localizedDescription);
	[cell.linphoneImage setImage:[UIImage imageNamed:@"linphone_logo"]];

	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:NO];

	SKProduct *product = LinphoneManager.instance.iapManager.productsAvailable[indexPath.row];
	[LinphoneManager.instance.iapManager purchaseWithID:product.productIdentifier];
}

@end
