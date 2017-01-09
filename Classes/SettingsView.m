/* SettingsViewController.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "SettingsView.h"
#import "LinphoneManager.h"
#import "LinphoneAppDelegate.h"
#import "PhoneMainView.h"
#import "Utils.h"

#import "DCRoundSwitch.h"

#import "IASKSpecifierValuesViewController.h"
#import "IASKPSTextFieldSpecifierViewCell.h"
#import "IASKPSTitleValueSpecifierViewCell.h"
#import "IASKSpecifier.h"
#import "IASKTextField.h"
#include "linphone/lpconfig.h"

#ifdef DEBUG
@interface UIDevice (debug)

- (void)_setBatteryLevel:(float)level;
- (void)_setBatteryState:(int)state;

@end
#endif

@interface SettingsView (private)

+ (IASKSpecifier *)filterSpecifier:(IASKSpecifier *)specifier;

@end

#pragma mark - IASKSwitchEx Class

@interface IASKSwitchEx : DCRoundSwitch {
	NSString *_key;
}

@property(nonatomic, strong) NSString *key;

@end

@implementation IASKSwitchEx

@synthesize key = _key;

- (void)dealloc {
	_key = nil;
}

@end

#pragma mark - IASKSpecifierValuesViewControllerEx Class

// Patch IASKSpecifierValuesViewController
@interface IASKSpecifierValuesViewControllerEx : IASKSpecifierValuesViewController

@end

@implementation IASKSpecifierValuesViewControllerEx

- (void)initIASKSpecifierValuesViewControllerEx {
	[self.view setBackgroundColor:[UIColor clearColor]];
}

- (id)init {
	self = [super init];
	if (self != nil) {
		[self initIASKSpecifierValuesViewControllerEx];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
	self = [super initWithCoder:aDecoder];
	if (self != nil) {
		[self initIASKSpecifierValuesViewControllerEx];
	}
	return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
	self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
	if (self != nil) {
		[self initIASKSpecifierValuesViewControllerEx];
	}
	return self;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell = [super tableView:tableView cellForRowAtIndexPath:indexPath];
	return cell;
}

@end

#pragma mark - IASKAppSettingsViewControllerEx Class

@interface IASKAppSettingsViewController (PrivateInterface)
- (UITableViewCell *)newCellForIdentifier:(NSString *)identifier;
@end
;

@interface IASKAppSettingsViewControllerEx : IASKAppSettingsViewController

@end

@implementation IASKAppSettingsViewControllerEx

- (UITableViewCell *)newCellForIdentifier:(NSString *)identifier {
	UITableViewCell *cell = nil;
	if ([identifier isEqualToString:kIASKPSToggleSwitchSpecifier]) {
		cell = [[UITableViewCell alloc] initWithStyle:UITableViewCellStyleDefault
									  reuseIdentifier:kIASKPSToggleSwitchSpecifier];
		cell.accessoryView = [[IASKSwitchEx alloc] initWithFrame:CGRectMake(0, 0, 79, 27)];
		[((IASKSwitchEx *)cell.accessoryView) addTarget:self
												 action:@selector(toggledValue:)
									   forControlEvents:UIControlEventValueChanged];
		[((IASKSwitchEx *)cell.accessoryView) setOnTintColor:LINPHONE_MAIN_COLOR];
		cell.selectionStyle = UITableViewCellSelectionStyleNone;
		cell.textLabel.minimumScaleFactor = kIASKMinimumFontSize / [UIFont systemFontSize];
		cell.detailTextLabel.minimumScaleFactor = kIASKMinimumFontSize / [UIFont systemFontSize];
	} else {
		cell = [super newCellForIdentifier:identifier];
	}
	return cell;
}

- (void)toggledValue:(id)sender {
	IASKSwitchEx *toggle = (IASKSwitchEx *)sender;
	IASKSpecifier *spec = [_settingsReader specifierForKey:[toggle key]];

	if ([toggle isOn]) {
		if ([spec trueValue] != nil) {
			[self.settingsStore setObject:[spec trueValue] forKey:[toggle key]];
		} else {
			[self.settingsStore setBool:YES forKey:[toggle key]];
		}
	} else {
		if ([spec falseValue] != nil) {
			[self.settingsStore setObject:[spec falseValue] forKey:[toggle key]];
		} else {
			[self.settingsStore setBool:NO forKey:[toggle key]];
		}
	}
	// Start notification after animation of DCRoundSwitch
	dispatch_async(dispatch_get_main_queue(), ^{
	  [NSNotificationCenter.defaultCenter
		  postNotificationName:kIASKAppSettingChanged
						object:[toggle key]
					  userInfo:[NSDictionary dictionaryWithObject:[self.settingsStore objectForKey:[toggle key]]
														   forKey:[toggle key]]];
	});
}

- (void)initIASKAppSettingsViewControllerEx {
	[self.view setBackgroundColor:[UIColor clearColor]];

	// Force kIASKSpecifierValuesViewControllerIndex
	static int kIASKSpecifierValuesViewControllerIndex = 0;
	_viewList = [[NSMutableArray alloc] init];
	[_viewList addObject:[NSDictionary dictionaryWithObjectsAndKeys:@"IASKSpecifierValuesView", @"ViewName", nil]];
	[_viewList addObject:[NSDictionary dictionaryWithObjectsAndKeys:@"IASKAppSettingsView", @"ViewName", nil]];

	NSMutableDictionary *newItemDict = [NSMutableDictionary dictionaryWithCapacity:3];
	[newItemDict addEntriesFromDictionary:[_viewList objectAtIndex:kIASKSpecifierValuesViewControllerIndex]]; // copy
	// the
	// title
	// and
	// explain
	// strings

	IASKSpecifierValuesViewController *targetViewController = [[IASKSpecifierValuesViewControllerEx alloc] init];
	// add the new view controller to the dictionary and then to the 'viewList' array
	[newItemDict setObject:targetViewController forKey:@"viewController"];
	[_viewList replaceObjectAtIndex:kIASKSpecifierValuesViewControllerIndex withObject:newItemDict];
}

- (IASKSettingsReader *)settingsReader {
	IASKSettingsReader *r = [super settingsReader];
	NSMutableArray *dataSource = [NSMutableArray arrayWithArray:[r dataSource]];
	for (int i = 0; i < [dataSource count]; ++i) {
		NSMutableArray *specifiers = [NSMutableArray arrayWithArray:[dataSource objectAtIndex:i]];
		for (int j = 0; j < [specifiers count]; ++j) {
			id sp = [specifiers objectAtIndex:j];
			if ([sp isKindOfClass:[IASKSpecifier class]]) {
				sp = [SettingsView filterSpecifier:sp];
			}
			[specifiers replaceObjectAtIndex:j withObject:sp];
		}

		[dataSource replaceObjectAtIndex:i withObject:specifiers];
	}
	[r setDataSource:dataSource];
	return r;
}

- (id)initWithStyle:(UITableViewStyle)style {
	self = [super initWithStyle:style];
	if (self != nil) {
		[self initIASKAppSettingsViewControllerEx];
	}
	return self;
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	UIEdgeInsets inset = {0, 0, 10, 0};
	UIScrollView *scrollView = self.tableView;
	[scrollView setContentInset:inset];
	[scrollView setScrollIndicatorInsets:inset];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	UITableViewCell *cell = [super tableView:tableView cellForRowAtIndexPath:indexPath];

	if ([cell isKindOfClass:[IASKPSTextFieldSpecifierViewCell class]]) {
		UITextField *field = ((IASKPSTextFieldSpecifierViewCell *)cell).textField;
		[field setTextColor:LINPHONE_MAIN_COLOR];
	}

	if ([cell isKindOfClass:[IASKPSTitleValueSpecifierViewCell class]]) {
		cell.detailTextLabel.textColor = [UIColor grayColor];
	} else {
		cell.detailTextLabel.textColor = LINPHONE_MAIN_COLOR;
	}
	return cell;
}
@end

#pragma mark - UINavigationBarEx Class

@interface UINavigationBarEx : UINavigationBar
@end

@implementation UINavigationBarEx

INIT_WITH_COMMON_CF {
	[self setTintColor:[LINPHONE_MAIN_COLOR adjustHue:5.0f / 180.0f saturation:0.0f brightness:0.0f alpha:0.0f]];
	return self;
}

@end

#pragma mark - UINavigationControllerEx Class

@interface UINavigationControllerEx : UINavigationController

@end

@implementation UINavigationControllerEx

- (id)initWithRootViewController:(UIViewController *)rootViewController {
	[UINavigationControllerEx removeBackground:rootViewController.view];
	return [self initWithRootViewController:rootViewController];
}

+ (void)removeBackground:(UIView *)view {
	// iOS7 transparent background is *really* transparent: with an alpha != 0
	// it messes up the transitions. Use non-transparent BG for iOS7
	if ([[[UIDevice currentDevice] systemVersion] floatValue] >= 7)
		[view setBackgroundColor:LINPHONE_SETTINGS_BG_IOS7];
	else
		[view setBackgroundColor:[UIColor clearColor]];
}

- (void)pushViewController:(UIViewController *)viewController animated:(BOOL)animated {

	@try {
		[UINavigationControllerEx removeBackground:viewController.view];

		[viewController view]; // Force view
		UILabel *labelTitleView = [[UILabel alloc] init];
		labelTitleView.backgroundColor = [UIColor clearColor];
		labelTitleView.textColor =
			[UIColor colorWithRed:0x41 / 255.0f green:0x48 / 255.0f blue:0x4f / 255.0f alpha:1.0];
		labelTitleView.shadowColor = [UIColor colorWithWhite:1.0 alpha:0.5];
		labelTitleView.font = [UIFont boldSystemFontOfSize:20];
		labelTitleView.shadowOffset = CGSizeMake(0, 1);
		labelTitleView.textAlignment = NSTextAlignmentCenter;
		labelTitleView.text = viewController.title;
		[labelTitleView sizeToFit];
		viewController.navigationItem.titleView = labelTitleView;

		[super pushViewController:viewController animated:animated];
	} @catch (NSException *e) {
		// when device is slow and you are typing an item too much, a crash may happen
		// because we try to push the same view multiple times - in that case we should
		// do nothing but wait for device to respond again.
		LOGI(@"Failed to push view because it's already there: %@", e.reason);
		[self popToViewController:viewController animated:YES];
	}
}

- (void)setViewControllers:(NSArray *)viewControllers {
	for (UIViewController *controller in viewControllers) {
		[UINavigationControllerEx removeBackground:controller.view];
	}
	[super setViewControllers:viewControllers];
}

- (void)setViewControllers:(NSArray *)viewControllers animated:(BOOL)animated {
	for (UIViewController *controller in viewControllers) {
		[UINavigationControllerEx removeBackground:controller.view];
	}
	[super setViewControllers:viewControllers animated:animated];
}

@end

@implementation SettingsView

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

	settingsStore = [[LinphoneCoreSettingsStore alloc] init];

	_settingsController.showDoneButton = FALSE;
	_settingsController.delegate = self;
	_settingsController.showCreditsFooter = FALSE;
	_settingsController.settingsStore = settingsStore;

	[_navigationController.view setBackgroundColor:[UIColor clearColor]];

	_navigationController.view.frame = self.subView.frame;
	[_navigationController pushViewController:_settingsController animated:FALSE];
	[self.view addSubview:_navigationController.view];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[_settingsController dismiss:self];
	// Set observer
	[NSNotificationCenter.defaultCenter removeObserver:self name:kIASKAppSettingChanged object:nil];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	// Sync settings with linphone core settings
	[settingsStore transformLinphoneCoreToKeys];
	[self recomputeAccountLabelsAndSync];

	// Set observer
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(appSettingChanged:)
											   name:kIASKAppSettingChanged
											 object:nil];
}

#pragma mark - Account Creator callbacks

void update_hash_cbs(LinphoneAccountCreator *creator, LinphoneAccountCreatorStatus status, const char *resp) {
	SettingsView *thiz = (__bridge SettingsView *)(linphone_account_creator_get_user_data(creator));
	
	switch (status) {
		case LinphoneAccountCreatorOK:
			[thiz updatePassword:creator];
			break;
		default:
			[thiz showError:status];
			break;
	}
}
	
- (void) showError:(LinphoneAccountCreatorStatus) status {
	_tmpPwd = NULL;
	NSString* err;
	switch (status) {
		case LinphoneAccountCreatorAccountNotExist:
			err = NSLocalizedString(@"Bad credentials, check your account settings", nil);
			break;
		case LinphoneAccountCreatorErrorServer:
			err = NSLocalizedString(@"Server error, please try again later.", nil);
			break;
		default:
			err = NSLocalizedString(@"Unknown error, please try again later.", nil);
			break;
	}
	
	UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error while changing your password", nil)
																	 message:err
															  preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {}];
	
	[errView addAction:defaultAction];
	[self presentViewController:errView animated:YES completion:nil];
}

- (void) updatePassword:(LinphoneAccountCreator*) creator {
	linphone_account_creator_set_password(creator, _tmpPwd.UTF8String);
	[settingsStore setObject:_tmpPwd forKey:@"account_mandatory_password_preference"];
	
	LinphoneProxyConfig *config = bctbx_list_nth_data(linphone_core_get_proxy_config_list(LC),
													  [settingsStore integerForKey:@"current_proxy_config_preference"]);
	if (config != NULL) {
		const LinphoneAuthInfo *auth = linphone_proxy_config_find_auth_info(config);
		if (auth) {
			LinphoneAuthInfo * newAuth = linphone_auth_info_clone(auth);
			linphone_auth_info_set_passwd(newAuth, _tmpPwd.UTF8String);
			linphone_core_remove_auth_info(LC, auth);
			linphone_core_add_auth_info(LC, newAuth);
		}
	}
	[self recomputeAccountLabelsAndSync];
	[settingsStore setObject:_tmpPwd forKey:@"account_mandatory_password_preference"];
	_tmpPwd = NULL;
	
	
	UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Changing your password", nil)
																	 message:NSLocalizedString(@"Your password has been successfully changed", nil)
															  preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {}];
	
	[errView addAction:defaultAction];
	[self presentViewController:errView animated:YES completion:nil];
}

#pragma mark - Event Functions

- (void)appSettingChanged:(NSNotification *)notif {
	NSMutableSet *hiddenKeys = [NSMutableSet setWithSet:[_settingsController hiddenKeys]];
	NSMutableArray *keys = [NSMutableArray array];
	BOOL removeFromHiddenKeys = TRUE;

	if ([@"enable_video_preference" compare:notif.object] == NSOrderedSame) {
		removeFromHiddenKeys = [[notif.userInfo objectForKey:@"enable_video_preference"] boolValue];
		[keys addObject:@"video_menu"];
	} else if ([@"random_port_preference" compare:notif.object] == NSOrderedSame) {
		removeFromHiddenKeys = ![[notif.userInfo objectForKey:@"random_port_preference"] boolValue];
		[keys addObject:@"port_preference"];
	} else if ([@"backgroundmode_preference" compare:notif.object] == NSOrderedSame) {
		removeFromHiddenKeys = [[notif.userInfo objectForKey:@"backgroundmode_preference"] boolValue];
		[keys addObject:@"start_at_boot_preference"];
	} else if ([@"stun_preference" compare:notif.object] == NSOrderedSame) {
		NSString *stun_server = [notif.userInfo objectForKey:@"stun_preference"];
		removeFromHiddenKeys = (stun_server && ([stun_server length] > 0));
		[keys addObject:@"ice_preference"];
	} else if ([@"turn_preference" compare:notif.object] == NSOrderedSame) {
		LinphoneNatPolicy *LNP = linphone_core_get_nat_policy(LC);
		linphone_nat_policy_enable_turn(LNP, !linphone_nat_policy_turn_enabled(LNP));
		[keys addObject:@"turn_preference"];
	} else if ([@"debugenable_preference" compare:notif.object] == NSOrderedSame) {
		int debugLevel = [[notif.userInfo objectForKey:@"debugenable_preference"] intValue];
		BOOL debugEnabled = (debugLevel >= ORTP_DEBUG && debugLevel < ORTP_ERROR);
		removeFromHiddenKeys = debugEnabled;
		[keys addObject:@"send_logs_button"];
		[keys addObject:@"reset_logs_button"];
		[Log enableLogs:debugLevel];
		[LinphoneManager.instance lpConfigSetInt:debugLevel forKey:@"debugenable_preference"];
	} else if ([@"account_mandatory_advanced_preference" compare:notif.object] == NSOrderedSame) {
		removeFromHiddenKeys = [[notif.userInfo objectForKey:@"account_mandatory_advanced_preference"] boolValue];
		for (NSString *key in settingsStore->dict) {
			if (([key hasPrefix:@"account_"]) && (![key hasPrefix:@"account_mandatory_"])) {
				[keys addObject:key];
			}
		}
	} else if ([@"video_preset_preference" compare:notif.object] == NSOrderedSame) {
		NSString *video_preset = [notif.userInfo objectForKey:@"video_preset_preference"];
		removeFromHiddenKeys = [video_preset isEqualToString:@"custom"];
		[keys addObject:@"video_preferred_fps_preference"];
		[keys addObject:@"download_bandwidth_preference"];
	} else if ([notif.object isEqualToString:@"show_msg_in_notif"]) {
		// we have to register again to the iOS notification, because we change the actions associated with IM_MSG
		UIApplication *app = [UIApplication sharedApplication];
		LinphoneAppDelegate *delegate = (LinphoneAppDelegate *)app.delegate;
		[delegate registerForNotifications:app];
	}

	for (NSString *key in keys) {
		if (removeFromHiddenKeys)
			[hiddenKeys removeObject:key];
		else
			[hiddenKeys addObject:key];
	}

	[_settingsController setHiddenKeys:hiddenKeys animated:TRUE];
}

#pragma mark -

+ (IASKSpecifier *)filterSpecifier:(IASKSpecifier *)specifier {
	if (!linphone_core_sip_transport_supported(LC, LinphoneTransportTls)) {
		if ([[specifier key] isEqualToString:@"account_transport_preference"]) {
			NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:[specifier specifierDict]];
			NSMutableArray *titles = [NSMutableArray arrayWithArray:[dict objectForKey:@"Titles"]];
			[titles removeObject:@"TLS"];
			[dict setObject:titles forKey:@"Titles"];
			NSMutableArray *values = [NSMutableArray arrayWithArray:[dict objectForKey:@"Values"]];
			[values removeObject:@"tls"];
			[dict setObject:values forKey:@"Values"];
			return [[IASKSpecifier alloc] initWithSpecifier:dict];
		}
	} else {
		if ([[specifier key] isEqualToString:@"media_encryption_preference"]) {
			NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:[specifier specifierDict]];
			if (!linphone_core_media_encryption_supported(LC, LinphoneMediaEncryptionZRTP)) {
				NSMutableArray *titles = [NSMutableArray arrayWithArray:[dict objectForKey:@"Titles"]];
				[titles removeObject:@"ZRTP"];
				[dict setObject:titles forKey:@"Titles"];
				NSMutableArray *values = [NSMutableArray arrayWithArray:[dict objectForKey:@"Values"]];
				[values removeObject:@"ZRTP"];
				[dict setObject:values forKey:@"Values"];
			}
			if (!linphone_core_media_encryption_supported(LC, LinphoneMediaEncryptionSRTP)) {
				NSMutableArray *titles = [NSMutableArray arrayWithArray:[dict objectForKey:@"Titles"]];
				[titles removeObject:@"SRTP"];
				[dict setObject:titles forKey:@"Titles"];
				NSMutableArray *values = [NSMutableArray arrayWithArray:[dict objectForKey:@"Values"]];
				[values removeObject:@"SRTP"];
				[dict setObject:values forKey:@"Values"];
			}
			if (!linphone_core_media_encryption_supported(LC, LinphoneMediaEncryptionDTLS)) {
				NSMutableArray *titles = [NSMutableArray arrayWithArray:[dict objectForKey:@"Titles"]];
				[titles removeObject:@"DTLS"];
				[dict setObject:titles forKey:@"Titles"];
				NSMutableArray *values = [NSMutableArray arrayWithArray:[dict objectForKey:@"Values"]];
				[values removeObject:@"DTLS"];
				[dict setObject:values forKey:@"Values"];
			}
			return [[IASKSpecifier alloc] initWithSpecifier:dict];
		}
	}

	if ([specifier.key hasPrefix:@"menu_account_"]) {
		const bctbx_list_t *accounts = linphone_core_get_proxy_config_list(LC);
		int index = [specifier.key substringFromIndex:@"menu_account_".length].intValue - 1;
		if (index < bctbx_list_size(accounts)) {
			LinphoneProxyConfig *proxy = (LinphoneProxyConfig *)bctbx_list_nth_data(accounts, index);
			NSString *name = [NSString
				stringWithUTF8String:linphone_address_get_username(linphone_proxy_config_get_identity_address(proxy))];
			[specifier.specifierDict setValue:name forKey:kIASKTitle];
		}
	}

	return specifier;
}

- (NSSet *)findHiddenKeys {
	LinphoneManager *lm = LinphoneManager.instance;
	NSMutableSet *hiddenKeys = [NSMutableSet set];

	const MSList *accounts = linphone_core_get_proxy_config_list(LC);
	for (size_t i = bctbx_list_size(accounts) + 1; i <= 5; i++) {
		[hiddenKeys addObject:[NSString stringWithFormat:@"menu_account_%lu", i]];
	}

	if (!linphone_core_sip_transport_supported(LC, LinphoneTransportTls)) {
		[hiddenKeys addObject:@"media_encryption_preference"];
	}

	if (!linphone_core_lime_available(LC)) {
		[hiddenKeys addObject:@"use_lime_preference"];
	}

#ifndef DEBUG
	[hiddenKeys addObject:@"debug_actions_group"];
	[hiddenKeys addObject:@"release_button"];
	[hiddenKeys addObject:@"clear_cache_button"];
	[hiddenKeys addObject:@"battery_alert_button"];
	[hiddenKeys addObject:@"enable_auto_answer_preference"];
	[hiddenKeys addObject:@"flush_images_button"];
#endif

	int debugLevel = [LinphoneManager.instance lpConfigIntForKey:@"debugenable_preference"];
	BOOL debugEnabled = (debugLevel >= ORTP_DEBUG && debugLevel < ORTP_ERROR);
	if (!debugEnabled) {
		[hiddenKeys addObject:@"send_logs_button"];
		[hiddenKeys addObject:@"reset_logs_button"];
	}

	[hiddenKeys addObject:@"playback_gain_preference"];
	[hiddenKeys addObject:@"microphone_gain_preference"];

	[hiddenKeys addObject:@"network_limit_group"];

	[hiddenKeys addObject:@"incoming_call_timeout_preference"];
	[hiddenKeys addObject:@"in_call_timeout_preference"];

	[hiddenKeys addObject:@"wifi_only_preference"];

	if (!linphone_core_video_supported(LC)) {
		[hiddenKeys addObject:@"video_menu"];
	}

	if (![LinphoneManager isCodecSupported:"h264"]) {
		[hiddenKeys addObject:@"h264_preference"];
	}
	if (![LinphoneManager isCodecSupported:"mp4v-es"]) {
		[hiddenKeys addObject:@"mp4v-es_preference"];
	}

	if (![LinphoneManager isNotIphone3G])
		[hiddenKeys addObject:@"silk_24k_preference"];

	UIDevice *device = [UIDevice currentDevice];
	if (![device respondsToSelector:@selector(isMultitaskingSupported)] || ![device isMultitaskingSupported]) {
		[hiddenKeys addObject:@"backgroundmode_preference"];
		[hiddenKeys addObject:@"start_at_boot_preference"];
	} else {
		if (![lm lpConfigBoolForKey:@"backgroundmode_preference"]) {
			[hiddenKeys addObject:@"start_at_boot_preference"];
		}
	}

	[hiddenKeys addObject:@"enable_first_login_view_preference"];

	if (!linphone_core_video_supported(LC)) {
		[hiddenKeys addObject:@"enable_video_preference"];
	}

	if (!linphone_core_video_display_enabled(LC)) {
		[hiddenKeys addObject:@"video_menu"];
	}

	if (!linphone_core_get_video_preset(LC) || strcmp(linphone_core_get_video_preset(LC), "custom") != 0) {
		[hiddenKeys addObject:@"video_preferred_fps_preference"];
		[hiddenKeys addObject:@"download_bandwidth_preference"];
	}

	[hiddenKeys addObjectsFromArray:[[LinphoneManager unsupportedCodecs] allObjects]];

	BOOL random_port = [lm lpConfigBoolForKey:@"random_port_preference"];
	if (random_port) {
		[hiddenKeys addObject:@"port_preference"];
	}

	if (linphone_core_get_stun_server(LC) == NULL) {
		[hiddenKeys addObject:@"ice_preference"];
	}

	if (![lm lpConfigBoolForKey:@"debugenable_preference"]) {
		[hiddenKeys addObject:@"console_button"];
	}

	if (!IPAD) {
		[hiddenKeys addObject:@"preview_preference"];
	}

	if ([lm lpConfigBoolForKey:@"hide_run_assistant_preference"]) {
		[hiddenKeys addObject:@"assistant_button"];
	}

	if (!linphone_core_tunnel_available()) {
		[hiddenKeys addObject:@"tunnel_menu"];
	}

	if (![lm lpConfigBoolForKey:@"account_mandatory_advanced_preference"]) {
		for (NSString *key in settingsStore->dict) {
			if (([key hasPrefix:@"account_"]) && (![key hasPrefix:@"account_mandatory_"])) {
				[hiddenKeys addObject:key];
			}
		}
	}

	if (![[LinphoneManager.instance iapManager] enabled]) {
		[hiddenKeys addObject:@"in_app_products_button"];
	}

	if ([[UIDevice currentDevice].systemVersion floatValue] < 8) {
		[hiddenKeys addObject:@"repeat_call_notification_preference"];
	}

	return hiddenKeys;
}

- (void)recomputeAccountLabelsAndSync {
	// it's a bit violent... but IASK is not designed to dynamically change subviews' name
	_settingsController.hiddenKeys = [self findHiddenKeys];
	[_settingsController.settingsReader indexPathForKey:@"menu_account_1"]; // force refresh username'
	[_settingsController.settingsReader indexPathForKey:@"menu_account_2"]; // force refresh username'
	[_settingsController.settingsReader indexPathForKey:@"menu_account_3"]; // force refresh username'
	[_settingsController.settingsReader indexPathForKey:@"menu_account_4"]; // force refresh username'
	[_settingsController.settingsReader indexPathForKey:@"menu_account_5"]; // force refresh username'
	[[_settingsController tableView] reloadData];
}

#pragma mark - IASKSettingsDelegate Functions

- (void)settingsViewControllerDidEnd:(IASKAppSettingsViewController *)sender {
}

- (void)settingsViewControllerWillAppear:(IASKAppSettingsViewController *)sender {
	_backButton.hidden = (sender.file == nil || [sender.file isEqualToString:@"Root"]);
	_titleLabel.text = sender.title;

	// going to account: fill account specific info
	if ([sender.file isEqualToString:@"Account"]) {
		LOGI(@"Going editting account %@", sender.title);
		[settingsStore transformAccountToKeys:sender.title];
		// coming back to default: if we were in account, we must synchronize account now
	} else if ([sender.file isEqualToString:@"Root"]) {
		[settingsStore synchronize];
		[self recomputeAccountLabelsAndSync];
	}
}

- (void)settingsViewController:(IASKAppSettingsViewController *)sender
	  buttonTappedForSpecifier:(IASKSpecifier *)specifier {
	NSString *key = [specifier.specifierDict objectForKey:kIASKKey];
#ifdef DEBUG
	if ([key isEqual:@"release_button"]) {
		[UIApplication sharedApplication].keyWindow.rootViewController = nil;
		[[UIApplication sharedApplication].keyWindow setRootViewController:nil];
		[LinphoneManager.instance destroyLinphoneCore];
		[LinphoneManager instanceRelease];
	} else if ([key isEqual:@"clear_cache_button"]) {
		[PhoneMainView.instance.mainViewController
			clearCache:[NSArray arrayWithObject:[PhoneMainView.instance currentView]]];
	} else if ([key isEqual:@"battery_alert_button"]) {
		[[UIDevice currentDevice] _setBatteryState:UIDeviceBatteryStateUnplugged];
		[[UIDevice currentDevice] _setBatteryLevel:0.01f];
		[NSNotificationCenter.defaultCenter postNotificationName:UIDeviceBatteryLevelDidChangeNotification object:self];
	} else if ([key isEqual:@"flush_images_button"]) {
		const MSList *rooms = linphone_core_get_chat_rooms(LC);
		while (rooms) {
			const MSList *messages = linphone_chat_room_get_history(rooms->data, 0);
			while (messages) {
				LinphoneChatMessage *msg = messages->data;
				if (!linphone_chat_message_is_outgoing(msg)) {
					[LinphoneManager setValueInMessageAppData:nil forKey:@"localimage" inMessage:messages->data];
				}
				messages = messages->next;
			}
			rooms = rooms->next;
		}
	}
#endif
	if ([key isEqual:@"assistant_button"]) {
		[PhoneMainView.instance changeCurrentView:AssistantView.compositeViewDescription];
		return;
	} else if ([key isEqual:@"account_mandatory_remove_button"]) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Warning", nil)
																		 message:NSLocalizedString(@"Are you sure to want to remove your proxy setup?", nil)
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		UIAlertAction* continueAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Yes", nil)
																 style:UIAlertActionStyleDefault
															   handler:^(UIAlertAction * action) {
																   [settingsStore removeAccount];
																   [self recomputeAccountLabelsAndSync];
																   [_settingsController.navigationController popViewControllerAnimated:NO];
															   }];
		
		[errView addAction:defaultAction];
		[errView addAction:continueAction];
		[self presentViewController:errView animated:YES completion:nil];
	} else if ([key isEqual:@"account_mandatory_change_password"]) {
		UIAlertController *alertView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Change your password", nil)
																		 message:NSLocalizedString(@"Please enter and confirm your new password", nil)
																  preferredStyle:UIAlertControllerStyleAlert];
		
		[alertView addTextFieldWithConfigurationHandler:^(UITextField *textField) {
			textField.placeholder = NSLocalizedString(@"Password", nil);
			textField.clearButtonMode = UITextFieldViewModeWhileEditing;
			textField.borderStyle = UITextBorderStyleRoundedRect;
			textField.secureTextEntry = YES;
		}];
		
		[alertView addTextFieldWithConfigurationHandler:^(UITextField *textField) {
			textField.placeholder = NSLocalizedString(@"Confirm password", nil);
			textField.clearButtonMode = UITextFieldViewModeWhileEditing;
			textField.borderStyle = UITextBorderStyleRoundedRect;
			textField.secureTextEntry = YES;
		}];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		UIAlertAction* continueAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Save", nil)
																 style:UIAlertActionStyleDefault
															   handler:^(UIAlertAction * action) {
																   NSString * pwd = alertView.textFields[0].text;
																   NSString * conf_pwd = alertView.textFields[1].text;
																   if (pwd && ![pwd isEqualToString:@""]) {
																	   if ([pwd isEqualToString:conf_pwd]) {
																		   _tmpPwd = pwd;
																		   LinphoneProxyConfig *config = bctbx_list_nth_data(linphone_core_get_proxy_config_list(LC),
																													[settingsStore integerForKey:@"current_proxy_config_preference"]);
																		   const LinphoneAuthInfo *ai = linphone_proxy_config_find_auth_info(config);
																   
																		   LinphoneAccountCreator *account_creator = linphone_account_creator_new(
																												  LC, [LinphoneManager.instance lpConfigStringForKey:@"xmlrpc_url" inSection:@"assistant" withDefault:@""]
																												  .UTF8String);
																   
																		   linphone_account_creator_set_username(account_creator, linphone_auth_info_get_username(ai));
																		   if (linphone_auth_info_get_passwd(ai) && !(strcmp(linphone_auth_info_get_passwd(ai),"") == 0)) {
																			   linphone_account_creator_set_password(account_creator, linphone_auth_info_get_passwd(ai));
																		   } else {
																			   linphone_account_creator_set_ha1(account_creator, linphone_auth_info_get_ha1(ai));
																		   }
																		   
																		   linphone_account_creator_set_domain(account_creator, linphone_auth_info_get_domain(ai));
																		   linphone_account_creator_set_user_data(account_creator, (__bridge void *)(self));
																		   linphone_account_creator_cbs_set_update_hash(linphone_account_creator_get_callbacks(account_creator), update_hash_cbs);
																		   linphone_account_creator_update_password(account_creator, pwd.UTF8String);
																	   } else {
																		   UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error while changing your password", nil)
																																			message:NSLocalizedString(@"Your confirmation password doesn't match your password", nil)
																																	 preferredStyle:UIAlertControllerStyleAlert];
																	   
																		   UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
																															 handler:^(UIAlertAction * action) {}];
																	   
																		   [errView addAction:defaultAction];
																		   [self presentViewController:errView animated:YES completion:nil];
																	   }
																   } else {
																	   
																	   UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error while changing your password", nil)
																																		message:NSLocalizedString(@"Please enter and confirm your new password", nil)
																																 preferredStyle:UIAlertControllerStyleAlert];
																	   
																	   UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK" style:UIAlertActionStyleDefault
																															 handler:^(UIAlertAction * action) {}];
																	   
																	   [errView addAction:defaultAction];
																	   [self presentViewController:errView animated:YES completion:nil];
																   }
															   }];

		
		[alertView addAction:defaultAction];
		[alertView addAction:continueAction];
		[self presentViewController:alertView animated:YES completion:nil];
	} else if ([key isEqual:@"reset_logs_button"]) {
		linphone_core_reset_log_collection();
	} else if ([key isEqual:@"send_logs_button"]) {
		NSString *message;

		if ([LinphoneManager.instance lpConfigBoolForKey:@"send_logs_include_linphonerc_and_chathistory"]) {
			message = NSLocalizedString(
				@"Warning: an email will be created with 3 attachments:\n- Application "
				@"logs\n- Linphone configuration\n- Chats history.\nThey may contain "
				@"private informations (MIGHT contain clear-text password!).\nYou can remove one or several "
				@"of these attachments before sending your email, however there are all "
				@"important to diagnostize your issue.",
				nil);
		} else {
			message = NSLocalizedString(@"Warning: an email will be created with application "
										@"logs. It may contain "
										@"private informations (but no password!).\nThese logs are "
										@"important to diagnostize your issue.",
										nil);
		}
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Sending logs", nil)
																		 message:message
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		UIAlertAction* continueAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"I got it, continue", nil)
																 style:UIAlertActionStyleDefault
															   handler:^(UIAlertAction * action) {
																   [self sendEmailWithDebugAttachments];
															   }];
		
		[errView addAction:defaultAction];
		[errView addAction:continueAction];
		[self presentViewController:errView animated:YES completion:nil];
	}
}

#pragma mark - Mail composer for sending logs

- (void)sendEmailWithDebugAttachments {
	NSMutableArray *attachments = [[NSMutableArray alloc] initWithCapacity:3];

	// retrieve linphone logs if available
	char *filepath = linphone_core_compress_log_collection();
	if (filepath != NULL) {
		NSString *filename = [[NSString stringWithUTF8String:filepath] componentsSeparatedByString:@"/"].lastObject;
		NSString *mimeType = nil;
		if ([filename hasSuffix:@".txt"]) {
			mimeType = @"text/plain";
		} else if ([filename hasSuffix:@".gz"]) {
			mimeType = @"application/gzip";
		} else {
			LOGE(@"Unknown extension type: %@, not attaching logs", filename);
		}

		if (mimeType != nil) {
			[attachments addObject:@[ [NSString stringWithUTF8String:filepath], mimeType, filename ]];
		}
	}
	ms_free(filepath);

	if ([LinphoneManager.instance lpConfigBoolForKey:@"send_logs_include_linphonerc_and_chathistory"]) {
		// retrieve linphone rc
		[attachments
			addObject:@[ [LinphoneManager documentFile:@"linphonerc"], @"text/plain", @"linphone-configuration.rc" ]];

		// retrieve historydb
		[attachments addObject:@[
			[LinphoneManager documentFile:@"linphone_chats.db"],
			@"application/x-sqlite3",
			@"linphone-chats-history.db"
		]];
	}

	if (attachments.count == 0) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Cannot send logs", nil)
																		 message:NSLocalizedString(@"Nothing could be collected from your application, aborting now.", nil)
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Cancel", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
		return;
	}

	[self emailAttachments:attachments];
}
- (void)emailAttachments:(NSArray *)attachments {
	NSString *error = nil;
#if TARGET_IPHONE_SIMULATOR
	error = @"Cannot send emails on the Simulator. To test this feature, please use a real device.";
#else
	if ([MFMailComposeViewController canSendMail] == NO) {
		error = NSLocalizedString(
			@"Your device is not configured to send emails. Please configure mail application prior to send logs.",
			nil);
	}
#endif

	if (error != nil) {
		UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Cannot send email", nil)
																		 message:error
																  preferredStyle:UIAlertControllerStyleAlert];
		
		UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"Continue", nil)
																style:UIAlertActionStyleDefault
															  handler:^(UIAlertAction * action) {}];
		
		[errView addAction:defaultAction];
		[self presentViewController:errView animated:YES completion:nil];
	} else {
		MFMailComposeViewController *picker = [[MFMailComposeViewController alloc] init];
		picker.mailComposeDelegate = self;

		[picker setSubject:NSLocalizedString(@"<Please describe your problem or you will be ignored>",
											 @"Email title for people wanting to send a bug report")];
		[picker setToRecipients:[NSArray
									arrayWithObjects:[LinphoneManager.instance lpConfigStringForKey:@"debug_popup_email"
																						withDefault:@""],
													 nil]];
		[picker setMessageBody:NSLocalizedString(@"Here are information about an issue I had on my device.\nI was "
												 @"doing ...\nI expected Linphone to ...\nInstead, I got an "
												 @"unexpected result: ...",
												 @"Template email for people wanting to send a bug report")
						isHTML:NO];
		for (NSArray *attachment in attachments) {
			if ([[NSFileManager defaultManager] fileExistsAtPath:attachment[0]]) {
				[picker addAttachmentData:[NSData dataWithContentsOfFile:attachment[0]]
								 mimeType:attachment[1]
								 fileName:attachment[2]];
			}
		}
		[self.view.window.rootViewController presentViewController:picker animated:true completion:nil];
	}
}

- (void)mailComposeController:(MFMailComposeViewController *)controller
		  didFinishWithResult:(MFMailComposeResult)result
						error:(NSError *)error {
	if (error != nil) {
		LOGW(@"Error while sending mail: %@", error);
	} else {
		LOGI(@"Mail completed with status: %d", result);
	}
	[controller dismissViewControllerAnimated:true completion:nil];
}

- (IBAction)onDialerBackClick:(id)sender {
	[_settingsController.navigationController popViewControllerAnimated:NO];
	[PhoneMainView.instance popToView:DialerView.compositeViewDescription];
}

- (IBAction)onBackClick:(id)sender {
	[_settingsController.navigationController popViewControllerAnimated:YES];
}
@end
