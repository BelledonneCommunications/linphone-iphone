//
//  UIChatCreateCellViewTableViewCell.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/10/15.
//
//

#import <UIKit/UIKit.h>

@interface UIChatCreateCell : UITableViewCell
@property(weak, nonatomic) IBOutlet UILabel *displayNameLabel;
@property(weak, nonatomic) IBOutlet UILabel *addressLabel;
@property (weak, nonatomic) IBOutlet UIImageView *selectedImage;
@property (weak, nonatomic) IBOutlet UIImageView *linphoneImage;
@property (weak, nonatomic) IBOutlet UIRoundedImageView *avatarImage;
@property (weak, nonatomic) IBOutlet UIImageView *securityImage;
@property (weak, nonatomic) IBOutlet UIView *greyView;

- (id)initWithIdentifier:(NSString *)identifier;

@end
