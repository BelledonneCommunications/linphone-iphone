//
//  FileTransferDelegate.h
//  linphone
//
//  Created by Gautier Pelloux-Prayer on 10/06/15.
//
//

#import <Foundation/Foundation.h>

#import "LinphoneManager.h"

@interface FileTransferDelegate : NSObject

- (void)upload:(UIImage *)image withassetId:(NSString *)phAssetId forChatRoom:(LinphoneChatRoom *)chatRoom withQuality:(float)quality;
- (void)uploadFile:(NSData *)data forChatRoom:(LinphoneChatRoom *)chatRoom withName:(NSString *)name;
- (void)uploadVideo:(NSData *)data withassetId:(NSString *)phAssetId forChatRoom:(LinphoneChatRoom *)chatRoom;
- (void)cancel;
- (BOOL)download:(LinphoneChatMessage *)message;
- (void)stopAndDestroy;

@property() LinphoneChatMessage *message;
@property() NSString *text;
@end
