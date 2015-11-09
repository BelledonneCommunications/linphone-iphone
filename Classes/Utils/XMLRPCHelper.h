//
//  UrmetXMLRPC.h
//  IperVoice
//
//  Created by guillaume on 27/05/2015.
//  Copyright (c) 2015 Urmet. All rights reserved.
//

#import <XMLRPCConnection.h>
#import <XMLRPCConnectionDelegate.h>
#import <XMLRPCConnectionManager.h>
#import <XMLRPCResponse.h>
#import <XMLRPCRequest.h>

@interface XMLRPCHelper : NSObject <XMLRPCConnectionDelegate>
/* This class is only here to handle XMLRPC responses.
 *
 * The implementation for didReceiveResponse: (XMLRPCResponse *)response will check if the XMLRPC
 * responded 'OK', in which case the view will return to idle, or if there is an error, an
 * Alert will be displayed with the error message.
 *
 * All the rest is implemented to do nothing, which is what we want for Urmet
 */

/**
 * Will send the XML request to the 'xmlrpc_url' server that is defined in the 'assistant' section
 * of the linphonerc file.
 * You must implement the - didReceiveResponse method if you are using this.
 */
- (void)sendXMLRequestMethod:(NSString *)method withParams:(NSArray *)params;
- (void)sendXMLRequestMethod:(NSString *)method
				  withParams:(NSArray *)params
				   onSuccess:(BOOL (^)(XMLRPCResponse *response))block;
- (void)sendXMLRequestMethod:(NSString *)method
				  withParams:(NSArray *)params
				   onSuccess:(BOOL (^)(XMLRPCResponse *response))successBlock
					 onError:(BOOL (^)(XMLRPCRequest *request))errorBlock;

@end
