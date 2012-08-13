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
@synthesize detailsButton;
@synthesize deleteButton;

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
    }
    return self;
}

- (void)dealloc {
    [addressLabel release];
    [chatContentLabel release];
    [avatarImage release];
    [detailsButton release];
    [deleteButton release];
    
    [chat release];
    
    [super dealloc];
}


#pragma mark - Property Funcitons

- (void)setChat:(ChatModel *)achat {
    chat = achat;
    [self update];
}


#pragma mark - 

- (void)update {
    if(chat == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat cell: null chat"];
        return;
    }
    
    NSString *displayName = nil;
    UIImage *image = nil;
    NSString *normalizedSipAddress = [FastAddressBook normalizeSipURI:[chat remoteContact]];
    ABRecordRef contact =[[[LinphoneManager instance] fastAddressBook] getContact:normalizedSipAddress];
    if(contact != nil) {
        displayName = [FastAddressBook getContactDisplayName:contact];
        image = [FastAddressBook getContactImage:contact thumbnail:true];
    }
    
    // Display name
    if(displayName == nil) {
        displayName = [chat remoteContact];
    }
    [addressLabel setText:displayName];
    
    // Avatar
    if(image == nil) {
        image = [UIImage imageNamed:@"avatar_unknown_small.png"];
    }
    [avatarImage setImage:image];
    
    // Message
    [chatContentLabel setText:[chat message]];
}

- (void)layoutSubviews {
    [super layoutSubviews];
    //
    // Adapt size
    //
    CGRect displayNameFrame = [addressLabel frame];
    CGRect chatContentFrame = [chatContentLabel frame];
    
    // Compute firstName size
    CGSize displayNameSize = [[addressLabel text] sizeWithFont:[addressLabel font]];
    CGSize chatContentSize = [[chatContentLabel text] sizeWithFont:[chatContentLabel font]];
    float sum = displayNameSize.width + 5 + chatContentSize.width;
    float limit = self.bounds.size.width - 5 - displayNameFrame.origin.x;
    if(sum >limit) {
        displayNameSize.width *= limit/sum;
        chatContentSize.width *= limit/sum;
    }
    
    displayNameFrame.size.width = displayNameSize.width;
    chatContentFrame.size.width = chatContentSize.width;
    
    // Compute lastName size & position
    chatContentFrame.origin.x = displayNameFrame.origin.x + displayNameFrame.size.width;
    if(displayNameFrame.size.width)
        chatContentFrame.origin.x += 5;
    
    [addressLabel setFrame: displayNameFrame];
    [chatContentLabel setFrame: chatContentFrame];
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

#pragma mark - Action Functions

- (IBAction)onDetailsClick: (id) event {
    // Go to Chat room view
    ChatRoomViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ChatRoomViewController compositeViewDescription]  push:TRUE], ChatRoomViewController);
    if(controller !=nil) {
        [controller setRemoteAddress:[chat remoteContact]];
    }
}

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
