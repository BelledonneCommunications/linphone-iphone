//
//  TUFullNinePatch.m
//  NinePatch
//
//  Copyright 2009 Tortuga 22, Inc. All rights reserved.
//

#import "TUFullNinePatch.h"
#import "TUNinePatchProtocols.h"
#import "UIImage-TUNinePatch.h"

@interface TUFullNinePatch  ()

// Synthesized Properties
@property(nonatomic, retain, readwrite) UIImage *upperEdge;
@property(nonatomic, retain, readwrite) UIImage *lowerEdge;
@property(nonatomic, retain, readwrite) UIImage *leftEdge;
@property(nonatomic, retain, readwrite) UIImage *rightEdge;

@property(nonatomic, retain, readwrite) UIImage *upperLeftCorner;
@property(nonatomic, retain, readwrite) UIImage *lowerLeftCorner;
@property(nonatomic, retain, readwrite) UIImage *upperRightCorner;
@property(nonatomic, retain, readwrite) UIImage *lowerRightCorner;

@end


@implementation TUFullNinePatch

#pragma mark Synthesized Properties
@synthesize upperEdge = _upperEdge;
@synthesize lowerEdge = _lowerEdge;
@synthesize leftEdge = _leftEdge;
@synthesize rightEdge = _rightEdge;

@synthesize upperLeftCorner = _upperLeftCorner;
@synthesize lowerLeftCorner = _lowerLeftCorner;
@synthesize upperRightCorner = _upperRightCorner;
@synthesize lowerRightCorner = _lowerRightCorner;

#pragma mark NSCoding
-(id)initWithCoder:(NSCoder *)coder {
	if (self = [super initWithCoder:coder]) {
		self.upperEdge = (UIImage *) [coder decodeObjectForKey:@"upperEdge"];
		self.lowerEdge = (UIImage *) [coder decodeObjectForKey:@"lowerEdge"];
		self.leftEdge = (UIImage *) [coder decodeObjectForKey:@"leftEdge"];
		self.rightEdge = (UIImage *) [coder decodeObjectForKey:@"rightEdge"];
		self.upperLeftCorner = (UIImage *) [coder decodeObjectForKey:@"upperLeftCorner"];
		self.lowerLeftCorner = (UIImage *) [coder decodeObjectForKey:@"lowerLeftCorner"];
		self.upperRightCorner = (UIImage *) [coder decodeObjectForKey:@"upperRightCorner"];
		self.lowerRightCorner = (UIImage *) [coder decodeObjectForKey:@"lowerRightCorner"];
	}
	return self;
}

-(void)encodeWithCoder:(NSCoder *)coder {
	[super encodeWithCoder:coder];
	
	[coder encodeObject:self.upperEdge 
				 forKey:@"upperEdge"];
	
	[coder encodeObject:self.lowerEdge 
				 forKey:@"lowerEdge"];
	
	[coder encodeObject:self.leftEdge 
				 forKey:@"leftEdge"];
	
	[coder encodeObject:self.rightEdge 
				 forKey:@"rightEdge"];
	
	[coder encodeObject:self.upperLeftCorner 
				 forKey:@"upperLeftCorner"];
	
	[coder encodeObject:self.lowerLeftCorner 
				 forKey:@"lowerLeftCorner"];
	
	[coder encodeObject:self.upperRightCorner 
				 forKey:@"upperRightCorner"];

	[coder encodeObject:self.lowerRightCorner 
				 forKey:@"lowerRightCorner"];
}

#pragma mark NSCopying
-(id)copyWithZone:(NSZone *)zone {
	return [[[self class] allocWithZone:zone] initWithCenter:self.center 
											   contentRegion:self.contentRegion 
										tileCenterVertically:self.tileCenterVertically 
									  tileCenterHorizontally:self.tileCenterHorizontally 
											 upperLeftCorner:self.upperLeftCorner 
											upperRightCorner:self.upperRightCorner 
											 lowerLeftCorner:self.lowerLeftCorner 
											lowerRightCorner:self.lowerRightCorner 
													leftEdge:self.leftEdge 
												   rightEdge:self.rightEdge 
												   upperEdge:self.upperEdge 
												   lowerEdge:self.lowerEdge];
}

