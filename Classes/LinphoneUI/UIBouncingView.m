//
//  UIBouncingView.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 11/12/15.
//
//

#import "UIBouncingView.h"

#import "CAAnimation+Blocks.h"
#import "Utils.h"

static NSString *const kBounceAnimation = @"bounce";
static NSString *const kAppearAnimation = @"appear";
static NSString *const kDisappearAnimation = @"disappear";

@implementation UIBouncingView

INIT_WITH_COMMON {
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(settingsUpdate:)
												 name:kLinphoneSettingsUpdate
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(applicationWillEnterForeground:)
												 name:UIApplicationWillEnterForegroundNotification
											   object:nil];
	return self;
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)settingsUpdate:(NSNotification *)notif {
	if ([[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == false) {
		[self stopBounceAnimation:kBounceAnimation target:self];
	} else {
		if (![self isHidden] && [self.layer animationForKey:kBounceAnimation] == nil) {
			[self startBounceAnimation:kBounceAnimation target:self];
		}
	}
}

- (void)applicationWillEnterForeground:(NSNotification *)notif {
	// Force the animations
	if (!self.isHidden) {
		[self startAnimating:NO];
	} else {
		[self stopAnimating:NO];
	}
}

#pragma mark - Animation

- (void)appearAnimation:(NSString *)animationID target:(UIView *)target completion:(void (^)(BOOL finished))completion {
	CABasicAnimation *appear = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
	appear.duration = 0.4;
	appear.fromValue = [NSNumber numberWithDouble:0.0f];
	appear.toValue = [NSNumber numberWithDouble:1.0f];
	appear.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
	appear.fillMode = kCAFillModeForwards;
	appear.removedOnCompletion = NO;
	[appear setCompletion:completion];
	[target.layer addAnimation:appear forKey:animationID];
}

- (void)disappearAnimation:(NSString *)animationID
					target:(UIView *)target
				completion:(void (^)(BOOL finished))completion {
	CABasicAnimation *disappear = [CABasicAnimation animationWithKeyPath:@"transform.scale"];
	disappear.duration = 0.4;
	disappear.fromValue = [NSNumber numberWithDouble:1.0f];
	disappear.toValue = [NSNumber numberWithDouble:0.0f];
	disappear.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseOut];
	disappear.fillMode = kCAFillModeForwards;
	disappear.removedOnCompletion = NO;
	[disappear setCompletion:completion];
	[target.layer addAnimation:disappear forKey:animationID];
}

- (void)startBounceAnimation:(NSString *)animationID target:(UIView *)target {
	CABasicAnimation *bounce = [CABasicAnimation animationWithKeyPath:@"transform.translation.y"];
	bounce.duration = 0.3;
	bounce.fromValue = [NSNumber numberWithDouble:0.0f];
	bounce.toValue = [NSNumber numberWithDouble:8.0f];
	bounce.timingFunction = [CAMediaTimingFunction functionWithName:kCAMediaTimingFunctionEaseIn];
	bounce.autoreverses = TRUE;
	bounce.repeatCount = HUGE_VALF;
	[target.layer addAnimation:bounce forKey:animationID];
}

- (void)stopBounceAnimation:(NSString *)animationID target:(UIView *)target {
	[target.layer removeAnimationForKey:animationID];
}

- (void)startAnimating:(BOOL)animated {
	if (!self.hidden) {
		return;
	}

	[self setHidden:FALSE];
	if ([[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == true) {
		if (animated) {
			[self appearAnimation:kAppearAnimation
						   target:self
					   completion:^(BOOL finished) {
						 [self startBounceAnimation:kBounceAnimation target:self];
						 if (finished) {
							 [self.layer removeAnimationForKey:kAppearAnimation];
						 }
					   }];
		} else {
			[self startBounceAnimation:kBounceAnimation target:self];
		}
	}
}

- (void)stopAnimating:(BOOL)animated {
	if (self.hidden) {
		return;
	}

	[self stopBounceAnimation:kBounceAnimation target:self];
	if (animated) {
		[self disappearAnimation:kDisappearAnimation
						  target:self
					  completion:^(BOOL finished) {
						[self setHidden:TRUE];
						if (finished) {
							[self.layer removeAnimationForKey:kDisappearAnimation];
						}
					  }];
	} else {
		[self setHidden:TRUE];
	}
}
@end
