//
//  FileTransferDelegate.m
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 10/06/15.
//
//

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
		LOGI(@"Transfer of %s (%d bytes): download finished", linphone_content_get_name(content), size);
		assert([thiz.data length] == linphone_content_get_file_size(content));
        NSString *fileType = [NSString stringWithUTF8String:linphone_content_get_type(content)];
        if ([fileType isEqualToString:@"image"]) {
            // we're finished, save the image and update the message
            UIImage *image = [UIImage imageWithData:thiz.data];
            if (!image) {
                UIAlertController *errView = [UIAlertController
                                              alertControllerWithTitle:NSLocalizedString(@"File download error", nil)
                                              message:NSLocalizedString(@"Error while downloading the file.\n"
                                                                        @"The file is probably encrypted.\n"
                                                                        @"Please retry to download this file after activating LIME.",
                                                                        nil)
                                              preferredStyle:UIAlertControllerStyleAlert];

                UIAlertAction *defaultAction = [UIAlertAction actionWithTitle:@"OK"
                                                                        style:UIAlertActionStyleDefault
                                                                      handler:^(UIAlertAction *action){
                                                                      }];

                [errView addAction:defaultAction];
                [PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
                [thiz stopAndDestroy];
                return;
            }

            CFBridgingRetain(thiz);
            [[LinphoneManager.instance fileTransferDelegates] removeObject:thiz];

            // until image is properly saved, keep a reminder on it so that the
            // chat bubble is aware of the fact that image is being saved to device
            [LinphoneManager setValueInMessageAppData:@"saving..." forKey:@"localimage" inMessage:message];

            __block PHObjectPlaceholder *placeHolder;
            [[PHPhotoLibrary sharedPhotoLibrary] performChanges:^{
                PHAssetCreationRequest *request = [PHAssetCreationRequest creationRequestForAssetFromImage:image];
                placeHolder = [request placeholderForCreatedAsset];
            } completionHandler:^(BOOL success, NSError *error) {
                if (success) {
                    LOGI(@"Image saved to [%@]", [placeHolder localIdentifier]);
                    [LinphoneManager setValueInMessageAppData:[placeHolder localIdentifier]
                                                       forKey:@"localimage"
                                                    inMessage:message];
                } else {
                    LOGE(@"Cannot save image data downloaded [%@]", [error localizedDescription]);
                    [LinphoneManager setValueInMessageAppData:nil forKey:@"localimage" inMessage:message];
                    UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Transfer error", nil)
                                                                                     message:NSLocalizedString(@"Cannot write image to photo library",
                                                                                                               nil)
                                                                              preferredStyle:UIAlertControllerStyleAlert];
                    
                    UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:@"OK"
                                                                            style:UIAlertActionStyleDefault
                                                                          handler:^(UIAlertAction * action) {}];
                    
                    [errView addAction:defaultAction];
                    [PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
                }
                [NSNotificationCenter.defaultCenter
                 postNotificationName:kLinphoneFileTransferRecvUpdate
                 object:thiz
                 userInfo:@{
                            @"state" : @(LinphoneChatMessageStateDelivered),
                            // we dont want to trigger FileTransferDone here
                            @"image" : image,
                            @"progress" : @(1.f),
                            }];
                
                [thiz stopAndDestroy];
                CFRelease((__bridge CFTypeRef)thiz);
            }];
        }  else {
            [[LinphoneManager.instance fileTransferDelegates] removeObject:thiz];
            
            NSString *key = [fileType isEqualToString:@"file"] ? @"localfile" : @"localvideo";
            NSString *name =[NSString stringWithUTF8String:linphone_content_get_name(content)];
        
            [LinphoneManager setValueInMessageAppData:@"saving..." forKey:key inMessage:message];
           
            //write file to path
            dispatch_async(dispatch_get_main_queue(), ^{
                NSString *filePath = [LinphoneManager documentFile:name];
                [[NSFileManager defaultManager] createFileAtPath:filePath
                                                    contents:thiz.data
                                                  attributes:nil];
        
                [LinphoneManager setValueInMessageAppData:name forKey:key inMessage:message];
                     
                [NSNotificationCenter.defaultCenter
                 postNotificationName:kLinphoneFileTransferRecvUpdate
                 object:thiz
                 userInfo:@{
                            @"state" : @(LinphoneChatMessageStateDelivered), // we dont want to trigger
                            @"progress" : @(1.f),    // FileTransferDone here
                            }];
            
                [thiz stopAndDestroy];
            });
        }
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
		}
		return buffer;
	} else {
		LOGE(@"Transfer of %s (%d bytes): %d Error - no upload data in progress!", linphone_content_get_name(content),
			 total, offset);
	}

	return NULL;
}

