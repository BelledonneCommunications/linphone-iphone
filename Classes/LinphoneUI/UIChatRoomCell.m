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
#import "UILinphone.h"
#import "Utils.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

#import <AssetsLibrary/ALAsset.h>
#import <AssetsLibrary/ALAssetRepresentation.h>
#import <NinePatch.h>
#include "linphone/linphonecore.h"

@implementation UIChatRoomCell

@synthesize innerView;
@synthesize bubbleView;
@synthesize backgroundImage;
@synthesize messageImageView;
@synthesize messageText;
@synthesize deleteButton;
@synthesize dateLabel;
@synthesize statusImage;
@synthesize downloadButton;
@synthesize chatRoomDelegate;
@synthesize imageTapGestureRecognizer;
@synthesize resendTapGestureRecognizer;

static const CGFloat CELL_MIN_HEIGHT = 50.0f;
static const CGFloat CELL_MIN_WIDTH = 150.0f;
//static const CGFloat CELL_MAX_WIDTH = 320.0f;
static const CGFloat CELL_MESSAGE_X_MARGIN = 26.0f + 10.0f;
static const CGFloat CELL_MESSAGE_Y_MARGIN = 36.0f;
static const CGFloat CELL_FONT_SIZE = 17.0f;
static const CGFloat CELL_IMAGE_HEIGHT = 100.0f;
static const CGFloat CELL_IMAGE_WIDTH = 100.0f;
static UIFont *CELL_FONT = nil;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString*)identifier {
    if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
        [[NSBundle mainBundle] loadNibNamed:@"UIChatRoomCell"
                                      owner:self
                                    options:nil];
        imageTapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onImageClick:)];
        [messageImageView addGestureRecognizer:imageTapGestureRecognizer];

        resendTapGestureRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onResendClick:)];
        [dateLabel addGestureRecognizer:resendTapGestureRecognizer];

        [self addSubview:innerView];
        [deleteButton setAlpha:0.0f];
        
        // shift message box, otherwise it will collide with the bubble
        CGRect messageCoords = [messageText frame];
        messageCoords.origin.x   += 2;
        messageCoords.origin.y   += 2;
        messageCoords.size.width -= 5;
        [messageText setFrame:messageCoords];
        messageText.allowSelectAll = TRUE;
    }
    return self;
}

- (void)dealloc {
    [chatRoomDelegate release];
    [backgroundImage release];
    [innerView release];
    [bubbleView release];
    [messageText release];
    [messageImageView release];
    [deleteButton release];
    [dateLabel release];
    [statusImage release];
    [downloadButton release];
    [imageTapGestureRecognizer release];
    [resendTapGestureRecognizer release];
    
    [super dealloc];
}

#pragma mark - 

- (void)setChatMessage:(LinphoneChatMessage *)message {
    self->chat = message;
	[self update];
	
}

+ (NSString*)decodeTextMessage:(const char*)text {
    NSString* decoded = [NSString stringWithUTF8String:text];
    if( decoded == nil ){
        // couldn't decode the string as UTF8, do a lossy conversion
        decoded = [NSString stringWithCString:text encoding:NSASCIIStringEncoding];
        if( decoded == nil ){
            decoded = @"(invalid string)";
        }
    }
    return decoded;
}

