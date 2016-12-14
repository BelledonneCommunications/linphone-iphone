/* OutgoingCallViewController.m
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
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(bluetoothAvailabilityUpdateEvent:)
											   name:kLinphoneBluetoothAvailabilityUpdate
											 object:nil];

	LinphoneCall *call = linphone_core_get_current_call(LC);
	if (!call) {
		return;
	}

	const LinphoneAddress *addr = linphone_call_get_remote_address(call);
	[ContactDisplay setDisplayNameLabel:_nameLabel forAddress:addr];
	char *uri = linphone_address_as_string_uri_only(addr);
	_addressLabel.text = [NSString stringWithUTF8String:uri];
	ms_free(uri);
	[_avatarImage setImage:[FastAddressBook imageForAddress:addr thumbnail:NO] bordered:YES withRoundedRadius:YES];

	[self hideSpeaker:LinphoneManager.instance.bluetoothAvailable];

	[_speakerButton update];
	[_microButton update];
	[_routesButton update];
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
	[LinphoneManager.instance setBluetoothEnabled:TRUE];
}

- (IBAction)onRoutesEarpieceClick:(id)sender {
	[self hideRoutes:TRUE animated:TRUE];
	[LinphoneManager.instance setSpeakerEnabled:FALSE];
	[LinphoneManager.instance setBluetoothEnabled:FALSE];
}

- (IBAction)onRoutesSpeakerClick:(id)sender {
	[self hideRoutes:TRUE animated:TRUE];
	[LinphoneManager.instance setSpeakerEnabled:TRUE];
}

- (IBAction)onRoutesClick:(id)sender {
	if ([_routesView isHidden]) {
		[self hideRoutes:FALSE animated:ANIMATED];
	} else {
		[self hideRoutes:TRUE animated:ANIMATED];
	}
}

- (IBAction)onDeclineClick:(id)sender {
	LinphoneCall *call = linphone_core_get_current_call(LC);
	if (call) {
		/*if (floor(NSFoundationVersionNumber) > NSFoundationVersionNumber_iOS_9_x_Max) {
			NSUUID *uuid = [LinphoneManager.instance.providerDelegate.uuids objectForKey:[NSString
		stringWithUTF8String:linphone_call_log_get_call_id(linphone_call_get_call_log(call))]];
			if(!uuid) {
				linphone_core_terminate_call(LC, call);
				return;
			}
			CXEndCallAction *act = [[CXEndCallAction alloc] initWithCallUUID:uuid];
			CXTransaction *tr = [[CXTransaction alloc] initWithAction:act];
			[LinphoneManager.instance.providerDelegate.controller requestTransaction:tr completion:^(NSError *err){}];
		} else {*/
		linphone_core_terminate_call(LC, call);
		//}
	}
}

- (void)hideRoutes:(BOOL)hidden animated:(BOOL)animated {
	if (hidden) {
		[_routesButton setOff];
	} else {
		[_routesButton setOn];
	}

	_routesBluetoothButton.selected = LinphoneManager.instance.bluetoothEnabled;
	_routesSpeakerButton.selected = LinphoneManager.instance.speakerEnabled;
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
	[self hideSpeaker:available];
}

@end
