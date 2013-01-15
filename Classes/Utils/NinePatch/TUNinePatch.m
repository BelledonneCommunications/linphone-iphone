//
//  TUNinePatch.m
//  NinePatch
//
//  Copyright 2009 Tortuga 22, Inc. All rights reserved.
//

#import "TUNinePatch.h"
#import "TUVerticalNinePatch.h"
#import "TUHorizontalNinePatch.h"
#import "TUFullNinePatch.h"
#import "UIImage-TUNinePatch.h"

@interface TUNinePatch ()

@property(nonatomic, retain, readwrite) UIImage *center;
@property(nonatomic, assign, readwrite) CGRect contentRegion;
@property(nonatomic, assign, readwrite) BOOL tileCenterVertically;
@property(nonatomic, assign, readwrite) BOOL tileCenterHorizontally;

@end


@implementation TUNinePatch

#pragma mark Synthesized Properties
@synthesize center = _center;
@synthesize contentRegion = _contentRegion;
@synthesize tileCenterVertically = _tileCenterVertically;
@synthesize tileCenterHorizontally = _tileCenterHorizontally;

#pragma mark NSCoding
-(id)initWithCoder:(NSCoder *)coder {
	NPAOInputLog(coder);
	if (self = [super init]) {
		self.center = (UIImage *)[coder decodeObjectForKey:@"center"];
		self.contentRegion = [coder decodeCGRectForKey:@"contentRegion"];
		self.tileCenterVertically = [coder decodeBoolForKey:@"tileCenterVertically"];
		self.tileCenterHorizontally = [coder decodeBoolForKey:@"tileCenterHorizontally"];
	}
	return self;
}

-(void)encodeWithCoder:(NSCoder *)coder {
	NPAOInputLog(coder);
	[coder encodeObject:self.center 
				 forKey:@"center"];
	
	[coder encodeCGRect:self.contentRegion
				 forKey:@"contentRegion"];
	
	[coder encodeBool:self.tileCenterVertically 
			   forKey:@"tileCenterVertically"];

	[coder encodeBool:self.tileCenterHorizontally 
			   forKey:@"tileCenterHorizontally"];
}

#pragma mark NSCopying
-(id)copyWithZone:(NSZone *)zone {
	return [[[self class] allocWithZone:zone] initWithCenter:self.center 
											   contentRegion:self.contentRegion 
										tileCenterVertically:self.tileCenterVertically 
									  tileCenterHorizontally:self.tileCenterHorizontally];
}

#pragma mark Init + Dealloc
-(id)initWithCenter:(UIImage *)center contentRegion:(CGRect)contentRegion {
	return [self initWithCenter:center 
				  contentRegion:contentRegion 
		   tileCenterVertically:NO 
		 tileCenterHorizontally:NO];
}

-(id)initWithCenter:(UIImage *)center contentRegion:(CGRect)contentRegion tileCenterVertically:(BOOL)tileCenterVertically tileCenterHorizontally:(BOOL)tileCenterHorizontally {
	NPAInputLog(@"[%@:<0x%x> initWithCenter:%@ contentRegion:%@ tileCenterVertically:%d tileCenterHorizontally:%d]", [self class], ((NSUInteger) self), center, NSStringFromCGRect(contentRegion), tileCenterVertically, tileCenterHorizontally);
	NPParameterAssertNotNilIsKindOfClass(center, UIImage);
	if (self = [super init]) {
		self.center = center;
		self.contentRegion = contentRegion;
		self.tileCenterVertically = tileCenterVertically;
		self.tileCenterHorizontally = tileCenterHorizontally;
	}
	return self;
}

#pragma mark -
-(void)dealloc {
	self.center = nil;
	[super dealloc];
}

