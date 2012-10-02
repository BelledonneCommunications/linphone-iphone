/* NSURLConnection+SynchronousDelegate.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#import "NSURLConnection+SynchronousDelegate.h"

@interface NSURLConnectionSynchronousDelegate : NSObject<NSURLConnectionDelegate, NSURLConnectionDataDelegate> {
    @private
    id _delegate;
    dispatch_group_t _group;
}

@property (readonly) NSMutableData *data;
@property (readonly) NSError *error;
@property (readonly) NSURLResponse *response;

- (id)initWithDelegate:(id)delagate group:(dispatch_group_t)group;

@end

@implementation NSURLConnectionSynchronousDelegate

@synthesize data;
@synthesize error;
@synthesize response;

- (id)initWithDelegate:(id)delagate group:(dispatch_group_t)group{
    self = [super init];
    if(self) {
        self->_delegate = [delagate retain];
        self->_group = group;
        self->data = nil;
        self->error = nil;
        self->response = nil;
    }
    return self;
}

- (void)dealloc {
    [_delegate release];
    [data release];
    [error release];
    [response release];
    [super dealloc];
}

- (void)connection:(NSURLConnection *)connection didFailWithError:(NSError *)aerror {
    error = [aerror retain];
    if([_delegate respondsToSelector:@selector(connection:didFailWithError:)]) {
        [_delegate connection:connection didFailWithError:aerror];
    }
    dispatch_group_leave(_group);
    CFRunLoopStop(CFRunLoopGetCurrent());
}

- (BOOL)connectionShouldUseCredentialStorage:(NSURLConnection *)connection {
    if([_delegate respondsToSelector:@selector(connectionShouldUseCredentialStorage:)]) {
        return [_delegate connectionShouldUseCredentialStorage:connection];
    }
    
    return YES;
}

- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
    if([_delegate respondsToSelector:@selector(connection:canAuthenticateAgainstProtectionSpace:)]) {
        return [_delegate connection:connection canAuthenticateAgainstProtectionSpace:protectionSpace];
    }
    return NO;
}

- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
    if([_delegate respondsToSelector:@selector(connection:didReceiveAuthenticationChallenge:)]) {
        [_delegate connection:connection didReceiveAuthenticationChallenge:challenge];
    } else {
        [challenge.sender continueWithoutCredentialForAuthenticationChallenge:challenge];
    }
}

- (void)connection:(NSURLConnection *)connection didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
    if([_delegate respondsToSelector:@selector(connection:didCancelAuthenticationChallenge:)]) {
        [_delegate connection:connection didCancelAuthenticationChallenge:challenge];
    }
}

- (NSURLRequest *)connection:(NSURLConnection *)connection willSendRequest:(NSURLRequest *)request redirectResponse:(NSURLResponse *)aresponse {
    if([_delegate respondsToSelector:@selector(connection:willSendRequest:redirectResponse:)]) {
        return [_delegate connection:connection willSendRequest:request redirectResponse:aresponse];
    }
    return request;
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)aresponse {
    response = [aresponse retain];
    if([_delegate respondsToSelector:@selector(connection:didReceiveResponse:)]) {
        [_delegate connection:connection didReceiveResponse:aresponse];
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)adata {
    if([_delegate respondsToSelector:@selector(connection:didReceiveData:)]) {
        [_delegate connection:connection didReceiveData:adata];
    }
    if(data == nil) {
        data = [[NSMutableData alloc] initWithCapacity:[adata length]];
    }
    [data appendData:adata];
}

- (void)connection:(NSURLConnection *)connection   didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite {
    if([_delegate respondsToSelector:@selector(connection:didSendBodyData:totalBytesWritten:totalBytesExpectedToWrite:)]) {
        return [_delegate connection:connection didSendBodyData:bytesWritten totalBytesWritten:totalBytesWritten totalBytesExpectedToWrite:totalBytesExpectedToWrite];
    }
}

- (NSCachedURLResponse *)connection:(NSURLConnection *)connection willCacheResponse:(NSCachedURLResponse *)cachedResponse {
    if([_delegate respondsToSelector:@selector(connection:willCacheResponse:)]) {
        return [_delegate connection:connection willCacheResponse:cachedResponse];
    }
    return cachedResponse;
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    if([_delegate respondsToSelector:@selector(connectionDidFinishLoading:)]) {
        [_delegate connectionDidFinishLoading:connection];
    }
    dispatch_group_leave(_group);
    CFRunLoopStop(CFRunLoopGetCurrent());
}

@end

@implementation NSURLConnection (SynchronousDelegate)

+ (NSData *)sendSynchronousRequest:(NSURLRequest *)request returningResponse:(NSURLResponse **)response error:(NSError **)error delegate:(id) delegate {
    dispatch_group_t group = dispatch_group_create();
    dispatch_group_enter(group);
    
    NSURLConnectionSynchronousDelegate *privateDelegate = [[NSURLConnectionSynchronousDelegate alloc] initWithDelegate:delegate group:group];
    
    dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL), ^() {
        NSURLConnection *connection = [NSURLConnection connectionWithRequest:request delegate:privateDelegate];
        if(connection) {
            [connection start];
            
            CFRunLoopRun();
        }
    });

    // wait for block finished
    dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
    dispatch_release(group);
    
    NSData *data = [privateDelegate data];
    *error = [privateDelegate error];
    *response = [privateDelegate response];
    
    [delegate release];
    
    return data;
}

@end
