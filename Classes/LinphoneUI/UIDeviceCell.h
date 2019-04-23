//
//  UIDeviceCell.h
//  linphone
//
//  Created by Danmei Chen on 07/11/2018.
//

#import <UIKit/UIKit.h>

@interface UIDeviceCell : UITableViewCell <UIGestureRecognizerDelegate>

@property (weak, nonatomic) IBOutlet UILabel *deviceLabel;
@property (weak, nonatomic) IBOutlet UIButton *securityButton;
@property LinphoneParticipantDevice *device;
@property BOOL isOneToOne;

- (IBAction)onSecurityCallClick:(id)sender;
- (id)initWithIdentifier:(NSString *)identifier;
- (void)update;
@end
