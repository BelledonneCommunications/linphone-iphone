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

@implementation UIHistoryCell

@synthesize displayName;
@synthesize imageView;

- (id)init {
    if ((self = [super init]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UIHistoryCell"
                                                              owner:self
                                                            options:nil];
        
        if ([arrayOfViews count] >= 1) {
            [self addSubview:[[arrayOfViews objectAtIndex:0] retain]];
        }
    }
    return self;
}

- (IBAction)onDetails: (id) event {
    
}

- (void)update:(LinphoneCallLog*)  callLogs {
    // Set up the cell...
	LinphoneAddress* partyToDisplay; 
	NSString *name;
	if (callLogs->dir == LinphoneCallIncoming) {
        if (callLogs->status == LinphoneCallSuccess) {
            name = callLogs->video_enabled?@"appel-entrant.png":@"appel-entrant.png";
        } else {
            //missed call
            name = @"appel-manque.png";
        }
		partyToDisplay=callLogs->from;
		
	} else {
		name = callLogs->video_enabled?@"appel-sortant.png":@"appel-sortant.png";
		partyToDisplay=callLogs->to;
		
	}
	UIImage *image = [UIImage imageNamed:name];
	
	const char* username = linphone_address_get_username(partyToDisplay)!=0?linphone_address_get_username(partyToDisplay):"";
    
    //TODO
    //const char* displayName = linphone_address_get_display_name(partyToDisplay);
    
    [displayName setText:[NSString stringWithFormat:@"%s", username]];
    [imageView setImage: image];
}

- (void) dealloc {
    [displayName release];
    [imageView release];
    
    [super dealloc];
}

@end
