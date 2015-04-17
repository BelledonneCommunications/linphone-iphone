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

NSString *const kLinphoneIAPurchaseNotification = @"LinphoneIAProductsNotification";

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
	NSArray * list = [[[NSArray alloc] initWithArray:@[@"test.tunnel"]] autorelease];

	SKProductsRequest *productsRequest = [[SKProductsRequest alloc]
										  initWithProductIdentifiers:[NSSet setWithArray:list]];
	productsRequest.delegate = self;
	[productsRequest start];
}

- (void)productsRequest:(SKProductsRequest *)request
	 didReceiveResponse:(SKProductsResponse *)response {
	_productsAvailable= [[NSMutableArray arrayWithArray: response.products] retain];

	LOGI(@"Found %lu products available", (unsigned long)_productsAvailable.count);
	
	if (response.invalidProductIdentifiers.count > 0) {
		for (NSString *invalidIdentifier in response.invalidProductIdentifiers) {
			LOGW(@"Found product Identifier with invalid ID '%@'", invalidIdentifier);
		}
		[self postNotificationforStatus:IAPAvailableFailed];
	} else {
		[self postNotificationforStatus:IAPAvailableSucceeded];
	}
}

- (BOOL)isPurchased:(SKProduct*)product {
	for (SKProduct *prod in _productsPurchased) {
		if (prod == product) {
			bool isBought = true;
			LOGE(@"%@ is %s bought.", product.localizedTitle, isBought?"":"NOT");
			return isBought;
		}
	}
	return false;
}

- (void)purchaseWithID:(NSString *)productId {
	for (SKProduct *product in _productsAvailable) {
		if ([product.productIdentifier compare:productId options:NSLiteralSearch] == NSOrderedSame) {
			SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:product];
			[[SKPaymentQueue defaultQueue] addPayment:payment];
			return;
		}
	}
	[self postNotificationforStatus:IAPPurchaseFailed];
}

-(void)restore {
	_productsRestored = [[NSMutableArray alloc] initWithCapacity:0];
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {
	for(SKPaymentTransaction * transaction in transactions) {
		switch (transaction.transactionState) {
			case SKPaymentTransactionStatePurchasing:
				break;
			case SKPaymentTransactionStatePurchased:
			case SKPaymentTransactionStateRestored:
				[_productsPurchased addObject:transaction];
				[self completeTransaction:transaction forStatus:IAPPurchaseSucceeded];
				break;
			default:
				_errlast = [NSString stringWithFormat:@"Purchase of %@ failed.",transaction.payment.productIdentifier];
				[self completeTransaction:transaction forStatus:IAPPurchaseFailed];
				break;
		}
	}
}

- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray *)transactions {
	for(SKPaymentTransaction * transaction in transactions) {
		NSLog(@"%@ was removed from the payment queue.", transaction.payment.productIdentifier);
	}
}

- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error {
	if (error.code != SKErrorPaymentCancelled) {
		_errlast = [error localizedDescription];
		[self postNotificationforStatus:IAPRestoreFailed];
	}
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
	LOGI(@"All restorable transactions have been processed by the payment queue.");
//	for (SKPayment *payment in queue) {
//	[queue transactions]
//		[_productsRestored addObject:payment.productIdentifier];
//	}

	for (SKPaymentTransaction *transaction in queue.transactions) {
		NSString *productID = transaction.payment.productIdentifier;
		[_productsRestored addObject:productID];
		NSLog (@"product id is %@" , productID);
	}
}

-(void)postNotificationforStatus:(IAPPurchaseNotificationStatus)status {
	_status = status;
	[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneIAPurchaseNotification object:self];
	LOGI(@"Triggering notification for status %@", status);
}

-(void)completeTransaction:(SKPaymentTransaction *)transaction forStatus:(IAPPurchaseNotificationStatus)status {
	if (transaction.error.code != SKErrorPaymentCancelled) {
		[self postNotificationforStatus:status];
	} else {
		_status = status;
	}

	// Remove the transaction from the queue for purchased and restored statuses
	[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
}

@end
