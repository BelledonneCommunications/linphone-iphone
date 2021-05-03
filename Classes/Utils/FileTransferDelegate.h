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

#import <Foundation/Foundation.h>

#import "LinphoneManager.h"
#import "ChatConversationView.h"

@interface FileTransferDelegate : NSObject

- (void)uploadFileContent: (FileContext *)context forChatRoom:(LinphoneChatRoom *)chatRoom;
- (void)uploadData:(NSData *)data  forChatRoom:(LinphoneChatRoom *)chatRoom type:(NSString *)type subtype:(NSString *)subtype name:(NSString *)name key:(NSString *)key;
- (void)uploadImage:(UIImage *)image forChatRoom:(LinphoneChatRoom *)chatRoom withQuality:(float)quality;
- (void)uploadFile:(NSData *)data forChatRoom:(LinphoneChatRoom *)chatRoom withName:(NSString *)name;
- (void)uploadVideo:(NSData *)data withassetId:(NSString *)phAssetId forChatRoom:(LinphoneChatRoom *)chatRoom;
- (void)cancel;
- (BOOL)download:(LinphoneChatMessage *)message;
- (void)stopAndDestroy;
- (void)stopAndDestroyAndRemove:(BOOL)remove;
+ (FileTransferDelegate *)messageDelegate:(LinphoneChatMessage *)message;

@property() LinphoneChatMessage *message;
@property() NSString *text;
@end
