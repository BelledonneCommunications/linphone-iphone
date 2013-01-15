//
//  TUNinePatchCachingCategories.m
//  NinePatch
//
//  Copyright 2010 Tortuga 22, Inc. All rights reserved.
//

#import "TUNinePatchCachingCategories.h"

@implementation NSString (NinePatchCaching)

/**
 It's not clear we can't just use NSStringFromCGSize. This might get cut in a future revision.
 */
+(NSString *)ninePatchKeyStringForSize:(CGSize)size {
	return [NSString stringWithFormat:@"ninePatchKeyString.%#.0f.%#.0f",size.width,size.height];
}

@end

@implementation NSDictionary (NinePatchCaching)

/**
 Convenience method to make it a little less annoying to pull objects out of the caches keyed by their size.
 */
-(id)objectForSize:(CGSize)size {
	id object = nil;
	NSString *key = [NSString ninePatchKeyStringForSize:size];
	if (key) {
		object = [self objectForKey:key];
	}
	return object;
}

@end

@implementation NSMutableDictionary (NinePatchCaching)

/**
 Convenience method to make it a little less annoying to put objects in the caches keyed by their size.
 */
-(void)setObject:(id)object forSize:(CGSize)size {
	if (object) {
		NSString *key = [NSString ninePatchKeyStringForSize:size];
		if (key) {
			[self setObject:object 
					 forKey:key];
		}
	}
}

@end
