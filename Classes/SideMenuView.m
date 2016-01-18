//
//  SideMenuViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/07/15.
//
//

#import "SideMenuView.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation SideMenuView

- (void)viewDidLoad {
	[super viewDidLoad];
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 70000
	// it's better to detect only pan from screen edges
	UIScreenEdgePanGestureRecognizer *pan =
		[[UIScreenEdgePanGestureRecognizer alloc] initWithTarget:self action:@selector(onLateralSwipe:)];
	pan.edges = UIRectEdgeRight;
	[self.view addGestureRecognizer:pan];
	_swipeGestureRecognizer.enabled = NO;
#endif
}
- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(registrationUpdateEvent:)
											   name:kLinphoneRegistrationUpdate
											 object:nil];

	[self updateHeader];
	[_sideMenuTableViewController.tableView reloadData];
}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	_grayBackground.hidden = NO;
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	_grayBackground.hidden = YES;
	// should be better than that with alpha animation..
}

- (void)updateHeader {
	LinphoneProxyConfig *default_proxy = linphone_core_get_default_proxy_config(LC);

	if (default_proxy != NULL) {
		const LinphoneAddress *addr = linphone_proxy_config_get_identity_address(default_proxy);
		[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];
		char *as_string = linphone_address_as_string_uri_only(addr);
		[_addressButton setTitle:[NSString stringWithUTF8String:as_string] forState:UIControlStateNormal];
		ms_free(as_string);
		[_addressButton setImage:[StatusBarView imageForState:linphone_proxy_config_get_state(default_proxy)]
						forState:UIControlStateNormal];
	} else {
		_nameLabel.text = @"No account";
		[_addressButton setTitle:NSLocalizedString(@"No address", nil) forState:UIControlStateNormal];
		[_addressButton setImage:nil forState:UIControlStateNormal];
	}
	_avatarImage.image = [LinphoneUtils selfAvatar];
}

- (void)onLateralSwipe:(UIScreenEdgePanGestureRecognizer *)pan {
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}

- (IBAction)onHeaderClick:(id)sender {
	[PhoneMainView.instance changeCurrentView:SettingsView.compositeViewDescription];
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}

- (IBAction)onAvatarClick:(id)sender {
	// hide ourself because we are on top of image picker
	if (!IPAD) {
		[PhoneMainView.instance.mainViewController hideSideMenu:YES];
	}
	[ImagePickerView SelectImageFromDevice:self atPosition:_avatarImage inView:self.view];
}

- (IBAction)onBackgroundClicked:(id)sender {
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}

- (void)registrationUpdateEvent:(NSNotification *)notif {
	[self updateHeader];
	[_sideMenuTableViewController.tableView reloadData];
}

#pragma mark - Image picker delegate

- (void)imagePickerDelegateImage:(UIImage *)image info:(NSDictionary *)info {
	// Dismiss popover on iPad
	if (IPAD) {
		[VIEW(ImagePickerView).popoverController dismissPopoverAnimated:TRUE];
	} else {
		[PhoneMainView.instance.mainViewController hideSideMenu:NO];
	}

	NSURL *url = [info valueForKey:UIImagePickerControllerReferenceURL];
	[LinphoneManager.instance lpConfigSetString:url.absoluteString forKey:@"avatar"];
	_avatarImage.image = [LinphoneUtils selfAvatar];
}

@end
