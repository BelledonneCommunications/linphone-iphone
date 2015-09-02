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

@implementation UIChatRoomCell {
	FileTransferDelegate *ftd;
}

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
// static const CGFloat CELL_MAX_WIDTH = 320.0f;
static const CGFloat CELL_MESSAGE_X_MARGIN = 26.0f + 10.0f;
static const CGFloat CELL_MESSAGE_Y_MARGIN = 36.0f;
static const CGFloat CELL_FONT_SIZE = 17.0f;
static const CGFloat CELL_IMAGE_HEIGHT = 100.0f;
static const CGFloat CELL_IMAGE_WIDTH = 100.0f;
static UIFont *CELL_FONT = nil;

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		[[NSBundle mainBundle] loadNibNamed:@"UIChatRoomCell" owner:self options:nil];
		imageTapGestureRecognizer =
			[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onImageClick:)];
		[messageImageView addGestureRecognizer:imageTapGestureRecognizer];

		resendTapGestureRecognizer =
			[[UITapGestureRecognizer alloc] initWithTarget:self action:@selector(onResendClick:)];
		[innerView addGestureRecognizer:resendTapGestureRecognizer];

		[self addSubview:innerView];
		[deleteButton setAlpha:0.0f];

		// shift message box, otherwise it will collide with the bubble
		CGRect messageCoords = [messageText frame];
		messageCoords.origin.x += 2;
		messageCoords.origin.y += 2;
		messageCoords.size.width -= 5;
		[messageText setFrame:messageCoords];
		messageText.allowSelectAll = TRUE;
	}

	return self;
}

- (void)dealloc {
	[self disconnectFromFileDelegate];
	if (self->chat) {
		linphone_chat_message_unref(self->chat);
		linphone_chat_message_set_user_data(self->chat, NULL);
		linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(self->chat), NULL);
		self->chat = NULL;
	}
}

#pragma mark -

- (void)setChatMessage:(LinphoneChatMessage *)message {
	if (message != self->chat) {
		if (self->chat) {
			linphone_chat_message_unref(self->chat);
			linphone_chat_message_set_user_data(self->chat, NULL);
			linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(self->chat), NULL);
		}
		self->chat = message;
		messageImageView.image = nil;
		[self disconnectFromFileDelegate];
		if (self->chat) {
			linphone_chat_message_ref(self->chat);
			linphone_chat_message_set_user_data(self->chat, (void *)CFBridgingRetain(self));
			linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(self->chat),
															message_status);

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
		}
		[self update];
	}
}

+ (NSString *)decodeTextMessage:(const char *)text {
	NSString *decoded = [NSString stringWithUTF8String:text];
	if (decoded == nil) {
		// couldn't decode the string as UTF8, do a lossy conversion
		decoded = [NSString stringWithCString:text encoding:NSASCIIStringEncoding];
		if (decoded == nil) {
			decoded = @"(invalid string)";
		}
	}
	return decoded;
}

