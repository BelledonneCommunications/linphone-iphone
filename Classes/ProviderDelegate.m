//
//  ProviderDelegate.m
//  linphone
//
//  Created by REIS Benjamin on 20/09/2016.
//
//

#import <Foundation/Foundation.h>
#import "ProviderDelegate.h"

@implementation ProviderDelegate


- (void)providerDidReset:(CXProvider *)provider {
    
}

- (void) reportIncomingCallwithUUID:(NSUUID*)uuid handle:(NSString*) handle {
    CXProviderConfiguration* config = [[CXProviderConfiguration alloc] initWithLocalizedName:@"Linphone localized name"];
    config.ringtoneSound = @"notes_of_the_optimistic.caf";
    config.supportsVideo = TRUE;
    NSArray* ar = @[[NSNumber numberWithInt:(int)CXHandleTypeGeneric]];
    NSSet *handleTypes = [[NSSet alloc] initWithArray:ar];
    [config setSupportedHandleTypes:handleTypes];
    self.provider = [[CXProvider alloc] initWithConfiguration:config];
    [self.provider setDelegate:self queue:dispatch_get_main_queue()];
    // Create update to describe the incoming call and caller
    CXCallUpdate *update = [[CXCallUpdate alloc] init];
    update.remoteHandle = [[CXHandle alloc] initWithType:CXHandleTypeGeneric value:handle];
    
    // Report incoming call to system
    LOGD(@"CallKit: report new incoming call");
    [self.provider reportNewIncomingCallWithUUID:uuid update:update completion:^(NSError* error) {
        // Do something
        LOGD(@"CallKit");
    }];
}

- (instancetype) init {
    self = [super init];
    if (!self) {
        LOGD(@"ProviderDelegate not initialized...");
    }
    return self;
}

@end
