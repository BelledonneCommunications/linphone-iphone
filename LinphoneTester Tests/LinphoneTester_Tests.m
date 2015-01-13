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
@property (retain, nonatomic) NSString* bundlePath;
@property (retain, nonatomic) NSString* documentPath;
@end

@implementation LinphoneTester_Tests
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


+ (NSArray*)skippedSuites {
	NSArray* skipped_suites = @[@"Flexisip"];
	return skipped_suites;
}


+ (NSString*)safeifyTestString:(NSString*)testString{
    NSCharacterSet *charactersToRemove = [[NSCharacterSet alphanumericCharacterSet] invertedSet];
    return [[testString componentsSeparatedByCharactersInSet:charactersToRemove] componentsJoinedByString:@"_"];
}



+ (void)initialize {

    static char * bundle = NULL;
    static char * documents = NULL;
    liblinphone_tester_init();

    NSString* bundlePath = [[NSBundle mainBundle] bundlePath];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString* documentPath = [paths objectAtIndex:0];
    bundle = ms_strdup([bundlePath UTF8String]);
    documents = ms_strdup([documentPath UTF8String]);

    LSLog(@"Bundle path: %@", bundlePath);
    LSLog(@"Document path: %@", documentPath);

    liblinphone_tester_set_fileprefix(bundle);
    liblinphone_tester_set_writable_dir_prefix(documents);
	liblinphone_tester_keep_accounts(TRUE);

	int count = liblinphone_tester_nb_test_suites();

    for (int i=0; i<count; i++) {
        const char* suite = liblinphone_tester_test_suite_name(i);

		int test_count = liblinphone_tester_nb_tests(suite);
		for( int k = 0; k<test_count; k++){
			const char* test =liblinphone_tester_test_name(suite, k);
			NSString* sSuite = [NSString stringWithUTF8String:suite];
			NSString* sTest  = [NSString stringWithUTF8String:test];

			if( [[LinphoneTester_Tests skippedSuites] containsObject:sSuite] ) continue;
            // prepend "test_" so that it gets found by introspection
            NSString* safesTest    = [self safeifyTestString:sTest];
            NSString* safesSuite   = [self safeifyTestString:sSuite];
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
}

- (void)tearDown
{
    [super tearDown];
}

- (void)testForSuite:(NSString*)suite andTest:(NSString*)test
{
	LSLog(@"Launching test %@ from suite %@", test, suite);
	XCTAssertFalse(liblinphone_tester_run_tests([suite UTF8String], [test UTF8String]), @"Suite '%@' / Test '%@' failed", suite, test);
}

- (void)dealloc {
	liblinphone_tester_clear_accounts();
}

@end
