//
//  ProviderDelegate.m
//  linphone
//
//  Created by REIS Benjamin on 29/11/2016.
//
//

#import "ProviderDelegate.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#include "linphone/linphonecore.h"
#import <AVFoundation/AVAudioSession.h>
#import <Foundation/Foundation.h>

@implementation ProviderDelegate

- (instancetype)init {
	self = [super init];
	self.calls = [[NSMutableDictionary alloc] init];
	self.uuids = [[NSMutableDictionary alloc] init];
	self.pendingCall = NULL;
	self.pendingAddr = NULL;
	self.pendingCallVideo = FALSE;
	CXCallController *callController = [[CXCallController alloc] initWithQueue:dispatch_get_main_queue()];
	[callController.callObserver setDelegate:self queue:dispatch_get_main_queue()];
	self.controller = callController;
	self.callKitCalls = 0;

	if (!self) {
		LOGD(@"ProviderDelegate not initialized...");
	}
	return self;
}

- (void)config {
	CXProviderConfiguration *config = [[CXProviderConfiguration alloc]
		initWithLocalizedName:[NSBundle.mainBundle objectForInfoDictionaryKey:@"CFBundleDisplayName"]];
	config.ringtoneSound = @"notes_of_the_optimistic.caf";
	config.supportsVideo = FALSE;
	config.iconTemplateImageData = UIImagePNGRepresentation([UIImage imageNamed:@"callkit_logo"]);

	NSArray *ar = @[ [NSNumber numberWithInt:(int)CXHandleTypeGeneric] ];
	NSSet *handleTypes = [[NSSet alloc] initWithArray:ar];
	[config setSupportedHandleTypes:handleTypes];
	[config setMaximumCallGroups:2];
	[config setMaximumCallsPerCallGroup:1];
	self.provider = [[CXProvider alloc] initWithConfiguration:config];
	[self.provider setDelegate:self queue:dispatch_get_main_queue()];
}

- (void)configAudioSession:(AVAudioSession *)audioSession {
	[audioSession setCategory:AVAudioSessionCategoryPlayAndRecord
				  withOptions:AVAudioSessionCategoryOptionAllowBluetooth
						error:nil];
	[audioSession setMode:AVAudioSessionModeVoiceChat error:nil];
	double sampleRate = 44100.0;
	[audioSession setPreferredSampleRate:sampleRate error:nil];
}

- (void)reportIncomingCallwithUUID:(NSUUID *)uuid handle:(NSString *)handle video:(BOOL)video {
	// Create update to describe the incoming call and caller
	CXCallUpdate *update = [[CXCallUpdate alloc] init];
	update.remoteHandle = [[CXHandle alloc] initWithType:CXHandleTypeGeneric value:handle];
	update.supportsDTMF = TRUE;
	update.supportsHolding = TRUE;
	update.supportsGrouping = TRUE;
	update.supportsUngrouping = TRUE;
	update.hasVideo = video;

	// Report incoming call to system
	LOGD(@"CallKit: report new incoming call");
	[self.provider reportNewIncomingCallWithUUID:uuid
										  update:update
									  completion:^(NSError *error) {
									  }];
}

#pragma mark - CXProdiverDelegate Protocol

- (void)provider:(CXProvider *)provider performAnswerCallAction:(CXAnswerCallAction *)action {
	LOGD(@"CallKit : Answering Call");
	self.callKitCalls++;
	[self configAudioSession:[AVAudioSession sharedInstance]];
	[action fulfill];
	NSUUID *uuid = action.callUUID;

	NSString *callID = [self.calls objectForKey:uuid]; // first, make sure this callid is not already involved in a call
	LinphoneCall *call = [LinphoneManager.instance callByCallId:callID];
	if (call != NULL) {
		BOOL video = (!([UIApplication sharedApplication].applicationState == UIApplicationStateBackground) &&
					  linphone_core_get_video_policy(LC)->automatically_accept &&
					  linphone_call_params_video_enabled(linphone_call_get_remote_params((LinphoneCall *)call)));
		self.pendingCall = call;
		self.pendingCallVideo = video;
		return;
	};
}

- (void)provider:(CXProvider *)provider performStartCallAction:(CXStartCallAction *)action {
	LOGD(@"CallKit : Starting Call");
	// To restart Audio Unit
	[self configAudioSession:[AVAudioSession sharedInstance]];
	[action fulfill];
	NSUUID *uuid = action.callUUID;

	NSString *callID = [self.calls objectForKey:uuid]; // first, make sure this callid is not already involved in a call
	LinphoneCall *call;
	if (![callID isEqualToString:@""]) {
		call = linphone_core_get_current_call(LC);
	} else {
		call = [LinphoneManager.instance callByCallId:callID];
	}
	if (call != NULL) {
		_pendingCall = call;
	}
}

