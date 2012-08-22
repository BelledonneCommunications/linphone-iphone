/* BuschJaegerConfigParser.m
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

#import "BuschJaegerConfigParser.h"
#import "LinphoneManager.h"
#import "Utils.h"

@implementation BuschJaegerConfigParser

@synthesize outdoorStations;
@synthesize network;

/********
 [outdoorstation_0]
 address=elviish@test.linphone.org
 name=Front Door
 screenshot=yes
 surveillance=yes
 
 [outdoorstation_1]
 address=2
 name=Back Door
 screenshot=no
 surveillance=no
 
 [outdoorstation_2]
 address=3
 name=Roof Door
 screenshot=no
 surveillance=yes
 
 [levelpushbutton]
 name=Apartment Door
 
 [network]
 domain=test.linphone.org
 local-address=test.linphone.org
 global-address=sip.linphone.org
 
 [user_0]
 user=miaou
 opendoor=yes
 surveillance=yes
 switchlight=yes
 switching=yes
 eillance=no
 
 [outdoorstation_2]
 address=3
 name=Roof Door
 screenshot=no
 surveillance=yes
 
 [levelpushbutton]
 name=Apartment Door
 
 [network]
 domain=test.linphone.org
 local-address=test.linphone.org
 global-address=sip.linphone.org
 
 [user_0]
 user=miaou
 opendoor=yes
 surveillance=yes
 switchlight=yes
 switching=yes
***************/

- (id)init {
    self = [super init];
    if(self != nil) {
        outdoorStations = [[NSMutableSet alloc] init];
    }
    return self;
}

- (void)dealloc {
    [outdoorStations release];
    [super dealloc];
}

+ (NSString*)getRegexValue:(NSString*)regexString data:(NSString*)data {
    NSError  *error;
    NSRegularExpression *regex = [NSRegularExpression
             regularExpressionWithPattern:regexString
             options:0
             error:&error];
    
    NSTextCheckingResult* result = [regex firstMatchInString:data options:0 range:NSMakeRange(0, [data length])];
    if(result && result.numberOfRanges == 2) {
        NSRange range = [result rangeAtIndex:1];
        return [data substringWithRange:range];
    }
    return nil;
}

- (void)parseSection:(NSString*)section array:(NSArray*)array {
    id obj;
    if((obj = [OutdoorStation parse:section array:array]) != nil) {
        [outdoorStations addObject:obj];
    } else if((obj = [Network parse:section array:array]) != nil) {
        self.network = obj;
    } else {
        [LinphoneLogger log:LinphoneLoggerWarning format:@"Unknown section: %@", section];
    }
}

- (BOOL)parseConfig:(NSString*)data delegate:(id<BuschJaegerConfigParser>)delegate {
    [LinphoneLogger log:LinphoneLoggerDebug format:@"%@", data];
    NSArray *arr = [data componentsSeparatedByString:@"\n"];
    NSString *last_section = nil;
    int last_index = -1;
    
    for (int i = 0; i < [arr count]; ++i) {
        NSString *subStr = [arr objectAtIndex:i];
        if([subStr hasPrefix:@"["]) {
            if([subStr hasSuffix:@"]"]) {
                if(last_index != -1) {
                    NSArray *subArray = [NSArray arrayWithArray:[arr subarrayWithRange:NSMakeRange(last_index, i - last_index)]];
                    [self parseSection:last_section array:subArray];
                }
                last_section = subStr;
                last_index = i + 1;
            } else {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [delegate buschJaegerConfigParserError:NSLocalizedString(@"Invalid configuration file", nil)];
                });
                return FALSE;
            }
        }
    }
    if(last_index != -1) {
        NSArray *subArray = [NSArray arrayWithArray:[arr subarrayWithRange:NSMakeRange(last_index, [arr count] - last_index)]];
        [self parseSection:last_section array:subArray];
    }
    
    return TRUE;
}

- (void)reset {
    [outdoorStations removeAllObjects];
    self.network = nil;
}

- (BOOL)saveFile:(NSString*)file {
    NSMutableString *data = [NSMutableString string];
    for(OutdoorStation *os in outdoorStations) {
        [data appendString:[os write]];
    }
    
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsPath = [paths objectAtIndex:0];
    NSString *databaseDocumentPath = [documentsPath stringByAppendingPathComponent:file];
    
    NSError *error;
    if(![data writeToFile:databaseDocumentPath atomically:FALSE encoding:NSUTF8StringEncoding error:&error]) {
        [LinphoneLogger log:LinphoneLoggerError format:@"Can't write BuschJaeger ini file: %@", [error localizedDescription]];
        return FALSE;
    }
    return TRUE;
}

- (BOOL)loadFile:(NSString*)file {
    [self reset];
    
    NSFileManager *fileManager = [NSFileManager defaultManager];
    NSArray *paths = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES);
    NSString *documentsPath = [paths objectAtIndex:0];
    NSString *databaseDocumentPath = [documentsPath stringByAppendingPathComponent:file];
    if ([fileManager fileExistsAtPath:databaseDocumentPath] == NO) {
        [LinphoneLogger log:LinphoneLoggerError format:@"BuschJaeger ini file doesn't exist: %@", file];
        return FALSE;
    }
    NSError *error;
    NSString *data = [NSString stringWithContentsOfFile:databaseDocumentPath encoding:NSUTF8StringEncoding error:&error];
    if(data == nil) {
        [LinphoneLogger log:LinphoneLoggerError format:@"Can't read BuschJaeger ini file: %@", [error localizedDescription]];
        return FALSE;
    }
    return [self parseConfig:data delegate:nil];;
}

- (BOOL)parseQRCode:(NSString*)data delegate:(id<BuschJaegerConfigParser>)delegate {
    [self reset];
    NSString *urlString = [BuschJaegerConfigParser getRegexValue:@"URL=([^\\s]+)" data:data];
    NSString *userString = [BuschJaegerConfigParser getRegexValue:@"USER=([^\\s]+)" data:data];
    NSString *passwordString = [BuschJaegerConfigParser getRegexValue:@"PW=([^\\s]+)" data:data];

    if(urlString != nil && userString != nil && passwordString != nil) {
        NSURLRequest *request = [[NSURLRequest alloc] initWithURL:[NSURL URLWithString:urlString] cachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData timeoutInterval:5];
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL), ^(void) {
            NSURLResponse *response = nil;
            NSError *error = nil;
            NSData *data  = nil;
            data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
            if(data == nil) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [delegate buschJaegerConfigParserError:[error localizedDescription]];
                });
            } else {
                if([self parseConfig:[NSString stringWithUTF8String:[data bytes]] delegate:delegate]) {
                    [[NSUserDefaults standardUserDefaults] setObject:userString forKey:@"username_preference"];
                    [[NSUserDefaults standardUserDefaults] setObject:network.domain forKey:@"domain_preference"];
                    [[NSUserDefaults standardUserDefaults] setObject:passwordString forKey:@"password_preference"];
                    [[LinphoneManager instance] reconfigureLinphone];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [delegate buschJaegerConfigParserSuccess];
                    });
                }
            }
        });
        return TRUE;
    } else {
        return FALSE;
    }
}

@end
