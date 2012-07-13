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
@synthesize backButton;
@synthesize cancelButton;


#pragma mark - Lifecycle Functions

- (id)init  {
    self = [super initWithNibName:@"ContactDetailsViewController" bundle:[NSBundle mainBundle]];
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    [tableController release];
    
    [editButton release];
    [backButton release];
    [cancelButton release];
    
    [super dealloc];
}


#pragma mark - 

- (void)newContact {
    [tableController newContact];
    [self enableEdit:FALSE];
    [[tableController tableView] reloadData];
}

- (void)newContact:(NSString*)address {
    [tableController newContact];
    [tableController addSipField:address];
    [self enableEdit:FALSE];
    [[tableController tableView] reloadData];
}

- (void)editContact:(ABRecordRef)acontact {
    self->contact = acontact;
    [tableController setContactID:ABRecordGetRecordID(acontact)];
    [self enableEdit:FALSE];
    [[tableController tableView] reloadData];
}

- (void)editContact:(ABRecordRef)acontact address:(NSString*)address {
    self->contact = acontact;
    [tableController setContactID:ABRecordGetRecordID(acontact)];
    [tableController addSipField:address];
    [self enableEdit:FALSE];
    [[tableController tableView] reloadData];
}

#pragma mark - Property Functions

- (void)setContact:(ABRecordRef)acontact {
    self->contact = acontact;
    [tableController setContactID:ABRecordGetRecordID(acontact)];
    [self disableEdit:FALSE];
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
}

- (void)viewDidUnload {
    [super viewDidUnload];
    [tableController->footerController->removeButton removeTarget:self 
                                                           action:@selector(onRemove:) 
                                                 forControlEvents:UIControlEventTouchUpInside];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewWillDisappear:NO];
    }
    [self disableEdit:FALSE];
    [tableController resetData];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    if ([[UIDevice currentDevice].systemVersion doubleValue] < 5.0) {
        [tableController viewWillAppear:NO];
    }   
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


- (void)enableEdit:(BOOL)animated {
    if(![tableController isEditing]) {
        [tableController setEditing:TRUE animated:animated];
    }
    [editButton setOn];
    [cancelButton setHidden:FALSE];
    [backButton setHidden:TRUE];
}

- (void)disableEdit:(BOOL)animated {
    if([tableController isEditing]) {
        [tableController setEditing:FALSE animated:animated];
    }
    [editButton setOff];
    [cancelButton setHidden:TRUE];
    [backButton setHidden:FALSE];
}

#pragma mark - Action Functions

- (IBAction)onCancelClick:(id)event {
    [self disableEdit:FALSE];
    [tableController resetData];
    if([tableController contactID] == kABRecordInvalidID) {
        [[PhoneMainView instance] popView];
    }
}

- (IBAction)onBackClick:(id)event {
    [[PhoneMainView instance] popView];
}

- (IBAction)onEditClick:(id)event {
    if([tableController isEditing]) {
        [self disableEdit:TRUE];
        [tableController saveData];
    } else {
        [self enableEdit:TRUE];
    }
}

- (void)onRemove:(id)event {
    [self disableEdit:FALSE];
    [tableController removeContact];
    [[PhoneMainView instance] popView];
}

@end
