/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import <UIKit/UIKit.h>

#import "UICompositeView.h"
#import "ContactsListTableView.h"
#import "UIInterfaceStyleButton.h"

typedef enum _ContactSelectionMode { ContactSelectionModeNone, ContactSelectionModeEdit } ContactSelectionMode;

@interface ContactSelection : NSObject <UISearchBarDelegate> {
}

+ (void)setSelectionMode:(ContactSelectionMode)selectionMode;
+ (ContactSelectionMode)getSelectionMode;
+ (void)setAddAddress:(NSString *)address;
+ (NSString *)getAddAddress;
/*!
 * Filters contacts by SIP domain.
 * @param enabled  Wether SIP domain filter is enabled
 */
+ (void)enableSipFilter:(BOOL)enabled;

/*!
 * Wether SIP domain filter is enabled
 * @return the filter used, or nil if none.
 */
+ (BOOL)getSipFilterEnabled;

@end

@interface ContactsListView : UIViewController <UICompositeViewDelegate, UISearchBarDelegate, UIGestureRecognizerDelegate>

@property(strong, nonatomic) IBOutlet ContactsListTableView *tableController;
@property(strong, nonatomic) IBOutlet UIView *topBar;
@property(nonatomic, strong) IBOutlet UIButton *allButton;
@property(nonatomic, strong) IBOutlet UIButton *linphoneButton;
@property(nonatomic, strong) IBOutlet UIButton *addButton;
@property(strong, nonatomic) IBOutlet UISearchBar *searchBar;
@property(weak, nonatomic) IBOutlet UIImageView *selectedButtonImage;
@property (weak, nonatomic) IBOutlet UIInterfaceStyleButton *toggleSelectionButton;
@property (weak, nonatomic) IBOutlet UILabel *loadingLabel;
@property (weak, nonatomic) IBOutlet UIView *loadingView;
@property (weak, nonatomic) IBOutlet UILabel *ldapMoreResultsLabel;

- (IBAction)onAllClick:(id)event;
- (IBAction)onLinphoneClick:(id)event;
- (IBAction)onAddContactClick:(id)event;
- (IBAction)onDeleteClick:(id)sender;
- (IBAction)onEditionChangeClick:(id)sender;

@end
