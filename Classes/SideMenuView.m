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

#pragma deploymate push "ignored-api-availability"
	if (UIDevice.currentDevice.systemVersion.doubleValue >= 7) {
		// it's better to detect only pan from screen edges
		UIScreenEdgePanGestureRecognizer *pan =
			[[UIScreenEdgePanGestureRecognizer alloc] initWithTarget:self action:@selector(onLateralSwipe:)];
		pan.edges = UIRectEdgeRight;
		[self.view addGestureRecognizer:pan];
		_swipeGestureRecognizer.enabled = NO;
	}
#pragma deploymate pop
}
- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	[_sideMenuTableViewController viewWillAppear:animated];
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
		_addressLabel.text = [NSString stringWithUTF8String:linphone_proxy_config_get_identity(default_proxy)];
		_presenceImage.image = [StatusBarView imageForState:linphone_proxy_config_get_state(default_proxy)];
	} else {
		_nameLabel.text = linphone_core_get_proxy_config_list(LC) ? NSLocalizedString(@"No default account", nil) : NSLocalizedString(@"No account", nil);
		// display direct IP:port address so that we can be reached
		LinphoneAddress *addr = linphone_core_get_primary_contact_parsed(LC);
		if (addr) {
			char *as_string = linphone_address_as_string(addr);
			_addressLabel.text = [NSString stringWithFormat:@"%s", as_string];
			ms_free(as_string);
			linphone_address_destroy(addr);
		} else {
			_addressLabel.text = NSLocalizedString(@"No address", nil);
		}
		_presenceImage.image = nil;
	}
	_avatarImage.image = [LinphoneUtils selfAvatar];
}

#pragma deploymate push "ignored-api-availability"
- (void)onLateralSwipe:(UIScreenEdgePanGestureRecognizer *)pan {
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}
#pragma deploymate pop

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
	// When getting image from the camera, it may be 90° rotated due to orientation
	// (image.imageOrientation = UIImageOrientationRight). Just rotate it to be face up.
	if (image.imageOrientation != UIImageOrientationUp) {
		UIGraphicsBeginImageContextWithOptions(image.size, false, image.scale);
		[image drawInRect:CGRectMake(0, 0, image.size.width, image.size.height)];
		image = UIGraphicsGetImageFromCurrentImageContext();
		UIGraphicsEndImageContext();
	}

	// Dismiss popover on iPad
	if (IPAD) {
		[VIEW(ImagePickerView).popoverController dismissPopoverAnimated:TRUE];
	} else {
		[PhoneMainView.instance.mainViewController hideSideMenu:NO];
	}

	NSURL *url = [info valueForKey:UIImagePickerControllerReferenceURL];

	// taken from camera, must be saved to device first
	if (!url) {
		[LinphoneManager.instance.photoLibrary
			writeImageToSavedPhotosAlbum:image.CGImage
							 orientation:(ALAssetOrientation)[image imageOrientation]
						 completionBlock:^(NSURL *assetURL, NSError *error) {
						   if (error) {
							   LOGE(@"Cannot save image data downloaded [%@]", [error localizedDescription]);
						   } else {
							   LOGI(@"Image saved to [%@]", [assetURL absoluteString]);
						   }
						   [LinphoneManager.instance lpConfigSetString:assetURL.absoluteString forKey:@"avatar"];
						   _avatarImage.image = [LinphoneUtils selfAvatar];
						 }];
	} else {
		[LinphoneManager.instance lpConfigSetString:url.absoluteString forKey:@"avatar"];
		_avatarImage.image = [LinphoneUtils selfAvatar];
	}
}

@end
