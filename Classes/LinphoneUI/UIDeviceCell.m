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
		return;
	}
    [_securityButton setImage:[FastAddressBook imageForSecurityLevel:linphone_participant_device_get_security_level(_device)] forState:UIControlStateNormal];
    
	char *uri = linphone_address_as_string_uri_only(linphone_participant_device_get_address(_device));
    _deviceLabel.text = [NSString stringWithUTF8String:linphone_participant_device_get_name(_device) ? :
                         uri];
	ms_free(uri);
    if (_isOneToOne) {
        CGRect frame =_deviceLabel.frame;
        frame.origin.x = 30;
        _deviceLabel.frame = frame;
    }
    
    self.selectionStyle =UITableViewCellSelectionStyleNone;
	UITapGestureRecognizer *particpantsBarTap = [[UITapGestureRecognizer alloc] initWithTarget:self
																						action:@selector(onSecurityCallClick:)];
	particpantsBarTap.delegate = self;
	[self addGestureRecognizer:particpantsBarTap];
}

- (IBAction)onSecurityCallClick:(id)sender {
    const LinphoneAddress *addr = linphone_participant_device_get_address(_device);
	[CallManager.instance startCallWithAddr:(LinphoneAddress *)addr isSas:TRUE];
}

@end
