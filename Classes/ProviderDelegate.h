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

- (void)providerDidReset:(CXProvider *)provider;
@property CXProvider *provider;
- (void) reportIncomingCallwithUUID:(NSUUID*)uuid handle:(NSString*) handle;

@end

#endif /* ProviderDelegate_h */
