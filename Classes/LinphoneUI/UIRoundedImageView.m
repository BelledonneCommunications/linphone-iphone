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

INIT_WITH_COMMON {
	[self setRoundRadius];
	[self setBordered:YES];
}

- (void)setImage:(UIImage *)image {
	[self setImage:image bordered:NO withRoundedRadius:TRUE];
}

- (void)setImage:(UIImage *)image bordered:(BOOL)bordered withRoundedRadius:(BOOL)rounded {
	[super setImage:image];
	[self setBordered:bordered];
	[self setRoundRadius];
}

- (void)setBordered:(BOOL)bordered {
	if (bordered) {
		self.layer.borderWidth = 10;
		self.layer.borderColor = [UIColor colorWithPatternImage:[UIImage imageNamed:@"color_A.png"]].CGColor;
	} else {
		self.layer.borderWidth = 0;
	}
}

- (void)setRoundRadius {
	CALayer *imageLayer = self.layer;

	CGFloat height = self.frame.size.height;
	CGFloat width = self.frame.size.width;
	CGFloat roundRadius = MIN(width, height) / 2;

	CGRect frame = self.frame;
	frame.size.width = frame.size.height = MIN(width, height);
	self.bounds = frame;

	[imageLayer setCornerRadius:roundRadius];
}

@end
