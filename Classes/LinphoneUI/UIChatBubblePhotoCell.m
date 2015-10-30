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
	FileTransferDelegate *ftd;
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
	if (amessage == self.message) {
		return;
	}

	[self disconnectFromFileDelegate];
	_messageImageView.image = nil;
	_fileTransferProgress.progress = 0;

	[super setChatMessage:amessage];

	if (amessage) {
		const LinphoneContent *c = linphone_chat_message_get_file_transfer_information(amessage);
		if (c) {
			const char *name = linphone_content_get_name(c);
			for (FileTransferDelegate *aftd in [[LinphoneManager instance] fileTransferDelegates]) {
				if (linphone_chat_message_get_file_transfer_information(aftd.message) &&
					strcmp(name, linphone_content_get_name(
									 linphone_chat_message_get_file_transfer_information(amessage))) == 0) {
					LOGI(@"Chat message [%p] with file transfer delegate [%p], connecting to it!", amessage, aftd);
					[self connectToFileDelegate:aftd];
					break;
				}
			}
		}
	}
	[self update];
}

- (void)update {
	if (self.message == nil) {
		LOGW(@"Cannot update message room cell: NULL message");
		return;
	}
	[super update];

	LinphoneChatMessageState state = linphone_chat_message_get_state(self.message);
	if (state == LinphoneChatMessageStateDelivered || state == LinphoneChatMessageStateNotDelivered) {
		if (ftd) {
			[ftd stopAndDestroy];
			ftd = nil;
		}
	}

	const char *url = linphone_chat_message_get_external_body_url(self.message);
	BOOL is_external =
		(url && (strstr(url, "http") == url)) || linphone_chat_message_get_file_transfer_information(self.message);
	NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:self.message];

	assert(is_external || localImage);
	if (localImage) {
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
									   });
								   }
								 });
				}
				failureBlock:^(NSError *error) {
				  LOGE(@"Can't read image");
				}];
		}
		// we are uploading the image
		if (ftd.message != nil) {
			_cancelButton.hidden = NO;
			_fileTransferProgress.hidden = NO;
			_downloadButton.hidden = YES;
		} else {
			_cancelButton.hidden = _fileTransferProgress.hidden = _downloadButton.hidden = YES;
		}
		// we must download the image: either it has already started (show cancel button) or not yet (show download
		// button)
	} else {
		//		CGRect newFrame = _imageSubView.frame;
		//		newFrame.origin.y = _messageImageView.frame.origin.y + ((ftd.message == nil) ? 0 :
		//_messageImageView.frame.size.height);
		//		_imageSubView.frame = newFrame;
		_messageImageView.hidden = _cancelButton.hidden = (ftd.message == nil);
		_downloadButton.hidden = !_cancelButton.hidden;
		_fileTransferProgress.hidden = NO;
	}
}

- (IBAction)onDownloadClick:(id)event {
	[ftd cancel];
	ftd = [[FileTransferDelegate alloc] init];
	[self connectToFileDelegate:ftd];
	[ftd download:self.message];
	_cancelButton.hidden = NO;
	_downloadButton.hidden = YES;
}

- (IBAction)onCancelClick:(id)sender {
	FileTransferDelegate *tmp = ftd;
	[self disconnectFromFileDelegate];
	[tmp cancel];
	_fileTransferProgress.progress = 0;
	[self update];
}

- (void)onResendClick:(id)event {
	[super onResendClick:event];
}

- (IBAction)onImageClick:(id)event {
	LinphoneChatMessageState state = linphone_chat_message_get_state(self.message);
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

#pragma mark - LinphoneFileTransfer Notifications Handling

- (void)connectToFileDelegate:(FileTransferDelegate *)aftd {
	ftd = aftd;
	//	assert(ftd.message == self.message);
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
		// When uploading a file, the self.message file is first uploaded to the server,
		// so we are in progress state. Then state goes to filetransfertdone. Then,
		// the exact same self.message is sent to the other participant and we come
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

- (CGSize)viewSizeWithWidth:(int)width {
	static const CGFloat MARGIN_WIDTH = 60;
	static const CGFloat MARGIN_HEIGHT = 19 + 16 /*this 16 is because textview add some top&bottom padding*/;
	static const CGFloat IMAGE_HEIGHT = 100.0f;
	static const CGFloat IMAGE_WIDTH = 100.0f;

	NSString *localImage = [LinphoneManager getMessageAppDataForKey:@"localimage" inMessage:self.message];

	CGSize messageSize = (localImage != nil) ? CGSizeMake(IMAGE_WIDTH, IMAGE_HEIGHT) : CGSizeMake(50, 50);
	CGSize dateSize = [self computeBoundingBox:self.contactDateLabel.text
										  size:self.contactDateLabel.frame.size
										  font:self.contactDateLabel.font];

	CGSize bubbleSize;
	bubbleSize.width = MAX(messageSize.width, dateSize.width + self.statusImage.frame.size.width + 5) + MARGIN_WIDTH;
	bubbleSize.height = messageSize.height + MARGIN_HEIGHT;

	return bubbleSize;
}

@end
