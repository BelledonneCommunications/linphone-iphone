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
#import "ShopView.h"

// In app purchase are not supported by the Simulator
#import "XMLRPCHelper.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "StoreKit/StoreKit.h"

@interface InAppProductsManager ()
@property(strong, nonatomic) NSDate *expirationDate;
@property(strong, nonatomic) NSDictionary *accountCreationData;

@end



@implementation InAppProductsManager

@synthesize checkPeriod;
@synthesize warnBeforeExpiryPeriod;
@synthesize notificationCategory;

// LINPHONE_CAPABILITY_INAPP_PURCHASE must be defined in Linphone Build Settings
#if 1

- (instancetype)init {
	if ((self = [super init]) != nil) {
		_enabled = (([SKPaymentQueue canMakePayments]) &&
					([LinphoneManager.instance lpConfigBoolForKey:@"enabled" inSection:@"in_app_purchase"]));
		_initialized = false;
		_available = false;
		_accountActivationInProgress = false;
		checkPeriod = [LinphoneManager.instance lpConfigIntForKey:@"expiry_check_period" inSection:@"in_app_purchase"];
		warnBeforeExpiryPeriod = [LinphoneManager.instance lpConfigIntForKey:@"warn_before_expiry_period" inSection:@"in_app_purchase"];
		lastCheck = 0;

		[XMLRPCHelper.self initArray];
		//========// for test only
		// int testExpiry = [LinphoneManager.instance lpConfigIntForKey:@"expiry_time_test"
		// inSection:@"in_app_purchase"];
		// if (testExpiry > 0){
		//	expiryTime = time(NULL) + testExpiry;
		//}else expiryTime = 0;
		//========//
		if (_enabled) {
			// self.xmlrpc = [[InAppProductsXMLRPCDelegate alloc] init];
			_status = kIAPNotReady;
			[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
			[self loadProducts];
			[self checkAccountExpirationDate];
			[self checkAccountTrial];
			[self checkAccountExpired];
		}
		//[self check];
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

- (BOOL)purchaseWithID:(NSString *)productID {
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
		// Display waitview on click
		UIWindow *window = [[UIApplication sharedApplication] keyWindow];
		UIView *topView = window.rootViewController.view;
		UIView *waitview = (UIView *)[topView viewWithTag:288];
		[waitview setHidden:FALSE];

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

#pragma mark Receipt management

- (NSString *)getReceipt {
	NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
	// Test whether the receipt is present at the above URL
	if (![[NSFileManager defaultManager] fileExistsAtPath:[receiptURL path]]) {
		// We are probably in sandbox environment, trying to retrieve it...
		return nil;
	}

	NSString *receiptBase64 = [[NSData dataWithContentsOfURL:receiptURL] base64EncodedStringWithOptions:0];
	LOGI(@"Found appstore receipt %@", [receiptBase64 md5]);
	[self saveReceiptTemp:receiptBase64];
	return receiptBase64;
}

/**
 Save Receipt temporarily until xmlrpc server request completed and confirmation sent
**/
- (void)saveReceiptTemp:(NSString *)receipt {
	LOGE(@"===>>> saveReceiptTemp : TmpReceipt");
	[LinphoneManager.instance lpConfigSetString:receipt forKey:@"save_tmp_receipt" inSection:@"in_app_purchase"];
}

/**
 reset Receipt to empty after xmlrpc request confirmation received
 **/
- (void)removeTmpReceipt:(NSString *)receipt {
	LOGE(@"===>>> removeReceiptTemp : TmpReceipt");
	if ([LinphoneManager.instance lpConfigStringForKey:@"save_tmp_receipt" inSection:@"in_app_purchase"])
		[LinphoneManager.instance lpConfigSetString:@"0" forKey:@"save_tmp_receipt" inSection:@"in_app_purchase"];
}

/**
 get temp Receipt to retry xmlrpc request
 **/
- (NSString *)getTmpReceipt {
	LOGE(@"===>>> getReceiptTemp : TmpReceipt");
	return [LinphoneManager.instance lpConfigStringForKey:@"save_tmp_receipt" inSection:@"in_app_purchase"];
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
	// Hide waiting view
	UIWindow *window = [[UIApplication sharedApplication] keyWindow];
	UIView *topView = window.rootViewController.view;
	UIView *waitview = (UIView *)[topView viewWithTag:288];
	[waitview setHidden:TRUE];
	// only check the receipt if it has changed
	if (latestReceiptMD5 == nil || ![latestReceiptMD5 isEqualToString:[receiptBase64 md5]]) {
		[self updateAccountExpirationDate:receiptBase64];
		latestReceiptMD5 = [receiptBase64 md5];
		LOGI(@"XMLRPC query ");
	} else {
		LOGW(@"Not checking receipt since it has already been done!");
		_available = true;
	}
}

#pragma mark Getters

- (NSString *)getPhoneNumber {
	NSString *phoneNumber = @"";
	LinphoneProxyConfig *config = linphone_core_get_default_proxy_config(LC);
	if (config) {
		const LinphoneAddress *identity = linphone_proxy_config_get_identity_address(config);
		if (identity) {
			phoneNumber = [NSString stringWithUTF8String:linphone_address_get_username(identity)];
		}
	}
	return phoneNumber;
}

- (NSString *)getPassword {
	NSString *pass;
	LinphoneProxyConfig *cfg = linphone_core_get_default_proxy_config(LC);
	if (cfg && strcmp("sip.linphone.org", linphone_proxy_config_get_domain(cfg)) == 0) {
		const LinphoneAuthInfo *info = linphone_proxy_config_find_auth_info(cfg);
		const char *tmpPass;
		if (linphone_auth_info_get_passwd(info)) {
			tmpPass = linphone_auth_info_get_passwd(info);
		} else {
			tmpPass = linphone_auth_info_get_ha1(info);
		}
		pass = [NSString stringWithFormat:@"%s", tmpPass];
	}
	return pass;
}

#pragma mark Payment management

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
	if ([status isEqual:kIAPPurchaseFailed] || [status isEqual:kIAPPurchaseCancelled]) {
		// Hide waiting view
		UIWindow *window = [[UIApplication sharedApplication] keyWindow];
		UIView *topView = window.rootViewController.view;
		UIView *waitview = (UIView *)[topView viewWithTag:288];
		[waitview setHidden:TRUE];
	}
}

#pragma mark expiration notif

- (void) presentNotification:(int64_t) remaining{
	if (notificationCategory == nil) return;
	int days = (int)remaining / (3600 * 24);
	NSString * expireText;
	if (remaining >= 0){
		expireText = [NSString stringWithFormat:NSLocalizedString(@"Your account will expire in %i days.", nil), days];
	}else{
		expireText = [NSString stringWithFormat:NSLocalizedString(@"Your account has expired.", nil), days];
	}

	if ([UIApplication sharedApplication].applicationState == UIApplicationStateBackground){
		UILocalNotification *notification = [[UILocalNotification alloc] init];
		if (notification) {

			notification.category = notificationCategory;
			notification.repeatInterval = 0;
			notification.applicationIconBadgeNumber = 1;
			notification.alertBody = expireText;

			[[UIApplication sharedApplication] presentLocalNotificationNow:notification];
		}

	}else{
		UIAlertController *alert = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Account expiring", nil)
																	   message:expireText
																preferredStyle:UIAlertControllerStyleAlert];

		UIAlertAction* buyAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Buy", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {
															  [PhoneMainView.instance changeCurrentView:ShopView.compositeViewDescription];
														  }];

		UIAlertAction *laterAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Later", nil)
															  style:UIAlertActionStyleCancel
															handler:^(UIAlertAction *action){
																// [alert dismissViewControllerAnimated:FALSE];
															}];

		[alert addAction:buyAction];
		[alert addAction:laterAction];
		[PhoneMainView.instance presentViewController:alert animated:YES completion:nil];
	}
}

