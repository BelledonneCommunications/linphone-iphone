//
//  UIAvatarPresence.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/04/16.
//
//

@interface UIAvatarPresence : UIRoundedImageView

@property(nonatomic, setter=setFriend:) LinphoneFriend *friend;
@property(nonatomic, readonly) UIImageView *presenceImage;

@end
