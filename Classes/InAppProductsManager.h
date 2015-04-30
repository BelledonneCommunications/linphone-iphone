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
#import <XMLRPCConnectionDelegate.h>

@interface InAppProductsXMLRPCDelegate : NSObject <XMLRPCConnectionDelegate>

@end

#define IAPNotReadyYet			@"IAPNotReadyYet" // startup status, manager is not ready yet
#define IAPAvailableSucceeded	@"IAPAvailableSucceeded" //no data
#define IAPAvailableFailed		@"IAPAvailableFailed" //data: error_msg
#define IAPPurchaseTrying		@"IAPPurchaseTrying" //data: product_id
#define IAPPurchaseFailed		@"IAPPurchaseFailed" //data: product_id, error_msg
#define IAPPurchaseSucceeded	@"IAPPurchaseSucceeded" //data: product_id, expires_date
#define IAPRestoreFailed		@"IAPRestoreFailed" //data: error_msg
#define IAPRestoreSucceeded		@"IAPRestoreSucceeded" //no data
#define IAPReceiptFailed		@"IAPReceiptFailed" //data: error_msg
#define IAPReceiptSucceeded		@"IAPReceiptSucceeded" //no data
typedef NSString*               IAPPurchaseNotificationStatus;

// InAppProductsManager take care of any in app purchase accessible within Linphone
// In order to use it, you must configure your linphonerc configuration correctly, such as:
//[in_app_purchase]
//enabled=1
//paid_account_id=test.autorenew_7days
//receipt_validation_url=https://www.linphone.org/inapp.php
//products_list=test.autorenew_7days
// Note: in Sandbox mode (test), autorenewal expire time is speed up (see http://stackoverflow.com/questions/8815271/what-expiry-date-should-i-see-for-in-app-purchase-in-the-application-sandbox) so that 7 days renewal is only 3 minutes!

@interface InAppProductsManager : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver> {
	NSString *latestReceiptMD5;
}

// needed because request:didFailWithError method is already used by SKProductsRequestDelegate...
@property (nonatomic, retain) InAppProductsXMLRPCDelegate *xmlrpc;
@property (nonatomic, retain) IAPPurchaseNotificationStatus status;
@property (nonatomic, strong) NSMutableArray *productsAvailable;
@property (nonatomic, strong) NSMutableArray *productsIDPurchased;

@property (readonly) BOOL enabled;

- (BOOL)isPurchasedWithID:(NSString*)productId;
- (void)purchaseAccount:(NSString*)sipURI withPassword:(NSString*)password;
- (BOOL)purchaseWitID:(NSString *)productID;

// restore user purchases. Must be at first launch or a user action ONLY.
- (void)restore;
- (void)retrievePurchases;
// internal API only due to methods conflict
- (void)XMLRPCRequest:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response;
// internal API only due to methods conflict
- (void)XMLRPCRequest:(XMLRPCRequest *)request didFailWithError:(NSError *)error;

@end
