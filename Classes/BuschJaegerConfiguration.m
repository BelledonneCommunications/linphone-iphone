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

#import "BuschJaegerConfiguration.h"
#import "LinphoneManager.h"
#import "Utils.h"
#import "NSURLConnection+SynchronousDelegate.h"

@implementation BuschJaegerConfiguration

@synthesize outdoorStations;
@synthesize users;
@synthesize network;
@synthesize history;
@synthesize levelPushButton;
@synthesize certificates;

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
        users = [[NSMutableSet alloc] init];
        history = [[NSMutableSet alloc] init];
        network = [[Network alloc] init];
        levelPushButton = [[LevelPushButton alloc] init];
        certificates = NULL;
        [self reloadCertificates];
    }
    return self;
}

- (void)dealloc {
    [outdoorStations release];
    [users release];
    [history release];
    [network release];
    [levelPushButton release];
    if(certificates != NULL) {
        CFRelease(certificates);
    }
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
        return [data substringWithRange:[result rangeAtIndex:1]];
    }
    return nil;
}

- (BOOL)parseHistory:(NSString*)data delegate:(id<BuschJaegerConfigurationDelegate>)delegate {
    NSArray *arr = [data componentsSeparatedByString:@"\n"];
    for (NSString *line in arr) {
        if([line length]) {
            History *his = [History parse:line];
            if(his) {
                [history addObject:his];
            }
        }
    }
    return TRUE;
}

- (void)parseSection:(NSString*)section array:(NSArray*)array {
    id obj;
    if((obj = [OutdoorStation parse:section array:array]) != nil) {
        [outdoorStations addObject:obj];
    } else if((obj = [User parse:section array:array]) != nil) {
        [users addObject:obj];
    } else if((obj = [Network parse:section array:array]) != nil) {
        if(network != nil) {
            [network release];
        }
        network = [obj retain];
    } else if((obj = [LevelPushButton parse:section array:array]) != nil) {
        if(levelPushButton != nil) {
            [levelPushButton release];
        }
        levelPushButton = [obj retain];
    }else {
        [LinphoneLogger log:LinphoneLoggerWarning format:@"Unknown section: %@", section];
    }
}

- (BOOL)parseConfig:(NSString*)data delegate:(id<BuschJaegerConfigurationDelegate>)delegate {
    [LinphoneLogger log:LinphoneLoggerDebug format:@"%@", data];
    NSArray *arr = [data componentsSeparatedByString:@"\n"];
    NSString *last_section = nil;
    int last_index = -1;
    
    for (int i = 0; i < [arr count]; ++i) {
        NSString *subStr = [arr objectAtIndex:i];
        if([subStr hasPrefix:@"["]) {
            if([subStr hasSuffix:@"]"]) {
                if(last_index != -1) {
                    NSArray *subArray = [arr subarrayWithRange:NSMakeRange(last_index, i - last_index)];
                    [self parseSection:last_section array:subArray];
                }
                last_section = subStr;
                last_index = i + 1;
            } else {
                [self reset];
                dispatch_async(dispatch_get_main_queue(), ^{
                    [delegate buschJaegerConfigurationError:NSLocalizedString(@"Invalid configuration file", nil)];
                });
                return FALSE;
            }
        }
    }
    if(last_index != -1) {
        NSArray *subArray = [arr subarrayWithRange:NSMakeRange(last_index, [arr count] - last_index)];
        [self parseSection:last_section array:subArray];
    }
    
    return TRUE;
}

