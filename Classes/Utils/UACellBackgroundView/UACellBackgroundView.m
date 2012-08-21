//
//  UACellBackgroundView.m
//  Ambiance
//
//  Created by Matt Coneybeare on 1/31/10.
//  Copyright 2010 Urban Apps LLC. All rights reserved.
//
//  Modified by Diorcet Yann on 07/12/12

#define kDefaultMargin           10

#import "UACellBackgroundView.h"

static void addRoundedRectToPath(CGContextRef context, CGRect rect, float ovalWidth,float ovalHeight);

@implementation UACellBackgroundView

@synthesize position;
@synthesize backgroundColor;
@synthesize borderColor;
@synthesize automaticPositioning;

- (void)initUACellBackgroundView {
    backgroundColor = nil;
    [self setBackgroundColor:[UIColor colorWithRed:0.02 green:0.549 blue:0.961 alpha:1.0]];
    borderColor = nil;
    [self setBorderColor:[UIColor grayColor]];
    automaticPositioning = TRUE;
}
     
- (id)init {
    self = [super init];
    if(self != nil) {
        [self initUACellBackgroundView];
    }
    return self; 
}

- (id)initWithCoder:(NSCoder *)aDecoder {
    self = [super initWithCoder:aDecoder];
    if(self != nil) {
        [self initUACellBackgroundView];
    }
    return self;
}

- (id)initWithFrame:(CGRect)frame {
    self = [super initWithFrame:frame];
    if(self != nil) {
        [self initUACellBackgroundView];
    }
    return self;
}

- (BOOL)isOpaque {
    return NO;
}
     
- (void)setBackgroundColor:(UIColor *)abackgroundColor {
    if(backgroundColor != nil) {
        [backgroundColor release];
    }
    backgroundColor = [[UIColor alloc] initWithCGColor:abackgroundColor.CGColor];
    [self setNeedsDisplay];
}

- (void)setBorderColor:(UIColor *)aborderColor {
    if(borderColor != nil) {
        [borderColor release];
    }
    
    borderColor = [[UIColor alloc] initWithCGColor:aborderColor.CGColor];
    [self setNeedsDisplay];
}

- (void)layoutSubviews {
    [super layoutSubviews];
    
    if(!automaticPositioning) 
        return;
    
    //
    // Auto found position
    //
    
    UIView *view = [self superview]; 
    // Find TableViewCell
    if(view != nil && ![view isKindOfClass:[UITableView class]]) view = [view superview];
    
    UIView *cellView = [self superview]; 
    // Find TableViewCell
    if(cellView != nil && ![cellView isKindOfClass:[UITableViewCell class]]) cellView = [cellView superview];
    
    if(view != nil && cellView != nil) {
        UITableViewCell *cell = (UITableViewCell*)cellView;
        UITableView *tableView = (UITableView*)view;
        
        if([tableView style] == UITableViewStyleGrouped) {
            NSIndexPath *path = [tableView indexPathForCell:cell];
            if(path) {
                int count = [tableView numberOfRowsInSection:[path section]];
                // Set Position for background view
                if([path row] == 0) {
                    if([path row] == (count - 1)) {
                        [self setPosition:UACellBackgroundViewPositionSingle];
                    } else {
                        [self setPosition:UACellBackgroundViewPositionTop];
                    }
                } else if([path row] == (count - 1)) {
                    [self setPosition:UACellBackgroundViewPositionBottom];
                } else {
                    [self setPosition:UACellBackgroundViewPositionMiddle];
                }
            }
        } else {
            [self setPosition:UACellBackgroundViewPositionMiddle];
        }
    }
    [self setNeedsDisplay];
}

