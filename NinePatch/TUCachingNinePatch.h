//
//  TUCachingNinePatch.h
//  NinePatch
//
//  Copyright 2010 Tortuga 22, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import "TUNinePatchProtocols.h"

@interface TUCachingNinePatch : NSObject < NSCoding, NSCopying > {
	id < TUNinePatch > _ninePatch;
	NSMutableDictionary *_ninePatchImageCache;
}

// Synthesized Properties
@property(nonatomic, retain, readonly) id < TUNinePatch > ninePatch;
@property(nonatomic, retain, readonly) NSMutableDictionary *ninePatchImageCache;

// Init + Dealloc
-(id)initWithNinePatchNamed:(NSString *)ninePatchName;
-(id)initWithNinePatch:(id < TUNinePatch >)ninePatch;
+(id)ninePatchCacheWithNinePatchNamed:(NSString *)ninePatchName;
+(id)ninePatchCacheWithNinePatch:(id < TUNinePatch >)ninePatch;
-(void)dealloc;

// NSCoding
-(id)initWithCoder:(NSCoder *)aDecoder;
-(void)encodeWithCoder:(NSCoder *)anEncoder;

// NSCopying
-(id)copyWithZone:(NSZone *)zone;

// Nib
-(void)awakeFromNib;

// Cache Management
-(void)flushCachedImages;

// Image Access - Utility Accessors
-(UIImage *)imageOfSize:(CGSize)size;

// Cache Access
-(void)cacheImage:(UIImage *)image ofSize:(CGSize)size;
-(UIImage *)cachedImageOfSize:(CGSize)size;

// Image Construction
-(UIImage *)constructImageOfSize:(CGSize)size;

@end
