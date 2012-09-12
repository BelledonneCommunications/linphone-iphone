/* BuschJaegerStationViewController.m
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
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

#import "BuschJaegerStationTableViewController.h"
#import "BuschJaegerUtils.h"
#import "UACellBackgroundView.h"
#import "UIStationCell.h"
#import "LinphoneManager.h"

@implementation BuschJaegerStationTableViewController

@synthesize stations;


#pragma mark - Lifecycle Functions

- (void)dealloc {
    [stations release];
    [super dealloc];
}


#pragma mark - Property Functions

- (void)setStations:(NSArray *)astations {
    if ([astations isEqualToArray:stations]) {
        return;
    }

    [stations release];
    stations = [astations retain];
    [self.tableView reloadData];
}


#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [stations count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"UIStationCell";
    UIStationCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {
        cell = [[[UIStationCell alloc] initWithIdentifier:kCellId] autorelease];
        
        // Background View
        UACellBackgroundView *selectedBackgroundView = [[[UACellBackgroundView alloc] initWithFrame:CGRectZero] autorelease];
        cell.selectedBackgroundView = selectedBackgroundView;
        [selectedBackgroundView setBackgroundColor:BUSCHJAEGER_NORMAL_COLOR];
        [selectedBackgroundView setBorderColor:[UIColor clearColor]];
    }
	
    [cell setStation:[stations objectAtIndex:[indexPath row]]];
    
    return cell;
}


#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
   [tableView deselectRowAtIndexPath:indexPath animated:NO];
    OutdoorStation *os = [stations objectAtIndex:[indexPath row]];
    User *usr = [[LinphoneManager instance].configuration getCurrentUser];
    if(!os.surveillance || usr == nil || !usr.surveillance) {
        UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"Surveillance issue",nil)
                                                            message:NSLocalizedString(@"You can't call this camera", nil)
                                                           delegate:nil
                                                  cancelButtonTitle:NSLocalizedString(@"Continue",nil)
                                                  otherButtonTitles:nil,nil];
        [errorView show];
        [errorView release];
        return;
    }
    NSString *addr = [FastAddressBook normalizeSipURI:[os address]];
    [[LinphoneManager instance] call:addr displayName:[os name] transfer:FALSE];
}


@end
