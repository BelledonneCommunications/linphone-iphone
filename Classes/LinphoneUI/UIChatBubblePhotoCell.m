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
#import <AVFoundation/AVFoundation.h>
#import <AudioToolbox/AudioToolbox.h>
#import <AVKit/AVKit.h>

@implementation UIChatBubblePhotoCell {
	FileTransferDelegate *_ftd;
    CGSize imageSize, bubbleSize, videoDefaultSize;
    ChatConversationTableView *chatTableView;
    BOOL assetIsLoaded;
}

#pragma mark - Lifecycle Functions

- (id)initWithIdentifier:(NSString *)identifier {
	if ((self = [super initWithStyle:UITableViewCellStyleDefault reuseIdentifier:identifier]) != nil) {
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
		[self addSubview:sub];
        chatTableView = VIEW(ChatConversationView).tableController;
        videoDefaultSize = CGSizeMake(320, 240);
        assetIsLoaded = FALSE;
	}
	return self;
}

- (void)onDelete {
    [super onDelete];
}

#pragma mark -
- (void)setEvent:(LinphoneEventLog *)event {
	if (!event || !(linphone_event_log_get_type(event) == LinphoneEventLogTypeConferenceChatMessage))
		return;

	super.event = event;
	[self setChatMessage:linphone_event_log_get_chat_message(event)];
}

- (void)setChatMessage:(LinphoneChatMessage *)amessage {
	_imageGestureRecognizer.enabled = NO;
	_messageImageView.image = nil;
    _finalImage.image = nil;
    _finalImage.hidden = TRUE;
	_fileTransferProgress.progress = 0;
    assetIsLoaded = FALSE;
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

- (void) loadImageAsset:(PHAsset*) asset  image:(UIImage *)image {
    dispatch_async(dispatch_get_main_queue(), ^{
        [_finalImage setImage:image];
        [_messageImageView setAsset:asset];
        [_messageImageView stopLoading];
        _messageImageView.hidden = YES;
        _finalImage.hidden = NO;
        _fileView.hidden = YES;
        [self layoutSubviews];
    });
}

static const CGFloat CELL_IMAGE_X_MARGIN = 100;

- (void) loadAsset:(PHAsset *) asset {
    PHImageRequestOptions *options = [[PHImageRequestOptions alloc] init];
    options.synchronous = TRUE;
    [[PHImageManager defaultManager] requestImageForAsset:asset targetSize:PHImageManagerMaximumSize contentMode:PHImageContentModeDefault options:options
                                            resultHandler:^(UIImage *image, NSDictionary * info) {
                                                if (image) {
                                                    imageSize = [UIChatBubbleTextCell getMediaMessageSizefromOriginalSize:[image size] withWidth:chatTableView.tableView.frame.size.width - CELL_IMAGE_X_MARGIN];
                                                    [chatTableView.imagesInChatroom setObject:image forKey:[asset localIdentifier]];
                                                    [self loadImageAsset:asset image:image];
                                                }
                                                else {
                                                    LOGE(@"Can't read image");
                                                }
                                            }];
}

- (void) loadFileAsset {
    dispatch_async(dispatch_get_main_queue(), ^{
        _fileName.hidden = _fileView.hidden = _fileButton.hidden = NO;
        _imageGestureRecognizer.enabled = NO;
    });
}

- (void) loadPlaceholder {
    dispatch_async(dispatch_get_main_queue(), ^{
        // Change this to load placeholder image when no asset id
        //[_finalImage setImage:image];
        //[_messageImageView setAsset:asset];
        [_messageImageView stopLoading];
        _messageImageView.hidden = YES;
        _imageGestureRecognizer.enabled = YES;
        _finalImage.hidden = NO;
        [self layoutSubviews];
    });
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
    NSString *localVideo = [LinphoneManager getMessageAppDataForKey:@"localvideo" inMessage:self.message];
    NSString *localFile = [LinphoneManager getMessageAppDataForKey:@"localfile" inMessage:self.message];
	assert(is_external || localImage || localVideo || localFile);
    
    LinphoneContent *fileContent = linphone_chat_message_get_file_transfer_information(self.message);
    NSString *type = fileContent ? [NSString stringWithUTF8String:linphone_content_get_type(fileContent)] : nil;
    
    if (!(localImage || localVideo || localFile)) {
        _playButton.hidden = YES;
        _fileName.hidden = _fileView.hidden = _fileButton.hidden = YES;
        _messageImageView.hidden = _cancelButton.hidden = (_ftd.message == nil);
        _downloadButton.hidden = !_cancelButton.hidden;
        _fileTransferProgress.hidden = NO;
    } else {
        // file is being saved on device - just wait for it
        if ([localImage isEqualToString:@"saving..."] || [localVideo isEqualToString:@"saving..."] || [localFile isEqualToString:@"saving..."]) {
            _cancelButton.hidden = _fileTransferProgress.hidden = _downloadButton.hidden = _playButton.hidden = _fileName.hidden = _fileView.hidden = _fileButton.hidden = YES;
		} else {
			if(!assetIsLoaded) {
				assetIsLoaded = TRUE;
				if (localImage) {
					// we did not load the image yet, so start doing so
					if (_messageImageView.image == nil) {
						[self loadFirstImage:localImage type:PHAssetMediaTypeImage];
						_imageGestureRecognizer.enabled = YES;
					}
				}
				else if (localVideo) {
					if (_messageImageView.image == nil) {
						[self loadFirstImage:localVideo type:PHAssetMediaTypeVideo];
						_imageGestureRecognizer.enabled = NO;
					}
				}
				else if (localFile) {
					if ([type isEqualToString:@"video"]) {
						UIImage* image = [UIChatBubbleTextCell getImageFromVideoUrl:[VIEW(ChatConversationView) getICloudFileUrl:localFile]];
						[self loadImageAsset:nil image:image];
						_imageGestureRecognizer.enabled = NO;
					} else if ([localFile hasSuffix:@"JPG"] || [localFile hasSuffix:@"PNG"] || [localFile hasSuffix:@"jpg"] || [localFile hasSuffix:@"png"]) {
						NSData *data = [NSData dataWithContentsOfURL:[VIEW(ChatConversationView) getICloudFileUrl:localFile]];
						UIImage *image = [[UIImage alloc] initWithData:data];
						[self loadImageAsset:nil image:image];
						_imageGestureRecognizer.enabled = YES;
					} else {
						NSString *text = [NSString stringWithFormat:@"📎 %@",localFile];
						_fileName.text = text;
						[self loadFileAsset];
					}
				}
			}
		}
		// we are uploading the image
		if (_ftd.message != nil) {
			_cancelButton.hidden = NO;
			_fileTransferProgress.hidden = NO;
			_downloadButton.hidden = YES;
			_playButton.hidden = YES;
			_fileName.hidden = _fileView.hidden = _fileButton.hidden =YES;
		} else {
			_cancelButton.hidden = _fileTransferProgress.hidden = _downloadButton.hidden =  YES;
			_playButton.hidden = ![type isEqualToString:@"video"];
			_fileName.hidden = _fileView.hidden = _fileButton.hidden = localFile ? NO : YES;
			// Should fix cell not resizing after doanloading image.
			//[self layoutSubviews];
		}
    }
}

- (void)loadFirstImage:(NSString *)key type:(PHAssetMediaType)type {
    [_messageImageView startLoading];
    PHFetchResult<PHAsset *> *assets = [LinphoneManager getPHAssets:key];
    UIImage *img = nil;
    
    img = [chatTableView.imagesInChatroom objectForKey:key];
    PHAsset *asset = [assets firstObject];
    if (!asset)
        [self loadPlaceholder];
    else if (asset.mediaType != type)
        img = nil;
    if (img)
        [self loadImageAsset:asset image:img];
    else
        [self loadAsset:asset];
}

- (void)fileErrorBlock {
    DTActionSheet *sheet = [[DTActionSheet alloc] initWithTitle:NSLocalizedString(@"Can't find this file", nil)];
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), ^{
        [sheet addCancelButtonWithTitle:NSLocalizedString(@"OK", nil) block:nil];
        dispatch_async(dispatch_get_main_queue(), ^{
            [sheet showInView:PhoneMainView.instance.view];
        });
    });
}