#pragma mark Convenience Constructors
+(id < TUNinePatch >)ninePatchWithNinePatchImage:(UIImage *)ninePatchImage {
	NPAInputLog(@"ninePatchWithNinePatchImage:'%@'",ninePatchImage);
	id < TUNinePatch > outPatch = nil;
	if (ninePatchImage) {
		@try {
			outPatch = [self ninePatchWithImage:[ninePatchImage imageAsNinePatchImage] 
							  stretchableRegion:[self stretchableRegionOfNinePatchImage:ninePatchImage] 
								  contentRegion:[self contentRegionOfNinePatchImage:ninePatchImage]
						   tileCenterVertically:[self shouldTileCenterVerticallyForNinePatchImage:ninePatchImage]
						 tileCenterHorizontally:[self shouldTileCenterHorizontallyForNinePatchImage:ninePatchImage]];
		}
		@catch (NSException * e) {
			NPLogException(e);
			outPatch = nil;
		}
	}
	NPAssertNilOrConformsToProtocol(outPatch,TUNinePatch);
	NPOOutputLog(outPatch);
	return outPatch;
}

+(id < TUNinePatch >)ninePatchWithImage:(UIImage *)image stretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"ninePatchWithImage:'%@' stretchableRegion:'%@'",image,NSStringFromCGRect(stretchableRegion));
	NPParameterAssertNotNilIsKindOfClass(image,UIImage);
	return [self ninePatchWithImage:image 
				  stretchableRegion:stretchableRegion 
					  contentRegion:CGRectZero
			   tileCenterVertically:NO
			 tileCenterHorizontally:NO];
}

