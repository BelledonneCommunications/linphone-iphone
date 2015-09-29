//
//  UIPausedCallCell.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 11/09/15.
//
//

#import "UICheckBoxTVCell.h"

#import "UIRoundedImageView.h"
#import "LinphoneManager.h"

@interface UICallPausedCell : UICheckBoxTVCell

@property(weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property(weak, nonatomic) IBOutlet UILabel *nameLabel;
@property(weak, nonatomic) IBOutlet UILabel *durationLabel;

- (id)initWithIdentifier:(NSString *)identifier;
- (void)setCall:(LinphoneCall *)call;

@end
