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

#import "EphemeralSettingsView.h"
#import "PhoneMainView.h"
#import "UIDeviceCell.h"



@implementation EphemeralSettingsView
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



-(void) viewDidLoad {
	[super viewDidLoad];
	self.tableView.tableHeaderView = ({
		UIView *line = [[UIView alloc]
						initWithFrame:CGRectMake(0, 0,
						self.tableView.frame.size.width, 1 / UIScreen.mainScreen.scale)];
		line.backgroundColor = self.tableView.separatorColor;
		line;
	});
	self.tableView.tintColor =  UIColorFromRGB(0x96c11f);
	self.explanations.text = NSLocalizedString(@"Messages will be deleted on both ends once they have been read and after the selected timeout.", nil);
	self.titleText.text = NSLocalizedString(@"Ephemeral messages", nil);

}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    _tableView.dataSource = self;
    _tableView.delegate = self;
	[self setIndexBasedOnRoomSetting];
    [_tableView reloadData];
}

#pragma mark - Action Functions
- (IBAction)onBackClick:(id)sender {
    ChatConversationView *view = VIEW(ChatConversationView);
    [PhoneMainView.instance popToView:view.compositeViewDescription];
}


- (IBAction)onSaveClick:(id)sender {
	[self setRoomSettingsBasedOnIndex];
	ChatConversationView *view = VIEW(ChatConversationView);
	[PhoneMainView.instance popToView:view.compositeViewDescription];
}

#pragma mark - TableView

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return 6;
}


- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
   UITableViewCell *cell = [[UITableViewCell alloc] init];
	
	switch(indexPath.row) {
		case 0:cell.textLabel.text = NSLocalizedString(@"Disabled",nil);break;
		case 1:cell.textLabel.text = NSLocalizedString(@"1 minute",nil);break;
		case 2:cell.textLabel.text = NSLocalizedString(@"1 hour",nil);break;
		case 3:cell.textLabel.text = NSLocalizedString(@"1 day",nil);break;
		case 4:cell.textLabel.text = NSLocalizedString(@"3 days",nil);break;
		case 5:cell.textLabel.text = NSLocalizedString(@"1 week",nil);break;
	}
	if (indexPath.row == _selectedIndex) {
		cell.accessoryType = UITableViewCellAccessoryCheckmark;
		cell.textLabel.font = [UIFont fontWithDescriptor:[cell.textLabel.font.fontDescriptor fontDescriptorWithSymbolicTraits:UIFontDescriptorTraitBold]
													size:cell.textLabel.font.pointSize];
	}
	return cell;
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	_selectedIndex = indexPath.row;
	[_tableView reloadData];
}

#pragma mark - BL Functions

-(void) setIndexBasedOnRoomSetting {
	if (!linphone_chat_room_ephemeral_enabled(_room)) {
		_selectedIndex = 0;
	}else if (linphone_chat_room_get_ephemeral_lifetime(_room) == 60) {
		_selectedIndex = 1;
	}else if (linphone_chat_room_get_ephemeral_lifetime(_room) == 3600) {
		_selectedIndex = 2;
	}else if (linphone_chat_room_get_ephemeral_lifetime(_room) == 86400) {
		_selectedIndex = 3;
	}else if (linphone_chat_room_get_ephemeral_lifetime(_room) == 3*86400) {
		_selectedIndex = 4;
	}else if (linphone_chat_room_get_ephemeral_lifetime(_room) == 7*86400) {
		_selectedIndex = 5;
	}
}

-(void) setRoomSettingsBasedOnIndex {
	if (_selectedIndex == 0) {
		linphone_chat_room_enable_ephemeral(_room, false);
		return;
	}
	
	linphone_chat_room_enable_ephemeral(_room, true);
	switch (_selectedIndex) {
		case 1: linphone_chat_room_set_ephemeral_lifetime(_room, 60);break;
		case 2: linphone_chat_room_set_ephemeral_lifetime(_room, 3600);break;
		case 3: linphone_chat_room_set_ephemeral_lifetime(_room, 86400);break;
		case 4: linphone_chat_room_set_ephemeral_lifetime(_room, 3*86400);break;
		case 5: linphone_chat_room_set_ephemeral_lifetime(_room, 7*86400);break;
	}
	
}

@end
