/* ImageSharing.m
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

#import "ImageSharing.h"
#import "Utils.h"
#import "LinphoneManager.h"

@implementation ImageSharing

@synthesize connection;
@synthesize data;
@synthesize upload;
@synthesize userInfo;

#pragma mark - Lifecycle Functions

+ (id)imageSharingUpload:(NSURL*)url image:(UIImage*)image delegate:(id<ImageSharingDelegate>)delegate userInfo:(id)auserInfo{
    ImageSharing *imgs = [[ImageSharing alloc] init];
    if(imgs != nil) {
        imgs.userInfo = auserInfo;
        imgs->upload = TRUE;
        imgs->delegate = [delegate retain];
        imgs->data = [[NSMutableData alloc] init];
        if(delegate) {
            [delegate imageSharingProgress:imgs progress:0];
        }
        [imgs uploadImageTo:url image:image];
    }
    return imgs;
}

+ (id)imageSharingDownload:(NSURL*)url delegate:(id<ImageSharingDelegate>)delegate userInfo:(id)auserInfo{
    ImageSharing *imgs = [[ImageSharing alloc] init];
    if(imgs != nil) {
        imgs.userInfo = auserInfo;
        imgs->upload = FALSE;
        imgs->delegate = [delegate retain];
        imgs->data = [[NSMutableData alloc] init];
        if(delegate) {
            [delegate imageSharingProgress:imgs progress:0];
        }
        [imgs downloadImageFrom:url];
    }
    return imgs;
}

- (void)dealloc {
    [connection release];
    [data release];
    [delegate release];
    [userInfo release];
    [super dealloc];
}


#pragma mark -

- (void)cancel {
    [connection cancel];
    [LinphoneLogger log:LinphoneLoggerLog format:@"File transfer interrupted by user"];
    if(delegate) {
        [delegate imageSharingAborted:self];
    }
}


- (void)downloadImageFrom:(NSURL*)url {
	[LinphoneLogger log:LinphoneLoggerLog format:@"downloading [%@]", [url absoluteString]];
    
	NSURLRequest* request = [NSURLRequest requestWithURL:url
											 cachePolicy:NSURLRequestUseProtocolCachePolicy
										 timeoutInterval:60.0];
    
	connection = [[NSURLConnection alloc] initWithRequest:request delegate: self];
}


- (void)uploadImageTo:(NSURL*)url image:(UIImage*)image {
    [LinphoneLogger log:LinphoneLoggerLog format:@"downloading [%@]", [url absoluteString]];
	
	// setting up the request object now
	NSMutableURLRequest *request = [[[NSMutableURLRequest alloc] init] autorelease];
	[request setURL:url];
	[request setHTTPMethod:@"POST"];
	
	/*
	 add some header info now
	 we always need a boundary when we post a file
	 also we need to set the content type
	 
	 You might want to generate a random boundary.. this is just the same
	 as my output from wireshark on a valid html post
	 */
	NSString *boundary = @"---------------------------14737809831466499882746641449";
	NSString *contentType = [NSString stringWithFormat:@"multipart/form-data; boundary=%@",boundary];
	[request addValue:contentType forHTTPHeaderField: @"Content-Type"];
	
	/*
	 now lets create the body of the post
	 */
	NSMutableData *body = [NSMutableData data];
    NSString *imageName = [NSString stringWithFormat:@"%i-%f.jpg", [image hash],[NSDate timeIntervalSinceReferenceDate]];
	[body appendData:[[NSString stringWithFormat:@"\r\n--%@\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];
	[body appendData:[[NSString stringWithFormat:@"Content-Disposition: form-data; name=\"userfile\"; filename=\"%@\"\r\n",imageName] dataUsingEncoding:NSUTF8StringEncoding]];
	[body appendData:[@"Content-Type: application/octet-stream\r\n\r\n" dataUsingEncoding:NSUTF8StringEncoding]];
	[body appendData:[NSData dataWithData:UIImageJPEGRepresentation(image, 1.0)]];
	[body appendData:[[NSString stringWithFormat:@"\r\n--%@--\r\n",boundary] dataUsingEncoding:NSUTF8StringEncoding]];
	[request setHTTPBody:body];
	
	connection = [[NSURLConnection alloc] initWithRequest:(NSURLRequest *)request delegate:self];
}


#pragma mark - NSURLConnectionDelegate

- (void)connection:(NSURLConnection *)aconnection didFailWithError:(NSError *)error {
    if(delegate) {
        [delegate imageSharingError:self error:error];
    }
    [self release];
}

- (void)connection:(NSURLConnection *)connection didSendBodyData:(NSInteger)bytesWritten totalBytesWritten:(NSInteger)totalBytesWritten totalBytesExpectedToWrite:(NSInteger)totalBytesExpectedToWrite {
    if(upload && delegate) {
        [delegate imageSharingProgress:self progress:(float)totalBytesWritten/(float)totalBytesExpectedToWrite];
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveData:(NSData *)adata {
    [data appendData:adata];
    if(!upload && delegate) {
        [delegate imageSharingProgress:self progress:(float)data.length/(float)totalBytesExpectedToRead];
    }
}

- (void)connection:(NSURLConnection *)connection didReceiveResponse:(NSURLResponse *)response {
	NSHTTPURLResponse * httpResponse = (NSHTTPURLResponse *) response;
	statusCode = httpResponse.statusCode;
	[LinphoneLogger log:LinphoneLoggerLog format:@"File transfer status code [%i]", statusCode];
    
    if (statusCode == 200 && !upload) {
        totalBytesExpectedToRead = [response expectedContentLength];
    }
}

- (void)connectionDidFinishLoading:(NSURLConnection *)connection {
    if(statusCode >= 400) {
        NSError *error = [NSError errorWithDomain:@"ImageSharing" code:statusCode userInfo:nil];
        if(delegate) {
            [delegate imageSharingError:self error:error];
        }
        return;
    }
	if (upload) {
        NSString* imageRemoteUrl = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
        [LinphoneLogger log:LinphoneLoggerLog format:@"File can be downloaded from [%@]", imageRemoteUrl];
        if(delegate) {
            [delegate imageSharingUploadDone:self url:[NSURL URLWithString:imageRemoteUrl]];
        }
	} else {
		UIImage* image = [UIImage imageWithData:data];
        [LinphoneLogger log:LinphoneLoggerLog format:@"File downloaded"];
        if(delegate) {
            [delegate imageSharingDownloadDone:self image:image];
        }
	}
    [self release];
}

@end
