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

static void linphone_iphone_file_transfer_recv(LinphoneChatMessage *message, const LinphoneContent *content,
											   const LinphoneBuffer *buffer) {
	FileTransferDelegate *thiz = (__bridge FileTransferDelegate *)linphone_chat_message_get_user_data(message);
	size_t size = linphone_buffer_get_size(buffer);

	if (!thiz.data) {
		thiz.data = [[NSMutableData alloc] initWithCapacity:linphone_content_get_size(content)];
	}

	if (size == 0) {
		LOGI(@"Transfer of %s (%d bytes): download finished", linphone_content_get_name(content), size);
		assert([thiz.data length] == linphone_content_get_size(content));

		// we're finished, save the image and update the message
		UIImage *image = [UIImage imageWithData:thiz.data];


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
	FileTransferDelegate *thiz = (__bridge FileTransferDelegate *)linphone_chat_message_get_user_data(message);
	size_t total = thiz.data.length;
	if (thiz.data) {
		size_t remaining = total - offset;

		NSMutableDictionary *dict = [NSMutableDictionary dictionaryWithDictionary:@{
			@"state" : @(linphone_chat_message_get_state(message)),
			@"progress" : @(offset * 1.f / total),
		}];
		LOGD(@"Transfer of %s (%d bytes): already sent %ld, remaining %ld", linphone_content_get_name(content), total,
			 offset, remaining);
		[[NSNotificationCenter defaultCenter] postNotificationName:kLinphoneFileTransferSendUpdate
															object:thiz
														  userInfo:dict];
		@try {
			return linphone_buffer_new_from_data([thiz.data subdataWithRange:NSMakeRange(offset, size)].bytes, size);
		} @catch (NSException *exception) {
			LOGE(@"Exception: %@", exception);
		}
	} else {
		LOGE(@"Transfer of %s (%d bytes): %d Error - no upload data in progress!", linphone_content_get_name(content),
			 total, offset);
	}

	return NULL;
}

static void message_status(LinphoneChatMessage *msg, LinphoneChatMessageState state) {
	FileTransferDelegate *thiz = (__bridge FileTransferDelegate *)linphone_chat_message_get_user_data(msg);

	NSString *notification =
		linphone_chat_message_is_outgoing(msg) ? kLinphoneFileTransferSendUpdate : kLinphoneFileTransferRecvUpdate;

	const char *text = (linphone_chat_message_get_file_transfer_information(msg) != NULL)
						   ? "photo transfer"
						   : linphone_chat_message_get_text(msg);
	LOGI(@"Delivery status for [%s] is [%s]", text, linphone_chat_message_state_to_string(state));

	NSDictionary *dict = @{ @"state" : @(state), @"progress" : @0.f };
	if (state == LinphoneChatMessageStateFileTransferDone || state == LinphoneChatMessageStateFileTransferError) {
		thiz.message = NULL;
	}
	[[NSNotificationCenter defaultCenter] postNotificationName:notification object:thiz userInfo:dict];
	if (linphone_chat_message_is_outgoing(msg)) {
		[thiz stopAndDestroy];
	}
}

- (void)upload:(UIImage *)image withURL:(NSURL *)url forChatRoom:(LinphoneChatRoom *)chatRoom {
	LinphoneContent *content = linphone_core_create_content(linphone_chat_room_get_lc(chatRoom));
	_data = [NSMutableData dataWithData:UIImageJPEGRepresentation(image, 1.0)];
	linphone_content_set_type(content, "image");
	linphone_content_set_subtype(content, "jpeg");
	linphone_content_set_name(content,
							  [[NSString stringWithFormat:@"%li-%f.jpg", (long)[image hash],
														  [NSDate timeIntervalSinceReferenceDate]] UTF8String]);
	linphone_content_set_size(content, [_data length]);

	CFTypeRef myself = (__bridge CFTypeRef)self;
	_message = linphone_chat_room_create_file_transfer_message(chatRoom, content);
	linphone_chat_message_ref(_message);
	linphone_chat_message_set_user_data(_message, (void *)CFRetain(myself));
	linphone_chat_message_cbs_set_file_transfer_send(linphone_chat_message_get_callbacks(_message),
													 linphone_iphone_file_transfer_send);
	linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(_message), message_status);

	if (url) {
		// internal url is saved in the appdata for display and later save
		[LinphoneManager setValueInMessageAppData:[url absoluteString] forKey:@"localimage" inMessage:_message];
	}

	linphone_chat_room_send_chat_message(chatRoom, _message);
}

- (BOOL)download:(LinphoneChatMessage *)message {
	_message = message;
	// we need to keep a ref on the message to continue downloading even if user quit a chatroom which destroy all chat
	// messages
	linphone_chat_message_ref(_message);
	const char *url = linphone_chat_message_get_external_body_url(_message);
	LOGI(@"Content to download: %s", url);

	if (url == nil)
		return FALSE;

	linphone_chat_message_set_user_data(_message, (void *)CFBridgingRetain(self));

	linphone_chat_message_cbs_set_file_transfer_recv(linphone_chat_message_get_callbacks(_message),
													 linphone_iphone_file_transfer_recv);
	linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(_message), message_status);

	linphone_chat_message_download_file(_message);

	return TRUE;
}

- (void)stopAndDestroy {
	[[[LinphoneManager instance] fileTransferDelegates] removeObject:self];
	if (_message != NULL) {
		linphone_chat_message_set_user_data(_message, NULL);

		linphone_chat_message_cbs_set_file_transfer_progress_indication(linphone_chat_message_get_callbacks(_message),
																		NULL);
		linphone_chat_message_cbs_set_file_transfer_recv(linphone_chat_message_get_callbacks(_message), NULL);
		linphone_chat_message_cbs_set_msg_state_changed(linphone_chat_message_get_callbacks(_message), NULL);
		linphone_chat_message_cancel_file_transfer(_message);
		linphone_chat_message_unref(_message);
	}
	_message = nil;
	_data = nil;
}

- (void)cancel {
	[self stopAndDestroy];
}

@end
