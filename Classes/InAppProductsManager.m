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

#import "LinphoneManager.h"

#import "PhoneMainView.h"

@interface InAppProductsManager ()
@property(strong, nonatomic) NSDate *expirationDate;
@property(strong, nonatomic) NSDictionary *accountCreationData;
// needed because request:didFailWithError method is already used by SKProductsRequestDelegate...
@property(nonatomic, strong) InAppProductsXMLRPCDelegate *xmlrpc;
@end

@implementation InAppProductsManager

// LINPHONE_CAPABILITY_INAPP_PURCHASE must be defined in Linphone Build Settings
#if LINPHONE_CAPABILITY_INAPP_PURCHASE && !TARGET_IPHONE_SIMULATOR

- (instancetype)init {
	if ((self = [super init]) != nil) {
		_enabled = (([SKPaymentQueue canMakePayments]) &&
					([LinphoneManager.instance lpConfigBoolForKey:@"enabled" inSection:@"in_app_purchase"]));
		_initialized = false;
		_available = false;
		_accountActivationInProgress = false;
		if (_enabled) {
			self.xmlrpc = [[InAppProductsXMLRPCDelegate alloc] init];
			_status = kIAPNotReady;
			[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
			[self loadProducts];
		}
	}
	return self;
}

#pragma mark Public API

- (BOOL)isPurchasedWithID:(NSString *)productID {
	if (!_enabled)
		return FALSE;

	for (NSString *prod in _productsIDPurchased) {
		NSDate *now = [[NSDate alloc] init];
		// since multiple ID represent the same product, we must not check it
		if (/*[prod isEqual: productID] &&*/ [self.expirationDate earlierDate:now] == now) {
			bool isBought = true;
			LOGE(@"%@ is %s bought.", prod, isBought ? "" : "NOT");
			return isBought;
		}
	}
	return false;
}

- (BOOL)purchaseWitID:(NSString *)productID {
	if (!_enabled || !_initialized || !_available) {
		NSDictionary *dict = @{
			@"product_id" : productID,
			@"error_msg" : NSLocalizedString(@"Cannot purchase anything yet, please try again later.", nil)
		};
		[self postNotificationforStatus:kIAPPurchaseFailed withDict:dict];
		return FALSE;
	}
	SKProduct *prod = [self productIDAvailable:productID];
	if (prod) {
		NSDictionary *dict = @{ @"product_id" : productID };
		[self postNotificationforStatus:kIAPPurchaseTrying withDict:dict];
		SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:prod];
		_available = false;
		[[SKPaymentQueue defaultQueue] addPayment:payment];
		[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
		return TRUE;
	} else {
		NSDictionary *dict = @{ @"product_id" : productID, @"error_msg" : @"Product not available" };
		[self postNotificationforStatus:kIAPPurchaseFailed withDict:dict];
		return FALSE;
	}
}

- (BOOL)purchaseAccount:(NSString *)phoneNumber
		   withPassword:(NSString *)password
			   andEmail:(NSString *)email
				monthly:(BOOL)monthly {
	if (phoneNumber) {
		NSString *productID =
			[LinphoneManager.instance lpConfigStringForKey:(monthly ? @"paid_account_id_monthly" : @"paid_account_id")
												 inSection:@"in_app_purchase"];
		self.accountCreationData = @{ @"phoneNumber" : phoneNumber, @"password" : password, @"email" : email };

		if (![self purchaseWitID:productID]) {
			self.accountCreationData = nil;
		}
		return true;
	}
	return false;
}

- (BOOL)activateAccount:(NSString *)phoneNumber {
	if (phoneNumber) {
		NSString *receiptBase64 = [self getReceipt];
		if (receiptBase64) {
			NSURL *URL = [NSURL URLWithString:[LinphoneManager.instance lpConfigStringForKey:@"receipt_validation_url"
																				   inSection:@"in_app_purchase"]];
			XMLRPCRequest *request = [[XMLRPCRequest alloc] initWithURL:URL];
			// buying for the first time: need to create the account
			// if ([transaction.transactionIdentifier
			// isEqualToString:transaction.originalTransaction.transactionIdentifier]) {
			[request setMethod:@"activate_account"
				withParameters:[NSArray arrayWithObjects:@"", phoneNumber, receiptBase64, @"", @"apple", nil]];
			_accountActivationInProgress = YES;
			XMLRPCConnectionManager *manager = [XMLRPCConnectionManager sharedManager];
			[manager spawnConnectionWithXMLRPCRequest:request delegate:self.xmlrpc];
			LOGI(@"XMLRPC query %@", [request method]);
			return true;
		} else {
			LOGE(@"Trying to activate account but no receipt available yet (probably doing it too soon)");
		}
	}
	return false;
}

- (BOOL)restore {
	if (!_enabled || !_initialized || !_available) {
		NSDictionary *dict = @{ @"error_msg" : NSLocalizedString(@"In apps not ready yet", nil) };
		[self postNotificationforStatus:kIAPRestoreFailed withDict:dict];
		return FALSE;
	}
	LOGI(@"Restoring user purchases...");
	// force new query of our server
	latestReceiptMD5 = nil;
	_available = false;
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
	return TRUE;
}

- (BOOL)retrievePurchases {
	if (!_enabled | !_initialized | !_available) {
		NSDictionary *dict = @{ @"error_msg" : NSLocalizedString(@"In apps not ready yet", nil) };
		[self postNotificationforStatus:kIAPRestoreFailed withDict:dict];
		return FALSE;
	} else if ([[self getPhoneNumber] length] == 0) {
		LOGW(@"Not retrieving purchase since not account configured yet");
		return FALSE;
	} else {
		_available = false;
		[self validateReceipt:nil];
		return TRUE;
	}
}

#pragma mark ProductListLoading

- (void)loadProducts {
	NSArray *list = [[[LinphoneManager.instance lpConfigStringForKey:@"products_list" inSection:@"in_app_purchase"]
		stringByReplacingOccurrencesOfString:@" "
								  withString:@""] componentsSeparatedByString:@","];

	_productsIDPurchased = [[NSMutableArray alloc] initWithCapacity:0];

	SKProductsRequest *productsRequest =
		[[SKProductsRequest alloc] initWithProductIdentifiers:[NSSet setWithArray:list]];
	productsRequest.delegate = self;
	[productsRequest start];
}

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response {
	_productsAvailable = [NSMutableArray arrayWithArray:response.products];

	LOGI(@"Found %lu products available", (unsigned long)_productsAvailable.count);
	_initialized = true;
	if (response.invalidProductIdentifiers.count > 0) {
		for (NSString *invalidIdentifier in response.invalidProductIdentifiers) {
			LOGW(@"Found product Identifier with invalid ID '%@'", invalidIdentifier);
		}
	} else {
		_available = true;
		[self postNotificationforStatus:kIAPReady withDict:nil];
	}
}

- (void)request:(SKRequest *)request didFailWithError:(NSError *)error {
	LOGE(@"Impossible to retrieve list of products: %@", [error localizedDescription]);
	// well, let's retry...
	[self loadProducts];
}

#pragma mark Other

- (SKProduct *)productIDAvailable:(NSString *)productID {
	if (!_enabled || !_initialized)
		return nil;
	for (SKProduct *product in _productsAvailable) {
		if ([product.productIdentifier compare:productID options:NSLiteralSearch] == NSOrderedSame) {
			return product;
		}
	}
	return nil;
}

- (void)requestDidFinish:(SKRequest *)request {
	if ([request isKindOfClass:[SKReceiptRefreshRequest class]]) {
		NSURL *receiptUrl = [[NSBundle mainBundle] appStoreReceiptURL];
		if ([[NSFileManager defaultManager] fileExistsAtPath:[receiptUrl path]]) {
			LOGI(@"App Receipt exists");
			[self validateReceipt:nil];
		} else {
			// This can happen if the user cancels the login screen for the store.
			// If we get here it means there is no receipt and an attempt to get it failed because the user cancelled
			// the login.
			LOGF(@"Receipt request done but there is no receipt");
		}
	}
}

- (NSString *)getReceipt {
	NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
	// Test whether the receipt is present at the above URL
	if (![[NSFileManager defaultManager] fileExistsAtPath:[receiptURL path]]) {
		// We are probably in sandbox environment, trying to retrieve it...
		return nil;
	}

	NSString *receiptBase64 = [[NSData dataWithContentsOfURL:receiptURL] base64EncodedStringWithOptions:0];
	LOGI(@"Found appstore receipt %@", [receiptBase64 md5]);
	return receiptBase64;
}

- (void)validateReceipt:(SKPaymentTransaction *)transaction {
	NSString *receiptBase64 = [self getReceipt];
	if (receiptBase64 == nil) {
		SKRequest *req = [[SKReceiptRefreshRequest alloc] init];
		LOGI(@"Receipt not found yet, trying to retrieve it...");
		req.delegate = self;
		[req start];
		return;
	}
	// only check the receipt if it has changed
	if (latestReceiptMD5 == nil || ![latestReceiptMD5 isEqualToString:[receiptBase64 md5]]) {
		// transaction is null when restoring user purchases at application start or if user clicks the "restore" button
		// We must validate the receipt on our server
		NSURL *URL = [NSURL URLWithString:[LinphoneManager.instance lpConfigStringForKey:@"receipt_validation_url"
																			   inSection:@"in_app_purchase"]];
		XMLRPCRequest *request = [[XMLRPCRequest alloc] initWithURL:URL];
		// buying for the first time: need to create the account
		// if ([transaction.transactionIdentifier
		// isEqualToString:transaction.originalTransaction.transactionIdentifier]) {
		if (self.accountCreationData.count == 3) {
			[request setMethod:@"create_account_from_in_app_purchase"
				withParameters:[NSArray arrayWithObjects:@"", [_accountCreationData objectForKey:@"phoneNumber"],
														 receiptBase64, @"", @"apple",
														 [_accountCreationData objectForKey:@"email"], nil]];
			self.accountCreationData = nil;
			// otherwise simply renewing
		} else {
			if ([[self getPhoneNumber] length] > 0) {
				[request setMethod:@"get_expiration_date"
					withParameters:[NSArray
									   arrayWithObjects:[self getPhoneNumber], receiptBase64, @"", @"apple", nil]];
			} else {
				LOGW(@"No SIP URI configured, doing nothing");
				_available = true;
				return;
			}
		}
		latestReceiptMD5 = [receiptBase64 md5];

		XMLRPCConnectionManager *manager = [XMLRPCConnectionManager sharedManager];
		[manager spawnConnectionWithXMLRPCRequest:request delegate:self.xmlrpc];
		LOGI(@"XMLRPC query %@", [request method]);
	} else {
		LOGW(@"Not checking receipt since it has already been done!");
		_available = true;
	}
}

- (NSString *)getPhoneNumber {
	NSString *phoneNumber = @"";
	LinphoneProxyConfig *config = linphone_core_get_default_proxy_config(LC);
	if (config) {
		const char *identity = linphone_proxy_config_get_identity(config);
		if (identity) {
			LinphoneAddress *addr = linphone_core_interpret_url(LC, identity);
			if (addr) {
				phoneNumber = [NSString stringWithUTF8String:linphone_address_get_username(addr)];
				linphone_address_destroy(addr);
			}
		}
	}
	return phoneNumber;
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {
	for (SKPaymentTransaction *transaction in transactions) {
		switch (transaction.transactionState) {
			case SKPaymentTransactionStatePurchasing:
				break;
			case SKPaymentTransactionStatePurchased:
			case SKPaymentTransactionStateRestored: {
				if (!_initialized) {
					LOGW(@"Pending transactions before end of initialization, not verifying receipt");
				} else {
					[self validateReceipt:transaction];
				}
				[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
				break;
			}
			case SKPaymentTransactionStateDeferred:
				LOGI(@"Waiting for parent approval...");
				// could do some UI stuff
				break;
			case SKPaymentTransactionStateFailed: {
				_available = true;
				[[SKPaymentQueue defaultQueue] finishTransaction:transaction];
				if (transaction.error.code == SKErrorPaymentCancelled) {
					LOGI(@"SKPaymentTransactionStateFailed: cancelled");
					NSDictionary *dict = @{ @"product_id" : transaction.payment.productIdentifier };
					[self postNotificationforStatus:kIAPPurchaseCancelled withDict:dict];
				} else {
					NSString *errlast =
						[NSString stringWithFormat:@"Purchase failed: %@.", transaction.error.localizedDescription];
					LOGE(@"SKPaymentTransactionStateFailed: %@", errlast);
					NSDictionary *dict =
						@{ @"product_id" : transaction.payment.productIdentifier,
						   @"error_msg" : errlast };
					[self postNotificationforStatus:kIAPPurchaseFailed withDict:dict];
				}
				break;
			}
		}
	}
}

- (void)paymentQueue:(SKPaymentQueue *)queue removedTransactions:(NSArray *)transactions {
	for (SKPaymentTransaction *transaction in transactions) {
		LOGD(@"%@ was removed from the payment queue.", transaction.payment.productIdentifier);
	}
}

- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error {
	if (error.code != SKErrorPaymentCancelled) {
		NSDictionary *dict = @{ @"error_msg" : [error localizedDescription] };
		[self postNotificationforStatus:kIAPRestoreFailed withDict:dict];
	}
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue {
	LOGI(@"All restorable transactions have been processed by the payment queue.");
}

- (void)postNotificationforStatus:(IAPPurchaseNotificationStatus)status withDict:(NSDictionary *)dict {
	_status = status;
	LOGI(@"Triggering notification for status %@", status);
	[NSNotificationCenter.defaultCenter postNotificationName:status object:self userInfo:dict];
}

- (void)XMLRPCRequest:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response {
	if (!_enabled)
		return;

	_available = true;

	if ([[request method] isEqualToString:@"activate_account"]) {
		_accountActivationInProgress = NO;
	}

	LOGI(@"XMLRPC response %@: %@", [request method], [response body]);
	NSString *productID =
		[LinphoneManager.instance lpConfigStringForKey:@"paid_account_id" inSection:@"in_app_purchase"];

	// validation succeeded
	if (![response isFault] && [response object] != nil) {
		if (([[request method] isEqualToString:@"get_expiration_date"]) ||
			([[request method] isEqualToString:@"create_account_from_in_app_purchase"])) {
			[_productsIDPurchased removeObject:productID];
			// response object can either be expiration date (long long number or an error string)
			double timeinterval = [[response object] doubleValue];
			if (timeinterval != 0.0f) {
				self.expirationDate = [NSDate dateWithTimeIntervalSince1970:timeinterval / 1000];
				NSDate *now = [[NSDate alloc] init];
				NSDictionary *dict = @{ @"product_id" : productID, @"expires_date" : self.expirationDate };
				if ([self.expirationDate earlierDate:now] == self.expirationDate) {
					LOGW(@"Account has expired");
					[self postNotificationforStatus:kIAPPurchaseExpired withDict:dict];
				} else {
					LOGI(@"Account valid until %@", self.expirationDate);
					[_productsIDPurchased addObject:productID];
					[self postNotificationforStatus:kIAPPurchaseSucceeded withDict:dict];
				}
			} else {
				self.expirationDate = nil;
				NSString *error = [response object];
				LOGE(@"Failed with error %@", error);
				NSString *errorMsg;
				if ([error isEqualToString:@"ERROR_ACCOUNT_ALREADY_EXISTS"]) {
					errorMsg = NSLocalizedString(@"This account is already registered.", nil);
				} else if ([error isEqualToString:@"ERROR_UID_ALREADY_IN_USE"]) {
					errorMsg = NSLocalizedString(@"You already own an account.", nil);
				} else if ([error isEqualToString:@"ERROR_ACCOUNT_DOESNT_EXIST"]) {
					errorMsg =
						NSLocalizedString(@"You have already purchased an account but it does not exist anymore.", nil);
				} else if ([error isEqualToString:@"ERROR_PURCHASE_CANCELLED"]) {
					errorMsg = NSLocalizedString(@"You cancelled your account.", nil);
				} else {
					errorMsg = [NSString stringWithFormat:NSLocalizedString(@"Unknown error (%@).", nil), error];
				}
				NSDictionary *dict = @{ @"product_id" : productID, @"error_msg" : errorMsg };
				[self postNotificationforStatus:kIAPPurchaseFailed withDict:dict];
			}
		}
	} else {
		NSString *errorString = NSLocalizedString(@"Unknown error", nil);
		if ([response isFault]) {
			errorString =
				[NSString stringWithFormat:NSLocalizedString(@"Communication issue (%@)", nil), [response faultString]];
		} else if ([response object] == nil) {
			errorString = NSLocalizedString(@"Invalid server response", nil);
		}
		LOGE(@"Communication issue (%@)", [response faultString]);
		UIAlertView *errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Communication issue", nil)
															message:errorString
														   delegate:nil
												  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
												  otherButtonTitles:nil, nil];
		[errorView show];

		latestReceiptMD5 = nil;
		NSDictionary *dict = @{ @"error_msg" : errorString };
		[self postNotificationforStatus:kIAPPurchaseFailed withDict:dict];
	}
}

- (void)XMLRPCRequest:(XMLRPCRequest *)request didFailWithError:(NSError *)error {
	if (!_enabled)
		return;

	_available = true;

	if ([[request method] isEqualToString:@"activate_account"]) {
		_accountActivationInProgress = NO;
	}

	LOGE(@"Communication issue (%@)", [error localizedDescription]);
	NSString *errorString =
		[NSString stringWithFormat:NSLocalizedString(@"Communication issue (%@)", nil), [error localizedDescription]];
	UIAlertView *errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Communication issue", nil)
														message:errorString
													   delegate:nil
											  cancelButtonTitle:NSLocalizedString(@"Continue", nil)
											  otherButtonTitles:nil, nil];
	[errorView show];
	latestReceiptMD5 = nil;
	NSDictionary *dict = @{ @"error_msg" : errorString };
	[self postNotificationforStatus:kIAPReceiptFailed withDict:dict];
}
#else
- (void)postNotificationforStatus:(IAPPurchaseNotificationStatus)status {
	_status = status;
	[NSNotificationCenter.defaultCenter postNotificationName:status object:self userInfo:nil];
	LOGE(@"Not supported, triggering %@", status);
}
- (BOOL)purchaseAccount:(NSString *)phoneNumber
		   withPassword:(NSString *)password
			   andEmail:(NSString *)email
				monthly:(BOOL)monthly {
	[self postNotificationforStatus:kIAPPurchaseFailed];
	return false;
}
- (BOOL)restore {
	[self postNotificationforStatus:kIAPRestoreFailed];
	return false;
}
- (BOOL)retrievePurchases {
	[self postNotificationforStatus:kIAPRestoreFailed];
	return false;
}
- (BOOL)purchaseWitID:(NSString *)productID {
	[self postNotificationforStatus:kIAPPurchaseFailed];
	return FALSE;
}
- (BOOL)isPurchasedWithID:(NSString *)productId {
	return FALSE;
}

- (void)XMLRPCRequest:(XMLRPCRequest *)request didFailWithError:(NSError *)error {
}
- (void)XMLRPCRequest:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response {
}
- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions {
}
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response {
}
- (BOOL)activateAccount:(NSString *)phoneNumber {
	return FALSE;
}
#endif
@end

@implementation InAppProductsXMLRPCDelegate {
	InAppProductsManager *iapm;
}

#pragma mark - XMLRPCConnectionDelegate Functions

- (void)request:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response {
	[[LinphoneManager.instance iapManager] XMLRPCRequest:request didReceiveResponse:response];
}

- (void)request:(XMLRPCRequest *)request didFailWithError:(NSError *)error {
	[[LinphoneManager.instance iapManager] XMLRPCRequest:request didFailWithError:error];
}

- (BOOL)request:(XMLRPCRequest *)request canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
	return FALSE;
}

- (void)request:(XMLRPCRequest *)request didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
}

- (void)request:(XMLRPCRequest *)request didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
}
@end
