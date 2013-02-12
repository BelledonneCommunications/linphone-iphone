/* UIRemoteImageView.m
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

#import "UIRemoteImageView.h"
#import "BuschJaegerConfiguration.h"
#import "LinphoneManager.h"
#import "NSURLConnection+SynchronousDelegate.h"

@implementation UIRemoteImageView

@synthesize waitIndicatorView;


#pragma mark - Lifecycle Functions

- (void)initUIRemoteImageView {
    waitIndicatorView = [[UIActivityIndicatorView alloc] initWithFrame:self.frame];
    waitIndicatorView.hidesWhenStopped = TRUE;
    waitIndicatorView.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    [self addSubview:waitIndicatorView];
    
    // Create Queue
    queue = [[NSOperationQueue alloc] init];
    queue.name = @"RemoteImage";
    queue.maxConcurrentOperationCount = 1;
}

- (id)init {
    self = [super init];
    if(self != nil) {
        [self initUIRemoteImageView];
    }
    return self;
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if(self != nil) {
        [self initUIRemoteImageView];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if(self != nil) {
        [self initUIRemoteImageView];
    }
    return self;
}

- (id)initWithImage:(UIImage *)image {
    self = [super initWithImage:image];
    if(self != nil) {
        [self initUIRemoteImageView];
    }
    return self;
}

- (id)initWithImage:(UIImage *)image highlightedImage:(UIImage *)highlightedImage {
    self = [super initWithImage:image highlightedImage:highlightedImage];
    if(self != nil) {
        [self initUIRemoteImageView];
    }
    return self;
}

- (void)dealloc {
    [waitIndicatorView release];
    [queue release];
    
    [super dealloc];
}


#pragma mark - 

- (void)loadImage:(NSString*)url {
    [waitIndicatorView startAnimating];
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:url] cachePolicy:NSURLRequestReturnCacheDataElseLoad timeoutInterval:30];
    if(request != nil) {
        [queue cancelAllOperations];
        NSBlockOperation *operation = [[[NSBlockOperation alloc] init] autorelease];
        [operation addExecutionBlock:^(void) {
            NSURLResponse *response = nil;
            NSError *error = nil;
            NSData *data  = nil;
          
            
            data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error delegate:self];
            if(![operation isCancelled]) {
                if(data != nil) {
                    UIImage *image = [UIImage imageWithData:data];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [waitIndicatorView stopAnimating];
                        [self setImage:image];
                    });
                } else {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [waitIndicatorView stopAnimating];
                    });
                }
            }
        }];
        [queue addOperation:operation];
    }
}
- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace
{
    BuschJaegerConfiguration* conf = [[LinphoneManager instance] configuration];
    return [conf canHandleAuthChallenge:connection:protectionSpace];
}
- (void)connection:(NSURLConnection *)conn didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge
{
    BuschJaegerConfiguration* conf = [[LinphoneManager instance] configuration];
    [conf handleAuthChallenge:conn:challenge];
}
@end
