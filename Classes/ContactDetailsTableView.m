/* ContactDetailsTableViewController.m
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

#import "ContactDetailsTableView.h"
#import "PhoneMainView.h"
#import "UIContactDetailsCell.h"
#import "Utils.h"
#import "OrderedDictionary.h"

@implementation ContactDetailsTableView

#pragma mark - Property Functions

- (NSMutableArray *)getSectionData:(NSInteger)section {
	if (section == ContactSections_Number) {
		return _contact.phoneNumbers;
	} else if (section == ContactSections_Sip) {
		return _contact.sipAddresses;
	} else if (section == ContactSections_Email) {
		if ([LinphoneManager.instance lpConfigBoolForKey:@"show_contacts_emails_preference"] == true) {
			return _contact.emails;
		}
	}
	return nil;
}

- (void)removeEmptyEntry:(UITableView *)tableview section:(NSInteger)section animated:(BOOL)animated {
	NSMutableArray *sectionDict = [self getSectionData:section];
	for (NSInteger i = sectionDict.count - 1; i >= 0; i--) {
		NSString *value = sectionDict[i];
		if (value.length == 0) {
			[self removeEntry:tableview indexPath:[NSIndexPath indexPathForRow:i inSection:section] animated:animated];
		}
	}
}

- (void)removeEntry:(UITableView *)tableview indexPath:(NSIndexPath *)path animated:(BOOL)animated {
	bool rmed = YES;
	if (path.section == ContactSections_Number) {
		rmed = [_contact removePhoneNumberAtIndex:path.row];
	} else if (path.section == ContactSections_Sip) {
		rmed = [_contact removeSipAddressAtIndex:path.row];
	} else if (path.section == ContactSections_Email) {
		rmed = [_contact removeEmailAtIndex:path.row];
	} else {
		rmed = NO;
	}

	if (rmed) {
		[tableview deleteRowsAtIndexPaths:@[ path ]
						 withRowAnimation:animated ? UITableViewRowAnimationFade : UITableViewRowAnimationNone];
	} else {
		LOGW(@"Cannot remove entry at path %@, skipping", path);
	}
}

- (void)addEntry:(UITableView *)tableview section:(NSInteger)section animated:(BOOL)animated value:(NSString *)value {
	bool added = FALSE;
	if (section == ContactSections_Number) {
		added = [_contact addPhoneNumber:value];
	} else if (section == ContactSections_Sip) {
		added = [_contact addSipAddress:value];
	} else if (section == ContactSections_Email) {
		added = [_contact addEmail:value];
	}

	if (added) {
		NSUInteger count = [self getSectionData:section].count;
		[tableview insertRowsAtIndexPaths:@[ [NSIndexPath indexPathForRow:count - 1 inSection:section] ]
						 withRowAnimation:animated ? UITableViewRowAnimationFade : UITableViewRowAnimationNone];
	} else {
		LOGW(@"Cannot add entry '%@' in section %d, skipping", value, section);
	}
}

- (void)setContact:(Contact *)acontact {
	if (acontact == _contact)
		return;
	_contact = acontact;
	[self loadData];
}

- (void)addPhoneField:(NSString *)number {
	ContactSections i = 0;
	while (i != ContactSections_MAX && i != ContactSections_Number)
		++i;
	[self addEntry:[self tableView] section:i animated:FALSE value:number];
}

- (void)addSipField:(NSString *)address {
	ContactSections i = 0;
	while (i != ContactSections_MAX && i != ContactSections_Sip)
		++i;
	[self addEntry:[self tableView] section:i animated:FALSE value:address];
}

- (void)addEmailField:(NSString *)address {
	ContactSections i = 0;
	while (i != ContactSections_MAX && i != ContactSections_Email)
		++i;
	[self addEntry:[self tableView] section:i animated:FALSE value:address];
}

- (BOOL)isValid {
	BOOL hasName = (_contact.firstName.length + _contact.lastName.length > 0);
	BOOL hasAddr = (_contact.phoneNumbers.count + _contact.sipAddresses.count) > 0;
	return hasName && hasAddr;
}

#pragma mark - UITableViewDataSource Functions

- (void)loadData {
	[self.tableView reloadData];
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return ContactSections_MAX;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (section == ContactSections_FirstName || section == ContactSections_LastName) {
		/*first and last name only when editting */
		return (self.tableView.isEditing) ? 1 : 0;
	} else if (section == ContactSections_Sip) {
		return _contact.sipAddresses.count;
	} else if (section == ContactSections_Number) {
		return _contact.phoneNumbers.count;
	} else if (section == ContactSections_Email) {
		BOOL showEmails = [LinphoneManager.instance lpConfigBoolForKey:@"show_contacts_emails_preference"];
		return showEmails ? _contact.emails.count : 0;
	}
	return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	static NSString *kCellId = @"UIContactDetailsCell";
	UIContactDetailsCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIContactDetailsCell alloc] initWithIdentifier:kCellId];
		[cell.editTextfield setDelegate:self];
	}

	cell.selectionStyle = UITableViewCellSelectionStyleNone;

	cell.indexPath = indexPath;
	[cell hideDeleteButton:NO];
	[cell.editTextfield setKeyboardType:UIKeyboardTypeDefault];
	NSString *value = @"";
	if (indexPath.section == ContactSections_FirstName) {
		value = _contact.firstName;
		[cell hideDeleteButton:YES];
	} else if (indexPath.section == ContactSections_LastName) {
		value = _contact.lastName;
		[cell hideDeleteButton:YES];
	} else if ([indexPath section] == ContactSections_Number) {
		value = _contact.phoneNumbers[indexPath.row];
		[cell.editTextfield setKeyboardType:UIKeyboardTypePhonePad];
	} else if ([indexPath section] == ContactSections_Sip) {
		value = _contact.sipAddresses[indexPath.row];
		LinphoneAddress *addr = NULL;
		if ([LinphoneManager.instance lpConfigBoolForKey:@"contact_display_username_only"] &&
			(addr = linphone_core_interpret_url(LC, [value UTF8String]))) {
			value = [NSString stringWithCString:linphone_address_get_username(addr)
									   encoding:[NSString defaultCStringEncoding]];
			linphone_address_destroy(addr);
		}
		[cell.editTextfield setKeyboardType:UIKeyboardTypeASCIICapable];
	} else if ([indexPath section] == ContactSections_Email) {
		value = _contact.emails[indexPath.row];
		[cell.editTextfield setKeyboardType:UIKeyboardTypeEmailAddress];
	}

	[cell setAddress:value];

	return cell;
}