- (void)provider:(CXProvider *)provider performEndCallAction:(CXEndCallAction *)action {
	LOGD(@"CallKit : Ending the Call");
	self.callKitCalls--;
	[action fulfill];
	if (linphone_core_is_in_conference(LC)) {
		LinphoneManager.instance.conf = TRUE;
		linphone_core_terminate_conference(LC);
	} else if (linphone_core_get_calls_nb(LC) > 1) {
		LinphoneManager.instance.conf = TRUE;
		linphone_core_terminate_all_calls(LC);
	} else {
		NSUUID *uuid = action.callUUID;
		NSString *callID = [self.calls objectForKey:uuid];
		LinphoneCall *call = [LinphoneManager.instance callByCallId:callID];
		if (call) {
			linphone_call_terminate((LinphoneCall *)call);
		}
		[self.uuids removeObjectForKey:callID];
		[self.calls removeObjectForKey:uuid];
	}
}

- (void)provider:(CXProvider *)provider performSetMutedCallAction:(nonnull CXSetMutedCallAction *)action {
	[action fulfill];
	if ([[PhoneMainView.instance currentView] equal:CallView.compositeViewDescription]) {
		CallView *view = (CallView *)[PhoneMainView.instance popToView:CallView.compositeViewDescription];
		[view.microButton toggle];
	}
}

- (void)provider:(CXProvider *)provider performSetHeldCallAction:(nonnull CXSetHeldCallAction *)action {
	LOGD(@"CallKit : Call paused status changed");
	[action fulfill];
	if (linphone_core_is_in_conference(LC) && action.isOnHold) {
		linphone_core_leave_conference(LC);
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallUpdate object:self];
		return;
	}

	if (linphone_core_get_calls_nb(LC) > 1 && action.isOnHold) {
		linphone_core_pause_all_calls(LC);
		return;
	}

	NSUUID *uuid = action.callUUID;
	NSString *callID = [self.calls objectForKey:uuid];
	if (!callID) {
		return;
	}

	LinphoneCall *call = [LinphoneManager.instance callByCallId:callID];
	if (call) {
		if (action.isOnHold) {
			linphone_call_pause((LinphoneCall *)call);
		} else {
			[self configAudioSession:[AVAudioSession sharedInstance]];
			if (linphone_core_get_conference(LC)) {
				linphone_core_enter_conference(LC);
				[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallUpdate object:self];
			} else {
				_pendingCall = call;
			}
		}
	}
}

- (void)provider:(CXProvider *)provider performPlayDTMFCallAction:(CXPlayDTMFCallAction *)action {
	LOGD(@"CallKit : playing DTMF");
	[action fulfill];
	NSUUID *call_uuid = action.callUUID;
	NSString *callID = [self.calls objectForKey:call_uuid];
	LinphoneCall *call = [LinphoneManager.instance callByCallId:callID];
	char digit = action.digits.UTF8String[0];
	linphone_call_send_dtmf((LinphoneCall *)call, digit);
}

- (void)provider:(CXProvider *)provider didActivateAudioSession:(AVAudioSession *)audioSession {
	LOGD(@"CallKit : Audio session activated");
	// Now we can (re)start the call
	if (_pendingCall) {
		LinphoneCallState state = linphone_call_get_state(_pendingCall);
		switch (state) {
			case LinphoneCallIncomingReceived:
				[LinphoneManager.instance acceptCall:(LinphoneCall *)_pendingCall evenWithVideo:_pendingCallVideo];
				break;
			case LinphoneCallPaused:
				linphone_call_resume((LinphoneCall *)_pendingCall);
				break;
			case LinphoneCallStreamsRunning:
				// May happen when multiple calls
				break;
			default:
				break;
		}
	} else {
		if (_pendingAddr) {
			[LinphoneManager.instance doCall:_pendingAddr];
		} else {
			LOGE(@"CallKit : No pending call");
		}
	}

	_pendingCall = NULL;
	_pendingAddr = NULL;
	_pendingCallVideo = FALSE;
}

- (void)provider:(CXProvider *)provider didDeactivateAudioSession:(nonnull AVAudioSession *)audioSession {
	LOGD(@"CallKit : Audio session deactivated");

	_pendingCall = NULL;
	_pendingAddr = NULL;
	_pendingCallVideo = FALSE;
}

- (void)providerDidReset:(CXProvider *)provider {
	LOGD(@"CallKit : Provider reset");
	LinphoneManager.instance.conf = TRUE;
	linphone_core_terminate_all_calls(LC);
	[self.calls removeAllObjects];
	[self.uuids removeAllObjects];
}

#pragma mark - CXCallObserverDelegate Protocol

- (void)callObserver:(CXCallObserver *)callObserver callChanged:(CXCall *)call {
	LOGD(@"CallKit : Call changed");
}

@end