- (void)update {
	if (chat == nil) {
		LOGW(@"Cannot update chat room cell: null chat");
		return;
	}
	const char *url = linphone_chat_message_get_external_body_url(chat);
	const char *text = linphone_chat_message_get_text(chat);
	BOOL is_external =
		(url && (strstr(url, "http") == url)) || linphone_chat_message_get_file_transfer_information(chat);
	NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:chat];

	// this is an image (either to download or already downloaded)
	if (is_external || localImage) {
		if (localImage) {
			if (messageImageView.image == nil) {
				NSURL *imageUrl = [NSURL URLWithString:localImage];
				messageText.hidden = YES;
				[messageImageView startLoading];
				__block LinphoneChatMessage *achat = chat;
				[[LinphoneManager instance]
						.photoLibrary assetForURL:imageUrl
					resultBlock:^(ALAsset *asset) {
					  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL),
									 ^(void) {
									   if (achat == self->chat) { // Avoid glitch and scrolling
										   UIImage *image = [[UIImage alloc] initWithCGImage:[asset thumbnail]];
										   dispatch_async(dispatch_get_main_queue(), ^{
											 [messageImageView setImage:image];
											 [messageImageView setFullImageUrl:asset];
											 [messageImageView stopLoading];
											 messageImageView.hidden = NO;
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
				downloadButton.hidden = YES;
			} else {
				_cancelButton.hidden = _fileTransferProgress.hidden = downloadButton.hidden = YES;
			}
		} else {
			messageText.hidden = YES;
			messageImageView.hidden = _cancelButton.hidden = _fileTransferProgress.hidden = (ftd.message == nil);
			downloadButton.hidden = !_cancelButton.hidden;
		}
		// simple text message
	} else {
		[messageText setHidden:FALSE];
		if (text) {
			NSString *nstext = [UIChatRoomCell decodeTextMessage:text];

			/* We need to use an attributed string here so that data detector don't mess
			* with the text style. See http://stackoverflow.com/a/20669356 */

			NSAttributedString *attr_text =
				[[NSAttributedString alloc] initWithString:nstext
												attributes:@{
													NSFontAttributeName : [UIFont systemFontOfSize:17.0],
													NSForegroundColorAttributeName : [UIColor darkGrayColor]
												}];
			messageText.attributedText = attr_text;

		} else {
			messageText.text = @"";
		}
		messageImageView.hidden = YES;
		_cancelButton.hidden = _fileTransferProgress.hidden = downloadButton.hidden = YES;
	}

	// Date
	time_t chattime = linphone_chat_message_get_time(chat);
	NSDate *message_date = (chattime == 0) ? [[NSDate alloc] init] : [NSDate dateWithTimeIntervalSince1970:chattime];
	NSDateFormatter *dateFormatter = [[NSDateFormatter alloc] init];
	[dateFormatter setTimeStyle:NSDateFormatterMediumStyle];
	[dateFormatter setDateStyle:NSDateFormatterMediumStyle];
	NSLocale *locale = [NSLocale currentLocale];
	[dateFormatter setLocale:locale];
	[dateLabel setText:[dateFormatter stringFromDate:message_date]];

	LinphoneChatMessageState state = linphone_chat_message_get_state(chat);
	BOOL outgoing = linphone_chat_message_is_outgoing(chat);

	if (!outgoing) {
		[statusImage setAccessibilityValue:@"incoming"];
		statusImage.hidden = TRUE; // not useful for incoming chats..
	} else if (state == LinphoneChatMessageStateInProgress) {
		[statusImage setImage:[UIImage imageNamed:@"chat_message_inprogress.png"]];
		[statusImage setAccessibilityValue:@"in progress"];
		statusImage.hidden = FALSE;
	} else if (state == LinphoneChatMessageStateDelivered || state == LinphoneChatMessageStateFileTransferDone) {
		[statusImage setImage:[UIImage imageNamed:@"chat_message_delivered.png"]];
		[statusImage setAccessibilityValue:@"delivered"];
		statusImage.hidden = FALSE;
	} else {
		[statusImage setImage:[UIImage imageNamed:@"chat_message_not_delivered.png"]];
		[statusImage setAccessibilityValue:@"not delivered"];
		statusImage.hidden = FALSE;

		NSAttributedString *resend_text =
			[[NSAttributedString alloc] initWithString:NSLocalizedString(@"Resend", @"Resend")
											attributes:@{NSForegroundColorAttributeName : [UIColor redColor]}];
		[dateLabel setAttributedText:resend_text];
	}

	if (outgoing) {
		[messageText setAccessibilityLabel:@"Outgoing message"];
	} else {
		[messageText setAccessibilityLabel:@"Incoming message"];
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
	if (editing) {
		[deleteButton setAlpha:1.0f];
	} else {
		[deleteButton setAlpha:0.0f];
	}
	if (animated) {
		[UIView commitAnimations];
	}
}

+ (CGSize)viewSize:(LinphoneChatMessage *)chat width:(int)width {
	CGSize messageSize;
	const char *url = linphone_chat_message_get_external_body_url(chat);
	const char *text = linphone_chat_message_get_text(chat);
	NSString *messageText = text ? [UIChatRoomCell decodeTextMessage:text] : @"";
	if (url == nil && linphone_chat_message_get_file_transfer_information(chat) == NULL) {
		if (CELL_FONT == nil) {
			CELL_FONT = [UIFont systemFontOfSize:CELL_FONT_SIZE];
		}

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 70000

		if ([[[UIDevice currentDevice] systemVersion] doubleValue] >= 7) {
			messageSize = [messageText boundingRectWithSize:CGSizeMake(width - CELL_MESSAGE_X_MARGIN, CGFLOAT_MAX)
													options:(NSStringDrawingUsesLineFragmentOrigin |
															 NSStringDrawingTruncatesLastVisibleLine |
															 NSStringDrawingUsesFontLeading)
												 attributes:@{
													 NSFontAttributeName : CELL_FONT
												 } context:nil]
							  .size;
		} else
#endif
		{
			messageSize = [messageText sizeWithFont:CELL_FONT
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
	return [UIChatRoomCell viewSize:chatMessage width:width].height;
}

#pragma mark - View Functions

- (void)layoutSubviews {
	[super layoutSubviews];
	if (chat != nil) {
		// Resize inner
		CGRect innerFrame;
		BOOL is_outgoing = linphone_chat_message_is_outgoing(chat);
		innerFrame.size = [UIChatRoomCell viewSize:chat width:[self frame].size.width];
		if (!is_outgoing) { // Inverted
			innerFrame.origin.x = 0.0f;
			innerFrame.origin.y = 0.0f;
		} else {
			innerFrame.origin.x = [self frame].size.width - innerFrame.size.width;
			innerFrame.origin.y = 0.0f;
		}
		[innerView setFrame:innerFrame];

		CGRect messageFrame = [bubbleView frame];
		messageFrame.origin.y = ([innerView frame].size.height - messageFrame.size.height) / 2;
		if (!is_outgoing) { // Inverted
			UIImage *image = [UIImage imageNamed:@"chat_bubble_incoming"];
			image = [image resizableImageWithCapInsets:UIEdgeInsetsMake(26, 32, 34, 56)];
			[backgroundImage setImage:image];
			messageFrame.origin.y += 5;
		} else {
			UIImage *image = [UIImage imageNamed:@"chat_bubble_outgoing"];
			image = [image resizableImageWithCapInsets:UIEdgeInsetsMake(14, 15, 25, 40)];
			[backgroundImage setImage:image];
			messageFrame.origin.y -= 5;
		}
		[bubbleView setFrame:messageFrame];
	}
}

#pragma mark - Action Functions

- (IBAction)onDeleteClick:(id)event {
	if (chat != NULL) {
		if (ftd.message != nil) {
			[ftd cancel];
		}
		UIView *view = [self superview];
		// Find TableViewCell
		while (view != nil && ![view isKindOfClass:[UITableView class]])
			view = [view superview];
		if (view != nil) {
			UITableView *tableView = (UITableView *)view;
			NSIndexPath *indexPath = [tableView indexPathForCell:self];
			[[tableView dataSource] tableView:tableView
						   commitEditingStyle:UITableViewCellEditingStyleDelete
							forRowAtIndexPath:indexPath];
		}
	}
}

- (IBAction)onDownloadClick:(id)event {
	if (ftd.message == nil) {
		ftd = [[FileTransferDelegate alloc] init];
		[self connectToFileDelegate:ftd];
		[ftd download:chat];
		_cancelButton.hidden = NO;
		downloadButton.hidden = YES;
	}
}

- (IBAction)onCancelDownloadClick:(id)sender {
	FileTransferDelegate *tmp = ftd;
	[self disconnectFromFileDelegate];
	[tmp cancel];
	[self update];
}

- (IBAction)onImageClick:(id)event {
	LinphoneChatMessageState state = linphone_chat_message_get_state(self->chat);
	if (state == LinphoneChatMessageStateNotDelivered) {
		[self onResendClick:nil];
	} else {
		if (![messageImageView isLoading]) {
			ImageViewController *controller = DYNAMIC_CAST(
				[[PhoneMainView instance] changeCurrentView:[ImageViewController compositeViewDescription] push:TRUE],
				ImageViewController);
			if (controller != nil) {
				CGImageRef fullScreenRef = [[messageImageView.fullImageUrl defaultRepresentation] fullScreenImage];
				UIImage *fullScreen = [UIImage imageWithCGImage:fullScreenRef];
				[controller setImage:fullScreen];
			}
		}
	}
}

- (IBAction)onResendClick:(id)event {
	if (chat == nil)
		return;

	LinphoneChatMessageState state = linphone_chat_message_get_state(self->chat);
	if (state == LinphoneChatMessageStateNotDelivered) {
		if (linphone_chat_message_get_file_transfer_information(chat) != NULL) {
			NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:chat];
			NSURL *imageUrl = [NSURL URLWithString:localImage];

			[self onDeleteClick:nil];

			[[LinphoneManager instance]
					.photoLibrary assetForURL:imageUrl
				resultBlock:^(ALAsset *asset) {
				  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL),
								 ^(void) {
								   UIImage *image = [[UIImage alloc] initWithCGImage:[asset thumbnail]];
								   [chatRoomDelegate chatRoomStartImageUpload:image url:imageUrl];
								 });
				}
				failureBlock:^(NSError *error) {
				  LOGE(@"Can't read image");
				}];
		} else {
			const char *text = linphone_chat_message_get_text(self->chat);
			NSString *message = text ? [NSString stringWithUTF8String:text] : nil;

			[self onDeleteClick:nil];

			double delayInSeconds = 0.4;
			dispatch_time_t popTime = dispatch_time(DISPATCH_TIME_NOW, (int64_t)(delayInSeconds * NSEC_PER_SEC));
			dispatch_after(popTime, dispatch_get_main_queue(), ^(void) {
			  [chatRoomDelegate resendChat:message withExternalUrl:nil];
			});
		}
	}
}
#pragma mark - State changed handling
static void message_status(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	UIChatRoomCell *thiz = (__bridge UIChatRoomCell *)linphone_chat_message_get_user_data(msg);
	LOGI(@"State for message [%p] changed to %s", msg, linphone_chat_message_state_to_string(state));
	if (linphone_chat_message_get_file_transfer_information(msg) != NULL) {
		if (state == LinphoneChatMessageStateDelivered || state == LinphoneChatMessageStateNotDelivered) {
			// we need to refresh the tableview because the filetransfer delegate unreffed
			// the chat message before state was LinphoneChatMessageStateFileTransferDone -
			// if we are coming back from another view between unreffing and change of state,
			// the transient message will not be found and it will not appear in the list of
			// message, so we must refresh the table when we change to this state to ensure that
			// all transient messages apppear
			//		ChatRoomViewController *controller = DYNAMIC_CAST(
			//			[[PhoneMainView instance] changeCurrentView:[ChatRoomViewController compositeViewDescription]
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
		// the exact same message is sent to the other chat participant and we come
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
