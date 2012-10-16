/* UIChatCell.m
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

#import "UIChatCell.h"
#import "PhoneMainView.h"
#import "LinphoneManager.h"
#import "Utils.h"

@implementation UIChatCell

@synthesize avatarImage;
@synthesize addressLabel;
@synthesize chatContentLabel;
@synthesize deleteButton;
@synthesize unreadMessageLabel;
@synthesize unreadMessageView;

@synthesize chat;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString*)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UIChatCell"
                                                              owner:self
                                                            options:nil];
        
        if ([arrayOfViews count] >= 1) {
            [self addSubview:[[arrayOfViews objectAtIndex:0] retain]];
        }
        [chatContentLabel setAdjustsFontSizeToFitWidth:TRUE]; // Auto shrink: IB lack!
    }
    return self;
}

- (void)dealloc {
    [addressLabel release];
    [chatContentLabel release];
    [avatarImage release];
    [deleteButton release];
    [unreadMessageLabel release];
    [unreadMessageView release];
    
    [chat release];
    
    [super dealloc];
}


#pragma mark - Property Funcitons

- (void)setChat:(ChatModel *)achat {
    if(chat == achat)
        return;
    if(chat != nil) {
        [chat release];
    }
    if(achat) {
        chat = [achat retain];
    }
    [self update];
}


#pragma mark - 

- (void)update {
	NSString *displayName = nil;
    UIImage *image = nil;
    if(chat == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat cell: null chat"];
        return;
    }
    LinphoneAddress* linphoneAddress = linphone_core_interpret_url([LinphoneManager getLc], [[chat remoteContact] UTF8String]);
	if (linphoneAddress == NULL)
		return;
	char *tmp = linphone_address_as_string_uri_only(linphoneAddress);
	NSString *normalizedSipAddress = [NSString stringWithUTF8String:tmp];
	ms_free(tmp);
    
    ABRecordRef contact = [[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
    if(contact != nil) {
        displayName = [FastAddressBook getContactDisplayName:contact];
        image = [FastAddressBook getContactImage:contact thumbnail:true];
    }
    
    // Display name
    if(displayName == nil) {
        displayName = [NSString stringWithUTF8String:linphone_address_get_username(linphoneAddress)];
    }
    [addressLabel setText:displayName];
    
    // Avatar
    if(image == nil) {
        image = [UIImage imageNamed:@"avatar_unknown_small.png"];
    }
    [avatarImage setImage:image];
    
    // Message
    if([chat isExternalImage] || [chat isInternalImage]) {
        [chatContentLabel setText:@""];
    } else {
        [chatContentLabel setText:[chat message]];
    }
    
    int count = [ChatModel unreadMessages:[chat remoteContact]];
    if(count > 0) {
        [unreadMessageView setHidden:FALSE];
        [unreadMessageLabel setText:[NSString stringWithFormat:@"%i", count]];
    } else {
        [unreadMessageView setHidden:TRUE];
    }
    
    linphone_address_destroy(linphoneAddress);
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
    } else {
        [deleteButton setAlpha:0.0f];    
    }
    if(animated) {
        [UIView commitAnimations];
    }
}


#pragma mark - Action Functions

- (IBAction)onDeleteClick: (id) event {
    if(chat != NULL) {
        UIView *view = [self superview]; 
        // Find TableViewCell
        if(view != nil && ![view isKindOfClass:[UITableView class]]) view = [view superview];
        if(view != nil) {
            UITableView *tableView = (UITableView*) view;
            NSIndexPath *indexPath = [tableView indexPathForCell:self];
            [[tableView dataSource] tableView:tableView commitEditingStyle:UITableViewCellEditingStyleDelete forRowAtIndexPath:indexPath];
        }
    }
}

@end