+(id < TUNinePatch >)ninePatchWithImage:(UIImage *)image stretchableRegion:(CGRect)stretchableRegion contentRegion:(CGRect)contentRegion tileCenterVertically:(BOOL)tileCenterVertically tileCenterHorizontally:(BOOL)tileCenterHorizontally {
	NPAInputLog(@"ninePatchWithImage:'%@' stretchableRegion:'%@' contentRegion:'%@' tileCenterVertically:'%@' tileCenterHorizontally:'%@'",image,NSStringFromCGRect(stretchableRegion),NSStringFromCGRect(contentRegion),TUYesOrNoString(tileCenterVertically),TUYesOrNoString(tileCenterHorizontally));
	NPParameterAssertNotNilIsKindOfClass(image,UIImage);	
	NPParameterAssert(stretchableRegion.origin.x >= 0.0f);
	NPParameterAssert(stretchableRegion.origin.y >= 0.0f);
	NPParameterAssert([image size].width >= stretchableRegion.origin.x + stretchableRegion.size.width);
	NPParameterAssert([image size].height >= stretchableRegion.origin.y + stretchableRegion.size.height);
	id < TUNinePatch > ninePatch = nil;
	if (image) {
		CGFloat imageWidth = [image size].width;
		CGFloat imageHeight = [image size].height;
		CGRect fixedStretchableRegion = stretchableRegion;
		CGFloat stretchableRegionMinX = CGRectGetMinX(fixedStretchableRegion);
		CGFloat stretchableRegionMinY = CGRectGetMinY(fixedStretchableRegion);
		CGFloat stretchableRegionMaxX = CGRectGetMaxX(fixedStretchableRegion);
		CGFloat stretchableRegionMaxY = CGRectGetMaxY(fixedStretchableRegion);
		BOOL stretchesOnLeft = (stretchableRegionMinX > 0.0f)?(YES):(NO);
		BOOL stretchesOnRight = (stretchableRegionMaxX < imageWidth)?(YES):(NO);
		BOOL stretchesOnTop = (stretchableRegionMinY > 0.0f)?(YES):(NO);
		BOOL stretchesOnBottom = (stretchableRegionMaxY < imageHeight)?(YES):(NO);
		BOOL stretchesHorizontally = (stretchesOnLeft || stretchesOnRight)?(YES):(NO);
		BOOL stretchesVertically = (stretchesOnTop || stretchesOnBottom)?(YES):(NO);
		if (stretchesVertically && stretchesHorizontally) {
			LLog(@"...the specified stretchable region stretches horizontally and vertically.");
			UIImage *center = [image extractCenterForStretchableRegion:fixedStretchableRegion];
			UIImage *upperLeftCorner = [image extractUpperLeftCornerForStretchableRegion:fixedStretchableRegion];
			UIImage *upperRightCorner = [image extractUpperRightCornerForStretchableRegion:fixedStretchableRegion];
			UIImage *lowerLeftCorner = [image extractLowerLeftCornerForStretchableRegion:fixedStretchableRegion];
			UIImage *lowerRightCorner = [image extractLowerRightCornerForStretchableRegion:fixedStretchableRegion];
			UIImage *leftEdge = [image extractLeftEdgeForStretchableRegion:fixedStretchableRegion];
			UIImage *rightEdge = [image extractRightEdgeForStretchableRegion:fixedStretchableRegion];
			UIImage *lowerEdge = [image extractLowerEdgeForStretchableRegion:fixedStretchableRegion];
			UIImage *upperEdge = [image extractUpperEdgeForStretchableRegion:fixedStretchableRegion];

			// Mega-Block of size sanity checking 
			
			// Given that the only major bug encountered while developing this library 
			// proved to be a difficult-to-track-down source of off-by-one errors in
			// the sizes of the slices, you can understand the paranoia here.
			// 
			// Just remember to build with the assertion-checking off.
			
			NPAssertCorrectSubimageWidthDecomposition(image, upperLeftCorner, upperEdge, upperRightCorner);
			NPAssertCorrectSubimageWidthDecomposition(image, leftEdge, upperEdge, rightEdge);
			NPAssertCorrectSubimageWidthDecomposition(image, lowerLeftCorner, lowerEdge, lowerRightCorner);
			
			NPAssertCorrectSubimageHeightDecomposition(image, upperLeftCorner, leftEdge, lowerLeftCorner);
			NPAssertCorrectSubimageHeightDecomposition(image, upperEdge, center, lowerEdge);
			NPAssertCorrectSubimageHeightDecomposition(image, upperRightCorner, rightEdge, lowerRightCorner);
			
			NPAssertWithinOne(([upperLeftCorner size].height), ([upperRightCorner size].height));
			NPAssertWithinOne(([upperLeftCorner size].height), ([upperEdge size].height));
			NPAssertWithinOne(([upperEdge size].height), ([upperRightCorner size].height));
			
			NPAssertWithinOne(([leftEdge size].height), ([center size].height));
			NPAssertWithinOne(([center size].height), ([rightEdge size].height));
			NPAssertWithinOne(([rightEdge size].height), ([leftEdge size].height));
			
			NPAssertWithinOne(([lowerLeftCorner size].height), ([lowerRightCorner size].height));
			NPAssertWithinOne(([lowerRightCorner size].height), ([lowerEdge size].height));
			NPAssertWithinOne(([lowerEdge size].height), ([lowerLeftCorner size].height));

			NPAssertWithinOne(([upperLeftCorner size].width), ([leftEdge size].width));
			NPAssertWithinOne(([leftEdge size].width), ([lowerLeftCorner size].width));
			NPAssertWithinOne(([upperLeftCorner size].width), ([lowerLeftCorner size].width));

			NPAssertWithinOne(([upperEdge size].width), ([center size].width));
			NPAssertWithinOne(([center size].width), ([lowerEdge size].width));
			NPAssertWithinOne(([upperEdge size].width), ([lowerEdge size].width));

			NPAssertWithinOne(([upperRightCorner size].width), ([rightEdge size].width));
			NPAssertWithinOne(([rightEdge size].width), ([lowerRightCorner size].width));
			NPAssertWithinOne(([lowerRightCorner size].width), ([upperRightCorner size].width));
			
			ninePatch = [[[TUFullNinePatch alloc] initWithCenter:center
												   contentRegion:contentRegion 
											tileCenterVertically:tileCenterVertically
										  tileCenterHorizontally:tileCenterHorizontally
												 upperLeftCorner:upperLeftCorner
												upperRightCorner:upperRightCorner
												 lowerLeftCorner:lowerLeftCorner
												lowerRightCorner:lowerRightCorner
														leftEdge:leftEdge
													   rightEdge:rightEdge
													   upperEdge:upperEdge
													   lowerEdge:lowerEdge] autorelease];
		} else if (stretchesVertically) {
			UIImage *center = [image extractCenterForStretchableRegion:fixedStretchableRegion];
			UIImage *upperEdge = [image extractUpperEdgeForStretchableRegion:fixedStretchableRegion];
			UIImage *lowerEdge = [image extractLowerEdgeForStretchableRegion:fixedStretchableRegion];
			
			NPAssertCorrectSubimageHeightDecomposition(image,center,upperEdge,lowerEdge);
			NPAssertWithinOne(([center size].width),([upperEdge size].width));
			NPAssertWithinOne(([lowerEdge size].width),([upperEdge size].width));
			NPAssertWithinOne(([center size].width),([lowerEdge size].width));
			
			ninePatch = [[[TUVerticalNinePatch alloc] initWithCenter:center
													   contentRegion:contentRegion
												tileCenterVertically:tileCenterVertically
											  tileCenterHorizontally:tileCenterHorizontally
														   upperEdge:upperEdge
														   lowerEdge:lowerEdge] autorelease];
		} else if (stretchesHorizontally) {
			UIImage *center = [image extractCenterForStretchableRegion:fixedStretchableRegion];
			UIImage *leftEdge = [image extractLeftEdgeForStretchableRegion:fixedStretchableRegion];
			UIImage *rightEdge = [image extractRightEdgeForStretchableRegion:fixedStretchableRegion];

			NPAssertCorrectSubimageWidthDecomposition(image, leftEdge, center, rightEdge);
			NPAssertWithinOne(([center size].height),([leftEdge size].height));
			NPAssertWithinOne(([center size].height),([rightEdge size].height));
			NPAssertWithinOne(([leftEdge size].height),([rightEdge size].height));
			
			ninePatch = [[[TUHorizontalNinePatch alloc] initWithCenter:center
														 contentRegion:contentRegion
												  tileCenterVertically:tileCenterVertically
												tileCenterHorizontally:tileCenterHorizontally
															  leftEdge:leftEdge
															 rightEdge:rightEdge] autorelease];
		} else {
			ninePatch = [[[self alloc] initWithCenter:image 
										contentRegion:contentRegion
								 tileCenterVertically:tileCenterVertically
							   tileCenterHorizontally:tileCenterHorizontally] autorelease];
		}
	}
	NPAssertNilOrConformsToProtocol(ninePatch,TUNinePatch);
	NPOOutputLog(ninePatch);
	return ninePatch;
}

