/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import "CallOutgoingView.h"
#import "PhoneMainView.h"

@implementation CallOutgoingView

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:nil
															   sideMenu:CallSideMenuView.class
															 fullscreen:false
														 isLeftFragment:NO
														   fragmentWith:nil];

		compositeDescription.darkBackground = true;
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

- (void)viewDidLoad {
	_routesEarpieceButton.enabled = !IPAD;
	
	[_zeroButton setDigit:'0'];
	[_zeroButton setDtmf:true];
	[_oneButton setDigit:'1'];
	[_oneButton setDtmf:true];
	[_twoButton setDigit:'2'];
	[_twoButton setDtmf:true];
	[_threeButton setDigit:'3'];
	[_threeButton setDtmf:true];
	[_fourButton setDigit:'4'];
	[_fourButton setDtmf:true];
	[_fiveButton setDigit:'5'];
	[_fiveButton setDtmf:true];
	[_sixButton setDigit:'6'];
	[_sixButton setDtmf:true];
	[_sevenButton setDigit:'7'];
	[_sevenButton setDtmf:true];
	[_eightButton setDigit:'8'];
	[_eightButton setDtmf:true];
	[_nineButton setDigit:'9'];
	[_nineButton setDtmf:true];
	[_starButton setDigit:'*'];
	[_starButton setDtmf:true];
	[_hashButton setDigit:'#'];
	[_hashButton setDtmf:true];
	
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(bluetoothAvailabilityUpdateEvent:)
											   name:kLinphoneBluetoothAvailabilityUpdate
											 object:nil];
	
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(callUpdateEvent:)
											   name:kLinphoneCallUpdate
											 object:nil];

	LinphoneCall *call = linphone_core_get_current_call(LC);
	if (!call) {
		return;
	}

	const LinphoneAddress *addr = linphone_call_get_remote_address(call);
	[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr withAddressLabel:_addressLabel];
	char *uri = linphone_address_as_string_uri_only(addr);
	ms_free(uri);
	[_avatarImage setImage:[FastAddressBook imageForAddress:addr] bordered:NO withRoundedRadius:YES];

	[self hideSpeaker:LinphoneManager.instance.bluetoothAvailable];

	[_speakerButton update];
	[_microButton update];
	[_routesButton update];
	[self hidePad:TRUE animated:FALSE];
	[self callUpdate:call state:linphone_call_get_state(call) animated:true];

}

- (void)viewDidAppear:(BOOL)animated {
	[super viewDidAppear:animated];
	// if there is no call (for whatever reason), we must wait viewDidAppear method
	// before popping current view, because UICompositeView cannot handle view change
	// directly in viewWillAppear (this would lead to crash in deallocated memory - easily
	// reproductible on iPad mini).
	if (!linphone_core_get_current_call(LC)) {
		[PhoneMainView.instance popCurrentView];
	}
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

- (IBAction)onRoutesBluetoothClick:(id)sender {
	[self hideRoutes:TRUE animated:TRUE];
	[CallManager.instance changeRouteToBluetooth];
}

- (IBAction)onRoutesEarpieceClick:(id)sender {
	[self hideRoutes:TRUE animated:TRUE];
	[CallManager.instance changeRouteToDefault];
}

- (IBAction)onRoutesSpeakerClick:(id)sender {
	[self hideRoutes:TRUE animated:TRUE];
	[CallManager.instance changeRouteToSpeaker];
}

- (IBAction)onRoutesClick:(id)sender {
	if ([_routesView isHidden]) {
		[self hideRoutes:FALSE animated:ANIMATED];
	} else {
		[self hideRoutes:TRUE animated:ANIMATED];
	}
}

- (IBAction)onNumpadClick:(id)sender {
	if ([_numpadView isHidden]) {
		[self hidePad:FALSE animated:ANIMATED];
	} else {
		[self hidePad:TRUE animated:ANIMATED];
	}
}

- (IBAction)onDeclineClick:(id)sender {
	LinphoneCall *call = linphone_core_get_current_call(LC);
	if (call) {
		[CallManager.instance terminateCallWithCall:call];
	}
}

- (void)hidePad:(BOOL)hidden animated:(BOOL)animated {
	if (hidden) {
		[_numpadButton setOff];
	} else {
		[_numpadButton setOn];
	}
	if (hidden != _numpadView.hidden) {
		if (animated) {
			[self hideAnimation:hidden forView:_numpadView completion:nil];
		} else {
			[_numpadView setHidden:hidden];
		}
	}
}

- (void)hideRoutes:(BOOL)hidden animated:(BOOL)animated {
	if (hidden) {
		[_routesButton setOff];
	} else {
		[_routesButton setOn];
	}

	_routesBluetoothButton.selected = [CallManager.instance isBluetoothEnabled];
	_routesSpeakerButton.selected = [CallManager.instance isSpeakerEnabled];
	_routesEarpieceButton.selected = !_routesBluetoothButton.selected && !_routesSpeakerButton.selected;

	if (hidden != _routesView.hidden) {
		[_routesView setHidden:hidden];
	}
}

- (void)hideSpeaker:(BOOL)hidden {
	_speakerButton.hidden = hidden;
	_routesButton.hidden = !hidden;
}

#pragma mark - Event Functions

- (void)bluetoothAvailabilityUpdateEvent:(NSNotification *)notif {
	bool available = [[notif.userInfo objectForKey:@"available"] intValue];
	dispatch_async(dispatch_get_main_queue(), ^{
		[self hideSpeaker:available];
	});
}

- (void)callUpdateEvent:(NSNotification *)notif {
	LinphoneCall *call = [[notif.userInfo objectForKey:@"call"] pointerValue];
	LinphoneCallState state = [[notif.userInfo objectForKey:@"state"] intValue];
	[self callUpdate:call state:state animated:TRUE];
}

- (void)callUpdate:(LinphoneCall *)call state:(LinphoneCallState)state animated:(BOOL)animated {
	_declineButton_earlyMedia.hidden = linphone_call_get_state(call) != LinphoneCallStateOutgoingEarlyMedia;
	_declineButton.hidden = !_declineButton_earlyMedia.hidden;
	_numpadButton.hidden = _declineButton_earlyMedia.hidden;
}

#pragma mark - Animation

- (void)hideAnimation:(BOOL)hidden forView:(UIView *)target completion:(void (^)(BOOL finished))completion {
	if (hidden) {
	int original_y = target.frame.origin.y;
	CGRect newFrame = target.frame;
	newFrame.origin.y = self.view.frame.size.height;
	[UIView animateWithDuration:0.5
		delay:0.0
		options:UIViewAnimationOptionCurveEaseIn
		animations:^{
		  target.frame = newFrame;
		}
		completion:^(BOOL finished) {
		  CGRect originFrame = target.frame;
		  originFrame.origin.y = original_y;
		  target.hidden = YES;
		  target.frame = originFrame;
		  if (completion)
			  completion(finished);
		}];
	} else {
		CGRect frame = target.frame;
		int original_y = frame.origin.y;
		frame.origin.y = self.view.frame.size.height;
		target.frame = frame;
		frame.origin.y = original_y;
		target.hidden = NO;

		[UIView animateWithDuration:0.5
			delay:0.0
			options:UIViewAnimationOptionCurveEaseOut
			animations:^{
			  target.frame = frame;
			}
			completion:^(BOOL finished) {
			  target.frame = frame; // in case application did not finish
			  if (completion)
				  completion(finished);
			}];
	}
}


@end
