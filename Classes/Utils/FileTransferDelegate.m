//
//  FileTransferDelegate.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 10/06/15.
//
//

#import "FileTransferDelegate.h"
@interface FileTransferDelegate ()
@property(strong) NSMutableData *data;
@end

@implementation FileTransferDelegate

- (void)dealloc {
	if (_message != nil) {
		[self cancel];
	}
}

+ (FileTransferDelegate *)messageDelegate:(LinphoneChatMessage *)message {
	for (FileTransferDelegate *ftd in [[LinphoneManager instance] fileTransferDelegates]) {
		if (ftd.message == message) {
			return ftd;
		}
	}
	return nil;
}

static void linphone_iphone_file_transfer_recv(LinphoneChatMessage *message, const LinphoneContent *content,
											   const LinphoneBuffer *buffer) {
	FileTransferDelegate *thiz = [FileTransferDelegate messageDelegate:message];
	size_t size = linphone_buffer_get_size(buffer);

	if (!thiz.data) {
		thiz.data = [[NSMutableData alloc] initWithCapacity:linphone_content_get_size(content)];
	}

	if (size == 0) {
		LOGI(@"Transfer of %s (%d bytes): download finished", linphone_content_get_name(content), size);
		assert([thiz.data length] == linphone_content_get_size(content));

		// we're finished, save the image and update the message
		UIImage *image = [UIImage imageWithData:thiz.data];

		CFBridgingRetain(thiz);
		[[[LinphoneManager instance] fileTransferDelegates] removeObject:thiz];

		[[LinphoneManager instance]
				.photoLibrary
			writeImageToSavedPhotosAlbum:image.CGImage
							 orientation:(ALAssetOrientation)[image imageOrientation]
						 completionBlock:^(NSURL *assetURL, NSError *error) {
						   if (error) {
							   LOGE(@"Cannot save image data downloaded [%@]", [error localizedDescription]);

							   UIAlertView *errorAlert = [[UIAlertView alloc]
									   initWithTitle:NSLocalizedString(@"Transfer error", nil)
											 message:NSLocalizedString(@"Cannot write image to photo library", nil)
											delegate:nil
								   cancelButtonTitle:NSLocalizedString(@"Ok", nil)
								   otherButtonTitles:nil, nil];
							   [errorAlert show];
						   } else {
							   LOGI(@"Image saved to [%@]", [assetURL absoluteString]);
							   [LinphoneManager setValueInMessageAppData:[assetURL absoluteString]
																  forKey:@"localimage"
															   inMessage:message];
						   }
						   linphone_chat_message_unref(thiz.message);
						   thiz.message = NULL;
						   [[NSNotificationCenter defaultCenter]
							   postNotificationName:kLinphoneFileTransferRecvUpdate
											 object:thiz
										   userInfo:@{
											   @"state" : @(linphone_chat_message_get_state(message)),
											   @"image" : image,
											   @"progress" :
												   @([thiz.data length] * 1.f / linphone_content_get_size(content)),
										   }];

						   CFRelease((__bridge CFTypeRef)thiz);
						 }];
	} else {
		LOGD(@"Transfer of %s (%d bytes): already %ld sent, adding %ld", linphone_content_get_name(content),
			 linphone_content_get_size(content), [thiz.data length], size);
		[thiz.data appendBytes:linphone_buffer_get_string_content(buffer) length:size];
		[[NSNotificationCenter defaultCenter]
			postNotificationName:kLinphoneFileTransferRecvUpdate
						  object:thiz
						userInfo:@{
							@"state" : @(linphone_chat_message_get_state(message)),
							@"progress" : @([thiz.data length] * 1.f / linphone_content_get_size(content)),
						}];
	}
}