- (void)drawRect:(CGRect)aRect {
    // Drawing code
    
    CGContextRef c = UIGraphicsGetCurrentContext();	
    
    int lineWidth = 1;
	
    CGRect rect = [self bounds];	
    CGFloat minx = CGRectGetMinX(rect), midx = CGRectGetMidX(rect), maxx = CGRectGetMaxX(rect);
    CGFloat miny = CGRectGetMinY(rect), midy = CGRectGetMidY(rect), maxy = CGRectGetMaxY(rect);
    miny -= 1;
	
    CGFloat locations[2] = { 0.0, 1.0 };
    CGColorSpaceRef myColorspace = CGColorGetColorSpace([[self backgroundColor] CGColor]);
    CGGradientRef myGradient = nil;
    const CGFloat *default_components = CGColorGetComponents([[self backgroundColor] CGColor]);
    CGFloat components[8] = {default_components[0], default_components[1], default_components[2], default_components[3], default_components[0] * 0.766f, default_components[1] * 0.766f, default_components[2] * 0.766f, default_components[3]};
    CGContextSetStrokeColorWithColor(c, [borderColor CGColor]);
    CGContextSetLineWidth(c, lineWidth);
    CGContextSetAllowsAntialiasing(c, YES);
    CGContextSetShouldAntialias(c, YES);
    
    if (position == UACellBackgroundViewPositionTop) {
		
        miny += 1;
		
        CGMutablePathRef path = CGPathCreateMutable();
        CGPathMoveToPoint(path, NULL, minx, maxy);
        CGPathAddArcToPoint(path, NULL, minx, miny, midx, miny, kDefaultMargin);
        CGPathAddArcToPoint(path, NULL, maxx, miny, maxx, maxy, kDefaultMargin);
        CGPathAddLineToPoint(path, NULL, maxx, maxy);
        CGPathAddLineToPoint(path, NULL, minx, maxy);
        CGPathCloseSubpath(path);
		
        // Fill and stroke the path
        CGContextSaveGState(c);
        CGContextAddPath(c, path);
        CGContextClip(c);
		
        myGradient = CGGradientCreateWithColorComponents(myColorspace, components, locations, 2);
        CGContextDrawLinearGradient(c, myGradient, CGPointMake(minx,miny), CGPointMake(minx,maxy), 0);
		
        CGContextAddPath(c, path);
        CGPathRelease(path);
        CGContextStrokePath(c);
        CGContextRestoreGState(c);		
        
    } else if (position == UACellBackgroundViewPositionBottom) {
        
        CGMutablePathRef path = CGPathCreateMutable();
        CGPathMoveToPoint(path, NULL, minx, miny);
        CGPathAddArcToPoint(path, NULL, minx, maxy, midx, maxy, kDefaultMargin);
        CGPathAddArcToPoint(path, NULL, maxx, maxy, maxx, miny, kDefaultMargin);
        CGPathAddLineToPoint(path, NULL, maxx, miny);
        CGPathAddLineToPoint(path, NULL, minx, miny);
        CGPathCloseSubpath(path);
		
        // Fill and stroke the path
        CGContextSaveGState(c);
        CGContextAddPath(c, path);
        CGContextClip(c);
		
        myGradient = CGGradientCreateWithColorComponents(myColorspace, components, locations, 2);
        CGContextDrawLinearGradient(c, myGradient, CGPointMake(minx,miny), CGPointMake(minx,maxy), 0);
		
        CGContextAddPath(c, path);
        CGPathRelease(path);
        CGContextStrokePath(c);
        CGContextRestoreGState(c);
        
		
    } else if (position == UACellBackgroundViewPositionMiddle) {
		
        CGMutablePathRef path = CGPathCreateMutable();
        CGPathMoveToPoint(path, NULL, minx, miny);
        CGPathAddLineToPoint(path, NULL, maxx, miny);
        CGPathAddLineToPoint(path, NULL, maxx, maxy);
        CGPathAddLineToPoint(path, NULL, minx, maxy);
        CGPathAddLineToPoint(path, NULL, minx, miny);
        CGPathCloseSubpath(path);
		
        // Fill and stroke the path
        CGContextSaveGState(c);
        CGContextAddPath(c, path);
        CGContextClip(c);
		
        myGradient = CGGradientCreateWithColorComponents(myColorspace, components, locations, 2);
        CGContextDrawLinearGradient(c, myGradient, CGPointMake(minx,miny), CGPointMake(minx,maxy), 0);
		
        CGContextAddPath(c, path);
        CGPathRelease(path);
        CGContextStrokePath(c);
        CGContextRestoreGState(c);
		
    } else if (position == UACellBackgroundViewPositionSingle) {
        miny += 1;
		
        CGMutablePathRef path = CGPathCreateMutable();
        CGPathMoveToPoint(path, NULL, minx, midy);
        CGPathAddArcToPoint(path, NULL, minx, miny, midx, miny, kDefaultMargin);
        CGPathAddArcToPoint(path, NULL, maxx, miny, maxx, midy, kDefaultMargin);
        CGPathAddArcToPoint(path, NULL, maxx, maxy, midx, maxy, kDefaultMargin);
        CGPathAddArcToPoint(path, NULL, minx, maxy, minx, midy, kDefaultMargin);
        CGPathCloseSubpath(path);
		
		
        // Fill and stroke the path
        CGContextSaveGState(c);
        CGContextAddPath(c, path);
        CGContextClip(c);
		
		
        myGradient = CGGradientCreateWithColorComponents(myColorspace, components, locations, 2);
        CGContextDrawLinearGradient(c, myGradient, CGPointMake(minx,miny), CGPointMake(minx,maxy), 0);
		
        CGContextAddPath(c, path);
        CGPathRelease(path);
        CGContextStrokePath(c);
        CGContextRestoreGState(c);	
    }
	
    CGColorSpaceRelease(myColorspace);
    CGGradientRelease(myGradient);
    return;
}

- (void)dealloc {
    [backgroundColor release];
    [super dealloc];
}

- (void)setPosition:(UACellBackgroundViewPosition)newPosition {
    if (position != newPosition) {
        position = newPosition;
        [self setNeedsDisplay];
    }
}

@end

static void addRoundedRectToPath(CGContextRef context, CGRect rect, float ovalWidth,float ovalHeight) {
    float fw, fh;
	
    if (ovalWidth == 0 || ovalHeight == 0) {// 1
        CGContextAddRect(context, rect);
        return;
    }
	
    CGContextSaveGState(context);// 2
	
    CGContextTranslateCTM (context, CGRectGetMinX(rect),// 3
                           CGRectGetMinY(rect));
    CGContextScaleCTM (context, ovalWidth, ovalHeight);// 4
    fw = CGRectGetWidth (rect) / ovalWidth;// 5
    fh = CGRectGetHeight (rect) / ovalHeight;// 6
	
    CGContextMoveToPoint(context, fw, fh/2); // 7
    CGContextAddArcToPoint(context, fw, fh, fw/2, fh, 1);// 8
    CGContextAddArcToPoint(context, 0, fh, 0, fh/2, 1);// 9
    CGContextAddArcToPoint(context, 0, 0, fw/2, 0, 1);// 10
    CGContextAddArcToPoint(context, fw, 0, fw, fh/2, 1); // 11
    CGContextClosePath(context);// 12
	
    CGContextRestoreGState(context);// 13
}
