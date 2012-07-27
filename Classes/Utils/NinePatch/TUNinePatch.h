//
//  TUNinePatch.h
//  NinePatch
//
//  Copyright 2009 Tortuga 22, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import "TUNinePatchProtocols.h"

/**
 Abstract base class for concrete NinePatches; this is the public interface into the TUNinePatch class cluster. Note particularly that TUNinePatch itself doesn't actually implement the TUNinePatch protocol, but its convenience methods promise to supply objects that do implement TUNinePatch. You should really only be using the classlevel convenience methods on this class unless you know what you're doing. If a method isn't documented it's probably not really for public use yet.
 */
@interface TUNinePatch : NSObject < NSCoding, NSCopying > {
	UIImage *_center;
	CGRect _contentRegion;
	BOOL _tileCenterVertically;
	BOOL _tileCenterHorizontally;
}

// Synthesized Properties
@property(nonatomic, retain, readonly) UIImage *center;
@property(nonatomic, assign, readonly) CGRect contentRegion;
@property(nonatomic, assign, readonly) BOOL tileCenterVertically;
@property(nonatomic, assign, readonly) BOOL tileCenterHorizontally;

// NSCoding
-(id)initWithCoder:(NSCoder *)coder;
-(void)encodeWithCoder:(NSCoder *)coder;

// NSCopying
-(id)copyWithZone:(NSZone *)zone;

// Init + Dealloc
-(id)initWithCenter:(UIImage *)center contentRegion:(CGRect)contentRegion;
-(id)initWithCenter:(UIImage *)center contentRegion:(CGRect)contentRegion tileCenterVertically:(BOOL)tileCenterVertically tileCenterHorizontally:(BOOL)tileCenterHorizontally;
-(void)dealloc;

// Convenience Constructors
/**
 This parses ninePatchImage and instantiates an instance of the appropriate TUNinePatch subclass.
 
 @param ninePatchImage A non-nil UIImage object containing the contents of a .9.png file (eg: it still contains the 1px border containing the scaling information).
 @returns An autoreleased object implementing the TUNinePatch protocol, loaded with the contents of ninePatchImage. Can be nil if errors encountered.
 */
+(id < TUNinePatch >)ninePatchWithNinePatchImage:(UIImage *)ninePatchImage;

/**
 Calls through to ninePatchWithImage:stretchableRegion:contentRegion:tileCenterVertically:tileCenterHorizontally with contentRegion=CGRectZero and NO on the tiling params. Will probably get made private or protected soon.
 */
+(id < TUNinePatch >)ninePatchWithImage:(UIImage *)image stretchableRegion:(CGRect)stretchableRegion;

/**
 Creates a NinePatch using the passed-in image and the passed-in scaling information. This method may go protected soon, leaving only the ninePatchWithNinePatchImage: as a public convenience (possibly with the addition of ninePatchNamed: method as well). The argument for goign protected or private is that if this library winds up expanding discontinuous stretchable regions (as is done on Android) then there would be a separate interface for passing in 4 stretchable regions, making the public interface cmoplicated and "multiple ways in".
 
 @param image A non-nil UIImage object that contains the displayable content. This is contents of .9.png file AFTER removing the 1px border.
 @param stretchableRegion Rect specifying the bounds of the central stretchable region.  In the .9.png this is specified on the left and top margins.
 @param contentRegion Rect specifying the bounds of the content region, EG the box into which associated content might fit. In the .9.png this is specified on the bottom and right margins.
 @param tileCenterVertically (Currently unsupported) is intended to specify whether or not the center scales by resizing or scales by tiling. Not fully supported at this time, only use if you know what you're doing.
 @param tileCenterHorizontally (Currently unsupported) is intended to specify whether or not the center scales by resizing or scales by tiling. Not fully supported at this time, only use if you know what you're doing.
 
 @returns An object implementing the TUNinePatch protocol. Can be nil if problems were encountered.
 */
+(id < TUNinePatch >)ninePatchWithImage:(UIImage *)image stretchableRegion:(CGRect)stretchableRegion contentRegion:(CGRect)contentRegion tileCenterVertically:(BOOL)tileCenterVertically tileCenterHorizontally:(BOOL)tileCenterHorizontally;

// Bundle Loading
/**
 Creates a ninepatch in two steps: it takes filename, and tries to load @"filename.9.png" from the main bundle. If that loads it then attempts to construct a NinePatch from that image file's contents. Differs from UIImage's analogous method in that no caching is done.
 
 @param filename A non-nil NSString containing the filename of the source image but NOT including ".9.png".
 @returns An object implementing the TUNinePatch protocol. Can be nil if problems encountered.
 */
+(id < TUNinePatch >)ninePatchNamed:(NSString *)filename;

// Nine Patch Image Manipulation - High Level
+(CGRect)rectFromHorizontalRange:(NSRange)horizontalRange verticalRange:(NSRange)verticalRange;
+(CGRect)stretchableRegionOfNinePatchImage:(UIImage *)ninePatchImage;
+(CGRect)contentRegionOfNinePatchImage:(UIImage *)ninePatchImage;
+(BOOL)shouldTileCenterHorizontallyForNinePatchImage:(UIImage *)ninePatchImage;
+(BOOL)shouldTileCenterVerticallyForNinePatchImage:(UIImage *)ninePatchImage;

// Drawing Utility
-(void)drawInRect:(CGRect)rect;

// Diagnostic Utilities
-(UIImage *)upperEdge;
-(UIImage *)lowerEdge;
-(UIImage *)leftEdge;
-(UIImage *)rightEdge;

-(UIImage *)upperLeftCorner;
-(UIImage *)lowerLeftCorner;
-(UIImage *)upperRightCorner;
-(UIImage *)lowerRightCorner;

// TUNinePatch Protocol Methods - Drawing
-(void)inContext:(CGContextRef)context drawAtPoint:(CGPoint)point forContentOfSize:(CGSize)contentSize;
-(void)inContext:(CGContextRef)context drawCenteredInRect:(CGRect)rect forContentOfSize:(CGSize)contentSize;
-(void)inContext:(CGContextRef)context drawInRect:(CGRect)rect;

// TUNinePatch Protocol Methods - Image Construction
-(UIImage *)imageOfSize:(CGSize)size;

// TUNinePatch Protocol Methods - Sizing
-(BOOL)stretchesHorizontally;
-(BOOL)stretchesVertically;
-(CGFloat)minimumWidth;
-(CGFloat)minimumHeight;
-(CGSize)minimumSize;
-(CGSize)sizeForContentOfSize:(CGSize)contentSize;
-(CGPoint)upperLeftCornerForContentWhenDrawnAtPoint:(CGPoint)point;

// TUNinePatch Protocol Methods - Geometry
-(CGFloat)leftEdgeWidth;
-(CGFloat)rightEdgeWidth;
-(CGFloat)upperEdgeHeight;
-(CGFloat)lowerEdgeHeight;

// Customized Description
-(NSString *)description;
-(NSString *)descriptionPostfix;

@end