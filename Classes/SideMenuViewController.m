//
//  SideMenuViewController.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 28/07/15.
//
//

#import "SideMenuViewController.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation SideMenuViewController

#pragma mark - Lifecycle Functions

- (id)init {
	return [super initWithNibName:@"SideMenuViewController" bundle:[NSBundle mainBundle]];
}

- (void)updateHeader {
	LinphoneProxyConfig *default_proxy = linphone_core_get_default_proxy_config([LinphoneManager getLc]);
	if (default_proxy != NULL) {
		const LinphoneAddress *addr = linphone_proxy_config_get_identity_address(default_proxy);
		[FastAddressBook setDisplayNameLabel:_nameLabel forAddress:addr];
		char *as_string = linphone_address_as_string(addr);
		_addressLabel.text = [NSString stringWithUTF8String:as_string];
		ms_free(as_string);
		[FastAddressBook getContactImage:[FastAddressBook getContactWithLinphoneAddress:addr] thumbnail:NO];
	}
}

- (void)viewWillAppear:(BOOL)animated {
	[self updateHeader];
}

- (IBAction)onLateralSwipe:(id)sender {
	[[PhoneMainView instance].mainViewController hideSideMenu:YES];
}

- (IBAction)onHeaderClick:(id)sender {
	[PhoneMainView.instance changeCurrentView:SettingsViewController.compositeViewDescription];
	[PhoneMainView.instance.mainViewController hideSideMenu:YES];
}
@end
