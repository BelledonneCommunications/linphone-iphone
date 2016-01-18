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
		[NSNotificationCenter.defaultCenter addObserver:self
											   selector:@selector(callUpdateEvent:)
												   name:kLinphoneCallUpdate
												 object:nil];
	}
	return self;
}

- (void)dealloc {
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (void)callUpdateEvent:(NSNotification *)notif {
	[self update];
}

- (void)update {
	self.hidden = (_tableView.isEditing || linphone_core_get_current_call(LC) == NULL);
}

- (IBAction)onBackToCallClick:(id)sender {
	[PhoneMainView.instance popToView:CallView.compositeViewDescription];
}

@end
