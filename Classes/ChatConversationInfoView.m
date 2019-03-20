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

+ (void)displayCreationError {
	static UIAlertController *errorView = nil;
	// avoid having multiple popups
	[PhoneMainView.instance dismissViewControllerAnimated:YES completion:nil];
	errorView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Chat room creation error", nil)
													message:NSLocalizedString(@"Chat room could not be created on server", nil)
											 preferredStyle:UIAlertControllerStyleAlert];

	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {}];
	[errorView addAction:defaultAction];
	[PhoneMainView.instance presentViewController:errorView animated:YES completion:nil];
}

- (void)viewDidLoad {
	[super viewDidLoad];
	UITapGestureRecognizer *tap = [[UITapGestureRecognizer alloc] initWithTarget:self
																		  action:@selector(dismissKeyboards)];
	tap.delegate = self;
	[self.view addGestureRecognizer:tap];

	UITapGestureRecognizer *particpantsBarTap = [[UITapGestureRecognizer alloc] initWithTarget:self
																						action:@selector(onAddClick:)];
	particpantsBarTap.delegate = self;
	[_participantsBar addGestureRecognizer:particpantsBarTap];

	_nameLabel.delegate = self;
	_tableView.dataSource = self;
	_tableView.delegate	= self;
	_admins = [[NSMutableArray alloc] init];
	_oldAdmins = [[NSMutableArray alloc] init];
	_oldContacts = [[NSMutableArray alloc] init];
	_room = NULL;
	_chatRoomCbs = NULL;
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	_waitView.hidden = YES;

	if (_create)
		_room = NULL;

	_nameLabel.text = _room && linphone_chat_room_get_subject(_room)
		? [NSString stringWithUTF8String:linphone_chat_room_get_subject(_room)]
		: @"";
	_nextButton.enabled = _nameLabel.text.length > 0 && _contacts.count > 0;
	LinphoneParticipant *me = _room && !linphone_chat_room_has_been_left(_room)
		? linphone_chat_room_get_me(_room)
		: NULL;
	_imAdmin = me
		? linphone_participant_is_admin(me)
		: false;
	_quitButton.hidden = _create || (me == NULL);
	_nameLabel.enabled = _create || _imAdmin;
	_addButton.hidden = !_create && !_imAdmin;
	_nextButton.hidden = !_create && !_imAdmin;

	CGFloat height = _quitButton.hidden
		? self.view.frame.size.height - _tableView.frame.origin.y
		: _quitButton.frame.origin.y - _tableView.frame.origin.y - 10;
	[_tableView setFrame:CGRectMake(
		_tableView.frame.origin.x,
		_tableView.frame.origin.y,
		_tableView.frame.size.width,
		height
	)];

	if (_room) {
		_chatRoomCbs = linphone_factory_create_chat_room_cbs(linphone_factory_get());
		linphone_chat_room_cbs_set_state_changed(_chatRoomCbs, main_view_chat_room_state_changed);
		linphone_chat_room_cbs_set_subject_changed(_chatRoomCbs, chat_room_subject_changed);
		linphone_chat_room_cbs_set_participant_added(_chatRoomCbs, chat_room_participant_added);
		linphone_chat_room_cbs_set_participant_removed(_chatRoomCbs, chat_room_participant_removed);
		linphone_chat_room_cbs_set_participant_admin_status_changed(_chatRoomCbs, chat_room_participant_admin_status_changed);
		linphone_chat_room_cbs_set_user_data(_chatRoomCbs, (__bridge void*)self);
		linphone_chat_room_add_callbacks(_room, _chatRoomCbs);
	}

	[_tableView reloadData];
}

- (void)viewWillDisappear:(BOOL)animated {
	if (!_room || !_chatRoomCbs)
		return;

	linphone_chat_room_remove_callbacks(_room, _chatRoomCbs);
	_chatRoomCbs = NULL;
}

#pragma mark - next functions

