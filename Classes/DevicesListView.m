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

#import "linphoneapp-Swift.h"
#import "DevicesListView.h"
#import "PhoneMainView.h"
#import "UIDeviceCell.h"

@implementation DevicesMenuEntry
- (id)init:(LinphoneParticipantDevice *)dev isMe:(BOOL)isMe isFirst:(BOOL)first isUnique:(BOOL)unique index:(NSInteger)idx{
	if ((self = [super init])) {
		device = dev;
		isMyself = isMe;
		isFirst = first;
		isUnique = unique;
		index = idx;
		isListOpen = FALSE;
	}
	return self;
}

@end

@implementation DevicesListView
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

#pragma mark - ViewController Functions
- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    _tableView.dataSource = self;
    _tableView.delegate    = self;
    bctbx_list_t *participants = linphone_chat_room_get_participants(_room);
    _devicesMenuEntries = [NSMutableArray array];
	if (linphone_chat_room_get_capabilities(_room) & LinphoneChatRoomCapabilitiesOneToOne) {
		LinphoneParticipant *firstParticipant = participants ? (LinphoneParticipant *)participants->data : NULL;
		const LinphoneAddress *addr = firstParticipant ? linphone_participant_get_address(firstParticipant) : linphone_chat_room_get_peer_address(_room);
		[ContactDisplay setDisplayNameLabel:_addressLabel forAddress:addr];
	} else {
		_addressLabel.text = [NSString stringWithUTF8String:linphone_chat_room_get_subject(_room) ?: LINPHONE_DUMMY_SUBJECT];
		_addressLabel.text = [NSString stringWithFormat:NSLocalizedString(@"%@'s devices", nil), _addressLabel.text];
	}

	LinphoneParticipant *participant;
	for (int i=0; i<bctbx_list_size(participants); i++) {
		participant = (LinphoneParticipant *)bctbx_list_nth_data(participants,i);
		bctbx_list_t *devices = linphone_participant_get_devices(participant);
		DevicesMenuEntry *entry = [[DevicesMenuEntry alloc] init:(LinphoneParticipantDevice *)bctbx_list_nth_data(devices, 0) isMe:FALSE isFirst:TRUE isUnique:(bctbx_list_size(devices)<2) index:i];
		UILabel *lab = [[UILabel alloc] init];
		[ContactDisplay setDisplayNameLabel:lab forAddress:linphone_participant_get_address(participant)];
		entry->displayName = lab.text;
		entry->participant = participant;
		[_devicesMenuEntries addObject:entry];
		bctbx_list_free(devices);
	}

	LinphoneParticipant *me = linphone_chat_room_get_me(_room);
	bctbx_list_t *devices = linphone_participant_get_devices(me);
	DevicesMenuEntry *entry = [[DevicesMenuEntry alloc] init:(LinphoneParticipantDevice *)bctbx_list_nth_data(devices, 0) isMe:TRUE isFirst:TRUE isUnique:(bctbx_list_size(devices)<2) index:bctbx_list_size(participants)];
	entry->participant = me;
	[_devicesMenuEntries addObject:entry];
	bctbx_list_free(devices);
	
	bctbx_list_free(participants);

    _tableView.separatorStyle = UITableViewCellSeparatorStyleNone;
    [_tableView reloadData];
}

#pragma mark - Action Functions
- (IBAction)onBackClick:(id)sender {
    ChatConversationView *view = VIEW(ChatConversationView);
    [PhoneMainView.instance popToView:view.compositeViewDescription];
}

#pragma mark - TableView

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [_devicesMenuEntries count];
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(nonnull NSIndexPath *)indexPath
{
	return 56.0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
	NSString *kCellId = NSStringFromClass(UIDeviceCell.class);
	UIDeviceCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
	if (cell == nil) {
		cell = [[UIDeviceCell alloc] initWithIdentifier:kCellId];
	}
	DevicesMenuEntry *entry = [_devicesMenuEntries objectAtIndex:indexPath.row];
	if (entry->isFirst) {
		entry->isMyself ? cell.deviceLabel.text = NSLocalizedString(@"Me", nil) : [ContactDisplay setDisplayNameLabel:cell.deviceLabel forAddress:linphone_participant_get_address(entry->participant)];
	}
	cell.device = entry->device;
	cell.isFirst = entry->isFirst;
	cell.isUnique = entry->isUnique;
	cell.isListOpen = entry->isListOpen;
	[cell update];

    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	DevicesMenuEntry *entry = [_devicesMenuEntries objectAtIndex:indexPath.row];
	if (entry->isUnique || !entry->isFirst) {
		if (!entry->device) {
			LOGE(@"Can not call, because the device is null.");
			[_tableView reloadData];
		} else {
			const LinphoneAddress *addr = linphone_participant_device_get_address(entry->device);
			[CallManager.instance startCallWithAddr:(LinphoneAddress *)addr isSas:TRUE isVideo:false isConference:false];
		}
	} else {
		bctbx_list_t *devices = linphone_participant_get_devices(entry->participant);
		if (entry->isListOpen) {
			entry->isListOpen = FALSE;
			[_devicesMenuEntries replaceObjectAtIndex:indexPath.row withObject:entry];
			for (int i=0; i< bctbx_list_size(devices); i++) {
				[_devicesMenuEntries removeObjectAtIndex:indexPath.row+1];
			}
		} else {
			entry->isListOpen = TRUE;
			[_devicesMenuEntries replaceObjectAtIndex:indexPath.row withObject:entry];
			LinphoneParticipantDevice *device;
			for (int i=0; i<bctbx_list_size(devices); i++) {
				device = (LinphoneParticipantDevice *)bctbx_list_nth_data(devices,i);
				DevicesMenuEntry *tempEntry = [[DevicesMenuEntry alloc] init:device isMe:entry->isMyself isFirst:FALSE isUnique:FALSE index:entry->index];
				[_devicesMenuEntries insertObject:tempEntry atIndex:indexPath.row+i+1];
			}
		}
		bctbx_list_free(devices);
		[_tableView reloadData];
	}
}

@end
