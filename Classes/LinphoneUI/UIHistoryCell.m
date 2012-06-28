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

@implementation UIHistoryCell

@synthesize displayNameLabel;
@synthesize imageView;
@synthesize deleteButton;
@synthesize detailsButton;

#define DETAILS_DISABLED

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

- (IBAction)onDetails:(id) event {
    if(callLog != NULL) {
        
    }
}

- (IBAction)onDelete:(id)event {
    if(callLog != NULL) {
        linphone_core_remove_call_log([LinphoneManager getLc], callLog);
        UITableView *parentTable = (UITableView *)self.superview;
        [parentTable reloadData];
    }
}

- (void)update:(LinphoneCallLog*)  aCallLog {
    self->callLog = aCallLog;
    // Set up the cell...
	LinphoneAddress* partyToDisplay; 
	UIImage *image;
	if (aCallLog->dir == LinphoneCallIncoming) {
        if (aCallLog->status == LinphoneCallSuccess) {
            image = [UIImage imageNamed:aCallLog->video_enabled?@"appel-entrant.png":@"appel-entrant.png"];
        } else {
            //missed call
            image = [UIImage imageNamed:@"appel-manque.png"];
        }
		partyToDisplay = aCallLog->from;
	} else {
		image = [UIImage imageNamed:aCallLog->video_enabled?@"appel-sortant.png":@"appel-sortant.png"];
		partyToDisplay = aCallLog->to;
	}
    
	const char* username = linphone_address_get_username(partyToDisplay)!=0?linphone_address_get_username(partyToDisplay):"";
    //const char* displayName = linphone_address_get_display_name(partyToDisplay);
    
    [displayNameLabel setText:[NSString stringWithUTF8String: username]];
    [imageView setImage: image];
}

- (void)enterEditMode {
    [deleteButton setHidden:false];
    [detailsButton setHidden:true];
}

- (void)exitEditMode {
#ifdef DETAILS_DISABLED
    [detailsButton setHidden:true];
#else
    [detailsButton setHidden:false];
#endif
    [deleteButton setHidden:true];
}

- (void) dealloc {
    [detailsButton release];
    [deleteButton release];
    [displayNameLabel release];
    [imageView release];
    
    [super dealloc];
}

@end
