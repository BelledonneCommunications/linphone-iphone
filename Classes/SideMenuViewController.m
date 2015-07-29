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

- (void)viewWillAppear:(BOOL)animated {
	LinphoneProxyConfig *default_proxy = linphone_core_get_default_proxy_config([LinphoneManager getLc]);
	linphone_proxy_config_get [FastAddressBook setDisplayNameLabel:_nameLabel forAddress:@"toto replace me"];
	[FastAddressBook setDisplayNameLabel:_addressLabel forAddress:@"yolo"];
	[FastAddressBook getContactImage:nil thumbnail:NO];
}

- (IBAction)onLateralSwipe:(id)sender {
	LOGI(@"Swipe!");
	[[PhoneMainView instance].mainViewController hideSideMenu:YES];
}
@end