#pragma mark Bundle Loading
+(id < TUNinePatch >)ninePatchNamed:(NSString *)filename {
	NPParameterAssertNotNilIsKindOfClass(filename,NSString);
	id < TUNinePatch > outPatch = nil;
	if (filename) {
		NSBundle *mainBundle = [NSBundle mainBundle];
		if (mainBundle) {
			NSString *filePath = [mainBundle pathForResource:[NSString stringWithFormat:@"%@.9",filename] 
													  ofType:@"png"];
			if (filePath) {
				UIImage *ninepatch = [[UIImage alloc] initWithContentsOfFile:filePath];
				if (ninepatch) {
					@try {
						outPatch = [self ninePatchWithNinePatchImage:ninepatch];
					}
					@catch (NSException * e) {
						NPLogException(e);
						outPatch = nil;
					}
					@finally {
						[ninepatch release];
					}
				}
			}
		}
	}
	NPAssertNilOrConformsToProtocol(outPatch,TUNinePatch);
	NPOOutputLog(outPatch);
	return outPatch;
}

#pragma mark Nine Patch Image Manipulation - High Level
+(CGRect)rectFromHorizontalRange:(NSRange)horizontalRange verticalRange:(NSRange)verticalRange {
	NPAInputLog(@"rectFromHorizontalRange:'%@' verticalRange:'%@'",NSStringFromRange(horizontalRange),NSStringFromRange(verticalRange));
	CGFloat minX = (TUIsNotFoundRange(horizontalRange))?(0.0f):((CGFloat) horizontalRange.location);
	CGFloat width = (TUIsNotFoundRange(horizontalRange))?(0.0f):((CGFloat) horizontalRange.length);
	CGFloat minY = (TUIsNotFoundRange(verticalRange)?(0.0f):((CGFloat) verticalRange.location));
	CGFloat height = (TUIsNotFoundRange(verticalRange)?(0.0f):((CGFloat) verticalRange.length));
	CGRect outRect = CGRectMake(minX,minY,width,height);
	NPCGROutputLog(outRect);
	return outRect;
}

