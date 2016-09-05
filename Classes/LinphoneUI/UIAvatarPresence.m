//
//  UIAvatarPresence.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/04/16.
//
//

#import "UIAvatarPresence.h"

@implementation UIAvatarPresence

INIT_WITH_COMMON_CF {
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(onPresenceChanged:)
											   name:kLinphoneNotifyPresenceReceivedForUriOrTel
											 object:nil];

	if (!_presenceImage) {
		_presenceImage = [[UIImageView alloc] init];
		_presenceImage.tag = 883;
		[self addSubview:_presenceImage];
	}
	CGSize s = self.frame.size;
	int is = MIN(s.width, s.height);
	// place it in bottom right corner
	_presenceImage.frame = CGRectMake(.5 * (s.width - is) + .7 * is, .5 * (s.height - is) + .7 * is, .2 * is, .2 * is);

	_presenceImage.image = [UIImage imageNamed:@"presence_unregistered"];

	return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
};
- (void)setFrame:(CGRect)frame {
	[super setFrame:frame];

	CGSize s = self.frame.size;
	int is = MIN(s.width, s.height);
	// place it in bottom right corner
	_presenceImage.frame = CGRectMake(.5 * (s.width - is) + .7 * is, .5 * (s.height - is) + .7 * is, .2 * is, .2 * is);
}

- (void)onPresenceChanged:(NSNotification *)k {
	LinphoneFriend *f = [[k.userInfo valueForKey:@"friend"] pointerValue];
	// only consider event if it's about us
	if (!_friend || f != _friend) {
		return;
	}
	[self updatePresenceImage];
}

- (void)updatePresenceImage {
	LinphonePresenceBasicStatus basic =
		_friend ? linphone_presence_model_get_basic_status(linphone_friend_get_presence_model(_friend))
				: LinphonePresenceBasicStatusClosed;
	const LinphonePresenceModel *model = _friend ? linphone_friend_get_presence_model(_friend) : NULL;
	LinphonePresenceActivity *activity =
		model ? linphone_presence_model_get_activity(model) ?: LinphonePresenceActivityOffline : NULL;

	LOGE(@"Friend %s status is now %s/%s since %@", _friend ? linphone_friend_get_name(_friend) : "NULL",
		 basic == LinphonePresenceBasicStatusOpen ? "open" : "closed", linphone_presence_activity_to_string(activity),
		 [NSDate dateWithTimeIntervalSince1970:linphone_presence_model_get_timestamp(model)]);

	NSString *imageName;
	if (basic == LinphonePresenceBasicStatusClosed) {
		imageName =
			(_friend && linphone_friend_is_presence_received(_friend)) ? @"presence_away" : @"presence_unregistered";
	} else if (linphone_presence_activity_get_type(activity) == LinphonePresenceActivityTV) {
		imageName = @"presence_online";
	} else {
		imageName = @"presence_away";
	}
	_presenceImage.image = [UIImage imageNamed:imageName];
}

- (void)setFriend:(LinphoneFriend *) friend {
	_friend = friend;
	[self updatePresenceImage];
}

@end
