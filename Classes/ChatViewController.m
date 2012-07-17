/* ChatViewController.m
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

#import "ChatViewController.h"
#import "PhoneMainView.h"

#import "ChatModel.h"
@implementation ChatViewController

@synthesize tableController;
@synthesize editButton;

#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"ChatViewController" bundle:[NSBundle mainBundle]];
}


- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    
    [tableController release];
    [editButton release];
    
    [super dealloc];
}

#pragma mark - ViewController Functions

- (void)viewDidLoad {
    [super viewDidLoad];
    
    // Set selected+over background: IB lack !
    [editButton setImage:[UIImage imageNamed:@"chat_ok_over.png"] 
                forState:(UIControlStateHighlighted | UIControlStateSelected)];
}

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [[NSNotificationCenter defaultCenter] addObserver:self 
                                             selector:@selector(textReceivedEvent:) 
                                                 name:@"LinphoneTextReceived" 
                                               object:nil];
    if([tableController isEditing])
        [tableController setEditing:FALSE animated:FALSE];
    [editButton setOff];
    [tableController loadData];
}

- (void)viewWillDisappear:(BOOL)animated {
    [super viewWillDisappear:animated];
    
    [[NSNotificationCenter defaultCenter] removeObserver:self 
                                                    name:@"LinphoneTextReceived" 
                                                  object:nil];
}


#pragma mark - Event Functions

- (void)textReceivedEvent:(NSNotification *)notif {
    [tableController loadData];
}


#pragma mark - UICompositeViewDelegate Functions

+ (UICompositeViewDescription*) compositeViewDescription {
    UICompositeViewDescription *description = [UICompositeViewDescription alloc];
    description->content = @"ChatViewController";
    description->tabBar = @"UIMainBar";
    description->tabBarEnabled = true;
    description->stateBar = nil;
    description->stateBarEnabled = false;
    description->fullscreen = false;
    return description;
}


#pragma mark - Action Functions

- (IBAction)onAddClick:(id)event {
    [[PhoneMainView instance] changeView:PhoneView_ChatRoom push:TRUE];
}

- (IBAction)onEditClick:(id)event {
    [tableController setEditing:![tableController isEditing] animated:TRUE];
}

@end