#pragma mark Init + Dealloc
-(id)initWithCenter:(UIImage *)center contentRegion:(CGRect)contentRegion tileCenterVertically:(BOOL)tileCenterVertically tileCenterHorizontally:(BOOL)tileCenterHorizontally upperLeftCorner:(UIImage *)upperLeftCorner upperRightCorner:(UIImage *)upperRightCorner lowerLeftCorner:(UIImage *)lowerLeftCorner lowerRightCorner:(UIImage *)lowerRightCorner leftEdge:(UIImage *)leftEdge rightEdge:(UIImage *)rightEdge upperEdge:(UIImage *)upperEdge lowerEdge:(UIImage *)lowerEdge {
	NPParameterAssertNotNilIsKindOfClass(upperLeftCorner,UIImage);
	NPParameterAssertNotNilIsKindOfClass(lowerLeftCorner,UIImage);
	NPParameterAssertNotNilIsKindOfClass(upperRightCorner,UIImage);
	NPParameterAssertNotNilIsKindOfClass(lowerRightCorner,UIImage);
	NPParameterAssertNotNilIsKindOfClass(leftEdge,UIImage);
	NPParameterAssertNotNilIsKindOfClass(rightEdge,UIImage);
	NPParameterAssertNotNilIsKindOfClass(upperEdge,UIImage);
	NPParameterAssertNotNilIsKindOfClass(lowerEdge,UIImage);
	if (self = [super initWithCenter:center 
					   contentRegion:contentRegion 
				tileCenterVertically:tileCenterVertically 
			  tileCenterHorizontally:tileCenterHorizontally]) {
		self.upperEdge = upperEdge;
		self.lowerEdge = lowerEdge;
		self.leftEdge = leftEdge;
		self.rightEdge = rightEdge;
		self.upperLeftCorner = upperLeftCorner;
		self.lowerLeftCorner = lowerLeftCorner;
		self.upperRightCorner = upperRightCorner;
		self.lowerRightCorner = lowerRightCorner;
	}
	return self;
}

#pragma mark
-(void)dealloc {
	self.upperEdge = nil;
	self.lowerEdge = nil;
	self.leftEdge = nil;
	self.rightEdge = nil;
	self.upperLeftCorner = nil;
	self.lowerLeftCorner = nil;
	self.upperRightCorner = nil;
	self.lowerRightCorner = nil;
	[super dealloc];
}

#pragma mark Sanity-Checking Tools
-(BOOL)checkSizeSanityAgainstOriginalImage:(UIImage *)originalImage {
	CGSize os = [originalImage size];
	CGFloat ow = os.width;
	CGFloat oh = os.height;

	CGFloat tr = ([[self upperEdge] size].width + [[self upperLeftCorner] size].width + [[self upperRightCorner] size].width);
	CGFloat mr = ([[self center] size].width + [[self leftEdge] size].width + [[self rightEdge] size].width);
	CGFloat lr = ([[self lowerEdge] size].width + [[self lowerLeftCorner] size].width + [[self lowerRightCorner] size].width);
	
	
	BOOL topRow = TUWithinEpsilon(1.0f,ow,tr);
	BOOL midRow = TUWithinEpsilon(1.0f,ow,mr);
	BOOL lowRow = TUWithinEpsilon(1.0f,ow,lr);

	CGFloat lc = ([[self upperLeftCorner] size].height + [[self lowerLeftCorner] size].height + [[self leftEdge] size].height);
	CGFloat mc = ([[self center] size].height + [[self upperEdge] size].height + [[self lowerEdge] size].height);
	CGFloat rc = ([[self upperRightCorner] size].height + [[self lowerRightCorner] size].height + [[self rightEdge] size].height);

	BOOL lCol = TUWithinEpsilon(1.0f,oh,lc);
	BOOL mCol = TUWithinEpsilon(1.0f,oh,mc);
	BOOL rCol = TUWithinEpsilon(1.0f,oh,rc);	
	
	BOOL sizesMatch = TUForceYesOrNo(midRow && topRow && lowRow && mCol && lCol && rCol);
	DLog(@"SANITY sizesMatch: '%@.'",TUYesOrNoString(sizesMatch));
	DLog(@"SANITY topRow: '%@', midRow:'%@, lowRow:'%@'.", TUYesOrNoString(topRow),TUYesOrNoString(midRow),TUYesOrNoString(lowRow));
	DLog(@"SANITY lCol: '%@', mCol:'%@, rCol:'%@'.", TUYesOrNoString(lCol),TUYesOrNoString(mCol),TUYesOrNoString(rCol));	
	DLog(@"SANITY <ow: '%.1f', tr: '%.1f', mr: '%.1f', lr: '%.1f'>",ow,tr,mr,lr);
	DLog(@"SANITY <oh: '%.1f', lc: '%.1f', mc: '%.1f', lc: '%.1f'>",oh,lc,mc,rc);
	return sizesMatch;
}

