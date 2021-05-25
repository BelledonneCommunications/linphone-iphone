/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
	LinphoneAccount *default_account = linphone_core_get_default_account(LC);

	if (default_account != NULL) {
		const LinphoneAddress *addr = linphone_account_params_get_identity_address(linphone_account_get_params(default_account));
		[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];
		char *str = addr ? linphone_address_as_string(addr) : nil;
		_addressLabel.text = str ? [NSString stringWithUTF8String:str] : NSLocalizedString(@"No address", nil);
		if (str) ms_free(str);
	} else {
		_nameLabel.text = linphone_core_get_account_list(LC) ? NSLocalizedString(@"No default account", nil) : NSLocalizedString(@"No account", nil);
		// display direct IP:port address so that we can be reached
		LinphoneAddress *addr = linphone_core_get_primary_contact_parsed(LC);
		if (addr) {
			char *as_string = linphone_address_as_string(addr);
			_addressLabel.text = [NSString stringWithFormat:@"%s", as_string];
			ms_free(as_string);
			linphone_address_unref(addr);
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
	[ImagePickerView SelectImageFromDevice:self atPosition:_avatarImage inView:self.view withDocumentMenuDelegate:nil];
}

- (IBAction)onBackgroundClicked:(id)sender {
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}

- (void)registrationUpdateEvent:(NSNotification *)notif {
	[self updateHeader];
	[_sideMenuTableViewController.tableView reloadData];
}

#pragma mark - Image picker delegate

- (void)imagePickerDelegateImage:(UIImage *)image info:(NSString *)phAssetId {
	// When getting image from the camera, it may be 90° rotated due to orientation
	// (image.imageOrientation = UIImageOrientationRight). Just rotate it to be face up.
	if (image.imageOrientation != UIImageOrientationUp) {
		UIGraphicsBeginImageContextWithOptions(image.size, false, image.scale);
		[image drawInRect:CGRectMake(0, 0, image.size.width, image.size.height)];
		image = UIGraphicsGetImageFromCurrentImageContext();
		UIGraphicsEndImageContext();
	}
    
    [LinphoneManager.instance lpConfigSetString:phAssetId forKey:@"avatar"];
    _avatarImage.image = [LinphoneUtils selfAvatar];
    [LinphoneManager.instance loadAvatar];

	// Dismiss popover on iPad
	if (IPAD) {
		[VIEW(ImagePickerView).popoverController dismissPopoverAnimated:TRUE];
	} else {
		[PhoneMainView.instance.mainViewController hideSideMenu:NO];
	}
}

- (void)imagePickerDelegateVideo:(NSURL*)url info:(NSDictionary *)info {
	return; // Avatar video not supported (yet ;) )
}

@end
