/* HistoryDetailsViewController.m
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

#import "HistoryDetailsView.h"
#import "PhoneMainView.h"
#import "FastAddressBook.h"

@implementation HistoryDetailsView

#pragma mark - UICompositeViewDelegate Functions

static UICompositeViewDescription *compositeDescription = nil;

+ (UICompositeViewDescription *)compositeViewDescription {
	if (compositeDescription == nil) {
		compositeDescription = [[UICompositeViewDescription alloc] init:self.class
															  statusBar:StatusBarView.class
																 tabBar:TabBarView.class
															   sideMenu:SideMenuView.class
															 fullscreen:false
														 isLeftFragment:NO
														   fragmentWith:HistoryListView.class];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

#pragma mark - Property Functions

- (void)setCallLogId:(NSString *)acallLogId {
	_callLogId = [acallLogId copy];
	[self update];
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
	[super viewDidLoad];

	// if we use fragments, remove back button
	if (IPAD) {
		_backButton.hidden = YES;
		_backButton.alpha = 0;
		
	}

	UITapGestureRecognizer *headerTapGesture =
		[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onContactClick:)];
	[_headerView addGestureRecognizer:headerTapGesture];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	[self update];

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(update)
											   name:kLinphoneAddressBookUpdate
											 object:nil];

	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(coreUpdateEvent:)
											   name:kLinphoneCoreUpdate
											 object:nil];
	
	[[NSNotificationCenter defaultCenter] addObserver: self
											 selector: @selector(deviceOrientationDidChange:)
												 name: UIDeviceOrientationDidChangeNotification
											   object: nil];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[NSNotificationCenter.defaultCenter removeObserver:self];
}

#pragma mark - Event Functions

- (void)coreUpdateEvent:(NSNotification *)notif {
	[self update];
}

- (void) deviceOrientationDidChange:(NSNotification*) notif {
	if (IPAD) {
		[self update];
	}
}

#pragma mark -

- (void)update {
	// Look for the call log
	callLog = NULL;
	if (_callLogId) {
		const MSList *list = linphone_core_get_call_logs(LC);
		while (list != NULL) {
			LinphoneCallLog *log = (LinphoneCallLog *)list->data;
			const char *cid = linphone_call_log_get_call_id(log);
			if (cid != NULL && [_callLogId isEqualToString:[NSString stringWithUTF8String:cid]]) {
				callLog = log;
				break;
			}
			list = list->next;
		}
	}

	// Pop if callLog is null
	if (callLog == NULL) {
		_emptyLabel.hidden = NO;
		_addContactButton.hidden = YES;
		return;
	}
	_emptyLabel.hidden = YES;

	LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
	_addContactButton.hidden = ([FastAddressBook getContactWithAddress:addr] != nil);
	[ContactDisplay setDisplayNameLabel:_contactLabel forAddress:addr];
	[_avatarImage setImage:[FastAddressBook imageForAddress:addr thumbnail:NO] bordered:NO withRoundedRadius:YES];
	char *addrURI = linphone_address_as_string_uri_only(addr);
	_addressLabel.text = [NSString stringWithUTF8String:addrURI];
	ms_free(addrURI);

	[_tableView loadDataForAddress:(callLog ? linphone_call_log_get_remote_address(callLog) : NULL)];
}

#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
	HistoryListView *view = VIEW(HistoryListView);
	[PhoneMainView.instance popToView:view.compositeViewDescription];
}

- (IBAction)onContactClick:(id)event {
	LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
	Contact *contact = [FastAddressBook getContactWithAddress:addr];
	if (contact) {
		ContactDetailsView *view = VIEW(ContactDetailsView);
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
		[ContactSelection setSelectionMode:ContactSelectionModeNone];
		[view setContact:contact];
	}
}

- (IBAction)onAddContactClick:(id)event {
	LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
	char *lAddress = linphone_address_as_string_uri_only(addr);
	if (lAddress != NULL) {
		[ContactSelection setAddAddress:[NSString stringWithUTF8String:lAddress]];
		[ContactSelection setSelectionMode:ContactSelectionModeEdit];

		[ContactSelection setSipFilter:nil];
		[ContactSelection enableEmailFilter:FALSE];
		[ContactSelection setNameOrEmailFilter:nil];
		[PhoneMainView.instance changeCurrentView:ContactsListView.compositeViewDescription];
		ms_free(lAddress);
	}
}

- (IBAction)onCallClick:(id)event {
	LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
	[LinphoneManager.instance call:addr];
}

- (IBAction)onChatClick:(id)event {
	const LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
	if (addr == NULL)
		return;
	ChatConversationView *view = VIEW(ChatConversationView);
	LinphoneChatRoom *room = linphone_core_get_chat_room(LC, addr);
	[view setChatRoom:room];
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

@end