#pragma mark TUNinePatch Overrides
-(void)drawInRect:(CGRect)rect {
	NPSelfProperty(center);
	NPSelfProperty(leftEdge);
	NPSelfProperty(rightEdge);
	NPSelfProperty(lowerEdge);
	NPSelfProperty(upperEdge);
	NPSelfProperty(upperLeftCorner);
	NPSelfProperty(upperRightCorner);
	NPSelfProperty(lowerLeftCorner);
	NPSelfProperty(lowerRightCorner);
	
	CGFloat leftEdgeWidth = [self leftEdgeWidth];
	CGFloat rightEdgeWidth = [self rightEdgeWidth];
	BOOL hasLeftEdge  = (leftEdgeWidth > 0.0f)?(YES):(NO);
	BOOL hasRightEdge = (rightEdgeWidth > 0.0f)?(YES):(NO);
	
	CGFloat upperEdgeHeight = [self upperEdgeHeight];
	CGFloat lowerEdgeHeight = [self lowerEdgeHeight];
	BOOL hasUpperEdge = (upperEdgeHeight > 0.0f)?(YES):(NO);
	BOOL hasLowerEdge = (lowerEdgeHeight > 0.0f)?(YES):(NO);
	
	BOOL hasUpperLeftCorner = (hasLeftEdge && hasUpperEdge)?(YES):(NO);
	BOOL hasUpperRightCorner = (hasRightEdge && hasUpperEdge)?(YES):(NO);
	BOOL hasLowerLeftCorner = (hasLeftEdge && hasLowerEdge)?(YES):(NO);
	BOOL hasLowerRightCorner = (hasRightEdge && hasLowerEdge)?(YES):(NO);
	
	
	
	CGFloat contentWidth = TUTruncateAtZero(rect.size.width - (leftEdgeWidth + rightEdgeWidth));
	CGFloat contentHeight = TUTruncateAtZero(rect.size.height - (upperEdgeHeight + lowerEdgeHeight));
	BOOL hasContentWidth = (contentWidth > 0.0f)?(YES):(NO);
	BOOL hasContentHeight = (contentWidth > 0.0f)?(YES):(NO);
	if (hasContentWidth && hasContentHeight) {
		[self.center drawInRect:CGRectMake(CGRectGetMinX(rect) + leftEdgeWidth, CGRectGetMinY(rect) + upperEdgeHeight, contentWidth, contentHeight)];
	}
	if (hasContentWidth) {
		if (hasUpperEdge) {
			[self.upperEdge drawInRect:CGRectMake(
												  CGRectGetMinX(rect) + leftEdgeWidth, 
												  CGRectGetMinY(rect), 
												  contentWidth, 
												  upperEdgeHeight)];
		}
		if (hasLowerEdge) {
			[self.lowerEdge drawInRect:CGRectMake(
												  CGRectGetMinX(rect) + leftEdgeWidth, 
												  CGRectGetMaxY(rect) - lowerEdgeHeight, 
												  contentWidth, 
												  lowerEdgeHeight)];
		}
	}
	if (hasContentHeight) {
		if (hasLeftEdge) {
			[self.leftEdge drawInRect:CGRectMake(
												 CGRectGetMinX(rect), 
												 CGRectGetMinY(rect) + upperEdgeHeight, 
												 leftEdgeWidth, 
												 contentHeight)];
		}
		if (hasRightEdge) {
			[self.rightEdge drawInRect:CGRectMake(
												  CGRectGetMaxX(rect) - rightEdgeWidth, 
												  CGRectGetMinY(rect) + upperEdgeHeight, 
												  rightEdgeWidth, 
												  contentHeight)];
		}
	}
	if (hasUpperLeftCorner && self.upperLeftCorner) {
		[self.upperLeftCorner drawAtPoint:CGPointMake(CGRectGetMinX(rect), CGRectGetMinY(rect))];
	}
	if (hasUpperRightCorner && self.upperRightCorner) {
		[self.upperRightCorner drawAtPoint:CGPointMake(CGRectGetMaxX(rect) - rightEdgeWidth, CGRectGetMinY(rect))];
	}
	if (hasLowerLeftCorner && self.lowerLeftCorner) {
		[self.lowerLeftCorner drawAtPoint:CGPointMake(CGRectGetMinX(rect), CGRectGetMaxY(rect) - lowerEdgeHeight)];
	}
	if (hasLowerRightCorner && self.lowerRightCorner) {
		[self.lowerRightCorner drawAtPoint:CGPointMake(CGRectGetMaxX(rect) - rightEdgeWidth, CGRectGetMaxY(rect) - lowerEdgeHeight)];
	}
}
			 
