/* ContactsViewController.m
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

#import "PhoneMainView.h"
#import <AddressBook/ABPerson.h>

@implementation ContactSelection

static ContactSelectionMode sSelectionMode = ContactSelectionModeNone;
static NSString *sAddAddress = nil;
static NSString *sSipFilter = nil;
static BOOL sEnableEmailFilter = FALSE;
static NSString *sNameOrEmailFilter;

+ (void)setSelectionMode:(ContactSelectionMode)selectionMode {
	sSelectionMode = selectionMode;
}

+ (ContactSelectionMode)getSelectionMode {
	return sSelectionMode;
}

+ (void)setAddAddress:(NSString *)address {
	sAddAddress = address;
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

	_searchBar.showsCancelButton = (_searchBar.text.length > 0);

	if (tableController.isEditing) {
		tableController.editing = NO;
	}
	[self refreshButtons];
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
}

- (void) viewWillDisappear:(BOOL)animated {
	self.view = NULL;
	[self.tableController removeAllContacts];
}

#pragma mark -

- (void)changeView:(ContactsCategory)view {
	CGRect frame = _selectedButtonImage.frame;
	if (view == ContactsAll && !allButton.selected) {
		frame.origin.x = allButton.frame.origin.x;
		[ContactSelection setSipFilter:nil];
		[ContactSelection enableEmailFilter:FALSE];
		allButton.selected = TRUE;
		linphoneButton.selected = FALSE;
		[tableController loadData];
	} else if (view == ContactsLinphone && !linphoneButton.selected) {
		frame.origin.x = linphoneButton.frame.origin.x;
		[ContactSelection setSipFilter:LinphoneManager.instance.contactFilter];
		[ContactSelection enableEmailFilter:FALSE];
		linphoneButton.selected = TRUE;
		allButton.selected = FALSE;
		[tableController loadData];
	}
	_selectedButtonImage.frame = frame;
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
	NSString *msg = [NSString stringWithFormat:NSLocalizedString(@"Do you want to delete selected contacts?", nil)];
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
	allButton.hidden = linphoneButton.hidden = _selectedButtonImage.hidden = addButton.hidden =
		self.tableController.isEditing;
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
	searchBar.text = @"";
	[self searchBar:searchBar textDidChange:@""];
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
	if (![self.searchBar isFirstResponder]) {
		return NO;
	}
	return YES;
}

@end
