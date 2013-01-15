//
//  TUNinePatch.h
//  NinePatch
//
//  Copyright 2009 Tortuga 22, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>
#import <CoreGraphics/CoreGraphics.h>

/**
 
 Defines the methods shared by all concrete NinePatch classes. Expect many of these methods to be removed from the 
 protocol as the library is improved.
 
 */
@protocol TUNinePatch < NSObject, NSCoding, NSCopying >

// TUNinePatch Protocol Methods - Drawing
/**
 This draws the NinePatch with its upper-left corner at a specified location in a specified context, scaled to fit content of the size. This is a good method to use when drawing NinePatches that use the content bounds. This method will probably be improved in the future to return the upper-left corner for the content to be drawn.
 
 @param context A non-nil CGContextRef into which NinePatch will be drawn.
 @param point The upper left corner the NinePatch will be drawn at.
 @param size The size of the content the NinePatch will be sized to contain.
 */
-(void)inContext:(CGContextRef)context drawAtPoint:(CGPoint)point forContentOfSize:(CGSize)size;

/**
 This draws the NinePatch centered (horizontally and vertically) inside the containmentRect in the specified context, sized to fit content of the passed-in size. This is essentially a convenience method wrapping inContext:drawAtPoint:forContentOfSize:. Like its cousin it'll probably be modified to return information about where the content should be drawn. Will let the NinePatch overflow the containmentRect if the necessary size is sufficiently large; it'll be centered, just too big.

 @param context A non-nil CGContextRef into which NinePatch will be drawn.
 @param containmentRect the rect in which the NinePatch will be centered.
 @param size The size of the content the NinePatch will be sized to contain.
 */
-(void)inContext:(CGContextRef)context drawCenteredInRect:(CGRect)containmentRect forContentOfSize:(CGSize)size;

/** 
 This method draws the NinePatch into the passed-in context filling the passed-in rect. In all current implementations of TUNinePatch the other drawing utilities eventually call through this method to do their actual drawing.
 
 @param context A non-nil CGContextRef into which NinePatch will be drawn.
 @param rect The rect into which the NinePatch will be drawn. NinePatch is scaled to fill the rect.
 */
-(void)inContext:(CGContextRef)context drawInRect:(CGRect)rect;

// TUNinePatch Protocol Methods - Image Construction
/**
 Renders the NinePatch into an image of the requested size. Implementations vary in their memory management here -- to guarantee the returned image hangs around you should retain it.
 
 @param size The size the output UIImage should be.
 @returns A UIImage rendering of the NinePatch scaled like the passed-in size.
 */
-(UIImage *)imageOfSize:(CGSize)size;

// TUNinePatch Protocol Methods - Sizing
/**
 Returns YES if the NinePatch scales horizontally, NO otherwise. May be removed from protocol in future.
 */
-(BOOL)stretchesHorizontally;
/**
 Returns YES if the NinePatch scales vertically, NO otherwise. May be removed from protocol in future.
 */
-(BOOL)stretchesVertically;

/**
 Returns smallest horizontal size you scan scale NinePatch down to. This is usually equal to the leftEdge + the rightEdge (sending central column width -> 0). May be removed from protocol in future.
 
 @returns minimumWidth the smallest width NinePatch will render at.
 */
-(CGFloat)minimumWidth;

/**
 Returns smallest vertical size you scan scale NinePatch down to. This is usually equal to the topEdge + bottomEdge (sending central row height -> 0). May be removed from protocol in future.
 
 @returns minimumHeight the smallest height NinePatch will render at.
 */
-(CGFloat)minimumHeight;

/**
 Returns CGSize created from minimumWidth and minimumHeight in the obvious way.
 */
-(CGSize)minimumSize;

/**
 This is used for layout. Recall that NinePatch allows you to specify a "content region" (into which the content must fit) as a way of standardizing the amount of padding you do when using a particular NinePatch. This method returns the size the NinePatch needs to be drawn at to correctly accommodate content of the passed-in size. Note that for NinePatches that don't specify anything wrt content size this is the identity function.
 
 @param contentSize The size of the content you're using the NinePatch to display.
 @returns The size you need to draw the NinePatch at to accommodate the content.
 */
-(CGSize)sizeForContentOfSize:(CGSize)contentSize;

/**
 This is used for layout. This is basically a convenience function to calculate the offset for where "content" starts. It's the identity function when the NinePatch doesn't specify anything for the content region.
 
 @param point The point at which you're planning to place the upper left corner of the NinePatch when you draw it.
 @returns The point at which you need to place the upper left corner of the content you're going to draw.
 */
-(CGPoint)upperLeftCornerForContentWhenDrawnAtPoint:(CGPoint)point;

// TUNinePatch Protocol Methods - Geometry
/**
 The width of the left column. This geometric property lookup is probably going to get removed from the protocol.
 */
-(CGFloat)leftEdgeWidth;

/**
 The width of the right column. This geometric property lookup is probably going to get removed from the protocol.
 */
-(CGFloat)rightEdgeWidth;

/**
 The width of the upper row. This geometric property lookup is probably going to get removed from the protocol.
 */
-(CGFloat)upperEdgeHeight;

/**
 The width of the lower column. This geometric property lookup is probably going to get removed from the protocol.
 */
-(CGFloat)lowerEdgeHeight;

@end