#pragma mark -
-(CGFloat)leftEdgeWidth {
	return ((self.upperLeftCorner)?
			([self.upperLeftCorner size].width):
			(((self.lowerLeftCorner)?
			  ([self.lowerLeftCorner size].width):(0.0f))));
}
	 
-(CGFloat)rightEdgeWidth {
	return ((self.upperRightCorner)?
			([self.upperRightCorner size].width)
			:(((self.lowerRightCorner)?
			   ([self.lowerRightCorner size].width):(0.0f))));
}
			 
-(CGFloat)upperEdgeHeight {
	return ((self.upperLeftCorner)?
			([self.upperLeftCorner size].height)
			:(((self.upperRightCorner)?
			   ([self.upperRightCorner size].height):(0.0f))));
}
			 
-(CGFloat)lowerEdgeHeight {
	return ((self.lowerLeftCorner)?
			([self.lowerLeftCorner size].height)
			:(((self.lowerRightCorner)?
			   ([self.lowerRightCorner size].height):(0.0f))));
}

#pragma mark Customized Description Overrides
-(NSString *)descriptionPostfix {
	return [NSString stringWithFormat:@"%@, self.upperEdge:<'%@'>, self.lowerEdge:<'%@'>, self.leftEdge:<'%@'>, self.rightEdge:<'%@'>,  self.upperLeftCorner:<'%@'>, self.lowerLeftCorner:<'%@'>, self.upperRightCorner:<'%@'>, self.lowerRightCorner:<'%@'>",
			[super descriptionPostfix],
			self.upperEdge, 
			self.lowerEdge,
			self.leftEdge,
			self.rightEdge,
			self.upperLeftCorner,
			self.lowerLeftCorner,
			self.upperRightCorner,
			self.lowerRightCorner];
}

#pragma mark Image Logging
-(void)logExplodedImage {
	CGFloat centerWidth = ((self.center)?(self.center.size.width):(0.0f));
	CGFloat centerHeight = ((self.center)?(self.center.size.height):(0.0f));
	CGSize mySize = CGSizeMake(
		4.0f + (centerWidth + ([self leftEdgeWidth]) + ([self rightEdgeWidth])),
		4.0f + (centerHeight + ([self upperEdgeHeight]) + ([self lowerEdgeHeight]))	
	);
	UIGraphicsBeginImageContext(mySize);
	[self.center drawAtPoint:CGPointMake(2.0f + [self leftEdgeWidth], 2.0f + [self upperEdgeHeight])];
	[self.upperLeftCorner drawAtPoint:CGPointMake(0.0f, 0.0f)];
	[self.leftEdge drawAtPoint:CGPointMake(0.0f, [self upperEdgeHeight] + 2.0f)];
	[self.lowerLeftCorner drawAtPoint:CGPointMake(0.0f, [self upperEdgeHeight] + 4.0f + centerHeight)];
	[self.upperEdge drawAtPoint:CGPointMake(2.0f + [self leftEdgeWidth], 0.0f)];
	[self.upperRightCorner drawAtPoint:CGPointMake(4.0f + [self leftEdgeWidth] + centerWidth, 0.0f)];
	[self.rightEdge drawAtPoint:CGPointMake(4.0f + [self leftEdgeWidth] + centerWidth, 2.0f + [self upperEdgeHeight])];
	[self.lowerRightCorner drawAtPoint:CGPointMake(4.0f + [self leftEdgeWidth] + centerWidth, 4.0f + [self upperEdgeHeight] + centerHeight)];
	[self.lowerEdge drawAtPoint:CGPointMake(2.0f + [self leftEdgeWidth], 4.0f + [self upperEdgeHeight] + centerHeight)];
	IMLog(UIGraphicsGetImageFromCurrentImageContext(), @"explodedNinePatchImage");
	UIGraphicsEndImageContext();
}
			 
@end