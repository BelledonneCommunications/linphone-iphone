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

#include "private.h"

@implementation InCallTableViewController

enum TableSection {
    ConferenceSection = 0,
    CallSection = 1
};

#pragma mark - Lifecycle Functions

- (void)initInCallTableViewController {
    self->callCellData = [[NSMutableDictionary alloc] init];
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
    [callCellData removeAllObjects];
    [callCellData release];
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
    return linphone_call_get_current_params(call)->in_conference;
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
        ms_error("Cannot find call with index %d (in conf: %d)", index, conf);
        return nil;
    } else {
        return (LinphoneCall*)calls->data;
    }
}


#pragma mark - 

- (void)removeCallData:(LinphoneCall*) call {
    // Remove data associated with the call
    if(call != NULL) {
        NSValue *value = [NSValue valueWithPointer:call];
        UICallCellData * data = [callCellData objectForKey:value];
        if(data == nil) {
            [callCellData removeObjectForKey:value];
        }
    }
}

- (UICallCellData*)addCallData:(LinphoneCall*) call {
    // Handle data associated with the call
    UICallCellData * data = nil;
    if(call != NULL) {
        NSValue *value = [NSValue valueWithPointer:call];
        data = [callCellData objectForKey:value];
        if(data == nil) {
            data = [[UICallCellData alloc] init:call];
            [callCellData setObject:data forKey:value];
        }
    }
    return data;
}

- (UICallCellData*)getCallData:(LinphoneCall*) call {
    // Handle data associated with the call
    UICallCellData * data = nil;
    if(call != NULL) {
        NSValue *value = [NSValue valueWithPointer:call];
        data = [callCellData objectForKey:value];
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
    for(id key in callCellData) {
        UICallCellData *data = [callCellData objectForKey:key];
        data->minimize = true;
    }
    [[self tableView] reloadData];
}

- (void)maximizeAll {
    for(id key in callCellData) {
        UICallCellData *data = [callCellData objectForKey:key];
        data->minimize = false;
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
    
    if (linphone_core_get_calls_nb(lc) > 1 || linphone_core_get_conference_size(lc) > 0) {
        tableView.scrollEnabled = true;
    } else {
        tableView.scrollEnabled = false;
    }
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
        return [[UIView alloc] initWithFrame:CGRectZero];
    } else if(section == ConferenceSection) {
        LinphoneCore* lc = [LinphoneManager getLc];
        if(linphone_core_get_conference_size(lc) > 0){
            UIConferenceHeader *headerController = [[UIConferenceHeader alloc] init];
            [headerController update];
            UIView *headerView = [headerController view];
            [headerController release];
            return headerView;
        } else {
            return [[UIView alloc] initWithFrame:CGRectZero];
        }
    }
    return [[UIView alloc] initWithFrame:CGRectZero];
}

- (UIView *)tableView:(UITableView *)tableView viewForFooterInSection:(NSInteger)section {    
    return [[UIView alloc] initWithFrame:CGRectZero];
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
    UICallCellData* data = [callCellData objectForKey:[NSValue valueWithPointer:call]];
    if(data != nil &&data->minimize)
        return [UICallCell getMinimizedHeight];
    return [UICallCell getMaximizedHeight];
}

@end
