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

static void file_transfer_progress_indication_recv(LinphoneChatMessage *message, LinphoneContent* content, size_t offset, size_t total) {
	FileTransferDelegate *thiz = [FileTransferDelegate messageDelegate:message];

	if (offset == total) {
		NSString *name = [NSString stringWithUTF8String: linphone_content_get_name(content) ? : ""];
		LOGI(@"Transfer of %@ (%d bytes): download finished", name, total);
		NSString *fileType = [NSString stringWithUTF8String:linphone_content_get_type(content)];
		NSString *key = [ChatConversationView getKeyFromFileType:fileType fileName:name];

		dispatch_async(dispatch_get_main_queue(), ^{
			[LinphoneManager setValueInMessageAppData:name
										   forKey:key
										inMessage:message];
			dispatch_async(dispatch_get_main_queue(), ^{
				if ([ConfigManager.instance lpConfigBoolForKeyWithKey:@"auto_write_to_gallery_preference"]) {
					[ChatConversationView writeMediaToGallery:name fileType:fileType];
				}
			});
		});
	} else {
		LOGD(@"Transfer of %s (%d bytes): already %ld recv", linphone_content_get_name(content),
			 total, offset);
		[NSNotificationCenter.defaultCenter
			postNotificationName:kLinphoneFileTransferRecvUpdate
						  object:thiz
						userInfo:@{
							@"state" : @(linphone_chat_message_get_state(message)),
							@"progress" : @(offset * 1.f / total),
						}];
	}
}

static void file_transfer_progress_indication_send(LinphoneChatMessage *message, LinphoneContent* content, size_t offset, size_t total) {
	FileTransferDelegate *thiz = [FileTransferDelegate messageDelegate:message];
	if (total) {
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
		// this is the last time we will be notified, so destroy ourselve
		if (offset == total) {
			LOGI(@"Upload ended");

			thiz.message = NULL;
			[thiz stopAndDestroy];
		}
	} else {
		LOGE(@"Transfer of %s (%d bytes): %d Error - no upload data in progress!", linphone_content_get_name(content),
			 total, offset);
	}
}

- (void)uploadData:(NSData *)data  forChatRoom:(LinphoneChatRoom *)chatRoom type:(NSString *)type subtype:(NSString *)subtype name:(NSString *)name key:(NSString *)key{
	if ([[LinphoneManager.instance fileTransferDelegates] containsObject:self]) {
		LOGW(@"fileTransferDelegates has already added %p", self);
		return;
	}
	[LinphoneManager.instance.fileTransferDelegates addObject:self];
	[ChatConversationView writeFileInCache:data name:name];

	LinphoneContent *content = linphone_core_create_content(linphone_chat_room_get_core(chatRoom));
	linphone_content_set_type(content, [type UTF8String]);
	linphone_content_set_subtype(content, [subtype UTF8String]);
	linphone_content_set_name(content, [name UTF8String]);
	linphone_content_set_file_path(content, [[LinphoneManager cacheDirectory] stringByAppendingPathComponent:name].UTF8String);
	_message = linphone_chat_room_create_file_transfer_message(chatRoom, content);
	BOOL isOneToOneChat = linphone_chat_room_get_capabilities(chatRoom) & LinphoneChatRoomCapabilitiesOneToOne;
	if (!isOneToOneChat && (_text!=nil && ![_text isEqualToString:@""]))
		linphone_chat_message_add_text_content(_message, [_text UTF8String]);
	linphone_content_unref(content);

	linphone_chat_message_cbs_set_file_transfer_progress_indication(linphone_chat_message_get_callbacks(_message), file_transfer_progress_indication_send);

	[LinphoneManager setValueInMessageAppData:name forKey:key inMessage:_message];

	LOGI(@"%p Uploading content from message %p", self, _message);
	linphone_chat_message_send(_message);
}