- (void)tableView:(UITableView *)tableView
	commitEditingStyle:(UITableViewCellEditingStyle)editingStyle
	 forRowAtIndexPath:(NSIndexPath *)indexPath {
	[LinphoneUtils findAndResignFirstResponder:[self tableView]];
	if (editingStyle == UITableViewCellEditingStyleInsert) {
		[tableView beginUpdates];
		[self addEntry:tableView section:[indexPath section] animated:TRUE value:@""];
		[tableView endUpdates];
	} else if (editingStyle == UITableViewCellEditingStyleDelete) {
		[tableView beginUpdates];
		[self removeEntry:tableView indexPath:indexPath animated:TRUE];
		[tableView endUpdates];
	}
}

#pragma mark - UITableViewDelegate Functions

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	BOOL showEmails = [LinphoneManager.instance lpConfigBoolForKey:@"show_contacts_emails_preference"];
	if (editing) {
		// add phone/SIP/email entries so that the user can add new data
		for (int section = 0; section < [self numberOfSectionsInTableView:[self tableView]]; ++section) {
			if (section == ContactSections_Number || section == ContactSections_Sip ||
				(showEmails && section == ContactSections_Email)) {
				[self addEntry:self.tableView section:section animated:animated value:@""];
			}
		}
		_editButton.enabled = [self isValid];
	} else {
		[LinphoneUtils findAndResignFirstResponder:[self tableView]];
		// remove empty phone numbers
		for (int section = 0; section < [self numberOfSectionsInTableView:[self tableView]]; ++section) {
			// remove phony entries that were not filled by the user
			if (section == ContactSections_Number || section == ContactSections_Sip ||
				(showEmails && section == ContactSections_Email)) {

				[self removeEmptyEntry:self.tableView section:section animated:NO];
				// the section is empty -> remove titles
				if ([[self getSectionData:section] count] == 0) {
					[self.tableView
						  reloadSections:[NSIndexSet indexSetWithIndex:section]
						withRowAnimation:animated ? UITableViewRowAnimationFade : UITableViewRowAnimationNone];
				}
			}
		}
		_editButton.enabled = YES;
	}
	// order is imported here: empty rows must be deleted before table change editing mode
	[super setEditing:editing animated:animated];

	[self loadData];
}

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	NSString *text = nil;
	BOOL canAddEntry = self.tableView.isEditing;
	NSString *addEntryName = nil;
	if (section == ContactSections_FirstName && self.tableView.isEditing) {
		text = NSLocalizedString(@"First name", nil);
		canAddEntry = NO;
	} else if (section == ContactSections_LastName && self.tableView.isEditing) {
		text = NSLocalizedString(@"Last name", nil);
		canAddEntry = NO;
	} else if ([self getSectionData:section].count > 0 || self.tableView.isEditing) {
		if (section == ContactSections_Number) {
			text = NSLocalizedString(@"Phone numbers", nil);
			addEntryName = NSLocalizedString(@"Add new phone number", nil);
		} else if (section == ContactSections_Sip) {
			text = NSLocalizedString(@"SIP addresses", nil);
			addEntryName = NSLocalizedString(@"Add new SIP address", nil);
		} else if (section == ContactSections_Email &&
				   [LinphoneManager.instance lpConfigBoolForKey:@"show_contacts_emails_preference"]) {
			text = NSLocalizedString(@"Email addresses", nil);
			addEntryName = NSLocalizedString(@"Add new email", nil);
		}
	}

	if (!text) {
		return nil;
	}

	CGRect frame = CGRectMake(0, 0, tableView.frame.size.width, 30);
	UIView *tempView = [[UIView alloc] initWithFrame:frame];
	tempView.backgroundColor = [UIColor whiteColor];

	UILabel *tempLabel = [[UILabel alloc] initWithFrame:frame];
	tempLabel.backgroundColor = [UIColor clearColor];
	tempLabel.textColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_E.png"]];
	tempLabel.text = text.uppercaseString;
	tempLabel.textAlignment = NSTextAlignmentCenter;
	tempLabel.font = [UIFont systemFontOfSize:15];
	tempLabel.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin;
	[tempView addSubview:tempLabel];

	if (canAddEntry) {
		frame.origin.x = (tableView.frame.size.width - 30 /*image size*/) / 2 - 5 /*right offset*/;
		UIIconButton *tempAddButton = [[UIIconButton alloc] initWithFrame:frame];
		[tempAddButton setImage:[UIImage imageNamed:@"add_field_default.png"] forState:UIControlStateNormal];
		[tempAddButton setImage:[UIImage imageNamed:@"add_field_over.png"] forState:UIControlStateHighlighted];
		[tempAddButton setImage:[UIImage imageNamed:@"add_field_over.png"] forState:UIControlStateSelected];
		[tempAddButton addTarget:self action:@selector(onAddClick:) forControlEvents:UIControlEventTouchUpInside];
		tempAddButton.tag = section;
		tempAddButton.accessibilityLabel = addEntryName;
		tempAddButton.autoresizingMask = UIViewAutoresizingFlexibleLeftMargin;
		[tempView addSubview:tempAddButton];
	}

	return tempView;
}

