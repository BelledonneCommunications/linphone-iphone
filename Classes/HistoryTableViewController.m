/* HistoryTableViewController.m
 *
 * Copyright (C) 2009  Belledonne Comunications, Grenoble, France
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

#import "HistoryTableViewController.h"
#import "UIHistoryCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation HistoryTableViewController

#pragma mark - Lifecycle Functions

- (id)init {
    if((self = [super init]) != nil) {
        self->editMode = false;
    }
    return self;
}


#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	const MSList * logs = linphone_core_get_call_logs([LinphoneManager getLc]);
	return ms_list_size(logs);
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UIHistoryCell *cell = [tableView dequeueReusableCellWithIdentifier:@"UIHistoryCell"];
    if (cell == nil) {
        cell = [[UIHistoryCell alloc] initWithIdentifier:@"UIHistoryCell"];
    }
    
	const MSList * logs = linphone_core_get_call_logs([LinphoneManager getLc]);
	LinphoneCallLog*  callLogs = ms_list_nth_data(logs,  indexPath.row);
    
    if(editMode) 
        [cell enterEditMode];
    else 
        [cell exitEditMode];
    
    [cell setCallLog:callLogs];
    [cell update];
	
    return cell;
}


#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
	[tableView deselectRowAtIndexPath:indexPath animated:NO];
	
	const MSList * logs = linphone_core_get_call_logs([LinphoneManager getLc]);
	LinphoneCallLog*  callLogs = ms_list_nth_data(logs,  indexPath.row) ;
	LinphoneAddress* partyToCall; 
	if (callLogs->dir == LinphoneCallIncoming) {
		partyToCall=callLogs->from;
	} else {
		partyToCall=callLogs->to;
	}
	const char* username = linphone_address_get_username(partyToCall)!=0?linphone_address_get_username(partyToCall):"";
	const char* displayName = linphone_address_get_display_name(partyToCall)!=0?linphone_address_get_display_name(partyToCall):"";
	const char* domain = linphone_address_get_domain(partyToCall);
	
	LinphoneProxyConfig* proxyCfg;
	linphone_core_get_default_proxy([LinphoneManager getLc],&proxyCfg);
	
	NSString* phoneNumber;
	
	if (proxyCfg && (strcmp(domain, linphone_proxy_config_get_domain(proxyCfg)) == 0)) {
		phoneNumber = [[NSString alloc] initWithCString:username encoding:[NSString defaultCStringEncoding]];
	} else {
		phoneNumber = [[NSString alloc] initWithCString:linphone_address_as_string_uri_only(partyToCall) encoding:[NSString defaultCStringEncoding]];
	}
    
    NSString* dispName = [[NSString alloc] initWithCString:displayName encoding:[NSString defaultCStringEncoding]];
    
    // Go to dialer view
    NSDictionary *dict = [[[NSDictionary alloc] initWithObjectsAndKeys:
                          [[[NSArray alloc] initWithObjects: phoneNumber, nil] autorelease]
                          , @"setAddress:",
                          nil] autorelease];
    [[PhoneMainView instance] changeView:PhoneView_Dialer dict:dict];

	[phoneNumber release];
    [dispName release];
}


#pragma mark - 

- (void) toggleEditMode {
    editMode = !editMode;
    [(UITableView*)[self view] reloadData];
}

- (void) enterEditMode {
    if(!editMode) {
        [self toggleEditMode];
    }
}

- (void) exitEditMode {
    if(editMode) {
        [self toggleEditMode];
    }
}

@end