- (void)uploadFileContent: (FileContext *)context forChatRoom:(LinphoneChatRoom *)chatRoom {
	[LinphoneManager.instance.fileTransferDelegates addObject:self];
	
	_message = linphone_chat_room_create_empty_message(chatRoom);
	NSMutableArray<NSString *> *names = [[NSMutableArray alloc] init];
	NSMutableArray<NSString *> *types = [[NSMutableArray alloc] init];

	int i = 0;
	for (i = 0; i < [context count]; ++i) {
		LinphoneContent *content = linphone_core_create_content(linphone_chat_room_get_core(chatRoom));
		NSString *type = [context.typesArray objectAtIndex:i];
		NSString *name = [context.namesArray objectAtIndex:i];
		NSData *data = [context.datasArray objectAtIndex:i];

		[ChatConversationView writeFileInCache:data name:name];
		
		linphone_content_set_type(content, [type UTF8String]);
		
		linphone_content_set_subtype(content, [name.pathExtension UTF8String]);
		linphone_content_set_name(content, [name UTF8String]);
		linphone_content_set_file_path(content, [[LinphoneManager cacheDirectory] stringByAppendingPathComponent:name].UTF8String);
		[names addObject:name];
		[types addObject:type];
		linphone_chat_message_add_file_content(_message, content);
		linphone_content_unref(content);
	}

	if (_text!=nil && ![_text isEqualToString:@""])
		linphone_chat_message_add_text_content(_message, [_text UTF8String]);

	// todo indication progress
	[LinphoneManager setValueInMessageAppData:names forKey:@"multiparts" inMessage:_message];
	[LinphoneManager setValueInMessageAppData:types forKey:@"multipartstypes" inMessage:_message];
	LOGI(@"%p Uploading content from message %p", self, _message);
	linphone_chat_message_send(_message);
}


- (void)uploadImage:(UIImage *)image forChatRoom:(LinphoneChatRoom *)chatRoom withQuality:(float)quality {
	NSString *name = [NSString stringWithFormat:@"%li-%f.jpg", (long)image.hash, [NSDate timeIntervalSinceReferenceDate]];
	NSData *data = UIImageJPEGRepresentation(image, quality);
	[self uploadData:data forChatRoom:chatRoom type:@"image" subtype:@"jpg" name:name key:@"localimage"];
}

- (void)uploadVideo:(NSData *)data withassetId:(NSString *)phAssetId forChatRoom:(LinphoneChatRoom *)chatRoom  {
	NSString *name = [NSString stringWithFormat:@"IMG-%f.MOV",  [NSDate timeIntervalSinceReferenceDate]];
	[self uploadData:data forChatRoom:chatRoom type:@"video" subtype:@"mov" name:name key:@"localvideo"];
}

- (void)uploadFile:(NSData *)data forChatRoom:(LinphoneChatRoom *)chatRoom withName:(NSString *)name {
	NSURL *url = [ChatConversationView getCacheFileUrl:name];
	AVAsset *asset = [AVURLAsset URLAssetWithURL:url options:nil];
	NSString *fileType = [[asset tracksWithMediaType:AVMediaTypeVideo] count] > 0 ? @"video" : @"file";
	NSString *key = [ChatConversationView getKeyFromFileType:fileType fileName:name];

	[self uploadData:data forChatRoom:chatRoom type:fileType subtype:name.lastPathComponent name:name key:key];
}

- (BOOL)download:(LinphoneChatMessage *)message {
	if ([[LinphoneManager.instance fileTransferDelegates] containsObject:self]) {
		LOGW(@"fileTransferDelegates has already added %p", self);
		return FALSE;
	}
	[[LinphoneManager.instance fileTransferDelegates] addObject:self];

	_message = message;

	LinphoneContent *content = linphone_chat_message_get_file_transfer_information(_message);
	if (content == nil) return FALSE;

	LOGI(@"%p Downloading content in %p ", self, message);

	linphone_chat_message_cbs_set_file_transfer_progress_indication(linphone_chat_message_get_callbacks(_message), file_transfer_progress_indication_recv);
	linphone_content_set_file_path(content, [[LinphoneManager cacheDirectory] stringByAppendingPathComponent:[NSString stringWithUTF8String:linphone_content_get_name(content)]].UTF8String);
	linphone_chat_message_download_content(_message, content);

	return TRUE;
}

- (void)stopAndDestroy {
	[self stopAndDestroyAndRemove:TRUE];
}

- (void)stopAndDestroyAndRemove:(BOOL)remove {
	if (remove)
		[[LinphoneManager.instance fileTransferDelegates] removeObject:self];

	if (_message != NULL) {
		LinphoneChatMessage *msg = _message;
		_message = NULL;
		LOGI(@"%p Cancelling transfer from %p", self, msg);
		linphone_chat_message_cbs_set_file_transfer_progress_indication(linphone_chat_message_get_callbacks(msg), NULL);
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
