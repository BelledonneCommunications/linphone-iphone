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

#import <UIKit/UIKit.h>
#include "linphone/linphonecore.h"

#import "linphone/api/c-event-log.h"
#import "linphone/api/c-chat-room.h"

#import "UICheckBoxTableView.h"

@interface FileContext : NSObject
@property NSMutableArray <NSString *> *typesArray;
@property NSMutableArray <NSData *> *datasArray;
@property NSMutableArray <UIImage *> *previewsArray;
@property NSMutableArray <NSString *> *namesArray;
@property NSMutableArray <NSUUID *> *uuidsArray;

- (void)clear;
- (NSUInteger)count;
- (void)addObject:(NSData *)data name:(NSString *)name type:(NSString *)type;
@end

@protocol ChatConversationDelegate <NSObject>

- (BOOL)resendMultiFiles:(FileContext *)newFileContext message:(NSString *)message;
- (BOOL)resendFile:(NSData *)data withName:(NSString *)name type:(NSString *)type key:(NSString *)key message:(NSString *)message;
- (BOOL)startFileUpload:(NSData *)data withName:(NSString *)name;
- (void)resendChat:(NSString *)message withExternalUrl:(NSString *)url;
- (void)tableViewIsScrolling;

@end

@interface ChatConversationTableView : UICheckBoxTableView {
  @private
	NSMutableArray *eventList;
    NSMutableArray *totalEventList;
}

@property(nonatomic) LinphoneChatRoom *chatRoom;
@property(nonatomic) NSInteger currentIndex;
@property(nonatomic, strong) id<ChatConversationDelegate> chatRoomDelegate;
@property NSMutableDictionary<NSString *, UIImage *> *imagesInChatroom;
@property(nonatomic) BOOL vfsEnabled;

- (void)addEventEntry:(LinphoneEventLog *)event;
- (void)scrollToBottom:(BOOL)animated;
- (void)scrollToLastUnread:(BOOL)animated;
- (void)updateEventEntry:(LinphoneEventLog *)event;
- (void)refreshData;

@end
