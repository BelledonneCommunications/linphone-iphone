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
- (void)uploadFile:(NSData *)data forChatRoom:(LinphoneChatRoom *)chatRoom withUrl:(NSURL *)url;
- (void)cancel;
- (BOOL)download:(LinphoneChatMessage *)message;
- (void)stopAndDestroy;

@property() LinphoneChatMessage *message;
@end
