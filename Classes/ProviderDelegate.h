//
//  ProviderDelegate.h
//  linphone
//
//  Created by REIS Benjamin on 20/09/2016.
//
//

#import <CallKit/CallKit.h>

#ifndef ProviderDelegate_h
#define ProviderDelegate_h

@interface ProviderDelegate : NSObject <CXProviderDelegate>

@property CXProvider *provider;
@property NSMutableDictionary* calls;
@property NSMutableDictionary* uuids;
@property BOOL callKit;

- (void)providerDidReset:(CXProvider *)provider;
- (void) reportIncomingCallwithUUID:(NSUUID*)uuid handle:(NSString*) handle;

@end

#endif /* ProviderDelegate_h */