- (void)onAddClick:(id)sender {
	NSInteger section = ((UIButton *)sender).tag;
	UITableView *tableView = VIEW(ContactDetailsView).tableController.tableView;
	NSInteger count = [self.tableView numberOfRowsInSection:section];
	NSIndexPath *indexPath = [NSIndexPath indexPathForRow:count inSection:section];
	[tableView.dataSource tableView:tableView
				 commitEditingStyle:UITableViewCellEditingStyleInsert
				  forRowAtIndexPath:indexPath];
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView
		   editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
	return UITableViewCellEditingStyleNone;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
	if (tableView.isEditing) {
		return 44;
	} else {
		return 88;
	}
}
- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section {
	return 1e-5;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	if (section == 0 ||
		(!self.tableView.isEditing && (section == ContactSections_FirstName || section == ContactSections_LastName))) {
		return 1e-5;
	}
	return [self tableView:tableView viewForHeaderInSection:section].frame.size.height;
}

#pragma mark - UITextFieldDelegate Functions

- (BOOL)textFieldShouldReturn:(UITextField *)textField {
	[textField resignFirstResponder];
	return YES;
}

- (void)textFieldUpdated:(UITextField *)textField {
	UIView *view = [textField superview];
	while (view != nil && ![view isKindOfClass:[UIContactDetailsCell class]])
		view = [view superview];
	if (view != nil) {
		UIContactDetailsCell *cell = (UIContactDetailsCell *)view;
		// we cannot use indexPathForCell method here because if the cell is not visible anymore,
		// it will return nil..
		NSIndexPath *path = [self.tableView indexPathForCell:cell]; // [self.tableView indexPathForCell:cell];
		ContactSections sect = (ContactSections)[path section];
		NSString *value = [textField text];

		switch (sect) {
			case ContactSections_FirstName:
				_contact.firstName = value;
				break;
			case ContactSections_LastName:
				_contact.lastName = value;
				break;
			case ContactSections_Sip:
				[_contact setSipAddress:value atIndex:path.row];
				value = _contact.sipAddresses[path.row]; // in case of reformatting
				break;
			case ContactSections_Email:
				[_contact setEmail:value atIndex:path.row];
				value = _contact.emails[path.row]; // in case of reformatting
				break;
			case ContactSections_Number:
				[_contact setPhoneNumber:value atIndex:path.row];
				value = _contact.phoneNumbers[path.row]; // in case of reformatting
				break;
			case ContactSections_MAX:
			case ContactSections_None:
				break;
		}
		cell.editTextfield.text = value;
		_editButton.enabled = [self isValid];
	}
}

