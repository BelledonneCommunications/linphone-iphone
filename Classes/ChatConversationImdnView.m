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

#import <Foundation/Foundation.h>

#import "ChatConversationImdnView.h"
#import "PhoneMainView.h"
#import "UIChatBubbleTextCell.h"
#import "UIChatBubblePhotoCell.h"
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
}

- (void)viewWillAppear:(BOOL)animated {
	[super viewWillAppear:animated];
	
	_cell = [VIEW(ChatConversationView).tableController buildMessageCell:_event];
	_cell.frame = CGRectMake(-10,0,_msgView.frame.size.width,_msgView.frame.size.height);
	_cell.isFirst = true;
	_cell.isLast = true;
	[_cell update];
	_cell.popupMenuAllowed = false;
	for (UIView *v in [_msgView subviews]) {
		[v removeFromSuperview];
	}
	[_msgView addSubview:_cell];
	
	
	_tableView.delegate = self;
	_tableView.dataSource = self;

    [self updateImdnList];
	[self fitContent];
	[self startEphemeralDisplayTimer];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(ephemeralDeleted:)
											   name:kLinphoneEphemeralMessageDeletedInRoom
											 object:nil];

}

-(void) viewWillDisappear:(BOOL)animated {
	[self stopEphemeralDisplayTimer];
	[NSNotificationCenter.defaultCenter removeObserver:self];
	[super viewWillDisappear:animated];
}

- (void)updateImdnList {
    if (_event) {
		LinphoneChatMessage *_msg = linphone_event_log_get_chat_message(_event);
		if (_msg) {
			_displayedList = linphone_chat_message_get_participants_by_imdn_state(_msg, LinphoneChatMessageStateDisplayed);
			_receivedList = linphone_chat_message_get_participants_by_imdn_state(_msg, LinphoneChatMessageStateDeliveredToUser);
			_notReceivedList = linphone_chat_message_get_participants_by_imdn_state(_msg, LinphoneChatMessageStateDelivered);
			_errorList = linphone_chat_message_get_participants_by_imdn_state(_msg, LinphoneChatMessageStateNotDelivered);
			
			[_tableView reloadData];
		}
    }
}

- (void)fitContent {
	LinphoneChatMessage *_msg = linphone_event_log_get_chat_message(_event);
	CGSize messageSize = [UIChatBubbleTextCell ViewHeightForMessage:_msg withWidth:self.view.frame.size.width];
	[_msgView setFrame:CGRectMake(_msgView.frame.origin.x,
								  _msgView.frame.origin.y,
								  self.view.frame.size.width,
								  messageSize.height+5)];
	
	[_tableView setFrame:CGRectMake(_tableView.frame.origin.x,
									_msgView.frame.origin.y + _msgView.frame.size.height + 10,
									_tableView.frame.size.width,
									self.view.frame.size.height - (_msgView.frame.origin.y + _msgView.frame.size.height))];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation {
	[self fitContent];
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

	if (@available(iOS 13, *)) {
		[view setBackgroundColor:[UIColor secondarySystemBackgroundColor]];
	} else {
		[view setBackgroundColor:[UIColor colorWithPatternImage:[UIImage imageNamed:@"color_G.png"]]];
	}
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

#pragma mark ephemeral messages

-(void) startEphemeralDisplayTimer {
	_ephemeralDisplayTimer = [NSTimer scheduledTimerWithTimeInterval:1
															  target:self
															selector:@selector(updateEphemeralTimes)
															userInfo:nil
															 repeats:YES];
}

-(void) updateEphemeralTimes {
	NSDateComponentsFormatter *f= [[NSDateComponentsFormatter alloc] init];
	f.unitsStyle = NSDateComponentsFormatterUnitsStylePositional;
	f.zeroFormattingBehavior = NSDateComponentsFormatterZeroFormattingBehaviorPad;
		
	LinphoneChatMessage *_msg = _event ? linphone_event_log_get_chat_message(_event) : nil;
	if (_msg && linphone_chat_message_is_ephemeral(_msg)) {
		long duration = linphone_chat_message_get_ephemeral_expire_time(_msg) == 0 ?
			linphone_chat_room_get_ephemeral_lifetime(linphone_chat_message_get_chat_room(_msg)) :
			linphone_chat_message_get_ephemeral_expire_time(_msg)-[NSDate date].timeIntervalSince1970;
		f.allowedUnits = (duration > 86400 ? kCFCalendarUnitDay : 0)|(duration > 3600 ? kCFCalendarUnitHour : 0)|kCFCalendarUnitMinute|kCFCalendarUnitSecond;
		_cell.ephemeralTime.text =  [f stringFromTimeInterval:duration];
		_cell.ephemeralTime.hidden = NO;
		_cell.ephemeralIcon.hidden = NO;
	}
}

-(void) stopEphemeralDisplayTimer {
	[_ephemeralDisplayTimer invalidate];
}

- (void)ephemeralDeleted:(NSNotification *)notif {
	[PhoneMainView.instance popToView:ChatConversationView.compositeViewDescription];
}

@end
