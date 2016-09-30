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

#import <Foundation/Foundation.h>
#import <StoreKit/StoreKit.h>

#define kIAPNotReady @"IAPNotReady"					  // startup status, manager is not ready yet
#define kIAPReady @"IAPReady"						  // no data
#define kIAPPurchaseTrying @"IAPPurchaseTrying"		  // data: product_id
#define kIAPPurchaseCancelled @"IAPPurchaseCancelled" // data: product_id
#define kIAPPurchaseFailed @"IAPPurchaseFailed"		  // data: product_id, error_msg
#define kIAPPurchaseSucceeded @"IAPPurchaseSucceeded" // data: product_id, expires_date
#define kIAPPurchaseExpired @"IAPPurchaseExpired"	 // data: product_id, expires_date
#define kIAPRestoreFailed @"IAPRestoreFailed"		  // data: error_msg
#define kIAPRestoreSucceeded @"IAPRestoreSucceeded"   // no data
#define kIAPReceiptFailed @"IAPReceiptFailed"		  // data: error_msg
#define kIAPReceiptSucceeded @"IAPReceiptSucceeded"   // no data
typedef NSString *IAPPurchaseNotificationStatus;

// InAppProductsManager take care of any in app purchase accessible within Linphone
// In order to use it, you must configure your linphonerc configuration correctly, such as:
//[in_app_purchase]
// enabled=1
// paid_account_id=test.autorenew_7days
// receipt_validation_url=https://www.linphone.org/inapp.php
// products_list=test.autorenew_7days
// expiry_check_period = 86400
// warn_before_expiry_period = 604800
// Note: in Sandbox mode (test), autorenewal expire time is speed up (see
// http://stackoverflow.com/questions/8815271/what-expiry-date-should-i-see-for-in-app-purchase-in-the-application-sandbox)
// so that 7 days renewal is only 3 minutes and:
// 1 week = 3 minutes
// 1 month = 5 minutes
// 2 months = 10 minutes
// 3 months = 15 minutes
// 6 months = 30 minutes
// 1 year = 1 hour

@interface InAppProductsManager : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver> {
	NSString *latestReceiptMD5;
	time_t lastCheck;
	time_t expiryTime;
}

@property(nonatomic, strong) IAPPurchaseNotificationStatus status;
@property(nonatomic, strong) NSMutableArray *productsAvailable;
@property(nonatomic, strong) NSMutableArray *productsIDPurchased;
//Period of time between each expiration check. Default value is given in linphonerc.
@property time_t checkPeriod;
//Period of time before expiration during which we warn the user about the need to renew the account.
@property time_t warnBeforeExpiryPeriod;
//The notification category to use for displaying notification related to account expiry.
@property NSString *notificationCategory;

// TRUE when in app purchase capability is available - not modified during runtime
@property(readonly) BOOL enabled;
// TRUE when manager is correctly set up - must first retrieve products available and validate current receipt on our
// server
@property(readonly) BOOL initialized;
// TRUE if manager is available for usage - will be FALSE if an operation is already in progress or if not initialized
// or not enabled
@property(readonly) BOOL available;

// TRUE if accountActivate was started but we did not receive response from server yet
@property(readonly) BOOL accountActivationInProgress;

// TRUE if accountActivate activated
@property(readonly) BOOL accountActivated;

- (BOOL)isPurchasedWithID:(NSString *)productId;
// Purchase an account. You should not use this if manager is not available yet.
/*- (BOOL)purchaseAccount:(NSString *)phoneNumber
		   withPassword:(NSString *)password
			   andEmail:(NSString *)email
				monthly:(BOOL)monthly;
*/
// Purchase a product. You should not use this if manager is not available yet.
- (BOOL)purchaseWithID:(NSString *)productID;
// Activate purchased account.
//- (BOOL)activateAccount:(NSString *)phoneNumber;
// Check if account is activated.
//- (BOOL)checkAccountActivated:(NSString *)phoneNumber;

// restore user purchases. You should not use this if manager is not available yet. Must be at a user action ONLY.
- (BOOL)restore;
// retrieve purchases on our server. You should not use this if manager is not available yet.
// Warning: on first run, this will open a popup to user to provide iTunes Store credentials
- (BOOL)retrievePurchases;

//Check if account is about to expire, and if yes launch a notification.
- (void)check;

// deal with xmlrpc response
//- (void)dealWithXmlRpcResponse:(LinphoneXmlRpcRequest *)request;

@end
