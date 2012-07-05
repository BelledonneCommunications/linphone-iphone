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


#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"ChatViewController" bundle:[NSBundle mainBundle]];
}


#pragma mark - ViewController Functions

- (void)viewWillAppear:(BOOL)animated {
    [super viewWillAppear:animated];
    [tableController setData:[ChatModel listConversations]];
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
    ChatModel* line= [[ChatModel alloc] init];
    line.localContact = @"";
    line.remoteContact = @"truc";
    line.message = @"blabla";
    line.direction = [NSNumber numberWithInt:1];
    line.time = [NSDate date];
    [line create];
    [tableController setData:[ChatModel listConversations]];
}

@end
