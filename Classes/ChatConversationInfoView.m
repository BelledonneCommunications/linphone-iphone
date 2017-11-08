//
//  ChatConversationInfoView.m
//  linphone
//
//  Created by REIS Benjamin on 23/10/2017.
//

#import <Foundation/Foundation.h>

#import "ChatConversationInfoView.h"
#import "PhoneMainView.h"
#import "UIChatConversationInfoTableViewCell.h"

#import "linphone/core.h"

@implementation ChatConversationInfoView

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
														   fragmentWith:ChatsListView.class];
	}
	return compositeDescription;
}

- (UICompositeViewDescription *)compositeViewDescription {
	return self.class.compositeViewDescription;
}

- (void)viewDidLoad {
	[super viewDidLoad];
	// if we use fragments, remove back button
	if (IPAD) {
		_backButton.hidden = YES;
	}
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc]
									initWithTarget:self
									action:@selector(dismissKeyboards)];
	tap.delegate = self;
	[self.view addGestureRecognizer:tap];
	_nameLabel.delegate = self;
	_tableView.dataSource = self;
	_tableView.delegate	= self;
	_admins = [[NSMutableArray alloc] init];
	_oldAdmins = [[NSMutableArray alloc] init];
	_oldContacts = [[NSMutableDictionary alloc] init];
	_room = NULL;
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	_nextButton.enabled = _nameLabel.text.length > 0 && _contacts.count > 0;
	[_tableView reloadData];
	_quitButton.hidden = _create;
}

#pragma mark - next functions

- (void)onCreate {
	LinphoneChatRoom *room = linphone_core_create_client_group_chat_room(LC, _nameLabel.text.UTF8String);
	if(!room) {
		return;
	}
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(room);
	linphone_chat_room_cbs_set_state_changed(cbs, chat_room_state_changed);
	linphone_chat_room_cbs_set_user_data(cbs, (__bridge void*)self);
	bctbx_list_t *addresses = NULL;
	for(NSString *addr in _contacts.allKeys) {
		LinphoneAddress *linphoneAddress = linphone_address_new(addr.UTF8String);
		if (!linphoneAddress)
			continue;

		if (!addresses) {
			addresses = bctbx_list_new((void *)linphoneAddress);
			continue;
		}
		addresses = bctbx_list_append(addresses, (void *)linphoneAddress);
	}
	linphone_chat_room_add_participants(room, addresses);
	bctbx_list_free_with_data(addresses, (void (*)(void *))linphone_address_unref);
}