- (void)playVideoByPlayer:(AVPlayer *)player {
    AVPlayerViewController *controller = [[AVPlayerViewController alloc] init];
    [PhoneMainView.instance presentViewController:controller animated:YES completion:nil];
    controller.player = player;
    [player play];
}

- (IBAction)onDownloadClick:(id)event {
	[_ftd cancel];
	_ftd = [[FileTransferDelegate alloc] init];
	[self connectToFileDelegate:_ftd];
	[_ftd download:self.message];
	_cancelButton.hidden = NO;
	_downloadButton.hidden = YES;
    _playButton.hidden = YES;
    _fileName.hidden = _fileView.hidden = _fileButton.hidden = YES;
}

- (IBAction)onPlayClick:(id)sender {
    PHAsset *asset = [_messageImageView asset];
    if (!asset) {
        NSString *localFile = [LinphoneManager getMessageAppDataForKey:@"localfile" inMessage:self.message];
        AVPlayer *player = [AVPlayer playerWithURL:[VIEW(ChatConversationView) getICloudFileUrl:localFile]];
        [self playVideoByPlayer:player];
        return;
    }
    PHVideoRequestOptions *options = [[PHVideoRequestOptions alloc] init];
   // options.synchronous = TRUE;
    [[PHImageManager defaultManager] requestPlayerItemForVideo:asset options:options resultHandler:^(AVPlayerItem * _Nullable playerItem, NSDictionary * _Nullable info) {
        if(playerItem) {
            AVPlayer *player = [AVPlayer playerWithPlayerItem:playerItem];
            [self playVideoByPlayer:player];
        } else {
            [self fileErrorBlock];
        }
    }];
}

- (IBAction)onFileClick:(id)sender {
    ChatConversationView *view = VIEW(ChatConversationView);
    NSString *name = [LinphoneManager getMessageAppDataForKey:@"localfile" inMessage:self.message];
    [view openFileWithURL:[view getICloudFileUrl:name]];
}


