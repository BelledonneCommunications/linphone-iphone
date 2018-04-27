//
//  ChatConversationImdnView.m
//  linphone
//
//  Created by REIS Benjamin on 25/04/2018.
//

#import <Foundation/Foundation.h>

#import "ChatConversationImdnTableViewHeader.h"
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

- (void)viewWillDisappear:(BOOL)animated {
	for (UIView *view in _scrollView.subviews) {
		if (view == _readHeader || view == _deliveredHeader || view == _undeliveredHeader)
			continue;

		[view removeFromSuperview];
	}
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	const LinphoneAddress *addr = linphone_chat_message_get_from_address(_msg);
	BOOL outgoing = linphone_chat_message_is_outgoing(_msg);

	_msgDateLabel.text = [NSString stringWithFormat:@"%@ - %@",
						  [LinphoneUtils timeToString:linphone_chat_message_get_time(_msg) withFormat:LinphoneDateChatBubble],
						  [FastAddressBook displayNameForAddress:addr]];
	_msgAvatarImage.image = outgoing ? [LinphoneUtils selfAvatar] : [FastAddressBook imageForAddress:addr];
	_msgText.text = [NSString stringWithUTF8String:linphone_chat_message_get_text(_msg)];
	_msgBackgroundColorImage.image = _msgBottomBar.image = [UIImage imageNamed:(outgoing ? @"color_A.png" : @"color_D.png")];
	_msgDateLabel.textColor = [UIColor colorWithPatternImage:_msgBackgroundColorImage.image];

	[_msgView setFrame:CGRectMake(_msgView.frame.origin.x,
								  _msgView.frame.origin.y,
								  _msgView.frame.size.width,
								  [UIChatBubbleTextCell ViewHeightForMessage:_msg withWidth:self.view.frame.size.width].height)];

	[_whiteView setFrame:CGRectMake(self.view.frame.origin.x,
									self.view.frame.origin.y,
									self.view.frame.size.width,
									_msgView.frame.size.height + _msgView.frame.origin.y)];
	
	_scrollView.scrollEnabled = TRUE;
	_scrollView.userInteractionEnabled = TRUE;
	_scrollView.showsVerticalScrollIndicator = TRUE;

	_displayedList = linphone_chat_message_get_participants_that_have_displayed(_msg);
	_receivedList = linphone_chat_message_get_participants_that_have_received(_msg);
	_notReceivedList = linphone_chat_message_get_participants_that_have_not_received(_msg);

	if (_displayedList) {
	}
	if (_receivedList) {
	}
	if (_notReceivedList) {
	}
}

#pragma mark - TableView

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	NSInteger i = 0;
	if (_displayedList) i++;
	if (_receivedList) i++;
	if (_notReceivedList) i++;
	return i;
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section {
	return 23.0;
}

- (UITableViewHeaderFooterView *)headerViewForSection:(NSInteger)section {
	NSString *kHeaderId = NSStringFromClass(ChatConversationImdnTableViewHeader.class);
	ChatConversationImdnTableViewHeader *header = [tableView dequeueReusableHeaderFooterViewWithIdentifier:kHeaderId];
	if (!header)
		header = [[ChatConversationImdnTableViewHeader alloc] initWithIdentifier:kHeaderId];

	if (section == 1) {
		if (_displayedList) {
			header.
		} else if (_receivedList) {

		} else {

		}
	} else if (section == 2) {
		if (_receivedList) {

		} else {

		}
	}
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	if (section == 1) {
		if (_displayedList)
			return bctbx_list_size(_displayedList);
		else if (_receivedList)
			return bctbx_list_size(_receivedList);
		else
			return bctbx_list_size(_notReceivedList);
	} else if (section == 2) {
		if (_receivedList)
			return bctbx_list_size(_receivedList);
		else
			return bctbx_list_size(_notReceivedList);
	}
	return bctbx_list_size(_notReceivedList);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
	bctbx_list_t *list = NULL;
	if (indexPath.section == 1) {
		if (_displayedList)
			list = _displayedList;
		else if (_receivedList)
			list = _receivedList;
		else
			list = _notReceivedList;
	} else if (indexPath.section == 2) {
		if (_receivedList)
			list = _receivedList;
		else
			list = _notReceivedList;
	} else
		list = _notReceivedList;

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

	return cell;
}

- (IBAction)onBackClick:(id)sender {
	[PhoneMainView.instance popCurrentView];
}

@end
