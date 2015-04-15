/* InAppProductsManager.h
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

#import "InAppProductsManager.h"
#import "Utils.h"

@implementation InAppProductsManager {
	bool ready;
}

- (instancetype)init {
	if ((self = [super init]) != nil) {
		[self loadProducts];
		ready = false;
	}
	return self;
}

- (void)loadProducts {
	if (! [SKPaymentQueue canMakePayments]) {
		return;
	}
	
	NSURL *url = [[NSBundle mainBundle] URLForResource:@"in_app_products"
										 withExtension:@"plist"];

	inAppProducts = [NSArray arrayWithContentsOfURL:url];

	SKProductsRequest *productsRequest = [[SKProductsRequest alloc]
										  initWithProductIdentifiers:[NSSet setWithArray:inAppProducts]];
	productsRequest.delegate = self;
	[productsRequest start];
}

- (void)productsRequest:(SKProductsRequest *)request
	 didReceiveResponse:(SKProductsResponse *)response {
	inAppProducts = response.products;
	LOGI(@"Found %lu products purchasable", inAppProducts.count);

	for (NSString *invalidIdentifier in response.invalidProductIdentifiers) {
		LOGE(@"Product Identifier with invalid ID %@", invalidIdentifier);
	}
	ready = true;
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {

}

@end