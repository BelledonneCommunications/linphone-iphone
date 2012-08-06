#import "XMLRPCResponse.h"
#import "XMLRPCEventBasedParser.h"

@implementation XMLRPCResponse

- (id)initWithData: (NSData *)data {
    if (!data) {
        return nil;
    }

    self = [super init];
    if (self) {
        XMLRPCEventBasedParser *parser = [[XMLRPCEventBasedParser alloc] initWithData: data];
        
        if (!parser) {
#if ! __has_feature(objc_arc)
            [self release];
#endif
            return nil;
        }
    
        myBody = [[NSString alloc] initWithData: data encoding: NSUTF8StringEncoding];
        myObject = [parser parse];
#if ! __has_feature(objc_arc)
        [myObject retain];
#endif
        
        isFault = [parser isFault];
        
#if ! __has_feature(objc_arc)
        [parser release];
#endif
    }
    
    return self;
}

#pragma mark -

- (BOOL)isFault {
    return isFault;
}

- (NSNumber *)faultCode {
    if (isFault) {
        return [myObject objectForKey: @"faultCode"];
    }
    
    return nil;
}

- (NSString *)faultString {
    if (isFault) {
        return [myObject objectForKey: @"faultString"];
    }
    
    return nil;
}

#pragma mark -

- (id)object {
    return myObject;
}

#pragma mark -

- (NSString *)body {
    return myBody;
}

#pragma mark -

- (NSString *)description {
	NSMutableString	*msg = [NSMutableString stringWithCapacity:128];
	[msg appendFormat:@"[body=%@", myBody];
	if (isFault) {
		[msg appendFormat:@", fault[%@]='%@'", [self faultCode], [self faultString]];
	} else {
		[msg appendFormat:@", obj=%@", myObject];
	}
	[msg appendString:@"]"];
	return msg;
}

#pragma mark -

- (void)dealloc {
#if ! __has_feature(objc_arc)
    [myBody release];
    [myObject release];
    
    [super dealloc];
#endif
}

@end
