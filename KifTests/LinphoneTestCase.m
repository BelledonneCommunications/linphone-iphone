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

@implementation LinphoneTestCase

+ (void)initialize {
	// default is 0.01, which sometimes confuses the simulator to the point that
	// it will miss some keys
	[KIFTypist setKeystrokeDelay:0.05];

	NSString *language = [[NSLocale preferredLanguages] objectAtIndex:0];
	if (!([language isEqualToString:@"en"] || [language containsString:@"en-"])) {
		LOGF(@"Language must be en (English) instead of %@", language);
	}
}

- (NSString *)accountUsername {
    return @"testios";
}

- (NSString *)accountDomain {
    return @"sip.linphone.org";
}

- (NSString*)getUUID {
    return [[[NSUUID UUID] UUIDString] substringToIndex:8];
}

- (NSArray *)getUUIDArrayOfSize:(size_t)size {
    NSMutableArray* array = [NSMutableArray arrayWithCapacity:size];
    for (NSInteger i=0; i<size; i++) {
        [array setObject:[self getUUID] atIndexedSubscript:i];
    }
    return array;
}

- (void)beforeAll{
#if TARGET_IPHONE_SIMULATOR
    [tester acknowledgeSystemAlert];
#endif
	[super beforeAll];
}


static bool invalidAccount = true;

- (void)setInvalidAccountSet:(BOOL)invalidAccountSet {
    invalidAccount = invalidAccountSet;
}

- (BOOL)invalidAccountSet {
    return invalidAccount;
}

- (BOOL)hasValidProxyConfig {
    LinphoneCore* lc = [LinphoneManager getLc];
    const MSList* proxies = linphone_core_get_proxy_config_list(lc);
    BOOL isOK = false;
    while(proxies){
        LinphoneProxyConfig* cfg = (LinphoneProxyConfig*)proxies->data;
        const char*   domain = linphone_proxy_config_get_domain(cfg);
        const char* identity = linphone_proxy_config_get_identity(cfg);
        LinphoneAddress* addr = linphone_core_interpret_url(lc, identity);
        const char* username = linphone_address_get_username(addr);

		if (addr && (username && strcmp(username, [[self accountUsername] UTF8String]) == 0) &&
			(domain && strcmp(domain, [[self accountDomain] UTF8String]) == 0) &&
			linphone_proxy_config_get_state(cfg) == LinphoneRegistrationOk) {
			isOK = true;
			linphone_address_destroy(addr);
			break;
		} else if (addr) {
			linphone_address_destroy(addr);
		}

		proxies = proxies->next;
	}
	return isOK;
}

- (void)switchToValidAccountIfNeeded {
    [UIView setAnimationsEnabled:false];

	if (invalidAccount && ![self hasValidProxyConfig]) {

		[tester tapViewWithAccessibilityLabel:@"Settings"];
		[tester tapViewWithAccessibilityLabel:@"Run assistant"];
        [tester waitForTimeInterval:0.5];
		if ([tester tryFindingViewWithAccessibilityLabel:@"Launch Wizard" error:nil]) {
			[tester tapViewWithAccessibilityLabel:@"Launch Wizard"];
			[tester waitForTimeInterval:0.5];
		}

		NSLog(@"Switching to a valid account");

		[tester tapViewWithAccessibilityLabel:@"Start"];
		[tester tapViewWithAccessibilityLabel:@"Sign in linphone.org account"];

		[tester enterText:@"testios" intoViewWithAccessibilityLabel:@"Username"];
		[tester enterText:@"testtest" intoViewWithAccessibilityLabel:@"Password"];

		[tester tapViewWithAccessibilityLabel:@"Sign in"];

		invalidAccount = false;
	}
}


@end
