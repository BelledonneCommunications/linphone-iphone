//
//  UIDeviceCell.m
//  linphone
//
//  Created by Danmei Chen on 07/11/2018.
//

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
    [_securityButton setImage:[FastAddressBook imageForSecurityLevel:linphone_participant_device_get_security_level(_device)] forState:UIControlStateNormal];
    
    _deviceLabel.text = [NSString stringWithUTF8String:linphone_participant_device_get_name(_device) ? :
                         linphone_address_as_string_uri_only(linphone_participant_device_get_address(_device))];
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
    if (addr)
        [LinphoneManager.instance doCallWithSas:addr isSas:TRUE];
    else
        LOGE(@"CallKit : No call address");
}

@end
