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

+ (void)call:(id) object dict:(NSDictionary *) dict {
    for (NSString* identifier in dict) {
        if([identifier characterAtIndex:([identifier length] -1)] == ':') {
            NSArray *arguments = [dict objectForKey:identifier];
            SEL selector = NSSelectorFromString(identifier);
            NSMethodSignature *signature = [object methodSignatureForSelector:selector];
            NSInvocation *invocation = [NSInvocation invocationWithMethodSignature:signature];
            [invocation setTarget:object];
            [invocation setSelector:selector];
            for(int i=0; i<[arguments count]; i++)
            {
                id arg = [arguments objectAtIndex:i];
                [invocation setArgument:&arg atIndex:i+2]; // The first two arguments are the hidden arguments self and _cmd
            }
            [invocation invoke]; // Invoke the selector
        } else {
            NSDictionary *arguments = [dict objectForKey:identifier];
            id new_object = [object performSelector:NSSelectorFromString(identifier)];
            [AbstractCall call:new_object dict:arguments];
        }
    }
}

@end
