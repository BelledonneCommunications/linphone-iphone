/* UIEditableTableViewCell.m
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

#import "UIContactDetailsCell.h"
#import "FastAddressBook.h"
#import "Contact.h"
#import "PhoneMainView.h"

@implementation UIContactDetailsCell

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		NSArray *arrayOfViews =
			[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];

		// resize cell to match .nib size. It is needed when resized the cell to
		// correctly adapt its height too
		UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:0]);
		[self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
		[self addSubview:sub];
	}
	return self;
}

#pragma mark - UITableViewCell Functions

- (void)setAddress:(NSString *)address {
	_addressLabel.text = _editTextfield.text = address;
	char *normAddr = (char *)_addressLabel.text.UTF8String;
	if(_addressLabel.text && linphone_proxy_config_is_phone_number(linphone_core_get_default_proxy_config(LC), _addressLabel.text.UTF8String)) {
		normAddr = linphone_proxy_config_normalize_phone_number(linphone_core_get_default_proxy_config(LC),
																_addressLabel.text.UTF8String);
	}
	LinphoneAddress *addr = linphone_core_interpret_url(LC, normAddr);
	_chatButton.enabled = _callButton.enabled = (addr != NULL);

	_chatButton.accessibilityLabel =
		[NSString stringWithFormat:NSLocalizedString(@"Chat with %@", nil), _addressLabel.text];
	_callButton.accessibilityLabel = [NSString stringWithFormat:NSLocalizedString(@"Call %@", nil), _addressLabel.text];
	// Test presence
	Contact* contact = [FastAddressBook getContactWithAddress:(addr)];

	_linphoneImage.hidden = TRUE;
	if (contact) {
		self.linphoneImage.hidden =
			!((contact.friend &&
			   linphone_presence_model_get_basic_status(linphone_friend_get_presence_model_for_uri_or_tel(
				   contact.friend, _addressLabel.text.UTF8String)) == LinphonePresenceBasicStatusOpen) ||
			  (!linphone_proxy_config_is_phone_number(linphone_core_get_default_proxy_config(LC),
													  _addressLabel.text.UTF8String) &&
			   [FastAddressBook isSipURIValid:_addressLabel.text]));
	}

	if (addr) {
		linphone_address_destroy(addr);
	}
}

- (void)shouldHideLinphoneImageOfAddress {
	if (!_addressLabel.text) {
		return;
	}

	char *normAddr = (char *)_addressLabel.text.UTF8String;
	if (linphone_proxy_config_is_phone_number(linphone_core_get_default_proxy_config(LC),
											  _addressLabel.text.UTF8String)) {
		normAddr = linphone_proxy_config_normalize_phone_number(linphone_core_get_default_proxy_config(LC),
																_addressLabel.text.UTF8String);
	}
	LinphoneAddress *addr = linphone_core_interpret_url(LC, normAddr);

	// Test presence
	Contact *contact = [FastAddressBook getContactWithAddress:(addr)];

	if (contact) {
		self.linphoneImage.hidden = ! ((contact.friend && linphone_presence_model_get_basic_status(linphone_friend_get_presence_model_for_uri_or_tel(contact.friend, _addressLabel.text.UTF8String)) == LinphonePresenceBasicStatusOpen) || (!linphone_proxy_config_is_phone_number(linphone_core_get_default_proxy_config(LC), _addressLabel.text.UTF8String) && [FastAddressBook isSipURIValid:_addressLabel.text]));
	}
	
	if (addr) {
		linphone_address_destroy(addr);
	}
}

- (void)hideDeleteButton:(BOOL)hidden {
	if (_deleteButton.hidden == hidden)
		return;

	CGRect newFrame = _editTextfield.frame;
	newFrame.size.width = _editView.frame.size.width - newFrame.origin.x;
	if (hidden) {
		newFrame.size.width -= newFrame.origin.x; /* center view in super view */
	} else {
		newFrame.size.width -= _deleteButton.frame.size.width;
	}
	_editTextfield.frame = newFrame;
	_deleteButton.hidden = hidden;
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	[super setEditing:editing animated:animated];

	_defaultView.hidden = editing;
	_editView.hidden = !editing;
}

- (void)setEditing:(BOOL)editing {
	[self setEditing:editing animated:FALSE];
}

- (IBAction)onCallClick:(id)event {
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:_addressLabel.text];
	[LinphoneManager.instance call:addr];
	if (addr)
		linphone_address_destroy(addr);
}

- (IBAction)onChatClick:(id)event {
	LinphoneAddress *addr = [LinphoneUtils normalizeSipOrPhoneAddress:_addressLabel.text];
	if (addr == NULL)
		return;
	ChatConversationView *view = VIEW(ChatConversationView);
	LinphoneChatRoom *room = linphone_core_get_chat_room(LC, addr);
	[view setChatRoom:room];
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	linphone_address_destroy(addr);
}

- (IBAction)onDeleteClick:(id)sender {
	UITableView *tableView = VIEW(ContactDetailsView).tableController.tableView;
	NSIndexPath *indexPath = [tableView indexPathForCell:self];
	[tableView.dataSource tableView:tableView
				 commitEditingStyle:UITableViewCellEditingStyleDelete
				  forRowAtIndexPath:indexPath];
}

@end
