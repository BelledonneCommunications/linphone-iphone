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
#import "UIContactDetailsCell.h"

@implementation ContactDetailsView

#pragma mark - Lifecycle Functions

- (id)init {
	self = [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]];
	if (self != nil) {
		inhibUpdate = FALSE;
                [NSNotificationCenter.defaultCenter
                    addObserver:self
                       selector:@selector(onAddressBookUpdate:)
                           name:kLinphoneAddressBookUpdate
                         object:nil];
                [[NSNotificationCenter defaultCenter]
                    addObserver:self
                       selector:@selector(onAddressBookUpdate:)
                           name:CNContactStoreDidChangeNotification
                         object:nil];
        }
        return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark -

- (void)onAddressBookUpdate:(NSNotification *)k {
  if (!inhibUpdate && ![_tableController isEditing] &&
      (PhoneMainView.instance.currentView == self.compositeViewDescription) &&
      (_nameLabel.text == PhoneMainView.instance.currentName)) {
    [self resetData];
  }
}

- (void)resetData {
	if (self.isEditing) {
		[self setEditing:FALSE];
	}

	LOGI(@"Reset data to contact %p", _contact);
	[_avatarImage setImage:[FastAddressBook imageForContact:_contact] bordered:NO withRoundedRadius:YES];
	[_tableController setContact:_contact];
	_emptyLabel.hidden = YES;
	_avatarImage.hidden = !_emptyLabel.hidden;
	_deleteButton.hidden = !_emptyLabel.hidden;
	_editButton.hidden = !_emptyLabel.hidden;
}

- (void)removeContact {
	inhibUpdate = TRUE;
	[[LinphoneManager.instance fastAddressBook] deleteContact:_contact];
	inhibUpdate = FALSE;

	if (IPAD) {
		ContactsListView *view = VIEW(ContactsListView);
		if (![view .tableController selectFirstRow]) {
			[self setContact:nil];
		}
	}

	[PhoneMainView.instance popCurrentView];
}

- (void)saveData {
	if (_contact == NULL) {
		[PhoneMainView.instance popCurrentView];
		return;
	}
        PhoneMainView.instance.currentName = _contact.displayName;
        _nameLabel.text = PhoneMainView.instance.currentName;

    // fix no sipaddresses in contact.friend
    const MSList *sips = linphone_friend_get_addresses(_contact.friend);
    while (sips) {
        linphone_friend_remove_address(_contact.friend, sips->data);
        sips = sips->next;
    }
    
    for (NSString *sipAddr in _contact.sipAddresses) {
        LinphoneAddress *addr = linphone_core_interpret_url(LC, sipAddr.UTF8String);
        if (addr) {
            linphone_friend_add_address(_contact.friend, addr);
            linphone_address_destroy(addr);
        }
    }
        [LinphoneManager.instance.fastAddressBook saveContact:_contact];
}

- (void)selectContact:(Contact *)acontact andReload:(BOOL)reload {
	if (self.isEditing) {
		[self setEditing:FALSE];
	}

	_contact = acontact;
	_emptyLabel.hidden = (_contact != NULL);
	_avatarImage.hidden = !_emptyLabel.hidden;
	_deleteButton.hidden = !_emptyLabel.hidden;
	_editButton.hidden = !_emptyLabel.hidden;

	[_avatarImage setImage:[FastAddressBook imageForContact:_contact] bordered:NO withRoundedRadius:YES];
	[ContactDisplay setDisplayNameLabel:_nameLabel forContact:_contact];
	[_tableController setContact:_contact];

	if (reload) {
		[self setEditing:TRUE animated:FALSE];
	}
}

