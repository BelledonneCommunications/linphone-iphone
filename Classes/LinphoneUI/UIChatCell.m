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
#import "Utils.h"

@implementation UIChatCell

@synthesize avatarImage;
@synthesize displayNameLabel;
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
    [displayNameLabel release];
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
    
    [avatarImage setImage:[UIImage imageNamed:@"avatar_unknown_small.png"]];
    [displayNameLabel setText:[chat remoteContact]];
    [chatContentLabel setText:[chat message]];
    
    //
    // Adapt size
    //
    CGRect displayNameFrame = [displayNameLabel frame];
    CGRect chatContentFrame = [chatContentLabel frame];
    
    chatContentFrame.origin.x -= displayNameFrame.size.width;
    
    // Compute firstName size
    CGSize contraints;
    contraints.height = [displayNameLabel frame].size.height;
    contraints.width = ([chatContentLabel frame].size.width + [chatContentLabel frame].origin.x) - [displayNameLabel frame].origin.x;
    CGSize firstNameSize = [[displayNameLabel text] sizeWithFont:[displayNameLabel font] constrainedToSize: contraints];
    displayNameFrame.size.width = firstNameSize.width;
    
    // Compute lastName size & position
    chatContentFrame.origin.x += displayNameFrame.size.width;
    chatContentFrame.size.width = (contraints.width + [displayNameLabel frame].origin.x) - chatContentFrame.origin.x;
    
    [displayNameLabel setFrame: displayNameFrame];
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
    ChatRoomViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeView:PhoneView_ChatRoom  push:TRUE], ChatRoomViewController);
    if(controller !=nil) {
        [controller setRemoteContact:[chat remoteContact]];
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
