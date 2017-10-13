//
//  ProviderDelegate.h
//  linphone
//
//  Created by REIS Benjamin on 29/11/2016.
//
//

#import <CallKit/CallKit.h>

#ifndef ProviderDelegate_h
#define ProviderDelegate_h

@interface ProviderDelegate : NSObject <CXProviderDelegate, CXCallObserverDelegate>

@property CXProvider *provider;
@property CXCallObserver *observer;
@property CXCallController *controller;
@property NSMutableDictionary *calls;
@property NSMutableDictionary *uuids;
@property(nonatomic) LinphoneCall *pendingCall;
@property LinphoneAddress *pendingAddr;
@property BOOL pendingCallVideo;
@property int callKitCalls;

- (void)reportIncomingCall:(LinphoneCall *) call withUUID:(NSUUID *)uuid handle:(NSString *)handle video:(BOOL)video;
- (void)config;
- (void)configAudioSession:(AVAudioSession *)audioSession;
@end

#endif /* ProviderDelegate_h */
