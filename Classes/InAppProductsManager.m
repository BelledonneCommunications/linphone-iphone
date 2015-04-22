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

#import <XMLRPCConnection.h>
#import <XMLRPCConnectionManager.h>
#import <XMLRPCResponse.h>
#import <XMLRPCRequest.h>

#import "Utils.h"
#import "LinphoneManager.h"

NSString *const kLinphoneIAPurchaseNotification = @"LinphoneIAProductsNotification";


@implementation InAppProductsXMLRPCDelegate {
	InAppProductsManager *iapm;
}

#pragma mark - XMLRPCConnectionDelegate Functions

- (void)request:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response {
	[[[LinphoneManager instance] iapManager] XMLRPCRequest:request didReceiveResponse:response];
}

- (void)request:(XMLRPCRequest *)request didFailWithError:(NSError *)error {
	[[[LinphoneManager instance] iapManager] XMLRPCRequest:request didFailWithError:error];
}

- (BOOL)request:(XMLRPCRequest *)request canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
	return FALSE;
}

- (void)request:(XMLRPCRequest *)request didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {

}

- (void)request:(XMLRPCRequest *)request didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
	
}

@end

@implementation InAppProductsManager {
}

- (instancetype)init {
	if ((self = [super init]) != nil) {
		_xmlrpc = [[InAppProductsXMLRPCDelegate alloc] init];
		_status = IAPNotReadyYet;
		[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
		[self loadProducts];
	}
	return self;
}

#define INAPP_AVAIL() ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0) && ([SKPaymentQueue canMakePayments])

- (void)loadProducts {
	if (!INAPP_AVAIL()) return;

	NSArray * list = [[[[LinphoneManager instance] lpConfigStringForKey:@"products_list" forSection:@"in_app_purchase"] stringByReplacingOccurrencesOfString:@" " withString:@""] componentsSeparatedByString:@","];

	_productsIDPurchased = [[NSMutableArray alloc] initWithCapacity:0];

	SKProductsRequest *productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:[NSSet setWithArray:list]];
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

- (BOOL)isPurchasedWithID:(NSString *)productID {
	for (NSString *prod in _productsIDPurchased) {
		if ([prod isEqual: productID]) {
			bool isBought = true;
			LOGE(@"%@ is %s bought.", prod, isBought?"":"NOT");
			return isBought;
		}
	}
	return false;
}

- (void)purchaseWithID:(NSString *)productID {
	LOGI(@"Trying to purchase %@", productID);
	for (SKProduct *product in _productsAvailable) {
		if ([product.productIdentifier compare:productID options:NSLiteralSearch] == NSOrderedSame) {
			SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:product];
			[[SKPaymentQueue defaultQueue] addPayment:payment];
			return;
		}
	}
	LOGE(@"Impossible to find product with ID %@...", productID);
	[self postNotificationforStatus:IAPPurchaseFailed];
}

