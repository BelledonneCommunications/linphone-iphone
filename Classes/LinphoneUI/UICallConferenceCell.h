//
//  UIPausedCallCell.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 11/09/15.
//
//

#import "UIRoundedImageView.h"
#import "LinphoneManager.h"

@interface UICallConferenceCell : UITableViewCell

@property(weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property(weak, nonatomic) IBOutlet UILabel *nameLabel;
@property(weak, nonatomic) IBOutlet UILabel *durationLabel;
@property(nonatomic, setter=setCall:) LinphoneCall *call;

- (id)initWithIdentifier:(NSString *)identifier;
- (IBAction)onKickClick:(id)sender;

@end
