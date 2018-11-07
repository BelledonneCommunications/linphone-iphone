//
//  UIDevicesDetails.h
//  linphone
//
//  Created by Danmei Chen on 06/11/2018.
//

#import <UIKit/UIKit.h>

@interface UIDevicesDetails : UITableViewCell <UITableViewDelegate, UITableViewDataSource>

@property (weak, nonatomic) IBOutlet UIButton *dropMenuButton;
@property (weak, nonatomic) IBOutlet UILabel *addressLabel;
@property (weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property (weak, nonatomic) IBOutlet UIImageView *securityImage;
@property (weak, nonatomic) IBOutlet UITableView *devicesTable;
@property bctbx_list_t *devices;

- (id)initWithIdentifier:(NSString *)identifier;
@end
