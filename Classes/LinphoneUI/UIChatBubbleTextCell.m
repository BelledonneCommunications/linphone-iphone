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

@implementation UIChatBubbleTextCell

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		NSArray *arrayOfViews =
			[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
		// resize cell to match .nib size. It is needed when resized the cell to
		// correctly adapt its height too
		UIView *sub = ((UIView *)[arrayOfViews objectAtIndex:arrayOfViews.count - 1]);
		[self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
		[self addSubview:sub];
	}
	return self;
}

- (void)dealloc {
	[self setChatMessage:NULL];
}

#pragma mark -

- (void)setChatMessage:(LinphoneChatMessage *)amessage {
	if (amessage == _message) {
		return;
	}

	if (_message) {
		linphone_chat_message_unref(_message);
		linphone_chat_message_set_user_data(_message, NULL);
		linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(_message), NULL);
	}

	_message = amessage;
	if (amessage) {
		linphone_chat_message_ref(_message);
		linphone_chat_message_set_user_data(_message, (void *)CFBridgingRetain(self));
		linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(_message), message_status);
		[self update];
	}
}

+ (NSString *)TextMessageForChat:(LinphoneChatMessage *)_message {
	const char *text = linphone_chat_message_get_text(_message) ?: "";
	return [NSString stringWithUTF8String:text] ?: [NSString stringWithCString:text encoding:NSASCIIStringEncoding]
													   ?: NSLocalizedString(@"(invalid string)", nil);
}

- (NSString *)textMessage {
	return [self.class TextMessageForChat:_message];
}

- (void)update {
	if (_message == nil) {
		LOGW(@"Cannot update _message room cell: null _message");
		return;
	}

	if (_messageText) {
		[_messageText setHidden:FALSE];
		/* We need to use an attributed string here so that data detector don't mess
		 * with the text style. See http://stackoverflow.com/a/20669356 */

		NSAttributedString *attr_text =
			[[NSAttributedString alloc] initWithString:self.textMessage
											attributes:@{
												NSFontAttributeName : _messageText.font,
												NSForegroundColorAttributeName : [UIColor darkGrayColor]
											}];
		_messageText.attributedText = attr_text;
	}

	// Date
	_contactDateLabel.text = [NSString
		stringWithFormat:@"%@ - %@", [LinphoneUtils timeToString:linphone_chat_message_get_time(_message)
													  withFormat:NSLocalizedString(@"yyyy/MM/dd '-' HH'h'mm", nil)],
						 [FastAddressBook displayNameForAddress:linphone_chat_message_get_peer_address(_message)]];

	LinphoneChatMessageState state = linphone_chat_message_get_state(_message);
	BOOL outgoing = linphone_chat_message_is_outgoing(_message);

	if (outgoing) {
		[LinphoneUtils setSelfAvatar:_avatarImage];
	} else {
		ABRecordRef contact = [FastAddressBook getContactWithAddress:linphone_chat_message_get_peer_address(_message)];
		[_avatarImage setImage:[FastAddressBook imageForContact:contact thumbnail:YES]
					  bordered:NO
			 withRoundedRadius:YES];
	}
	_backgroundColorImage.image = _bottomBarColor.image =
		[UIImage imageNamed:(outgoing ? @"color_A.png" : @"color_D.png")];

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
		[_messageText setAccessibilityLabel:@"Outgoing _message"];
	} else {
		[_messageText setAccessibilityLabel:@"Incoming _message"];
	}
}

- (void)setEditing:(BOOL)editing {
	[self setEditing:editing animated:FALSE];
}

- (void)setEditing:(BOOL)editing animated:(BOOL)animated {
	_messageText.userInteractionEnabled = !editing;
}

#pragma mark - Action Functions

- (IBAction)onDeleteClick:(id)event {
	if (_message != NULL) {
		UITableView *tableView = VIEW(ChatConversationView).tableController.tableView;
		NSIndexPath *indexPath = [tableView indexPathForCell:self];
		[tableView.dataSource tableView:tableView
					 commitEditingStyle:UITableViewCellEditingStyleDelete
					  forRowAtIndexPath:indexPath];
	}
}

- (IBAction)onResendClick:(id)event {
	if (_message == nil)
		return;

	LinphoneChatMessageState state = linphone_chat_message_get_state(_message);
	if (state == LinphoneChatMessageStateNotDelivered) {
		if (linphone_chat_message_get_file_transfer_information(_message) != NULL) {
			NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:_message];
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

#pragma mark - Bubble size computing

- (CGSize)computeBoundingBox:(NSString *)text size:(CGSize)size font:(UIFont *)font {
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 70000
	if ([[[UIDevice currentDevice] systemVersion] doubleValue] >= 7) {
		return [text boundingRectWithSize:size
								  options:(NSStringDrawingUsesLineFragmentOrigin | NSStringDrawingUsesFontLeading)
							   attributes:@{
								   NSFontAttributeName : font
							   }
								  context:nil]
			.size;
	}
#endif
	{ return [text sizeWithFont:font constrainedToSize:size lineBreakMode:NSLineBreakByCharWrapping]; }
}

- (CGSize)viewSizeWithWidth:(int)width {
	static const CGFloat TEXT_MIN_HEIGHT = 32.;
	static const CGFloat TEXT_MIN_WIDTH = 150.0f;
	static const CGFloat MARGIN_WIDTH = 60;
	static const CGFloat MARGIN_HEIGHT = 19 + 16 /*this 16 is because textview add some top&bottom padding*/;
	static const CGFloat IMAGE_HEIGHT = 100.0f;
	static const CGFloat IMAGE_WIDTH = 100.0f;
	static const CGFloat CHECK_BOX_WIDTH = 40;

	int messageAvailableWidth = width - MARGIN_WIDTH - CHECK_BOX_WIDTH;

	const char *url = linphone_chat_message_get_external_body_url(_message);
	NSString *text = [UIChatBubbleTextCell TextMessageForChat:_message];

	CGSize messageSize;
	if (url != nil || linphone_chat_message_get_file_transfer_information(_message) != NULL) {
		messageSize = CGSizeMake(IMAGE_WIDTH, IMAGE_HEIGHT);
	} else {
		messageSize =
			[self computeBoundingBox:text size:CGSizeMake(messageAvailableWidth, CGFLOAT_MAX) font:_messageText.font];
		messageSize.width = MAX(TEXT_MIN_WIDTH, ceil(messageSize.width));
		messageSize.height = MAX(TEXT_MIN_HEIGHT, ceil(messageSize.height));
	}
	CGSize dateSize =
		[self computeBoundingBox:_contactDateLabel.text size:_contactDateLabel.frame.size font:_contactDateLabel.font];

	CGSize bubbleSize;
	bubbleSize.width = MAX(messageSize.width, dateSize.width + _statusImage.frame.size.width + 5) + MARGIN_WIDTH;
	bubbleSize.height = messageSize.height + MARGIN_HEIGHT;

	return bubbleSize;
}

- (void)layoutSubviews {
	[super layoutSubviews];
	if (_message != nil) {
		UITableView *tableView = VIEW(ChatConversationView).tableController.tableView;
		BOOL is_outgoing = linphone_chat_message_is_outgoing(_message);
		CGRect bubbleFrame = _bubbleView.frame;
		bubbleFrame.size = [self viewSizeWithWidth:self.frame.size.width];
		bubbleFrame.origin.x =
			tableView.isEditing ? 0 : (is_outgoing ? self.frame.size.width - bubbleFrame.size.width : 0);
		_bubbleView.frame = bubbleFrame;
	}
}

@end
