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

#import "ContactsViewController.h"
#import "PhoneMainView.h"
#import "Utils.h"

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
	if (sAddAddress != nil) {
		sAddAddress = nil;
	}
	if (address != nil) {
		sAddAddress = address;
	}
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

@implementation ContactsViewController

@synthesize tableController;
@synthesize tableView;

@synthesize sysViewController;

@synthesize allButton;
@synthesize linphoneButton;
@synthesize backButton;
@synthesize addButton;
@synthesize toolBar;

typedef enum _HistoryView { History_All, History_Linphone, History_Search, History_MAX } HistoryView;

#pragma mark - Lifecycle Functions

- (id)init {
	return [super initWithNibName:@"ContactsViewController" bundle:[NSBundle mainBundle]];
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:@"Contacts"
																content:@"ContactsViewController"
															   stateBar:nil
																 tabBar:@"UIMainBar"
															 fullscreen:false
														  landscapeMode:[LinphoneManager runningOnIpad]
														   portraitMode:true];
	}
	return compositeDescription;
}

#pragma mark - ViewController Functions

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
}

- (void)relayoutTableView {
	CGRect subViewFrame = self.view.frame;
	// let the toolBar be visible
	subViewFrame.origin.y += self.toolBar.frame.size.height;
	subViewFrame.size.height -= self.toolBar.frame.size.height;
	[UIView animateWithDuration:0.2
					 animations:^{
					   self.tableView.frame = subViewFrame;
					 }];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	// cannot change search bar icon nor text font from the interface builder...
	// [_searchBar setImage:[UIImage imageNamed:@"contact_search.png" ] forSearchBarIcon:UISearchBarIconSearch
	// state:UIControlStateNormal];
	// UITextField *searchText = [_searchBar valueForKey:@"_searchField"];
	// [searchText setFont:[UIFont fontWithName:@"CustomFont" size:12]];
	_searchBar.showsCancelButton = (_searchBar.text.length > 0);
	CGRect frame = _searchBar.frame;
	frame.origin.y = toolBar.frame.origin.y + toolBar.frame.size.height;
	_searchBar.frame = frame;

	BOOL use_system = [[LinphoneManager instance] lpConfigBoolForKey:@"use_system_contacts"];
	if (use_system && !self.sysViewController) { // use system contacts
		ABPeoplePickerNavigationController *picker = [[ABPeoplePickerNavigationController alloc] init];
		picker.peoplePickerDelegate = self;
		picker.view.frame = self.view.frame;
		[self.view addSubview:picker.view];
		self.sysViewController = picker;
		self.searchBar.hidden = TRUE;
	} else if (!use_system) {
		[self update];
	} else {
		// if table is already created, simply refresh buttons (selection mode changed, etc.)
		[self refreshButtons];
	}
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	if (![FastAddressBook isAuthorized]) {
		UIAlertView *error = [[UIAlertView alloc]
				initWithTitle:NSLocalizedString(@"Address book", nil)
					  message:NSLocalizedString(@"You must authorize the application to have access to address book.\n"
												 "Toggle the application in Settings > Privacy > Contacts",
												nil)
					 delegate:nil
			cancelButtonTitle:NSLocalizedString(@"Continue", nil)
			otherButtonTitles:nil];
		[error show];
		[[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]];
	}
}

- (void)centerTextOnIcon:(UIButton *)button {
	UIEdgeInsets inset = button.titleEdgeInsets;
	inset.left = -(button.imageView.frame.size.width);
	button.titleEdgeInsets = inset;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	[self changeView:History_All];
	[self centerTextOnIcon:allButton];
	[self centerTextOnIcon:linphoneButton];
}

#pragma mark -

