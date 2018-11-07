//
//  UIDevicesDetails.m
//  linphone
//
//  Created by Danmei Chen on 06/11/2018.
//

#import "UIDevicesDetails.h"

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
    
        UITableViewCell *cell = [[UITableViewCell alloc] init];
        LinphoneParticipantDevice *device = (LinphoneParticipantDevice *)bctbx_list_nth_data(_devices, (int)[indexPath row]);
            
        cell.textLabel.text = [NSString stringWithUTF8String:linphone_address_as_string_uri_only(linphone_participant_device_get_address(device))];

        return cell;
}

- (void)tableView:(UITableView *)tableView willDisplayCell:(UITableViewCell *)cell forRowAtIndexPath:(NSIndexPath *)indexPath

{
    cell.backgroundColor = [UIColor colorWithRed:(245 / 255.0) green:(245 / 255.0) blue:(245 / 255.0) alpha:1.0];
    
}


@end
