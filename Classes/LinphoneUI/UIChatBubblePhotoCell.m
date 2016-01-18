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
	FileTransferDelegate *_ftd;
}

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
		// TODO: remove text cell subview
		NSArray *arrayOfViews =
			[[NSBundle mainBundle] loadNibNamed:NSStringFromClass(self.class) owner:self options:nil];
		// resize cell to match .nib size. It is needed when resized the cell to
		// correctly adapt its height too
		UIView *sub = nil;
		for (int i = 0; i < arrayOfViews.count; i++) {
			if ([arrayOfViews[i] isKindOfClass:UIView.class]) {
				sub = arrayOfViews[i];
				break;
			}
		}
		[self setFrame:CGRectMake(0, 0, sub.frame.size.width, sub.frame.size.height)];
		[self addSubview:sub];
	}
	return self;
}

#pragma mark -

- (void)setChatMessage:(LinphoneChatMessage *)amessage {
	_imageGestureRecognizer.enabled = NO;
	_messageImageView.image = nil;
	_fileTransferProgress.progress = 0;
	[self disconnectFromFileDelegate];

	if (amessage) {
		const LinphoneContent *c = linphone_chat_message_get_file_transfer_information(amessage);
		if (c) {
			const char *name = linphone_content_get_name(c);
			for (FileTransferDelegate *aftd in [LinphoneManager.instance fileTransferDelegates]) {
				if (linphone_chat_message_get_file_transfer_information(aftd.message) &&
					(linphone_chat_message_is_outgoing(aftd.message) == linphone_chat_message_is_outgoing(amessage)) &&
					strcmp(name, linphone_content_get_name(
									 linphone_chat_message_get_file_transfer_information(aftd.message))) == 0) {
					LOGI(@"Chat message [%p] with file transfer delegate [%p], connecting to it!", amessage, aftd);
					[self connectToFileDelegate:aftd];
					break;
				}
			}
		}
	}

	[super setChatMessage:amessage];
}

- (void)update {
	if (self.message == nil) {
		LOGW(@"Cannot update message room cell: NULL message");
		return;
	}
	[super update];

	const char *url = linphone_chat_message_get_external_body_url(self.message);
	BOOL is_external =
		(url && (strstr(url, "http") == url)) || linphone_chat_message_get_file_transfer_information(self.message);
	NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:self.message];
	BOOL fullScreenImage = NO;
	assert(is_external || localImage);
	if (localImage) {
		// image is being saved on device - just wait for it
		if ([localImage isEqualToString:@"saving..."]) {
			_cancelButton.hidden = _fileTransferProgress.hidden = _downloadButton.hidden = YES;
			fullScreenImage = YES;
		} else {
			// we did not load the image yet, so start doing so
			if (_messageImageView.image == nil) {
				NSURL *imageUrl = [NSURL URLWithString:localImage];
				[_messageImageView startLoading];
				__block LinphoneChatMessage *achat = self.message;
				[LinphoneManager.instance.photoLibrary assetForURL:imageUrl
					resultBlock:^(ALAsset *asset) {
					  dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL),
									 ^(void) {
									   if (achat == self.message) { // Avoid glitch and scrolling
										   UIImage *image = [[UIImage alloc] initWithCGImage:[asset thumbnail]];
										   dispatch_async(dispatch_get_main_queue(), ^{
											 [_messageImageView setImage:image];
											 [_messageImageView setFullImageUrl:asset];
											 [_messageImageView stopLoading];
											 _messageImageView.hidden = NO;
											 _imageGestureRecognizer.enabled = YES;
										   });
									   }
									 });
					}
					failureBlock:^(NSError *error) {
					  LOGE(@"Can't read image");
					}];
			}
			// we are uploading the image
			if (_ftd.message != nil) {
				_cancelButton.hidden = NO;
				_fileTransferProgress.hidden = NO;
				_downloadButton.hidden = YES;
			} else {
				_cancelButton.hidden = _fileTransferProgress.hidden = _downloadButton.hidden = YES;
				fullScreenImage = YES;
			}
		}
		// we must download the image: either it has already started (show cancel button) or not yet (show download
		// button)
	} else {
		_messageImageView.hidden = _cancelButton.hidden = (_ftd.message == nil);
		_downloadButton.hidden = !_cancelButton.hidden;
		_fileTransferProgress.hidden = NO;
	}

	// resize image so that it take the full bubble space available
	CGRect newFrame = _totalView.frame;
	newFrame.origin.x = newFrame.origin.y = 0;
	if (!fullScreenImage) {
		newFrame.size.height -= _imageSubView.frame.size.height;
	}
	_messageImageView.frame = newFrame;
}

