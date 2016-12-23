/* UIContactCell.m
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
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "UIContactCell.h"
#import "ContactsListTableView.h"
#import "FastAddressBook.h"
#import "PhoneMainView.h"
#import "UILabel+Boldify.h"
#import "Utils.h"

@implementation UIContactCell

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
		_contact = NULL;
		// Sections are wider on iPad and overlap linphone image - let's move it a bit
		if (IPAD) {
			CGRect frame = _linphoneImage.frame;
			frame.origin.x -= frame.size.width / 2;
			_linphoneImage.frame = frame;
		}

		[NSNotificationCenter.defaultCenter addObserver:self
											   selector:@selector(onPresenceChanged:)
												   name:kLinphoneNotifyPresenceReceivedForUriOrTel
												 object:nil];
	}
	return self;
}

- (void)dealloc {
	self.contact = NULL;
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - Notif

- (void)onPresenceChanged:(NSNotification *)k {
	LinphoneFriend *f = [[k.userInfo valueForKey:@"friend"] pointerValue];
	// only consider event if it's about us
	if (_contact && _nameLabel.text == PhoneMainView.instance.currentName) {
		if (!_contact.friend || f != _contact.friend) {
			return;
		}
		[self setContact:_contact];
	}
}

#pragma mark - Property Functions

- (void)setContact:(Contact *)acontact {
	_contact = acontact;
	_linphoneImage.hidden = TRUE;
	if(_contact) {
		[ContactDisplay setDisplayNameLabel:_nameLabel forContact:_contact];
		_linphoneImage.hidden =
			! ((_contact.friend && linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(_contact.friend)) == LinphonePresenceBasicStatusOpen) || [FastAddressBook contactHasValidSipDomain:_contact]);
	}
}

#pragma mark -

- (void)touchUp:(id)sender {
	[self setHighlighted:true animated:true];
}

- (void)touchDown:(id)sender {
	[self setHighlighted:false animated:true];
}

- (NSString *)accessibilityLabel {
	return _nameLabel.text;
}

- (void)setEditing:(BOOL)editing {
	[self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	if (animated) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.3];
	}
	_linphoneImage.alpha = editing ? 0 : 1;
	if (animated) {
		[UIView commitAnimations];
	}
}

@end
