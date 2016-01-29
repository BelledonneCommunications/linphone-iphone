/* ContactDetailsViewController.m
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

#import "ContactDetailsView.h"
#import "PhoneMainView.h"

@implementation ContactDetailsView

static void sync_address_book(ABAddressBookRef addressBook, CFDictionaryRef info, void *context);

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]];
	if (self != nil) {
		inhibUpdate = FALSE;
		addressBook = ABAddressBookCreateWithOptions(nil, nil);
		ABAddressBookRegisterExternalChangeCallback(addressBook, sync_address_book, (__bridge void *)(self));
	}
	return self;
}

- (void)dealloc {
	ABAddressBookUnregisterExternalChangeCallback(addressBook, sync_address_book, (__bridge void *)(self));
	CFRelease(addressBook);
}

#pragma mark -

- (void)resetData {
	if (self.isEditing) {
		[self setEditing:FALSE];
	}
	if (_contact == NULL) {
		ABAddressBookRevert(addressBook);
		return;
	}

	ABRecordID recordID = ABRecordGetRecordID(_contact);
	ABAddressBookRevert(addressBook);
	_contact = ABAddressBookGetPersonWithRecordID(addressBook, recordID);

	if (_contact != NULL) {
		LOGI(@"Reset data to contact %p", _contact);
		[_avatarImage setImage:[FastAddressBook imageForContact:_contact thumbnail:NO]
					  bordered:NO
			 withRoundedRadius:YES];
		[_tableController setContact:[[Contact alloc] initWithPerson:_contact]];
		_emptyLabel.hidden = YES;
	} else {
		_emptyLabel.hidden = NO;
	}
}

static void sync_address_book(ABAddressBookRef addressBook, CFDictionaryRef info, void *context) {
	ContactDetailsView *controller = (__bridge ContactDetailsView *)context;
	if (!controller->inhibUpdate && ![[controller tableController] isEditing]) {
		[controller resetData];
	}
}

- (void)removeContact {
	if (_contact != NULL) {
		inhibUpdate = TRUE;
		[[LinphoneManager.instance fastAddressBook] removeContact:_contact];
		inhibUpdate = FALSE;
	}
	[PhoneMainView.instance popCurrentView];
}

- (void)saveData {
	if (_contact == NULL) {
		[PhoneMainView.instance popCurrentView];
		return;
	}

	// Add contact to book
	CFErrorRef error = NULL;
	if (ABRecordGetRecordID(_contact) == kABRecordInvalidID) {
		ABAddressBookAddRecord(addressBook, _contact, (CFErrorRef *)&error);
		if (error != NULL) {
			LOGE(@"Add contact %p: Fail(%@)", _contact, [(__bridge NSError *)error localizedDescription]);
		} else {
			LOGI(@"Add contact %p: Success!", _contact);
		}
	}

	// Save address book
	error = NULL;
	inhibUpdate = TRUE;
	ABAddressBookSave(addressBook, &error);
	inhibUpdate = FALSE;
	if (error != NULL) {
		LOGE(@"Save AddressBook: Fail(%@)", [(__bridge NSError *)error localizedDescription]);
	} else {
		LOGI(@"Save AddressBook: Success!");
	}
	[LinphoneManager.instance.fastAddressBook reload];
}

- (void)selectContact:(ABRecordRef)acontact andReload:(BOOL)reload {
	_contact = NULL;
	[self resetData];

	_emptyLabel.hidden = (acontact != NULL);

	_contact = acontact;
	[_avatarImage setImage:[FastAddressBook imageForContact:_contact thumbnail:NO] bordered:NO withRoundedRadius:YES];
	[ContactDisplay setDisplayNameLabel:_nameLabel forContact:acontact];
	[_tableController setContact:[[Contact alloc] initWithPerson:_contact]];

	if (reload) {
		[self setEditing:TRUE animated:FALSE];
	}
}

- (void)addCurrentContactContactField:(NSString *)address {
	LinphoneAddress *linphoneAddress = linphone_core_interpret_url(LC, address.UTF8String);
	NSString *username =
		linphoneAddress ? [NSString stringWithUTF8String:linphone_address_get_username(linphoneAddress)] : address;

	if (([username rangeOfString:@"@"].length > 0) &&
		([LinphoneManager.instance lpConfigBoolForKey:@"show_contacts_emails_preference"] == true)) {
		[_tableController addEmailField:username];
	} else if ((linphone_proxy_config_is_phone_number(NULL, [username UTF8String])) &&
			   ([LinphoneManager.instance lpConfigBoolForKey:@"save_new_contacts_as_phone_number"] == true)) {
		[_tableController addPhoneField:username];
	} else {
		[_tableController addSipField:address];
	}
	if (linphoneAddress) {
		linphone_address_destroy(linphoneAddress);
	}
	[self setEditing:TRUE];
	[[_tableController tableView] reloadData];
}

- (void)newContact {
	[self selectContact:ABPersonCreate() andReload:YES];
}

- (void)newContact:(NSString *)address {
	[self selectContact:ABPersonCreate() andReload:NO];
	[self addCurrentContactContactField:address];
}

- (void)editContact:(ABRecordRef)acontact {
	[self selectContact:ABAddressBookGetPersonWithRecordID(addressBook, ABRecordGetRecordID(acontact)) andReload:YES];
}

- (void)editContact:(ABRecordRef)acontact address:(NSString *)address {
	[self selectContact:ABAddressBookGetPersonWithRecordID(addressBook, ABRecordGetRecordID(acontact)) andReload:NO];
	[self addCurrentContactContactField:address];
}

- (void)setContact:(ABRecordRef)acontact {
	[self selectContact:ABAddressBookGetPersonWithRecordID(addressBook, ABRecordGetRecordID(acontact)) andReload:NO];
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];

	// if we use fragments, remove back button
	if (IPAD) {
		_backButton.hidden = YES;
		_backButton.alpha = 0;
	}

	[self setContact:NULL];

	_tableController.tableView.accessibilityIdentifier = @"Contact table";

	[_editButton setImage:[UIImage imageNamed:@"valid_disabled.png"]
				 forState:(UIControlStateDisabled | UIControlStateSelected)];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	_editButton.hidden = ([ContactSelection getSelectionMode] != ContactSelectionModeEdit &&
						  [ContactSelection getSelectionMode] != ContactSelectionModeNone);
}

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:TabBarView.class
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:NO
														   fragmentWith:ContactsListView.class];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark -

- (void)setEditing:(BOOL)editing {
	[self setEditing:editing animated:NO];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	[super setEditing:editing animated:animated];

	if (animated) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:1.0];
	}
	[_tableController setEditing:editing animated:animated];
	if (editing) {
		[_editButton setOn];
	} else {
		[_editButton setOff];
	}
	_cancelButton.hidden = !editing;
	_backButton.hidden = editing;
	_nameLabel.hidden = editing;
	[ContactDisplay setDisplayNameLabel:_nameLabel forContact:_contact];

	if ([self viewIsCurrentlyPortrait]) {
		CGRect frame = self.contentView.frame;
		frame.size.height -= _avatarImage.frame.origin.y + _avatarImage.frame.size.height;
		frame.origin.y = _nameLabel.frame.origin.y;
		if (!editing) {
			frame.origin.y += _nameLabel.frame.size.height;
			frame.size.height -= _nameLabel.frame.size.height;
		}

		_tableController.tableView.frame = frame;
	}
	if (animated) {
		[UIView commitAnimations];
	}
}

#pragma mark - Action Functions

- (IBAction)onCancelClick:(id)event {
	[self setEditing:FALSE];
	[self resetData];
}

- (IBAction)onBackClick:(id)event {
	if ([ContactSelection getSelectionMode] == ContactSelectionModeEdit) {
		[ContactSelection setSelectionMode:ContactSelectionModeNone];
	}

	ContactsListView *view = VIEW(ContactsListView);
	[PhoneMainView.instance popToView:view.compositeViewDescription];
}

- (IBAction)onEditClick:(id)event {
	if (_tableController.isEditing) {
		[self setEditing:FALSE];
		[self saveData];
	} else {
		[self setEditing:TRUE];
	}
}

- (IBAction)onDeleteClick:(id)sender {
	NSString *msg = NSLocalizedString(@"Do you want to delete selected contact?", nil);
	[UIConfirmationDialog ShowWithMessage:msg
							cancelMessage:nil
						   confirmMessage:nil
							onCancelClick:nil
					  onConfirmationClick:^() {
						[self setEditing:FALSE];
						[self removeContact];
					  }];
}

- (IBAction)onAvatarClick:(id)sender {
	[LinphoneUtils findAndResignFirstResponder:self.view];
	if (_tableController.isEditing) {
		[ImagePickerView SelectImageFromDevice:self atPosition:_avatarImage inView:self.view];
	}
}

#pragma mark - Image picker delegate

- (void)imagePickerDelegateImage:(UIImage *)image info:(NSDictionary *)info {
	// Dismiss popover on iPad
	if (IPAD) {
		[VIEW(ImagePickerView).popoverController dismissPopoverAnimated:TRUE];
	}

	FastAddressBook *fab = LinphoneManager.instance.fastAddressBook;
	CFErrorRef error = NULL;
	if (!ABPersonRemoveImageData(_contact, (CFErrorRef *)&error)) {
		LOGI(@"Can't remove entry: %@", [(__bridge NSError *)error localizedDescription]);
	}
	NSData *dataRef = UIImageJPEGRepresentation(image, 0.9f);
	CFDataRef cfdata = CFDataCreate(NULL, [dataRef bytes], [dataRef length]);

	[fab saveAddressBook];

	if (!ABPersonSetImageData(_contact, cfdata, (CFErrorRef *)&error)) {
		LOGI(@"Can't add entry: %@", [(__bridge NSError *)error localizedDescription]);
	} else {
		[fab saveAddressBook];
	}

	CFRelease(cfdata);

	[_avatarImage setImage:[FastAddressBook imageForContact:_contact thumbnail:NO] bordered:NO withRoundedRadius:YES];
}
@end