- (IBAction)onDownloadClick:(id)event {
	[_ftd cancel];
	_ftd = [[FileTransferDelegate alloc] init];
	[self connectToFileDelegate:_ftd];
	[_ftd download:self.message];
	_cancelButton.hidden = NO;
	_downloadButton.hidden = YES;
}

- (IBAction)onCancelClick:(id)sender {
	FileTransferDelegate *tmp = _ftd;
	[self disconnectFromFileDelegate];
	_fileTransferProgress.progress = 0;
	[tmp cancel];
}

- (void)onResendClick:(id)event {
	if (_downloadButton.hidden == NO) {
		// if download button is displayed, click on it
		[self onDownloadClick:event];
	} else if (_cancelButton.hidden == NO) {
		[self onCancelClick:event];
	} else {
		[super onResendClick:event];
	}
}

- (IBAction)onImageClick:(id)event {
	LinphoneChatMessageState state = linphone_chat_message_get_state(self.message);
	if (state == LinphoneChatMessageStateNotDelivered) {
		[self onResendClick:event];
	} else {
		if (![_messageImageView isLoading]) {
			ImageView *view = VIEW(ImageView);
			[PhoneMainView.instance changeCurrentView:view.compositeViewDescription];
			CGImageRef fullScreenRef = [[_messageImageView.fullImageUrl defaultRepresentation] fullScreenImage];
			UIImage *fullScreen = [UIImage imageWithCGImage:fullScreenRef];
			[view setImage:fullScreen];
		}
	}
}

#pragma mark - LinphoneFileTransfer Notifications Handling

- (void)connectToFileDelegate:(FileTransferDelegate *)aftd {
	if (aftd.message && linphone_chat_message_get_state(aftd.message) == LinphoneChatMessageStateFileTransferError) {
		LOGW(@"This file transfer failed unexpectedly, cleaning it");
		[aftd stopAndDestroy];
		return;
	}

	_ftd = aftd;
	_fileTransferProgress.progress = 0;
	[NSNotificationCenter.defaultCenter removeObserver:self];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(onFileTransferSendUpdate:)
											   name:kLinphoneFileTransferSendUpdate
											 object:_ftd];
	[NSNotificationCenter.defaultCenter addObserver:self
										   selector:@selector(onFileTransferRecvUpdate:)
											   name:kLinphoneFileTransferRecvUpdate
											 object:_ftd];
}

- (void)disconnectFromFileDelegate {
	[NSNotificationCenter.defaultCenter removeObserver:self];
	_ftd = nil;
}

- (void)onFileTransferSendUpdate:(NSNotification *)notif {
	LinphoneChatMessageState state = [[[notif userInfo] objectForKey:@"state"] intValue];

	if (state == LinphoneChatMessageStateInProgress) {
		float progress = [[[notif userInfo] objectForKey:@"progress"] floatValue];
		// When uploading a file, the self.message file is first uploaded to the server,
		// so we are in progress state. Then state goes to filetransfertdone. Then,
		// the exact same self.message is sent to the other participant and we come
		// back to in progress again. This second time is NOT an upload, so we must
		// not update progress!
		_fileTransferProgress.progress = MAX(_fileTransferProgress.progress, progress);
		_fileTransferProgress.hidden = _cancelButton.hidden = (_fileTransferProgress.progress == 1.f);
	} else {
		ChatConversationView *view = VIEW(ChatConversationView);
		[view.tableController updateChatEntry:self.message];
	}
}
- (void)onFileTransferRecvUpdate:(NSNotification *)notif {
	LinphoneChatMessageState state = [[[notif userInfo] objectForKey:@"state"] intValue];
	if (state == LinphoneChatMessageStateInProgress) {
		float progress = [[[notif userInfo] objectForKey:@"progress"] floatValue];
		_fileTransferProgress.progress = MAX(_fileTransferProgress.progress, progress);
		_fileTransferProgress.hidden = _cancelButton.hidden = (_fileTransferProgress.progress == 1.f);
	} else {
		ChatConversationView *view = VIEW(ChatConversationView);
		[view.tableController updateChatEntry:self.message];
	}
}

@end
