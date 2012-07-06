//
//  UIImage-TUNinePatch.m
//  NinePatch
//
//  Copyright 2009 Tortuga 22, Inc. All rights reserved.
//

#import "UIImage-TUNinePatch.h"
#import "TUNinePatchProtocols.h"

void TUImageLog(UIImage *image, NSString *imageName) {
	if (image && imageName) {
		NSString *fullFileName = [imageName stringByAppendingString:@".png"];
		if (fullFileName) {
			NSString *documentsDirectory = [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
			if (documentsDirectory) {
				NSString *fullFilePath = [documentsDirectory stringByAppendingPathComponent:fullFileName];
				if (fullFilePath) {
					NSData *pngData = UIImagePNGRepresentation(image);
					if (pngData) {
						NSFileManager *fileManager = [NSFileManager defaultManager];
						if (fileManager) {
							BOOL succeeded = [fileManager createFileAtPath:fullFilePath contents:pngData attributes:nil];
							if (succeeded) {
								DLog(@"Seemingly successfully wrote image to file at: '%@'.",fullFilePath);
							} else {
								DLog(@"Seemingly failed to write image to file at: '%@'.",fullFilePath);
							}
						} else {
							LLog(@"Couldn't get default fileManager, aborting imagelog.");
						}
					} else {
						LLog(@"Couldn't get PNGRepresentation, aborting imagelog.");
					}
				} else {
					LLog(@"Couldn't get fullFilePath, aborting imagelog.");
				}
			} else {
				LLog(@"Couldn't get fullFilePath, aborting imagelog.");
			}
		} else {
			DLog(@"Could't get fullFileName, aborting imageLog.");
		}
	} else {
		DLog(@"Can't log image: '%@', imageName: '%@', as one or both are nil.",image, imageName);
	}
}

@implementation UIImage (TUNinePatch)

#pragma mark Black Pixel Searching - Corners
-(BOOL)upperLeftCornerIsBlackPixel {
	BOOL upperLeftCornerIsBlackPixel = NO;
	UIImage *upperLeftCorner = [self upperLeftCorner];
	if (upperLeftCorner) {
		upperLeftCornerIsBlackPixel = [upperLeftCorner isBlackPixel];
	}
	NPBOutputLog(upperLeftCornerIsBlackPixel);
	return upperLeftCornerIsBlackPixel;	
}

-(BOOL)upperRightCornerIsBlackPixel {
	BOOL upperRightCornerIsBlackPixel = NO;
	UIImage *upperRightCorner = [self upperRightCorner];
	if (upperRightCorner) {
		upperRightCornerIsBlackPixel = [upperRightCorner isBlackPixel];
	}
	NPBOutputLog(upperRightCornerIsBlackPixel);
	return upperRightCornerIsBlackPixel;
}

-(BOOL)lowerLeftCornerIsBlackPixel {
	BOOL lowerLeftCornerIsBlackPixel = NO;
	UIImage *lowerLeftCorner = [self lowerLeftCorner];
	if (lowerLeftCorner) {
		lowerLeftCornerIsBlackPixel = [lowerLeftCorner isBlackPixel];
	}
	NPBOutputLog(lowerLeftCornerIsBlackPixel);
	return lowerLeftCornerIsBlackPixel;	
}

-(BOOL)lowerRightCornerIsBlackPixel {
	BOOL lowerRightCornerIsBlackPixel = NO;
	UIImage *lowerRightCorner = [self lowerRightCorner];
	if (lowerRightCorner) {
		lowerRightCornerIsBlackPixel = [lowerRightCorner isBlackPixel];
	}
	NPBOutputLog(lowerRightCornerIsBlackPixel);
	return lowerRightCornerIsBlackPixel;
}

#pragma mark Pixel Tasting - Single Pixel
-(BOOL)isBlackPixel {
	NPAssert(([self size].width > 0.0f), @"Should have width > 0.0f");
	NPAssert(([self size].height > 0.0f), @"Should have height > 0.0f");
	BOOL isBlackPixel = NO;
	if (([self size].width > 0.0f) && ([self size].height > 0.0f)) {
		CGImageRef cgImage = [self CGImage];
		NSUInteger width = CGImageGetWidth(cgImage);
		NSUInteger height = CGImageGetHeight(cgImage);
		NSUInteger bytesPerRow = width * TURGBABytesPerPixel;
		NSUInteger bitsPerComponent = 8;
		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		UInt8 *pixelByteData = malloc(width * height * TURGBABytesPerPixel);
		
		
		CGContextRef context = CGBitmapContextCreate(
													 (void *)pixelByteData,
													 width,
													 height,
													 bitsPerComponent,
													 bytesPerRow,
													 colorSpace,
													 kCGImageAlphaPremultipliedLast);
		
		CGContextDrawImage(context, CGRectMake(0.0f,0.0f,1.0f,1.0f), cgImage);
		TURGBAPixel *pixelData = (TURGBAPixel *) CGBitmapContextGetData(context);
		if (pixelData) {
			isBlackPixel = TURGBAPixelIsBlack(pixelData[0]);
		}
		CGContextRelease(context);
		CGColorSpaceRelease(colorSpace);
		free(pixelByteData);
	}
	NPBOutputLog(isBlackPixel);
	return isBlackPixel;	
}

#pragma mark Black Pixel Searching - Strips
-(NSRange)blackPixelRangeInUpperStrip {
	NSRange blackPixelRangeInUpperStrip = TUNotFoundRange;
	UIImage *upperStrip = [self upperStrip];
	if (upperStrip) {
		blackPixelRangeInUpperStrip = [upperStrip blackPixelRangeAsHorizontalStrip];
	}
	NPNSROutputLog(blackPixelRangeInUpperStrip);
	return blackPixelRangeInUpperStrip;
}

-(NSRange)blackPixelRangeInLowerStrip {
	NSRange blackPixelRangeInLowerStrip = TUNotFoundRange;
	UIImage *lowerStrip = [self lowerStrip];
	if (lowerStrip) {
		blackPixelRangeInLowerStrip = [lowerStrip blackPixelRangeAsHorizontalStrip];
	}
	NPNSROutputLog(blackPixelRangeInLowerStrip);
	return blackPixelRangeInLowerStrip;
}

-(NSRange)blackPixelRangeInLeftStrip {
	NSRange blackPixelRangeInLeftStrip = TUNotFoundRange;
	UIImage *leftStrip = [self leftStrip];
	if (leftStrip) {
		blackPixelRangeInLeftStrip = [leftStrip blackPixelRangeAsVerticalStrip];
	}
	NPNSROutputLog(blackPixelRangeInLeftStrip);
	return blackPixelRangeInLeftStrip;
}

-(NSRange)blackPixelRangeInRightStrip {
	NSRange blackPixelRangeInRightStrip = TUNotFoundRange;
	UIImage *rightStrip = [self rightStrip];
	if (rightStrip) {
		blackPixelRangeInRightStrip = [rightStrip blackPixelRangeAsVerticalStrip];
	}
	NPNSROutputLog(blackPixelRangeInRightStrip);
	return blackPixelRangeInRightStrip;
}

#pragma mark Pixel Tasting - Strips
-(NSRange)blackPixelRangeAsVerticalStrip {
	NPAssert([self size].width == 1.0f, @"This method assumes the image has width == 1.0f");
	NSRange blackPixelRangeAsVerticalStrip = TUNotFoundRange;
	NSUInteger firstBlackPixel = NSNotFound;
	NSUInteger lastBlackPixel = NSNotFound;
	if ([self size].height > 0.0f) {
		CGImageRef cgImage = [self CGImage];
		
		NSUInteger width = CGImageGetWidth(cgImage);
		NSUInteger height = CGImageGetHeight(cgImage);
		NSUInteger bytesPerRow = width * TURGBABytesPerPixel;
		NSUInteger bitsPerComponent = 8;
		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		UInt8 *pixelByteData = malloc(width * height * TURGBABytesPerPixel);
		
		CGContextRef context = CGBitmapContextCreate(
													 (void *)pixelByteData,
													 width,
													 height,
													 bitsPerComponent,
													 bytesPerRow,
													 colorSpace,
													 kCGImageAlphaPremultipliedLast);

		// NEW: seeing nondetermnistic errors where sometimes the image is parsed right
		// and sometimes not parsed right. The followthing three lines paint the context
		// to solid white, then paste the image over it, so this ought to normalize the
		// outcome a bit more.
		CGRect contextBounds = CGRectMake(0.0f, 0.0f, width, height);
		CGContextSetFillColorWithColor(context, [[UIColor whiteColor] CGColor]);
		CGContextFillRect(context, contextBounds);
		
		// Having normalized the context we now paint the image
		CGContextDrawImage(context, contextBounds, cgImage);
		TURGBAPixel *pixelData = (TURGBAPixel *) CGBitmapContextGetData(context);
		if (pixelData) {
			// CF note in the AsHorizontal method below
			for (NSUInteger i = 0; i < height; i++) {
				if (TURGBAPixelIsBlack(pixelData[((height - 1) - i)])) {
					firstBlackPixel = ((height - 1) - i);
				}
				if (TURGBAPixelIsBlack(pixelData[i])) {
					lastBlackPixel = i;
				}
			}
			
			if ((firstBlackPixel != NSNotFound) && (lastBlackPixel != NSNotFound)) {
				NPAssert(lastBlackPixel >= firstBlackPixel, ([NSString stringWithFormat:@"Got firstBlackPixel:'%d' and lastBlackPixel:'%d'.",firstBlackPixel,lastBlackPixel]));
				blackPixelRangeAsVerticalStrip.location = TUTruncateWithin(firstBlackPixel, 0, height - 1) / self.scale;
				// We can't just use TUTruncateAtZero on lastBlackPixel - firstBlackPixel here.
				// The semantics of pixel coordinates are such that a zero difference between lastBlackPixel and firstBlackPixel is ok
				// but < 0 is obv. very bad.
				// Thus 1 + TUTruncateAtZero(lastBlackPixel - firstBlackPixel) won't work.
				// and fixing the expression s.t. it does work is more complicated than
				// just breaking it down like so.
				NSInteger length = lastBlackPixel - firstBlackPixel;
				if (length >= 0) {
					length += 1;
				} else {
					length = 0;
				}
				blackPixelRangeAsVerticalStrip.length = length/self.scale;
			}
		}
		CGContextRelease(context);
		CGColorSpaceRelease(colorSpace);
		free(pixelByteData);
	}
	NPNSROutputLog(blackPixelRangeAsVerticalStrip);
	return blackPixelRangeAsVerticalStrip;		
}

-(NSRange)blackPixelRangeAsHorizontalStrip {
	NPAssert([self size].height == 1.0f, @"This method assumes the image has height == 1.0f");
	NSRange blackPixelRangeAsHorizontalStrip = TUNotFoundRange;
	NSUInteger firstBlackPixel = NSNotFound;
	NSUInteger lastBlackPixel = NSNotFound;
	if ([self size].width > 0.0f) {
		CGImageRef cgImage = [self CGImage];
		
		NSUInteger width = CGImageGetWidth(cgImage);
		NSUInteger height = CGImageGetHeight(cgImage);
		NSUInteger bytesPerRow = width * TURGBABytesPerPixel;
		NSUInteger bitsPerComponent = 8;
		CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
		UInt8 *pixelByteData = malloc(width * height * TURGBABytesPerPixel);
		
		CGContextRef context = CGBitmapContextCreate(
													 (void *)pixelByteData,
													 width,
													 height,
													 bitsPerComponent,
													 bytesPerRow,
													 colorSpace,
													 kCGImageAlphaPremultipliedLast);
		
		// NEW: seeing nondetermnistic errors where sometimes the image is parsed right
		// and sometimes not parsed right. The followthing three lines paint the context
		// to solid white, then paste the image over it, so this ought to normalize the
		// outcome a bit more.
		CGRect contextBounds = CGRectMake(0.0f, 0.0f, width, height);
		CGContextSetFillColorWithColor(context, [[UIColor whiteColor] CGColor]);
		CGContextFillRect(context, contextBounds);
		
		// Having normalized the context we now paint the image		
		CGContextDrawImage(context, contextBounds, cgImage);
		TURGBAPixel *pixelData = (TURGBAPixel *) CGBitmapContextGetData(context);
		if (pixelData) {
			// The for loop below is walking the strip from both ends.
			// Basically you could do this check a bunch of ways, with a 
			// bunch of trade-offs in terms of how fast it is and how robust it
			// is and how any "format errors" in your nine patch manifest.
			//
			// What I have found is that ninepatch is a fussy format, with a 
			// common failure mode being that you painted a pixel "black" but
			// either got the alpha wrong, or it wasn't quite black, or it
			// didn't composite to black, etc., and thus get invalid ninepatches.
			//
			// What I do here is just look for the highest and lowest black pixels,
			// and treat anything in between as also black. EG: 
			//
			// - if X == black and O == not-black
			// - then these square brackes - [ and ] - enclose the "black" region
			//
			// - then: OOOOXXXXXOOOOO -> OOOO[XXXXX]OOOOO
			// - but also: OOOXXOOXXOOO -> OOO[XXOOXX]OOO
			// - and even: OXOOOOOOOXO -> O[XOOOOOOOX]O
			// 
			// This is a judgement call on my part, in that the approach I can take to
			// accomplish this is straightforward without any complicated state tracking,
			// and the behavior it has in the face of "invalid" nine-patches is generally
			// what I meant, anyways.
			//
			// The actual implementation is straightforward but suboptimal.
			// I look through the array once, iterating i from 0->(width -1).
			// On each iteration I taste the pixel @ i and at (width - 1) -1,
			// and if the pixel @ i is black I set the "lastBlackPixel" == i
			// and if the pixel @ (width - 1) - i is black I set the "firstBlackPixel"
			// to (width - 1) - i. 
			//
			// Once the loop is done the values in the lastBlackPixel and firstBlackPixel
			// contain what they ought to have.
			//
			// Given the continual risk of hard-to-spot off-by-one errors throughout this
			// library I've opted to keep it dumb and suboptimal in places like this one,
			// so that I can be more comfortable that what problems there are are elsewhere.
			//
			// If you subseqently do add an improved loop please wrap it in ifdefs like
			// #ifdef CLEVERNESS YOUR-CODE #else DUMB-CODE #endif
			//
			for (NSUInteger i = 0; i < width; i++) {
				if (TURGBAPixelIsBlack(pixelData[((width - 1) - i)])) {
					firstBlackPixel = ((width - 1) - i);
				}
				if (TURGBAPixelIsBlack(pixelData[i])) {
					lastBlackPixel = i;
				}
			}
			
			if ((firstBlackPixel != NSNotFound) && (lastBlackPixel != NSNotFound)) {
				NPAssert(lastBlackPixel >= firstBlackPixel, ([NSString stringWithFormat:@"Got firstBlackPixel:'%d' and lastBlackPixel:'%d'.",firstBlackPixel,lastBlackPixel]));
				blackPixelRangeAsHorizontalStrip.location = TUTruncateWithin(firstBlackPixel, 0, width - 1) / self.scale;
				// We can't just use TUTruncateAtZero on lastBlackPixel - firstBlackPixel here.
				// The semantics of pixel coordinates are such that a zero difference between lastBlackPixel and firstBlackPixel is ok
				// but < 0 is obv. very bad.
				// Thus 1 + TUTruncateAtZero(lastBlackPixel - firstBlackPixel) won't work.
				// and fixing the expression s.t. it does work is more complicated than
				// just breaking it down like so.
				NSInteger length = lastBlackPixel - firstBlackPixel;
				if (length >= 0) {
					length += 1;
				} else {
					length = 0;
				}
				blackPixelRangeAsHorizontalStrip.length = length / self.scale;
			}
		}
		CGContextRelease(context);
		CGColorSpaceRelease(colorSpace);
		free(pixelByteData);
	}
	NPNSROutputLog(blackPixelRangeAsHorizontalStrip);
	return blackPixelRangeAsHorizontalStrip;	
}

#pragma mark Corners - Rects
-(CGRect)upperLeftCornerRect {
	return CGRectMake(0.0f, 0.0f, 1.0f/self.scale, 1.0f/self.scale);
}

-(CGRect)lowerLeftCornerRect {
	return CGRectMake(0.0f, [self size].height - (1.0f/self.scale), 1.0f/self.scale, 1.0f/self.scale);
}

-(CGRect)upperRightCornerRect {
	return CGRectMake([self size].width - (1.0f/self.scale), 0.0f,  1.0f/self.scale, 1.0f/self.scale);
}

-(CGRect)lowerRightCornerRect {
	return CGRectMake([self size].width - 1.0f, [self size].height - (1.0f/self.scale),  1.0f/self.scale, 1.0f/self.scale);
}

#pragma mark Corners - Slicing
-(UIImage *)upperLeftCorner {
	return [self subImageInRect:[self upperLeftCornerRect]];
}

-(UIImage *)lowerLeftCorner {
	return [self subImageInRect:[self lowerLeftCornerRect]];
}

-(UIImage *)upperRightCorner {
	return [self subImageInRect:[self upperRightCornerRect]];
}

-(UIImage *)lowerRightCorner {
	return [self subImageInRect:[self lowerRightCornerRect]];
}

#pragma mark Strips - Sizing
-(CGRect)upperStripRect {
	CGSize selfSize = [self size];
	CGFloat stripWidth = TUTruncateAtZero(selfSize.width - (2.0f/self.scale));
	return CGRectMake((1.0f/self.scale), 0.0f, stripWidth, 1.0f/self.scale);
}

-(CGRect)lowerStripRect {
	CGSize selfSize = [self size];
	CGFloat stripWidth = TUTruncateAtZero(selfSize.width - (2.0f/self.scale));
	return CGRectMake(1.0f/self.scale, selfSize.height - (1.0f/self.scale), stripWidth, 1.0f/self.scale);
}

-(CGRect)leftStripRect {
	CGSize selfSize = [self size];
	CGFloat stripHeight = TUTruncateAtZero(selfSize.height - (2.0f/self.scale));
	return CGRectMake(0.0f, 1.0f/self.scale, 1.0f/self.scale, stripHeight);	
}

-(CGRect)rightStripRect {
	CGSize selfSize = [self size];
	CGFloat stripHeight = TUTruncateAtZero(selfSize.height - (2.0f/self.scale));
	return CGRectMake(selfSize.width - (1.0f/self.scale), 1.0f/self.scale, 1.0f/self.scale, stripHeight);
}

#pragma mark Strips - Slicing
-(UIImage *)upperStrip {
	return [self subImageInRect:[self upperStripRect]];
}

-(UIImage *)lowerStrip {
	return [self subImageInRect:[self lowerStripRect]];
}

-(UIImage *)leftStrip {
	return [self subImageInRect:[self leftStripRect]];
}

-(UIImage *)rightStrip {
	return [self subImageInRect:[self rightStripRect]];
}

-(UIImage *)subImageInRect:(CGRect)rect {
	NPAInputLog(@"subImageInRect:'%@'",NSStringFromCGRect(rect));
	UIImage *subImage = nil;
	CGImageRef cir = [self CGImage];
	if (cir) {
		rect.origin.x *= self.scale;
		rect.origin.y *= self.scale;
		rect.size.width *= self.scale;
		rect.size.height *= self.scale;
		CGImageRef subCGImage = CGImageCreateWithImageInRect(cir, rect);
		if (subCGImage) {
			subImage = [UIImage imageWithCGImage:subCGImage scale:self.scale orientation:self.imageOrientation];
			CGImageRelease(subCGImage);
			NPAssertNilOrIsKindOfClass(subImage,UIImage);
			NPAssert((CGSizeEqualToSize([subImage size], rect.size)), @"Shouldn't get unequal subimage and requested sizes.");
		} else {
			DLog(@"Couldn't create subImage in rect: '%@'.", NSStringFromCGRect(rect));
		}
	} else {
		LLog(@"self.CGImage is somehow nil.");
	}
	//DLog(@"[%@:<0x%x> subImageInRect:%@] yielded subImage: '%@' of size: '%@'", [self class], ((NSUInteger) self), NSStringFromCGRect(rect), subImage, NSStringFromCGSize([subImage size]));
	//IMLog(self, @"subImageInRectSourceImage");
	//IMLog(subImage, @"subImageInRect");
	NPOOutputLog(subImage);
	return subImage;
}

#pragma mark Nine-Patch Content Extraction
-(UIImage *)imageAsNinePatchImage {
	UIImage *imageOfNinePatchImage = nil;
	CGFloat width = [self size].width - (2.0f/self.scale);
	CGFloat height = [self size].height - (2.0f/self.scale);
	if (width > 0.0f && height > 0.0f) {
		imageOfNinePatchImage = [self subImageInRect:CGRectMake((1.0f/self.scale), (1.0f/self.scale), width, height)];
	}
	NPOOutputLog(imageOfNinePatchImage);
	return imageOfNinePatchImage;
}

#pragma mark -
-(UIImage *)extractUpperLeftCornerForStretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"extractUpperLeftCornerForStretchableRegion:'%@'",NSStringFromCGRect(stretchableRegion));
	UIImage *upperLeftCorner = [self subImageInRect:CGRectMake(0.0f, 0.0f, CGRectGetMinX(stretchableRegion), CGRectGetMinY(stretchableRegion))];
	NPOOutputLog(upperLeftCorner);
	return upperLeftCorner;
}

-(UIImage *)extractUpperRightCornerForStretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"extractUpperRightCornerForStretchableRegion:'%@'",NSStringFromCGRect(stretchableRegion));
	UIImage *upperRightCorner = [self subImageInRect:CGRectMake(CGRectGetMaxX(stretchableRegion), 0.0f, [self size].width - CGRectGetMaxX(stretchableRegion), CGRectGetMinY(stretchableRegion))];
	NPOOutputLog(upperRightCorner);
	return upperRightCorner;
}

-(UIImage *)extractLowerLeftCornerForStretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"extractUpperRightCornerForStretchableRegion:'%@'",NSStringFromCGRect(stretchableRegion));
	UIImage *lowerLeftCorner = [self subImageInRect:CGRectMake(0.0f, CGRectGetMaxY(stretchableRegion), CGRectGetMinX(stretchableRegion), [self size].height - CGRectGetMaxY(stretchableRegion))];
	NPOOutputLog(lowerLeftCorner);
	return lowerLeftCorner;
}

-(UIImage *)extractLowerRightCornerForStretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"extractLowerRightCornerForStretchableRegion:'%@'",NSStringFromCGRect(stretchableRegion));
	UIImage *lowerRightCorner = [self subImageInRect:CGRectMake(CGRectGetMaxX(stretchableRegion), CGRectGetMaxY(stretchableRegion), [self size].width - CGRectGetMaxX(stretchableRegion), [self size].height - CGRectGetMaxY(stretchableRegion))];
	NPOOutputLog(lowerRightCorner);
	return lowerRightCorner;	
}