- (void)modifyTmpContact:(Contact *)acontact {
	if (_tmpContact) {
		_tmpContact = nil;
	}
	if (!acontact) {
		return;
	}
        @synchronized(LinphoneManager.instance.fastAddressBook) {
          _tmpContact = [[Contact alloc]
              initWithCNContact:[LinphoneManager.instance.fastAddressBook
                                    getCNContactFromContact:acontact]];
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
	_isAdding = TRUE;
        CNContact *contact = [[CNContact alloc] init];
        [self selectContact:[[Contact alloc] initWithCNContact:contact]
                  andReload:YES];
}

- (void)newContact:(NSString *)address {
  CNContact *contact = [[CNContact alloc] init];
  Contact *mContact = [[Contact alloc] initWithCNContact:contact];
  [self selectContact:mContact andReload:NO];
  [self addCurrentContactContactField:address];
  // force to restart server subscription to add new contact into the list
  [LinphoneManager.instance becomeActive];
}

- (void)editContact:(Contact *)acontact {
	[self modifyTmpContact:acontact];
	[self selectContact:acontact andReload:YES];
}

- (void)editContact:(Contact *)acontact address:(NSString *)address {
	[self modifyTmpContact:acontact];
	[self selectContact:acontact andReload:NO];
	[self addCurrentContactContactField:address];
}

- (void)setContact:(Contact *)acontact {
	[self selectContact:acontact andReload:NO];
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
	
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
								   initWithTarget:self
								   action:@selector(dismissKeyboards)];
	
	[self.view addGestureRecognizer:tap];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	_waitView.hidden = YES;
	_editButton.hidden = ([ContactSelection getSelectionMode] != ContactSelectionModeEdit &&
						  [ContactSelection getSelectionMode] != ContactSelectionModeNone);
	[_tableController.tableView addObserver:self forKeyPath:@"contentSize" options:0 context:NULL];
	_tableController.waitView = _waitView;
	self.tmpContact = NULL;
	
	[[NSNotificationCenter defaultCenter] addObserver: self
											 selector: @selector(deviceOrientationDidChange:)
												 name: UIDeviceOrientationDidChangeNotification
											   object: nil];
	if (IPAD && self.contact == NULL) {
		_editButton.hidden = TRUE;
		_deleteButton.hidden = TRUE;
	}
	PhoneMainView.instance.currentName = _nameLabel.text;
	// Update presence for contact
	for (NSInteger j = 0; j < [self.tableController.tableView numberOfSections]; ++j) {
		for (NSInteger i = 0; i < [self.tableController.tableView numberOfRowsInSection:j]; ++i) {
			[(UIContactDetailsCell *)[self.tableController.tableView
				cellForRowAtIndexPath:[NSIndexPath indexPathForRow:i inSection:j]] shouldHideLinphoneImageOfAddress];
		}
	}
}

- (void)deviceOrientationDidChange:(NSNotification*)notif {
	if (IPAD) {
		if (self.contact == NULL || (self.contact.firstName == NULL && self.contact.lastName == NULL)) {
			if (! self.tableController.isEditing) {
				_editButton.hidden = TRUE;
				_deleteButton.hidden = TRUE;
				_avatarImage.hidden = TRUE;
				_emptyLabel.hidden = FALSE;
			}
		}
	}
	
	if (self.tableController.isEditing) {
		_backButton.hidden = TRUE;
		_cancelButton.hidden = FALSE;
	} else {
		if (!IPAD) {
			_backButton.hidden = FALSE;
		}
		_cancelButton.hidden = TRUE;
	}
    
    [self recomputeTableViewSize:_editButton.hidden];
}

