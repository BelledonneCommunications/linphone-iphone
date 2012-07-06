//
//  TUHorizontalNinePatch.m
//  NinePatch
//
//  Copyright 2009 Tortuga 22, Inc. All rights reserved.
//

#import "TUHorizontalNinePatch.h"

@interface TUHorizontalNinePatch ()

// Synthesized Properties
@property(nonatomic, retain, readwrite) UIImage *leftEdge;
@property(nonatomic, retain, readwrite) UIImage *rightEdge;

@end


@implementation TUHorizontalNinePatch

#pragma mark Synthesized Properties
@synthesize leftEdge = _leftEdge;
@synthesize rightEdge = _rightEdge;

#pragma mark NSCoding
-(id)initWithCoder:(NSCoder *)coder {
	if (self = [super initWithCoder:coder]) {
		self.leftEdge = (UIImage *)[coder decodeObjectForKey:@"leftEdge"];
		self.rightEdge = (UIImage *)[coder decodeObjectForKey:@"rightEdge"];
	}
	return self;
}

-(void)encodeWithCoder:(NSCoder *)coder {
	[super encodeWithCoder:coder];
	
	[coder encodeObject:self.leftEdge 
				 forKey:@"leftEdge"];

	[coder encodeObject:self.rightEdge 
				 forKey:@"rightEdge"];
}

#pragma mark NSCopying
-(id)copyWithZone:(NSZone *)zone {
	return [[[self class] allocWithZone:zone] initWithCenter:self.center 
											   contentRegion:self.contentRegion 
										tileCenterVertically:self.tileCenterVertically 
									  tileCenterHorizontally:self.tileCenterHorizontally 
													leftEdge:self.leftEdge 
												   rightEdge:self.rightEdge];
}

#pragma mark Init + Dealloc
-(id)initWithCenter:(UIImage *)center contentRegion:(CGRect)contentRegion tileCenterVertically:(BOOL)tileCenterVertically tileCenterHorizontally:(BOOL)tileCenterHorizontally leftEdge:(UIImage *)leftEdge rightEdge:(UIImage *)rightEdge {
	NPParameterAssertNotNilIsKindOfClass(leftEdge,UIImage);
	NPParameterAssertNotNilIsKindOfClass(rightEdge,UIImage);
	if (self = [super initWithCenter:center 
					   contentRegion:contentRegion 
				tileCenterVertically:tileCenterVertically 
			  tileCenterHorizontally:tileCenterHorizontally]) {
		self.leftEdge = leftEdge;
		self.rightEdge = rightEdge;
	}
	return self;
}

#pragma mark -
-(void)dealloc {
	self.leftEdge = nil;
	self.rightEdge = nil;
	[super dealloc];
}

#pragma mark TUNinePatch Overrides
-(void)drawInRect:(CGRect)rect {
	CGFloat height = [self minimumHeight];
	[self.center drawInRect:CGRectMake(CGRectGetMinX(rect) + [self leftEdgeWidth], CGRectGetMinY(rect), CGRectGetWidth(rect) - ([self leftEdgeWidth] + [self rightEdgeWidth]), height)];
	if (self.leftEdge) {
		[self.leftEdge drawAtPoint:CGPointMake(CGRectGetMinX(rect),CGRectGetMinY(rect))];
	}
	if (self.rightEdge) {
		[self.rightEdge drawAtPoint:CGPointMake(CGRectGetMaxX(rect) - [self rightEdgeWidth], CGRectGetMinY(rect))];
	}
}

#pragma mark -
-(BOOL)stretchesVertically {
	return NO;
}

#pragma mark -
-(CGSize)sizeForContentOfSize:(CGSize)contentSize {
	CGSize outSize = [super sizeForContentOfSize:contentSize];
	outSize.height = [self minimumHeight];
	return outSize;
}

#pragma mark -
-(CGFloat)leftEdgeWidth {
	CGFloat leftEdgeWidth = 0.0f;
	if (self.leftEdge) {
		leftEdgeWidth = [self.leftEdge size].width;
	}
	return leftEdgeWidth;
}

-(CGFloat)rightEdgeWidth {
	CGFloat rightEdgeWidth = 0.0f;
	if (self.leftEdge) {
		rightEdgeWidth = [self.rightEdge size].width;
	}
	return rightEdgeWidth;
}

#pragma mark Customized Description Overrides
-(NSString *)descriptionPostfix {
	return [NSString stringWithFormat:@"%@, self.leftEdge:<'%@'>, self.rightEdge:<'%@'>", 
			[super descriptionPostfix],
			self.leftEdge, 
			self.rightEdge];
}

@end