- (BOOL)downloadCertificates:(id<BuschJaegerConfigurationDelegate>)delegate {
    if(network.tlsCertificate && [network.tlsCertificate length] > 0) {
        NSURL *pemUrl = [NSURL URLWithString:network.tlsCertificate];
        NSURL *derUrl = [NSURL URLWithString:network.derCertificate];
        if(pemUrl != nil && derUrl != nil) {
            NSURLRequest *pemRequest = [NSURLRequest requestWithURL:pemUrl cachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData timeoutInterval:5];
            NSURLRequest *derRequest = [NSURLRequest requestWithURL:pemUrl cachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData timeoutInterval:5];
            if(pemRequest != nil && derRequest != nil) {
                dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL), ^(void) {
                    NSURLResponse *response = nil;
                    NSError *error = nil;
                    NSData *data  = nil;
                    data = [NSURLConnection sendSynchronousRequest:pemRequest returningResponse:&response error:&error delegate:self];
                    if(data == nil) {
                        dispatch_async(dispatch_get_main_queue(), ^{
                            [delegate buschJaegerConfigurationError:[error localizedDescription]];
                        });
                    } else {
                        NSHTTPURLResponse *urlResponse = (NSHTTPURLResponse*) response;
                        if(urlResponse.statusCode == 200) {
                            if(![data writeToFile:[LinphoneManager documentFile:kLinphonePEMPath] atomically:TRUE]) {
                                [self reset];
                                dispatch_async(dispatch_get_main_queue(), ^{
                                    [delegate buschJaegerConfigurationError:NSLocalizedString(@"Unknown issue when saving configuration", nil)];
                                });
                                return;
                            }
                        } else {
                            [self reset];
                            dispatch_async(dispatch_get_main_queue(), ^{
                                [delegate buschJaegerConfigurationError:[NSString stringWithFormat:@"Request not succeed (Status code:%d)", urlResponse.statusCode]];
                            });
                            return;
                        }
                    }
                    
                    error = nil;
                    data  = nil;
                    data = [NSURLConnection sendSynchronousRequest:derRequest returningResponse:&response error:&error delegate:self];
                    if(data == nil) {
                        dispatch_async(dispatch_get_main_queue(), ^{
                            [delegate buschJaegerConfigurationError:[error localizedDescription]];
                        });
                    } else {
                        NSHTTPURLResponse *urlResponse = (NSHTTPURLResponse*) response;
                        if(urlResponse.statusCode == 200) {
                            if(![data writeToFile:[LinphoneManager documentFile:kLinphoneDERPath] atomically:TRUE]) {
                                [self reset];
                                dispatch_async(dispatch_get_main_queue(), ^{
                                    [delegate buschJaegerConfigurationError:NSLocalizedString(@"Unknown issue when saving configuration", nil)];
                                });
                                return;
                            }
                        } else {
                            [self reset];
                            dispatch_async(dispatch_get_main_queue(), ^{
                                [delegate buschJaegerConfigurationError:[NSString stringWithFormat:@"Request not succeed (Status code:%d)", urlResponse.statusCode]];
                            });
                            return;
                        }
                    }
                    
                    [self reloadCertificates];
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [delegate buschJaegerConfigurationSuccess];
                    });
                });
                return TRUE;
            }
        } else {
            [self reset];
            dispatch_async(dispatch_get_main_queue(), ^{
                    [delegate buschJaegerConfigurationError:NSLocalizedString(@"Invalid configuration file", nil)];
            });
        }
    } 
    return FALSE;
}

- (void)unloadCertificates {
}

- (void)reloadCertificates {
    [[LinphoneManager instance] destroyLibLinphone];
    [[LinphoneManager instance] startLibLinphone];
    [self unloadCertificates];
    [self loadCertificates];
}

- (void)loadCertificates {
    
    if(certificates != NULL) {
        CFRelease(certificates);
        certificates = NULL;
    }
    NSData *data = [NSData dataWithContentsOfFile:[LinphoneManager documentFile:kLinphoneDERPath]];
    if(data != NULL) {
        SecCertificateRef rootcert = SecCertificateCreateWithData(kCFAllocatorDefault, (CFDataRef)data);
        if(rootcert) {
            const void *certs[] = { rootcert };
            certificates = CFArrayCreate(NULL, (const void**)certs, sizeof(certs) / sizeof(certs[0]), &kCFTypeArrayCallBacks);
            [LinphoneLogger log:LinphoneLoggerLog format:@"Certificates loaded"];
        } else {
            [LinphoneLogger log:LinphoneLoggerError format:@"Can't load certificates"];
        }
    } else {
        [LinphoneLogger log:LinphoneLoggerError format:@"Certificates file doesn't exist"];
    }
}

