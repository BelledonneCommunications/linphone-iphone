/* HistoryDetailsViewController.m
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

#import "HistoryDetailsViewController.h"
#import "PhoneMainView.h"
#import "FastAddressBook.h"

@implementation HistoryDetailsViewController

@synthesize callLog;
@synthesize avatarImage;
@synthesize addressLabel;
@synthesize dateLabel;
@synthesize dateHeaderLabel;
@synthesize durationLabel;
@synthesize durationHeaderLabel;
@synthesize typeLabel;
@synthesize typeHeaderLabel;
@synthesize addressButton;


#pragma mark - LifeCycle Functions

- (void)dealloc {
    [avatarImage release];
    [addressLabel release];
    [dateLabel release];
    [dateHeaderLabel release];
    [durationLabel release];
    [durationHeaderLabel release];
    [typeLabel release];
    [typeHeaderLabel release];
    [addressButton release];
    
     [super dealloc];
}


#pragma mark - UICompositeViewDelegate Functions

+ (UICompositeViewDescription*) compositeViewDescription {
    UICompositeViewDescription *description = [UICompositeViewDescription alloc];
    description->content = @"HistoryDetailsViewController";
    description->tabBar = @"UIMainBar";
    description->tabBarEnabled = true;
    description->stateBar = nil;
    description->stateBarEnabled = false;
    description->fullscreen = false;
    return description;
}


#pragma mark - Property Functions

- (void)setCallLogValue:(NSValue*)vcallLog {
    [self setCallLog:[vcallLog pointerValue]];
}

- (void)setCallLog:(LinphoneCallLog *)acallLog {
    self->callLog = acallLog;
    [self update];
}


#pragma mark - 

- (void)update {
    // Set up the cell...
	LinphoneAddress* partyToDisplay; 
	if (callLog->dir == LinphoneCallIncoming) {
		partyToDisplay = callLog->from;
	} else {
		partyToDisplay = callLog->to;
	}
}


#pragma mark - Action Functions

- (IBAction)onBackClick:(id)event {
    [[PhoneMainView instance] popView];
}

- (IBAction)onContactClick:(id)event {
    
}

- (IBAction)onAddressClick:(id)event {

}

@end
