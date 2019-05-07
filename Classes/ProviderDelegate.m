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
#import "SVProgressHUD.h"


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
	NSError *err = nil;
	[audioSession setCategory:AVAudioSessionCategoryPlayAndRecord
                         mode:AVAudioSessionModeVoiceChat
                      options:AVAudioSessionCategoryOptionAllowBluetooth | AVAudioSessionCategoryOptionAllowBluetoothA2DP
						error:&err];
	if (err) {
		LOGE(@"Unable to change audio session because: %@", err.localizedDescription);
		err = nil;
	}
	[audioSession setMode:AVAudioSessionModeVoiceChat error:&err];
	if (err) {
		LOGE(@"Unable to change audio mode because : %@", err.localizedDescription);
		err = nil;
	}
	double sampleRate = 48000.0;
	[audioSession setPreferredSampleRate:sampleRate error:&err];
	if (err) {
		LOGE(@"Unable to change preferred sample rate because : %@", err.localizedDescription);
		err = nil;
	}
}

- (void)reportIncomingCall:(LinphoneCall *) call withUUID:(NSUUID *)uuid handle:(NSString *)handle video:(BOOL)video; {
	// Create update to describe the incoming call and caller
	CXCallUpdate *update = [[CXCallUpdate alloc] init];
	update.remoteHandle = [[CXHandle alloc] initWithType:CXHandleTypeGeneric value:handle];
	update.supportsDTMF = TRUE;
	update.supportsHolding = TRUE;
	update.supportsGrouping = TRUE;
	update.supportsUngrouping = TRUE;
	update.hasVideo = _pendingCallVideo = video;

	// Report incoming call to system
    LOGD(@"CallKit: report new incoming call with call-id: [%@] and UUID: [%@]", [_calls objectForKey:uuid], uuid);
	[self.provider reportNewIncomingCallWithUUID:uuid
										  update:update
									  completion:^(NSError *error) {
										  if (error) {
											  LOGE(@"CallKit: cannot complete incoming call with call-id: [%@] and UUID: [%@] from [%@] caused by [%@]",
                                                   [_calls objectForKey:uuid], uuid, handle, [error localizedDescription]);
											  if ([error code] == CXErrorCodeIncomingCallErrorFilteredByDoNotDisturb ||
												  [error code] == CXErrorCodeIncomingCallErrorFilteredByBlockList)
												  linphone_call_decline(call,LinphoneReasonBusy); /*to give a chance for other devices to answer*/
											  else
												  linphone_call_decline(call,LinphoneReasonUnknown);
										  }
									  }];
}

- (void)setPendingCall:(LinphoneCall *)pendingCall {
	if (pendingCall) {
		_pendingCall = pendingCall;
        if (_pendingCall)
            linphone_call_ref(_pendingCall);
	} else if (_pendingCall) {
        linphone_call_unref(_pendingCall);
		_pendingCall = NULL;
	}
}

#pragma mark - CXProviderDelegate Protocol

- (void)provider:(CXProvider *)provider performAnswerCallAction:(CXAnswerCallAction *)action {
    NSUUID *uuid = action.callUUID;
    NSString *callID = [self.calls objectForKey:uuid]; // first, make sure this callid is not already involved in a call
	LOGD(@"CallKit: Answering call with call-id: [%@] and UUID: [%@]", callID, uuid);
	[self configAudioSession:[AVAudioSession sharedInstance]];
	[action fulfill];
	LinphoneCall *call = [LinphoneManager.instance callByCallId:callID];
	if (!call)
		return;

	self.callKitCalls++;
    self.pendingCall = call;
}

- (void)provider:(CXProvider *)provider performStartCallAction:(CXStartCallAction *)action {
    NSUUID *uuid = action.callUUID;
    NSString *callID = [self.calls objectForKey:uuid]; // first, make sure this callid is not already involved in a call
	LOGD(@"CallKit: Starting Call with call-id: [%@] and UUID: [%@]", callID, uuid);
	// To restart Audio Unit
	[self configAudioSession:[AVAudioSession sharedInstance]];
	[action fulfill];
	LinphoneCall *call;
	if (![callID isEqualToString:@""]) {
		call = linphone_core_get_current_call(LC);
	} else {
		call = [LinphoneManager.instance callByCallId:callID];
	}
	if (call != NULL) {
        self.callKitCalls++;
		self.pendingCall = call;
	}
}