- (void)uploadData:(NSData *)data  forChatRoom:(LinphoneChatRoom *)chatRoom type:(NSString *)type subtype:(NSString *)subtype name:(NSString *)name key:(NSString *)key keyData:(NSString *)keyData qualityData:(NSNumber *)qualityData {
    [LinphoneManager.instance.fileTransferDelegates addObject:self];
    
    LinphoneContent *content = linphone_core_create_content(linphone_chat_room_get_core(chatRoom));
    _data = [NSMutableData dataWithData:data];
    linphone_content_set_type(content, [type UTF8String]);
    linphone_content_set_subtype(content, [subtype UTF8String]);
    linphone_content_set_name(content, [name UTF8String]);
    linphone_content_set_size(content, _data.length);
    
    _message = linphone_chat_room_create_file_transfer_message(chatRoom, content);
    linphone_content_unref(content);
    
    linphone_chat_message_cbs_set_file_transfer_send(linphone_chat_message_get_callbacks(_message),
                                                     linphone_iphone_file_transfer_send);

    // internal url is saved in the appdata for display and later save
    [LinphoneManager setValueInMessageAppData:keyData forKey:key inMessage:_message];
    [LinphoneManager setValueInMessageAppData:qualityData forKey:@"uploadQuality" inMessage:_message];
    
    LOGI(@"%p Uploading content from message %p", self, _message);
    linphone_chat_room_send_chat_message(chatRoom, _message);
    
    if (linphone_core_lime_enabled(LC) == LinphoneLimeMandatory && !linphone_chat_room_lime_available(chatRoom)) {
        [LinphoneManager.instance alertLIME:chatRoom];
    }
}

- (void)upload:(UIImage *)image withassetId:(NSString *)phAssetId forChatRoom:(LinphoneChatRoom *)chatRoom withQuality:(float)quality {
    NSString *name = [NSString stringWithFormat:@"%li-%f.jpg", (long)image.hash, [NSDate timeIntervalSinceReferenceDate]];
    if (phAssetId)
        [self uploadData:UIImageJPEGRepresentation(image, quality) forChatRoom:chatRoom type:@"image" subtype:@"jpeg" name:name key:@"localimage" keyData:phAssetId qualityData:[NSNumber numberWithFloat:quality]];
    else
        [self uploadData:UIImageJPEGRepresentation(image, quality) forChatRoom:chatRoom type:@"image" subtype:@"jpeg" name:name key:@"localimage" keyData:nil qualityData:nil];
}

- (void)uploadFile:(NSData *)data forChatRoom:(LinphoneChatRoom *)chatRoom withUrl:(NSURL *)url {
    NSString *name = [url lastPathComponent];
    //save file to Documents
    NSString *filePath = [LinphoneManager documentFile:name];
    [[NSFileManager defaultManager] createFileAtPath:filePath
                                            contents:[NSMutableData dataWithData:data]
                                          attributes:nil];
        
    if ([[url pathExtension] isEqualToString:@"MOV"])
        [self uploadData:data forChatRoom:chatRoom type:nil subtype:nil name:name key:@"localvideo" keyData:name qualityData:nil];
    else
        [self uploadData:data forChatRoom:chatRoom type:@"file" subtype:nil name:name key:@"localfile" keyData:name qualityData:nil];
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