- (void)textFieldDidEndEditing:(UITextField *)textField {
	[self textFieldUpdated:textField];
}

- (BOOL)textField:(UITextField *)textField
	shouldChangeCharactersInRange:(NSRange)range
				replacementString:(NSString *)string {
#if 0
	// every time we modify contact entry, we must check if we can enable "edit" button
	UIView *view = [textField superview];
	while (view != nil && ![view isKindOfClass:[UIContactDetailsCell class]])
		view = [view superview];

	UIContactDetailsCell *cell = (UIContactDetailsCell *)view;
	// we cannot use indexPathForCell method here because if the cell is not visible anymore,
	// it will return nil..
	NSIndexPath *path = cell.indexPath;

	_editButton.enabled = NO;
	for (ContactSections s = ContactSections_Sip; !_editButton.enabled && s <= ContactSections_Number; s++) {
		for (int i = 0; !_editButton.enabled && i < [self tableView:self.tableView numberOfRowsInSection:s]; i++) {
			NSIndexPath *cellpath = [NSIndexPath indexPathForRow:i inSection:s];
			if ([cellpath isEqual:path]) {
				_editButton.enabled = (textField.text.length > 0);
			} else {
				UIContactDetailsCell *cell =
					(UIContactDetailsCell *)[self tableView:self.tableView cellForRowAtIndexPath:cellpath];
				_editButton.enabled = (cell.editTextfield.text.length > 0);
			}
		}
	}
#else
	[self textFieldUpdated:textField];
#endif
	return YES;
}

@end
