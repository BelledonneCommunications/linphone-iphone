/* OutdoorStation.m
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

#import "OutdoorStation.h"
#import "BuschJaegerConfiguration.h"
#import "Utils.h"

@implementation OutdoorStation

@synthesize ID;
@synthesize name;
@synthesize address;
@synthesize type;
@synthesize screenshot;
@synthesize surveillance;

- (void)dealloc {
    self.name = nil;
    self.address = nil;
    [super dealloc];
}

+ (id)parse:(NSString*)section array:(NSArray*)array {
    NSString *param;
    OutdoorStation *os = nil;
    if((param = [BuschJaegerConfiguration getRegexValue:@"^\\[outdoorstation_([\\d]+)\\]$" data:section]) != nil) {
        os = [[[OutdoorStation alloc] init] autorelease];
        os.ID = [param intValue];
        NSString *param;
        for(NSString *entry in array) {
            if((param = [BuschJaegerConfiguration getRegexValue:@"^address=(.*)$" data:entry]) != nil) {
                os.address = param;
            } else if((param = [BuschJaegerConfiguration getRegexValue:@"^name=(.*)$" data:entry]) != nil) {
                os.name = param;
            } else if((param = [BuschJaegerConfiguration getRegexValue:@"^type=(.*)$" data:entry]) != nil) {
                os.type = param;
            } else if((param = [BuschJaegerConfiguration getRegexValue:@"^screenshot=(.*)$" data:entry]) != nil) {
                os.screenshot = [param compare:@"yes" options:NSCaseInsensitiveSearch] == 0 || [param compare:@"true" options:NSCaseInsensitiveSearch] == 0;
            } else if((param = [BuschJaegerConfiguration getRegexValue:@"^surveillance=(.*)$" data:entry]) != nil) {
                os.surveillance = [param compare:@"yes" options:NSCaseInsensitiveSearch] == 0 || [param compare:@"true" options:NSCaseInsensitiveSearch] == 0;
            } else if([[entry stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceCharacterSet]] length] != 0){
                [LinphoneLogger log:LinphoneLoggerWarning format:@"Unknown entry in %@ section: %@", section, entry];
            }
        }
    }

    return os;
}

- (NSString*)write {
    NSMutableString *str = [NSMutableString string];
    [str appendString:[NSString stringWithFormat:@"\n[outdoorstation_%i]\n", ID]];
    [str appendString:[NSString stringWithFormat:@"address=%@\n", address]];
    [str appendString:[NSString stringWithFormat:@"name=%@\n", name]];
    [str appendString:[NSString stringWithFormat:@"type=%@\n", type]];
    [str appendString:[NSString stringWithFormat:@"screenshot=%@\n", screenshot?@"yes":@"no"]];
    [str appendString:[NSString stringWithFormat:@"surveillance=%@\n", surveillance?@"yes":@"no"]];
    return str;
}

@end
