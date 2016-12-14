/* UIPauseButton.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "UIPauseButton.h"
#import "LinphoneManager.h"
#import "Utils.h"

@implementation UIPauseButton

#pragma mark - Lifecycle Functions

- (void)initUIPauseButton {
	type = UIPauseButtonType_CurrentCall;
}

- (id)init {
	self = [super init];
	if (self) {
		[self initUIPauseButton];
	}
	return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
	self = [super initWithCoder:decoder];
	if (self) {
		[self initUIPauseButton];
	}
	return self;
}

- (id)initWithFrame:(CGRect)frame {
	self = [super initWithFrame:frame];
	if (self) {
		[self initUIPauseButton];
	}
	return self;
}

#pragma mark - Static Functions

+ (bool)isInConference:(LinphoneCall *)call {
	if (!call)
		return false;
	return linphone_call_params_get_local_conference_mode(linphone_call_get_current_params(call));
}

+ (LinphoneCall *)getCall {
	LinphoneCall *currentCall = linphone_core_get_current_call(LC);
	if (currentCall == nil && linphone_core_get_calls_nb(LC) == 1) {
		currentCall = (LinphoneCall *)linphone_core_get_calls(LC)->data;
	}
	return currentCall;
}

#pragma mark -

- (void)setType:(UIPauseButtonType)atype call:(LinphoneCall *)acall {
	type = atype;
	call = acall;
}

#pragma mark - UIToggleButtonDelegate Functions

- (void)onOn {
	switch (type) {
		case UIPauseButtonType_Call: {
			if (call != nil) {
				if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
					NSUUID *uuid = (NSUUID *)[LinphoneManager.instance.providerDelegate.uuids
						objectForKey:[NSString stringWithUTF8String:linphone_call_log_get_call_id(
																		linphone_call_get_call_log(call))]];
					if (!uuid) {
						linphone_core_pause_call(LC, call);
						return;
					}
					CXSetHeldCallAction *act = [[CXSetHeldCallAction alloc] initWithCallUUID:uuid onHold:YES];
					CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
					[LinphoneManager.instance.providerDelegate.controller requestTransaction:tr
																				  completion:^(NSError *err){
																				  }];
				} else {
					linphone_core_pause_call(LC, call);
				}
			} else {
				LOGW(@"Cannot toggle pause buttton, because no current call");
			}
			break;
		}
		case UIPauseButtonType_Conference: {
			linphone_core_leave_conference(LC);

			// Fake event
			[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallUpdate object:self];
			break;
		}
		case UIPauseButtonType_CurrentCall: {
			LinphoneCall *currentCall = [UIPauseButton getCall];
			if (currentCall != nil) {
				if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
					NSUUID *uuid = (NSUUID *)[LinphoneManager.instance.providerDelegate.uuids
						objectForKey:[NSString stringWithUTF8String:linphone_call_log_get_call_id(
																		linphone_call_get_call_log(currentCall))]];
					if (!uuid) {
						linphone_core_pause_call(LC, currentCall);
						return;
					}
					CXSetHeldCallAction *act = [[CXSetHeldCallAction alloc] initWithCallUUID:uuid onHold:YES];
					CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
					[LinphoneManager.instance.providerDelegate.controller requestTransaction:tr
																				  completion:^(NSError *err){
																				  }];
				} else {
					linphone_core_pause_call(LC, currentCall);
				}
			} else {
				LOGW(@"Cannot toggle pause buttton, because no current call");
			}
			break;
		}
	}
}

- (void)onOff {
	switch (type) {
		case UIPauseButtonType_Call: {
			if (call != nil) {
				if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
					NSUUID *uuid = (NSUUID *)[LinphoneManager.instance.providerDelegate.uuids
						objectForKey:[NSString stringWithUTF8String:linphone_call_log_get_call_id(
																		linphone_call_get_call_log(call))]];
					if (!uuid) {
						linphone_core_resume_call(LC, call);
						return;
					}
					CXSetHeldCallAction *act = [[CXSetHeldCallAction alloc] initWithCallUUID:uuid onHold:NO];
					CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
					[LinphoneManager.instance.providerDelegate.controller requestTransaction:tr
																				  completion:^(NSError *err){
																				  }];
				} else {
					linphone_core_resume_call(LC, call);
				}
			} else {
				LOGW(@"Cannot toggle pause buttton, because no current call");
			}
			break;
		}
		case UIPauseButtonType_Conference: {
			if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
				NSUUID *uuid = (NSUUID *)[LinphoneManager.instance.providerDelegate.uuids allValues].firstObject;
				if (!uuid) {
					linphone_core_enter_conference(LC);
					// Fake event
					[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallUpdate object:self];
					return;
				}
				CXSetHeldCallAction *act = [[CXSetHeldCallAction alloc] initWithCallUUID:uuid onHold:NO];
				CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
				[LinphoneManager.instance.providerDelegate.controller requestTransaction:tr
																			  completion:^(NSError *err){
																			  }];
			} else {
				linphone_core_enter_conference(LC);
				// Fake event
				[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneCallUpdate object:self];
			}
			break;
		}
		case UIPauseButtonType_CurrentCall: {
			LinphoneCall *currentCall = [UIPauseButton getCall];
			if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
				NSUUID *uuid = (NSUUID *)[LinphoneManager.instance.providerDelegate.uuids
					objectForKey:[NSString stringWithUTF8String:linphone_call_log_get_call_id(
																	linphone_call_get_call_log(currentCall))]];
				if (!uuid) {
					linphone_core_resume_call(LC, currentCall);
					return;
				}
				CXSetHeldCallAction *act = [[CXSetHeldCallAction alloc] initWithCallUUID:uuid onHold:NO];
				CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
				[LinphoneManager.instance.providerDelegate.controller requestTransaction:tr
																			  completion:^(NSError *err){
																			  }];
			} else {
				linphone_core_resume_call(LC, currentCall);
			}
			break;
		}
	}
}

- (bool)onUpdate {
	bool ret = false;
	LinphoneCall *c = call;
	switch (type) {
		case UIPauseButtonType_Conference: {
			self.enabled = (linphone_core_get_conference_size(LC) > 0);
			if (self.enabled) {
				ret = (!linphone_core_is_in_conference(LC));
			}
			break;
		}
		case UIPauseButtonType_CurrentCall:
			c = [UIPauseButton getCall];
		case UIPauseButtonType_Call: {
			if (c != nil) {
				LinphoneCallState state = linphone_call_get_state(c);
				ret = (state == LinphoneCallPaused || state == LinphoneCallPausing);
				self.enabled = !linphone_core_sound_resources_locked(LC) &&
							   (state == LinphoneCallPaused || state == LinphoneCallPausing ||
								state == LinphoneCallStreamsRunning);
			} else {
				self.enabled = FALSE;
			}
			break;
		}
	}
	return ret;
}

@end