static LinphoneBuffer *linphone_iphone_file_transfer_send(LinphoneChatMessage *message, const LinphoneContent *content,
														  size_t offset, size_t size) {
	FileTransferDelegate *thiz = [FileTransferDelegate messageDelegate:message];
	size_t total = thiz.data.length;
	if (thiz.data) {
		size_t remaining = total - offset;

		NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:@{
			@"state" : @(linphone_chat_message_get_state(message)),
			@"progress" : @(offset * 1.f / total),
		}];
		LOGD(@"Transfer of %s (%d bytes): already sent %ld (%f%%), remaining %ld", linphone_content_get_name(content),
			 total, offset, offset * 100.f / total, remaining);
		[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneFileTransferSendUpdate
															object:thiz
														  userInfo:dict];

		LinphoneBuffer *buffer = NULL;
		@try {
			buffer = linphone_buffer_new_from_data([thiz.data subdataWithRange:NSMakeRange(offset, size)].bytes, size);
		} @catch (NSException *exception) {
			LOGE(@"Exception: %@", exception);
		}

		// this is the last time we will be notified, so destroy ourselve
		if (remaining <= size) {
			LOGI(@"Upload ended, unreffing %p", thiz.message);
			linphone_chat_message_unref(thiz.message);
			thiz.message = NULL;
			[thiz stopAndDestroy];
		}
		return buffer;
	} else {
		LOGE(@"Transfer of %s (%d bytes): %d Error - no upload data in progress!", linphone_content_get_name(content),
			 total, offset);
	}

	return NULL;
}

- (void)upload:(UIImage *)image withURL:(NSURL *)url forChatRoom:(LinphoneChatRoom *)chatRoom {
	[LinphoneManager.instance.fileTransferDelegates addObject:self];

	LinphoneContent *content = linphone_core_create_content(linphone_chat_room_get_lc(chatRoom));
	_data = [NSMutableData dataWithData:UIImageJPEGRepresentation(image, 1.0)];
	linphone_content_set_type(content, "image");
	linphone_content_set_subtype(content, "jpeg");
	linphone_content_set_name(content,
							  [[NSString stringWithFormat:@"%li-%f.jpg", (long)image.hash,
														  [NSDate timeIntervalSinceReferenceDate]] UTF8String]);
	linphone_content_set_size(content, _data.length);

	_message = linphone_chat_room_create_file_transfer_message(chatRoom, content);
	linphone_content_unref(content);

	linphone_chat_message_cbs_set_file_transfer_send(linphone_chat_message_get_callbacks(_message),
													 linphone_iphone_file_transfer_send);

	if (url) {
		// internal url is saved in the appdata for display and later save
		[LinphoneManager setValueInMessageAppData:[url absoluteString] forKey:@"localimage" inMessage:_message];
	}

	LOGI(@"%p Uploading content from message %p", self, _message);

	linphone_chat_room_send_chat_message(chatRoom, _message);
}

- (BOOL)download:(LinphoneChatMessage *)message {
	[[[LinphoneManager instance] fileTransferDelegates] addObject:self];

	_message = message;
	// we need to keep a ref on the message to continue downloading even if user quit a chatroom which destroy all chat
	// messages
	linphone_chat_message_ref(_message);
	const char *url = linphone_chat_message_get_external_body_url(_message);
	LOGI(@"%p Downloading content in %p from %s", self, message, url);

	if (url == nil)
		return FALSE;

	linphone_chat_message_cbs_set_file_transfer_recv(linphone_chat_message_get_callbacks(_message),
													 linphone_iphone_file_transfer_recv);

	linphone_chat_message_download_file(_message);

	return TRUE;
}

- (void)stopAndDestroy {
	[[[LinphoneManager instance] fileTransferDelegates] removeObject:self];
	if (_message != NULL) {
		LinphoneChatMessage *msg = _message;
		_message = NULL;
		LOGI(@"%p Cancelling transfer from %p", self, msg);
		linphone_chat_message_cbs_set_file_transfer_send(linphone_chat_message_get_callbacks(msg), NULL);
		linphone_chat_message_cbs_set_file_transfer_recv(linphone_chat_message_get_callbacks(msg), NULL);
		// when we cancel file transfer, this will automatically trigger NotDelivered callback... recalling ourself a
		// second time so we have to unset message BEFORE calling this
		linphone_chat_message_cancel_file_transfer(msg);
		linphone_chat_message_unref(msg);
	}
	_data = nil;
	LOGI(@"%p Destroying", self);
}

- (void)cancel {
	[self stopAndDestroy];
}

@end
