//
//  TUHorizontalNinePatch.h
//  NinePatch
//
//  Copyright 2009 Tortuga 22, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>

#import "TUNinePatch.h"
#import "TUNinePatchProtocols.h"

/**
 Concrete TUNinePatch instance. Handles NinePatches that stretch horizontally but not vertically. Only instantiate directly if you know what you're doing.
 */
@interface TUHorizontalNinePatch : TUNinePatch  < TUNinePatch > {
	UIImage *_leftEdge;
	UIImage *_rightEdge;
}

// Synthesized Properties
@property(nonatomic, retain, readonly) UIImage *leftEdge;
@property(nonatomic, retain, readonly) UIImage *rightEdge;

// Init + Dealloc
-(id)initWithCenter:(UIImage *)center contentRegion:(CGRect)contentRegion tileCenterVertically:(BOOL)tileCenterVertically tileCenterHorizontally:(BOOL)tileCenterHorizontally leftEdge:(UIImage *)leftEdge rightEdge:(UIImage *)rightEdge;
-(void)dealloc;

// TUNinePatch Overrides
-(void)drawInRect:(CGRect)rect;
-(BOOL)stretchesVertically;
-(CGSize)sizeForContentOfSize:(CGSize)contentSize;
-(CGFloat)leftEdgeWidth;
-(CGFloat)rightEdgeWidth;

@end
