//
//  UIBackToCallButton.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 26/10/15.
//
//

#import "UIBackToCallButton.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation UIBackToCallButton

- (instancetype)init {
	if (self = [super init]) {
		[[NSNotificationCenter defaultCenter] addObserver:self
												 selector:@selector(callUpdateEvent:)
													 name:kLinphoneCallUpdate
												   object:nil];
	}
	return self;
}

- (void)dealloc {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

- (void)callUpdateEvent:(NSNotification *)notif {
	[self update];
}

- (void)update {
	self.hidden |= (linphone_core_get_current_call([LinphoneManager getLc]) == NULL);
}

- (IBAction)onBackToCallClick:(id)sender {
	CallView *view = VIEW(CallView);
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

@end
