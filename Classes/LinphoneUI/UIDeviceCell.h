//
//  UIDeviceCell.h
//  linphone
//
//  Created by Danmei Chen on 07/11/2018.
//

#import <UIKit/UIKit.h>

@interface UIDeviceCell : UITableViewCell

@property (weak, nonatomic) IBOutlet UILabel *deviceLabel;
@property (weak, nonatomic) IBOutlet UIImageView *securityImage;
@property LinphoneParticipantDevice *device;
@property BOOL isOneToOne;

- (id)initWithIdentifier:(NSString *)identifier;
- (void)update;
@end
