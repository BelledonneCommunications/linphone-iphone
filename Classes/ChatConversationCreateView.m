//
//  ChatConversationCreateViewViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 12/10/15.
//
//

#import "ChatConversationCreateView.h"
#import "PhoneMainView.h"

@implementation ChatConversationCreateView

#pragma mark - UICompositeViewDelegate Functions

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

	// if we use fragments, remove back button
	if (IPAD) {
		_backButton.hidden = YES;
	}
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
								   initWithTarget:self
								   action:@selector(dismissKeyboards)];
	tap.delegate = self;
	[self.view addGestureRecognizer:tap];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[_tableController.tableView reloadData];
}

#pragma mark - searchBar delegate

- (IBAction)onBackClick:(id)sender {
	[PhoneMainView.instance popCurrentView];
}

- (void)dismissKeyboards {
	if ([self.tableController.searchBar isFirstResponder]) {
		[self.tableController.searchBar resignFirstResponder];
	}
}

#pragma mark - GestureRecognizerDelegate

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldReceiveTouch:(UITouch *)touch
{
	if (![self.tableController.searchBar isFirstResponder]) {
		return NO;
	}
	return YES;
}

@end
