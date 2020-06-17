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

#import <UIKit/UIKit.h>
#import "UICompositeView.h"

@interface DevicesMenuEntry : NSObject {
@public
    LinphoneParticipant *participant;
    NSInteger numberOfDevices;
	BOOL myself;
};
@end

@interface DevicesListView : UIViewController <UICompositeViewDelegate, UITableViewDelegate, UITableViewDataSource>

@property (weak, nonatomic) IBOutlet UILabel *addressLabel;
@property (weak, nonatomic) IBOutlet UITableView *tableView;

@property(nonatomic) LinphoneChatRoom *room;
@property bctbx_list_t *devices;
@property NSMutableArray *devicesMenuEntries;

- (IBAction)onBackClick:(id)sender;

@end
