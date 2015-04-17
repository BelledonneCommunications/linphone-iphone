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

extern NSString *const kLinphoneIAPurchaseNotification;

@interface InAppProductsManager : NSObject <SKProductsRequestDelegate, SKPaymentTransactionObserver>

#define IAPAvailableSucceeded	@"IAPAvailableSucceeded"
#define IAPAvailableFailed		@"IAPAvailableFailed"
#define IAPPurchaseFailed		@"IAPPurchaseFailed"
#define IAPPurchaseSucceeded	@"IAPPurchaseSucceeded"
#define IAPRestoreFailed		@"IAPRestoreFailed"
#define IAPRestoreSucceeded		@"IAPRestoreSucceeded"
#define IAPReceiptFailed		@"IAPReceiptFailed"
#define IAPReceiptSucceeded		@"IAPReceiptSucceeded"

typedef NSString*               IAPPurchaseNotificationStatus;

@property (nonatomic, retain) IAPPurchaseNotificationStatus status;
@property (nonatomic, copy) NSString *errlast;

@property (nonatomic, strong) NSMutableArray *productsAvailable;
@property (nonatomic, strong) NSMutableArray *productsIDPurchased;

- (void)loadProducts;
- (BOOL)isPurchasedWithID:(NSString*)productId;
- (void)purchaseWithID:(NSString*)productId;
- (void)restore;

@end
