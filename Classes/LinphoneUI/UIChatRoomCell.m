/* UIChatRoomCell.m
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

#import "UIChatRoomCell.h"

#import <NinePatch.h>

@implementation UIChatRoomCell

@synthesize contentView;
@synthesize backgroundImage;
@synthesize messageLabel;
@synthesize deleteButton;
@synthesize chat;

static const CGFloat CELL_MIN_HEIGHT = 65.0f;
static const CGFloat CELL_MESSAGE_MAX_WIDTH = 280.0f;
static const CGFloat CELL_FONT_SIZE = 17.0f;
static UIFont *CELL_FONT = nil;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString*)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        [[NSBundle mainBundle] loadNibNamed:@"UIChatRoomCell"
                                      owner:self
                                    options:nil];
        [self addSubview:contentView];
    }
    return self;
}

- (void)dealloc {
    [backgroundImage release];
    [contentView release];
    [messageLabel release];
    [deleteButton release];
    
    [chat release];
    
    [super dealloc];
}


#pragma mark - 

- (void)update {
    if(chat != nil) {
        if([chat direction]) {
            [backgroundImage setImage:[TUNinePatchCache imageOfSize:[backgroundImage bounds].size
                                                  forNinePatchNamed:@"chat_bubble_incoming"]];
        } else {
            [backgroundImage setImage:[TUNinePatchCache imageOfSize:[backgroundImage bounds].size
                                                  forNinePatchNamed:@"chat_bubble_outgoing"]];
        }
        [messageLabel setText:[chat message]];
    }
    CGRect frame = [messageLabel frame];
    frame.size.height = [UIChatRoomCell messageHeight:[chat message]];
    [messageLabel setFrame:frame];
}

+ (CGFloat)messageHeight:(NSString*)message {
    if(CELL_FONT == nil) {
        CELL_FONT = [UIFont systemFontOfSize:CELL_FONT_SIZE];
    }
    CGSize messageSize = [message sizeWithFont: CELL_FONT
                           constrainedToSize: CGSizeMake(CELL_MESSAGE_MAX_WIDTH, 10000.0f) 
                               lineBreakMode: UILineBreakModeTailTruncation]; 
    return messageSize.height;
}

+ (CGFloat)height:(ChatModel*)chat {
    CGFloat height = [UIChatRoomCell messageHeight:[chat message]];
    height += 20;
    if(height < CELL_MIN_HEIGHT)
        height = CELL_MIN_HEIGHT;
    return height;
}

- (void)enterEditMode {
    [deleteButton setHidden:false];
}

- (void)exitEditMode {
    [deleteButton setHidden:true];
}

#pragma mark - View Functions

- (void)layoutSubviews {
    // Resize content
    CGRect frame = [contentView frame];
    frame.size = [self frame].size;
    [contentView setFrame:frame];
}


#pragma mark - Action Functions

- (IBAction)onDeleteClick: (id) event {
    if(chat != NULL) {
        [chat delete];
        UITableView *parentTable = (UITableView *)self.superview;
        [parentTable reloadData];
    }
}

@end
