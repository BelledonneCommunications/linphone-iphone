/* UIContactCell.m
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

#import "UIContactCell.h"

@implementation UIContactCell

@synthesize firstName;
@synthesize lastName;

- (id)init {
    if ((self = [super init]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UIContactCell"
                                                              owner:self
                                                            options:nil];
    
        if ([arrayOfViews count] >= 1) {
            [self addSubview:[[arrayOfViews objectAtIndex:0] retain]];
        }
    }
    return self;
}

- (void)touchUp:(id) sender {
    [self setHighlighted:true animated:true];
}

- (void)touchDown:(id) sender {
    [self setHighlighted:false animated:true];
}

- (void) update:(ABRecordRef) record {
    CFStringRef lFirstName = ABRecordCopyValue(record, kABPersonFirstNameProperty);
    CFStringRef lLocalizedFirstName = (lFirstName != nil)?ABAddressBookCopyLocalizedLabel(lFirstName):nil;
    CFStringRef lLastName = ABRecordCopyValue(record, kABPersonLastNameProperty);
    CFStringRef lLocalizedLastName = (lFirstName != nil)?ABAddressBookCopyLocalizedLabel(lLastName):nil;
    
    if(lLocalizedFirstName != nil)
        [firstName setText: [(NSString *)lLocalizedFirstName retain]];
    else
        [firstName setText: @""];
    
    if(lLocalizedLastName != nil)
        [lastName setText: [(NSString *)lLocalizedLastName retain]];
    else
        [lastName setText: @""];
    
    if(lLocalizedLastName != nil)
        CFRelease(lLocalizedLastName);
    if(lLastName != nil)
        CFRelease(lLastName);
    if(lLocalizedFirstName != nil)
        CFRelease(lLocalizedFirstName);
    if(lFirstName != nil)
        CFRelease(lFirstName);
    
    CGRect firstNameFrame = [firstName frame];
    CGRect lastNameFrame = [lastName frame];
    
    lastNameFrame.origin.x -= firstNameFrame.size.width;
    
    // Compute firstName size
    CGSize contraints;
    contraints.height = [firstName frame].size.height;
    contraints.width = ([lastName frame].size.width + [lastName frame].origin.x) - [firstName frame].origin.x;
    CGSize firstNameSize = [[firstName text] sizeWithFont:[firstName font] constrainedToSize: contraints];
    firstNameFrame.size.width = firstNameSize.width;
    
    // Compute lastName size & position
    lastNameFrame.origin.x += firstNameFrame.size.width;
    lastNameFrame.size.width = (contraints.width + [firstName frame].origin.x) - lastNameFrame.origin.x;
    
    [firstName setFrame: firstNameFrame];
    [lastName setFrame: lastNameFrame];
}

- (void) dealloc {
    [firstName release];
    [lastName release];

    [super dealloc];
}

@end
