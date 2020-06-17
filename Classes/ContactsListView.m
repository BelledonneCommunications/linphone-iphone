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

#import "PhoneMainView.h"

@implementation ContactSelection

static ContactSelectionMode sSelectionMode = ContactSelectionModeNone;
static NSString *sAddAddress = nil;
static NSString *sSipFilter = nil;
static BOOL sEnableEmailFilter = FALSE;
static NSString *sNameOrEmailFilter;
static BOOL addAddressFromOthers = FALSE;

+ (void)setSelectionMode:(ContactSelectionMode)selectionMode {
	sSelectionMode = selectionMode;
}

+ (ContactSelectionMode)getSelectionMode {
	return sSelectionMode;
}

+ (void)setAddAddress:(NSString *)address {
	sAddAddress = address;
	addAddressFromOthers = true;
}

+ (NSString *)getAddAddress {
	return sAddAddress;
}

+ (void)setSipFilter:(NSString *)domain {
	sSipFilter = domain;
}

+ (NSString *)getSipFilter {
	return sSipFilter;
}

+ (void)enableEmailFilter:(BOOL)enable {
	sEnableEmailFilter = enable;
}

+ (BOOL)emailFilterEnabled {
	return sEnableEmailFilter;
}

+ (void)setNameOrEmailFilter:(NSString *)fuzzyName {
	sNameOrEmailFilter = fuzzyName;
}

+ (NSString *)getNameOrEmailFilter {
	return sNameOrEmailFilter;
}

@end

@implementation ContactsListView

@synthesize tableController;
@synthesize allButton;
@synthesize linphoneButton;
@synthesize addButton;
@synthesize topBar;

typedef enum { ContactsAll, ContactsLinphone, ContactsMAX } ContactsCategory;

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:TabBarView.class
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:YES
														   fragmentWith:ContactDetailsView.class];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];
	tableController.tableView.accessibilityIdentifier = @"Contacts table";
	[self changeView:ContactsAll];
	/*if ([tableController totalNumberOfItems] == 0) {
		[self changeView:ContactsAll];
	 }*/
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
								   initWithTarget:self
								   action:@selector(dismissKeyboards)];
	
	[tap setDelegate:self];
	[self.view addGestureRecognizer:tap];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[ContactSelection setNameOrEmailFilter:@""];
	_searchBar.showsCancelButton = (_searchBar.text.length > 0);

	int y = _searchBar.frame.origin.y + _searchBar.frame.size.height;
	[tableController.tableView setFrame:CGRectMake(tableController.tableView.frame.origin.x,
												   y,
												   tableController.tableView.frame.size.width,
												   tableController.tableView.frame.size.height)];
	[tableController.emptyView setFrame:CGRectMake(tableController.emptyView.frame.origin.x,
												   y,
												   tableController.emptyView.frame.size.width,
												   tableController.emptyView.frame.size.height)];

	if (tableController.isEditing) {
		tableController.editing = NO;
	}
	[self refreshButtons];
	[_toggleSelectionButton setImage:[UIImage imageNamed:@"select_all_default.png"] forState:UIControlStateSelected];
	if ([LinphoneManager.instance lpConfigBoolForKey:@"hide_linphone_contacts" inSection:@"app"]) {
		self.linphoneButton.hidden = TRUE;
		self.selectedButtonImage.hidden = TRUE;
	}
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	if (![FastAddressBook isAuthorized]) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Address book", nil)
																		 message:NSLocalizedString(@"You must authorize the application to have access to address book.\n"
																								   "Toggle the application in Settings > Privacy > Contacts",
																								   nil)
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Continue", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
		[PhoneMainView.instance popCurrentView];
	}
	
	// show message toast when add contact from address
	if ([ContactSelection getAddAddress] != nil && addAddressFromOthers) {
		UIAlertController *infoView = [UIAlertController
									   alertControllerWithTitle:NSLocalizedString(@"Info", nil)
									   message:NSLocalizedString(@"Select a contact or create a new one.",nil)
									   preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK"
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction *action){
															  }];
		
		[infoView addAction:defaultAction];
		addAddressFromOthers = FALSE;
		[PhoneMainView.instance presentViewController:infoView animated:YES completion:nil];
	}
}

