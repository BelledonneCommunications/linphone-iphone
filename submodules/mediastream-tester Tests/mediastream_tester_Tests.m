//
//  mediastream_tester_Tests.m
//  mediastream-tester Tests
//
//  Created by guillaume on 21/11/2014.
//
//

#import <UIKit/UIKit.h>
#import <XCTest/XCTest.h>
#import "NSObject+DTRuntime.h"

#include "mediastreamer2_tester.h"

@interface mediastream_tester_Tests : XCTestCase
@property (nonatomic,retain) NSString* bundlePath;
@property (nonatomic,retain) NSString* documentPath;
@property (nonatomic,retain) NSString* staticImagePath;
@end

@implementation mediastream_tester_Tests

+ (NSArray*)skippedSuites {
    NSArray* skipped_suites = @[];
    return skipped_suites;
}

+ (NSString*)safeifyTestString:(NSString*)testString{
    NSArray* invalidChars= @[@"[", @"]", @" ", @"-", @"."];
    NSString* safeString = testString;

    for (NSString* c in invalidChars) {
        safeString = [safeString stringByReplacingOccurrencesOfString:c withString:@"_"];
    }
    return safeString;
}

+ (void)initialize {
    mediastreamer2_tester_init();
    ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL);

    int count = mediastreamer2_tester_nb_test_suites();
    for (int i=0; i<count; i++) {
        const char* suite = mediastreamer2_tester_test_suite_name(i);

        int test_count = mediastreamer2_tester_nb_tests(suite);
        for( int k = 0; k<test_count; k++){
            const char* test =mediastreamer2_tester_test_name(suite, k);
            NSString* sSuite = [NSString stringWithUTF8String:suite];
            NSString* sTest  = [NSString stringWithUTF8String:test];

            if( [[mediastream_tester_Tests skippedSuites] containsObject:sSuite] ) continue;

            // prepend test_ so that it gets found by introspection
            NSString* safesTest    = [self safeifyTestString:sTest];
            NSString* safesSuite   = [self safeifyTestString:sSuite];
            NSString *selectorName = [NSString stringWithFormat:@"test_%@__%@", safesSuite, safesTest];
            NSLog(@"Adding test: %@", selectorName);
            [mediastream_tester_Tests addInstanceMethodWithSelectorName:selectorName block:^(mediastream_tester_Tests* myself) {
                [myself testForSuite:sSuite andTest:sTest];
            }];
        }
    }


}

- (void)setUp {
    [super setUp];
    self.bundlePath = [[NSBundle mainBundle] bundlePath];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    self.documentPath = [paths objectAtIndex:0];
    self.staticImagePath = [[self.bundlePath stringByAppendingString:@"/"] stringByAppendingString:@"nowebcamCIF.jpg"];

    NSLog(@"Bundle path: %@", self.bundlePath);
    NSLog(@"Document path: %@", self.documentPath);

    mediastreamer2_tester_set_file_root( [self.bundlePath UTF8String] );
    mediastreamer2_tester_set_writable_dir( [self.documentPath UTF8String] );
    ms_static_image_set_default_image( [self.staticImagePath UTF8String]);

    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testForSuite:(NSString*)suite andTest:(NSString*)test
{
    NSLog(@"Launching test %@ from suite %@", test, suite);
    XCTAssertFalse(mediastreamer2_tester_run_tests([suite UTF8String], [test UTF8String]), @"Suite '%@' / Test '%@' failed", suite, test);
}

@end
