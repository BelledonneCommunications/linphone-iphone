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
#import "BuschJaegerConfigParser.h"
#import "Utils.h"

@implementation OutdoorStation

@synthesize ID;
@synthesize name;
@synthesize address;
@synthesize type;
@synthesize screenshot;
@synthesize surveillance;

- (id)initWithId:(int)aID {
    self = [super init];
    if(self != nil) {
        self->ID = aID;
    }
    return self;
}

- (void)dealloc {
    self.name = nil;
    self.address = nil;
    [super dealloc];
}

+ (id)parse:(NSString*)section array:(NSArray*)array {
    NSString *param;
    OutdoorStation *os = nil;
    if((param = [BuschJaegerConfigParser getRegexValue:@"^\\[outdoorstation_([\\d]+)\\]$" data:section]) != nil) {
        os = [[OutdoorStation alloc] initWithId:[param intValue]];
        NSString *param;
        for(NSString *entry in array) {
            if((param = [BuschJaegerConfigParser getRegexValue:@"^address=(.*)$" data:entry]) != nil) {
                os.address = param;
            } else if((param = [BuschJaegerConfigParser getRegexValue:@"^name=(.*)$" data:entry]) != nil) {
                os.name = param;
            } else if((param = [BuschJaegerConfigParser getRegexValue:@"^type=(.*)$" data:entry]) != nil) {
                os.type = param;
            } else if((param = [BuschJaegerConfigParser getRegexValue:@"^screenshot=(.*)$" data:entry]) != nil) {
                os.screenshot = [param compare:@"yes" options:NSCaseInsensitiveSearch] || [param compare:@"true" options:NSCaseInsensitiveSearch];
            } else if((param = [BuschJaegerConfigParser getRegexValue:@"^surveillance=(.*)$" data:entry]) != nil) {
                os.surveillance = [param compare:@"yes" options:NSCaseInsensitiveSearch] || [param compare:@"true" options:NSCaseInsensitiveSearch];
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