+(CGRect)stretchableRegionOfNinePatchImage:(UIImage *)ninePatchImage {
	NPAInputLog(@"stretchableRegionOfNinePatchImage:'%@'",ninePatchImage);
	NPParameterAssertNotNilIsKindOfClass(ninePatchImage,UIImage);
	CGRect outRect = CGRectZero;
	if (ninePatchImage) {
		outRect = [self rectFromHorizontalRange:[ninePatchImage blackPixelRangeInUpperStrip]
								  verticalRange:[ninePatchImage blackPixelRangeInLeftStrip]];
	}
	NPCGROutputLog(outRect);
	return outRect;
}

+(CGRect)contentRegionOfNinePatchImage:(UIImage *)ninePatchImage {
	NPAInputLog(@"contentRegionOfNinePatchImage:'%@'",ninePatchImage);
	NPParameterAssertNotNilIsKindOfClass(ninePatchImage,UIImage);
	CGRect outRect = CGRectZero;
	if (ninePatchImage) {
		outRect = [self rectFromHorizontalRange:[ninePatchImage blackPixelRangeInLowerStrip]
								  verticalRange:[ninePatchImage blackPixelRangeInRightStrip]];
	}
	NPCGROutputLog(outRect);
	return outRect;
}

+(BOOL)shouldTileCenterHorizontallyForNinePatchImage:(UIImage *)ninePatchImage {
	NPAInputLog(@"shouldTileCenterHorizontallyForNinePatchImage:'%@'",ninePatchImage);
	NPParameterAssertNotNilIsKindOfClass(ninePatchImage,UIImage);
	BOOL shouldTileCenterHorizontallyForNinePatchImage = NO;
	if (ninePatchImage) {
		shouldTileCenterHorizontallyForNinePatchImage = [ninePatchImage upperLeftCornerIsBlackPixel];
	}
	NPBOutputLog(shouldTileCenterHorizontallyForNinePatchImage);
	return shouldTileCenterHorizontallyForNinePatchImage;
}

+(BOOL)shouldTileCenterVerticallyForNinePatchImage:(UIImage *)ninePatchImage {
	NPAInputLog(@"shouldTileCenterHorizontallyForNinePatchImage:'%@'",ninePatchImage);
	NPParameterAssertNotNilIsKindOfClass(ninePatchImage,UIImage);
	BOOL shouldTileCenterVerticallyForNinePatchImage = NO;
	if (ninePatchImage) {
		shouldTileCenterVerticallyForNinePatchImage = [ninePatchImage lowerLeftCornerIsBlackPixel];
	}
	NPBOutputLog(shouldTileCenterVerticallyForNinePatchImage);
	return shouldTileCenterVerticallyForNinePatchImage;
}

#pragma mark Drawing Utility
-(void)drawInRect:(CGRect)rect {
	if (self.center) {
		if (self.tileCenterHorizontally && self.tileCenterVertically) {
			[self.center drawAsPatternInRect:rect];
		} else {
			// NB: this behavior is not 100% accurate
			// in that it only works right for tiling on and off
			// half-tiling has to wait
			[self.center drawInRect:rect];
		}
	}
}

#pragma mark Diagnostic Utilities
-(UIImage *)upperEdge {
	return nil;
}

-(UIImage *)lowerEdge {
	return nil;
}

-(UIImage *)leftEdge {
	return nil;
}

-(UIImage *)rightEdge {
	return nil;
}

-(UIImage *)upperLeftCorner {
	return nil;
}

-(UIImage *)lowerLeftCorner {
	return nil;
}

-(UIImage *)upperRightCorner {
	return nil;
}

-(UIImage *)lowerRightCorner {
	return nil;
}

#pragma mark TUNinePatch Protocol Methods - Drawing
-(void)inContext:(CGContextRef)context drawAtPoint:(CGPoint)point forContentOfSize:(CGSize)contentSize {
	NPParameterAssert(context != nil);
	NPAInputLog(@"inContext:'%@' drawAtPoint:'%@' forContentOfSize:'%@'",context,NSStringFromCGPoint(point),NSStringFromCGSize(contentSize));
	CGSize size = [self sizeForContentOfSize:contentSize];
	[self inContext:context 
		 drawInRect:CGRectMake(point.x, point.y, size.width, size.height)];
}