-(void)restore {
	LOGI(@"Restoring user purchases...");
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

- (void)requestDidFinish:(SKRequest *)request {
	if([request isKindOfClass:[SKReceiptRefreshRequest class]]) {
		NSURL *receiptUrl = [[NSBundle mainBundle] appStoreReceiptURL];
		if ([[NSFileManager defaultManager] fileExistsAtPath:[receiptUrl path]]) {
			LOGI(@"App Receipt exists");
			[self checkReceipt:nil];
		} else {
			LOGE(@"Receipt request done but there is no receipt");

			// This can happen if the user cancels the login screen for the store.
			// If we get here it means there is no receipt and an attempt to get it failed because the user cancelled the login.
			//[self trackFailedAttempt];
		}
	}
}

- (void)request:(SKRequest *)skrequest didFailWithError:(NSError *)error {
	LOGE(@"Did not get receipt: %@", error.localizedDescription);
	[self setStatus:IAPReceiptFailed];
}

- (void)checkReceipt: (SKPaymentTransaction*)transaction {
	NSData *receiptData = nil;
	NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
	// Test whether the receipt is present at the above URL
	if([[NSFileManager defaultManager] fileExistsAtPath:[receiptURL path]]) {
		receiptData = [NSData dataWithContentsOfURL:receiptURL];
		LOGI(@"Found appstore receipt containing: %@", receiptData);
	} else {
		// We are probably in sandbox environment, trying to retrieve it...
		SKRequest* req = [[SKReceiptRefreshRequest alloc] init];
		LOGI(@"Receipt not found yet, trying to retrieve it...");
		req.delegate = self;
		[req start];
		return;
	}

	// We must validate the receipt on our server
	NSURL *URL = [NSURL URLWithString:[[LinphoneManager instance] lpConfigStringForKey:@"receipt_validation_url" forSection:@"in_app_purchase"]];

	XMLRPCRequest *request = [[XMLRPCRequest alloc] initWithURL: URL];
	[request setMethod: @"create_account_from_in_app_purchase" withParameters:[NSArray arrayWithObjects:
																			   @"toto@test.linphone.org",
																			   @"toto",
																			   receiptData,
																			   @"",
																			   @"apple",
																			   nil]];
	XMLRPCConnectionManager *manager = [XMLRPCConnectionManager sharedManager];
	[manager spawnConnectionWithXMLRPCRequest: request delegate: self.xmlrpc];
	LOGE(@"XMLRPC query %@: %@", [request method], [request body]);
	[request release];
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
			case SKPaymentTransactionStateDeferred:
				//waiting for parent approval
				break;
			case SKPaymentTransactionStateFailed:
				_errlast = [NSString stringWithFormat:@"Purchase of %@ failed: %@.",transaction.payment.productIdentifier,transaction.error.localizedDescription];
				LOGE(@"%@", _errlast);
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

- (void)retrievePurchases {
	LOGE(@"todo");//[self checkReceipt:nil];
}

- (void)XMLRPCRequest:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response {
	LOGI(@"XMLRPC response %@: %@", [request method], [response body]);
	//	[waitView setHidden:true];
	if ([response isFault]) {
		LOGE(@"Communication issue (%@)", [response faultString]);
		//		NSString *errorString = [NSString stringWithFormat:NSLocalizedString(@"Communication issue (%@)", nil), [response faultString]];
		//		UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Communication issue",nil)
		//															message:errorString
		//														   delegate:nil
		//												  cancelButtonTitle:NSLocalizedString(@"Continue",nil)
		//												  otherButtonTitles:nil,nil];
		//		[errorView show];
		//		[errorView release];
	} else if([response object] != nil) {
		//Don't handle if not object: HTTP/Communication Error
		if([[request method] isEqualToString:@"check_account"]) {
			if([response object] == [NSNumber numberWithInt:0]) {
				//				UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Check issue",nil)
				//																	message:NSLocalizedString(@"Username already exists", nil)
				//																   delegate:nil
				//														  cancelButtonTitle:NSLocalizedString(@"Continue",nil)
				//														  otherButtonTitles:nil,nil];
				//				[errorView show];
				//				[errorView release];!
				[self postNotificationforStatus:IAPReceiptSucceeded];
				return;
			}
		}
	}
	[self postNotificationforStatus:IAPReceiptFailed];
}

- (void)XMLRPCRequest:(XMLRPCRequest *)request didFailWithError:(NSError *)error {
	LOGE(@"Communication issue (%@)", [error localizedDescription]);
	//	NSString *errorString = [NSString stringWithFormat:NSLocalizedString(@"Communication issue (%@)", nil), [error localizedDescription]];
	//	UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Communication issue", nil)
	//														message:errorString
	//													   delegate:nil
	//											  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
	//											  otherButtonTitles:nil,nil];
	//	[errorView show];
	//	[errorView release];
	//	[waitView setHidden:true];

	[self postNotificationforStatus:IAPReceiptFailed];
}
@end

