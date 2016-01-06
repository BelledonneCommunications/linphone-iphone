//
//  UrmetXMLRPC.m
//  IperVoice
//
//  Created by guillaume on 01/06/2015.
//  Copyright (c) 2015 Urmet. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "LinphoneManager.h"
#import "DTAlertView.h"
#import "XMLRPCHelper.h"
#import "Utils.h"

/* This subclass allows use to store the block to execute on success */
@interface BlockXMLRPCRequest : XMLRPCRequest
@property(copy, nonatomic) BOOL (^successBlock)(XMLRPCResponse *response);
@property(copy, nonatomic) BOOL (^xmlErrorBlock)(XMLRPCRequest *request);
@end

@implementation BlockXMLRPCRequest
@end

@implementation XMLRPCHelper

#pragma mark - API

static XMLRPCHelper *xmlManager = nil;

+ (XMLRPCHelper *)xml {
	@synchronized(self) {
		if (xmlManager == nil) {
			xmlManager = [[XMLRPCHelper alloc] init];
		}
	}
	return xmlManager;
}

- (void)sendXMLRequestMethod:(NSString *)method withParams:(NSArray *)params {
	[self sendXMLRequestMethod:method withParams:params onSuccess:nil onError:nil];
}

- (void)sendXMLRequestMethod:(NSString *)method
				  withParams:(NSArray *)params
				   onSuccess:(BOOL (^)(XMLRPCResponse *))successBlock {
	[self sendXMLRequestMethod:method withParams:params onSuccess:successBlock onError:nil];
}

- (void)sendXMLRequestMethod:(NSString *)method
				  withParams:(NSArray *)params
				   onSuccess:(BOOL (^)(XMLRPCResponse *))successBlock
					 onError:(BOOL (^)(XMLRPCRequest *req))errorBlock {
	LOGI(@"XMLRPC %@ - %@", method, params);
	NSURL *URL =
		[NSURL URLWithString:[LinphoneManager.instance lpConfigStringForKey:@"xmlrpc_url" inSection:@"assistant"]];
	BlockXMLRPCRequest *request = [[BlockXMLRPCRequest alloc] initWithURL:URL];
	[request setMethod:method withParameters:params];
	if (successBlock) {
		request.successBlock = successBlock;
	}
	if (errorBlock) {
		request.xmlErrorBlock = errorBlock;
	}

	XMLRPCConnectionManager *manager = [XMLRPCConnectionManager sharedManager];
	[manager spawnConnectionWithXMLRPCRequest:request delegate:self];
}

#pragma mark - XMLRPCConnectionHandler delegate

- (void)request:(XMLRPCRequest *)request didReceiveResponse:(XMLRPCResponse *)response {

	BlockXMLRPCRequest *req = (BlockXMLRPCRequest *)request;
	NSString *error = nil;
	BOOL handleHere = YES;

	LOGI(@"XMLRPC %@ - %@", [request method], [response body]);

	if (req.successBlock) {
		handleHere = req.successBlock(response);
	}
	if (!handleHere)
		return;

	if ([response isFault]) {
		error = response.faultString;
	} else if (response.object != nil && ![response.object isEqualToString:@"OK"]) {
		error = NSLocalizedString(@"Unknown error", nil);
	} else if ([response.object isEqualToString:@"OK"]) {
		// do nothing, if the client is interested in the response he will have handled it
	} else {
		LOGE(@"Empty object for XMLRPC response: HTTP error");
		error = NSLocalizedString(@"(no description)", nil);
	}

	if (error != nil) {
		[self displayErrorPopup:error];
	}
}

- (void)request:(XMLRPCRequest *)request didFailWithError:(NSError *)error {
	BlockXMLRPCRequest *req = (BlockXMLRPCRequest *)request;
	BOOL handleHere = YES;
	if (req.xmlErrorBlock) {
		handleHere = req.xmlErrorBlock(request);
	}
	if (!handleHere)
		return;
	// do not display technical message to the user..
	[self displayErrorPopup:NSLocalizedString(@"Server error", nil)]; // error.localizedDescription];
	LOGE(@"requestDidFailWithError: %@", error.localizedDescription);
}

- (BOOL)request:(XMLRPCRequest *)request canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
	return NO;
}

- (void)request:(XMLRPCRequest *)request didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
	/* Do nothing, not needed */
}

- (void)request:(XMLRPCRequest *)request didCancelAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
	/* Do nothing, not needed */
}

#pragma mark - Error alerts

- (void)displayErrorPopup:(NSString *)error {
	DTAlertView *av = [[DTAlertView alloc] initWithTitle:NSLocalizedString(@"Server request error", nil) message:error];
	[av addCancelButtonWithTitle:NSLocalizedString(@"OK", nil) block:nil];
	[av show];
}

+ (void)GetProvisioningURL:(NSString *)username
				  password:(NSString *)password
					domain:(NSString *)domain
				 OnSuccess:(void (^)(NSString *response))onSuccess {
	if (!username || !password || !domain) {
		onSuccess(nil);
		return;
	}

	[self.class.xml sendXMLRequestMethod:@"get_remote_provisioning_filename"
		withParams:@[ username, password, domain ]
		onSuccess:^BOOL(XMLRPCResponse *response) {
		  if (!response.isFault && response.object) {
			  NSString *url =
				  [NSString stringWithFormat:@"%@/%@.xml",
											 [LinphoneManager.instance lpConfigStringForKey:@"remote_prosivioning_root"
																				  inSection:@"assistant"],
											 response.object];
			  onSuccess(url);
		  } else {
			  onSuccess(nil);
		  }
		  return FALSE;

		}
		onError:^BOOL(XMLRPCRequest *request) {
		  onSuccess(nil);
		  return FALSE;
		}];
}
@end