-(void)inContext:(CGContextRef)context drawCenteredInRect:(CGRect)rect forContentOfSize:(CGSize)contentSize {
	NPParameterAssert(context != nil);
	NPAInputLog(@"inContext:'%@' drawCenteredInRect:'%@' forContentOfSize:'%@'",context,NSStringFromCGRect(rect),NSStringFromCGSize(contentSize));
	CGSize size = [self sizeForContentOfSize:contentSize];
	CGFloat xStart = floorf((CGRectGetWidth(rect) - size.width) * 0.5f);
	CGFloat yStart = floorf((CGRectGetHeight(rect) - size.height) * 0.5f);
	[self inContext:context 
		 drawInRect:CGRectMake(xStart, yStart, size.width, size.height)];
}

-(void)inContext:(CGContextRef)context drawInRect:(CGRect)rect {
	if (context) {
		CGContextSaveGState(context);
		CGContextBeginTransparencyLayer(context, nil);
		@try {
			[self drawInRect:rect];
		}
		@catch (NSException * e) {
			NPLogException(e);
		}
		@finally {
			CGContextEndTransparencyLayer(context);
			CGContextRestoreGState(context);
		}
	}
}

#pragma mark TUNinePatch Protocol Methods - Image Construction
-(UIImage *)imageOfSize:(CGSize)size {
	UIImage *image = nil;
	UIGraphicsBeginImageContext(size);
	[self drawInRect:CGRectMake(0.0f,0.0f,size.width,size.height)];
	image = UIGraphicsGetImageFromCurrentImageContext();
	UIGraphicsEndImageContext();
	return image;
}

#pragma mark TUNinePatch Protocol Methods - Sizing
-(BOOL)stretchesHorizontally {
	return YES;
}

-(BOOL)stretchesVertically {
	return YES;
}

#pragma mark -
-(CGFloat)minimumWidth {
	CGFloat minimumWidth = 0.0f;
	if (self.center) {
		minimumWidth = [self.center size].width + self.rightEdgeWidth + self.leftEdgeWidth;
	}
	return minimumWidth;
}

-(CGFloat)minimumHeight {
	CGFloat minimumHeight = 0.0f;
	if (self.center) {
		minimumHeight = [self.center size].height + self.upperEdgeHeight + self.lowerEdgeHeight;
	}
	return minimumHeight;
}

-(CGSize)minimumSize {
	return CGSizeMake([self minimumWidth], [self minimumHeight]);
}

-(CGSize)sizeForContentOfSize:(CGSize)contentSize {
	CGSize outSize = [self minimumSize];
	CGFloat contentRegionWidth = CGRectGetWidth(self.contentRegion);
	CGFloat contentRegionHeight = CGRectGetHeight(self.contentRegion);
	if ((contentRegionWidth > 0.0f) || (contentRegionHeight > 0.0f)) {
		// WE HAVE CONTENT REGION
		outSize.width += (contentSize.width > contentRegionWidth)?(contentSize.width - contentRegionWidth):(0.0f);
		outSize.height += (contentSize.height > contentRegionHeight)?(contentSize.height - contentRegionHeight):(0.0f);		
	} else {
		// WE ONLY NEED A SIMPLE "WHICH IS BIGGER?" CHECK
		outSize.width = (contentSize.width > outSize.width)?(contentSize.width):(outSize.width);
		outSize.height = (contentSize.height > outSize.height)?(contentSize.height):(outSize.height);
	}
	return outSize;
}

-(CGPoint)upperLeftCornerForContentWhenDrawnAtPoint:(CGPoint)point {
	return CGPointMake(point.x + CGRectGetMinX(self.contentRegion), point.y + CGRectGetMinY(self.contentRegion));
}

#pragma mark TUNinePatch Protocol Methods - Geometry
-(CGFloat)leftEdgeWidth {
	return 0.0f;
}

-(CGFloat)rightEdgeWidth {
	return 0.0f;
}

-(CGFloat)upperEdgeHeight {
	return 0.0f;
}

-(CGFloat)lowerEdgeHeight {
	return 0.0f;
}

#pragma mark Customized Description
-(NSString *)description {
	return [NSString stringWithFormat:@"<%@>:( %@ )",[super description],[self descriptionPostfix]];
}

-(NSString *)descriptionPostfix {
	return [NSString stringWithFormat:@"center:<'%@'>, contentRegion:<'%@'>", self.center, NSStringFromCGRect(self.contentRegion)];
}


@end