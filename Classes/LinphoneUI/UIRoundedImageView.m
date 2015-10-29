//
//  UIRoundedImageView.m
//  linphone
//
//  Created by guillaume on 13/05/2014.
//
//

#import "UIRoundedImageView.h"
#import <QuartzCore/QuartzCore.h>
#import "Utils.h"

@implementation UIRoundedImageView

- (id)init {
	self = [super init];
	if (self) {
		[self setRoundRadius:TRUE];
		[self setBordered:NO];
	}
	return self;
}

- (void)setImage:(UIImage *)image {
	[self setImage:image bordered:NO withRoundedRadius:TRUE];
}

- (void)setImage:(UIImage *)image bordered:(BOOL)bordered withRoundedRadius:(BOOL)rounded {
	[super setImage:image];
	if (image.size.width != image.size.height) {
		LOGI(@"Image is not squared (%fx%f) - cropping it", image.size.width, image.size.height);
	}
	[self setBordered:bordered];
	[self setRoundRadius:rounded];
}

- (void)setBordered:(BOOL)bordered {
	if (bordered) {
		self.layer.borderWidth = 10;
		self.layer.borderColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A.png"]].CGColor;
	} else {
		self.layer.borderWidth = 0;
	}
}
// warning: for non-squared image, this function will generate an ellipsoidal image, not a round image!
- (void)setRoundRadius:(BOOL)radius {
	CALayer *imageLayer = self.layer;
	CGFloat height = imageLayer.frame.size.height;
	CGFloat width = imageLayer.frame.size.width;
	CGFloat roundRadius = height > width ? width / 2 : height / 2;

	[imageLayer setCornerRadius:roundRadius];
	[imageLayer setMasksToBounds:YES];
}

@end