- (void) check{
	if (!_available) return;
	if (expiryTime == 0 || checkPeriod == 0) return;

	time_t now = time(NULL);

	if (now < lastCheck + checkPeriod) return;
	if (now >= expiryTime - warnBeforeExpiryPeriod){
		lastCheck = now;
		int64_t remaining = (int64_t)expiryTime - (int64_t)now;
		[self presentNotification: remaining];
	}

	if (![[self getTmpReceipt] isEqualToString:@""]) {
		LOGE(@"===>>> Check : getTmpReceipt != ''");
		[self updateAccountExpirationDate:[self getReceipt]];
	}
}

#pragma mark Intermediate XMLRPC call method

// Intermediate method to check XMLRPC account expiration date
- (BOOL)updateAccountExpirationDate:(NSString *)receiptBase64 {
	return [self
		callXmlrpcRequestWithParams:@"update_expiration_date"
						  onSuccess:^(NSString *response) {
							if (response) {
								// NSString *productID = [LinphoneManager.instance
								// lpConfigStringForKey:@"paid_account_id" inSection:@"in_app_purchase"];
								LOGI(@"update_expiration_date callback - response: %@", response);
								if ([response containsString:@"ERROR"]) {
									LOGE(@"Failed with error %@", response);
									NSString *errorMsg;
									if ([response isEqualToString:@"ERROR_ACCOUNT_ALREADY_EXISTS"]) {
										errorMsg = NSLocalizedString(@"This account is already registered.", nil);
									} else if ([response isEqualToString:@"ERROR_UID_ALREADY_IN_USE"]) {
										errorMsg = NSLocalizedString(@"You already own an account.", nil);
									} else if ([response isEqualToString:@"ERROR_ACCOUNT_DOESNT_EXIST"]) {
										errorMsg = NSLocalizedString(@"You have already purchased an account "
																	 @"but it does not exist anymore.",
																	 nil);
									} else if ([response isEqualToString:@"ERROR_PURCHASE_CANCELLED"]) {
										errorMsg = NSLocalizedString(@"You cancelled your account.", nil);
									} else {
										errorMsg = [NSString
											stringWithFormat:NSLocalizedString(@"Unknown error (%@).", nil), response];
									}
									// NSDictionary *dict = @{ @"product_id" : productID, @"error_msg" :
									// errorMsg };
									//[self postNotificationforStatus:kIAPPurchaseFailed withDict:dict];

								} else // remove temporarily receipt
									[self removeTmpReceipt:receiptBase64];
							}
						  }
							onError:NULL
							  extra:receiptBase64];
}