- (void)reset {
    [history removeAllObjects];
    [outdoorStations removeAllObjects];
    [users removeAllObjects];
    if(network != nil) {
        [network release];
    }
    network = [[Network alloc] init];
    if(levelPushButton != nil) {
        [levelPushButton release];
    }
    levelPushButton = [[LevelPushButton alloc] init];
}

- (BOOL)saveFile:(NSString*)file {
    NSMutableString *data = [NSMutableString string];
    for(OutdoorStation *os in outdoorStations) {
        [data appendString:[os write]];
    }
    for(User *usr in users) {
        [data appendString:[usr write]];
    }
    [data appendString:[network write]];
    [data appendString:[levelPushButton write]];
    
    NSString *databaseDocumentPath = [LinphoneManager documentFile:file];
    
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
    NSString *databaseDocumentPath = [LinphoneManager documentFile:file];
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
    return [self parseConfig:data delegate:nil];
}

- (BOOL)parseQRCode:(NSString*)data delegate:(id<BuschJaegerConfigurationDelegate>)delegate {
    [self reset];
    NSString *urlString = [BuschJaegerConfiguration getRegexValue:@"URL=([^\\s]+)" data:data];
    NSString *userString = [BuschJaegerConfiguration getRegexValue:@"USER=([^\\s]+)" data:data];
    NSString *passwordString = [BuschJaegerConfiguration getRegexValue:@"PW=([^\\s]+)" data:data];

    if(urlString != nil && userString != nil && passwordString != nil) {
        NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:urlString] cachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData timeoutInterval:5];
        if(request != nil) {
            dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL), ^(void) {
                NSURLResponse *response = nil;
                NSError *error = nil;
                NSData *data  = nil;
                data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error delegate:self];
                if(data == nil) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [delegate buschJaegerConfigurationError:[error localizedDescription]];
                    });
                } else {
                    NSHTTPURLResponse *urlResponse = (NSHTTPURLResponse*) response;
                    if(urlResponse.statusCode == 200) {
                        if([self parseConfig:[NSString stringWithUTF8String:[data bytes]] delegate:delegate]) {
                            [[NSUserDefaults standardUserDefaults] setObject:userString forKey:@"username_preference"];
                            [[NSUserDefaults standardUserDefaults] setObject:network.domain forKey:@"domain_preference"];
                            [[NSUserDefaults standardUserDefaults] setObject:passwordString forKey:@"password_preference"];
                            [[LinphoneManager instance] reconfigureLinphone];
                            if(![self downloadCertificates:delegate]) {
                                dispatch_async(dispatch_get_main_queue(), ^{
                                    [delegate buschJaegerConfigurationSuccess];
                                });
                            }
                        }
                    } else {
                        [self reset];
                        dispatch_async(dispatch_get_main_queue(), ^{
                            [delegate buschJaegerConfigurationError:[NSString stringWithFormat:@"Request not succeed (Status code:%d)", urlResponse.statusCode]];
                        });
                    }
                }
            });
            return TRUE;
        }
    }
    return FALSE;
}

- (BOOL)loadHistory:(BuschJaegerConfigurationRequestType)type delegate:(id<BuschJaegerConfigurationDelegate>)delegate {
    [history removeAllObjects];
    NSString *url = (type == BuschJaegerConfigurationRequestType_Local)? network.localHistory: network.globalHistory;
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:url] cachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData timeoutInterval:5];
    if(request != nil) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL), ^(void) {
            NSURLResponse *response = nil;
            NSError *error = nil;
            NSData *data  = nil;
            data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error delegate:self];
            if(data == nil) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [delegate buschJaegerConfigurationError:[error localizedDescription]];
                });
            } else {
                NSHTTPURLResponse *urlResponse = (NSHTTPURLResponse*) response;
                if(urlResponse.statusCode == 200) {
                    NSString *dataString = [[NSString alloc] initWithBytes:[data bytes] length:[data length] encoding: NSUTF8StringEncoding];
                    if([self parseHistory:dataString delegate:delegate]) {
                        dispatch_async(dispatch_get_main_queue(), ^{
                            [delegate buschJaegerConfigurationSuccess];
                        });
                    }
                    [dataString release];
                } else {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [delegate buschJaegerConfigurationError:[NSString stringWithFormat:@"Request not succeed (Status code:%d)", urlResponse.statusCode]];
                    });
                }
            }
        });
        return TRUE;
    }
    return FALSE;
}

