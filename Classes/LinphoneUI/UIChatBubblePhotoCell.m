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

#import "UIChatBubblePhotoCell.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"

#import <AssetsLibrary/ALAsset.h>
#import <AssetsLibrary/ALAssetRepresentation.h>

@implementation UIChatBubblePhotoCell {
	LinphoneChatMessage *message;
	FileTransferDelegate *ftd;
}

static const CGFloat CELL_MIN_HEIGHT = 50.0f;
static const CGFloat CELL_MIN_WIDTH = 150.0f;
static const CGFloat CELL_MESSAGE_X_MARGIN = 26.0f + 10.0f;
static const CGFloat CELL_MESSAGE_Y_MARGIN = 36.0f;
static const CGFloat CELL_FONT_SIZE = 17.0f;
static const CGFloat CELL_IMAGE_HEIGHT = 100.0f;
static const CGFloat CELL_IMAGE_WIDTH = 100.0f;
static UIFont *CELL_FONT = nil;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];

		// shift message box, otherwise it will collide with the bubble
		CGRect messageCoords = _messageText.frame;
		messageCoords.origin.x += 2;
		messageCoords.origin.y += 2;
		messageCoords.size.width -= 5;

		_messageText.frame = messageCoords;
		_messageText.allowSelectAll = TRUE;
	}

	return self;
}

- (void)dealloc {
	[self disconnectFromFileDelegate];
	if (message) {
		linphone_chat_message_set_user_data(message, NULL);
		linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(message), NULL);
		linphone_chat_message_unref(message);
		message = NULL;
	}
}

#pragma mark -

