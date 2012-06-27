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
#import "LinphoneManager.h"

#include "private.h"

@implementation InCallTableViewController

- (void)myInit {
    self->callCellData = [[NSMutableDictionary alloc] init];
}

- (id)init{
    self = [super init];
    if (self) {
		[self myInit];
    }
    return self;
}

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil {
    self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
    if (self) {
		[self myInit];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)decoder {
    self = [super initWithCoder:decoder];
    if (self) {
		[self myInit];
	}
    return self;
}	

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
    
    if (!conf && linphone_core_get_conference_size([LinphoneManager getLc]))
        index--;
    
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


- (UITableViewCell*)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    UICallCell *cell = [tableView dequeueReusableCellWithIdentifier:@"UICallCell"];
    if (cell == nil) {
        cell = [[UICallCell alloc] init];
    }
    
    LinphoneCore* lc = [LinphoneManager getLc];
    LinphoneCall* call = [InCallTableViewController retrieveCallAtIndex:indexPath.row inConference:NO];
    [cell setData:[self addCallData:call]];
    [cell update];
    
    if ([indexPath row] == 0) {
        [cell firstCell];
    } else {
        [cell otherCell];
    }
    
    if (linphone_core_get_calls_nb(lc) > 1) {
        tableView.scrollEnabled = TRUE;
    } else {
        tableView.scrollEnabled = FALSE;
    }
    return cell;
} 

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section
{
    LinphoneCore* lc = [LinphoneManager getLc];
    
    return [InCallTableViewController callCount:lc] + (int)(linphone_core_get_conference_size(lc) > 0);
}

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
    return 1;
    LinphoneCore* lc = [LinphoneManager getLc];
    int count = 0;
    
    if ([InCallTableViewController callCount:lc] > 0)
        count++;
    
    if (linphone_core_get_conference_size([LinphoneManager getLc]) > 0)
        count ++;
    
    return count;
}

- (NSString*)tableView:(UITableView *)tableView titleForHeaderInSection:(NSInteger)section
{
    return nil;
}

- (NSString*)tableView:(UITableView *)tableView titleForFooterInSection:(NSInteger)section
{
    return nil;
}

- (CGFloat)tableView:(UITableView *)tableView heightForRowAtIndexPath:(NSIndexPath *)indexPath
{
    LinphoneCall* call = [InCallTableViewController retrieveCallAtIndex:indexPath.row inConference:NO];
    UICallCellData* data = [callCellData objectForKey:[NSValue valueWithPointer:call]];
    if(data != nil &&data->minimize)
        return [UICallCell getMinimizedHeight];
    return [UICallCell getMaximizedHeight];
}

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath
{
    [tableView deselectRowAtIndexPath:indexPath animated:NO];
    
    LinphoneCore* lc = [LinphoneManager getLc];
    
    bool inConf = (indexPath.row == 0 && linphone_core_get_conference_size(lc) > 0);
    
    LinphoneCall* selectedCall = [InCallTableViewController retrieveCallAtIndex:indexPath.row inConference:inConf];
    
    if (inConf) {
        if (linphone_core_is_in_conference(lc))
            return;
        LinphoneCall* current = linphone_core_get_current_call(lc);
        if (current)
            linphone_core_pause_call(lc, current);
        linphone_core_enter_conference([LinphoneManager getLc]);
    } else if (selectedCall) {
        if (linphone_core_is_in_conference(lc)) {
            linphone_core_leave_conference(lc);
        }
        if(!linphone_core_sound_resources_locked(lc)) {
            linphone_core_resume_call([LinphoneManager getLc], selectedCall);
        }
    }
}

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

- (void)dealloc {
    [callCellData removeAllObjects];
    [callCellData dealloc];
    [super dealloc];
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


@end
