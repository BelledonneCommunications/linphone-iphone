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
#import "PhoneMainView.h"
#import "Utils.h"

#import <AddressBook/ABPerson.h>

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

- (void)dealloc {
    [tableController release];
    [tableView release];
    
    [allButton release];
    [linphoneButton release];
    
    [super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

+ (UICompositeViewDescription*) compositeViewDescription {
    UICompositeViewDescription *description = [UICompositeViewDescription alloc];
    description->content = @"ContactsViewController";
    description->tabBar = @"UIMainBar";
    description->tabBarEnabled = true;
    description->stateBar = nil;
    description->stateBarEnabled = false;
    description->fullscreen = false;
    return description;
}


#pragma mark - ViewController Functions

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewWillDisappear:NO];
    }
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewWillAppear:NO];
    }   
    
    [self changeView:History_All];
}

- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewDidAppear:NO];
    }   
}

- (void)viewDidDisappear:(BOOL)animated {
    [super viewDidDisappear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewDidDisappear:NO];
    }  
}

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Set selected+over background: IB lack !
    [linphoneButton setImage:[UIImage imageNamed:@"contacts_linphone_selected.png"] 
                 forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
    // Set selected+over background: IB lack !
    [allButton setImage:[UIImage imageNamed:@"contacts_all_selected.png"] 
                    forState:(UIControlStateHighlighted | UIControlStateSelected)];
}


#pragma mark -

- (void)setAddress:(NSString*)address {
    [tableController setTempAddress:address];
}

- (void)changeView: (HistoryView) view {
    if(view == History_All) {
        [tableController setSipFilter:FALSE];
        allButton.selected = TRUE;
    } else {
        allButton.selected = FALSE;
    }
    
    if(view == History_Linphone) {
        [tableController setSipFilter:TRUE];
        linphoneButton.selected = TRUE;
    } else {
        linphoneButton.selected = FALSE;
    }
}


#pragma mark - Action Functions

- (IBAction)onAllClick:(id)event {
    [self changeView: History_All];
}

- (IBAction)onLinphoneClick:(id)event {
    [self changeView: History_Linphone];
}

- (IBAction)onAddContactClick:(id)event {
    // Go to Contact details view
    ContactDetailsViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeView:PhoneView_ContactDetails push:TRUE], ContactDetailsViewController);
    if(controller != nil) {
        if([tableController tempAddress] == nil) {
            [controller newContact];
        } else {
            [controller newContact:[tableController tempAddress]];
            [tableController setTempAddress:nil];
        }
    }
}

@end
