/*
 * Copyright (c) 2010-2020 Belledonne Communications SARL.
 *
 * This file is part of linphone-iphone 
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

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
