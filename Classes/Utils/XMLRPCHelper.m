//
//  UrmetXMLRPC.m
//  IperVoice
//
//  Created by guillaume on 01/06/2015.
//  Copyright (c) 2015 Urmet. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "LinphoneManager.h"
#import "XMLRPCHelper.h"
#import "Utils.h"
#import "PhoneMainView.h"


@implementation XMLRPCHelper

#pragma mark - API

typedef void (^XMLRPCHelperBlock)(NSString *something);

XMLRPCHelperBlock successBlock = nil;
XMLRPCHelperBlock errorBlock = nil;

/*****************************************************************************************/

+ (void)sendXMLRPCRequest:(NSString *)method {
	[self sendXMLRPCRequestWithParams:method withParams:nil onSuccess:nil onError:nil];
}

+ (void)sendXMLRPCRequestWithParams:(NSString *)method withParams:(NSArray *)params {
	[self sendXMLRPCRequestWithParams:method withParams:params onSuccess:nil onError:nil];
}

+ (void)sendXMLRPCRequestWithParams:(NSString *)method
						 withParams:(NSArray *)params
						  onSuccess:(void (^)(NSString *))successBk {
	[self sendXMLRPCRequestWithParams:method withParams:params onSuccess:successBk onError:nil];
}

// change block by callback and implement callback with different behavior if success (: call InAppManager) or error (:
// manage error here)
+ (void)sendXMLRPCRequestWithParams:(NSString *)method
						 withParams:(NSArray *)params
						  onSuccess:(void (^)(NSString *))successBk
							onError:(void (^)(NSString *req))errorBk {
	LOGI(@"XMLRPC %@ - %@", method, params);
	const char *URL =
		[LinphoneManager.instance lpConfigStringForKey:@"receipt_validation_url" inSection:@"in_app_purchase"]
			.UTF8String;

	successBlock = successBk;
	errorBlock = errorBk;

	// Create LinphoneXMLRPCRequest
	LinphoneXmlRpcSession *requestSession = linphone_xml_rpc_session_new(LC, URL);
	LinphoneXmlRpcRequest *request = linphone_xml_rpc_request_new(method.UTF8String, LinphoneXmlRpcArgString);

	// Set argument to this LinphoneXMLRPCRequest
	for (NSString *item in params) {
		NSLog(@"Linphone XMLRPC Request with argument: %@", item);
		linphone_xml_rpc_request_add_string_arg(request, item.UTF8String);
	}

	// Ref and send the LinphoneXMLRPCRequest
	requestSession = linphone_xml_rpc_session_ref(requestSession);
	linphone_xml_rpc_session_send_request(requestSession, request);

	// Set the callbacks to this LinphoneXMLRPCRequest
	LinphoneXmlRpcRequestCbs *cbs = linphone_xml_rpc_request_get_callbacks(request);

	// Register XMLRPCHelper in user data to get it back on Callback rised
	XMLRPCHelper *xMLRPCHelper = [[XMLRPCHelper alloc] init];
	linphone_xml_rpc_request_set_user_data(request, ((void *)CFBridgingRetain(xMLRPCHelper)));

	// Set the response Callback
	linphone_xml_rpc_request_cbs_set_response(cbs, linphone_xmlrpc_call_back_received);
}

static void linphone_xmlrpc_call_back_received(LinphoneXmlRpcRequest *request) {
	[(__bridge XMLRPCHelper *)linphone_xml_rpc_request_get_user_data(request) dealWithXmlRpcResponse:request];
}

- (void)dealWithXmlRpcResponse:(LinphoneXmlRpcRequest *)request {
	NSString *responseString =
		[NSString stringWithFormat:@"%s", (linphone_xml_rpc_request_get_string_response(request))];
	LOGI(@"XMLRPC query: %@", responseString);
	if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
		// Call success block
		successBlock(responseString);
	} else if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusFailed) {
		if (errorBlock != nil) {
			LOGI(@"XMLRPC query ErrorBlock rised");
			errorBlock(responseString);
		}
		// Display Error alert
		[self displayErrorPopup:@"LinphoneXMLRPC Request Failed"];
	}
	linphone_xml_rpc_request_unref(request);
}

#pragma mark - Error alerts

- (void)displayErrorPopup:(NSString *)error {
	UIAlertController *errView = [UIAlertController alertControllerWithTitle:NSLocalizedString(@"Server request error", nil)
																	 message:error
															  preferredStyle:UIAlertControllerStyleAlert];
	
	UIAlertAction* defaultAction = [UIAlertAction actionWithTitle:NSLocalizedString(@"OK", nil)
															style:UIAlertActionStyleDefault
														  handler:^(UIAlertAction * action) {}];
	
	[errView addAction:defaultAction];
	[PhoneMainView.instance presentViewController:errView animated:YES completion:nil];
}

@end

/*****************************************************************************************/
