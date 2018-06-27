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

- (void)layoutSubviews {
    [super layoutSubviews];
    CGRect bubbleFrame = self.contentView.frame;
    int origin_x;
        
    bubbleFrame.size = CGSizeMake(_width, _height);

    origin_x = (_isOutgoing ? self.frame.size.width - bubbleFrame.size.width - 5 : 5);
        
    bubbleFrame.origin.x = origin_x;
    self.contentView.frame = bubbleFrame;
    
    _msgText.textContainerInset = UIEdgeInsetsZero;
    _msgText.textContainer.lineFragmentPadding = 0;
}

@end
