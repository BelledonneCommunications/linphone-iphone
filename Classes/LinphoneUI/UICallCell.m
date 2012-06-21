/* UICallCell.m
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

#import "UICallCell.h"

@implementation UICallCell

@synthesize firstBackground;
@synthesize otherBackground;
@synthesize stateView;
@synthesize addressLabel;
@synthesize timeLabel;

- (id)init {
    if ((self = [super init]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UICallCell"
                                                              owner:self
                                                            options:nil];
        
        if ([arrayOfViews count] >= 1) {
            [self addSubview:[[arrayOfViews objectAtIndex:0] retain]];
        }
    }
    return self;
}

- (void)firstCell{
    [firstBackground setHidden:false];
    [otherBackground setHidden:true];
}

- (void)otherCell{
    [firstBackground setHidden:true];
    [otherBackground setHidden:false];
}

- (void)updateCell:(LinphoneCall *)call {
    const LinphoneAddress* addr = linphone_call_get_remote_address(call);
    
    if (addr) {
		const char* lUserNameChars=linphone_address_get_username(addr);
		NSString* lUserName = lUserNameChars?[[[NSString alloc] initWithUTF8String:lUserNameChars] autorelease]:NSLocalizedString(@"Unknown",nil);
        NSMutableString* mss = [[NSMutableString alloc] init];
        // contact name 
        const char* n = linphone_address_get_display_name(addr);
        if (n) 
            [mss appendFormat:@"%s", n, nil];
        else
            [mss appendFormat:@"%@",lUserName , nil];
        
        [addressLabel setText:mss];
        
        // TODO
        //imageView.image = [[LinphoneManager instance] getImageFromAddressBook:lUserName];
		[mss release];
    } else {
        [addressLabel setText:@"Unknown"];
        //TODO
        //imageView.image = nil;
    }
    
    
    NSMutableString* msDuration = [[NSMutableString alloc] init ];
    int duration = linphone_call_get_duration(call);
    [msDuration appendFormat:@"%02i:%02i", (duration/60), duration - 60 * (duration / 60), nil];
    [timeLabel setText:msDuration];
    [msDuration release];
}

@end