- (void)provider:(CXProvider *)provider performEndCallAction:(CXEndCallAction *)action {
	self.callKitCalls--;
	[action fulfill];
	if (linphone_core_is_in_conference(LC)) {
		LinphoneManager.instance.conf = TRUE;
		linphone_core_terminate_conference(LC);
        LOGD(@"CallKit: Ending the conference");
	} else if (linphone_core_get_calls_nb(LC) > 1) {
		LinphoneManager.instance.conf = TRUE;
		linphone_core_terminate_all_calls(LC);
        LOGD(@"CallKit: Ending all the ongoing calls");
	} else {
		NSUUID *uuid = action.callUUID;
		NSString *callID = [self.calls objectForKey:uuid];
		if (callID) {
			dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
				LOGD(@"CallKit: Ending the call with call-id: [%@] and UUID: [%@]", callID, uuid);
				LinphoneCall *call = [LinphoneManager.instance callByCallId:callID];
				if (call) {
					NSUserDefaults *defaults = [[NSUserDefaults alloc] initWithSuiteName:[NSString stringWithFormat:@"group.%@.intentExtension",[[NSBundle mainBundle] bundleIdentifier]]];
					NSString *rejectionMessage = [defaults objectForKey:@"rejectionmessage"];
					if (rejectionMessage) {
						[defaults removeObjectForKey:@"rejectionmessage"];
						[defaults synchronize];
						if ([rejectionMessage isEqualToString:@"custom"]) {
							NSString *displayName = [FastAddressBook displayNameForAddress:linphone_call_get_remote_address(call)];
							UIAlertController *alert = [UIAlertController alertControllerWithTitle:nil message:[NSString stringWithFormat:NSLocalizedString(@"Decline incoming call from %@ with message:",nil),displayName] preferredStyle:UIAlertControllerStyleAlert];
							[alert addTextFieldWithConfigurationHandler:^(UITextField *textField) {
								textField.placeholder =NSLocalizedString(@"Message",nil);
								textField.secureTextEntry = NO;
							}];
							[alert addAction:[UIAlertAction actionWithTitle:NSLocalizedString(@"Decline",nil) style:UIAlertActionStyleDefault handler:^(UIAlertAction * action) {
								UITextField *tf = alert.textFields.firstObject;
								if (tf.text && tf.text.length > 0)
									[self declineCall:call withRejectionMessage:tf.text];
								else
									linphone_call_terminate((LinphoneCall *)call);
							}]];
							[[NSNotificationCenter defaultCenter] addObserverForName:kLinphoneCallUpdate object:nil queue:nil usingBlock:^(NSNotification *notif){
																			  dispatch_async(dispatch_get_main_queue(), ^ {
																				  LinphoneCall *acall = [[notif.userInfo objectForKey:@"call"] pointerValue];
																				  LinphoneCallState astate = [[notif.userInfo objectForKey:@"state"] intValue];
																				  if (acall == call && astate != LinphoneCallStateIncomingReceived) {
																					  [alert dismissViewControllerAnimated:NO completion:nil];
																				  }
																			  });
																		  }];
							[PhoneMainView.instance presentViewController:alert animated:YES completion:nil];
						} else
							[self declineCall:call withRejectionMessage:rejectionMessage];
					} else
						linphone_call_terminate((LinphoneCall *)call);
				}
				[self.uuids removeObjectForKey:callID];
				[self.calls removeObjectForKey:uuid];
			});
		}
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
	[action fulfill];
	if (linphone_core_is_in_conference(LC) && action.isOnHold) {
		linphone_core_leave_conference(LC);
        LOGD(@"CallKit: Leaving conference");
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallUpdate object:self];
		return;
	}

	if (linphone_core_get_calls_nb(LC) > 1 && action.isOnHold) {
		linphone_core_pause_all_calls(LC);
        LOGD(@"CallKit: Pausing all ongoing calls");
		return;
	}

	NSUUID *uuid = action.callUUID;
	NSString *callID = [self.calls objectForKey:uuid];
	if (!callID) {
		return;
	}

    LOGD(@"CallKit: Call  with call-id: [%@] and UUID: [%@] paused status changed to: []", callID, uuid, action.isOnHold ? @"Paused" : @"Resumed");
	LinphoneCall *call = [LinphoneManager.instance callByCallId:callID];
	if (!call)
        return;

	if (action.isOnHold) {
		LinphoneManager.instance.speakerBeforePause = LinphoneManager.instance.speakerEnabled;
		linphone_call_pause((LinphoneCall *)call);
	} else {
		if (linphone_core_get_conference(LC)) {
			linphone_core_enter_conference(LC);
			[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallUpdate object:self];
		} else {
			[self configAudioSession:[AVAudioSession sharedInstance]];
			self.pendingCall = call;
		}
	}
}

- (void)provider:(CXProvider *)provider performPlayDTMFCallAction:(CXPlayDTMFCallAction *)action {
	[action fulfill];
	NSUUID *uuid = action.callUUID;
	NSString *callID = [self.calls objectForKey:uuid];
    LOGD(@"CallKit: playing DTMF for call with call-id: [%@] and UUID: [%@]", callID, uuid);
	LinphoneCall *call = [LinphoneManager.instance callByCallId:callID];
	char digit = action.digits.UTF8String[0];
	linphone_call_send_dtmf((LinphoneCall *)call, digit);
}

