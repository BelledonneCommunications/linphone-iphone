//
//  LinphoneTestCase.m
//  linphone
//
//  Created by Guillaume BIENKOWSKI on 19/01/2015.
//
//

#import "LinphoneTestCase.h"

#import "LinphoneManager.h"

#import "KIF/KIFTypist.h"
#import "Log.h"
#import "Utils.h"

@implementation LinphoneTestCase

+ (void)initialize {
	// default is 0.01, which sometimes confuses the simulator to the point that
	// it will miss some keys
	[KIFTypist setKeystrokeDelay:0.05];

	NSString *language = [[NSLocale preferredLanguages] objectAtIndex:0];
	if (!([language isEqualToString:@"en"] || [language containsSubstring:@"en-"])) {
		LOGF(@"Language must be 'en' (English) instead of %@", language);
	}
}

- (void)beforeAll {
	[super beforeAll];

#if TARGET_IPHONE_SIMULATOR
	while ([tester acknowledgeSystemAlert]) {
		[tester waitForTimeInterval:.5f];
	};
#endif
	// remove any account
	[LinphoneManager.instance removeAllAccounts];

	// go to dialer
	for (NSString *button in @[ @"Cancel", @"Back", @"Hangup", @"Dialer" ]) {
		if ([tester tryFindingTappableViewWithAccessibilityLabel:button error:nil]) {
			[tester tapViewWithAccessibilityLabel:button traits:UIAccessibilityTraitButton];
		}
	}
}

- (void)beforeEach {
	[[LinphoneManager instance] lpConfigSetInt:ORTP_WARNING forKey:@"debugenable_preference"];
	[[LinphoneManager instance] lpConfigSetInt:NO forKey:@"animations_preference"];
}

- (NSString *)me {
	return [NSString stringWithFormat:@"testios-%@",
									  [[UIDevice currentDevice].identifierForVendor.UUIDString substringToIndex:6]]
		.lowercaseString;
}

- (NSString *)accountDomain {
	return @"test.linphone.org";
}

- (NSString *)getUUID {
	return [[[NSUUID UUID] UUIDString] substringToIndex:8].lowercaseString;
}

- (NSArray *)getUUIDArrayOfSize:(size_t)size {
	NSMutableArray *array = [NSMutableArray arrayWithCapacity:size];
	for (NSInteger i = 0; i < size; i++) {
		[array setObject:[self getUUID] atIndexedSubscript:i];
	}
	return array;
}

- (BOOL)hasValidProxyConfig {
	LinphoneCore *lc = [LinphoneManager getLc];
	const MSList *proxies = linphone_core_get_proxy_config_list(lc);
	BOOL isOK = false;
	while (proxies) {
		LinphoneProxyConfig *cfg = (LinphoneProxyConfig *)proxies->data;
		const char *domain = linphone_proxy_config_get_domain(cfg);
		const LinphoneAddress *addr = linphone_proxy_config_get_identity_address(cfg);
		const char *username = linphone_address_get_username(addr);

		if (addr && (username && strcmp(username, [[self me] UTF8String]) == 0) &&
			(domain && strcmp(domain, [[self accountDomain] UTF8String]) == 0) &&
			linphone_proxy_config_get_state(cfg) == LinphoneRegistrationOk) {
			isOK = true;
			break;
		}

		proxies = proxies->next;
	}
	return isOK;
}

- (void)switchToValidAccountIfNeeded {
	[UIView setAnimationsEnabled:false];

	if (![self hasValidProxyConfig]) {
		LOGI(@"Switching to a test account...");

		LinphoneCore *lc = [LinphoneManager getLc];
		linphone_core_clear_proxy_config(lc);
		linphone_core_clear_all_auth_info(lc);

		LinphoneAddress *testAddr = linphone_core_interpret_url(
			LC, [[NSString stringWithFormat:@"sip:%@@%@", [self me], [self accountDomain]] UTF8String]);
		linphone_address_set_header(testAddr, "X-Create-Account", "yes");
		linphone_address_set_transport(testAddr, LinphoneTransportTcp);
		linphone_address_set_port(testAddr, 0);

		LinphoneProxyConfig *testProxy = linphone_proxy_config_new();
		linphone_proxy_config_set_identity_address(testProxy, testAddr);
		char *server_addr = ms_strdup_printf("%s;transport=tcp", linphone_address_get_domain(testAddr));
		linphone_proxy_config_set_server_addr(testProxy, server_addr);
		linphone_proxy_config_set_route(testProxy, server_addr);
		ms_free(server_addr);

		LinphoneAuthInfo *testAuth = linphone_auth_info_new(linphone_address_get_username(testAddr), NULL,
															linphone_address_get_username(testAddr), NULL, NULL,
															linphone_address_get_domain(testAddr));

		[[LinphoneManager instance] configurePushTokenForProxyConfig:testProxy];
		[[LinphoneManager instance] removeAllAccounts];

		linphone_proxy_config_enable_register(testProxy, true);
		linphone_core_add_auth_info(lc, testAuth);
		linphone_core_add_proxy_config(lc, testProxy);
		linphone_core_set_default_proxy_config(lc, testProxy);

		linphone_proxy_config_unref(testProxy);
		linphone_auth_info_destroy(testAuth);
		linphone_address_destroy(testAddr);

		linphone_core_set_file_transfer_server(lc, "https://www.linphone.org:444/lft.php");

		// reload address book to prepend proxy config domain to contacts' phone number
		[[[LinphoneManager instance] fastAddressBook] reload];

		[self waitForRegistration];
		[[LinphoneManager instance] lpConfigSetInt:NO forKey:@"animations_preference"];
	}
}

- (UITableView *)findTableView:(NSString *)table {
	UITableView *tv = nil;
	NSError *err = nil;
	if ([tester tryFindingAccessibilityElement:nil view:&tv withIdentifier:table tappable:false error:&err]) {
		XCTAssertNotNil(tv);
	} else {
		XCTFail(@"Error: %@", err);
	}
	return tv;
}

- (void)waitForRegistration {
	// wait for account to be registered
	int timeout = 15;
	while (timeout &&
		   ![tester tryFindingViewWithAccessibilityLabel:@"Registration state"
												   value:@"Registered"
												  traits:UIAccessibilityTraitButton
												   error:nil]) {
		[tester waitForTimeInterval:1];
		timeout--;
	}
	[tester waitForViewWithAccessibilityLabel:@"Registration state"
										value:@"Registered"
									   traits:UIAccessibilityTraitButton];
}

@end
