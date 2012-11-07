/* LevelPushButton.m
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

#import "LevelPushButton.h"
#import "BuschJaegerConfiguration.h"
#import "Utils.h"

@implementation LevelPushButton

@synthesize name;

- (void)dealloc {
    [name release];
    
    [super dealloc];
}

- (NSString*)write {
    NSMutableString *str = [NSMutableString string];
    [str appendString:[NSString stringWithFormat:@"\n[levelpushbutton]\n"]];
    [str appendString:[NSString stringWithFormat:@"name=%@\n", name]];
    return str;
}

+ (id)parse:(NSString*)section array:(NSArray*)array {
    NSString *gparam;
    LevelPushButton *net = nil;
    if((gparam = [BuschJaegerConfiguration getRegexValue:@"^\\[(levelpushbutton)\\]$" data:section]) != nil) {
        net = [[[LevelPushButton alloc] init] autorelease];
        NSString *param;
        for(NSString *entry in array) {
            if((param = [BuschJaegerConfiguration getRegexValue:@"^name=(.*)$" data:entry]) != nil) {
                net.name = param;
            } else if([[entry stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] length] != 0){
                [LinphoneLogger log:LinphoneLoggerWarning format:@"Unknown entry in %@ section: %@", section, entry];
            }
        }
    }
    
    return net;
}

@end
