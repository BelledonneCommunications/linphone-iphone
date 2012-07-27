//
//  TUNinePatchCache.m
//  NinePatch
//
//  Copyright 2010 Tortuga 22, Inc. All rights reserved.
//

#import "TUNinePatchCache.h"
#import "TUCachingNinePatch.h"
#import "TUNinePatch.h"

@interface TUNinePatchCache ()

@property(nonatomic, retain, readwrite) NSMutableDictionary *ninePatchCache;

@end


@implementation TUNinePatchCache

#pragma mark Synthesized Properties
@synthesize ninePatchCache = _ninePatchCache;

#pragma mark Init + Dealloc
-(id)init {
	if (self = [super init]) {
		self.ninePatchCache = [NSMutableDictionary dictionary];
	}
	return self;
}

#pragma mark -
+(id)shared {
	static TUNinePatchCache *shared;
	if (!shared) {
		shared = [[self alloc] init];
	}
	return shared;
}

#pragma mark -
-(void)dealloc {
	self.ninePatchCache = nil;
	[super dealloc];
}

#pragma mark Getting Ninepatches Directly
// Getting Ninepatches Directly
-(id < TUNinePatch >)ninePatchNamed:(NSString *)ninePatchName {
	TUCachingNinePatch *cachingNinePatch = [self cachingNinePatchNamed:ninePatchName];
	NPAssertNilOrIsKindOfClass(cachingNinePatch,TUCachingNinePatch);
	return (!cachingNinePatch)?(nil):([cachingNinePatch ninePatch]);
}


-(TUCachingNinePatch *)cachingNinePatchNamed:(NSString *)ninePatchName {
	TUCachingNinePatch *cachingNinePatch = [self cachedCachingNinePatchNamed:ninePatchName];
	NPAssertNilOrIsKindOfClass(cachingNinePatch,TUCachingNinePatch);
	if (!cachingNinePatch) {
		cachingNinePatch = [self constructCachingNinePatchNamed:ninePatchName];
		NPAssertNilOrIsKindOfClass(cachingNinePatch,TUCachingNinePatch);
		if (cachingNinePatch) {
			[self cacheCachingNinePatch:cachingNinePatch 
								  named:ninePatchName];
		}
	}
	return cachingNinePatch;
}

-(void)cacheCachingNinePatch:(TUCachingNinePatch *)cachingNinePatch named:(NSString *)ninePatchName {
	NPAssertPropertyNonNil(ninePatchCache);
	if (cachingNinePatch && ninePatchName) {
		[self.ninePatchCache setObject:cachingNinePatch 
								forKey:ninePatchName];
	}
}

-(TUCachingNinePatch *)cachedCachingNinePatchNamed:(NSString *)ninePatchName {
	return (!ninePatchName)?(nil):([self.ninePatchCache objectForKey:ninePatchName]);
}

-(TUCachingNinePatch *)constructCachingNinePatchNamed:(NSString *)ninePatchName {
	return (!ninePatchName)?(nil):([TUCachingNinePatch ninePatchCacheWithNinePatchNamed:ninePatchName]);
}

#pragma mark Getting Images Directly
-(UIImage *)imageOfSize:(CGSize)size forNinePatchNamed:(NSString *)ninePatchName {
	NPParameterAssertNotNilIsKindOfClass(ninePatchName,NSString);
	UIImage *image = nil;
	TUCachingNinePatch *cachingNinePatch = [self cachingNinePatchNamed:ninePatchName];
	if (cachingNinePatch) {
		image = [cachingNinePatch imageOfSize:size];
	}
	return image;
}

#pragma mark Getting Ninepatches - Convenience
+(TUCachingNinePatch *)cachingNinePatchNamed:(NSString *)ninePatchName {
	return [[self shared] ninePatchNamed:ninePatchName];
}

+(id < TUNinePatch >)ninePatchNamed:(NSString *)ninePatchName {
	TUCachingNinePatch *cachingNinePatch = [[self shared] cachingNinePatchNamed:ninePatchName];
	NPAssertNilOrIsKindOfClass(cachingNinePatch,TUCachingNinePatch);
	return (!cachingNinePatch)?(nil):([cachingNinePatch ninePatch]);
}

#pragma mark Getting Images - Convenience
+(UIImage *)imageOfSize:(CGSize)size forNinePatchNamed:(NSString *)ninePatchName {
	return [[self shared] imageOfSize:size 
					forNinePatchNamed:ninePatchName];
}

#pragma mark Cache Management - Direct
-(void)flushCache {
	[self.ninePatchCache removeAllObjects];
}
-(void)flushCacheForNinePatchNamed:(NSString *)name {
	if (name) {
		[self.ninePatchCache removeObjectForKey:name];
	}
}

#pragma mark Cache Management - Convenience
+(void)flushCache {
	[[self shared] flushCache];
}

+(void)flushCacheForNinePatchNamed:(NSString *)name {
	[[self shared] flushCacheForNinePatchNamed:name];
}

@end