- (void)setChatMessage:(LinphoneChatMessage *)amessage {
	if (amessage == message) {
		return;
	}

	[self disconnectFromFileDelegate];
	_messageImageView.image = nil;

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

		const LinphoneContent *c = linphone_chat_message_get_file_transfer_information(message);
		if (c) {
			const char *name = linphone_content_get_name(c);
			for (FileTransferDelegate *aftd in [[LinphoneManager instance] fileTransferDelegates]) {
				if (linphone_chat_message_get_file_transfer_information(aftd.message) &&
					strcmp(name, linphone_content_get_name(
									 linphone_chat_message_get_file_transfer_information(aftd.message))) == 0) {
					LOGI(@"Chat message [%p] with file transfer delegate [%p], connecting to it!", message, aftd);
					[self connectToFileDelegate:aftd];
					break;
				}
			}
		}
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
	const char *url = linphone_chat_message_get_external_body_url(message);
	BOOL is_external =
		(url && (strstr(url, "http") == url)) || linphone_chat_message_get_file_transfer_information(message);
	NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:message];

	// this is an image (either to download or already downloaded)
	if (is_external || localImage) {
		if (localImage) {
			if (_messageImageView.image == nil) {
				NSURL *imageUrl = [NSURL URLWithString:localImage];
				_messageText.hidden = YES;
				[_messageImageView startLoading];
				__block LinphoneChatMessage *achat = message;
				[LinphoneManager.instance.photoLibrary assetForURL:imageUrl
					resultBlock:^(ALAsset *asset) {
					  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL),
									 ^(void) {
									   if (achat == message) { // Avoid glitch and scrolling
										   UIImage *image = [[UIImage alloc] initWithCGImage:[asset thumbnail]];
										   dispatch_async(dispatch_get_main_queue(), ^{
											 [_messageImageView setImage:image];
											 [_messageImageView setFullImageUrl:asset];
											 [_messageImageView stopLoading];
											 _messageImageView.hidden = NO;
										   });
									   }
									 });
					}
					failureBlock:^(NSError *error) {
					  LOGE(@"Can't read image");
					}];
			}
			if (ftd.message != nil) {
				_cancelButton.hidden = NO;
				_fileTransferProgress.hidden = NO;
				_downloadButton.hidden = YES;
			} else {
				_cancelButton.hidden = _fileTransferProgress.hidden = _downloadButton.hidden = YES;
			}
		} else {
			_messageText.hidden = YES;
			_messageImageView.hidden = _cancelButton.hidden = _fileTransferProgress.hidden = (ftd.message == nil);
			_downloadButton.hidden = !_cancelButton.hidden;
		}
		// simple text message
	} else {
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
		_messageImageView.hidden = YES;
		_cancelButton.hidden = _fileTransferProgress.hidden = _downloadButton.hidden = YES;
	}

	// Date
	_dateLabel.text =
		[LinphoneUtils timeToString:linphone_chat_message_get_time(message) withStyle:NSDateFormatterMediumStyle];

	LinphoneChatMessageState state = linphone_chat_message_get_state(message);
	BOOL outgoing = linphone_chat_message_is_outgoing(message);

	if (!outgoing) {
		[_statusImage setAccessibilityValue:@"incoming"];
		_statusImage.hidden = TRUE; // not useful for incoming chats..
	} else if (state == LinphoneChatMessageStateInProgress) {
		[_statusImage setImage:[UIImage imageNamed:@"chat_message_inprogress.png"]];
		[_statusImage setAccessibilityValue:@"in progress"];
		_statusImage.hidden = FALSE;
	} else if (state == LinphoneChatMessageStateDelivered || state == LinphoneChatMessageStateFileTransferDone) {
		[_statusImage setImage:[UIImage imageNamed:@"chat_message_delivered.png"]];
		[_statusImage setAccessibilityValue:@"delivered"];
		_statusImage.hidden = FALSE;
	} else {
		[_statusImage setImage:[UIImage imageNamed:@"chat_message_not_delivered.png"]];
		[_statusImage setAccessibilityValue:@"not delivered"];
		_statusImage.hidden = FALSE;

		NSAttributedString *resend_text =
			[[NSAttributedString alloc] initWithString:NSLocalizedString(@"Resend", @"Resend")
											attributes:@{NSForegroundColorAttributeName : [UIColor redColor]}];
		[_dateLabel setAttributedText:resend_text];
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

+ (CGSize)viewSize:(LinphoneChatMessage *)message width:(int)width {
	CGSize messageSize;
	const char *url = linphone_chat_message_get_external_body_url(message);
	if (url == nil && linphone_chat_message_get_file_transfer_information(message) == NULL) {
		NSString *text = [self.class TextMessageForChat:message];
		if (CELL_FONT == nil) {
			CELL_FONT = [UIFont systemFontOfSize:CELL_FONT_SIZE];
		}
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 70000
		if (UIDevice.currentDevice.systemVersion.doubleValue >= 7) {
			messageSize =
				[text boundingRectWithSize:CGSizeMake(width - CELL_MESSAGE_X_MARGIN, CGFLOAT_MAX)
								   options:(NSStringDrawingUsesLineFragmentOrigin |
											NSStringDrawingTruncatesLastVisibleLine | NSStringDrawingUsesFontLeading)
								attributes:@{
									NSFontAttributeName : CELL_FONT
								} context:nil]
					.size;
		} else
#endif
		{
			messageSize = [text sizeWithFont:CELL_FONT
						   constrainedToSize:CGSizeMake(width - CELL_MESSAGE_X_MARGIN, 10000.0f)
							   lineBreakMode:NSLineBreakByTruncatingTail];
		}
	} else {
		messageSize = CGSizeMake(CELL_IMAGE_WIDTH, CELL_IMAGE_HEIGHT);
	}
	messageSize.height += CELL_MESSAGE_Y_MARGIN;
	if (messageSize.height < CELL_MIN_HEIGHT)
		messageSize.height = CELL_MIN_HEIGHT;
	messageSize.width += CELL_MESSAGE_X_MARGIN;
	if (messageSize.width < CELL_MIN_WIDTH)
		messageSize.width = CELL_MIN_WIDTH;
	return messageSize;
}

+ (CGFloat)height:(LinphoneChatMessage *)chatMessage width:(int)width {
	return [self.class viewSize:chatMessage width:width].height;
}

#pragma mark - View Functions

- (void)layoutSubviews {
	[super layoutSubviews];
	if (message != nil) {
		BOOL is_outgoing = linphone_chat_message_is_outgoing(message);
		CGRect innerFrame;
		innerFrame.size = [self.class viewSize:message width:[self frame].size.width];
		innerFrame.origin.y = 0.0f;
		innerFrame.origin.x = is_outgoing ? self.frame.size.width - innerFrame.size.width : 0;
		_innerView.frame = innerFrame;

		CGRect messageFrame = _bubbleView.frame;
		messageFrame.origin.y = (_innerView.frame.size.height - messageFrame.size.height) / 2;
		if (!is_outgoing) {
			messageFrame.origin.y += 5;
		} else {
			messageFrame.origin.y -= 5;
		}
		_backgroundImage.image =
			is_outgoing ? [UIImage imageNamed:@"chat_bubble_outgoing"] : [UIImage imageNamed:@"chat_bubble_incoming"];
		_bubbleView.frame = messageFrame;
	}
}

#pragma mark - Action Functions

- (IBAction)onDeleteClick:(id)event {
	if (message != NULL) {
		[ftd cancel];
		UITableView *tableView = VIEW(ChatConversationView).tableController.tableView;
		NSIndexPath *indexPath = [tableView indexPathForCell:self];
		[tableView.dataSource tableView:tableView
					 commitEditingStyle:UITableViewCellEditingStyleDelete
					  forRowAtIndexPath:indexPath];
	}
}

- (IBAction)onDownloadClick:(id)event {
	[ftd cancel];
	ftd = [[FileTransferDelegate alloc] init];
	[self connectToFileDelegate:ftd];
	[ftd download:message];
	_cancelButton.hidden = NO;
	_downloadButton.hidden = YES;
}

- (IBAction)onCancelDownloadClick:(id)sender {
	FileTransferDelegate *tmp = ftd;
	[self disconnectFromFileDelegate];
	[tmp cancel];
	[self update];
}

- (IBAction)onImageClick:(id)event {
	LinphoneChatMessageState state = linphone_chat_message_get_state(message);
	if (state == LinphoneChatMessageStateNotDelivered) {
		[self onResendClick:event];
	} else {
		if (![_messageImageView isLoading]) {
			ImageView *view = VIEW(ImageView);
			[PhoneMainView.instance changeCurrentView:view.compositeViewDescription push:TRUE];
			CGImageRef fullScreenRef = [[_messageImageView.fullImageUrl defaultRepresentation] fullScreenImage];
			UIImage *fullScreen = [UIImage imageWithCGImage:fullScreenRef];
			[view setImage:fullScreen];
		}
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
	UIChatBubblePhotoCell *thiz = (__bridge UIChatBubblePhotoCell *)linphone_chat_message_get_user_data(msg);
	LOGI(@"State for message [%p] changed to %s", msg, linphone_chat_message_state_to_string(state));
	if (linphone_chat_message_get_file_transfer_information(msg) != NULL) {
		if (state == LinphoneChatMessageStateDelivered || state == LinphoneChatMessageStateNotDelivered) {
			// we need to refresh the tableview because the filetransfer delegate unreffed
			// the message message before state was LinphoneChatMessageStateFileTransferDone -
			// if we are coming back from another view between unreffing and change of state,
			// the transient message will not be found and it will not appear in the list of
			// message, so we must refresh the table when we change to this state to ensure that
			// all transient messages apppear
			//		ChatRoomViewController *controller = DYNAMIC_CAST(
			//			[PhoneMainView.instance changeCurrentView:ChatRoomViewController.compositeViewDescription
			// push:TRUE],
			//			ChatRoomViewController);
			//		[controller.tableController setChatRoom:linphone_chat_message_get_chat_room(msg)];
			// This is breaking interface too much, it must be fixed in file transfer cb.. meanwhile, disabling it.

			if (thiz->ftd) {
				[thiz->ftd stopAndDestroy];
				thiz->ftd = nil;
			}
		}
	}
	[thiz update];
}

#pragma mark - LinphoneFileTransfer Notifications Handling

- (void)connectToFileDelegate:(FileTransferDelegate *)aftd {
	ftd = aftd;
	_fileTransferProgress.progress = 0;
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(onFileTransferSendUpdate:)
												 name:kLinphoneFileTransferSendUpdate
											   object:ftd];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(onFileTransferRecvUpdate:)
												 name:kLinphoneFileTransferRecvUpdate
											   object:ftd];
}

