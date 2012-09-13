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
#include "linphonecore.h"

@implementation UIChatRoomCell

@synthesize innerView;
@synthesize bubbleView;
@synthesize backgroundImage;
@synthesize messageImageView;
@synthesize messageLabel;
@synthesize deleteButton;
@synthesize dateLabel;
@synthesize chat;
@synthesize statusImage;

static const CGFloat CELL_MIN_HEIGHT = 40.0f;
static const CGFloat CELL_MIN_WIDTH = 150.0f;
static const CGFloat CELL_MAX_WIDTH = 320.0f;
static const CGFloat CELL_MESSAGE_X_MARGIN = 26.0f;
static const CGFloat CELL_MESSAGE_Y_MARGIN = 36.0f;
static const CGFloat CELL_FONT_SIZE = 17.0f;
static const CGFloat CELL_IMAGE_HEIGHT = 50.0f;
static const CGFloat CELL_IMAGE_WIDTH = 50.0f;
static UIFont *CELL_FONT = nil;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString*)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        [[NSBundle mainBundle] loadNibNamed:@"UIChatRoomCell"
                                      owner:self
                                    options:nil];
        [self addSubview:innerView];
    }
    return self;
}

- (void)dealloc {
    [backgroundImage release];
    [innerView release];
    [bubbleView release];
    [messageLabel release];
    [messageImageView release];
    [deleteButton release];
    [dateLabel release];
    [statusImage release];
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
    chat = nil;
}

- (void)update {
    if(chat == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat room cell: null chat"];
        return;
    }
    if(true/*Change when image will be supported */) {
        [messageLabel setHidden:FALSE];
        [messageLabel setText:[chat message]];
        
        [messageImageView setHidden:TRUE];
    } else {
        [messageLabel setHidden:TRUE];
        
        [messageImageView setHidden:FALSE];
    }
    
    // Date
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
    [dateFormatter setDateStyle:NSDateFormatterMediumStyle];
    NSLocale *locale = [NSLocale currentLocale];
    [dateFormatter setLocale:locale];
    [dateLabel setText:[dateFormatter stringFromDate:[chat time]]];
    [dateFormatter release];
	if ([chat.state intValue] == LinphoneChatMessageStateInProgress) {
		[statusImage setImage:[UIImage imageNamed:@"chat_message_inprogress.png"] ];
		statusImage.hidden=FALSE;
	} else if ([chat.state intValue] == LinphoneChatMessageStateDelivered) {
		[statusImage setImage:[UIImage imageNamed:@"chat_message_delivered.png"] ];
		statusImage.hidden=FALSE;
	} else if ([chat.state intValue] == LinphoneChatMessageStateNotDelivered) {
		[statusImage setImage:[UIImage imageNamed:@"chat_message_not_delivered.png"]];
		statusImage.hidden=FALSE;
	} else {
		statusImage.hidden=TRUE;
	}
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

+ (CGSize)viewSize:(ChatModel*)chat width:(int)width {
    CGSize messageSize;
    if(true/*Change when image will be supported */) {
        if(CELL_FONT == nil) {
            CELL_FONT = [UIFont systemFontOfSize:CELL_FONT_SIZE];
        }
        messageSize = [[chat message] sizeWithFont: CELL_FONT
                                        constrainedToSize: CGSizeMake(width - CELL_MESSAGE_X_MARGIN, 10000.0f)
                                            lineBreakMode: UILineBreakModeTailTruncation];
    } else {
        messageSize = CGSizeMake(CELL_IMAGE_WIDTH, CELL_IMAGE_HEIGHT);
    }
    messageSize.height += CELL_MESSAGE_Y_MARGIN;
    if(messageSize.height < CELL_MIN_HEIGHT)
        messageSize.height = CELL_MIN_HEIGHT;
    messageSize.width += CELL_MESSAGE_X_MARGIN;
    if(messageSize.width < CELL_MIN_WIDTH)
        messageSize.width = CELL_MIN_WIDTH;
    return messageSize;
}

+ (CGFloat)height:(ChatModel*)chat width:(int)width {
    return [UIChatRoomCell viewSize:chat width:width].height;
}


#pragma mark - View Functions

- (void)layoutSubviews {
    [super layoutSubviews];
    if(chat != nil) {
        // Resize inner
        CGRect innerFrame;
        innerFrame.size = [UIChatRoomCell viewSize:chat width:[self frame].size.width];
        if([[chat direction] intValue]) { // Inverted
            innerFrame.origin.x = 0.0f;
            innerFrame.origin.y = 0.0f;
        } else {
            innerFrame.origin.x = [self frame].size.width - innerFrame.size.width;
            innerFrame.origin.y = 0.0f;
        }
        [innerView setFrame:innerFrame];

        CGRect messageFrame = [bubbleView frame];
        messageFrame.origin.y = ([innerView frame].size.height - messageFrame.size.height)/2;
        if([[chat direction] intValue]) { // Inverted
            [backgroundImage setImage:[TUNinePatchCache imageOfSize:[backgroundImage bounds].size
                                                  forNinePatchNamed:@"chat_bubble_incoming"]];
            messageFrame.origin.y += 5;
        } else {
            [backgroundImage setImage:[TUNinePatchCache imageOfSize:[backgroundImage bounds].size
                                                  forNinePatchNamed:@"chat_bubble_outgoing"]];
            messageFrame.origin.y -= 5;
        }
        [bubbleView setFrame:messageFrame];
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
