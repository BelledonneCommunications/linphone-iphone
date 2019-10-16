/*
 * Copyright (c) 2010-2019 Belledonne Communications SARL.
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
