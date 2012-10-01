/* InCallTableViewController.h
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import "InCallTableViewController.h"
#import "UICallCell.h"
#import "UIConferenceHeader.h"
#import "LinphoneManager.h"


@implementation InCallTableViewController

static NSString *const kLinphoneInCallCellData = @"LinphoneInCallCellData";

enum TableSection {
    ConferenceSection = 0,
    CallSection = 1
};

#pragma mark - Lifecycle Functions

- (void)initInCallTableViewController {
}

- (id)init{
    self = [super init];
    if (self) {
		[self initInCallTableViewController];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
		[self initInCallTableViewController];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self initInCallTableViewController];
	}
    return self;
}	

- (void)dealloc {
    [super dealloc];
}

#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    updateTime = [NSTimer scheduledTimerWithTimeInterval:1
                                                  target:self 
                                                selector:@selector(update) 
                                                userInfo:nil 
                                                 repeats:YES];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    if (updateTime != nil) {
        [updateTime invalidate];
        updateTime = nil;
    }
}


#pragma mark - Static Functions

+ (bool)isInConference:(LinphoneCall*) call {
    if (!call)
        return false;
    return linphone_call_is_in_conference(call);
}

+ (int)callCount:(LinphoneCore*) lc {
    int count = 0;
    const MSList* calls = linphone_core_get_calls(lc);
    
    while (calls != 0) {
        if (![InCallTableViewController isInConference:((LinphoneCall*)calls->data)]) {
            count++;
        }
        calls = calls->next;
    }
    return count;
}

+ (LinphoneCall*)retrieveCallAtIndex: (NSInteger) index inConference:(bool) conf{
    const MSList* calls = linphone_core_get_calls([LinphoneManager getLc]);
    
    while (calls != 0) {
        if ([InCallTableViewController isInConference:(LinphoneCall*)calls->data] == conf) {
            if (index == 0)
                break;
            index--;
        }
        calls = calls->next;
    }
    
    if (calls == 0) {
        [LinphoneLogger logc:LinphoneLoggerError format:"Cannot find call with index %d (in conf: %d)", index, conf];
        return nil;
    } else {
        return (LinphoneCall*)calls->data;
    }
}


#pragma mark - 

- (void)removeCallData:(LinphoneCall*) call {
    // Remove data associated with the call
    if(call != NULL) {
        LinphoneCallAppData* appData = (LinphoneCallAppData*) linphone_call_get_user_pointer(call);
        if(appData != NULL) {
            [appData->userInfos removeObjectForKey:kLinphoneInCallCellData];
        }
    }
}

- (UICallCellData*)addCallData:(LinphoneCall*) call {
    // Handle data associated with the call
    UICallCellData * data = nil;
    if(call != NULL) {
        LinphoneCallAppData* appData = (LinphoneCallAppData*) linphone_call_get_user_pointer(call);
        if(appData != NULL && [appData->userInfos objectForKey:kLinphoneInCallCellData] == nil) {
            [appData->userInfos setObject:[[UICallCellData alloc] init:call] forKey:kLinphoneInCallCellData];
        }
    }
    return data;
}

- (UICallCellData*)getCallData:(LinphoneCall*) call {
    // Handle data associated with the call
    UICallCellData * data = nil;
    if(call != NULL) {
        LinphoneCallAppData* appData = (LinphoneCallAppData*) linphone_call_get_user_pointer(call);
        if(appData != NULL) {
            data = [appData->userInfos objectForKey:kLinphoneInCallCellData];
        }
    }
    return data;
}

- (void)update {
    UITableView *tableView = [self tableView];
    for (int section = 0; section < [tableView numberOfSections]; section++) {
        for (int row = 0; row < [tableView numberOfRowsInSection:section]; row++) {
            NSIndexPath* cellPath = [NSIndexPath indexPathForRow:row inSection:section];
            UICallCell* cell = (UICallCell*) [tableView cellForRowAtIndexPath:cellPath];
            [cell update];        
        }
    }
}

- (void)minimizeAll {
    const MSList *list = linphone_core_get_calls([LinphoneManager getLc]);
    while(list != NULL) {
        UICallCellData *data = [self getCallData:(LinphoneCall*)list->data];
        data->minimize = true;
        list = list->next;
    }
    [[self tableView] reloadData];
}

- (void)maximizeAll {
    const MSList *list = linphone_core_get_calls([LinphoneManager getLc]);
    while(list != NULL) {
        UICallCellData *data = [self getCallData:(LinphoneCall*)list->data];
        data->minimize = false;
        list = list->next;
    }
    [[self tableView] reloadData];
}


#pragma mark - UITableViewDataSource Functions

- (UITableViewCell*)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"UICallCell";
    UICallCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {
        cell = [[[UICallCell alloc] initWithIdentifier:kCellId] autorelease];
    }
    
    bool inConference = indexPath.section == ConferenceSection;
    
    LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* currentCall = linphone_core_get_current_call(lc);
    LinphoneCall* call = [InCallTableViewController retrieveCallAtIndex:indexPath.row inConference:inConference];
    [cell setData:[self addCallData:call]];
    
    // Update cell
    if ([indexPath section] == CallSection && [indexPath row] == 0 && linphone_core_get_conference_size(lc) == 0) {
        [cell setFirstCell:true];
    } else {
        [cell setFirstCell:false];
    }
    [cell setCurrentCall:(currentCall == call)];
    [cell setConferenceCell:inConference];
    [cell update];
    
    /*if (linphone_core_get_calls_nb(lc) > 1 || linphone_core_get_conference_size(lc) > 0) {
        tableView.scrollEnabled = true;
    } else {
        tableView.scrollEnabled = false;
    }*/
    return cell;
} 

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
    int count = 0;
    
    LinphoneCore* lc = [LinphoneManager getLc];
    
    if(section == CallSection) {
        count = [InCallTableViewController callCount:lc];
    } else {
        count = linphone_core_get_conference_size(lc);
        if(linphone_core_is_in_conference(lc)) {
            count--;
        }
    }
    return count;
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 2;
}

