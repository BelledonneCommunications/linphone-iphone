/* UIChatRoomHeader.m
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

#import "UIChatRoomHeader.h"

@implementation UIChatRoomHeader

@synthesize avatarImage;
@synthesize addressLabel;
@synthesize contact;


#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"UIChatRoomHeader" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [avatarImage release];
    [addressLabel release];
    [contact release];
    [super dealloc];
}


#pragma mark - Property Functions

- (void)setContact:(NSString *)acontact {
    if(contact != nil) {
        [contact release];
    }
    contact = [acontact copy];
    [self update];
}


#pragma mark - 

- (void)update {
    if(contact != nil) {
        [avatarImage setImage:[UIImage imageNamed:@"avatar_unknown_small.png"]];
        [addressLabel setText:contact];
    }
}

+ (CGFloat)height {
    return 80.0f;
}

@end
