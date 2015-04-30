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

// In app purchase are not supported by the Simulator
#import <XMLRPCConnection.h>
#import <XMLRPCConnectionManager.h>
#import <XMLRPCResponse.h>
#import <XMLRPCRequest.h>

#import "Utils.h"
#import "LinphoneManager.h"

#import "PhoneMainView.h"
#import "InAppProductsViewController.h"



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
	NSString *accountCreationSipURI;
	NSString *accountCreationPassword;
}

#if !TARGET_IPHONE_SIMULATOR
- (instancetype)init {
	if ((self = [super init]) != nil) {
		LOGE(@"Todo: //waiting for parent approval");
		LOGE(@"Todo: if cancel date, no purchase");
		_xmlrpc = [[InAppProductsXMLRPCDelegate alloc] init];
		_status = IAPNotReadyYet;
		[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
		[self loadProducts];
	}
	return self;
}

#define INAPP_AVAIL() ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7.0) && ([SKPaymentQueue canMakePayments])

#pragma mark ProductListLoading

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
		NSDictionary* dict = @{@"error_msg": NSLocalizedString(@"Invalid products identifier", nil)};
		[self postNotificationforStatus:IAPAvailableFailed withDict:dict];
	} else {
		[self postNotificationforStatus:IAPAvailableSucceeded withDict:nil];
	}
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error {
	LOGE(@"Impossible to retrieve list of products: %@", [error localizedFailureReason]);
	NSDictionary* dict = @{@"error_msg": error ? [error localizedDescription] : NSLocalizedString(@"Product not available", commit)};
	[self postNotificationforStatus:IAPAvailableFailed withDict:dict];
	//well, let's retry...
	[self loadProducts];
}

#pragma mark Other
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

- (SKProduct*) productIDAvailable:(NSString*)productID {
	for (SKProduct *product in _productsAvailable) {
		if ([product.productIdentifier compare:productID options:NSLiteralSearch] == NSOrderedSame) {
			return product;
		}
	}
	return nil;
}
- (BOOL)purchaseWitID:(NSString *)productID {
	SKProduct *prod = [self productIDAvailable:productID];
	if (prod) {
		NSDictionary* dict = @{@"product_id": productID};
		[self postNotificationforStatus:IAPPurchaseTrying withDict:dict];
		SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:prod];
		[[SKPaymentQueue defaultQueue] addPayment:payment];
		[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
		return TRUE;
	} else {
		NSDictionary* dict = @{@"product_id": productID, @"error_msg": @"Product not available"};
		[self postNotificationforStatus:IAPPurchaseFailed withDict:dict];
		return FALSE;
	}
}

- (void)purchaseAccount:(NSString *)sipURI withPassword:(NSString *)password {
	NSString* productID = [[LinphoneManager instance] lpConfigStringForKey:@"paid_account_id" forSection:@"in_app_purchase"];
	accountCreationSipURI = [sipURI retain];
	accountCreationPassword = [password retain];
	if ([self purchaseWitID:productID]) {
		accountCreationPassword = nil;
		accountCreationSipURI = nil;
	}
}

