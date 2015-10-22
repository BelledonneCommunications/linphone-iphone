/* TabBarViewController.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "TabBarView.h"
#import "PhoneMainView.h"
#import "CAAnimation+Blocks.h"

@implementation TabBarView

static NSString *const kBounceAnimation = @"bounce";
static NSString *const kAppearAnimation = @"appear";
static NSString *const kDisappearAnimation = @"disappear";

@synthesize historyButton;
@synthesize contactsButton;
@synthesize dialerButton;
@synthesize chatButton;
@synthesize historyNotificationView;
@synthesize historyNotificationLabel;
@synthesize chatNotificationView;
@synthesize chatNotificationLabel;

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(changeViewEvent:)
												 name:kLinphoneMainViewChange
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(callUpdate:)
												 name:kLinphoneCallUpdate
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(messageReceived:)
												 name:kLinphoneMessageReceived
											   object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(settingsUpdate:)
												 name:kLinphoneSettingsUpdate
											   object:nil];
	[self update:FALSE];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)viewDidLoad {
	[super viewDidLoad];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(applicationWillEnterForeground:)
												 name:UIApplicationWillEnterForegroundNotification
											   object:nil];
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation
								duration:(NSTimeInterval)duration {
	// Force the animations
	[[self.view layer] removeAllAnimations];
	[historyNotificationView.layer setTransform:CATransform3DIdentity];
	[chatNotificationView.layer setTransform:CATransform3DIdentity];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[chatNotificationView setHidden:TRUE];
	[historyNotificationView setHidden:TRUE];
	[self update:FALSE];
}

#pragma mark - Event Functions

- (void)applicationWillEnterForeground:(NSNotification *)notif {
	// Force the animations
	[[self.view layer] removeAllAnimations];
	[historyNotificationView.layer setTransform:CATransform3DIdentity];
	[chatNotificationView.layer setTransform:CATransform3DIdentity];
	[chatNotificationView setHidden:TRUE];
	[historyNotificationView setHidden:TRUE];
	[self update:FALSE];
}

- (void)callUpdate:(NSNotification *)notif {
	// LinphoneCall *call = [[notif.userInfo objectForKey: @"call"] pointerValue];
	// LinphoneCallState state = [[notif.userInfo objectForKey: @"state"] intValue];
	[self updateMissedCall:linphone_core_get_missed_calls_count([LinphoneManager getLc]) appear:TRUE];
}

- (void)changeViewEvent:(NSNotification *)notif {
	UICompositeViewDescription *view = [notif.userInfo objectForKey:@"view"];
	if (view != nil) {
		[self updateSelectedButton:view];
	}
}

- (void)settingsUpdate:(NSNotification *)notif {
	if ([[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == false) {
		[self stopBounceAnimation:kBounceAnimation target:chatNotificationView];
		chatNotificationView.layer.transform = CATransform3DIdentity;
		[self stopBounceAnimation:kBounceAnimation target:historyNotificationView];
		historyNotificationView.layer.transform = CATransform3DIdentity;
	} else {
		if (![chatNotificationView isHidden] && [chatNotificationView.layer animationForKey:kBounceAnimation] == nil) {
			[self startBounceAnimation:kBounceAnimation target:chatNotificationView];
		}
		if (![historyNotificationView isHidden] &&
			[historyNotificationView.layer animationForKey:kBounceAnimation] == nil) {
			[self startBounceAnimation:kBounceAnimation target:historyNotificationView];
		}
	}
}

- (void)messageReceived:(NSNotification *)notif {
	[self updateUnreadMessage:TRUE];
}

#pragma mark - UI Update

- (void)update:(BOOL)appear {
	[self updateSelectedButton:[PhoneMainView.instance firstView]];
	[self updateMissedCall:linphone_core_get_missed_calls_count([LinphoneManager getLc]) appear:appear];
	[self updateUnreadMessage:appear];
}

- (void)updateUnreadMessage:(BOOL)appear {
	int unreadMessage = [LinphoneManager unreadMessageCount];
	if (unreadMessage > 0) {
		if ([chatNotificationView isHidden]) {
			[chatNotificationView setHidden:FALSE];
			if ([[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == true) {
				if (appear) {
					[self appearAnimation:kAppearAnimation
								   target:chatNotificationView
							   completion:^(BOOL finished) {
								 [self startBounceAnimation:kBounceAnimation target:chatNotificationView];
								 [chatNotificationView.layer removeAnimationForKey:kAppearAnimation];
							   }];
				} else {
					[self startBounceAnimation:kBounceAnimation target:chatNotificationView];
				}
			}
		}
		[chatNotificationLabel setText:[NSString stringWithFormat:@"%i", unreadMessage]];
	} else {
		if (![chatNotificationView isHidden]) {
			[self stopBounceAnimation:kBounceAnimation target:chatNotificationView];
			if (appear) {
				[self disappearAnimation:kDisappearAnimation
								  target:chatNotificationView
							  completion:^(BOOL finished) {
								[chatNotificationView setHidden:TRUE];
								[chatNotificationView.layer removeAnimationForKey:kDisappearAnimation];
							  }];
			} else {
				[chatNotificationView setHidden:TRUE];
			}
		}
	}
}

- (void)updateMissedCall:(int)missedCall appear:(BOOL)appear {
	if (missedCall > 0) {
		if ([historyNotificationView isHidden]) {
			[historyNotificationView setHidden:FALSE];
			if ([[LinphoneManager instance] lpConfigBoolForKey:@"animations_preference"] == true) {
				if (appear) {
					[self appearAnimation:kAppearAnimation
								   target:historyNotificationView
							   completion:^(BOOL finished) {
								 [self startBounceAnimation:kBounceAnimation target:historyNotificationView];
								 [historyNotificationView.layer removeAnimationForKey:kAppearAnimation];
							   }];
				} else {
					[self startBounceAnimation:kBounceAnimation target:historyNotificationView];
				}
			}
		}
		[historyNotificationLabel setText:[NSString stringWithFormat:@"%i", missedCall]];
	} else {
		if (![historyNotificationView isHidden]) {
			[self stopBounceAnimation:kBounceAnimation target:historyNotificationView];
			if (appear) {
				[self disappearAnimation:kDisappearAnimation
								  target:historyNotificationView
							  completion:^(BOOL finished) {
								[historyNotificationView setHidden:TRUE];
								[historyNotificationView.layer removeAnimationForKey:kDisappearAnimation];
							  }];
			} else {
				[historyNotificationView setHidden:TRUE];
			}
		}
	}
}

- (void)updateSelectedButton:(UICompositeViewDescription *)view {
	historyButton.selected = [view equal:HistoryListView.compositeViewDescription];
	contactsButton.selected = [view equal:ContactsListView.compositeViewDescription];
	dialerButton.selected = [view equal:DialerView.compositeViewDescription];
	chatButton.selected = [view equal:ChatsListView.compositeViewDescription];
	CGRect selectedNewFrame = _selectedButtonImage.frame;
	selectedNewFrame.origin.x =
		(historyButton.selected
			 ? historyButton.frame.origin.x
			 : (contactsButton.selected
					? contactsButton.frame.origin.x
					: (dialerButton.selected
						   ? dialerButton.frame.origin.x
						   : (chatButton.selected ? chatButton.frame.origin.x
												  : -selectedNewFrame.size.width /*hide it if none is selected*/))));
	_selectedButtonImage.frame = selectedNewFrame;
}