- (void)provider:(CXProvider *)provider didActivateAudioSession:(AVAudioSession *)audioSession {
	LOGD(@"CallKit: Audio session activated");
	// Now we can (re)start the call
	if (self.pendingCall) {
		LinphoneCallState state = linphone_call_get_state(self.pendingCall);
		switch (state) {
			case LinphoneCallIncomingReceived:
				[LinphoneManager.instance acceptCall:(LinphoneCall *)self.pendingCall evenWithVideo:_pendingCallVideo];
				break;
			case LinphoneCallPaused:
				linphone_call_resume((LinphoneCall *)self.pendingCall);
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
			LOGE(@"CallKit: No pending call");
		}
	}

    [self setPendingCall:NULL];
	if (_pendingAddr)
		linphone_address_unref(_pendingAddr);
	_pendingAddr = NULL;
	_pendingCallVideo = FALSE;
}

- (void)provider:(CXProvider *)provider didDeactivateAudioSession:(nonnull AVAudioSession *)audioSession {
	LOGD(@"CallKit : Audio session deactivated");
    [self setPendingCall:NULL];
	if (_pendingAddr)
		linphone_address_unref(_pendingAddr);
	_pendingAddr = NULL;
	_pendingCallVideo = FALSE;
}

- (void)providerDidReset:(CXProvider *)provider {
	LOGD(@"CallKit: Provider reset");
	LinphoneManager.instance.conf = TRUE;
	linphone_core_terminate_all_calls(LC);
	[self.calls removeAllObjects];
	[self.uuids removeAllObjects];
}

#pragma mark - CXCallObserverDelegate Protocol

- (void)callObserver:(CXCallObserver *)callObserver callChanged:(CXCall *)call {
	LOGD(@"CallKit: Call changed");
}

#pragma mark - CallKit decline with message

static NSString *pendingRejectionMessage;

void chat_room_state_changed(LinphoneChatRoom *cr, LinphoneChatRoomState newState) {
	[SVProgressHUD dismiss];
	switch (newState) {
		case LinphoneChatRoomStateCreated: {
			LOGI(@"Chat room [%p] created on server.", cr);
			linphone_chat_room_remove_callbacks(cr, linphone_chat_room_get_current_callbacks(cr));
			LinphoneChatMessage *m = linphone_chat_room_create_message(cr, pendingRejectionMessage.UTF8String);
			linphone_chat_room_send_chat_message(cr, m);
			break;
		}
		default:
			break;
	}
}

-(void) displayError:(NSString *)message {
	UIAlertController *errView =
	[UIAlertController alertControllerWithTitle:NSLocalizedString(@"Error", nil)
										message:NSLocalizedString(message, nil)
								 preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK"
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction *action) {}];
	[errView addAction:defaultAction];
	[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
}

-(void) declineCall:(LinphoneCall *)_call withRejectionMessage:(NSString *)rejectionMessage {
	const LinphoneAddress *addr = linphone_call_get_remote_address(_call);
	const LinphoneAddress *local = linphone_proxy_config_get_contact(linphone_core_get_default_proxy_config(LC));
	LinphoneChatRoom *chatRoom = linphone_core_find_one_to_one_chat_room_2(LC, local, addr,true);
	if (!chatRoom) {
		bctbx_list_t *addresses = bctbx_list_new((void*)addr);
		LinphoneChatRoomParams *param = linphone_core_create_default_chat_room_params(LC);
		linphone_chat_room_params_enable_group(param, NO);
		linphone_chat_room_params_enable_encryption(param, YES);
		chatRoom = linphone_core_create_chat_room_2(LC, param, LINPHONE_DUMMY_SUBJECT, addresses);
		if (!chatRoom) {
			[SVProgressHUD dismiss];
			LOGI(@"Failed sending rejection message as unable to create chat room : RM = %@ ",rejectionMessage);
			[self displayError:@"An error occured sending message."];
			linphone_call_terminate((LinphoneCall *)_call);
			return;
		}
		[SVProgressHUD show];
		pendingRejectionMessage = rejectionMessage;
		LinphoneChatRoomCbs *cbs = linphone_factory_create_chat_room_cbs(linphone_factory_get());
		linphone_chat_room_cbs_set_state_changed(cbs, chat_room_state_changed);
		linphone_chat_room_add_callbacks(chatRoom, cbs);
		bctbx_list_free_with_data(addresses, (void (*)(void *))linphone_address_unref);
	} else {
		LinphoneChatMessage *m = linphone_chat_room_create_message(chatRoom, rejectionMessage.UTF8String);
		linphone_chat_room_send_chat_message(chatRoom, m);
		linphone_call_terminate((LinphoneCall *)_call);
	}
}

@end