- (void)onCreate {
	bctbx_list_t *addresses = NULL;
	for (NSString *addr in _contacts) {
		LinphoneAddress *linphoneAddress = linphone_address_new(addr.UTF8String);
		if (!linphoneAddress)
			continue;

		if (!addresses) {
			addresses = bctbx_list_new((void *)linphoneAddress);
			continue;
		}
		addresses = bctbx_list_append(addresses, (void *)linphoneAddress);
	}
	[PhoneMainView.instance createChatRoom:_nameLabel.text.UTF8String addresses:addresses andWaitView:_waitView isEncrypted:_encrypted isGroup:TRUE];
	bctbx_list_free_with_data(addresses, (void (*)(void *))linphone_address_unref);
}

- (void)onValidate {
	ChatConversationView *view = VIEW(ChatConversationView);
	// Change subject if necessary
	if (![_oldSubject isEqualToString:_nameLabel.text])
		linphone_chat_room_set_subject(_room, _nameLabel.text.UTF8String);

	// Add participants if necessary
	bctbx_list_t *addedPartipants = NULL;
	for (NSString *uri in _contacts) {
		if ([_oldContacts containsObject:uri])
			continue;

		LinphoneAddress *addr = linphone_address_new(uri.UTF8String);
		linphone_address_clean(addr);//keep only username@domain
		if (addedPartipants)
			addedPartipants = bctbx_list_append(addedPartipants, addr);
		else
			addedPartipants = bctbx_list_new(addr);
	}
	if (addedPartipants) {
		linphone_chat_room_add_participants(_room, addedPartipants);
		bctbx_list_free_with_data(addedPartipants, (void (*)(void *))linphone_address_unref);
	}


	// Remove participants if necessary
	bctbx_list_t *removedPartipants = NULL;
	for (NSString *uri in _oldContacts) {
		if ([_contacts containsObject:uri])
			continue;

		LinphoneAddress *addr = linphone_address_new(uri.UTF8String);
		LinphoneParticipant *participant = linphone_participant_ref(linphone_chat_room_find_participant(_room, addr));
		if (!participant)
			continue;

		if (removedPartipants)
			removedPartipants = bctbx_list_append(removedPartipants, participant);
		else
			removedPartipants = bctbx_list_new(participant);

		linphone_address_unref(addr);
	}
	if (removedPartipants) {
		linphone_chat_room_remove_participants(_room, removedPartipants);
		bctbx_list_free_with_data(removedPartipants, (void (*)(void *))linphone_participant_unref);
	}

	// add admins if necessary
	for (NSString *admin in _admins) {
		if ([_oldAdmins containsObject:admin])
			continue;

		LinphoneAddress *addr = linphone_address_new(admin.UTF8String);
		LinphoneParticipant *participant = linphone_chat_room_find_participant(_room, addr);
		if (!participant)
			continue;

		linphone_chat_room_set_participant_admin_status(_room, participant, true);
		linphone_address_unref(addr);
	}

	// remove admins if necessary
	for (NSString *admin in _oldAdmins) {
		if ([_admins containsObject:admin])
			continue;

		LinphoneAddress *addr = linphone_address_new(admin.UTF8String);
		LinphoneParticipant *participant = linphone_chat_room_find_participant(_room, addr);
		if (!participant)
			continue;

		linphone_chat_room_set_participant_admin_status(_room, participant, false);
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
		view.tableController.contactsGroup = [_contacts mutableCopy];
		view.tableController.notFirstTime = TRUE;
		view.isForEditing = FALSE;
		[PhoneMainView.instance popToView:view.compositeViewDescription];
	} else {
		ChatConversationView *view = VIEW(ChatConversationView);
		[PhoneMainView.instance popToView:view.compositeViewDescription];
	}
}

- (IBAction)onQuitClick:(id)sender {
	NSString *msg =
	[NSString stringWithFormat:NSLocalizedString(@"Do you want to leave this conversation?", nil)];
	[UIConfirmationDialog ShowWithMessage:msg
							cancelMessage:nil
						   confirmMessage:NSLocalizedString(@"LEAVE", nil)
							onCancelClick:^() {}
					  onConfirmationClick:^() {
						  linphone_chat_room_leave(_room);
					  }];
}