// Intermediate method to check XMLRPC account expiration date
- (BOOL)checkAccountExpirationDate {
	return [self callXmlrpcRequestWithParams:@"get_account_expiration"
								   onSuccess:^(NSString *response) {
									 if (response) {
										 LOGI(@"get_account_expiration callback - response: %@", response);
										 if ([response containsString:@"ERROR_NO_EXPIRATION"]) {
											 expiryTime = 0;
										 }
									 }
								   }
									 onError:NULL
									   extra:NULL];
}

// Intermediate method to check XMLRPC account trial
- (BOOL)checkAccountTrial {
	return [self callXmlrpcRequestWithParams:@"is_account_trial"
								   onSuccess:^(NSString *response) {
									 if (response) {
										 LOGI(@"is_account_trial callback - response: %@", response);
									 }
								   }
									 onError:NULL
									   extra:NULL];
}

// Intermediate method to check XMLRPC account expired
- (BOOL)checkAccountExpired {
	return [self callXmlrpcRequestWithParams:@"is_account_expired"
								   onSuccess:^(NSString *response) {
									 if (response) {
										 LOGI(@"is_account_expired callback - response: %@", response);
									 }
								   }
									 onError:NULL
									   extra:NULL];
}

// Intermediate method to check check payload signature
- (BOOL)checkPayloadSignature {
	return [self callXmlrpcRequestWithParams:@"check_payload_signature"
								   onSuccess:^(NSString *response) {
									 if (response) {
										 LOGI(@"check_payload_signature callback - response: %@", response);
									 }
								   }
									 onError:NULL
									   extra:NULL];
}

// Generic function to call sendXMLRPCRequestWithParams
- (BOOL)callXmlrpcRequestWithParams:(NSString *)method
						  onSuccess:(void (^)(NSString *))successBk
							onError:(void (^)(NSString *req))errorBk
							  extra:(NSString *)extra {
	if ([[self getPhoneNumber] length] > 0) {
		NSString *phoneNumber = [self getPhoneNumber];
		NSString *password = [self getPassword];
		NSArray *args;
		if (extra != NULL)
			args = @[ phoneNumber, password, @"", extra ];
		else
			args = @[ phoneNumber, password, @"" ];
		if (successBk && errorBk)
			[XMLRPCHelper.self sendXMLRPCRequestWithParams:method withParams:args onSuccess:successBk onError:errorBk];
		else if (successBk)
			[XMLRPCHelper.self sendXMLRPCRequestWithParams:method withParams:args onSuccess:successBk];
		else
			[XMLRPCHelper.self sendXMLRPCRequestWithParams:method withParams:args];
		return TRUE;

	} else {
		LOGW(@"No SIP URI configured, can't get account expiration date.");
		return FALSE;
	}
}

#pragma mark Other
#else
- (void)postNotificationforStatus:(IAPPurchaseNotificationStatus)status {
	_status = status;
	[NSNotificationCenter.defaultCenter postNotificationName:status object:self userInfo:nil];
	LOGE(@"Not supported, triggering %@", status);
}

#endif
@end