- (void) viewWillDisappear:(BOOL)animated {
	self.view = NULL;
	[self.tableController removeAllContacts];
}

#pragma mark -

- (void)changeView:(ContactsCategory)view {
	CGRect frame = _selectedButtonImage.frame;
	if (view == ContactsAll && !allButton.selected) {
		//REQUIRED TO RELOAD WITH FILTER
		[LinphoneManager.instance setContactsUpdated:TRUE];
		frame.origin.x = allButton.frame.origin.x;
		[ContactSelection setSipFilter:nil];
		[ContactSelection enableEmailFilter:FALSE];
		allButton.selected = TRUE;
		linphoneButton.selected = FALSE;
		[tableController loadData];
	} else if (view == ContactsLinphone && !linphoneButton.selected) {
		//REQUIRED TO RELOAD WITH FILTER
		[LinphoneManager.instance setContactsUpdated:TRUE];
		frame.origin.x = linphoneButton.frame.origin.x;
		[ContactSelection setSipFilter:LinphoneManager.instance.contactFilter];
		[ContactSelection enableEmailFilter:FALSE];
		linphoneButton.selected = TRUE;
		allButton.selected = FALSE;
		[tableController loadData];
	}
	_selectedButtonImage.frame = frame;
	if ([LinphoneManager.instance lpConfigBoolForKey:@"hide_linphone_contacts" inSection:@"app"]) {
		allButton.selected = FALSE;
	}
}

- (void)refreshButtons {
	[addButton setHidden:FALSE];
	[self changeView:[ContactSelection getSipFilter] ? ContactsLinphone : ContactsAll];
}

#pragma mark - Action Functions

- (IBAction)onAllClick:(id)event {
	[self changeView:ContactsAll];
}

- (IBAction)onLinphoneClick:(id)event {
	[self changeView:ContactsLinphone];
}

- (IBAction)onAddContactClick:(id)event {
	ContactDetailsView *view = VIEW(ContactDetailsView);
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	view.isAdding = TRUE;
	if ([ContactSelection getAddAddress] == nil) {
		[view newContact];
	} else {
		[view newContact:[ContactSelection getAddAddress]];
	}
}

- (IBAction)onDeleteClick:(id)sender {
	NSString *msg = [NSString stringWithFormat:NSLocalizedString(@"Do you want to delete selected contacts?\nThey will also be deleted from your phone's address book.", nil)];
	[LinphoneManager.instance setContactsUpdated:TRUE];
	[UIConfirmationDialog ShowWithMessage:msg
		cancelMessage:nil
		confirmMessage:nil
		onCancelClick:^() {
		  [self onEditionChangeClick:nil];
		}
		onConfirmationClick:^() {
		  [tableController removeSelectionUsing:nil];
		  [tableController loadData];
		  [self onEditionChangeClick:nil];
		}];
}

- (IBAction)onEditionChangeClick:(id)sender {
	allButton.hidden = linphoneButton.hidden = _selectedButtonImage.hidden = addButton.hidden =	self.tableController.isEditing;
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
	searchBar.text = @"";
	[self searchBar:searchBar textDidChange:@""];
	[LinphoneManager.instance setContactsUpdated:TRUE];
	[tableController loadData];
	[searchBar resignFirstResponder];
}

- (void)dismissKeyboards {
	if ([self.searchBar isFirstResponder]){
		[self.searchBar resignFirstResponder];
	}
}

#pragma mark - searchBar delegate

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
	// display searchtext in UPPERCASE
	// searchBar.text = [searchText uppercaseString];
	[ContactSelection setNameOrEmailFilter:searchText];
	if (searchText.length == 0) {
		[LinphoneManager.instance setContactsUpdated:TRUE];
		[tableController loadData];
	} else {
		[tableController loadSearchedData];
	}
}

- (void)searchBarTextDidEndEditing:(UISearchBar *)searchBar {
	[searchBar setShowsCancelButton:FALSE animated:TRUE];
}

- (void)searchBarTextDidBeginEditing:(UISearchBar *)searchBar {
	[searchBar setShowsCancelButton:TRUE animated:TRUE];
}

- (void)searchBarSearchButtonClicked:(UISearchBar *)searchBar {
	[searchBar resignFirstResponder];
}

#pragma mark - GestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
	return NO;
}

@end