- (void)changeView:(HistoryView)view {
	if (view == History_All) {
		[ContactSelection setSipFilter:nil];
		[ContactSelection enableEmailFilter:FALSE];
		[tableController loadData];
		allButton.selected = TRUE;
	} else {
		allButton.selected = FALSE;
	}

	if (view == History_Linphone) {
		[ContactSelection setSipFilter:[LinphoneManager instance].contactFilter];
		[ContactSelection enableEmailFilter:FALSE];
		[tableController loadData];
		linphoneButton.selected = TRUE;
	} else {
		linphoneButton.selected = FALSE;
	}
}

- (void)refreshButtons {
	switch ([ContactSelection getSelectionMode]) {
	case ContactSelectionModePhone:
	case ContactSelectionModeMessage:
		[addButton setHidden:TRUE];
		[backButton setHidden:FALSE];
		break;
	default:
		[addButton setHidden:FALSE];
		[backButton setHidden:TRUE];
		break;
	}
	if ([ContactSelection getSipFilter]) {
		allButton.selected = FALSE;
		linphoneButton.selected = TRUE;
	} else {
		allButton.selected = TRUE;
		linphoneButton.selected = FALSE;
	}
}

- (void)update {
	[self refreshButtons];
	[tableController loadData];
}

#pragma mark - Action Functions

- (IBAction)onAllClick:(id)event {
	[self changeView:History_All];
}

- (IBAction)onLinphoneClick:(id)event {
	[self changeView:History_Linphone];
}

- (IBAction)onAddContactClick:(id)event {
	// Go to Contact details view
	ContactDetailsViewController *controller = DYNAMIC_CAST(
		[[PhoneMainView instance] changeCurrentView:[ContactDetailsViewController compositeViewDescription] push:TRUE],
		ContactDetailsViewController);
	if (controller != nil) {
		if ([ContactSelection getAddAddress] == nil) {
			[controller newContact];
		} else {
			[controller newContact:[ContactSelection getAddAddress]];
		}
	}
}

- (IBAction)onBackClick:(id)event {
	[[PhoneMainView instance] popCurrentView];
}

- (void)searchBarCancelButtonClicked:(UISearchBar *)searchBar {
	[self searchBar:searchBar textDidChange:nil];
	[searchBar resignFirstResponder];
}

#pragma mark - Rotation handling

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[super didRotateFromInterfaceOrientation:fromInterfaceOrientation];
	// the searchbar overlaps the subview in most rotation cases, we have to re-layout the view manually:
	[self relayoutTableView];
}

#pragma mark - ABPeoplePickerDelegate

- (void)peoplePickerNavigationControllerDidCancel:(ABPeoplePickerNavigationController *)peoplePicker {
	[[PhoneMainView instance] popCurrentView];
	return;
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person {
	return true;
}

- (BOOL)peoplePickerNavigationController:(ABPeoplePickerNavigationController *)peoplePicker
	  shouldContinueAfterSelectingPerson:(ABRecordRef)person
								property:(ABPropertyID)property
							  identifier:(ABMultiValueIdentifier)identifier {

	CFTypeRef multiValue = ABRecordCopyValue(person, property);
	CFIndex valueIdx = ABMultiValueGetIndexForIdentifier(multiValue, identifier);
	NSString *phoneNumber = (NSString *)CFBridgingRelease(ABMultiValueCopyValueAtIndex(multiValue, valueIdx));
	// Go to dialer view
	DialerViewController *controller =
		DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[DialerViewController compositeViewDescription]],
					 DialerViewController);
	if (controller != nil) {
		[controller call:phoneNumber displayName:(NSString *)CFBridgingRelease(ABRecordCopyCompositeName(person))];
	}
	CFRelease(multiValue);
	return false;
}

#pragma mark - searchBar delegate

- (void)searchBar:(UISearchBar *)searchBar textDidChange:(NSString *)searchText {
	// display searchtext in UPPERCASE
	// searchBar.text = [searchText uppercaseString];
	searchBar.showsCancelButton = (searchText.length > 0);
	[ContactSelection setNameOrEmailFilter:searchText];
	[tableController loadData];
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

- (void)viewDidUnload {
	[self setToolBar:nil];
	[super viewDidUnload];
}
@end