- (BOOL)removeHistory:(BuschJaegerConfigurationRequestType)type history:(History*)ahistory delegate:(id<BuschJaegerConfigurationDelegate>)delegate {
    NSString *url = [NSString stringWithFormat:@"%@/adduser.cgi?type=delhistory&id=%d", [self getGateway:type], ahistory.ID];
    NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:url] cachePolicy:NSURLRequestReloadIgnoringLocalAndRemoteCacheData timeoutInterval:5];
    if(request != nil) {
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL), ^(void) {
            NSURLResponse *response = nil;
            NSError *error = nil;
            NSData *data  = nil;
            data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error delegate:self];
            if(data == nil) {
                dispatch_async(dispatch_get_main_queue(), ^{
                    [delegate buschJaegerConfigurationError:[error localizedDescription]];
                });
            } else {
                NSHTTPURLResponse *urlResponse = (NSHTTPURLResponse*) response;
                if(urlResponse.statusCode == 200) {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [delegate buschJaegerConfigurationSuccess];
                    });
                } else {
                    dispatch_async(dispatch_get_main_queue(), ^{
                        [delegate buschJaegerConfigurationError:[NSString stringWithFormat:@"Request not succeed (Status code:%d)", urlResponse.statusCode]];
                    });
                }
            }
        });
        return TRUE;
    }
    return FALSE;
}


- (NSString*)getGateway:(BuschJaegerConfigurationRequestType)type {
    NSString *gateway = nil;
    NSString *urlString = (type == BuschJaegerConfigurationRequestType_Local)? network.localHistory: network.globalHistory;
    NSURL *url = [NSURL URLWithString:urlString];
    NSRange range = [urlString rangeOfString:[url relativePath]];
    if(range.location != NSNotFound) {
        gateway = [urlString substringToIndex:range.location];
    }
    return gateway;
}

- (NSString*)getImageUrl:(BuschJaegerConfigurationRequestType)type image:(NSString *)image {
    return [NSString stringWithFormat:@"%@/%@", [self getGateway:type], image];
}

- (User*)getCurrentUser {
    NSString *username = [[NSUserDefaults standardUserDefaults] stringForKey:@"username_preference"];
    NSEnumerator *enumerator = [users objectEnumerator];
    
    User *usr;
    while ((usr = [enumerator nextObject])) {
        if([usr.name compare:username options:0] == 0) {
            return usr;
        }
    }
    return nil;
}


#pragma mark - NSURLConnectionDelegate Function

- (BOOL)connection:(NSURLConnection *)connection canAuthenticateAgainstProtectionSpace:(NSURLProtectionSpace *)protectionSpace {
    return [protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust];
}

- (void)connection:(NSURLConnection *)connection didReceiveAuthenticationChallenge:(NSURLAuthenticationChallenge *)challenge {
    if ([challenge.protectionSpace.authenticationMethod isEqualToString:NSURLAuthenticationMethodServerTrust]) {
        SecTrustRef trust = [challenge.protectionSpace serverTrust];
        NSArray *anchors = (NSArray*)certificates;
        SecTrustSetAnchorCertificates(trust, (CFArrayRef)anchors);
        SecTrustSetAnchorCertificatesOnly(trust, YES);

        SecTrustResultType result = kSecTrustResultInvalid;
        OSStatus sanityChesk = SecTrustEvaluate(trust, &result);
        
        if (sanityChesk == noErr) {
            if(result == kSecTrustResultConfirm || result == kSecTrustResultProceed || result == kSecTrustResultUnspecified) {
                [[challenge sender] useCredential:[NSURLCredential credentialForTrust:challenge.protectionSpace.serverTrust] forAuthenticationChallenge:challenge];
                return;
            } 
        }
        [challenge.sender continueWithoutCredentialForAuthenticationChallenge:challenge];
    } else {
        [challenge.sender continueWithoutCredentialForAuthenticationChallenge:challenge];
    }
}

@end
