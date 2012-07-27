//
//  TUNinePatchCachingCategories.h
//  NinePatch
//
//  Copyright 2010 Tortuga 22, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>

@interface NSString (NinePatchCaching)

+(NSString *)ninePatchKeyStringForSize:(CGSize)size;

@end

@interface NSDictionary (NinePatchCaching)

-(id)objectForSize:(CGSize)size;

@end

@interface NSMutableDictionary (NinePatchCaching)

-(void)setObject:(id)object forSize:(CGSize)size;

@end
