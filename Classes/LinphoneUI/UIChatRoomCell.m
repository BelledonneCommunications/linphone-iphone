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

static const CGFloat CELL_MIN_HEIGHT = 40.0f;
static const CGFloat CELL_MIN_WIDTH = 150.0f;
static const CGFloat CELL_MAX_WIDTH = 320.0f;
static const CGFloat CELL_MESSAGE_X_MARGIN = 26.0f;
static const CGFloat CELL_MESSAGE_Y_MARGIN = 33.0f;
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
    chat = [achat retain];
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
    if(chat != nil) {
        // Resize Content
        CGRect contentFrame = [contentView frame];
        contentFrame.size = [UIChatRoomCell viewSize:[chat message] width:[self frame].size.width];
        if([[chat direction] intValue]) { // Inverted
            contentFrame.origin.x = 0.0f;
            contentFrame.origin.y = 0.0f;
        } else {
            contentFrame.origin.x = [self frame].size.width - contentFrame.size.width;
            contentFrame.origin.y = 0.0f;   
        }
        [contentView setFrame:contentFrame];
        
        CGRect messageFrame = [messageView frame];
        messageFrame.origin.y = ([contentView frame].size.height - messageFrame.size.height)/2;
        if([[chat direction] intValue]) { // Inverted	
            [backgroundImage setImage:[TUNinePatchCache imageOfSize:[backgroundImage bounds].size
                                                  forNinePatchNamed:@"chat_bubble_incoming"]];
            messageFrame.origin.y += 5;
        } else {
            [backgroundImage setImage:[TUNinePatchCache imageOfSize:[backgroundImage bounds].size
                                                  forNinePatchNamed:@"chat_bubble_outgoing"]];
            messageFrame.origin.y -= 5;
        }
        [messageView setFrame:messageFrame];
    }
}

+ (CGSize)viewSize:(NSString*)message width:(int)width {
    if(CELL_FONT == nil) {
        CELL_FONT = [UIFont systemFontOfSize:CELL_FONT_SIZE];
    }
    CGSize messageSize = [message sizeWithFont: CELL_FONT
                           constrainedToSize: CGSizeMake(width - CELL_MESSAGE_X_MARGIN, 10000.0f) 
                               lineBreakMode: UILineBreakModeTailTruncation]; 
    messageSize.height += CELL_MESSAGE_Y_MARGIN;
    if(messageSize.height < CELL_MIN_HEIGHT)
         messageSize.height = CELL_MIN_HEIGHT;
    messageSize.width += CELL_MESSAGE_X_MARGIN;
    if(messageSize.width < CELL_MIN_WIDTH)
        messageSize.width = CELL_MIN_WIDTH;
    return messageSize;
}

+ (CGFloat)height:(ChatModel*)chat width:(int)width {
    return [UIChatRoomCell viewSize:[chat message] width:width].height;
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
