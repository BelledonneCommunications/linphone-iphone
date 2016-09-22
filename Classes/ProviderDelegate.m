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
    const bctbx_list_t *calls = linphone_core_get_calls(LC);
    LinphoneCall* call = (LinphoneCall*)bctbx_list_find_custom(calls, (bctbx_compare_func)comp_call_id, [callID UTF8String]);
    linphone_core_accept_call(LC, call);
    [PhoneMainView.instance popToView:CallView.compositeViewDescription];
    [self.provider reportCallWithUUID:uuid endedAtDate:NULL reason:CXCallEndedReasonFailed];
    [action fulfill];
    
}

static int comp_call_id(const LinphoneCall *call, const char *callid) {
    if (linphone_call_log_get_call_id(linphone_call_get_call_log(call)) == nil) {
        ms_error("no callid for call [%p]", call);
        return 1;
    }
    return strcmp(linphone_call_log_get_call_id(linphone_call_get_call_log(call)), callid);
}

- (void)provider:(CXProvider *)provider performEndCallAction:(CXAnswerCallAction *)action {
    LOGD(@"CallKit : Ending the Call");
    [action fulfill];
    [PhoneMainView.instance popToView:DialerView.compositeViewDescription];
    NSUUID* uuid = action.callUUID;
    //NSString* callID = [self.calls objectForKey:uuid];
    [self.calls removeObjectForKey:uuid];
    const bctbx_list_t *calls = linphone_core_get_calls(LC);
    //LinphoneCall* call = (LinphoneCall*)bctbx_list_find_custom(calls, (bctbx_compare_func)comp_call_id, [callID UTF8String]);
    @try {
        linphone_core_terminate_call(LC, calls->data);
    } @catch (NSException* e) {
        LOGD(e.description);
    }
}

@end
