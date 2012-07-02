/* ContactsViewController.m
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

#import "ContactsViewController.h"

#import "AddressBook/ABPerson.h"

@implementation ContactsViewController

@synthesize tableController;
@synthesize tableView;

@synthesize allButton;
@synthesize linphoneButton;

typedef enum _HistoryView {
    History_All,
    History_Linphone,
    History_MAX
} HistoryView;

#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"ContactsViewController" bundle:[NSBundle mainBundle]];
}


#pragma mark - ViewController Functions

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
	[self.tableView reloadData];
}


- (void)viewDidLoad {
    [super viewDidLoad];
    [self changeView: History_All];
}


#pragma mark -

- (void)changeView: (HistoryView) view {
    if(view == History_All) {
        allButton.selected = TRUE;
    } else {
        allButton.selected = FALSE;
    }
    
    if(view == History_Linphone) {
        linphoneButton.selected = TRUE;
    } else {
        linphoneButton.selected = FALSE;
    }
}

#pragma mark - Action Functions

- (IBAction)onAllClick: (id) event {
    [self changeView: History_All];
}

- (IBAction)onLinphoneClick: (id) event {
    [self changeView: History_Linphone];
}

@end
