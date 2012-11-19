/* History.m
 *
 * Copyright (C) 2012  Belledonne Comunications, Grenoble, France
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

#import "History.h"
#import "Utils.h"

@implementation History

@synthesize ID;
@synthesize date;
@synthesize stationID;
@synthesize incoming;
@synthesize images;

- (void)dealloc {
    [date release];
    [images release];
    
    [super dealloc];
}

- (NSString*)write {
    return nil;
}

//2 20120425 1022 3 O 20120425-2022-1.jpg 20120425-2023-2.jpg 20120425-2024-3.jpg
+ (id)parse:(NSString*)line {
    History *history = nil;
    NSError *error;
    NSRegularExpression *regex = [NSRegularExpression
                                  regularExpressionWithPattern:@"([\\d]+) ([\\d]+ [\\d]+) ([\\d]+) (i|o)"
                                  options:NSRegularExpressionCaseInsensitive
                                  error:&error];
    
    NSTextCheckingResult* result = [regex firstMatchInString:line options:0 range:NSMakeRange(0, [line length])];
    if(result && result.numberOfRanges == 5) {
        history = [[[History alloc] init] autorelease];
        history.ID = [[line substringWithRange:[result rangeAtIndex:1]] intValue];
        
        NSDateFormatter *dateFormat = [[NSDateFormatter alloc] init];
        [dateFormat setDateFormat:@"yyyyMMdd HHmm"];
        NSString *stringDate = [line substringWithRange:[result rangeAtIndex:2]];
        history.date = [dateFormat dateFromString:stringDate];
        [dateFormat release];
        
        history.stationID = [[line substringWithRange:[result rangeAtIndex:3]] intValue];
        
        history.incoming = [[line substringWithRange:[result rangeAtIndex:4]] compare:@"i" options:NSCaseInsensitiveSearch] == 0;
        
        NSRange range = [result rangeAtIndex:0];
        range.location += range.length;
        range.length = [line length] - range.location;
        NSArray *array = [[line substringWithRange:range] componentsSeparatedByString:@" "];
        history.images = [array filteredArrayUsingPredicate:[NSPredicate predicateWithFormat:@"length > 0"]];
    } else {
        [LinphoneLogger log:LinphoneLoggerWarning format:@"Invalid history line: %@", line];
    }
    
    return history;
}

@end
