#import <Foundation/Foundation.h>

#import "XMLRPCEncoder.h"

@interface XMLRPCRequest : NSObject {
    NSMutableURLRequest *myRequest;
    id<XMLRPCEncoder> myXMLEncoder;
}

- (id)initWithURL: (NSURL *)URL;

#pragma mark -

- (void)setURL: (NSURL *)URL;

- (NSURL *)URL;

#pragma mark -

- (void)setUserAgent: (NSString *)userAgent;

- (NSString *)userAgent;

#pragma mark -
- (void)setEncoder: (id<XMLRPCEncoder>) encoder;

- (void)setMethod: (NSString *)method;

- (void)setMethod: (NSString *)method withParameter: (id)parameter;

- (void)setMethod: (NSString *)method withParameters: (NSArray *)parameters;

#pragma mark -

- (NSString *)method;

- (NSArray *)parameters;

#pragma mark -

- (NSString *)body;

#pragma mark -

- (NSURLRequest *)request;

#pragma mark -

- (void)setValue: (NSString *)value forHTTPHeaderField: (NSString *)header;

@end
