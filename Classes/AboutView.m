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

#import "PhoneMainView.h"
#import "LinphoneManager.h"
#import "LinphoneIOSVersion.h"

@implementation AboutView

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;
+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:nil
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:YES
														   fragmentWith:nil];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];
	NSString *name = [NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleDisplayName"];
	_nameLabel.text = name;
	NSDictionary *infoDict = [[NSBundle mainBundle] infoDictionary];
    NSString *curVersion = [NSString stringWithFormat:@"version %@",[infoDict objectForKey:@"CFBundleShortVersionString"]];
	_appVersionLabel.text = [NSString stringWithFormat:@"%@ iOS %@", name, curVersion];
	_libVersionLabel.text = [NSString stringWithFormat:@"%@ SDK %s", name, LINPHONE_SDK_VERSION];
	UITapGestureRecognizer *tapGestureRecognizer =
		[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onLicenceTap)];
	tapGestureRecognizer.numberOfTapsRequired = 1;
	[_licenceLabel addGestureRecognizer:tapGestureRecognizer];
	_licenceLabel.userInteractionEnabled = YES;
	UITapGestureRecognizer *tapGestureRecognizerPolicy =
		[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onPolicyTap)];
	tapGestureRecognizerPolicy.numberOfTapsRequired = 1;
	[_policyLabel addGestureRecognizer:tapGestureRecognizerPolicy];
	_policyLabel.userInteractionEnabled = YES;
}

#pragma mark - Action Functions

- (IBAction)onLinkTap:(id)sender {
	UIGestureRecognizer *gest = sender;
	NSString *url = ((UILabel *)gest.view).text;
	if (![UIApplication.sharedApplication openURL:[NSURL URLWithString:url]]) {
		LOGE(@"Failed to open %@, invalid URL", url);
	}
}

- (IBAction)onPolicyTap {
	NSString *url = @"https://www.linphone.org/terms-and-privacy";
	if (![UIApplication.sharedApplication openURL:[NSURL URLWithString:url]]) {
		LOGE(@"Failed to open %@, invalid URL", url);
	}
}

- (IBAction)onLicenceTap {
	NSString *url = @"https://www.gnu.org/licenses/gpl-3.0.html";
	if (![UIApplication.sharedApplication openURL:[NSURL URLWithString:url]]) {
		LOGE(@"Failed to open %@, invalid URL", url);
	}
}

- (IBAction)onDialerBackClick:(id)sender {
	[PhoneMainView.instance popCurrentView];
}
@end
