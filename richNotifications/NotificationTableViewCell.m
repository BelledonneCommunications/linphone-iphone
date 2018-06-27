//
//  NotificationTableViewCell.m
//  richNotifications
//
//  Created by David Idmansour on 26/06/2018.
//

#import "NotificationTableViewCell.h"

@implementation NotificationTableViewCell

- (void)awakeFromNib {
    [super awakeFromNib];
    // Initialization code
    _contactImage.layer.cornerRadius = _contactImage.frame.size.height / 2;
    _contactImage.clipsToBounds = YES;
    [self.contentView sendSubviewToBack:_background];
}

- (void)setSelected:(BOOL)selected animated:(BOOL)animated {
    [super setSelected:selected animated:animated];

    // Configure the view for the selected state
}

@end
