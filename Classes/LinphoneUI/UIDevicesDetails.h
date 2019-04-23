//
//  UIDevicesDetails.h
//  linphone
//
//  Created by Danmei Chen on 06/11/2018.
//

#import <UIKit/UIKit.h>

@interface UIDevicesDetails : UITableViewCell <UITableViewDelegate, UITableViewDataSource, UIGestureRecognizerDelegate>

@property (weak, nonatomic) IBOutlet UIButton *dropMenuButton;
@property (weak, nonatomic) IBOutlet UILabel *addressLabel;
@property (weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property (weak, nonatomic) IBOutlet UIImageView *securityImage;
@property (weak, nonatomic) IBOutlet UIButton *securityButton;
@property (weak, nonatomic) IBOutlet UITableView *devicesTable;
@property bctbx_list_t *devices;
@property LinphoneParticipant *participant;

- (IBAction)onSecurityCallClick:(id)sender;
- (id)initWithIdentifier:(NSString *)identifier;
- (void)update:(BOOL)listOpen;
@end