- (IBAction)onAddClick:(id)sender {
	ChatConversationCreateView *view = VIEW(ChatConversationCreateView);
	view.tableController.notFirstTime = TRUE;
	view.isForEditing = !_create;
    view.isGroupChat = TRUE;
	view.tableController.contactsGroup = [_contacts mutableCopy];
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
	cell.uri = _contacts[indexPath.row];
	LinphoneAddress *addr = linphone_address_new(cell.uri.UTF8String);
	cell.nameLabel.text = [FastAddressBook displayNameForAddress:addr];
	[cell.avatarImage setImage:[FastAddressBook imageForAddress:addr] bordered:YES withRoundedRadius:YES];
	cell.controllerView = self;
	if(![_admins containsObject:cell.uri]) {
		cell.adminLabel.enabled	= FALSE;
		cell.adminImage.image = [UIImage imageNamed:@"check_unselected.png"];
	}
	cell.adminButton.hidden = _create || (!_imAdmin && !cell.adminLabel.enabled) || ![_oldContacts containsObject:cell.uri];
	cell.adminButton.userInteractionEnabled = _imAdmin;
	cell.removeButton.hidden = !_create && !_imAdmin;
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

- (void)myAdminStatusChanged:(BOOL)admin {
	NSString *message = admin
		? NSLocalizedString(@"You are now an admin of the chat room", nil)
		: NSLocalizedString(@"You are no longer an admin of the chat room", nil);

	static UIAlertController *alertView = nil;
	// avoid having multiple popups
	[PhoneMainView.instance dismissViewControllerAnimated:YES completion:nil];
	alertView = [UIAlertController alertControllerWithTitle:[NSString stringWithFormat:@"%@", message]
																	   message:nil
											 preferredStyle:UIAlertControllerStyleAlert];

	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {}];
	[alertView addAction:defaultAction];
	[PhoneMainView.instance presentViewController:alertView animated:YES completion:nil];
}

void chat_room_subject_changed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationInfoView *view = (__bridge ChatConversationInfoView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	view.nameLabel.text = [NSString stringWithUTF8String:linphone_event_log_get_subject(event_log)];
}

void chat_room_participant_added(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationInfoView *view = (__bridge ChatConversationInfoView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	NSString *participantAddress = [NSString stringWithUTF8String:linphone_address_as_string(linphone_event_log_get_participant_address(event_log))];
	[view.oldContacts addObject:participantAddress];
	[view.contacts addObject:participantAddress];
	[view.tableView reloadData];
}

void chat_room_participant_removed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationInfoView *view = (__bridge ChatConversationInfoView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	NSString *participantAddress = [NSString stringWithUTF8String:linphone_address_as_string(linphone_event_log_get_participant_address(event_log))];
	[view.oldContacts removeObject:participantAddress];
	[view.contacts removeObject:participantAddress];
	[view.tableView reloadData];
}

void chat_room_participant_admin_status_changed(LinphoneChatRoom *cr, const LinphoneEventLog *event_log) {
	ChatConversationInfoView *view = (__bridge ChatConversationInfoView *)linphone_chat_room_cbs_get_user_data(linphone_chat_room_get_current_callbacks(cr));
	NSString *participantAddress = [NSString stringWithUTF8String:linphone_address_as_string(linphone_event_log_get_participant_address(event_log))];

	LinphoneParticipant *me = linphone_chat_room_get_me(cr);
	if (me && linphone_address_equal(linphone_participant_get_address(me), linphone_event_log_get_participant_address(event_log))) {
		[view myAdminStatusChanged:(linphone_event_log_get_type(event_log) == LinphoneEventLogTypeConferenceParticipantSetAdmin)];
		[view viewWillAppear:TRUE];
		return;
	}

	if (linphone_event_log_get_type(event_log) == LinphoneEventLogTypeConferenceParticipantSetAdmin) {
		[view.admins addObject:participantAddress];
		[view.oldAdmins addObject:participantAddress];
	} else { // linphone_event_log_get_type(event_log) == LinphoneEventLogTypeConferenceParticipantUnsetAdmin
		[view.admins removeObject:participantAddress];
		[view.oldAdmins removeObject:participantAddress];
	}
	[view.tableView reloadData];
}

@end
