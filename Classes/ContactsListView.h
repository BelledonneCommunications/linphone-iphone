/* ContactsViewController.h
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

#import <UIKit/UIKit.h>
#import <AddressBookUI/ABPeoplePickerNavigationController.h>

#import "UICompositeView.h"
#import "ContactsListTableView.h"
#import "UIIconButton.h"

typedef enum _ContactSelectionMode { ContactSelectionModeNone, ContactSelectionModeEdit } ContactSelectionMode;

@interface ContactSelection : NSObject <UISearchBarDelegate> {
}

+ (void)setSelectionMode:(ContactSelectionMode)selectionMode;
+ (ContactSelectionMode)getSelectionMode;
+ (void)setAddAddress:(NSString *)address;
+ (NSString *)getAddAddress;
/*!
 * Filters contacts by SIP domain.
 * @param domain SIP domain to filter. Use @"*" or nil to disable it.
 */
+ (void)setSipFilter:(NSString *)domain;

/*!
 * Weither contacts are filtered by SIP domain or not.
 * @return the filter used, or nil if none.
 */
+ (NSString *)getSipFilter;

/*!
 * Weither always keep contacts with an email address or not.
 * @param enable TRUE if you want to always keep contacts with an email.
 */
+ (void)enableEmailFilter:(BOOL)enable;

/*!
 * Weither always keep contacts with an email address or not.
 * @return TRUE if this behaviour is enabled.
 */
+ (BOOL)emailFilterEnabled;

/*!
 * Filters contacts by name and/or email fuzzy matching pattern.
 * @param fuzzyName fuzzy word to match. Use nil to disable it.
 */
+ (void)setNameOrEmailFilter:(NSString *)fuzzyName;

/*!
 * Weither contacts are filtered by name and/or email.
 * @return the filter used, or nil if none.
 */
+ (NSString *)getNameOrEmailFilter;

@end

@interface ContactsListView : UIViewController <UICompositeViewDelegate, UISearchBarDelegate, UIGestureRecognizerDelegate>

@property(strong, nonatomic) IBOutlet ContactsListTableView *tableController;
@property(strong, nonatomic) IBOutlet UIView *topBar;
@property(nonatomic, strong) IBOutlet UIButton *allButton;
@property(nonatomic, strong) IBOutlet UIButton *linphoneButton;
@property(nonatomic, strong) IBOutlet UIButton *addButton;
@property(strong, nonatomic) IBOutlet UISearchBar *searchBar;
@property(weak, nonatomic) IBOutlet UIIconButton *deleteButton;
@property(weak, nonatomic) IBOutlet UIImageView *selectedButtonImage;

- (IBAction)onAllClick:(id)event;
- (IBAction)onLinphoneClick:(id)event;
- (IBAction)onAddContactClick:(id)event;
- (IBAction)onDeleteClick:(id)sender;
- (IBAction)onEditionChangeClick:(id)sender;

@end
