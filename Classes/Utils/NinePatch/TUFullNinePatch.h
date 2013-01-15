//
//  TUFullNinePatch.h
//  NinePatch
//
//  Copyright 2009 Tortuga 22, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>

#import "TUNinePatch.h"
#import "TUNinePatchProtocols.h"

/**
 Concrete TUNinePatch instance. Handles NinePatches that stretch horizontally and vertically. Only instantiate directly if you know what you're doing.
 */
@interface TUFullNinePatch : TUNinePatch < TUNinePatch > {
	UIImage *_upperEdge;
	UIImage *_lowerEdge;
	UIImage *_leftEdge;
	UIImage *_rightEdge;
	UIImage *_upperLeftCorner;
	UIImage *_lowerLeftCorner;
	UIImage *_upperRightCorner;
	UIImage *_lowerRightCorner;
}

// Synthesized Properties
@property(nonatomic, retain, readonly) UIImage *upperEdge;
@property(nonatomic, retain, readonly) UIImage *lowerEdge;
@property(nonatomic, retain, readonly) UIImage *leftEdge;
@property(nonatomic, retain, readonly) UIImage *rightEdge;

@property(nonatomic, retain, readonly) UIImage *upperLeftCorner;
@property(nonatomic, retain, readonly) UIImage *lowerLeftCorner;
@property(nonatomic, retain, readonly) UIImage *upperRightCorner;
@property(nonatomic, retain, readonly) UIImage *lowerRightCorner;

// Init + Dealloc
-(id)initWithCenter:(UIImage *)center contentRegion:(CGRect)contentRegion tileCenterVertically:(BOOL)tileCenterVertically tileCenterHorizontally:(BOOL)tileCenterHorizontally upperLeftCorner:(UIImage *)upperLeftCorner upperRightCorner:(UIImage *)upperRightCorner lowerLeftCorner:(UIImage *)lowerLeftCorner lowerRightCorner:(UIImage *)lowerRightCorner leftEdge:(UIImage *)leftEdge rightEdge:(UIImage *)rightEdge upperEdge:(UIImage *)upperEdge lowerEdge:(UIImage *)lowerEdge;
-(void)dealloc;

// Sanity-Checking Tools
-(BOOL)checkSizeSanityAgainstOriginalImage:(UIImage *)originalImage;

// TUNinePatch Overrides
-(void)drawInRect:(CGRect)rect;
-(CGFloat)leftEdgeWidth;
-(CGFloat)rightEdgeWidth;
-(CGFloat)upperEdgeHeight;
-(CGFloat)lowerEdgeHeight;

// Image Logging
-(void)logExplodedImage;

@end