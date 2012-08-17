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

@implementation OutdoorStation

@synthesize ID;
@synthesize name;
@synthesize address;
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

@end