-(void)restore {
	LOGI(@"Restoring user purchases...");
	//force new query of our server
	latestReceiptMD5 = nil;
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

- (void)requestDidFinish:(SKRequest *)request {
	if([request isKindOfClass:[SKReceiptRefreshRequest class]]) {
		NSURL *receiptUrl = [[NSBundle mainBundle] appStoreReceiptURL];
		if ([[NSFileManager defaultManager] fileExistsAtPath:[receiptUrl path]]) {
			LOGI(@"App Receipt exists");
			[self validateReceipt:nil];
		} else {
			// This can happen if the user cancels the login screen for the store.
			// If we get here it means there is no receipt and an attempt to get it failed because the user cancelled the login.
			LOGF(@"Receipt request done but there is no receipt");
		}
	}
}

- (void)validateReceipt: (SKPaymentTransaction*)transaction {
	NSString *receiptBase64 = nil;
	NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];

	// Test whether the receipt is present at the above URL
	if(![[NSFileManager defaultManager] fileExistsAtPath:[receiptURL path]]) {
		// We are probably in sandbox environment, trying to retrieve it...
		SKRequest* req = [[SKReceiptRefreshRequest alloc] init];
		LOGI(@"Receipt not found yet, trying to retrieve it...");
		req.delegate = self;
		[req start];
		return;
	}

	LOGI(@"Found appstore receipt");
	receiptBase64 = [[NSData dataWithContentsOfURL:receiptURL] base64EncodedStringWithOptions:0];
	//only check the receipt if it has changed
	if (latestReceiptMD5 == nil || ! [latestReceiptMD5 isEqualToString:[receiptBase64 md5]]) {
		// We must validate the receipt on our server
		NSURL *URL = [NSURL URLWithString:[[LinphoneManager instance] lpConfigStringForKey:@"receipt_validation_url" forSection:@"in_app_purchase"]];

		XMLRPCRequest *request = [[XMLRPCRequest alloc] initWithURL: URL];

		// Happen when restoring user purchases at application start or if user click the "restore" button
		if (transaction == nil) {
			[request setMethod: @"get_expiration_date" withParameters:[NSArray arrayWithObjects:
																	   @"",
																	   receiptBase64,
																	   @"",
																	   @"apple",
																	   nil]];
		} else if ([transaction.payment.productIdentifier isEqualToString:[[LinphoneManager instance] lpConfigStringForKey:@"paid_account_id" forSection:@"in_app_purchase"]]) {
			//buying for the first time: need to create the account
			if ([transaction.transactionIdentifier isEqualToString:transaction.originalTransaction.transactionIdentifier]) {
				[request setMethod: @"create_account_from_in_app_purchase" withParameters:[NSArray arrayWithObjects:
																						   @"",
																						   accountCreationSipURI,
																						   accountCreationPassword,
																						   receiptBase64,
																						   @"",
																						   @"apple",
																						   nil]];
				accountCreationSipURI = nil;
				accountCreationPassword = nil;
				//simply renewing
			} else {
				[request setMethod: @"get_expiration_date" withParameters:[NSArray arrayWithObjects:
																		   @"",
																		   receiptBase64,
																		   @"",
																		   @"apple",
																		   nil]];
			}
		} else {
			LOGE(@"Hum, not handling product with ID %@", transaction.payment.productIdentifier);
			return;
		}

		latestReceiptMD5 = [[receiptBase64 md5] retain];

		XMLRPCConnectionManager *manager = [XMLRPCConnectionManager sharedManager];
		[manager spawnConnectionWithXMLRPCRequest: request delegate: self.xmlrpc];
		LOGI(@"XMLRPC query %@: %@", [request method], [request body]);
		[request release];
	} else {
		LOGW(@"Not checking receipt since it has already been done!");
	}
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {
	for(SKPaymentTransaction * transaction in transactions) {
		switch (transaction.transactionState) {
			case SKPaymentTransactionStatePurchasing:
				break;
			case SKPaymentTransactionStatePurchased:
			case SKPaymentTransactionStateRestored: {
				[self validateReceipt: transaction];
				[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
				break;
			}
			case SKPaymentTransactionStateDeferred:
				//waiting for parent approval
				break;
			case SKPaymentTransactionStateFailed: {
				NSString* errlast = [NSString stringWithFormat:@"Purchase of %@ failed: %@.",transaction.payment.productIdentifier,transaction.error.localizedDescription];
				LOGE(@"SKPaymentTransactionStateFailed: %@", errlast);
				NSDictionary* dict = @{@"product_id": transaction.payment.productIdentifier, @"error_msg": errlast};
				[self postNotificationforStatus:IAPPurchaseFailed withDict:dict];
				[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
				break;
			}
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
		NSDictionary* dict = @{@"error_msg": [error localizedDescription]};
		[self postNotificationforStatus:IAPRestoreFailed withDict:dict];
	}
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue {
	LOGI(@"All restorable transactions have been processed by the payment queue.");
}

-(void)postNotificationforStatus:(IAPPurchaseNotificationStatus)status withDict:(NSDictionary*)dict {
	_status = status;
	[[NSNotificationCenter defaultCenter] postNotificationName:status object:self userInfo:dict];
	LOGI(@"Triggering notification for status %@", status);
}

- (void)retrievePurchases {
	[self validateReceipt:nil];
}

- (void)XMLRPCRequest:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response {
	LOGI(@"XMLRPC response %@: %@", [request method], [response body]);
	NSString* productID = [[LinphoneManager instance] lpConfigStringForKey:@"paid_account_id" forSection:@"in_app_purchase"];

	// validation succeeded
	if(! [response isFault] && [response object] != nil) {
		if([[request method] isEqualToString:@"get_expiration_date"]) {
			//first remove it from list
			[_productsIDPurchased removeObject:productID];

			double expirationTime = [[response object] doubleValue] / 1000;
			NSDate * expirationDate = [NSDate dateWithTimeIntervalSince1970:expirationTime];
			NSDate *now = [[NSDate alloc] init];
			if (([expirationDate earlierDate:now] == expirationDate) || (expirationTime < 1)) {
				LOGI(@"Account has expired");
				[[PhoneMainView instance] changeCurrentView:[InAppProductsViewController compositeViewDescription]];
				expirationDate = [NSDate dateWithTimeIntervalSince1970:0];
			} else {
				[_productsIDPurchased addObject:productID];
			}
			NSDictionary* dict = @{@"product_id": productID, @"expires_date": expirationDate};
			[self postNotificationforStatus:IAPReceiptSucceeded withDict:dict];
		} else 	if([[request method] isEqualToString:@"create_account_from_in_app_purchase"]) {
			[_productsIDPurchased removeObject:productID];

			double timeinterval = [[response object] doubleValue] / 1000;
			if (timeinterval != -2) {
				NSDate *expirationDate = [NSDate dateWithTimeIntervalSince1970:timeinterval];
				NSDate *now = [[NSDate alloc] init];
				if ([expirationDate earlierDate:now] == expirationDate) {
					LOGI(@"Account has expired");
					[[PhoneMainView instance] changeCurrentView:[InAppProductsViewController compositeViewDescription]];
					expirationDate = [NSDate dateWithTimeIntervalSince1970:0];
				} else {
					[_productsIDPurchased addObject:productID];
				}
				NSDictionary* dict = @{@"product_id": productID, @"expires_date": expirationDate};
				[self postNotificationforStatus:IAPPurchaseSucceeded withDict:dict];
			} else {
				NSDictionary* dict = @{@"product_id": productID, @"error_msg": @"Unknown error"};
				[self postNotificationforStatus:IAPPurchaseFailed withDict:dict];
			}
		}
	} else {
		NSString *errorString = NSLocalizedString(@"Unknown error", nil);
		if ([response isFault]) {
			errorString = [NSString stringWithFormat:NSLocalizedString(@"Communication issue (%@)", nil), [response faultString]];
		} else if([response object] == nil) {
			errorString = NSLocalizedString(@"Invalid server response", nil);
		}
		LOGE(@"Communication issue (%@)", [response faultString]);
		UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Communication issue",nil)
															message:errorString
														   delegate:nil
												  cancelButtonTitle:NSLocalizedString(@"Continue",nil)
												  otherButtonTitles:nil,nil];
		[errorView show];
		[errorView release];

		latestReceiptMD5 = nil;
		NSDictionary* dict = @{@"error_msg": errorString};
		[self postNotificationforStatus:IAPReceiptFailed withDict:dict];
	}
}

- (void)XMLRPCRequest:(XMLRPCRequest *)request didFailWithError:(NSError *)error {
	LOGE(@"Communication issue (%@)", [error localizedDescription]);
	NSString *errorString = [NSString stringWithFormat:NSLocalizedString(@"Communication issue (%@)", nil), [error localizedDescription]];
	UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Communication issue", nil)
														message:errorString
													   delegate:nil
											  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
											  otherButtonTitles:nil,nil];
	[errorView show];
	[errorView release];
	latestReceiptMD5 = nil;
	NSDictionary* dict = @{@"error_msg": errorString};
	[self postNotificationforStatus:IAPReceiptFailed withDict:dict];
}
#else
- (void)purchaseAccount:(NSString *)sipURI withPassword:(NSString *)password { LOGE(@"Not supported"); }
- (void)purchaseWithID:(NSString *)productId { LOGE(@"Not supported"); }
- (void)restore { LOGE(@"Not supported"); }
- (void)XMLRPCRequest:(XMLRPCRequest *)request didFailWithError:(NSError *)error { LOGE(@"Not supported"); }
- (void)XMLRPCRequest:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response { LOGE(@"Not supported"); }
- (BOOL)isPurchasedWithID:(NSString *)productId { LOGE(@"Not supported"); return FALSE; }
- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions { LOGE(@"Not supported"); }
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response { LOGE(@"Not supported"); }
- (void)retrievePurchases { LOGE(@"Not supported"); }
#endif
@end
