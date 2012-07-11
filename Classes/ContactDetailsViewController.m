/* ContactDetailsViewController.m
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

#import "ContactDetailsViewController.h"
#import "PhoneMainView.h"

@implementation ContactDetailsViewController

@synthesize tableController;
@synthesize contact;
@synthesize editButton;


#pragma mark - Lifecycle Functions

- (id)init  {
    self = [super initWithNibName:@"ContactDetailsViewController" bundle:[NSBundle mainBundle]];
    return self;
}

- (void)dealloc {
    [tableController release];
    
    [super dealloc];
}


#pragma mark - 

- (void)newContact {
    [tableController newContact];
    [tableController setEditing:TRUE animated:FALSE];
    [[tableController tableView] reloadData];
    [editButton setOn];
}

- (void)newContact:(NSString*)address {
    [tableController newContact:address];
    [tableController setEditing:TRUE animated:FALSE];
    [[tableController tableView] reloadData];
    [editButton setOn];
}


#pragma mark - Property Functions

- (void)setContact:(ABRecordRef)acontact {
    self->contact = acontact;
    [tableController setContactID:ABRecordGetRecordID(acontact)];
}


#pragma mark - ViewController Functions

- (void)viewDidLoad{
    [super viewDidLoad];
    // Set selected+over background: IB lack !
    [editButton setImage:[UIImage imageNamed:@"contact_ok_over.png"] 
                forState:(UIControlStateHighlighted | UIControlStateSelected)];
    
    // Force view load
    [tableController->footerController view];
    [tableController->footerController->removeButton addTarget:self 
                                                        action:@selector(onRemove:) 
                                              forControlEvents:UIControlEventTouchUpInside];
    [[tableController tableView] setBackgroundColor:[UIColor clearColor]]; // Can't do it in Xib: issue with ios4
}

- (void)viewDidUnload {
    [super viewDidUnload];
    [tableController->footerController->removeButton removeTarget:self 
                                                           action:@selector(onRemove:) 
                                                 forControlEvents:UIControlEventTouchUpInside];
}

- (void)viewWillAppear:(BOOL)animated {
    if([tableController isEditing]) {
        [tableController resetData];
        [tableController setEditing:FALSE animated:FALSE];
    }
    [super viewWillAppear:animated];
    [editButton setOff];
}

#pragma mark - UICompositeViewDelegate Functions

+ (UICompositeViewDescription*) compositeViewDescription {
    UICompositeViewDescription *description = [UICompositeViewDescription alloc];
    description->content = @"ContactDetailsViewController";
    description->tabBar = @"UIMainBar";
    description->tabBarEnabled = true;
    description->stateBar = nil;
    description->stateBarEnabled = false;
    description->fullscreen = false;
    return description;
}


#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
    if([tableController isEditing]) {
        [tableController setEditing:FALSE animated:FALSE];
        [tableController resetData];
        [editButton setOff];
        if([tableController contactID] == kABRecordInvalidID) {
            [[PhoneMainView instance] popView];
        }
    } else {
        [[PhoneMainView instance] popView];
    }
}

- (IBAction)onEditClick:(id)event {
    [[tableController tableView] beginUpdates];
    [tableController setEditing:![tableController isEditing] animated:TRUE];
    [[tableController tableView] endUpdates];
    if(![tableController isEditing]) {
        [tableController saveData];
    }
}

- (IBAction)onRemove:(id)event {
    [tableController setEditing:FALSE animated:FALSE];
    [tableController removeContact];
    [editButton setOff];
    [[PhoneMainView instance] popView];
}

@end