- (void)disconnectFromFileDelegate {
	[[NSNotificationCenter defaultCenter] removeObserver:self];
	ftd = nil;
}

- (void)onFileTransferSendUpdate:(NSNotification *)notif {
	LinphoneChatMessageState state = [[[notif userInfo] objectForKey:@"state"] intValue];

	if (state == LinphoneChatMessageStateInProgress) {
		float progress = [[[notif userInfo] objectForKey:@"progress"] floatValue];
		// When uploading a file, the message file is first uploaded to the server,
		// so we are in progress state. Then state goes to filetransfertdone. Then,
		// the exact same message is sent to the other participant and we come
		// back to in progress again. This second time is NOT an upload, so we must
		// not update progress!
		_fileTransferProgress.progress = MAX(_fileTransferProgress.progress, progress);
		_fileTransferProgress.hidden = _cancelButton.hidden = (_fileTransferProgress.progress == 1.f);
	} else {
		[self update];
	}
}
- (void)onFileTransferRecvUpdate:(NSNotification *)notif {
	LinphoneChatMessageState state = [[[notif userInfo] objectForKey:@"state"] intValue];
	if (state == LinphoneChatMessageStateInProgress) {
		float progress = [[[notif userInfo] objectForKey:@"progress"] floatValue];
		_fileTransferProgress.progress = MAX(_fileTransferProgress.progress, progress);
		_fileTransferProgress.hidden = _cancelButton.hidden = (_fileTransferProgress.progress == 1.f);
	} else {
		[self update];
	}
}

@end
