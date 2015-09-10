//
//  SideMenuViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/07/15.
//
//

#import <AssetsLibrary/ALAsset.h>

#import "SideMenuView.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation SideMenuView

#pragma mark - Lifecycle Functions

- (id)init {
	return [super initWithNibName:NSStringFromClass(self.class) bundle:[NSBundle mainBundle]];
}

- (void)updateHeader {
	LinphoneProxyConfig *default_proxy = linphone_core_get_default_proxy_config([LinphoneManager getLc]);
	if (default_proxy != NULL) {
		const LinphoneAddress *addr = linphone_proxy_config_get_identity_address(default_proxy);
		[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];
		char *as_string = linphone_address_as_string(addr);
		_addressLabel.text = [NSString stringWithUTF8String:as_string];
		ms_free(as_string);
		[FastAddressBook getContactImage:[FastAddressBook getContactWithLinphoneAddress:addr] thumbnail:NO];
	}
	NSURL *url = [NSURL URLWithString:[LinphoneManager.instance lpConfigStringForKey:@"avatar"]];
	if (url) {
		[LinphoneManager.instance.photoLibrary assetForURL:url
			resultBlock:^(ALAsset *asset) {
			  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL), ^(void) {
				UIImage *decodedImage = [[UIImage alloc] initWithCGImage:[asset thumbnail]];
				dispatch_async(dispatch_get_main_queue(), ^{
				  [_avatarImage setImage:decodedImage];
				});
			  });
			}
			failureBlock:^(NSError *error) {
			  LOGE(@"Can't read image");
			}];
	} else {
		[_avatarImage setImage:[UIImage imageNamed:@"avatar"]];
	}
}

- (void)viewWillAppear:(BOOL)animated {
	[self updateHeader];
	[_sideMenuTableViewController.tableView reloadData];
}

- (IBAction)onLateralSwipe:(id)sender {
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}

- (IBAction)onHeaderClick:(id)sender {
	[PhoneMainView.instance changeCurrentView:SettingsView.compositeViewDescription];
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}

- (IBAction)onAvatarClick:(id)sender {
	// hide ourself because we are on top of image picker
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
	[ImagePickerView SelectImageFromDevice:self atPosition:CGRectNull inView:nil];
}

#pragma mark - Image picker delegate

- (void)imagePickerDelegateImage:(UIImage *)image info:(NSDictionary *)info {
	NSURL *url = [info valueForKey:UIImagePickerControllerReferenceURL];
	[LinphoneManager.instance lpConfigSetString:url.absoluteString forKey:@"avatar"];
	[PhoneMainView.instance.mainViewController hideSideMenu:NO];
}

@end
