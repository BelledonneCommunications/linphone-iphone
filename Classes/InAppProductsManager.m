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

NSString *const kInAppProductsReady = @"InAppProductsReady";

@implementation InAppProductsManager {
	bool ready;
}

- (instancetype)init {
	if ((self = [super init]) != nil) {
		ready = false;
		[self loadProducts];
	}
	return self;
}

- (void)loadProducts {
	if (! [SKPaymentQueue canMakePayments]) {
		return;
	}
	//TODO: move this list elsewhere
	NSArray * list = [[NSArray alloc] initWithArray:@[@"test.tunnel"]];

	SKProductsRequest *productsRequest = [[SKProductsRequest alloc]
										  initWithProductIdentifiers:[NSSet setWithArray:list]];
	productsRequest.delegate = self;
	[productsRequest start];
}

- (void)productsRequest:(SKProductsRequest *)request
	 didReceiveResponse:(SKProductsResponse *)response {
	_inAppProducts = [response.products retain];
	LOGI(@"Found %lu products purchasable", (unsigned long)_inAppProducts.count);

	for (NSString *invalidIdentifier in response.invalidProductIdentifiers) {
		LOGE(@"Product Identifier with invalid ID %@", invalidIdentifier);
	}
	ready = true;

	NSDictionary* dict = [NSDictionary dictionaryWithObjectsAndKeys:
						  _inAppProducts, @"products",
						  nil];

	dispatch_async(dispatch_get_main_queue(), ^(void){
		[[NSNotificationCenter defaultCenter] postNotificationName:kInAppProductsReady object:self userInfo:dict];
	});
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {

}

- (BOOL)isPurchased:(SKProduct*)product {
	for (SKProduct *prod in _inAppProducts) {
		if (prod == product) {
			LOGE(@"Is %@ bought? assuming NO", product.localizedTitle);
			return false; //todo
		}
	}
	return false;
}

- (void)purchaseWithID:(NSString *)productId {
	
}

@end