- (IBAction)onCancelClick:(id)sender {
	FileTransferDelegate *tmp = _ftd;
	[self disconnectFromFileDelegate];
	_fileTransferProgress.progress = 0;
	[tmp cancel];
	if (!linphone_core_is_network_reachable(LC)) {
		[self update];
	}
}

- (void)onResendClick:(id)event {
	if (_downloadButton.hidden == NO) {
		// if download button is displayed, click on it
		[self onDownloadClick:event];
	} else if (_cancelButton.hidden == NO) {
		[self onCancelClick:event];
    } else {
		[super onResend];
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
            PHAsset *asset = [_messageImageView asset];
            if (!asset) {
                 NSString *localFile = [LinphoneManager getMessageAppDataForKey:@"localfile" inMessage:self.message];
                if ([localFile hasSuffix:@"JPG"] || [localFile hasSuffix:@"PNG"] || [localFile hasSuffix:@"jpg"] || [localFile hasSuffix:@"png"]) {
                    NSData *data = [NSData dataWithContentsOfURL:[VIEW(ChatConversationView) getICloudFileUrl:localFile]];
                    UIImage *image = [[UIImage alloc] initWithData:data];
                    if (image)
                        [view setImage:image];
                    else
                        LOGE(@"Can't read image");
                }
                return;
            }
            PHImageRequestOptions *options = [[PHImageRequestOptions alloc] init];
            options.synchronous = TRUE;
            [[PHImageManager defaultManager] requestImageForAsset:asset targetSize:PHImageManagerMaximumSize contentMode:PHImageContentModeDefault options:options
                                                    resultHandler:^(UIImage *image, NSDictionary * info) {
                                                        if (image) {
                                                            [view setImage:image];
                                                        }
                                                        else {
                                                            LOGE(@"Can't read image");
                                                        }
                                                    }];
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
	[NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneFileTransferSendUpdate object:_ftd];
    [NSNotificationCenter.defaultCenter removeObserver:self name:kLinphoneFileTransferRecvUpdate object:_ftd];
	_ftd = nil;
}

- (void)onFileTransferSendUpdate:(NSNotification *)notif {
	LinphoneChatMessageState state = [[[notif userInfo] objectForKey:@"state"] intValue];

	if (state == LinphoneChatMessageStateInProgress || state == LinphoneChatMessageStateFileTransferInProgress) {
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
		[view.tableController updateEventEntry:self.event];
	}
}
- (void)onFileTransferRecvUpdate:(NSNotification *)notif {
	LinphoneChatMessageState state = [[[notif userInfo] objectForKey:@"state"] intValue];
	if (state == LinphoneChatMessageStateInProgress || state == LinphoneChatMessageStateFileTransferInProgress) {
		float progress = [[[notif userInfo] objectForKey:@"progress"] floatValue];
		_fileTransferProgress.progress = MAX(_fileTransferProgress.progress, progress);
		_fileTransferProgress.hidden = _cancelButton.hidden = (_fileTransferProgress.progress == 1.f);
	} else {
		ChatConversationView *view = VIEW(ChatConversationView);
		[view.tableController updateEventEntry:self.event];
	}
}

- (void)layoutSubviews {
    BOOL is_outgoing = linphone_chat_message_is_outgoing(super.message);
    CGRect bubbleFrame = super.bubbleView.frame;
    int origin_x;
    
    bubbleSize = [UIChatBubbleTextCell ViewSizeForMessage:[self message] withWidth:chatTableView.tableView.frame.size.width];
    
    bubbleFrame.size = bubbleSize;
    
    if (chatTableView.tableView.isEditing) {
        origin_x = 0;
    } else {
        origin_x = (is_outgoing ? self.frame.size.width - bubbleFrame.size.width : 0);
    }
    
    bubbleFrame.origin.x = origin_x;

    super.bubbleView.frame = bubbleFrame;
    
    // Resizing Image view
    if (_finalImage.image) {
        CGRect imgFrame = self.finalAssetView.frame;            
        imgFrame.size = [UIChatBubbleTextCell getMediaMessageSizefromOriginalSize:[_finalImage.image size] withWidth:chatTableView.tableView.frame.size.width - CELL_IMAGE_X_MARGIN];
        imgFrame.origin.x = (self.innerView.frame.size.width - imgFrame.size.width-17)/2;
        self.finalAssetView.frame = imgFrame;
    }

    // Positioning text message
    const char *utf8Text = linphone_chat_message_get_text_content(self.message);
    
    CGRect textFrame = self.messageText.frame;
    if (_finalImage.image)
        textFrame.origin = CGPointMake(textFrame.origin.x, self.finalAssetView.frame.origin.y + self.finalAssetView.frame.size.height);
    else
        // When image hasn't be download
        textFrame.origin = CGPointMake(textFrame.origin.x, _imageSubView.frame.size.height + _imageSubView.frame.origin.y - 10);
    if (!utf8Text) {
        textFrame.size.height = 0;
    } else {
        textFrame.size.height = bubbleFrame.size.height - 90;//textFrame.origin.x;
    }
    
    self.messageText.frame = textFrame;
}

@end


