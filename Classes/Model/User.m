/* User.m
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

#import "User.h"
#import "BuschJaegerConfiguration.h"
#import "Utils.h"

@implementation User

@synthesize ID;
@synthesize name;
@synthesize opendoor;
@synthesize surveillance;
@synthesize switchlight;
@synthesize switching;

/*
 user=miaou
 opendoor=yes
 surveillance=yes
 switchlight=yes
 switching=yes
 */

- (void)dealloc {
    [name release];
    
    [super dealloc];
}

- (NSString*)write {
    NSMutableString *str = [NSMutableString string];
    [str appendString:[NSString stringWithFormat:@"\n[user_%i]\n", ID]];
    [str appendString:[NSString stringWithFormat:@"user=%@\n", name]];
    [str appendString:[NSString stringWithFormat:@"opendoor=%@\n", opendoor?@"yes":@"no"]];
    [str appendString:[NSString stringWithFormat:@"surveillance=%@\n", surveillance?@"yes":@"no"]];
    [str appendString:[NSString stringWithFormat:@"switchlight=%@\n", switchlight?@"yes":@"no"]];
    [str appendString:[NSString stringWithFormat:@"switching=%@\n", switching?@"yes":@"no"]];
    return str;
}

+ (id)parse:(NSString*)section array:(NSArray*)array {
    NSString *param;
    User *usr = nil;
    if((param = [BuschJaegerConfiguration getRegexValue:@"^\\[user_([\\d]+)\\]$" data:section]) != nil) {
        usr = [[[User alloc] init] autorelease];
        usr.ID = [param intValue];
        NSString *param;
        for(NSString *entry in array) {
            if((param = [BuschJaegerConfiguration getRegexValue:@"^user=(.*)$" data:entry]) != nil) {
                usr.name = param;
            } else if((param = [BuschJaegerConfiguration getRegexValue:@"^opendoor=(.*)$" data:entry]) != nil) {
                usr.opendoor = [param compare:@"yes" options:NSCaseInsensitiveSearch] == 0 || [param compare:@"true" options:NSCaseInsensitiveSearch] == 0;
            } else if((param = [BuschJaegerConfiguration getRegexValue:@"^surveillance=(.*)$" data:entry]) != nil) {
                usr.surveillance = [param compare:@"yes" options:NSCaseInsensitiveSearch] == 0 || [param compare:@"true" options:NSCaseInsensitiveSearch] == 0;
            } else if((param = [BuschJaegerConfiguration getRegexValue:@"^switchlight=(.*)$" data:entry]) != nil) {
                usr.switchlight = [param compare:@"yes" options:NSCaseInsensitiveSearch] == 0 || [param compare:@"true" options:NSCaseInsensitiveSearch] == 0;
            } else if((param = [BuschJaegerConfiguration getRegexValue:@"^switching=(.*)$" data:entry]) != nil) {
                usr.switching = [param compare:@"yes" options:NSCaseInsensitiveSearch] == 0 || [param compare:@"true" options:NSCaseInsensitiveSearch] == 0;
            } else if([[entry stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] length] != 0){
                [LinphoneLogger log:LinphoneLoggerWarning format:@"Unknown entry in %@ section: %@", section, entry];
            }
        }
    }
    
    return usr;
}
@end
