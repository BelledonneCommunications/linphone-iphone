//
//  ProviderDelegate.m
//  linphone
//
//  Created by REIS Benjamin on 20/09/2016.
//
//

#import <Foundation/Foundation.h>
#import "ProviderDelegate.h"
#import "PhoneMainView.h"
#import "LinphoneManager.h"

@implementation ProviderDelegate


- (void)providerDidReset:(CXProvider *)provider {
    
}

- (void) reportIncomingCallwithUUID:(NSUUID*)uuid handle:(NSString*) handle {
    CXProviderConfiguration* config = [[CXProviderConfiguration alloc] initWithLocalizedName:@"Incoming call"];
    config.ringtoneSound = @"notes_of_the_optimistic.caf";
    config.supportsVideo = TRUE;
    config.iconTemplateImageData = [NSData dataWithContentsOfFile:@"logo.png"];
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
    }];
}

- (instancetype) init {
    self = [super init];
    self.calls = [[NSMutableDictionary alloc] init];
	self.uuids = [[NSMutableDictionary alloc] init];
    if (!self) {
        LOGD(@"ProviderDelegate not initialized...");
    }
    return self;
}

- (void)provider:(CXProvider *)provider performAnswerCallAction:(CXAnswerCallAction *)action {
    LOGD(@"CallKit : Answering Call");
    NSUUID* uuid = action.callUUID;
    self.provider = provider;
    
    NSString* callID = [self.calls objectForKey:uuid];
	[LinphoneManager.instance setCallKit:TRUE];
	[LinphoneManager.instance acceptCallForCallId:callID];
    [action fulfill];
    
}

- (void)provider:(CXProvider *)provider performEndCallAction:(CXEndCallAction *)action {
    LOGD(@"CallKit : Ending the Call");
	linphone_core_terminate_call(LC, linphone_core_get_current_call(LC));
	[action fulfill];
}

@end
