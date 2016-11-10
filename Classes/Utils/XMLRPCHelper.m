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

/* This subclass allows use to store the block to execute on success */
@interface XMLRPCRequestObject : NSObject
@property(copy, nonatomic) void (^XMLRPCHelperBlockSuccess)(NSString *something);
@property(copy, nonatomic) void (^XMLRPCHelperBlockError)(NSString *something);
@property LinphoneXmlRpcRequest *xmlRpcRequest;
@end

@implementation XMLRPCRequestObject
@end

@implementation XMLRPCHelper

#pragma mark - API

// typedef void (^XMLRPCHelperBlock)(NSString *something);

// XMLRPCHelperBlock successBlock = nil;
// XMLRPCHelperBlock errorBlock = nil;

NSMutableArray *personsArray;

/*****************************************************************************************/
+ (void)initArray {
	personsArray = [[NSMutableArray alloc] init];
}

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
	XMLRPCRequestObject *requestObject = [XMLRPCRequestObject alloc];
	const char *URL =
		[LinphoneManager.instance lpConfigStringForKey:@"receipt_validation_url" inSection:@"in_app_purchase"]
			.UTF8String;

	requestObject.XMLRPCHelperBlockSuccess = successBk;
	requestObject.XMLRPCHelperBlockError = errorBk;

	// Create LinphoneXMLRPCRequest
	LinphoneXmlRpcSession *requestSession = linphone_xml_rpc_session_new(LC, URL);
	// LinphoneXmlRpcRequest *request = linphone_xml_rpc_request_new(method.UTF8String, LinphoneXmlRpcArgString);
	requestObject.xmlRpcRequest = linphone_xml_rpc_request_new(method.UTF8String, LinphoneXmlRpcArgString);

	[personsArray addObject:requestObject];
	// Set argument to this LinphoneXMLRPCRequest
	for (NSString *item in params) {
		NSLog(@"Linphone XMLRPC Request with argument: %@", item);
		linphone_xml_rpc_request_add_string_arg(requestObject.xmlRpcRequest, item.UTF8String);
	}

	// Ref and send the LinphoneXMLRPCRequest
	requestSession = linphone_xml_rpc_session_ref(requestSession);
	linphone_xml_rpc_session_send_request(requestSession, requestObject.xmlRpcRequest);

	// Set the callbacks to this LinphoneXMLRPCRequest
	LinphoneXmlRpcRequestCbs *cbs = linphone_xml_rpc_request_get_callbacks(requestObject.xmlRpcRequest);

	// Register XMLRPCHelper in user data to get it back on Callback rised
	XMLRPCHelper *xMLRPCHelper = [[XMLRPCHelper alloc] init];
	linphone_xml_rpc_request_set_user_data(requestObject.xmlRpcRequest, ((void *)CFBridgingRetain(xMLRPCHelper)));

	// Set the response Callback
	linphone_xml_rpc_request_cbs_set_response(cbs, linphone_xmlrpc_call_back_received);
}

static void linphone_xmlrpc_call_back_received(LinphoneXmlRpcRequest *request) {
	[(__bridge XMLRPCHelper *)linphone_xml_rpc_request_get_user_data(request) dealWithXmlRpcResponse:request];
}

- (void)dealWithXmlRpcResponse:(LinphoneXmlRpcRequest *)request {
	XMLRPCRequestObject *xmlrpcObject;
	NSInteger index = 0;
	for (int i = 0; i < [personsArray count]; i++) {
		xmlrpcObject = [personsArray objectAtIndex:i];
		if (xmlrpcObject.xmlRpcRequest == request)
			break;
		index++;
	}

	NSString *responseString =
		[NSString stringWithFormat:@"%s", (linphone_xml_rpc_request_get_string_response(request))];
	LOGI(@"XMLRPC query: %@", responseString);
	if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusOk) {
		// Call success block
		xmlrpcObject.XMLRPCHelperBlockSuccess(responseString);
	} else if (linphone_xml_rpc_request_get_status(request) == LinphoneXmlRpcStatusFailed) {
		if (xmlrpcObject.XMLRPCHelperBlockError != nil) {
			LOGI(@"XMLRPC query ErrorBlock rised");
			xmlrpcObject.XMLRPCHelperBlockError(responseString);
		}
		// Display Error alert
		[self displayErrorPopup:@"LinphoneXMLRPC Request Failed"];
	}
	linphone_xml_rpc_request_unref(request);
	[personsArray removeObjectAtIndex:index];
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