- (void)update {
    if(chat == nil) {
        [LinphoneLogger logc:LinphoneLoggerWarning format:"Cannot update chat room cell: null chat"];
        return;
    }
    const char*      url = linphone_chat_message_get_external_body_url(chat);
    const char*     text = linphone_chat_message_get_text(chat);
    BOOL     is_external = url && (strstr(url, "http") == url);
    NSString* localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:chat];


    if(is_external && !localImage ) {
        [messageText setHidden:TRUE];
        [messageImageView setImage:nil];
        [messageImageView setHidden:TRUE];
        [downloadButton setHidden:FALSE];

    } else if(localImage) {

        NSURL* imageUrl = [NSURL URLWithString:localImage];

        [messageText setHidden:TRUE];
        [messageImageView setImage:nil];
        [messageImageView startLoading];
        __block LinphoneChatMessage *achat = chat;
        [[LinphoneManager instance].photoLibrary assetForURL:imageUrl resultBlock:^(ALAsset *asset) {
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL), ^(void) {
                if(achat == self->chat) { //Avoid glitch and scrolling
					UIImage* image = [[UIImage alloc] initWithCGImage:[asset thumbnail]];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [messageImageView setImage:image];
                        [messageImageView setFullImageUrl:asset];
                        [messageImageView stopLoading];
                        [image release];
                    });
                }
            });
        } failureBlock:^(NSError *error) {
            [LinphoneLogger log:LinphoneLoggerError format:@"Can't read image"];
        }];
        
        [messageImageView setHidden:FALSE];
        [downloadButton setHidden:TRUE];
    } else {
        // simple text message
        [messageText setHidden:FALSE];
        if ( text ){
            NSString* nstext = [UIChatRoomCell decodeTextMessage:text];

            /* We need to use an attributed string here so that data detector don't mess
             * with the text style. See http://stackoverflow.com/a/20669356 */

            NSAttributedString* attr_text = [[NSAttributedString alloc]
                                             initWithString:nstext
                                             attributes:@{NSFontAttributeName:[UIFont systemFontOfSize:17.0],
                                                          NSForegroundColorAttributeName:[UIColor darkGrayColor]}];
            messageText.attributedText = attr_text;
            [attr_text release];

        } else {
            messageText.text = @"";
        }

        [messageImageView setImage:nil];
        [messageImageView setHidden:TRUE];
        
        [downloadButton setHidden:TRUE];
    }
    
    // Date
    NSDate* message_date = [NSDate dateWithTimeIntervalSince1970:linphone_chat_message_get_time(chat)];
    NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
    [dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
    [dateFormatter setDateStyle:NSDateFormatterMediumStyle];
    NSLocale *locale = [NSLocale currentLocale];
    [dateFormatter setLocale:locale];
    [dateLabel setText:[dateFormatter stringFromDate:message_date]];
    [dateFormatter release];

    LinphoneChatMessageState state = linphone_chat_message_get_state(chat);
    BOOL outgoing = linphone_chat_message_is_outgoing(chat);

    if( !outgoing ){
        statusImage.hidden = TRUE; // not useful for incoming chats..
    } else if (state== LinphoneChatMessageStateInProgress) {
		[statusImage setImage:[UIImage imageNamed:@"chat_message_inprogress.png"]];
        [statusImage setAccessibilityValue:@"in progress"];
		statusImage.hidden = FALSE;
	} else if (state == LinphoneChatMessageStateDelivered) {
		[statusImage setImage:[UIImage imageNamed:@"chat_message_delivered.png"]];
        [statusImage setAccessibilityValue:@"delivered"];
		statusImage.hidden = FALSE;
	} else {
		[statusImage setImage:[UIImage imageNamed:@"chat_message_not_delivered.png"]];
        [statusImage setAccessibilityValue:@"not delivered"];
		statusImage.hidden = FALSE;

        NSAttributedString* resend_text = [[NSAttributedString alloc]
                                           initWithString:NSLocalizedString(@"Resend", @"Resend")
                                           attributes:@{NSForegroundColorAttributeName: [UIColor redColor]}];
        [dateLabel setAttributedText:resend_text];
        [resend_text release];
	}
    
    if( outgoing){
        [messageText setAccessibilityLabel:@"Outgoing message"];
    } else {
        [messageText setAccessibilityLabel:@"Incoming message"];
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

+ (CGSize)viewSize:(LinphoneChatMessage*)chat width:(int)width {
    CGSize messageSize;
    const char* url  = linphone_chat_message_get_external_body_url(chat);
    const char* text = linphone_chat_message_get_text(chat);
    NSString* messageText = text ? [UIChatRoomCell decodeTextMessage:text] : @"";
    if(url == nil) {
        if(CELL_FONT == nil) {
            CELL_FONT = [UIFont systemFontOfSize:CELL_FONT_SIZE];
        }

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 70000

        if( [[[UIDevice currentDevice] systemVersion] doubleValue] >= 7){
            messageSize = [messageText
                           boundingRectWithSize:CGSizeMake(width - CELL_MESSAGE_X_MARGIN, CGFLOAT_MAX)
                           options:(NSStringDrawingUsesLineFragmentOrigin|NSStringDrawingTruncatesLastVisibleLine|NSStringDrawingUsesFontLeading)
                           attributes:@{NSFontAttributeName: CELL_FONT}
                           context:nil].size;
        } else
#endif
        {
            messageSize = [messageText sizeWithFont: CELL_FONT
                                     constrainedToSize: CGSizeMake(width - CELL_MESSAGE_X_MARGIN, 10000.0f)
                                         lineBreakMode: NSLineBreakByTruncatingTail];
        }
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

+ (CGFloat)height:(LinphoneChatMessage*)chatMessage width:(int)width {
    return [UIChatRoomCell viewSize:chatMessage width:width].height;
}


#pragma mark - View Functions

- (void)layoutSubviews {
    [super layoutSubviews];
    if(chat != nil) {
        // Resize inner
        CGRect innerFrame;
        BOOL is_outgoing = linphone_chat_message_is_outgoing(chat);
        innerFrame.size = [UIChatRoomCell viewSize:chat width:[self frame].size.width];
        if(!is_outgoing) { // Inverted
            innerFrame.origin.x = 0.0f;
            innerFrame.origin.y = 0.0f;
        } else {
            innerFrame.origin.x = [self frame].size.width - innerFrame.size.width;
            innerFrame.origin.y = 0.0f;
        }
        [innerView setFrame:innerFrame];

        CGRect messageFrame = [bubbleView frame];
        messageFrame.origin.y = ([innerView frame].size.height - messageFrame.size.height)/2;
        if(!is_outgoing) { // Inverted
            UIImage* image = [UIImage imageNamed:@"chat_bubble_incoming"];
            image = [image resizableImageWithCapInsets:UIEdgeInsetsMake(26, 32, 34, 56)];
            [backgroundImage setImage:image];
            messageFrame.origin.y += 5;
        } else {
            UIImage* image = [UIImage imageNamed:@"chat_bubble_outgoing"];
            image = [image resizableImageWithCapInsets:UIEdgeInsetsMake(14, 15, 25, 40)];
            [backgroundImage setImage:image];
            messageFrame.origin.y -= 5;
        }
        [bubbleView setFrame:messageFrame];
    }
}


#pragma mark - Action Functions

- (IBAction)onDeleteClick:(id)event {
    if(chat != NULL) {
        UIView *view = [self superview]; 
        // Find TableViewCell
        while(view != nil && ![view isKindOfClass:[UITableView class]]) view = [view superview];
        if(view != nil) {
            UITableView *tableView = (UITableView*) view;
            NSIndexPath *indexPath = [tableView indexPathForCell:self];
            [[tableView dataSource] tableView:tableView commitEditingStyle:UITableViewCellEditingStyleDelete forRowAtIndexPath:indexPath];
        }
    }
}

- (IBAction)onDownloadClick:(id)event {
    NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:linphone_chat_message_get_external_body_url(chat)]];
    [chatRoomDelegate chatRoomStartImageDownload:url userInfo:[NSValue valueWithPointer:chat]];

}

- (IBAction)onImageClick:(id)event {
    if(![messageImageView isLoading]) {
        ImageViewController *controller = DYNAMIC_CAST([[PhoneMainView instance] changeCurrentView:[ImageViewController compositeViewDescription] push:TRUE], ImageViewController);
        if(controller != nil) {
            CGImageRef fullScreenRef = [[messageImageView.fullImageUrl defaultRepresentation] fullScreenImage];
            UIImage* fullScreen = [UIImage imageWithCGImage:fullScreenRef];
            [controller setImage:fullScreen];
        }
    }
}

- (IBAction)onResendClick:(id)event {
    if( chat == nil ) return;

    LinphoneChatMessageState state = linphone_chat_message_get_state(self->chat);
    if (state == LinphoneChatMessageStateNotDelivered) {
        const char* text = linphone_chat_message_get_text(self->chat);
        const char* url = linphone_chat_message_get_external_body_url(self->chat);
        NSString* message = text ? [NSString stringWithUTF8String:text] : nil;
        NSString* exturl  = url ? [NSString stringWithUTF8String:url] : nil;

        [self onDeleteClick:nil];

        double delayInSeconds = 0.4;
        dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
        dispatch_after(popTime, dispatch_get_main_queue(), ^(void){
            [chatRoomDelegate resendChat:message withExternalUrl:exturl];
        });
    }
}

@end
