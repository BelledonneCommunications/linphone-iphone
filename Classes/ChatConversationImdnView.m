//
//  ChatConversationImdnView.m
//  linphone
//
//  Created by REIS Benjamin on 25/04/2018.
//

#import <Foundation/Foundation.h>

#import "ChatConversationImdnView.h"
#import "PhoneMainView.h"
#import "UIChatBubbleTextCell.h"
#import "UIChatConversationImdnTableViewCell.h"

@implementation ChatConversationImdnView

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
	_msg = NULL;
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	const LinphoneAddress *addr = linphone_chat_message_get_from_address(_msg);
	BOOL outgoing = linphone_chat_message_is_outgoing(_msg);

	_msgDateLabel.text = [NSString stringWithFormat:@"%@ - %@",
						  [LinphoneUtils timeToString:linphone_chat_message_get_time(_msg) withFormat:LinphoneDateChatBubble],
						  [FastAddressBook displayNameForAddress:addr]];
	_msgAvatarImage.image = outgoing ? [LinphoneUtils selfAvatar] : [FastAddressBook imageForAddress:addr];
    _msgText.text =  messageText;
	_msgBackgroundColorImage.image = _msgBottomBar.image = [UIImage imageNamed:(outgoing ? @"color_A.png" : @"color_D.png")];
	_msgDateLabel.textColor = [UIColor colorWithPatternImage:_msgBackgroundColorImage.image];

	_tableView.delegate = self;
	_tableView.dataSource = self;

    [self updateImdnList];
}

- (void)updateImdnList {
    if (_msg) {
        _displayedList = linphone_chat_message_get_participants_by_imdn_state(_msg, LinphoneChatMessageStateDisplayed);
        _receivedList = linphone_chat_message_get_participants_by_imdn_state(_msg, LinphoneChatMessageStateDeliveredToUser);
        _notReceivedList = linphone_chat_message_get_participants_by_imdn_state(_msg, LinphoneChatMessageStateDelivered);
        _errorList = linphone_chat_message_get_participants_by_imdn_state(_msg, LinphoneChatMessageStateNotDelivered);
    
        [_tableView reloadData];
    }
}

- (void)fitContent {
    [self setMessageText];
    
	BOOL outgoing = linphone_chat_message_is_outgoing(_msg);
	_msgBackgroundColorImage.image = _msgBottomBar.image = [UIImage imageNamed:(outgoing ? @"color_A.png" : @"color_D.png")];
	_msgDateLabel.textColor = [UIColor colorWithPatternImage:_msgBackgroundColorImage.image];
	[_msgView setFrame:CGRectMake(_msgView.frame.origin.x,
								  _msgView.frame.origin.y,
								  _msgView.frame.size.width,
                                  [UIChatBubbleTextCell ViewHeightForMessageText:_msg withWidth:self.view.frame.size.width textForImdn:messageText].height)];
	
	[_tableView setFrame:CGRectMake(_tableView.frame.origin.x,
									_msgView.frame.origin.y + _msgView.frame.size.height + 10,
									_tableView.frame.size.width,
									self.view.frame.size.height - (_msgView.frame.origin.y + _msgView.frame.size.height))];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[self fitContent];
}

