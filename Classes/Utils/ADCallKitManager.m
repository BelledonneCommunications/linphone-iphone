//
//  ADCallKitManager.m
//  Copyright © 2016 Appdios Inc. All rights reserved.
//

#import "ADCallKitManager.h"
#import <Intents/Intents.h>

NS_ASSUME_NONNULL_BEGIN

@implementation CXTransaction (ADPrivateAdditions)

+ (CXTransaction *)transactionWithActions:(NSArray<CXAction *> *)actions {
	CXTransaction *transcation = [[CXTransaction alloc] init];
	for (CXAction *action in actions) {
		[transcation addAction:action];
	}
	return transcation;
}

@end

@interface ADCallKitManager () <CXProviderDelegate>
@property(nonatomic, strong) CXProvider *provider;
@property(nonatomic, strong) CXCallController *callController;
@property(nonatomic, copy) ADCallKitActionNotificationBlock actionNotificationBlock;
@end

@implementation ADCallKitManager

static const NSInteger ADDefaultMaximumCallsPerCallGroup = 1;
static const NSInteger ADDefaultMaximumCallGroups = 1;

- (void)providerDidReset:(CXProvider *)provider {
}

+ (ADCallKitManager *)sharedInstance {
	static ADCallKitManager *instance = nil;
	static dispatch_once_t predicate;
	dispatch_once(&predicate, ^{
	  instance = [[super allocWithZone:nil] init];
	});
	return instance;
}

- (void)setupWithAppName:(NSString *)appName
			  supportsVideo:(BOOL)supportsVideo
	actionNotificationBlock:(ADCallKitActionNotificationBlock)actionNotificationBlock {
	CXProviderConfiguration *configuration = [[CXProviderConfiguration alloc] initWithLocalizedName:appName];
	configuration.maximumCallGroups = ADDefaultMaximumCallGroups;
	configuration.maximumCallsPerCallGroup = ADDefaultMaximumCallsPerCallGroup;
	configuration.supportsVideo = supportsVideo;

	self.provider = [[CXProvider alloc] initWithConfiguration:configuration];
	[self.provider setDelegate:self queue:self.completionQueue ? self.completionQueue : dispatch_get_main_queue()];

	if (CXProvider.authorizationStatus == CXAuthorizationStatusNotDetermined) {
		[self.provider requestAuthorization];
	}

	self.callController = [[CXCallController alloc] initWithQueue:dispatch_get_main_queue()];
	self.actionNotificationBlock = actionNotificationBlock;
}

- (void)setCompletionQueue:(dispatch_queue_t)completionQueue {
	_completionQueue = completionQueue;
	if (self.provider) {
		[self.provider setDelegate:self queue:_completionQueue];
	}
}

- (NSUUID *)reportIncomingCallWithContact:(ADContactProtocol *)contact
							   completion:(ADCallKitManagerCompletion)completion {
	NSUUID *callUUID = [NSUUID UUID];

	CXCallUpdate *callUpdate = [[CXCallUpdate alloc] init];
	callUpdate.callerIdentifier = [contact uniqueIdentifier];
	callUpdate.localizedCallerName = [contact displayName];
	[self.provider reportNewIncomingCallWithUUID:callUUID update:callUpdate completion:completion];
	return callUUID;
}

- (NSUUID *)reportOutgoingCallWithContact:(ADContactProtocol *)contact
							   completion:(ADCallKitManagerCompletion)completion {
	NSUUID *callUUID = [NSUUID UUID];

	CXStartCallAction *action = [[CXStartCallAction alloc] initWithCallUUID:callUUID];
	action.contactIdentifier = [contact uniqueIdentifier];
	action.destination = [contact phoneNumber];

	[self.callController requestTransaction:[CXTransaction transactionWithActions:@[ action ]] completion:completion];
	return callUUID;
}

- (void)updateCall:(NSUUID *)callUUID state:(ADCallState)state {
	switch (state) {
		case ADCallStateConnecting:
			[self.provider reportOutgoingCallWithUUID:callUUID startedConnectingAtDate:nil];
			break;
		case ADCallStateConnected:
			[self.provider reportOutgoingCallWithUUID:callUUID connectedAtDate:nil];
			break;
		case ADCallStateEnded:
			[self.provider reportCallWithUUID:callUUID endedAtDate:nil reason:CXCallEndedReasonRemoteEnded];
			break;
		case ADCallStateEndedWithFailure:
			[self.provider reportCallWithUUID:callUUID endedAtDate:nil reason:CXCallEndedReasonFailed];
			break;
		case ADCallStateEndedUnanswered:
			[self.provider reportCallWithUUID:callUUID endedAtDate:nil reason:CXCallEndedReasonUnanswered];
			break;
		default:
			break;
	}
}

- (void)mute:(BOOL)mute callUUID:(NSUUID *)callUUID completion:(ADCallKitManagerCompletion)completion {
	CXSetMutedCallAction *action = [[CXSetMutedCallAction alloc] initWithCallUUID:callUUID];
	action.muted = mute;

	[self.callController requestTransaction:[CXTransaction transactionWithActions:@[ action ]] completion:completion];
}

- (void)hold:(BOOL)hold callUUID:(NSUUID *)callUUID completion:(ADCallKitManagerCompletion)completion {
	CXSetHeldCallAction *action = [[CXSetHeldCallAction alloc] initWithCallUUID:callUUID];
	action.onHold = hold;

	[self.callController requestTransaction:[CXTransaction transactionWithActions:@[ action ]] completion:completion];
}

- (void)endCall:(NSUUID *)callUUID completion:(ADCallKitManagerCompletion)completion {
	CXEndCallAction *action = [[CXEndCallAction alloc] initWithCallUUID:callUUID];

	[self.callController requestTransaction:[CXTransaction transactionWithActions:@[ action ]] completion:completion];
}

#pragma mark - CXProviderDelegate
- (void)provider:(CXProvider *)provider performAnswerCallAction:(nonnull CXAnswerCallAction *)action {
	if (self.actionNotificationBlock) {
		self.actionNotificationBlock(action, ADCallActionTypeAnswer);
	}
	[action fulfill];
}

- (void)provider:(CXProvider *)provider performEndCallAction:(nonnull CXEndCallAction *)action {
	if (self.actionNotificationBlock) {
		self.actionNotificationBlock(action, ADCallActionTypeEnd);
	}
	[action fulfill];
}

- (void)provider:(CXProvider *)provider performStartCallAction:(nonnull CXStartCallAction *)action {
	if (self.actionNotificationBlock) {
		self.actionNotificationBlock(action, ADCallActionTypeStart);
	}
	if (action.destination) {
		[action fulfill];
	} else {
		[action fail];
	}
}

- (void)provider:(CXProvider *)provider performSetMutedCallAction:(nonnull CXSetMutedCallAction *)action {
	if (self.actionNotificationBlock) {
		self.actionNotificationBlock(action, ADCallActionTypeMute);
	}
	[action fulfill];
}

- (void)provider:(CXProvider *)provider performSetHeldCallAction:(nonnull CXSetHeldCallAction *)action {
	if (self.actionNotificationBlock) {
		self.actionNotificationBlock(action, ADCallActionTypeHeld);
	}
	[action fulfill];
}

@end

@implementation ADContactProtocol

- (instancetype)init {
	return [super init];
}

- (instancetype)alloc {
	//	return [super alloc];
	return self;
}

@end
NS_ASSUME_NONNULL_END
