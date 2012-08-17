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

@implementation BuschJaegerConfigParser

+ (BOOL)parseQRCode:(NSString*)data delegate:(id<BuschJaegerConfigParser>)delegate{
    NSString *urlString;
    NSString *userString;
    NSString *passwordString;
    
    NSError  *error;
    NSRegularExpression *regex;
    
    {
        error = NULL;
        regex = [NSRegularExpression
                 regularExpressionWithPattern:@"URL=([^\\s]+)"
                 options:0
                 error:&error];
        
        NSTextCheckingResult* result = [regex firstMatchInString:data options:0 range:NSMakeRange(0, [data length])];
        if(result && result.numberOfRanges == 2) {
            NSRange range = [result rangeAtIndex:1];
            urlString = [data substringWithRange:range];
        }
    }
    
    {
        error = NULL;
        regex = [NSRegularExpression
                 regularExpressionWithPattern:@"USER=([^\\s]+)"
                 options:0
                 error:&error];
        
        NSTextCheckingResult* result = [regex firstMatchInString:data options:0 range:NSMakeRange(0, [data length])];
        if(result && result.numberOfRanges == 2) {
            NSRange range = [result rangeAtIndex:1];
            userString = [data substringWithRange:range];
        }
    }
    
    {
        error = NULL;
        regex = [NSRegularExpression
                 regularExpressionWithPattern:@"PW=([^\\s]+)"
                 options:0
                 error:&error];
        
        NSTextCheckingResult* result = [regex firstMatchInString:data options:0 range:NSMakeRange(0, [data length])];
        if(result && result.numberOfRanges == 2) {
            NSRange range = [result rangeAtIndex:1];
            passwordString = [data substringWithRange:range];
        }
    }
    if(urlString != nil && userString != nil && passwordString != nil) {
        NSURLRequest *request = [[NSURLRequest alloc] initWithURL:[NSURL URLWithString:urlString]];
        dispatch_async(dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, (unsigned long)NULL), ^(void) {
            NSURLResponse *response = nil;
            NSError *error = nil;
            NSData *data  = nil;
            data = [NSURLConnection sendSynchronousRequest:request returningResponse:&response error:&error];
            dispatch_async(dispatch_get_main_queue(), ^{
                if(data == nil) {
                    [delegate buschJaegerConfigParserError:[error localizedDescription]];
                } else {
                    NSString *dataString = [NSString stringWithUTF8String:[data bytes]];
                    [delegate buschJaegerConfigParserSuccessful];
                }
            });
        });
        return TRUE;
    } else {
        return FALSE;
    }
}

@end
