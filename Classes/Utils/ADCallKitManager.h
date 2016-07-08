//
//  ADCallKitManager.h
//  Copyright © 2016 Appdios Inc. All rights reserved.
//

#import <CallKit/CallKit.h>
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

typedef NS_ENUM(NSInteger, ADCallActionType) {
	ADCallActionTypeStart,
	ADCallActionTypeEnd,
	ADCallActionTypeAnswer,
	ADCallActionTypeMute,
	ADCallActionTypeHeld
};

typedef NS_ENUM(NSInteger, ADCallState) {
	ADCallStatePending,
	ADCallStateConnecting,
	ADCallStateConnected,
	ADCallStateEnded,
	ADCallStateEndedWithFailure,
	ADCallStateEndedUnanswered
};

typedef void (^ADCallKitManagerCompletion)(NSError *_Nullable error);
typedef void (^ADCallKitActionNotificationBlock)(CXCallAction *action, ADCallActionType actionType);

@interface ADContactProtocol : NSObject

@property(nonatomic) NSString *uniqueIdentifier;
@property(nonatomic) NSString *displayName;
@property(nonatomic) NSString *phoneNumber;

@end

@class ADUser;
@interface ADCallKitManager : NSObject

@property(nonatomic, strong) dispatch_queue_t completionQueue; // Default to mainQueue

+ (ADCallKitManager *)sharedInstance;
- (void)setupWithAppName:(NSString *)appName
			  supportsVideo:(BOOL)supportsVideo
	actionNotificationBlock:(ADCallKitActionNotificationBlock)actionNotificationBlock;

- (NSUUID *)reportIncomingCallWithContact:(ADContactProtocol *)contact
							   completion:(ADCallKitManagerCompletion)completion;
- (NSUUID *)reportOutgoingCallWithContact:(ADContactProtocol *)contact
							   completion:(ADCallKitManagerCompletion)completion;
- (void)updateCall:(NSUUID *)callUUID state:(ADCallState)state;

- (void)mute:(BOOL)mute callUUID:(NSUUID *)callUUID completion:(ADCallKitManagerCompletion)completion;
- (void)hold:(BOOL)hold callUUID:(NSUUID *)callUUID completion:(ADCallKitManagerCompletion)completion;
- (void)endCall:(NSUUID *)callUUID completion:(ADCallKitManagerCompletion)completion;

@end
NS_ASSUME_NONNULL_END
