//
//  XMLRPCHelper.h
//  Linphone
//
//  Created by Brieuc on 06/09/2016.
//

@interface XMLRPCHelper : NSObject
/* This class is only here to handle
 *
 * All the rest is implemented to do nothing
 */

@property(nonatomic, strong) NSMutableArray *personsArray;

+ (void)sendXMLRPCRequest:(NSString *)method;
+ (void)sendXMLRPCRequestWithParams:(NSString *)method withParams:(NSArray *)params;
+ (void)sendXMLRPCRequestWithParams:(NSString *)method
						 withParams:(NSArray *)params
						  onSuccess:(void (^)(NSString *response))block;
+ (void)sendXMLRPCRequestWithParams:(NSString *)method
						 withParams:(NSArray *)params
						  onSuccess:(void (^)(NSString *response))successBlock
							onError:(void (^)(NSString *response))errorBlock;

- (void)dealWithXmlRpcResponse:(LinphoneXmlRpcRequest *)request;
- (void)displayErrorPopup:(NSString *)error;
+ (void)initArray;

@end
