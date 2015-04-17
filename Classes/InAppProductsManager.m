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
#import "LinphoneManager.h"

NSString *const kLinphoneIAPurchaseNotification = @"LinphoneIAProductsNotification";

@implementation InAppProductsManager {
}

- (instancetype)init {
	if ((self = [super init]) != nil) {
		[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
		[self loadProducts];
	}
	return self;
}

- (void)loadProducts {
	if (! [SKPaymentQueue canMakePayments]) {
		return;
	}
	NSArray * list = [[[[LinphoneManager instance] lpConfigStringForKey:@"inapp_products_list"] stringByReplacingOccurrencesOfString:@" " withString:@""] componentsSeparatedByString:@","];

	_productsIDPurchased = [[NSMutableArray alloc] initWithCapacity:0];

	SKProductsRequest *productsRequest = [[SKProductsRequest alloc]
										  initWithProductIdentifiers:[NSSet setWithArray:list]];
	productsRequest.delegate = self;
	[productsRequest start];
}

- (void)productsRequest:(SKProductsRequest *)request
	 didReceiveResponse:(SKProductsResponse *)response {
	_productsAvailable = [[NSMutableArray arrayWithArray: response.products] retain];

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
	for (NSString *prod in _productsIDPurchased) {
		if ([prod isEqual: product.productIdentifier]) {
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
	LOGI(@"Restoring user purchases...");
	if (! [SKPaymentQueue canMakePayments]) {
		LOGF(@"Not allowed to do in app purchase!!!");
	}
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

- (void)checkReceipt: (SKPaymentTransaction*)transaction {
	NSData *receiptData = nil;
	NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
	// Test whether the receipt is present at the above URL
	if(![[NSFileManager defaultManager] fileExistsAtPath:[receiptURL path]]) {
		 receiptData = [NSData dataWithContentsOfURL:receiptURL];
	} else {
		LOGI(@"Could not find any receipt in application, using the transaction one!");
		receiptData = transaction.transactionReceipt;
	}

	// We must validate the receipt on our server
	NSURL *storeURL = [NSURL URLWithString:[[LinphoneManager instance] lpConfigStringForKey:@"inapp_receipt_validation_url"]];
	NSMutableURLRequest *storeRequest = [NSMutableURLRequest requestWithURL:storeURL];
	[storeRequest setHTTPMethod:@"POST"];
	[storeRequest setHTTPBody:receiptData];

	NSOperationQueue *queue = [[NSOperationQueue alloc] init];
	[NSURLConnection sendAsynchronousRequest:storeRequest
					 queue:queue
						   completionHandler:^(NSURLResponse *response, NSData *data, NSError *connectionError) {
							   if (data == nil) {
								   LOGE(@"Server replied nothing, aborting now.");
							   } else if (connectionError) {
								   LOGE(@"Could not verify the receipt, aborting now: (%d) %@.", connectionError.code, connectionError);
							   } else {
								   NSError *jsonError;
								   NSDictionary *jsonResponse = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
								   if (jsonError == nil) {
									   // Hourray, we can finally unlock purchase stuff in app!
									   LOGI(@"In apps purchased are %@", jsonResponse);
									   LOGE(@"To do: [_productsIDPurchased addObject:transaction.payment.productIdentifier];");
									   [self postNotificationforStatus:IAPReceiptSucceeded];
									   return;
								   } else {
									   LOGE(@"Impossible to parse receipt JSON response, aborting now because of %@: %@.", jsonError, [[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding]);
								   }
							   }
							   // Something failed when checking the receipt
							   [self postNotificationforStatus:IAPReceiptFailed];
						   }];
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {
	for(SKPaymentTransaction * transaction in transactions) {
		switch (transaction.transactionState) {
			case SKPaymentTransactionStatePurchasing:
				break;
			case SKPaymentTransactionStatePurchased:
			case SKPaymentTransactionStateRestored:
				[self checkReceipt: transaction];
				[self completeTransaction:transaction forStatus:IAPPurchaseSucceeded];
				break;

//			case SKPaymentTransactionStatePurchasing:
//				break;
//			case SKPaymentTransactionStateDeferred:
//				[self transactionDeferred:transaction];
//				break;
//			case SKPaymentTransactionStatePurchased:
//				[self completeTransaction:transaction];
//				break;
//			case SKPaymentTransactionStateRestored:
//				[self restoreTransaction:transaction];
//				break;
//			case SKPaymentTransactionStateFailed:
//				[self failedTransaction:transaction];
//				break;
			default:
				_errlast = [NSString stringWithFormat:@"Purchase of %@ failed.",transaction.payment.productIdentifier];
				[self completeTransaction:transaction forStatus:IAPPurchaseFailed];
				break;
		}
	}
}

- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray *)transactions {
	for(SKPaymentTransaction * transaction in transactions) {
		LOGI(@"%@ was removed from the payment queue.", transaction.payment.productIdentifier);
	}
}

- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error {
	if (error.code != SKErrorPaymentCancelled) {
		_errlast = [error localizedDescription];
		[self postNotificationforStatus:IAPRestoreFailed];
	}
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue {
	LOGI(@"All restorable transactions have been processed by the payment queue.");
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
