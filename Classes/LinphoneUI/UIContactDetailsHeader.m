/* UIContactDetailsHeader.m
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

#import "UIContactDetailsHeader.h"

@implementation UIContactDetailsHeader

@synthesize avatarImage;
@synthesize contactLabel;
@synthesize contact;


#pragma mark - Lifecycle Functions

- (id)init {
    return [super initWithNibName:@"UIContactDetailsHeader" bundle:[NSBundle mainBundle]];
}

- (void)dealloc {
    [avatarImage release];
    [contactLabel release];
    [super dealloc];
}


#pragma mark - 

- (void)update {
    if(contact) {
        // Avatar image
        {
            NSData  *imgData = (NSData *)ABPersonCopyImageDataWithFormat(contact, kABPersonImageFormatThumbnail);
            if(imgData != NULL) {
                UIImage *img = [[UIImage alloc] initWithData:imgData];
                [avatarImage setImage:img];
                [img release];
            } else {
                [avatarImage setImage:[UIImage imageNamed:@"avatar_unknown_small.png"]];
            }
        }
    
        // Contact label
        {
            CFStringRef lFirstName = ABRecordCopyValue(contact, kABPersonFirstNameProperty);
            CFStringRef lLocalizedFirstName = (lFirstName != nil)?ABAddressBookCopyLocalizedLabel(lFirstName):nil;
            CFStringRef lLastName = ABRecordCopyValue(contact, kABPersonLastNameProperty);
            CFStringRef lLocalizedLastName = (lFirstName != nil)?ABAddressBookCopyLocalizedLabel(lLastName):nil;
            [contactLabel setText:[NSString stringWithFormat:@"%@ %@", (NSString*)lLocalizedFirstName, (NSString*)lLocalizedLastName]];
            if(lLocalizedLastName != nil)
                CFRelease(lLocalizedLastName);
            if(lLastName != nil)
                CFRelease(lLastName);
            if(lLocalizedFirstName != nil)
                CFRelease(lLocalizedFirstName);
            if(lFirstName != nil)
                CFRelease(lFirstName);
        }
    }
}

+ (CGFloat)height {
    return 80.0f;
}

@end
