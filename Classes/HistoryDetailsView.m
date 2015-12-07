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
														  landscapeMode:LinphoneManager.runningOnIpad
														   portraitMode:true];
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

	UITapGestureRecognizer *headerTapGesture =
		[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onContactClick:)];
	[_headerView addGestureRecognizer:headerTapGesture];
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];

	[_tableView loadDataForAddress:(callLog ? linphone_call_log_get_remote_address(callLog) : NULL)];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(update)
												 name:kLinphoneAddressBookUpdate
											   object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(coreUpdateEvent:)
												 name:kLinphoneCoreUpdate
											   object:nil];
}

- (void)viewWillDisappear:(BOOL)animated {
	[super viewWillDisappear:animated];
	[[NSNotificationCenter defaultCenter] removeObserver:self];
}

#pragma mark - Event Functions

- (void)coreUpdateEvent:(NSNotification *)notif {
	[self update];
}

#pragma mark -

+ (void)adaptSize:(UILabel *)label field:(UIView *)field {
	//
	// Adapt size
	//
	CGRect labelFrame = [label frame];
	CGRect fieldFrame = [field frame];

	fieldFrame.origin.x -= labelFrame.size.width;

	// Compute firstName size
	CGSize contraints;
	contraints.height = [label frame].size.height;
	contraints.width = ([field frame].size.width + [field frame].origin.x) - [label frame].origin.x;
	CGSize firstNameSize = [[label text] sizeWithFont:[label font] constrainedToSize:contraints];
	labelFrame.size.width = firstNameSize.width;

	// Compute lastName size & position
	fieldFrame.origin.x += labelFrame.size.width;
	fieldFrame.size.width = (contraints.width + [label frame].origin.x) - fieldFrame.origin.x;

	[label setFrame:labelFrame];
	[field setFrame:fieldFrame];
}

- (void)retrieveCallLog {
	// Look for the call log
	callLog = NULL;
	const MSList *list = linphone_core_get_call_logs([LinphoneManager getLc]);
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

- (void)update {
	// Pop if callLog is null
	[self retrieveCallLog];
	if (callLog == NULL) {
		[PhoneMainView.instance popCurrentView];
		return;
	}

	LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
	_addContactButton.hidden = ([FastAddressBook getContactWithAddress:addr] != nil);
	[ContactDisplay setDisplayNameLabel:_contactLabel forAddress:addr];
	[_avatarImage setImage:[FastAddressBook imageForAddress:addr thumbnail:NO] bordered:NO withRoundedRadius:YES];
	char *addrURI = linphone_address_as_string_uri_only(addr);
	_addressLabel.text = [NSString stringWithUTF8String:addrURI];
	ms_free(addrURI);
}

#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
	HistoryListView *view = VIEW(HistoryListView);
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

- (IBAction)onContactClick:(id)event {
	LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
	ABRecordRef contact = [FastAddressBook getContactWithAddress:addr];
	if (contact) {
		ContactDetailsView *view = VIEW(ContactDetailsView);
		[PhoneMainView.instance changeCurrentView:view.compositeViewDescription push:TRUE];
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
		[PhoneMainView.instance changeCurrentView:ContactsListView.compositeViewDescription push:TRUE];
		ms_free(lAddress);
	}
}

- (IBAction)onCallClick:(id)event {
	LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
	char *lAddress = linphone_address_as_string_uri_only(addr);
	if (lAddress == NULL)
		return;
	NSString *displayName = [FastAddressBook displayNameForAddress:addr];

	DialerView *view = VIEW(DialerView);
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
	[view call:[NSString stringWithUTF8String:lAddress] displayName:displayName];
	ms_free(lAddress);
}

- (IBAction)onChatClick:(id)event {
	const LinphoneAddress *addr = linphone_call_log_get_remote_address(callLog);
	if (addr == NULL)
		return;
	[PhoneMainView.instance changeCurrentView:ChatsListView.compositeViewDescription];
	ChatConversationView *view = VIEW(ChatConversationView);
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription push:TRUE];
	LinphoneChatRoom *room = linphone_core_get_chat_room([LinphoneManager getLc], addr);
	[view setChatRoom:room];
}

@end