- (NSString*)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section {
    return @"";
}

- (NSString*)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section {
    return @"";
}


#pragma mark - UITableViewDelegate Functions

- (UIView *)tableView:(UITableView *)tableView viewForHeaderInSection:(NSInteger)section {    
    if(section == CallSection) {
        return [[[UIView alloc] initWithFrame:CGRectZero] autorelease];
    } else if(section == ConferenceSection) {
        LinphoneCore* lc = [LinphoneManager getLc];
        if(linphone_core_get_conference_size(lc) > 0){
            UIConferenceHeader *headerController = [[UIConferenceHeader alloc] init];
            [headerController update];
            UIView *headerView = [headerController view];
            [headerController release];
            return headerView;
        } else {
            return [[[UIView alloc] initWithFrame:CGRectZero] autorelease];
        }
    }
    return [[[UIView alloc] initWithFrame:CGRectZero] autorelease];
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {    
    return [[[UIView alloc] initWithFrame:CGRectZero] autorelease];
}

- (CGFloat)tableView:(UITableView *)tableView heightForHeaderInSection:(NSInteger)section { 
    LinphoneCore* lc = [LinphoneManager getLc];
    if(section == CallSection) {
        return 0.000001f; // Hack UITableView = 0
    } else if(section == ConferenceSection) {
        if(linphone_core_get_conference_size(lc) > 0) {
            return [UIConferenceHeader getHeight];
        }
    }
    return 0.000001f; // Hack UITableView = 0
}

- (CGFloat)tableView:(UITableView *)tableView heightForFooterInSection:(NSInteger)section { 
    LinphoneCore* lc = [LinphoneManager getLc];
    if(section == CallSection) {
        return 0.000001f; // Hack UITableView = 0
    } else if(section == ConferenceSection) {
        if(linphone_core_get_conference_size(lc) > 0) {
            return 20;
        }
    }
    return 0.000001f; // Hack UITableView = 0
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath {
    bool inConference = indexPath.section == ConferenceSection;
    LinphoneCall* call = [InCallTableViewController retrieveCallAtIndex:indexPath.row inConference:inConference];
    UICallCellData* data = [self getCallData:call];
    if(data != nil &&data->minimize)
        return [UICallCell getMinimizedHeight];
    return [UICallCell getMaximizedHeight];
}

@end