- (void)viewWillDisappear:(BOOL)animated {
	if (_tableController && _tableController.tableView && [_tableController.tableView observationInfo]) {
		[_tableController.tableView removeObserver:self forKeyPath:@"contentSize"];
	}
	[super viewWillDisappear:animated];
	PhoneMainView.instance.currentName = NULL;
	if (self.tmpContact) {
		_contact.firstName = _tmpContact.firstName.copy;
		_contact.lastName = _tmpContact.lastName.copy;
		while (_contact.sipAddresses.count > 0) {
			[_contact removeSipAddressAtIndex:0];
		}
		NSInteger nbSipAd = 0;
		while (_tmpContact.sipAddresses.count > nbSipAd) {
			[_contact addSipAddress:_tmpContact.sipAddresses[nbSipAd]];
			nbSipAd++;
		}
		while (_contact.phones.count > 0) {
			[_contact removePhoneNumberAtIndex:0];
		}
		NSInteger nbPhone = 0;
		while (_tmpContact.phones.count > nbPhone) {
			[_contact addPhoneNumber:_tmpContact.phones[nbPhone]];
			nbPhone++;
		}
		while (_contact.emails.count > 0) {
			[_contact removeEmailAtIndex:0];
		}
		NSInteger nbEmail = 0;
		while (_tmpContact.emails.count > nbEmail) {
			[_contact addEmail:_tmpContact.emails[nbEmail]];
			nbEmail++;
		}
		self.tmpContact = NULL;
		[self saveData];
	}

	BOOL rm = TRUE;
	for (NSString *sip in _contact.sipAddresses) {
		if (![sip isEqualToString:@""]) {
			rm = FALSE;
			break;
		}
	}
	if (rm) {
		for (NSString *phone in _contact.phones) {
			if (![phone isEqualToString:@""]) {
				rm = FALSE;
				break;
			}
		}
	}
	if (rm) {
		[LinphoneManager.instance.fastAddressBook deleteContact:_contact];
	}
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
	if (editing) {
		_editButton.hidden = FALSE;
		_deleteButton.hidden = FALSE;
		_avatarImage.hidden = FALSE;
	} else {
		_editButton.hidden = TRUE;
		_deleteButton.hidden = TRUE;
		_avatarImage.hidden = TRUE;
	}

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

    [self recomputeTableViewSize:editing];

	if (animated) {
		[UIView commitAnimations];
	}
}

- (void)recomputeTableViewSize:(BOOL)editing {
    CGRect frame = _tableController.tableView.frame;
    frame.origin.y = _avatarImage.frame.size.height + _avatarImage.frame.origin.y;
    if ([self viewIsCurrentlyPortrait] && !editing) {
        frame.origin.y += _nameLabel.frame.size.height;
    }
    
    frame.size.height = _tableController.tableView.contentSize.height;
    _tableController.tableView.frame = frame;
    [self recomputeContentViewSize];
}

- (void)observeValueForKeyPath:(NSString *)keyPath
					  ofObject:(id)object
						change:(NSDictionary *)change
					   context:(void *)context {
	CGRect frame = _tableController.tableView.frame;
	frame.size = _tableController.tableView.contentSize;
	_tableController.tableView.frame = frame;
	[self recomputeContentViewSize];
}

- (void)recomputeContentViewSize {
	_contentView.contentSize =
		CGSizeMake(_tableController.tableView.frame.size.width + _tableController.tableView.frame.origin.x,
				   _tableController.tableView.frame.size.height + _tableController.tableView.frame.origin.y);
}

#pragma mark - Action Functions

