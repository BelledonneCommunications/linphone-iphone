/* BuschJaegerConfigParser.h
 *
 * Copyright (C) 2011  Belledonne Comunications, Grenoble, France
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

#import <Foundation/Foundation.h>

#import "OutdoorStation.h"
#import "User.h"
#import "Network.h"
#import "History.h"
#import "LevelPushButton.h"

@protocol BuschJaegerConfigurationDelegate <NSObject>

- (void)buschJaegerConfigurationSuccess;
- (void)buschJaegerConfigurationError:(NSString *)error;

@end

@interface BuschJaegerConfiguration : NSObject<NSURLConnectionDelegate> {
}

typedef enum _BuschJaegerConfigurationRequestType{
    BuschJaegerConfigurationRequestType_Local,
    BuschJaegerConfigurationRequestType_Global
} BuschJaegerConfigurationRequestType;

@property (readonly) NSMutableSet *history;
@property (readonly) NSMutableSet *users;
@property (readonly) NSMutableSet *outdoorStations;
@property (readonly) Network *network;
@property (readonly) LevelPushButton *levelPushButton;
@property (readonly) CFArrayRef certificates;

- (void)reset;

- (BOOL)loadFile:(NSString*)file;
- (BOOL)saveFile:(NSString*)file;
- (BOOL)parseQRCode:(NSString*)data delegate:(id<BuschJaegerConfigurationDelegate>)delegate;

- (BOOL)loadHistory:(BuschJaegerConfigurationRequestType)type delegate:(id<BuschJaegerConfigurationDelegate>)delegate;
- (BOOL)removeHistory:(BuschJaegerConfigurationRequestType)type history:(History*)history delegate:(id<BuschJaegerConfigurationDelegate>)delegate;

- (User*)getCurrentUser;

- (NSString*)getImageUrl:(BuschJaegerConfigurationRequestType)type image:(NSString *)image;

- (NSString*)getGateway:(BuschJaegerConfigurationRequestType)type;

+ (NSString*)getRegexValue:(NSString*)regexString data:(NSString*)data;

@end
