/* ChatModel.h
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
 *  GNU General Public License for more details.                
 *                                                                      
 *  You should have received a copy of the GNU General Public License   
 *  along with this program; if not, write to the Free Software         
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */ 

#import <Foundation/Foundation.h>


@interface ChatModel : NSObject {
    @private
    NSNumber *chatId;
    NSString *localContact;
    NSString *remoteContact;
    NSNumber *direction; //0 outgoing 1 incoming
    NSString *message;
    NSDate *time;
    NSNumber *read;
	NSNumber *state; //0 IDLE, 1 in progress, 2 delivered, 3 not delivered see LinphoneChatMessageState
}

@property (readonly) NSNumber *chatId;
@property (copy) NSString *localContact;
@property (copy) NSString *remoteContact;
@property (copy) NSNumber *direction;
@property (copy) NSString *message;
@property (copy) NSDate *time;
@property (copy) NSNumber *read;
@property (copy) NSNumber *state;

- (BOOL)isExternalImage;
- (BOOL)isInternalImage;

- (void)create;
+ (ChatModel*)read:(NSNumber*)id;
- (void)update;
- (void)delete;

+ (NSMutableArray *)listConversations;
+ (NSMutableArray *)listMessages:(NSString *)contact;
+ (void)removeConversation:(NSString *)contact;
+ (int)unreadMessages;
+ (int)unreadMessages:(NSString *)contact;
+ (void)readConversation:(NSString *)contact;

@end
