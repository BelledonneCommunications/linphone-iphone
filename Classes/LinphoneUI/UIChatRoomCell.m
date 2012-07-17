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
#import "Utils.h"

#import <NinePatch.h>

@implementation UIChatRoomCell

@synthesize contentView;
@synthesize messageView;
@synthesize backgroundImage;
@synthesize messageLabel;
@synthesize deleteButton;
@synthesize dateLabel;
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
    [messageView release];
    [messageLabel release];
    [deleteButton release];
    [dateLabel release];
    
    [chat release];
    
    [super dealloc];
}


#pragma mark - 

- (void)setChat:(ChatModel *)achat {
    if(chat != nil) {
        [chat release];
    }
    chat = achat;
    [self update];
}

- (void)prepareForReuse {
    [super prepareForReuse];
    self->chat = nil;
}

- (void)update {
    if(chat == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat room cell: null chat"];
        return;
    }
    [messageLabel setText:[chat message]];
    
    // Date
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
    [dateFormatter setDateStyle:NSDateFormatterMediumStyle];
    NSLocale *locale = [NSLocale currentLocale];
    [dateFormatter setLocale:locale];
    [dateLabel setText:[dateFormatter stringFromDate:[chat time]]];
    [dateFormatter release];
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

- (void)resizeContent {
    // Resize content
    {
        CGRect frame = [contentView frame];
        frame.origin.x = 0.0f;
        frame.origin.y = 0.0f;
        frame.size = [self frame].size;
    [   contentView setFrame:frame];
    }
    
    if(chat != nil) {
        CGPoint center = [contentView center];
        if(![[chat direction] intValue]) { // Inverted	
            [backgroundImage setImage:[TUNinePatchCache imageOfSize:[backgroundImage bounds].size
                                                  forNinePatchNamed:@"chat_bubble_incoming"]];
            center.y += 6;
        } else {
            [backgroundImage setImage:[TUNinePatchCache imageOfSize:[backgroundImage bounds].size
                                                  forNinePatchNamed:@"chat_bubble_outgoing"]];
            center.y -= 6;
        }
        [messageView setCenter:center];
    }
    
    // Resize messageView
    {
        CGRect frame = [messageView frame];
        frame.size.height = [UIChatRoomCell messageHeight:[chat message]] + 10;
        [messageView setFrame:frame];
    }
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
    height += 40;
    if(height < CELL_MIN_HEIGHT)
        height = CELL_MIN_HEIGHT;
    return height;
}


#pragma mark - View Functions

- (void)layoutSubviews {
    [super layoutSubviews];
    [self resizeContent];
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
