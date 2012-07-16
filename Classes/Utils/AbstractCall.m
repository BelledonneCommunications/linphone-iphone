/* AbstractCall.m
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

#import "AbstractCall.h"

@implementation AbstractCall

@synthesize functionName;
@synthesize functionArgs;

+ (id)abstractCall:(NSString *)name, ... {
    AbstractCall *object;
    va_list args;
    va_start(args, name);
    object = [[AbstractCall alloc] init:name args:args];
    va_end(args);
    return [object autorelease];
}

- (id)init:(NSString *)name, ... {
    AbstractCall *object;
    va_list args;
    va_start(args, name);
    object = [self init:name args:args];
    va_end(args);
    return object;
}

- (id)init:(NSString *)name args:(va_list)args {
    self = [super init];
    if(self != nil) {
        self->functionName = name;
        NSMutableArray *array = [[NSMutableArray alloc] init];
        int count = 0;
        for(int i = 0; i < [self->functionName length]; ++i) {
            if([self->functionName characterAtIndex:i] == ':') {
                ++count;
            }
        }
        for (int i = 0; i < count; ++i) {
            id arg = va_arg(args, id);
            [array addObject:arg];
        }
        self->functionArgs = array;
    }
    return self;
}

- (void)dealloc {
    [functionName release];
    [functionArgs release];
    
    [super dealloc];
}

- (void)call:(id)object {
    @try {
        SEL selector = NSSelectorFromString(functionName);
        NSMethodSignature *signature = [object methodSignatureForSelector:selector];
        NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
        [invocation setTarget:object];
        [invocation setSelector:selector];
        for(int i=0; i<[functionArgs count]; i++)
        {
            id arg = [functionArgs objectAtIndex:i];
            [invocation setArgument:&arg atIndex:i+2]; // The first two arguments are the hidden arguments self and _cmd
        }
        [invocation invoke]; // Invoke the selector
    } @catch (NSException *exception) {
        NSLog(@"Abstract Call: Can't call %@ with arguments %@ on %@", functionName, functionArgs, object);
    }
}

@end