#pragma mark - Action Functions

- (IBAction)onHistoryClick:(id)event {
	[PhoneMainView.instance changeCurrentView:HistoryListView.compositeViewDescription];
}

- (IBAction)onContactsClick:(id)event {
	[ContactSelection setAddAddress:nil];
	[ContactSelection setSipFilter:nil];
	[ContactSelection enableEmailFilter:FALSE];
	[ContactSelection setNameOrEmailFilter:nil];
	[PhoneMainView.instance changeCurrentView:ContactsListView.compositeViewDescription];
}

- (IBAction)onDialerClick:(id)event {
	[PhoneMainView.instance changeCurrentView:DialerView.compositeViewDescription];
}

- (IBAction)onSettingsClick:(id)event {
	[PhoneMainView.instance changeCurrentView:SettingsView.compositeViewDescription];
}

- (IBAction)onChatClick:(id)event {
	[PhoneMainView.instance changeCurrentView:ChatsListView.compositeViewDescription];
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

#pragma mark - TPMultiLayoutViewController Functions

- (NSDictionary *)attributesForView:(UIView *)view {
	NSMutableDictionary *attributes = [NSMutableDictionary dictionary];

	[attributes setObject:[NSValue valueWithCGRect:view.frame] forKey:@"frame"];
	[attributes setObject:[NSValue valueWithCGRect:view.bounds] forKey:@"bounds"];
	if ([view isKindOfClass:[UIButton class]]) {
		UIButton *button = (UIButton *)view;
		[LinphoneUtils buttonMultiViewAddAttributes:attributes button:button];
	}
	[attributes setObject:[NSNumber numberWithInteger:view.autoresizingMask] forKey:@"autoresizingMask"];

	return attributes;
}

- (void)applyAttributes:(NSDictionary *)attributes toView:(UIView *)view {
	view.frame = [[attributes objectForKey:@"frame"] CGRectValue];
	view.bounds = [[attributes objectForKey:@"bounds"] CGRectValue];
	if ([view isKindOfClass:[UIButton class]]) {
		UIButton *button = (UIButton *)view;
		[LinphoneUtils buttonMultiViewApplyAttributes:attributes button:button];
	}
	view.autoresizingMask = [[attributes objectForKey:@"autoresizingMask"] integerValue];
}

@end
