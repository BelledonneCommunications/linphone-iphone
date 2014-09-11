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

@interface LinphoneTester_Tests : XCTestCase

@end

@implementation LinphoneTester_Tests {
	NSString* bundlePath;
	NSString* documentPath;
}


static void linphone_log_function(OrtpLogLevel lev, const char *fmt, va_list args) {
    NSString* log = [[NSString alloc] initWithFormat:[NSString stringWithUTF8String:fmt] arguments:args];
    NSLog(@"%@",log);
}


void LSLog(NSString* fmt, ...){
    va_list args;
    va_start(args, fmt);
    linphone_log_function(ORTP_MESSAGE, [fmt UTF8String], args);
    va_end(args);
}




- (id)init {
	self = [super init];
	if( self ){
		bundlePath = [[NSBundle mainBundle] bundlePath];
		NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
		documentPath = [paths objectAtIndex:0];
		LSLog(@"Bundle path: %@", bundlePath);
		LSLog(@"Document path: %@", documentPath);

		liblinphone_tester_set_fileprefix([bundlePath UTF8String]);
		liblinphone_tester_set_writable_dir_prefix( ms_strdup([documentPath UTF8String]) );
	}
	return self;
}

+ (NSArray*)skippedSuites {
	NSArray* skipped_suites = @[@"Flexisip"];
	return skipped_suites;
}

+ (void)initialize {
    liblinphone_tester_init();

    int count = liblinphone_tester_nb_test_suites();

    for (int i=0; i<count; i++) {
        const char* suite = liblinphone_tester_test_suite_name(i);

		LSLog(@"Running '%s' suite", suite);
		int test_count = liblinphone_tester_nb_tests(suite);
		for( int k = 0; k<test_count; k++){
			const char* test =liblinphone_tester_test_name(suite, k);
			NSString* sSuite = [NSString stringWithUTF8String:suite];
			NSString* sTest  = [NSString stringWithUTF8String:test];

			if( [[LinphoneTester_Tests skippedSuites] containsObject:sSuite] ) continue;

			// prepend test_ so that it gets found by introspection
            NSString* safesTest    = [sTest stringByReplacingOccurrencesOfString:@" " withString:@"_"];
            NSString* safesSuite   = [sSuite stringByReplacingOccurrencesOfString:@" " withString:@"_"];
            NSString *selectorName = [NSString stringWithFormat:@"test_%@__%@", safesSuite, safesTest];
			[LinphoneTester_Tests addInstanceMethodWithSelectorName:selectorName block:^(LinphoneTester_Tests* myself) {
				[myself testForSuite:sSuite andTest:sTest];
			}];
		}
    }

}

- (void)setUp
{
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown
{
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testForSuite:(NSString*)suite andTest:(NSString*)test
{
	XCTAssertFalse(liblinphone_tester_run_tests([suite UTF8String], [test UTF8String]), @"Suite '%@' / Test '%@' failed", suite, test);
}

@end
