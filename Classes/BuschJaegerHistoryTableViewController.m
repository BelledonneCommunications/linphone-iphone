/* BuschJaegerHistoryTableViewController.m
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

#import "BuschJaegerHistoryTableViewController.h"
#import "BuschJaegerUtils.h"
#import "BuschJaegerMainView.h"
#import "UIHistoryCell.h"
#import "UACellBackgroundView.h"

@implementation BuschJaegerHistoryTableViewController

@synthesize history;
@synthesize waitView;


#pragma mark - Lifecycle Functions

- (void)dealloc {
    [history release];
    [waitView release];
    [super dealloc];
}


#pragma mark - Property Functions

- (void)setHistory:(NSArray *)ahistory {
    if ([ahistory isEqualToArray:history]) {
        return;
    }
    
    [history release];
    history = [[NSMutableArray alloc] initWithArray:ahistory];
    [self.tableView reloadData];
}



#pragma mark - UITableViewDataSource Functions

- (NSInteger)numberOfSectionsInTableView:(UITableView *)tableView {
	return 1;
}

- (NSInteger)tableView:(UITableView *)tableView numberOfRowsInSection:(NSInteger)section {
	return [history count];
}

- (UITableViewCell *)tableView:(UITableView *)tableView cellForRowAtIndexPath:(NSIndexPath *)indexPath {
    static NSString *kCellId = @"UIHistoryCell";
    UIHistoryCell *cell = [tableView dequeueReusableCellWithIdentifier:kCellId];
    if (cell == nil) {
        cell = [[[UIHistoryCell alloc] initWithIdentifier:kCellId] autorelease];
        
        // Background View
        UACellBackgroundView *selectedBackgroundView = [[[UACellBackgroundView alloc] initWithFrame:CGRectZero] autorelease];
        cell.selectedBackgroundView = selectedBackgroundView;
        [selectedBackgroundView setBackgroundColor:BUSCHJAEGER_NORMAL_COLOR];
        [selectedBackgroundView setBorderColor:[UIColor clearColor]];
    }
	
    [cell setHistory:[history objectAtIndex:[indexPath row]]];
    
    return cell;
}


#pragma mark - UITableViewDelegate Functions

- (void)tableView:(UITableView *)tableView didSelectRowAtIndexPath:(NSIndexPath *)indexPath {
    [tableView deselectRowAtIndexPath:indexPath animated:NO];
    [[BuschJaegerMainView instance].historyDetailsView setHistory:[history objectAtIndex:[indexPath row]]];
    [[BuschJaegerMainView instance].navigationController pushViewController:[BuschJaegerMainView instance].historyDetailsView animated:FALSE];
}

- (UITableViewCellEditingStyle)tableView:(UITableView *)tableView editingStyleForRowAtIndexPath:(NSIndexPath *)indexPath {
    // Detemine if it's in editing mode
    if (self.editing) {
        return UITableViewCellEditingStyleDelete;
    }
    return UITableViewCellEditingStyleNone;
}

- (void)tableView:(UITableView *)tableView commitEditingStyle:(UITableViewCellEditingStyle)editingStyle forRowAtIndexPath:(NSIndexPath *)indexPath  {
    if(editingStyle == UITableViewCellEditingStyleDelete) {
        History *ahistory = [history objectAtIndex:[indexPath row]];
        [[LinphoneManager instance].configuration removeHistory:ahistory delegate:self];
        [tableView beginUpdates];
        [history removeObjectAtIndex:[indexPath row]];
        [tableView deleteRowsAtIndexPaths:[NSArray arrayWithObject:indexPath] withRowAnimation:UITableViewRowAnimationFade];
        [tableView endUpdates];
    }
}


#pragma mark - BuschJaegerConfigurationDelegate Functions

- (void)buschJaegerConfigurationSuccess {
    
}

- (void)buschJaegerConfigurationError:(NSString *)error {
    UIAlertView* errorView = [[UIAlertView alloc] initWithTitle:NSLocalizedString(@"History delete error",nil)
                                                        message:[NSString stringWithFormat:NSLocalizedString(@"Connection issue: %@", nil), error]
                                                       delegate:nil
                                              cancelButtonTitle:NSLocalizedString(@"Continue",nil)
                                              otherButtonTitles:nil,nil];
    [errorView show];
    [errorView release];
    if(![LinphoneManager runningOnIpad]) {
        [[BuschJaegerMainView instance].historyView reload];
    } else {
        [[BuschJaegerMainView instance].welcomeView reloadHistory];
    }
}

@end
