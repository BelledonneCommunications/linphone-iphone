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

#import "UIDeviceCell.h"

@implementation UIDeviceCell
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

- (void)update {
	if (!_device) {
		LOGE(@"Can not update, because the device is null.");
		_securityButton.hidden = FALSE;
		_dropMenuButton.hidden = TRUE;
		return;
	}
	
	if (_isFirst) {
		_securityImage.hidden = _avatarImage.hidden = FALSE;
		if (@available(iOS 13, *)) {
			self.backgroundColor = [UIColor systemBackgroundColor];
		} else {
			self.backgroundColor = [UIColor whiteColor];
		}
	} else {
		_securityImage.hidden = _avatarImage.hidden = TRUE;
		char *uri = linphone_address_as_string_uri_only(linphone_participant_device_get_address(_device));
		_deviceLabel.text = [NSString stringWithUTF8String:linphone_participant_device_get_name(_device) ? :
							 uri];
		ms_free(uri);
		self.backgroundColor = [UIColor colorWithRed:(245 / 255.0) green:(245 / 255.0) blue:(245 / 255.0) alpha:1.0];
	}
	if (_isUnique || !_isFirst) {
		[_securityButton setImage:[FastAddressBook imageForSecurityLevel:linphone_participant_device_get_security_level(_device)] forState:UIControlStateNormal];
		_securityButton.hidden = FALSE;
		_dropMenuButton.hidden = TRUE;
	} else {
		UIImage *image = _isListOpen ? [UIImage imageNamed:@"chevron_list_open"] : [UIImage imageNamed:@"chevron_list_close"];
		[_dropMenuButton setImage:image forState:UIControlStateNormal];
		_securityButton.hidden = TRUE;
		_dropMenuButton.hidden = FALSE;
	}
}

@end