#pragma mark -
-(UIImage *)extractLeftEdgeForStretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"extractLeftEdgeForStretchableRegion:'%@'",NSStringFromCGRect(stretchableRegion));
	UIImage *leftEdge = [self subImageInRect:CGRectMake(0.0f, CGRectGetMinY(stretchableRegion), CGRectGetMinX(stretchableRegion), CGRectGetHeight(stretchableRegion))];
	NPOOutputLog(leftEdge);
	return leftEdge;
}

-(UIImage *)extractRightEdgeForStretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"extractRightEdgeForStretchableRegion:'%@'",NSStringFromCGRect(stretchableRegion));
	UIImage *rightEdge = [self subImageInRect:CGRectMake(CGRectGetMaxX(stretchableRegion), CGRectGetMinY(stretchableRegion), [self size].width - CGRectGetMaxX(stretchableRegion), CGRectGetHeight(stretchableRegion))];
	NPOOutputLog(rightEdge);
	return rightEdge;
}

-(UIImage *)extractUpperEdgeForStretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"extractUpperEdgeForStretchableRegion:'%@'",NSStringFromCGRect(stretchableRegion));
	UIImage *upperEdge = [self subImageInRect:CGRectMake(CGRectGetMinX(stretchableRegion), 0.0f, CGRectGetWidth(stretchableRegion), CGRectGetMinY(stretchableRegion))];
	NPOOutputLog(upperEdge);
	return upperEdge;
}

-(UIImage *)extractLowerEdgeForStretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"extractLowerEdgeForStretchableRegion:'%@'",NSStringFromCGRect(stretchableRegion));
	UIImage *lowerEdge = [self subImageInRect:CGRectMake(CGRectGetMinX(stretchableRegion), CGRectGetMaxY(stretchableRegion), CGRectGetWidth(stretchableRegion), [self size].height - CGRectGetMaxY(stretchableRegion))];
	NPOOutputLog(center);
	return lowerEdge;
}

#pragma mark -
-(UIImage *)extractCenterForStretchableRegion:(CGRect)stretchableRegion {
	NPAInputLog(@"extractCenterForStretchableRegion:'%@'",NSStringFromCGRect(stretchableRegion));
	UIImage *center = [self subImageInRect:stretchableRegion];
	NPOOutputLog(center);
	return center;
}


@end
