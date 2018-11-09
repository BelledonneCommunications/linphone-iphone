//
//  UIDevicesDetails.m
//  linphone
//
//  Created by Danmei Chen on 06/11/2018.
//

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
    if (bctbx_list_size(_devices) == 1) {
        _securityButton.hidden = FALSE;
        _dropMenuButton.hidden = TRUE;
    } else {
        UIImage *image = listOpen ? [UIImage imageNamed:@"chevron_list_open"] : [UIImage imageNamed:@"chevron_list_close"];
        [_dropMenuButton setImage:image forState:UIControlStateNormal];
    }
}

- (IBAction)onSecurityCallClick:(id)sender {
    LinphoneParticipantDevice *device = (LinphoneParticipantDevice *)bctbx_list_nth_data(_devices, 0);
    const LinphoneAddress *addr = linphone_participant_device_get_address(device);
    if (addr)
        [LinphoneManager.instance doCall:addr];
    else
        LOGE(@"CallKit : No call address");
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
