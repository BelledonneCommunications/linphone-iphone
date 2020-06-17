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