- (void)onValidate {
	ChatConversationView *view = VIEW(ChatConversationView);
	// Change subject if necessary
	if (![_oldSubject isEqualToString:_nameLabel.text])
		linphone_chat_room_set_subject(_room, _nameLabel.text.UTF8String);

	// Remove participants if necessary
	for (NSString *uri in _oldContacts.allKeys) {
		if ([_contacts objectForKey:uri])
			continue;

		LinphoneAddress *addr = linphone_address_new(uri.UTF8String);
		linphone_chat_room_remove_participant(_room, linphone_chat_room_find_participant(_room, addr));
		linphone_address_unref(addr);
	}

	// add admins if necessary
	for (NSString *admin in _admins) {
		if ([_oldAdmins containsObject:admin])
			continue;

		LinphoneAddress *addr = linphone_address_new(admin.UTF8String);
		linphone_chat_room_set_participant_admin_status(_room, linphone_chat_room_find_participant(_room, linphone_address_new(admin.UTF8String)), true);
		linphone_address_unref(addr);
	}

	// remove admins if necessary
	for (NSString *admin in _oldAdmins) {
		if ([_admins containsObject:admin])
			continue;

		LinphoneAddress *addr = linphone_address_new(admin.UTF8String);
		linphone_chat_room_set_participant_admin_status(_room, linphone_chat_room_find_participant(_room, linphone_address_new(admin.UTF8String)), false);
		linphone_address_unref(addr);
	}
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

#pragma mark - Buttons responders

- (IBAction)onNextClick:(id)sender {
	if(_create)
		[self onCreate];
	else
		[self onValidate];
}

- (IBAction)onBackClick:(id)sender {
	if(_create) {
		ChatConversationCreateView *view = VIEW(ChatConversationCreateView);
		view.tableController.contactsDict = _contacts;
		view.tableController.contactsGroup = [[_contacts allKeys] mutableCopy];
		view.tableController.notFirstTime = TRUE;
		view.isForEditing = FALSE;
		[PhoneMainView.instance popToView:view.compositeViewDescription];
	} else {
		ChatConversationView *view = VIEW(ChatConversationView);
		[PhoneMainView.instance popToView:view.compositeViewDescription];
	}
}

- (IBAction)onQuitClick:(id)sender {
	LinphoneChatRoomCbs *cbs = linphone_chat_room_get_callbacks(_room);
	linphone_chat_room_cbs_set_user_data(cbs, (__bridge void*)self);
	linphone_chat_room_cbs_set_state_changed(cbs, chat_room_state_changed);
	linphone_chat_room_leave(_room);
}

- (IBAction)onAddClick:(id)sender {
	ChatConversationCreateView *view = VIEW(ChatConversationCreateView);
	view.tableController.notFirstTime = TRUE;
	view.isForEditing = !_create;
	[view.tableController.contactsDict removeAllObjects];
	[view.tableController.contactsGroup removeAllObjects];
	[PhoneMainView.instance popToView:view.compositeViewDescription];
}

#pragma mark - TableView

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return _contacts.count;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	NSString *kCellId = NSStringFromClass(UIChatConversationInfoTableViewCell.class);
	UIChatConversationInfoTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIChatConversationInfoTableViewCell alloc] initWithIdentifier:kCellId];
	}
	cell.uri = _contacts.allKeys[indexPath.row];
	LinphoneAddress *addr = linphone_address_new(cell.uri.UTF8String);
	cell.nameLabel.text = [FastAddressBook displayNameForAddress:addr];
	cell.controllerView = self;
	if(![_admins containsObject:cell.uri]) {
		cell.adminLabel.enabled	= FALSE;
		cell.adminImage.image = [UIImage imageNamed:@"check_unselected.png"];
	}
	cell.adminButton.hidden = _create;
	linphone_address_unref(addr);
	return cell;
}

#pragma mark - searchBar delegate

- (void)dismissKeyboards {
	if ([_nameLabel isFirstResponder]) {
		[_nameLabel resignFirstResponder];
	}
}

#pragma mark - UITextFieldDelegate

- (BOOL)textField:(UITextField *)textField shouldChangeCharactersInRange:(NSRange)range replacementString:(NSString *)string {
	_nextButton.enabled = (!((string.length == 0 || string == nil || [string isEqual:@""]) && (textField.text.length == 1))
						   && _contacts.count > 0);
	return TRUE;
}

#pragma mark - chat room callbacks

- (void)goToChatRoom:(LinphoneChatRoom *)cr {
	ChatConversationView *view = VIEW(ChatConversationView);
	view.chatRoom = cr;
	[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
}

void chat_room_state_changed(LinphoneChatRoom *cr, LinphoneChatRoomState newState) {
	switch (newState) {
		case LinphoneChatRoomStateCreated:
			LOGI(@"Chat room [%p] created on server.", cr);
			[(__bridge ChatConversationInfoView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_callbacks(cr)) goToChatRoom:cr];
			break;
		case LinphoneChatRoomStateCreationFailed:
			LOGE(@"Chat room [%p] could not be created on server.", cr);
			break;
		case LinphoneChatRoomStateTerminated:
			LOGI(@"Chat room [%p] has been terminated.", cr);
			[(__bridge ChatConversationInfoView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_callbacks(cr)) goToChatRoom:cr];
			break;
		default:
			break;
	}
}

@end
