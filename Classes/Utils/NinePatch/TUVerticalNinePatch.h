//
//  TUVerticalNinePatch.h
//  NinePatch
//
//  Copyright 2009 Tortuga 22, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>
#import "TUNinePatch.h"
#import "TUNinePatchProtocols.h"

/**
 Concrete TUNinePatch instance. Handles NinePatches that only stretch vertically. Only instantiate directly if you know what you're doing.
 */
@interface TUVerticalNinePatch : TUNinePatch < TUNinePatch > {
	UIImage *_upperEdge;
	UIImage *_lowerEdge;
}

// Synthesized Properties
@property(nonatomic, retain, readonly) UIImage *upperEdge;
@property(nonatomic, retain, readonly) UIImage *lowerEdge;

// Init + Dealloc
-(id)initWithCenter:(UIImage *)center contentRegion:(CGRect)contentRegion tileCenterVertically:(BOOL)tileCenterVertically tileCenterHorizontally:(BOOL)tileCenterHorizontally upperEdge:(UIImage *)upperEdge lowerEdge:(UIImage *)lowerEdge;
-(void)dealloc;

// TUNinePatch Overrides
-(void)drawInRect:(CGRect)rect;
-(BOOL)stretchesHorizontally;
-(CGSize)sizeForContentOfSize:(CGSize)contentSize;
-(CGFloat)upperEdgeHeight;
-(CGFloat)lowerEdgeHeight;

@end