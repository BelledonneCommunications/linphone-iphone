/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#import "FileTransferDelegate.h"
#import "LinphoneManager.h"
#import "PhoneMainView.h"
#import "Utils.h"

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
	for (FileTransferDelegate *ftd in [LinphoneManager.instance fileTransferDelegates]) {
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
		thiz.data = [[NSMutableData alloc] initWithCapacity:linphone_content_get_file_size(content)];
	}

	if (size == 0) {
		NSString *name = [NSString stringWithUTF8String: linphone_content_get_name(content) ? : ""];
		LOGI(@"Transfer of %@ (%d bytes): download finished", name, size);
		assert([thiz.data length] == linphone_content_get_file_size(content));
		NSString *fileType = [NSString stringWithUTF8String:linphone_content_get_type(content)];
		NSString *key = [ChatConversationView getKeyFromFileType:fileType fileName:name];
		[LinphoneManager setValueInMessageAppData:@"saving..." forKey:key inMessage:message];

		dispatch_async(dispatch_get_main_queue(), ^{
			[ChatConversationView writeFileInCache:thiz.data name:name];
			[LinphoneManager setValueInMessageAppData:name
										   forKey:key
										inMessage:message];
			[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneFileTransferRecvUpdate
														  object:thiz
														userInfo:@{
															@"state" : @(LinphoneChatMessageStateDelivered), // we dont want to trigger
															@"progress" : @(1.f),    // FileTransferDone here
														}];
		});
	} else {
		LOGD(@"Transfer of %s (%d bytes): already %ld sent, adding %ld", linphone_content_get_name(content),
			 linphone_content_get_file_size(content), [thiz.data length], size);
		[thiz.data appendBytes:linphone_buffer_get_string_content(buffer) length:size];
		[NSNotificationCenter.defaultCenter
			postNotificationName:kLinphoneFileTransferRecvUpdate
						  object:thiz
						userInfo:@{
							@"state" : @(linphone_chat_message_get_state(message)),
							@"progress" : @([thiz.data length] * 1.f / linphone_content_get_file_size(content)),
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
		[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneFileTransferSendUpdate
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
			LOGI(@"Upload ended");
			

			linphone_chat_message_cbs_set_file_transfer_send(linphone_chat_message_get_callbacks(thiz.message), NULL);
			thiz.message = NULL;
			[thiz stopAndDestroy];

			//workaround fix : avoid chatconversationtableview scrolling
			[NSNotificationCenter.defaultCenter postNotificationName:kLinphoneFileTransferSendUpdate
															  object:thiz
															userInfo:@{@"state" : @(LinphoneChatMessageStateDelivered),
																	   }];
            
		}
		return buffer;
	} else {
		LOGE(@"Transfer of %s (%d bytes): %d Error - no upload data in progress!", linphone_content_get_name(content),
			 total, offset);
	}

	return NULL;
}

- (void)uploadData:(NSData *)data  forChatRoom:(LinphoneChatRoom *)chatRoom type:(NSString *)type subtype:(NSString *)subtype name:(NSString *)name key:(NSString *)key{
	[LinphoneManager.instance.fileTransferDelegates addObject:self];

	LinphoneContent *content = linphone_core_create_content(linphone_chat_room_get_core(chatRoom));
	_data = [NSMutableData dataWithData:data];
	linphone_content_set_type(content, [type UTF8String]);
	linphone_content_set_subtype(content, [subtype UTF8String]);
	linphone_content_set_name(content, [name UTF8String]);
	linphone_content_set_size(content, _data.length);
	_message = linphone_chat_room_create_file_transfer_message(chatRoom, content);
	BOOL isOneToOneChat = linphone_chat_room_get_capabilities(chatRoom) & LinphoneChatRoomCapabilitiesOneToOne;
	if (!isOneToOneChat && (_text!=nil && ![_text isEqualToString:@""]))
		linphone_chat_message_add_text_content(_message, [_text UTF8String]);
	linphone_content_unref(content);

	linphone_chat_message_cbs_set_file_transfer_send(linphone_chat_message_get_callbacks(_message), linphone_iphone_file_transfer_send);

	[LinphoneManager setValueInMessageAppData:name forKey:key inMessage:_message];

	LOGI(@"%p Uploading content from message %p", self, _message);
	linphone_chat_message_send(_message);
}

- (void)uploadImage:(UIImage *)image forChatRoom:(LinphoneChatRoom *)chatRoom withQuality:(float)quality {
	NSString *name = [NSString stringWithFormat:@"%li-%f.jpg", (long)image.hash, [NSDate timeIntervalSinceReferenceDate]];
	NSData *data = UIImageJPEGRepresentation(image, quality);
	[ChatConversationView writeFileInCache:data name:name];
	[self uploadData:data forChatRoom:chatRoom type:@"image" subtype:@"jpg" name:name key:@"localimage"];
}

- (void)uploadVideo:(NSData *)data withassetId:(NSString *)phAssetId forChatRoom:(LinphoneChatRoom *)chatRoom  {
	NSString *name = [NSString stringWithFormat:@"IMG-%f.MOV",  [NSDate timeIntervalSinceReferenceDate]];
	[ChatConversationView writeFileInCache:data name:name];
	[self uploadData:data forChatRoom:chatRoom type:@"video" subtype:@"mov" name:name key:@"localvideo"];
}

- (void)uploadFile:(NSData *)data forChatRoom:(LinphoneChatRoom *)chatRoom withName:(NSString *)name {
	[ChatConversationView writeFileInCache:data name:name];
	NSURL *url = [ChatConversationView getCacheFileUrl:name];
	AVAsset *asset = [AVURLAsset URLAssetWithURL:url options:nil];
	NSString *fileType = [[asset tracksWithMediaType:AVMediaTypeVideo] count] > 0 ? @"video" : @"file";
	NSString *key = [ChatConversationView getKeyFromFileType:fileType fileName:name];

	[self uploadData:data forChatRoom:chatRoom type:fileType subtype:name.lastPathComponent name:name key:key];
}

- (BOOL)download:(LinphoneChatMessage *)message {
	[[LinphoneManager.instance fileTransferDelegates] addObject:self];

	_message = message;

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
	[[LinphoneManager.instance fileTransferDelegates] removeObject:self];
	if (_message != NULL) {
		LinphoneChatMessage *msg = _message;
		_message = NULL;
		LOGI(@"%p Cancelling transfer from %p", self, msg);
		linphone_chat_message_cbs_set_file_transfer_send(linphone_chat_message_get_callbacks(msg), NULL);
		linphone_chat_message_cbs_set_file_transfer_recv(linphone_chat_message_get_callbacks(msg), NULL);
		// when we cancel file transfer, this will automatically trigger NotDelivered callback... recalling ourself a
		// second time so we have to unset message BEFORE calling this
		linphone_chat_message_cancel_file_transfer(msg);
	}
	_data = nil;
	LOGD(@"%p Destroying", self);
}

- (void)cancel {
	[self stopAndDestroy];
}

@end
