/* UIHistoryCell.m
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
 *  GNU Library General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 

#import "UIHistoryCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

@implementation UIHistoryCell

@synthesize callLog;
@synthesize displayNameLabel;
@synthesize imageView;
@synthesize deleteButton;
@synthesize detailsButton;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString*)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UIHistoryCell"
                                                              owner:self
                                                            options:nil];
        
        if ([arrayOfViews count] >= 1) {
            [self addSubview:[[arrayOfViews objectAtIndex:0] retain]];
        }
        
        self->callLog = NULL;
    }
    return self;
}

- (void) dealloc {
    [detailsButton release];
    [deleteButton release];
    [displayNameLabel release];
    [imageView release];
    
    [super dealloc];
}


#pragma mark - Action Functions

- (void)setCallLog:(LinphoneCallLog *)acallLog {
    callLog = acallLog;
    [self update];
}


#pragma mark - Action Functions

- (IBAction)onDetails:(id) event {
    if(callLog != NULL) {
        // Go to History details view
        [[PhoneMainView instance] changeView:PhoneView_HistoryDetails 
                                       calls:[NSArray arrayWithObjects:
                                              [AbstractCall abstractCall:@"setCallLogValue:", [NSValue valueWithPointer: callLog]],
                                              nil]
                                        push:TRUE];
    }
}

- (IBAction)onDelete:(id)event {
    if(callLog != NULL) {
        linphone_core_remove_call_log([LinphoneManager getLc], callLog);
        UITableView *parentTable = (UITableView *)self.superview;
        [parentTable reloadData];
    }
}


#pragma mark - 

- (void)update {
    
    // Set up the cell...
	LinphoneAddress* partyToDisplay; 
	UIImage *image;
	if (callLog->dir == LinphoneCallIncoming) {
        if (callLog->status == LinphoneCallSuccess) {
            image = [UIImage imageNamed:@"call_status_incoming.png"];
        } else {
            image = [UIImage imageNamed:@"call_status_missed.png"];
        }
		partyToDisplay = callLog->from;
	} else {
		image = [UIImage imageNamed:@"call_status_outgoing.png"];
		partyToDisplay = callLog->to;
	}
    
	const char* username = (linphone_address_get_display_name(partyToDisplay) != 0)? linphone_address_get_display_name(partyToDisplay):linphone_address_get_username(partyToDisplay);

    [displayNameLabel setText:[NSString stringWithUTF8String: username]];
    [imageView setImage: image];
}

- (void)setEditing:(BOOL)editing {
    [self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
    if(animated) {
        [UIView beginAnimations:nil context:nil];
        [UIView setAnimationDuration:0.3];
    }
    if(editing) {
        [deleteButton setAlpha:1.0f];
        [detailsButton setAlpha:0.0f]; 
    } else {
        [detailsButton setAlpha:1.0f];
        [deleteButton setAlpha:0.0f];    
    }
    if(animated) {
        [UIView commitAnimations];
    }
}

@end