- (IBAction)onCancelClick:(id)event {
	[self dismissKeyboards];
	if (!_isAdding) {
		_contact.firstName = _tmpContact.firstName.copy;
		_contact.lastName = _tmpContact.lastName.copy;
		while (_contact.sipAddresses.count > 0) {
			[_contact removeSipAddressAtIndex:0];
		}
		NSInteger nbSipAd = 0;
                if (_tmpContact.sipAddresses) {
                  while (_tmpContact.sipAddresses.count > nbSipAd) {
                    [_contact addSipAddress:_tmpContact.sipAddresses[nbSipAd]];
                    nbSipAd++;
                  }
                }
                while (_contact.phones.count > 0 &&
                       _contact.phones[0] != NULL) {
                  [_contact removePhoneNumberAtIndex:0];
                }
                NSInteger nbPhone = 0;
                if (_tmpContact.phones != NULL) {
                  while (_tmpContact.phones.count > nbPhone) {
                    [_contact addPhoneNumber:_tmpContact.phones[nbPhone]];
                    nbPhone++;
                  }
                }
                while (_contact.emails.count > 0) {
                  [_contact removeEmailAtIndex:0];
                }
                NSInteger nbEmail = 0;
                if (_tmpContact.emails != NULL) {
                  while (_tmpContact.emails.count > nbEmail) {
                    [_contact addEmail:_tmpContact.emails[nbEmail]];
                    nbEmail++;
                  }
                }
     //           [self saveData];
        } else {
          [LinphoneManager.instance.fastAddressBook deleteContact:_contact];
        }

        [self setEditing:FALSE];
        if (IPAD) {
          _emptyLabel.hidden = !_isAdding;
          _avatarImage.hidden = !_emptyLabel.hidden;
          _deleteButton.hidden = !_emptyLabel.hidden;
          _editButton.hidden = !_emptyLabel.hidden;
        } else {
          if (_isAdding) {
            [PhoneMainView.instance popCurrentView];
          } else {
            _avatarImage.hidden = FALSE;
            _deleteButton.hidden = FALSE;
            _editButton.hidden = FALSE;
          }
        }

        self.tmpContact = NULL;
        if (_isAdding) {
          [PhoneMainView.instance
              popToView:ContactsListView.compositeViewDescription];
          _isAdding = FALSE;
        }
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
		[LinphoneManager.instance setContactsUpdated:TRUE];
		if(![self hasDuplicateContactOf:_contact]){
			[self setEditing:FALSE];
			[self saveData];
			_isAdding = FALSE;
			self.tmpContact = NULL;
			_avatarImage.hidden = FALSE;
			_deleteButton.hidden = FALSE;
			_editButton.hidden = FALSE;
		}else{
			LOGE(@"====>>>> Duplicated Contacts detected !!!");
			[[[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Contact error", nil) message:NSLocalizedString(@"Contact duplicate", nil) delegate:nil cancelButtonTitle:nil otherButtonTitles:@"Continue", nil] show];
		}
	} else {
		[self modifyTmpContact:_contact];
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
						if (_tableController.isEditing) {
							[self onCancelClick:sender];
						}
						[self removeContact];
					  }];
}

- (IBAction)onAvatarClick:(id)sender {
	[LinphoneUtils findAndResignFirstResponder:self.view];
	if (_tableController.isEditing) {
		[ImagePickerView SelectImageFromDevice:self atPosition:_avatarImage inView:self.view withDocumentMenuDelegate:nil];
	}
}

- (void)dismissKeyboards {
	NSArray *cells = [self.tableController.tableView visibleCells];
	for (UIContactDetailsCell *cell in cells) {
		UIView * txt = cell.editTextfield;
		if ([txt isKindOfClass:[UITextField class]] && [txt isFirstResponder]) {
			[txt resignFirstResponder];
		}
	}
}

- (BOOL) hasDuplicateContactOf:(Contact*) contactToCheck{
	CNContactStore *store = [[CNContactStore alloc] init];
	NSArray *keysToFetch = @[
							 CNContactEmailAddressesKey, CNContactPhoneNumbersKey,
							 CNContactInstantMessageAddressesKey, CNInstantMessageAddressUsernameKey,
							 CNContactFamilyNameKey, CNContactGivenNameKey, CNContactPostalAddressesKey,
							 CNContactIdentifierKey, CNContactImageDataKey, CNContactNicknameKey
							 ];
	CNMutableContact *mCNContact =
	[[store unifiedContactWithIdentifier:contactToCheck.identifier keysToFetch:keysToFetch error:nil] mutableCopy];
	if(mCNContact == NULL){
		for(NSString *address in contactToCheck.sipAddresses){
			NSString *name = [FastAddressBook normalizeSipURI:address];
			if([LinphoneManager.instance.fastAddressBook.addressBookMap objectForKey:name]){
				return TRUE;
			}
		}
		return FALSE;
	}else{
		return FALSE;
	}
}


#pragma mark - Image picker delegate

- (void)imagePickerDelegateImage:(UIImage *)image info:(NSDictionary *)info {
	// When getting image from the camera, it may be 90° rotated due to orientation
	// (image.imageOrientation = UIImageOrientationRight). Just rotate it to be face up.
	if (image.imageOrientation != UIImageOrientationUp) {
		UIGraphicsBeginImageContextWithOptions(image.size, false, image.scale);
		[image drawInRect:CGRectMake(0, 0, image.size.width, image.size.height)];
		image = UIGraphicsGetImageFromCurrentImageContext();
		UIGraphicsEndImageContext();
	}

	// Dismiss popover on iPad
	if (IPAD) {
		[VIEW(ImagePickerView).popoverController dismissPopoverAnimated:TRUE];
	}

	[_contact setAvatar:image];

	[_avatarImage setImage:[FastAddressBook imageForContact:_contact] bordered:NO withRoundedRadius:YES];
}

- (void)imagePickerDelegateVideo:(NSURL*)url info:(NSDictionary *)info {
	return;
}

@end
