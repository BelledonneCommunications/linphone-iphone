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
#import "Utils.h"
#import "FastAddressBook.h"
#import "UILabel+Boldify.h"

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

		// Sections are wider on iPad and overlap linphone image - let's move it a bit
		if (LinphoneManager.runningOnIpad) {
			CGRect frame = _linphoneImage.frame;
			frame.origin.x -= frame.size.width / 2;
			_linphoneImage.frame = frame;
		}
	}
	return self;
}

#pragma mark - Property Functions

- (void)setContact:(ABRecordRef)acontact {
	_contact = acontact;
	[ContactDisplay setDisplayNameLabel:_nameLabel forContact:_contact];
	_linphoneImage.hidden = !([FastAddressBook contactHasValidSipDomain:_contact]);
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