- (void)setMessageText {
    const char *utf8Text= linphone_chat_message_get_text_content(_msg);
    LinphoneContent *fileContent = linphone_chat_message_get_file_transfer_information(_msg);
    messageText = nil;
    if (utf8Text) {
        messageText =  [NSString stringWithUTF8String:utf8Text];
        if (fileContent)
            messageText = [NSString stringWithFormat:@"%@\n%@", messageText, [NSString stringWithUTF8String: linphone_content_get_name(fileContent)]];
    } else {
        messageText = [NSString stringWithUTF8String: linphone_content_get_name(fileContent)];
    }
}
#pragma mark - TableView

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	NSInteger numberOfSection = 0;
	if (_displayedList) numberOfSection++;
	if (_receivedList) numberOfSection++;
	if (_notReceivedList) numberOfSection++;
	if (_errorList) numberOfSection++;
	return numberOfSection;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	return 23.0;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(nonnull NSIndexPath *)indexPath {
	return 44.0;
}
- (nullable UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {
	UILabel *label = [[UILabel alloc] initWithFrame:CGRectMake(0, 0, 0, 0)];
	label.numberOfLines = 1;
	UIView *view = [[UIView alloc] initWithFrame:CGRectMake(0, 0, tableView.frame.size.width, 23)];
	UIImage *image = NULL;

	if (section == 0) {
		if (_displayedList) {
			label.text = NSLocalizedString(@"Read", nil);
			label.textColor = [UIColor colorWithRed:(24 / 255.0) green:(167 / 255.0) blue:(175 / 255.0) alpha:1.0];
			image = [UIImage imageNamed:@"chat_read"];
		} else if (_receivedList) {
			label.text = NSLocalizedString(@"Delivered", nil);
			label.textColor = [UIColor grayColor];
			image = [UIImage imageNamed:@"chat_delivered"];
		} else if (_notReceivedList) {
			label.text = NSLocalizedString(@"Sent", nil);
			label.textColor = [UIColor grayColor];
		} else if (_errorList) {
			label.text = NSLocalizedString(@"Error", nil);
			label.textColor = [UIColor redColor];
			image = [UIImage imageNamed:@"chat_error"];
		}
	} else if (section == 1) {
		if (_displayedList && _receivedList) {
			label.text = NSLocalizedString(@"Delivered", nil);
			label.textColor = [UIColor grayColor];
			image = [UIImage imageNamed:@"chat_delivered"];
		} else if (_notReceivedList) {
			label.text = NSLocalizedString(@"Sent", nil);
			label.textColor = [UIColor grayColor];
		} else if (_errorList) {
			label.text = NSLocalizedString(@"Error", nil);
			label.textColor = [UIColor redColor];
			image = [UIImage imageNamed:@"chat_error"];
		}
	} else if (section == 2) {
		if (_displayedList && _receivedList && _notReceivedList) {
			label.text = NSLocalizedString(@"Sent", nil);
			label.textColor = [UIColor grayColor];
		} else if (_errorList) {
			label.text = NSLocalizedString(@"Error", nil);
			label.textColor = [UIColor redColor];
			image = [UIImage imageNamed:@"chat_error"];
		}
	} else if (section == 3) {
		label.text = NSLocalizedString(@"Error", nil);
		label.textColor = [UIColor redColor];
		image = [UIImage imageNamed:@"chat_error"];
	}

	[view addSubview:label];
	[label sizeToFit];
	[label setCenter:view.center];

	if (image) {
		UIImageView *imageView = [[UIImageView alloc] initWithImage:image];
		[view addSubview:imageView];
		[imageView setFrame:CGRectMake(label.frame.origin.x + label.frame.size.width + 5, 2, 19, 19)];
	}
	[view setBackgroundColor:[UIColor colorWithPatternImage:[UIImage imageNamed:@"color_G.png"]]];
	return view;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (section == 0) {
		if (_displayedList)
			return bctbx_list_size(_displayedList);
		else if (_receivedList)
			return bctbx_list_size(_receivedList);
		else if (_notReceivedList)
			return bctbx_list_size(_notReceivedList);
		else if (_errorList)
			return bctbx_list_size(_errorList);
	} else if (section == 1) {
		if (_displayedList &&_receivedList)
			return bctbx_list_size(_receivedList);
		else if (_notReceivedList)
			return bctbx_list_size(_notReceivedList);
		else if (_errorList)
			return bctbx_list_size(_errorList);
	} else if (section == 2) {
		if (_displayedList && _receivedList && _notReceivedList)
			return bctbx_list_size(_notReceivedList);
		else if (_errorList)
			return bctbx_list_size(_errorList);
	} else if (section == 3)
		return bctbx_list_size(_errorList);

	return 0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	bctbx_list_t *list = NULL;
	if (indexPath.section == 0) {
		if (_displayedList)
			list = _displayedList;
		else if (_receivedList)
			list = _receivedList;
		else if (_notReceivedList)
			list = _notReceivedList;
		else if (_errorList)
			list = _errorList;
	} else if (indexPath.section == 1) {
		if (_displayedList &&_receivedList)
			list = _receivedList;
		else if (_notReceivedList)
			list = _notReceivedList;
		else if (_errorList)
			list = _errorList;
	} else if (indexPath.section == 2) {
		if (_displayedList && _receivedList && _notReceivedList)
			list = _notReceivedList;
		else if (_errorList)
			list = _errorList;
	} else if (indexPath.section == 3)
		list = _errorList;

	if (!list)
		return nil;

	NSString *kCellId = NSStringFromClass(UIChatConversationImdnTableViewCell.class);
	UIChatConversationImdnTableViewCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIChatConversationImdnTableViewCell alloc] initWithIdentifier:kCellId];
	}
	LinphoneParticipantImdnState *state = bctbx_list_nth_data(list, (int)indexPath.row);
	const LinphoneParticipant *participant = linphone_participant_imdn_state_get_participant(state);
	time_t time = linphone_participant_imdn_state_get_state_change_time(state);
	const LinphoneAddress *addr = linphone_participant_get_address(participant);
	cell.displayName.text = [FastAddressBook displayNameForAddress:addr];
	cell.avatar.image = [FastAddressBook imageForAddress:addr];
	cell.dateLabel.text = [LinphoneUtils timeToString:time withFormat:LinphoneDateChatBubble];
	cell.userInteractionEnabled = false;

	return cell;
}

- (IBAction)onBackClick:(id)sender {
	[PhoneMainView.instance popCurrentView];
}

@end
