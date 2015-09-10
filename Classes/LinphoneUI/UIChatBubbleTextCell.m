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

#import "UIChatBubbleTextCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

#import <AssetsLibrary/ALAsset.h>
#import <AssetsLibrary/ALAssetRepresentation.h>

@implementation UIChatBubbleTextCell {
	LinphoneChatMessage *message;
}

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		NSArray *arrayOfViews = [[NSBundle mainBundle] loadNibNamed:@"UIChatBubbleTextCell" owner:self options:nil];
		[self.contentView addSubview:[arrayOfViews objectAtIndex:arrayOfViews.count - 1]];

#if 0
		// shift message box, otherwise it will collide with the bubble
		CGRect messageCoords = _messageText.frame;
		messageCoords.origin.x += 2;
		messageCoords.origin.y += 2;
		messageCoords.size.width -= 5;

		_messageText.frame = messageCoords;
		_messageText.allowSelectAll = TRUE;
#endif
	}

	return self;
}

- (void)dealloc {
	[self setChatMessage:NULL];
}

#pragma mark -

- (void)setChatMessage:(LinphoneChatMessage *)amessage {
	if (amessage == message) {
		return;
	}

	if (message) {
		linphone_chat_message_unref(message);
		linphone_chat_message_set_user_data(message, NULL);
		linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(message), NULL);
	}

	message = amessage;
	if (amessage) {
		linphone_chat_message_ref(message);
		linphone_chat_message_set_user_data(message, (void *)CFBridgingRetain(self));
		linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(message), message_status);
		[self update];
	}
}

+ (NSString *)TextMessageForChat:(LinphoneChatMessage *)message {
	const char *text = linphone_chat_message_get_text(message);
	return [NSString stringWithUTF8String:text] ?: [NSString stringWithCString:text encoding:NSASCIIStringEncoding]
													   ?: NSLocalizedString(@"(invalid string)", nil);
}

- (NSString *)textMessage {
	return [self.class TextMessageForChat:message];
}

- (void)update {
	if (message == nil) {
		LOGW(@"Cannot update message room cell: null message");
		return;
	}
	[_messageText setHidden:FALSE];
	/* We need to use an attributed string here so that data detector don't mess
	 * with the text style. See http://stackoverflow.com/a/20669356 */

	NSAttributedString *attr_text =
		[[NSAttributedString alloc] initWithString:self.textMessage
										attributes:@{
											NSFontAttributeName : [UIFont systemFontOfSize:17.0],
											NSForegroundColorAttributeName : [UIColor darkGrayColor]
										}];
	_messageText.attributedText = attr_text;

	// Date
	_contactDateLabel.text =
		[LinphoneUtils timeToString:linphone_chat_message_get_time(message) withStyle:NSDateFormatterMediumStyle];

	LinphoneChatMessageState state = linphone_chat_message_get_state(message);
	BOOL outgoing = linphone_chat_message_is_outgoing(message);

	if (!outgoing) {
		_statusImage.accessibilityValue = @"incoming";
		_statusImage.hidden = TRUE; // not useful for incoming chats..
	} else if (state == LinphoneChatMessageStateInProgress) {
		_statusImage.image = [UIImage imageNamed:@"chat_message_inprogress.png"];
		_statusImage.accessibilityValue = @"in progress";
		_statusImage.hidden = FALSE;
	} else if (state == LinphoneChatMessageStateDelivered) {
		_statusImage.image = [UIImage imageNamed:@"chat_message_delivered.png"];
		_statusImage.accessibilityValue = @"delivered";
		_statusImage.hidden = FALSE;
	} else {
		_statusImage.image = [UIImage imageNamed:@"chat_message_not_delivered.png"];
		_statusImage.accessibilityValue = @"not delivered";
		_statusImage.hidden = FALSE;

		NSAttributedString *resend_text =
			[[NSAttributedString alloc] initWithString:NSLocalizedString(@"Resend", @"Resend")
											attributes:@{NSForegroundColorAttributeName : [UIColor redColor]}];
		[_contactDateLabel setAttributedText:resend_text];
	}

	if (outgoing) {
		[_messageText setAccessibilityLabel:@"Outgoing message"];
	} else {
		[_messageText setAccessibilityLabel:@"Incoming message"];
	}
}

- (void)setEditing:(BOOL)editing {
	[self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	if (animated) {
		[UIView beginAnimations:nil context:nil];
		[UIView setAnimationDuration:0.3];
	}
	_deleteButton.hidden = !editing;
	if (animated) {
		[UIView commitAnimations];
	}
}

#pragma mark - View Functions

- (void)layoutSubviews {
	[super layoutSubviews];
	if (message != nil) {
		//		BOOL is_outgoing = linphone_chat_message_is_outgoing(message);
		CGRect newFrame;
		newFrame.size = [ChatConversationTableView viewSize:message width:self.frame.size.width];
		newFrame.origin.y = 0.0f;
		//		newFrame.origin.x = is_outgoing ? self.contentView.frame.size.width - newFrame.size.width : 0;
		self.contentView.frame = self.frame = newFrame;
	}
}

#pragma mark - Action Functions

- (IBAction)onDeleteClick:(id)event {
	if (message != NULL) {
		UITableView *tableView = VIEW(ChatConversationView).tableController.tableView;
		NSIndexPath *indexPath = [tableView indexPathForCell:self];
		[tableView.dataSource tableView:tableView
					 commitEditingStyle:UITableViewCellEditingStyleDelete
					  forRowAtIndexPath:indexPath];
	}
}

- (IBAction)onResendClick:(id)event {
	if (message == nil)
		return;

	LinphoneChatMessageState state = linphone_chat_message_get_state(message);
	if (state == LinphoneChatMessageStateNotDelivered) {
		if (linphone_chat_message_get_file_transfer_information(message) != NULL) {
			NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:message];
			NSURL *imageUrl = [NSURL URLWithString:localImage];

			[self onDeleteClick:nil];

			[[LinphoneManager instance]
					.photoLibrary assetForURL:imageUrl
				resultBlock:^(ALAsset *asset) {
				  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL),
								 ^(void) {
								   UIImage *image = [[UIImage alloc] initWithCGImage:[asset thumbnail]];
								   [_chatRoomDelegate startImageUpload:image url:imageUrl];
								 });
				}
				failureBlock:^(NSError *error) {
				  LOGE(@"Can't read image");
				}];
		} else {
			[self onDeleteClick:nil];

			double delayInSeconds = 0.4;
			dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
			dispatch_after(popTime, dispatch_get_main_queue(), ^(void) {
			  [_chatRoomDelegate resendChat:self.textMessage withExternalUrl:nil];
			});
		}
	}
}
#pragma mark - State changed handling
static void message_status(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	UIChatBubbleTextCell *thiz = (__bridge UIChatBubbleTextCell *)linphone_chat_message_get_user_data(msg);
	LOGI(@"State for message [%p] changed to %s", msg, linphone_chat_message_state_to_string(state));
	[thiz update];
}

@end
