//
//  DevicesListView.m
//  linphone
//
//  Created by Danmei Chen on 06/11/2018.
//

#import "DevicesListView.h"
#import "PhoneMainView.h"
#import "UIDevicesDetails.h"
#import "UIDeviceCell.h"

@implementation DevicesMenuEntry

- (id)initWithTitle:(LinphoneParticipant *)par number:(NSInteger)num isMe:(BOOL)isMe{
    if ((self = [super init])) {
        participant = par;
        numberOfDevices = num;
		myself = isMe;
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
        _devices = linphone_participant_get_devices(firstParticipant);
		_addressLabel.text = [NSString stringWithFormat:NSLocalizedString(@"devices", nil)];
		[_devicesMenuEntries
		 addObject:[[DevicesMenuEntry alloc] initWithTitle:firstParticipant number:0 isMe:FALSE]];
    } else {
        LinphoneParticipant *participant;
        for (int i=0; i<bctbx_list_size(participants); i++) {
            participant = (LinphoneParticipant *)bctbx_list_nth_data(participants,i);
            [_devicesMenuEntries
             addObject:[[DevicesMenuEntry alloc] initWithTitle:participant number:0 isMe:FALSE]];
        }
       
        _addressLabel.text = [NSString stringWithUTF8String:linphone_chat_room_get_subject(_room) ?: LINPHONE_DUMMY_SUBJECT];
		_addressLabel.text = [NSString stringWithFormat:NSLocalizedString(@"%@'s devices", nil), _addressLabel.text];
    }
	
	LinphoneParticipant *me = linphone_chat_room_get_me(_room);
	// not show me if there is only one device
	if (bctbx_list_size(linphone_participant_get_devices(me)) > 1)
		[_devicesMenuEntries addObject:[[DevicesMenuEntry alloc] initWithTitle:me number:0 isMe:TRUE]];

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
    DevicesMenuEntry *entry = [_devicesMenuEntries objectAtIndex:indexPath.row];
	if (entry->myself)
		return (entry->numberOfDevices + 1) * 56.0;
	return entry->numberOfDevices > 1 ? (entry->numberOfDevices + 1) * 56.0 : 56.0;
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath
{
    NSString *kCellId = NSStringFromClass(UIDevicesDetails.class);
    UIDevicesDetails *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
        
    if (cell == nil) {
        cell = [[UIDevicesDetails alloc] initWithIdentifier:kCellId];
    }
    
    DevicesMenuEntry *entry = [_devicesMenuEntries objectAtIndex:indexPath.row];
        
	entry->myself ? cell.addressLabel.text = NSLocalizedString(@"Me", nil) : [ContactDisplay setDisplayNameLabel:cell.addressLabel forAddress:linphone_participant_get_address(entry->participant)];
    cell.participant = entry->participant;
    [cell update:(entry->numberOfDevices != 0) isMyself:entry->myself];

    return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	DevicesMenuEntry *entry = [_devicesMenuEntries objectAtIndex:indexPath.row];
	NSInteger num = 0;
	if (entry->numberOfDevices == 0) {
		num =  bctbx_list_size(linphone_participant_get_devices(entry->participant));
		// not show current device
		if (entry->myself)
		num -= 1;
	}
	[_devicesMenuEntries replaceObjectAtIndex:indexPath.row withObject:[[DevicesMenuEntry alloc] initWithTitle:entry->participant number:num isMe:entry->myself]];
	[_tableView reloadData];
}

@end
