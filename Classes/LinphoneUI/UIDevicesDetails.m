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

#import "UIDevicesDetails.h"
#import "UIDeviceCell.h"

@implementation UIDevicesDetails
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
        _devicesTable.dataSource = self;
        _devicesTable.delegate    = self;
    }
    return self;
}

- (void)update:(BOOL)listOpen {
	_devices = linphone_participant_get_devices(_participant);

    UIImage *image = [FastAddressBook imageForSecurityLevel:linphone_participant_get_security_level(_participant)];
    if (bctbx_list_size(_devices) == 1) {
        [_securityButton setImage:image forState:UIControlStateNormal];
        _securityButton.hidden = FALSE;
        _dropMenuButton.hidden = TRUE;
		UITapGestureRecognizer *particpantsBarTap = [[UITapGestureRecognizer alloc] initWithTarget:self
																							action:@selector(onSecurityCallClick:)];
		particpantsBarTap.delegate = self;
		[self addGestureRecognizer:particpantsBarTap];
    } else {
        UIImage *image = listOpen ? [UIImage imageNamed:@"chevron_list_open"] : [UIImage imageNamed:@"chevron_list_close"];
        [_dropMenuButton setImage:image forState:UIControlStateNormal];
    }
    [_securityImage setImage:image];
}

- (IBAction)onSecurityCallClick:(id)sender {
    LinphoneParticipantDevice *device = (LinphoneParticipantDevice *)bctbx_list_nth_data(_devices, 0);
    const LinphoneAddress *addr = linphone_participant_device_get_address(device);
	[CallManager.instance startCallWithAddr:(LinphoneAddress *)addr isSas:TRUE];
}

#pragma mark - TableView

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    return bctbx_list_size(_devices);
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
    return 56.0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    
    NSString *kCellId = NSStringFromClass(UIDeviceCell.class);
    UIDeviceCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    
    if (cell == nil) {
        cell = [[UIDeviceCell alloc] initWithIdentifier:kCellId];
    }
    LinphoneParticipantDevice *device = (LinphoneParticipantDevice *)bctbx_list_nth_data(_devices, (int)[indexPath row]);
    cell.device = device;
    cell.isOneToOne = FALSE;
    [cell update];

    return cell;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath

{
    cell.backgroundColor = [UIColor colorWithRed:(245 / 255.0) green:(245 / 255.0) blue:(245 / 255.0) alpha:1.0];
    
}

@end
