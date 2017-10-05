//
//  ChatConversationCreateConfirmView.m
//  linphone
//
//  Created by REIS Benjamin on 04/10/2017.
//

#import "ChatConversationCreateConfirmView.h"
#import "PhoneMainView.h"

@implementation ChatConversationCreateConfirmView

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:TabBarView.class
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:NO
														   fragmentWith:ChatsListView.class];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	_validateButton.enabled = FALSE;
}

- (IBAction)onBackClick:(id)sender {
	[PhoneMainView.instance popToView:ChatConversationCreateView.compositeViewDescription];
}

- (IBAction)onValidateClick:(id)sender {
}
@end
