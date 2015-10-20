//
//  UIPausedCallCell.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 11/09/15.
//
//

#import "UIRoundedImageView.h"
#import "LinphoneManager.h"
#import "UIPauseButton.h"

@interface UICallPausedCell : UITableViewCell

@property(weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property(weak, nonatomic) IBOutlet UILabel *nameLabel;
@property(weak, nonatomic) IBOutlet UILabel *durationLabel;
@property(weak, nonatomic) IBOutlet UIPauseButton *pauseButton;

- (id)initWithIdentifier:(NSString *)identifier;
- (void)setCall:(LinphoneCall *)call;

@end
