//
//  LinphoneTester_Tests.m
//  LinphoneTester Tests
//
//  Created by guillaume on 10/09/2014.
//
//

#import <XCTest/XCTest.h>
#include "linphone/linphonecore.h"
#include "linphone/liblinphone_tester.h"
#import "NSObject+DTRuntime.h"
#import "Utils.h"
#import "Log.h"

@interface LinphoneTester_Tests : XCTestCase
@end

@implementation LinphoneTester_Tests

+ (NSString *)safetyTestString:(NSString *)testString {
	NSCharacterSet *charactersToRemove = [[NSCharacterSet alphanumericCharacterSet] invertedSet];
	return [[testString componentsSeparatedByCharactersInSet:charactersToRemove] componentsJoinedByString:@"_"];
}
void dummy_logger(const char *domain, OrtpLogLevel lev, const char *fmt, va_list args) {
}

+ (void)initialize {
	// turn off ALL logs because xcodebuild has problems with it
	// linphone_core_enable_logs_with_cb(dummy_logger);
	bc_tester_start(NULL);
	bc_tester_uninit();
	/*for (int i = 0; i < bc_tester_nb_suites(); i++) {
		const char *suite = bc_tester_suite_name(i);
		LOGE(@"suite = %s", suite);
		int test_count = bc_tester_nb_tests(suite);
		for (int k = 0; k < test_count; k++) {
			const char *test = bc_tester_test_name(suite, k);
			LOGE(@"\ttest = %s", test);
			if (suite) {
				NSString *sSuite = [NSString stringWithUTF8String:suite];
				if (test) {
					NSString *sTest = [NSString stringWithUTF8String:test];

					// prepend "test_" so that it gets found by introspection
					NSString *safesTest = [self safetyTestString:sTest];
					NSString *safesSuite = [self safetyTestString:sSuite];
					NSString *selectorName = [NSString stringWithFormat:@"test_%@__%@", safesSuite, safesTest];

					[LinphoneTester_Tests addInstanceMethodWithSelectorName:selectorName
																	  block:^(LinphoneTester_Tests *myself) {
																		[myself testForSuite:sSuite andTest:sTest];
																	  }];
				}
			}
		}
	}*/
}

- (void)testForSuite:(NSString *)suite andTest:(NSString *)test {
	LOGI(@"Launching test %@ from suite %@", test, suite);
	XCTAssertFalse(bc_tester_run_tests(suite.UTF8String, test.UTF8String, NULL), @"Suite '%@' / Test '%@' failed",
				   suite, test);
}

@end
