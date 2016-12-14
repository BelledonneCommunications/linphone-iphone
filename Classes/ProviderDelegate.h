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

- (void)reportIncomingCallwithUUID:(NSUUID *)uuid handle:(NSString *)handle video:(BOOL)video;
- (void)config;
@end

#endif /* ProviderDelegate_h */
